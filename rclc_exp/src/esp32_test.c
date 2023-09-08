#include <std_msgs/msg/int32.h>
#include "custom_interfaces/msg/message.h"
#include <stdlib.h>
#include "my_node.h"
#define NODE1_PUBLISHER_NUMBER 1
#define NODE1_SUBSCRIBER_NUMBER 1
#define NODE1_TIMER_NUMBER 1

#define TOPIC_NUMBER 1

volatile rcl_time_point_value_t start_time;
my_node_t * node1;

rclc_executor_semantics_t semantics;

void timer_callback_print1(
  my_node_t * node, 
  int timer_index, 
  int pub_index, 
  int min_run_time_ms,
  int max_run_time_ms,
  rclc_executor_semantics_t pub_semantics)
{
  std_msgs__msg__Int32 pub_msg;
  rcl_time_point_value_t now = rclc_now(&support);
  pub_msg.data = node->count[timer_index]++;
  printf("Timer %lu %ld %ld\n", (unsigned long) &node->timer[timer_index], pub_msg.data, now);
  busy_wait_random(min_run_time_ms, max_run_time_ms);
  RCSOFTCHECK(rclc_publish(&node->publisher[pub_index], &pub_msg, NULL, pub_semantics));
  now = rclc_now(&support);
  printf("Timer %lu %ld %ld\n", (unsigned long) &node->timer[timer_index], pub_msg.data, now);
}

void subscriber_callback_print1(
  my_node_t * node, 
  const std_msgs__msg__Int32 * msg,
  int sub_index,
  int pub_index,
  int min_run_time_ms,
  int max_run_time_ms,
  rclc_executor_semantics_t pub_semantics)
{
  rcl_time_point_value_t now;
  now = rclc_now(&support);
  printf("Subscriber %lu %ld %ld\n", (unsigned long) &node->subscriber[sub_index], msg->data, now);
  busy_wait_random(min_run_time_ms, max_run_time_ms);
  now = rclc_now(&support);
  if (pub_index >= 0)
  {
    // RCSOFTCHECK(rclc_publish(&node->publisher[pub_index], msg, NULL, pub_semantics));
    printf("Subscriber %lu %ld %ld\n", (unsigned long) &node->subscriber[sub_index], msg->data, now); 
  }
}

/***************************** CALLBACKS ***********************************/
void node1_timer1_callback(rcl_timer_t * timer, int64_t last_call_time)
{
  RCLC_UNUSED(last_call_time);
  if (timer == NULL)
  {
    printf("timer_callback Error: timer parameter is NULL\n");
    return;
  }
  int timer_index = 0;
  int pub_index = 0;
  timer_callback_print1(node1, timer_index, pub_index, 1, 2, semantics);
}

void node1_subscriber1_callback(const void * msgin)
{
  if (msgin == NULL) {
    printf("Callback: msg NULL\n");
    return;
  }
  const std_msgs__msg__Int32 * msg = (const std_msgs__msg__Int32 *)msgin;
  int sub_index = 0;
  int pub_index = -1;
  int min_run_time_ms = 5;
  int max_run_time_ms = 15;
  rclc_executor_semantics_t pub_semantics = semantics;
  subscriber_callback_print1(node1, msg, sub_index, pub_index, min_run_time_ms, max_run_time_ms, pub_semantics);
}

