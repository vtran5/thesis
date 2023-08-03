#include "custom_interfaces/msg/message.h"
#include "utilities.h"
#include <stdlib.h>
#include "my_node.h"
#define NODE4_PUBLISHER_NUMBER 0
#define NODE4_SUBSCRIBER_NUMBER 4
#define NODE4_TIMER_NUMBER 0

volatile rcl_time_point_value_t start_time;
my_node_t * node4;

rclc_executor_semantics_t semantics;

/***************************** CALLBACKS ***********************************/
void subscriber_callback_print(
  my_node_t * node, 
  const custom_interfaces__msg__Message * msg,
  int sub_index,
  int pub_index,
  int min_run_time_ms,
  int max_run_time_ms,
  rclc_executor_semantics_t pub_semantics)
{ 
  rcl_time_point_value_t now;
  now = rclc_now(&support);
  printf("Subscriber %lu %ld %ld\n", (unsigned long) &node->subscriber[sub_index], msg->frame_id, now);
  busy_wait_random(min_run_time_ms, max_run_time_ms);
  now = rclc_now(&support);
  if (pub_index >= 0)
  {
    RCSOFTCHECK(rclc_publish(&node->publisher[pub_index], msg, NULL, pub_semantics));
    printf("Subscriber %lu %ld %ld\n", (unsigned long) &node->subscriber[sub_index], msg->frame_id, now);   
  }
}

void node4_subscriber1_callback(const void * msgin)
{
  if (msgin == NULL) {
    printf("Callback: msg NULL\n");
    return;
  }
  const custom_interfaces__msg__Message * msg = (const custom_interfaces__msg__Message *)msgin;
  int sub_index = 0;
  int pub_index = -1;
  int min_run_time_ms = 0;
  int max_run_time_ms = 0;
  rclc_executor_semantics_t pub_semantics = semantics;
  subscriber_callback_print(node4, msg, sub_index, pub_index, min_run_time_ms, max_run_time_ms, pub_semantics);
}

void node4_subscriber2_callback(const void * msgin)
{
  if (msgin == NULL) {
    printf("Callback: msg NULL\n");
    return;
  }
  const custom_interfaces__msg__Message * msg = (const custom_interfaces__msg__Message *)msgin;
  int sub_index = 1;
  int pub_index = -1;
  int min_run_time_ms = 0;
  int max_run_time_ms = 0;
  rclc_executor_semantics_t pub_semantics = semantics;
  subscriber_callback_print(node4, msg, sub_index, pub_index, min_run_time_ms, max_run_time_ms, pub_semantics);
}

void node4_subscriber3_callback(const void * msgin)
{
  if (msgin == NULL) {
    printf("Callback: msg NULL\n");
    return;
  }
  const custom_interfaces__msg__Message * msg = (const custom_interfaces__msg__Message *)msgin;
  int sub_index = 2;
  int pub_index = -1;
  int min_run_time_ms = 0;
  int max_run_time_ms = 0;
  rclc_executor_semantics_t pub_semantics = semantics;
  subscriber_callback_print(node4, msg, sub_index, pub_index, min_run_time_ms, max_run_time_ms, pub_semantics);
}

void node4_subscriber4_callback(const void * msgin)
{
  if (msgin == NULL) {
    printf("Callback: msg NULL\n");
    return;
  }
  const custom_interfaces__msg__Message * msg = (const custom_interfaces__msg__Message *)msgin;
  int sub_index = 3;
  int pub_index = -1;
  int min_run_time_ms = 0;
  int max_run_time_ms = 0;
  rclc_executor_semantics_t pub_semantics = semantics;
  subscriber_callback_print(node4, msg, sub_index, pub_index, min_run_time_ms, max_run_time_ms, pub_semantics);
}

