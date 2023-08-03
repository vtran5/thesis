#include "custom_interfaces/msg/message.h"
#include "utilities.h"
#include <stdlib.h>
#include "my_node.h"

volatile rcl_time_point_value_t start_time;
my_node_t * node1;
my_node_t * node2;
my_node_t * node3;
my_node_t * node4;

rclc_executor_semantics_t semantics;

void node_timer_callback(rcl_timer_t * timer, void * context)
{
  timer_callback_context_t * context_ptr = (timer_callback_context_t *) context;
  if (timer == NULL)
  {
    printf("timer_callback Error: timer parameter is NULL\n");
    return;
  }
  custom_interfaces__msg__Message pub_msg;
  rcl_time_point_value_t now = rclc_now(&support);
  pub_msg.frame_id = context_ptr->node->count[context_ptr->timer_index]++;
  pub_msg.stamp = now;
  printf("Timer %lu %ld %ld\n", (unsigned long) &context_ptr->node->timer[context_ptr->timer_index], pub_msg.frame_id, now);
  busy_wait_random(context_ptr->min_execution_time_ms, context_ptr->max_execution_time_ms);
  if (context_ptr->pub_index >= 0)
  {
    RCSOFTCHECK(rclc_publish(&context_ptr->node->publisher[context_ptr->pub_index], &pub_msg, NULL, semantics));
  }
  now = rclc_now(&support);
  printf("Timer %lu %ld %ld\n", (unsigned long) &context_ptr->node->timer[context_ptr->timer_index], pub_msg.frame_id, now);
}

void node_subscriber_callback(const void * msgin, void * context)
{ 
  if (msgin == NULL) {
    printf("Callback: msg NULL\n");
    return;
  }
  subscriber_callback_context_t * context_ptr = (subscriber_callback_context_t *) context;
  const custom_interfaces__msg__Message * msg = (const custom_interfaces__msg__Message *)msgin;
  rcl_time_point_value_t now;
  now = rclc_now(&support);
  printf("Subscriber %lu %ld %ld\n", (unsigned long) &context_ptr->node->subscriber[context_ptr->sub_index], msg->frame_id, now);
  busy_wait_random(context_ptr->min_execution_time_ms, context_ptr->max_execution_time_ms);
  now = rclc_now(&support);
  if (context_ptr->pub_index >= 0)
  {
    RCSOFTCHECK(rclc_publish(&context_ptr->node->publisher[context_ptr->pub_index], msg, NULL, semantics));
    printf("Subscriber %lu %ld %ld\n", (unsigned long) &context_ptr->node->subscriber[context_ptr->sub_index], msg->frame_id, now);    
  }
}

void generate_topic_name(char* buffer, int node_num, int pub_num) {
  snprintf(buffer, 10*sizeof(char), "topic%02d%02d", node_num, pub_num);
}

void initialize_callbacks_and_topics(my_node_t **nodes, int num_nodes)
{
  for (int i = 0; i < num_nodes; i++)
  { 
    int j = 0;
    for (j = 0; j < nodes[i]->timer_num; j++)
    {
      nodes[i]->timer_callback[j] = &node_timer_callback;
    }
    for (j = 0; j < nodes[i]->sub_num; j++)
    {
      nodes[i]->subscriber_callback[j] = &node_subscriber_callback;
    }

    nodes[i]->pub_topic_name = create_topic_name_array(nodes[i]->pub_num);
    
    if (i == num_nodes -1)
      return;
    nodes[i+1]->sub_topic_name = create_topic_name_array(nodes[i+1]->sub_num);  
    // Last node doesn't publish to any node
    my_node_t *current_node = nodes[i];
    my_node_t *next_node = nodes[i + 1]; // Next node in the array

    // Assuming each node has the same number of publishers, subscribers, timers etc.
    for (j = 0; j < current_node->pub_num; j++)
    {
      // Create topic name for the current publisher in the current node
      generate_topic_name(current_node->pub_topic_name[j], i + 1, j + 1);
      // The next node's subscriber should listen to the current node's publisher topic
      strcpy(next_node->sub_topic_name[j], current_node->pub_topic_name[j]);
      // Link timer/subscriber callback to publisher in current node
      if (j < current_node->timer_num)
        current_node->callback[j] = (callback_t){RCLC_TIMER_WITH_CONTEXT, (void *)&current_node->timer[j]};
      else
        current_node->callback[j] = (callback_t){RCLC_SUBSCRIPTION, (void *) &current_node->subscriber[j-current_node->timer_num]};
    }
  }
}