/******************** MAIN PROGRAM ****************************************/
int main(int argc, char const *argv[])
{
    unsigned int timer_period = 100;
    unsigned int executor_period_input = 100;
    unsigned int experiment_duration = 10000;
    bool let = false;

    parse_user_arguments(argc, argv, &executor_period_input, &timer_period, &experiment_duration, &let);

    rcl_allocator_t allocator;
    rcl_allocator_state_t alloc_state = {0,0};
    allocator.state = &alloc_state;
    allocator.allocate = my_allocate;
    allocator.deallocate = my_deallocate;
    allocator.reallocate = my_reallocate;
    allocator.zero_allocate = my_zero_allocate;

    rcl_allocator_t allocator1;
    rcl_allocator_state_t alloc1_state = {0,0};
    allocator1.state = &alloc1_state;
    allocator1.allocate = my_allocate;
    allocator1.deallocate = my_deallocate;
    allocator1.reallocate = my_reallocate;
    allocator1.zero_allocate = my_zero_allocate;

    node1 = create_node(NODE1_TIMER_NUMBER, NODE1_PUBLISHER_NUMBER, NODE1_SUBSCRIBER_NUMBER, &allocator);

    node1->timer_callback[0] = &node1_timer1_callback;

    node1->subscriber_callback[0] = &node1_subscriber1_callback;

    srand(time(NULL));
    exit_flag = false;
    semantics = (let) ? LET : RCLCPP_EXECUTOR;
    const uint64_t timer_timeout_ns[NODE1_TIMER_NUMBER] = {RCL_MS_TO_NS(timer_period)};

    // create init_options
    RCCHECK(rclc_support_init(&support, argc, argv, &allocator));

    // create rcl_node
    init_node(node1, &support, "node_1");

    char ** node1_pub_topic_name = create_topic_name_array(NODE1_PUBLISHER_NUMBER, &allocator);

    char ** node1_sub_topic_name = create_topic_name_array(NODE1_SUBSCRIBER_NUMBER, &allocator);


    sprintf(node1_pub_topic_name[0], "esp_sub");        
    sprintf(node1_sub_topic_name[0], "esp_2");

    const rosidl_message_type_support_t * my_type_support =
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32);  
    
    // Setting the DDS QoS profile to have buffer depth = 1
    rmw_qos_profile_t profile = rmw_qos_profile_sensor_data;
    profile.depth = 1;
    // Init node 1
    init_node_timer(node1, &support, timer_timeout_ns);
    init_node_publisher(node1, my_type_support, node1_pub_topic_name, &profile, semantics);
    init_node_subscriber(node1, my_type_support, node1_sub_topic_name, &profile);

    ////////////////////////////////////////////////////////////////////////////
    // Configuration of RCL Executor
    ////////////////////////////////////////////////////////////////////////////
    const uint64_t timeout_ns = 0;
    
    const int num_executor = 1;
    
    rclc_executor_t * executor = malloc(num_executor*sizeof(rclc_executor_t));
    if(executor == NULL)
      printf("Fail to allocate memory for executor\n");

    rclc_executor_semantics_t * executor_semantics = malloc(num_executor*sizeof(rclc_executor_semantics_t));
    if(executor_semantics == NULL)
      printf("Fail to allocate memory for executor semantics\n");
    
    rcutils_time_point_value_t * callback_let_timer1 = create_time_array(NODE1_TIMER_NUMBER, &allocator);
    rcutils_time_point_value_t * callback_let_subscriber1 = create_time_array(NODE1_SUBSCRIBER_NUMBER, &allocator);

    callback_let_timer1[0] = RCUTILS_MS_TO_NS(10);
    callback_let_subscriber1[0] = RCUTILS_MS_TO_NS(20);

    unsigned int num_handles = 2;
    
    rcutils_time_point_value_t * executor_period = create_time_array(num_executor, &allocator);
    executor_period[0] = RCUTILS_MS_TO_NS(100);

    executor_semantics[0] = semantics;

    const int max_number_per_callback = 3; // Max number of calls per publisher per callback
    const int num_let_handles = 1; // max number of let handles per callback

    int i;
    for (i = 0; i < num_executor; i++)
    {
      executor[i] = rclc_executor_get_zero_initialized_executor();
      RCCHECK(rclc_executor_init(&executor[i], &support.context, num_handles, &allocator1)); 
      RCCHECK(rclc_executor_let_init(&executor[i], num_let_handles, CANCEL_NEXT_PERIOD));
      RCCHECK(rclc_executor_set_semantics(&executor[i], executor_semantics[i]));
      RCCHECK(rclc_executor_set_period(&executor[i], executor_period[i]));
      RCCHECK(rclc_executor_set_timeout(&executor[i],timeout_ns));
      printf("ExecutorID Executor%d %lu\n", i+1, (unsigned long) &executor[i]);
    }

    for (i = 0; i < NODE1_TIMER_NUMBER; i++)
    {
      RCCHECK(rclc_executor_add_timer(&executor[0], &node1->timer[i], callback_let_timer1[i]));
    }
    for (i = 0; i < NODE1_SUBSCRIBER_NUMBER; i++)
    {
      RCCHECK(rclc_executor_add_subscription(
        &executor[0], &node1->subscriber[i], &node1->sub_msg[i], node1->subscriber_callback[i],
        ON_NEW_DATA, callback_let_subscriber1[i], sizeof(std_msgs__msg__Int32)));
    }

    if (let)
    {
      for (i = 0; i < NODE1_PUBLISHER_NUMBER; i++)
      {
        RCCHECK(rclc_executor_add_publisher_LET(&executor[0], &node1->publisher[i], sizeof(std_msgs__msg__Int32),
          max_number_per_callback, &node1->timer[0], RCLC_TIMER));
      }
    }

    printf("PublisherID Publisher1 %lu\n", (unsigned long) &node1->publisher[0]);
    printf("SubscriberID Subscriber1 %lu\n", (unsigned long) &node1->subscriber[0]);
    printf("TimerID Timer1 %lu\n", (unsigned long) &node1->timer[0]);
    printf("Executor1 max size %zu current size %zu\n", alloc1_state.max_memory_size, alloc1_state.current_memory_size);
    ////////////////////////////////////////////////////////////////////////////
    // Configuration of Linux threads
    ////////////////////////////////////////////////////////////////////////////
    pthread_t thread1 = 0;
    int policy = SCHED_FIFO;
    rcl_time_point_value_t now = rclc_now(&support);
    printf("StartTime %ld\n", now);

    if (executor_period_input > 0)
    {
        struct arg_spin_period ex1 = {executor_period[0], &executor[0], &support};

        thread_create(&thread1, policy, 49, 0, rclc_executor_spin_period_with_exit_wrapper, &ex1);
        sleep_ms(2);
    }
    else
    {
        thread_create(&thread1, policy, 49, 0, rclc_executor_spin_wrapper, &executor[0]);
    }

    sleep_ms(experiment_duration);
    exit_flag = true;
    printf("Finish experiment\n");

    // Wait for threads to finish
    pthread_join(thread1, NULL);


    // clean up 
    for (i = 0; i < num_executor; i++)
    {
      if (let)
        RCCHECK(rclc_executor_let_fini(&executor[i]));
      RCCHECK(rclc_executor_fini(&executor[i]));
    }

    destroy_node(node1, &allocator);

    RCCHECK(rclc_support_fini(&support));  

    destroy_topic_name_array(node1_pub_topic_name, NODE1_PUBLISHER_NUMBER, &allocator);

    destroy_topic_name_array(node1_sub_topic_name, NODE1_SUBSCRIBER_NUMBER, &allocator);

    destroy_time_array(callback_let_timer1, &allocator);

    destroy_time_array(callback_let_subscriber1, &allocator);

    destroy_time_array(executor_period, &allocator);
    free(executor);
    free(executor_semantics);
 
    return 0;
}