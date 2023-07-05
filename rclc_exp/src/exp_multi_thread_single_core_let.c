#include "custom_interfaces/msg/message.h"
#include "utilities.h"
#include <stdlib.h>
#include "my_node.h"
#define NODE1_PUBLISHER_NUMBER 1
#define NODE1_SUBSCRIBER_NUMBER 0
#define NODE1_TIMER_NUMBER 1

#define NODE2_PUBLISHER_NUMBER 1
#define NODE2_SUBSCRIBER_NUMBER 1
#define NODE2_TIMER_NUMBER 0

#define NODE3_PUBLISHER_NUMBER 1
#define NODE3_SUBSCRIBER_NUMBER 1
#define NODE3_TIMER_NUMBER 0

#define NODE4_PUBLISHER_NUMBER 0
#define NODE4_SUBSCRIBER_NUMBER 1
#define NODE4_TIMER_NUMBER 0

#define TOPIC_NUMBER 3

#define LOGGER_DIM0 6


rclc_support_t support;
volatile rcl_time_point_value_t start_time;
my_node_t * node1;
my_node_t * node2;
my_node_t * node3;
my_node_t * node4;

rclc_executor_semantics_t semantics;

char stat1[200000000];
char stat2[200000000];
char stat3[200000000];
char stat4[200000000];

/***************************** CALLBACKS ***********************************/
void timer_callback(
  my_node_t * node, 
  char * stat, 
  int timer_index, 
  int pub_index, 
  rclc_executor_semantics_t pub_semantics)
{
  char temp[1000] = "";
  custom_interfaces__msg__Message pub_msg;
  rcl_time_point_value_t now = rclc_now(&support);
  if (node->first_run)
  {
    start_time = now;
    node->first_run = false;
    sprintf(temp, "StartTime %ld\n", start_time);
    strcat(stat,temp);
  }
  pub_msg.frame_id = node->count[0]++;
  pub_msg.stamp = now;
  sprintf(temp, "Timer %lu %ld %ld\n", (unsigned long) &node->timer[timer_index], pub_msg.frame_id, now);
  strcat(stat,temp);
  RCSOFTCHECK(rclc_publish(&node->publisher[pub_index], &pub_msg, NULL, pub_semantics));
}

void subscriber_callback(
  my_node_t * node, 
  char * stat,
  custom_interfaces__msg__Message * msg,
  int sub_index,
  int pub_index,
  int min_run_time_ms,
  int max_run_time_ms,
  rclc_executor_semantics_t pub_semantics)
{
  char temp[1000] = "";  
  rcl_time_point_value_t now;
  now = rclc_now(&support);
  sprintf(temp, "Subscriber %lu %ld %ld\n", (unsigned long) &node->subscriber[sub_index], msg->frame_id, now);
  strcat(stat,temp);
  busy_wait_random(min_run_time_ms, max_run_time_ms);
  now = rclc_now(&support);
  if (pub_index >= 0)
  {
    RCSOFTCHECK(rclc_publish(&node->publisher[pub_index], msg, NULL, pub_semantics));
    sprintf(temp, "Subscriber %lu %ld %ld\n", (unsigned long) &node->subscriber[sub_index], msg->frame_id, now);
    strcat(stat,temp);     
  }
}

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
  rclc_executor_semantics_t pub_semantics = semantics;
  timer_callback(node1, stat1, timer_index, pub_index, semantics);
}

void node2_subscriber1_callback(const void * msgin)
{
  if (msgin == NULL) {
    printf("Callback: msg NULL\n");
    return;
  }
  const custom_interfaces__msg__Message * msg = (const custom_interfaces__msg__Message *)msgin;
  int sub_index = 0;
  int pub_index = 0;
  int min_run_time_ms = 5;
  int max_run_time_ms = 15;
  rclc_executor_semantics_t pub_semantics = semantics;
  subscriber_callback(node2, stat2, msg, sub_index, pub_index, min_run_time_ms, max_run_time_ms, pub_semantics);
}