void initialize_node_entities(
  my_node_t ** nodes, 
  int timer_period, 
  int num_nodes, 
  const rosidl_message_type_support_t *  my_type_support,
  rmw_qos_profile_t * profile)
{
  for(int i = 0; i < num_nodes; i++)
  {
    uint64_t * timer_timeout_ns = malloc(nodes[i]->timer_num * sizeof(uint64_t));
    if (timer_timeout_ns == NULL)
      printf("Allocate memory failed\n");
    for (int j = 0; j < nodes[i]->timer_num; j++)
    {
      timer_timeout_ns[j] = RCL_MS_TO_NS(timer_period);
    }
    init_node_timer(nodes[i], &support, timer_timeout_ns);
    init_node_subscriber(nodes[i], my_type_support, nodes[i]->sub_topic_name, profile);
    init_node_publisher(nodes[i], my_type_support, nodes[i]->pub_topic_name, profile, semantics);
    free(timer_timeout_ns);
  }
}

void add_entities_to_executor(
  my_node_t ** nodes,
  rclc_executor_t * executor,
  int num_nodes,
  uint64_t callback_let,
  int min_execution_time_ms,
  int max_execution_time_ms,
  int max_call_num_per_callback
)
{
  for (int i = 0; i < num_nodes; i++)
  {
    int j;
    nodes[i]->sub_context = malloc(nodes[i]->sub_num*sizeof(subscriber_callback_context_t));
    nodes[i]->timer_context = malloc(nodes[i]->timer_num*sizeof(timer_callback_context_t));
    for (j = 0; j < nodes[i]->sub_num; j++)
    {
      if (nodes[i]->pub_num > 0)
        nodes[i]->sub_context[j].pub_index = j + nodes[i]->timer_num;
      else
        nodes[i]->sub_context[j].pub_index = -1;
      nodes[i]->sub_context[j].sub_index = j;
      nodes[i]->sub_context[j].node = nodes[i];
      nodes[i]->sub_context[j].min_execution_time_ms = min_execution_time_ms;
      nodes[i]->sub_context[j].max_execution_time_ms = max_execution_time_ms;
      RCCHECK(rclc_executor_add_subscription_with_context(
        &executor[i], &nodes[i]->subscriber[j], &nodes[i]->sub_msg[j], nodes[i]->subscriber_callback[j],
        (void *) &nodes[i]->sub_context[j], ON_NEW_DATA, callback_let, (int) sizeof(custom_interfaces__msg__Message)));
    }

    for (j = 0; j < nodes[i]->timer_num; j++)
    {
      if (nodes[i]->pub_num > 0)
        nodes[i]->timer_context[j].pub_index = j;
      else
        nodes[i]->timer_context[j].pub_index = -1;
      nodes[i]->timer_context[j].timer_index = j;
      nodes[i]->timer_context[j].node = nodes[i];
      nodes[i]->timer_context[j].min_execution_time_ms = min_execution_time_ms;
      nodes[i]->timer_context[j].max_execution_time_ms = max_execution_time_ms;
      RCCHECK(rclc_executor_add_timer_with_context(&executor[i], &nodes[i]->timer[j], nodes[i]->timer_callback[j],
        (void *) &nodes[i]->timer_context[j], callback_let));
    }

    if (semantics == LET)
    {
      for (j = 0; j < nodes[i]->pub_num; j++)
      {
        RCCHECK(rclc_executor_add_publisher_LET(&executor[i], &nodes[i]->publisher[j], sizeof(custom_interfaces__msg__Message),
          max_call_num_per_callback, nodes[i]->callback[j].handle_ptr, nodes[i]->callback[j].type));       
      }
    }
  }
}
/******************** MAIN PROGRAM ****************************************/
int main(int argc, char const *argv[])
{
  if (argc != 2) {
    printf("Usage: %s <config_file>\n", argv[0]);
    return 1;
  }

  FILE *file = fopen(argv[1], "r");
  if (!file) {
    printf("Error: Could not open file %s\n", argv[1]);
    return 1;
  }

  int num_nodes, executor_period, message_size, callback_let, timer_period;
  fscanf(file, "nodes: %d\n", &num_nodes);
  fscanf(file, "executor_period: %d\n", &executor_period);
  fscanf(file, "timer_period: %d\n", &timer_period);
  fscanf(file, "message_size: %d\n", &message_size);
  fscanf(file, "callback_let: %d\n", &callback_let);

  my_node_t ** nodes = malloc(num_nodes*sizeof(my_node_t *));

  for (int i = 0; i < num_nodes; i++) {
    int node_id, pub_num, sub_num, timer_num;
    fscanf(file, "%d: %d %d %d\n", &node_id, &pub_num, &sub_num, &timer_num);
    nodes[i] = create_node(timer_num, pub_num, sub_num);
  }

  fclose(file);

  rcl_allocator_t allocator = rcl_get_default_allocator();

  srand(time(NULL));
  exit_flag = false;
  semantics = LET;

  // create init_options
  RCCHECK(rclc_support_init(&support, argc, argv, &allocator));
// Assuming `nodes` is an array or similar container of pointers to nodes.
  for (int i = 0; i < num_nodes; i++) {
    char node_name[100];  // Assuming the node name won't exceed 99 characters.
    snprintf(node_name, sizeof(node_name), "node_%d", i + 1);  // Convert integer to string as part of node name.

    init_node(nodes[i], &support, node_name);
  }
  initialize_callbacks_and_topics(nodes, num_nodes);
  const rosidl_message_type_support_t * my_type_support =
    ROSIDL_GET_MSG_TYPE_SUPPORT(custom_interfaces, msg, Message);  
  
  // Setting the DDS QoS profile to have buffer depth = 1
  rmw_qos_profile_t profile = rmw_qos_profile_default;
  profile.depth = 1;

  initialize_node_entities(nodes, timer_period, num_nodes, my_type_support, &profile);

  ////////////////////////////////////////////////////////////////////////////
  // Configuration of RCL Executor
  ////////////////////////////////////////////////////////////////////////////
  const uint64_t timeout_ns = 0.2*executor_period;
  
  rclc_executor_t * executor = malloc(num_nodes*sizeof(rclc_executor_t));
  if(executor == NULL)
    printf("Fail to allocate memory for executor\n");
  
  unsigned int num_handles = 4; // max number of handles
  int min_execution_time_ms = 3;
  int max_execution_time_ms = 5;

  const int max_call_num_per_callback = 1; // Max number of calls per publisher per callback
  const int num_let_handles = 1; // max number of let handles per callback

  int i;
  int sub_count = 1;
  int timer_count = 1;
  int pub_count = 1;
  for (i = 0; i < num_nodes; i++)
  {
    num_handles = nodes[i]->timer_num + nodes[i]->sub_num;
    executor[i] = rclc_executor_get_zero_initialized_executor();
    RCCHECK(rclc_executor_init(&executor[i], &support.context, num_handles, &allocator)); 
    RCCHECK(rclc_executor_set_semantics(&executor[i], semantics));
    RCCHECK(rclc_executor_set_period(&executor[i], RCUTILS_MS_TO_NS(executor_period)));
    RCCHECK(rclc_executor_set_timeout(&executor[i],timeout_ns));
    printf("ExecutorID Executor%d %lu\n", i+1, (unsigned long) &executor[i]);
    print_id(nodes[i], &sub_count, &pub_count, &timer_count);
    if (semantics == LET)
    {
      RCCHECK(rclc_executor_let_init(&executor[i], num_let_handles, CANCEL_NEXT_PERIOD));
    }
  }

  add_entities_to_executor(nodes, executor, num_nodes, RCL_MS_TO_NS(callback_let),
      min_execution_time_ms, max_execution_time_ms, max_call_num_per_callback);

  ////////////////////////////////////////////////////////////////////////////
  // Configuration of Linux threads
  ////////////////////////////////////////////////////////////////////////////
  
  pthread_t * threads = malloc(num_nodes*sizeof(pthread_t));
  struct arg_spin_period * ex_args = malloc(num_nodes*sizeof(struct arg_spin_period));
  int policy = SCHED_FIFO;
  rcl_time_point_value_t now = rclc_now(&support);
  int experiment_duration_ms = 5000;
  printf("StartTime %ld\n", now);
  for (i = 0; i < num_nodes; i++)
  {
    if (executor_period > 0)
    {
      ex_args[i].period = RCL_MS_TO_NS(executor_period);
      ex_args[i].executor = &executor[i];
      ex_args[i].support = &support;
      thread_create(&threads[i], policy, 49, 0, rclc_executor_spin_period_with_exit_wrapper, &ex_args[i]);
    }
    else
    {
      thread_create(&threads[i], policy, 49, 0, rclc_executor_spin_wrapper, &executor[i]);
    }
  }

  sleep_ms(experiment_duration_ms);
  exit_flag = true;
  // Wait for threads to finish
  for (i = 0; i < num_nodes; i++)
  {
    pthread_join(threads[i], NULL);
  }

  // clean up 
  free(threads);
  free(ex_args);
  for (i = 0; i < num_nodes; i++)
  {
    RCCHECK(rclc_executor_let_fini(&executor[i]));
    RCCHECK(rclc_executor_fini(&executor[i]));
    destroy_node(nodes[i]);
    destroy_topic_name_array(nodes[i]->pub_topic_name, nodes[i]->pub_num);
    destroy_topic_name_array(nodes[i]->sub_topic_name, nodes[i]->sub_num);
    if (nodes[i]->sub_context != NULL)
    {
      free(nodes[i]->sub_context);
      nodes[i]->sub_context = NULL;
    }
    if (nodes[i]->timer_context != NULL)
    {
      free(nodes[i]->timer_context);
      nodes[i]->timer_context = NULL;
    }
    
  }

  RCCHECK(rclc_support_fini(&support));  

  free(executor);
  free(nodes);
  return 0;
}