/******************** MAIN PROGRAM ****************************************/
int main(int argc, char const *argv[])
{
    unsigned int timer_period = 100;
    unsigned int executor_period_input = 100;
    unsigned int experiment_duration = 10000;
    bool let = false;

    parse_user_arguments(argc, argv, &executor_period_input, &timer_period, &experiment_duration, &let);

    rcl_allocator_t allocator = rcl_get_default_allocator();
    node4 = create_node(NODE4_TIMER_NUMBER, NODE4_PUBLISHER_NUMBER, NODE4_SUBSCRIBER_NUMBER);

    node4->subscriber_callback[0] = &node4_subscriber1_callback;
    node4->subscriber_callback[1] = &node4_subscriber2_callback;
    node4->subscriber_callback[2] = &node4_subscriber3_callback;
    node4->subscriber_callback[3] = &node4_subscriber4_callback;

    srand(time(NULL));
    exit_flag = false;
    semantics = (let) ? LET : RCLCPP_EXECUTOR;

    // create init_options
    RCCHECK(rclc_support_init(&support, argc, argv, &allocator));

    // create rcl_node
    init_node(node4, &support, "node_4");

    char ** node4_sub_topic_name = create_topic_name_array(NODE4_SUBSCRIBER_NUMBER);


    sprintf(node4_sub_topic_name[0], "topic07");  
    sprintf(node4_sub_topic_name[1], "topic08");
    sprintf(node4_sub_topic_name[2], "topic09");
    sprintf(node4_sub_topic_name[3], "topic10");

    const rosidl_message_type_support_t * my_type_support =
      ROSIDL_GET_MSG_TYPE_SUPPORT(custom_interfaces, msg, Message);  
    
    // Setting the DDS QoS profile to have buffer depth = 1
    rmw_qos_profile_t profile = rmw_qos_profile_default;
    profile.depth = 1;

    // Init node 4
    init_node_subscriber(node4, my_type_support, node4_sub_topic_name, &profile);

    ////////////////////////////////////////////////////////////////////////////
    // Configuration of RCL Executor
    ////////////////////////////////////////////////////////////////////////////
    const uint64_t timeout_ns = 0.2*executor_period_input;
    
    const int num_executor = 1;
    
    rclc_executor_t * executor = malloc(num_executor*sizeof(rclc_executor_t));
    if(executor == NULL)
      printf("Fail to allocate memory for executor\n");

    rclc_executor_semantics_t * executor_semantics = malloc(num_executor*sizeof(rclc_executor_semantics_t));
    if(executor_semantics == NULL)
      printf("Fail to allocate memory for executor semantics\n");
    
    rcutils_time_point_value_t * callback_let_timer4 = create_time_array(NODE4_TIMER_NUMBER);
    rcutils_time_point_value_t * callback_let_subscriber4 = create_time_array(NODE4_SUBSCRIBER_NUMBER);

    callback_let_subscriber4[0] = RCUTILS_MS_TO_NS(5);
    callback_let_subscriber4[1] = RCUTILS_MS_TO_NS(5);
    callback_let_subscriber4[2] = RCUTILS_MS_TO_NS(5);
    callback_let_subscriber4[3] = RCUTILS_MS_TO_NS(5);

    unsigned int num_handles = 4;
    
    rcutils_time_point_value_t * executor_period = create_time_array(num_executor);
    executor_period[0] = RCUTILS_MS_TO_NS(10);

    executor_semantics[0] = semantics;

    const int max_number_per_callback = 2; // Max number of calls per publisher per callback
    const int num_let_handles = 4; // max number of let handles per executor
    const int max_intermediate_handles = 20; // max number of intermediate handles per executor

    int i;
    for (i = 0; i < num_executor; i++)
    {
      executor[i] = rclc_executor_get_zero_initialized_executor();
      RCCHECK(rclc_executor_init(&executor[i], &support.context, num_handles, &allocator)); 
      RCCHECK(rclc_executor_let_init(&executor[i], num_let_handles, CANCEL_NEXT_PERIOD));
      RCCHECK(rclc_executor_set_semantics(&executor[i], executor_semantics[i]));
      RCCHECK(rclc_executor_set_period(&executor[i], executor_period[i]));
      RCCHECK(rclc_executor_set_timeout(&executor[i],timeout_ns));
      printf("ExecutorID Executor%d %lu\n", i+1, (unsigned long) &executor[i]);
    }

    for (i = 0; i < NODE4_SUBSCRIBER_NUMBER; i++)
    {
      RCCHECK(rclc_executor_add_subscription(
        &executor[0], &node4->subscriber[i], &node4->sub_msg[i], node4->subscriber_callback[i],
        ON_NEW_DATA, callback_let_subscriber4[i], (int) sizeof(custom_interfaces__msg__Message)));
    }

    int sub_count = 1;
    int timer_count = 1;
    int pub_count = 1;

    print_id(node4, &sub_count, &pub_count, &timer_count);

    ////////////////////////////////////////////////////////////////////////////
    // Configuration of Linux threads
    ////////////////////////////////////////////////////////////////////////////
    pthread_t thread = 0;
    int policy = SCHED_FIFO;
    rcl_time_point_value_t now = rclc_now(&support);
    printf("StartTime %ld\n", now);

    if (executor_period_input > 0)
    {
        struct arg_spin_period ex4 = {executor_period[0], &executor[0], &support};
        thread_create(&thread, policy, 46, 1, rclc_executor_spin_period_with_exit_wrapper, &ex4);
    }
    else
    {
        thread_create(&thread, policy, 49, 1, rclc_executor_spin_wrapper, &executor[0]);
    }

    //sleep_ms(experiment_duration);
    //exit_flag = true;
    //printf("Finish experiment\n");

    // Wait for threads to finish
    pthread_join(thread, NULL);


    // clean up 
    for (i = 0; i < num_executor; i++)
    {
      RCCHECK(rclc_executor_let_fini(&executor[i]));
      RCCHECK(rclc_executor_fini(&executor[i]));
    }

    destroy_node(node4);

    RCCHECK(rclc_support_fini(&support));  

    destroy_topic_name_array(node4_sub_topic_name, NODE4_SUBSCRIBER_NUMBER);

    destroy_time_array(callback_let_subscriber4);

    destroy_time_array(executor_period);
    free(executor);
    free(executor_semantics);

    return 0;
}