void node3_subscriber1_callback(const void * msgin)
{
  if (msgin == NULL) {
    printf("Callback: msg NULL\n");
    return;
  }
  const custom_interfaces__msg__Message * msg = (const custom_interfaces__msg__Message *)msgin;
  int sub_index = 0;
  int pub_index = 0;
  int min_run_time_ms = 5;
  int max_run_time_ms = 20;
  rclc_executor_semantics_t pub_semantics = semantics;
  subscriber_callback(node3, stat3, msg, sub_index, pub_index, min_run_time_ms, max_run_time_ms, pub_semantics);
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
  subscriber_callback(node4, stat4, msg, sub_index, pub_index, min_run_time_ms, max_run_time_ms, pub_semantics);
}

/******************** MAIN PROGRAM ****************************************/
int main(int argc, char const *argv[])
{
    unsigned int timer_period = 100;
    unsigned int executor_period = 100;
    unsigned int experiment_duration = 10000;
    bool let = false;

    parse_user_arguments(argc, argv, &executor_period, &timer_period, &experiment_duration, &let);

    rcl_allocator_t allocator = rcl_get_default_allocator();
    node1 = create_node(NODE1_TIMER_NUMBER, NODE1_PUBLISHER_NUMBER, NODE1_SUBSCRIBER_NUMBER, 1);
    node2 = create_node(NODE2_TIMER_NUMBER, NODE2_PUBLISHER_NUMBER, NODE2_SUBSCRIBER_NUMBER, 0);
    node3 = create_node(NODE3_TIMER_NUMBER, NODE3_PUBLISHER_NUMBER, NODE3_SUBSCRIBER_NUMBER, 0);
    node4 = create_node(NODE4_TIMER_NUMBER, NODE4_PUBLISHER_NUMBER, NODE4_SUBSCRIBER_NUMBER, 0);

    node1->timer_callback[0] = &node1_timer1_callback;

    node2->subscriber_callback[0] = &node2_subscriber1_callback;
    node3->subscriber_callback[0] = &node3_subscriber1_callback;
    node4->subscriber_callback[0] = &node4_subscriber1_callback;

    srand(time(NULL));
    exit_flag = false;
    semantics = (let) ? LET : RCLCPP_EXECUTOR;
    const uint64_t timer_timeout_ns[NODE1_TIMER_NUMBER] = {RCL_MS_TO_NS(timer_period)};

    // create init_options
    RCCHECK(rclc_support_init(&support, argc, argv, &allocator));

    // create rcl_node
    init_node(node1, &support, "node_1");
    init_node(node2, &support, "node_2");
    init_node(node3, &support, "node_3");
    init_node(node4, &support, "node_4");

    char ** node1_pub_topic_name = NULL;
    char ** node2_pub_topic_name = NULL;
    char ** node3_pub_topic_name = NULL;
    char ** node4_pub_topic_name = NULL;

    char ** node1_sub_topic_name = NULL;
    char ** node2_sub_topic_name = NULL;
    char ** node3_sub_topic_name = NULL;
    char ** node4_sub_topic_name = NULL;

    node1_pub_topic_name = create_topic_name_array(NODE1_PUBLISHER_NUMBER);
    node2_pub_topic_name = create_topic_name_array(NODE2_PUBLISHER_NUMBER);
    node3_pub_topic_name = create_topic_name_array(NODE3_PUBLISHER_NUMBER);
    node4_pub_topic_name = create_topic_name_array(NODE4_PUBLISHER_NUMBER);

    node1_sub_topic_name = create_topic_name_array(NODE1_SUBSCRIBER_NUMBER);
    node2_sub_topic_name = create_topic_name_array(NODE2_SUBSCRIBER_NUMBER);
    node3_sub_topic_name = create_topic_name_array(NODE3_SUBSCRIBER_NUMBER);
    node4_sub_topic_name = create_topic_name_array(NODE4_SUBSCRIBER_NUMBER);


    sprintf(node1_pub_topic_name[0], "topic01");        
    sprintf(node2_sub_topic_name[0], "topic01");
    sprintf(node2_pub_topic_name[0], "topic02");
    sprintf(node3_sub_topic_name[0], "topic02");
    sprintf(node3_pub_topic_name[0], "topic03");
    sprintf(node4_sub_topic_name[0], "topic03");

    const rosidl_message_type_support_t * my_type_support =
      ROSIDL_GET_MSG_TYPE_SUPPORT(custom_interfaces, msg, Message);  
    
    // Setting the DDS QoS profile to have buffer depth = 1
    rmw_qos_profile_t profile = rmw_qos_profile_default;
    profile.depth = 1;
    // Init node 1
    init_node_timer(node1, &support, timer_timeout_ns);
    init_node_publisher(node1, my_type_support, node1_pub_topic_name, &profile);

    // Init node 2
    init_node_subscriber(node2, my_type_support, node2_sub_topic_name, &profile);
    init_node_publisher(node2, my_type_support, node2_pub_topic_name, &profile);

    // Init node 3
    init_node_subscriber(node3, my_type_support, node3_sub_topic_name, &profile);
    init_node_publisher(node3, my_type_support, node3_pub_topic_name, &profile);

    // Init node 4
    init_node_subscriber(node4, my_type_support, node4_sub_topic_name, &profile);

    ////////////////////////////////////////////////////////////////////////////
    // Configuration of RCL Executor
    ////////////////////////////////////////////////////////////////////////////
    const uint64_t timeout_ns = 0.2*executor_period;
    
    rclc_executor_t executor1;
    rclc_executor_t executor2;
    rclc_executor_t executor3;
    rclc_executor_t executor4;
    executor1 = rclc_executor_get_zero_initialized_executor();
    executor2 = rclc_executor_get_zero_initialized_executor();
    executor3 = rclc_executor_get_zero_initialized_executor();
    executor4 = rclc_executor_get_zero_initialized_executor();
    
    rcutils_time_point_value_t callback_let1 = RCUTILS_MS_TO_NS(10);
    rcutils_time_point_value_t callback_let2 = RCUTILS_MS_TO_NS(20);
    rcutils_time_point_value_t callback_let3 = RCUTILS_MS_TO_NS(40);
    rcutils_time_point_value_t callback_let4 = RCUTILS_MS_TO_NS(40);
    unsigned int num_handles = 1;
    
    const rcutils_time_point_value_t period1 = RCUTILS_MS_TO_NS(20);
    const rcutils_time_point_value_t period2 = RCUTILS_MS_TO_NS(50);
    const rcutils_time_point_value_t period3 = RCUTILS_MS_TO_NS(50);
    const rcutils_time_point_value_t period4 = RCUTILS_MS_TO_NS(50);
    const int max_number_per_callback = 3; // Max number of calls per publisher per callback
    const int num_let_handles = 1; // max number of let handles per callback
    const int max_intermediate_handles = 5;

    RCCHECK(rclc_executor_init(&executor1, &support.context, num_handles, &allocator));
    RCCHECK(rclc_executor_init(&executor2, &support.context, num_handles, &allocator));
    RCCHECK(rclc_executor_init(&executor3, &support.context, num_handles, &allocator));
    RCCHECK(rclc_executor_init(&executor4, &support.context, num_handles, &allocator));

    if(let)
    {
      RCCHECK(rclc_executor_let_init(&executor1, num_let_handles, max_intermediate_handles, CANCEL_NEXT_PERIOD));
      RCCHECK(rclc_executor_let_init(&executor2, num_let_handles, max_intermediate_handles, CANCEL_NEXT_PERIOD));
      RCCHECK(rclc_executor_let_init(&executor3, num_let_handles, max_intermediate_handles, CANCEL_NEXT_PERIOD));
      RCCHECK(rclc_executor_let_init(&executor4, num_let_handles, max_intermediate_handles, CANCEL_NEXT_PERIOD));

      RCCHECK(rclc_executor_set_semantics(&executor1, LET));
      RCCHECK(rclc_executor_set_semantics(&executor2, LET));
      RCCHECK(rclc_executor_set_semantics(&executor3, LET));
      RCCHECK(rclc_executor_set_semantics(&executor4, LET));
    }
    else
    {
      RCCHECK(rclc_executor_set_semantics(&executor1, RCLCPP_EXECUTOR));
      RCCHECK(rclc_executor_set_semantics(&executor2, RCLCPP_EXECUTOR));
      RCCHECK(rclc_executor_set_semantics(&executor3, RCLCPP_EXECUTOR));
      RCCHECK(rclc_executor_set_semantics(&executor4, RCLCPP_EXECUTOR));      
    }

    RCCHECK(rclc_executor_set_period(&executor1, period1));
    RCCHECK(rclc_executor_set_period(&executor2, period2));
    RCCHECK(rclc_executor_set_period(&executor3, period3));
    RCCHECK(rclc_executor_set_period(&executor4, period4));
    int i;
    for (i = 0; i < NODE1_TIMER_NUMBER; i++)
    {
      RCCHECK(rclc_executor_add_timer(&executor1, &node1->timer[i], callback_let1));
    }
    for (i = 0; i < NODE2_SUBSCRIBER_NUMBER; i++)
    {
      RCCHECK(rclc_executor_add_subscription(
        &executor2, &node2->subscriber[i], &node2->sub_msg[i], node2->subscriber_callback[i],
        ON_NEW_DATA, callback_let2, sizeof(custom_interfaces__msg__Message)));
    }
    for (i = 0; i < NODE3_SUBSCRIBER_NUMBER; i++)
    {
      RCCHECK(rclc_executor_add_subscription(
        &executor3, &node3->subscriber[i], &node3->sub_msg[i], node3->subscriber_callback[i],
        ON_NEW_DATA, callback_let3, sizeof(custom_interfaces__msg__Message)));
    }
    for (i = 0; i < NODE4_SUBSCRIBER_NUMBER; i++)
    {
      RCCHECK(rclc_executor_add_subscription(
        &executor4, &node4->subscriber[i], &node4->sub_msg[i], node4->subscriber_callback[i],
        ON_NEW_DATA, callback_let4, sizeof(custom_interfaces__msg__Message)));
    }
    if (let)
    {
      for (i = 0; i < NODE1_PUBLISHER_NUMBER; i++)
      {
        RCCHECK(rclc_executor_add_publisher_LET(&executor1, &node1->publisher[i],
          max_number_per_callback, &node1->timer[0], RCLC_TIMER));
      }
      for (i = 0; i < NODE2_PUBLISHER_NUMBER; i++)
      {
        RCCHECK(rclc_executor_add_publisher_LET(&executor2, &node2->publisher[i],
          max_number_per_callback, &node2->subscriber[0], RCLC_SUBSCRIPTION));
      }
      for (i = 0; i < NODE3_PUBLISHER_NUMBER; i++)
      {
        RCCHECK(rclc_executor_add_publisher_LET(&executor3, &node3->publisher[i],
          max_number_per_callback, &node3->subscriber[0], RCLC_SUBSCRIPTION));
      }
    }

    RCCHECK(rclc_executor_set_timeout(&executor1,timeout_ns));
    RCCHECK(rclc_executor_set_timeout(&executor2,timeout_ns));
    RCCHECK(rclc_executor_set_timeout(&executor3,timeout_ns));
    RCCHECK(rclc_executor_set_timeout(&executor4,timeout_ns));

    printf("ExecutorID Executor1 %lu\n", (unsigned long) &executor1);
    printf("ExecutorID Executor2 %lu\n", (unsigned long) &executor2);
    printf("ExecutorID Executor3 %lu\n", (unsigned long) &executor3);
    printf("ExecutorID Executor4 %lu\n", (unsigned long) &executor4);

    printf("PublisherID Publisher1 %lu\n", (unsigned long) &node1->publisher[0]);
    printf("PublisherID Publisher2 %lu\n", (unsigned long) &node2->publisher[0]);
    printf("PublisherID Publisher3 %lu\n", (unsigned long) &node3->publisher[0]);
    printf("SubscriberID Subscriber1 %lu\n", (unsigned long) &node2->subscriber[0]);
    printf("SubscriberID Subscriber2 %lu\n", (unsigned long) &node3->subscriber[0]);
    printf("SubscriberID Subscriber3 %lu\n", (unsigned long) &node4->subscriber[0]);
    printf("TimerID Timer1 %lu\n", (unsigned long) &node1->timer[0]);

    ////////////////////////////////////////////////////////////////////////////
    // Configuration of Linux threads
    ////////////////////////////////////////////////////////////////////////////
    pthread_t thread1 = 0;
    pthread_t thread2 = 0;
    pthread_t thread3 = 0;
    pthread_t thread4 = 0;
    int policy = SCHED_FIFO;
    rcl_time_point_value_t now = rclc_now(&support);
    printf("StartProgram %ld\n", now);

    if (executor_period > 0)
    {
        struct arg_spin_period ex1 = {period1, &executor1, &support};
        struct arg_spin_period ex2 = {period2, &executor2, &support};
        struct arg_spin_period ex3 = {period3, &executor3, &support};
        struct arg_spin_period ex4 = {period4, &executor4, &support};
        thread_create(&thread1, policy, 49, 0, rclc_executor_spin_period_with_exit_wrapper, &ex1);
        sleep_ms(2);
        thread_create(&thread2, policy, 48, 0, rclc_executor_spin_period_with_exit_wrapper, &ex2);
        sleep_ms(2);
        thread_create(&thread3, policy, 47, 0, rclc_executor_spin_period_with_exit_wrapper, &ex3);
        sleep_ms(2);
        thread_create(&thread4, policy, 46, 1, rclc_executor_spin_period_with_exit_wrapper, &ex4);
    }
    else
    {
        thread_create(&thread1, policy, 49, 0, rclc_executor_spin_wrapper, &executor1);
        thread_create(&thread1, policy, 49, 0, rclc_executor_spin_wrapper, &executor2);
        thread_create(&thread1, policy, 49, 0, rclc_executor_spin_wrapper, &executor3);
        thread_create(&thread1, policy, 49, 1, rclc_executor_spin_wrapper, &executor4);
    }


    //thread_create(&thread1, policy, 49, 0, rclc_executor_spin_wrapper, &executor1);

    sleep_ms(experiment_duration);
    exit_flag = true;
    printf("Finish experiment\n");

    // Wait for threads to finish
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    pthread_join(thread4, NULL);


    // clean up 
    RCCHECK(rclc_executor_let_fini(&executor1));
    RCCHECK(rclc_executor_let_fini(&executor2));
    RCCHECK(rclc_executor_let_fini(&executor3));
    RCCHECK(rclc_executor_let_fini(&executor4));

    RCCHECK(rclc_executor_fini(&executor1));
    RCCHECK(rclc_executor_fini(&executor2));
    RCCHECK(rclc_executor_fini(&executor3));
    RCCHECK(rclc_executor_fini(&executor4));


    destroy_node(node1);
    destroy_node(node2);
    destroy_node(node3);
    destroy_node(node4);

    RCCHECK(rclc_support_fini(&support));  

    destroy_topic_name_array(node1_pub_topic_name, NODE1_PUBLISHER_NUMBER);
    destroy_topic_name_array(node2_pub_topic_name, NODE2_PUBLISHER_NUMBER);
    destroy_topic_name_array(node3_pub_topic_name, NODE3_PUBLISHER_NUMBER);
    destroy_topic_name_array(node4_pub_topic_name, NODE4_PUBLISHER_NUMBER);

    destroy_topic_name_array(node1_sub_topic_name, NODE1_SUBSCRIBER_NUMBER);
    destroy_topic_name_array(node2_sub_topic_name, NODE2_SUBSCRIBER_NUMBER);
    destroy_topic_name_array(node3_sub_topic_name, NODE3_SUBSCRIBER_NUMBER);
    destroy_topic_name_array(node4_sub_topic_name, NODE4_SUBSCRIBER_NUMBER);

    
    printf("%s", stat1);
    printf("%s", stat2);
    printf("%s", stat3);
    printf("%s", stat4);
 
    return 0;
}
