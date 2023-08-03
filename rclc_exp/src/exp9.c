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

// Experiment with deadline violation

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
  timer_callback(node1, stat1, timer_index, pub_index, 1, 2, semantics);
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
  subscriber_callback(node2, stat2, msg, sub_index, pub_index, min_run_time_ms, max_run_time_ms, semantics);
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
  int max_run_time_ms = 60;
  subscriber_callback(node3, stat3, msg, sub_index, pub_index, min_run_time_ms, max_run_time_ms, semantics);
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
  subscriber_callback(node4, stat4, msg, sub_index, pub_index, min_run_time_ms, max_run_time_ms, semantics);
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
    node1 = create_node(NODE1_TIMER_NUMBER, NODE1_PUBLISHER_NUMBER, NODE1_SUBSCRIBER_NUMBER);
    node2 = create_node(NODE2_TIMER_NUMBER, NODE2_PUBLISHER_NUMBER, NODE2_SUBSCRIBER_NUMBER);
    node3 = create_node(NODE3_TIMER_NUMBER, NODE3_PUBLISHER_NUMBER, NODE3_SUBSCRIBER_NUMBER);
    node4 = create_node(NODE4_TIMER_NUMBER, NODE4_PUBLISHER_NUMBER, NODE4_SUBSCRIBER_NUMBER);

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

    char ** node1_pub_topic_name = create_topic_name_array(NODE1_PUBLISHER_NUMBER);
    char ** node2_pub_topic_name = create_topic_name_array(NODE2_PUBLISHER_NUMBER);
    char ** node3_pub_topic_name = create_topic_name_array(NODE3_PUBLISHER_NUMBER);
    char ** node4_pub_topic_name = create_topic_name_array(NODE4_PUBLISHER_NUMBER);

    char ** node1_sub_topic_name = create_topic_name_array(NODE1_SUBSCRIBER_NUMBER);
    char ** node2_sub_topic_name = create_topic_name_array(NODE2_SUBSCRIBER_NUMBER);
    char ** node3_sub_topic_name = create_topic_name_array(NODE3_SUBSCRIBER_NUMBER);
    char ** node4_sub_topic_name = create_topic_name_array(NODE4_SUBSCRIBER_NUMBER);


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
    init_node_publisher(node1, my_type_support, node1_pub_topic_name, &profile, semantics);

    // Init node 2
    init_node_subscriber(node2, my_type_support, node2_sub_topic_name, &profile);
    init_node_publisher(node2, my_type_support, node2_pub_topic_name, &profile, semantics);

    // Init node 3
    init_node_subscriber(node3, my_type_support, node3_sub_topic_name, &profile);
    init_node_publisher(node3, my_type_support, node3_pub_topic_name, &profile, semantics);

    // Init node 4
    init_node_subscriber(node4, my_type_support, node4_sub_topic_name, &profile);

    ////////////////////////////////////////////////////////////////////////////
    // Configuration of RCL Executor
    ////////////////////////////////////////////////////////////////////////////
    const uint64_t timeout_ns = 0.2*executor_period_input;
    
    const int num_executor = 4;
    
    rclc_executor_t * executor = malloc(num_executor*sizeof(rclc_executor_t));
    if(executor == NULL)
      printf("Fail to allocate memory for executor\n");

    rclc_executor_semantics_t * executor_semantics = malloc(num_executor*sizeof(rclc_executor_semantics_t));
    if(executor_semantics == NULL)
      printf("Fail to allocate memory for executor semantics\n");
    
    rcutils_time_point_value_t * callback_let_timer1 = create_time_array(NODE1_TIMER_NUMBER);
    rcutils_time_point_value_t * callback_let_subscriber1 = create_time_array(NODE1_SUBSCRIBER_NUMBER);
    rcutils_time_point_value_t * callback_let_timer2 = create_time_array(NODE2_TIMER_NUMBER);
    rcutils_time_point_value_t * callback_let_subscriber2 = create_time_array(NODE2_SUBSCRIBER_NUMBER);
    rcutils_time_point_value_t * callback_let_timer3 = create_time_array(NODE3_TIMER_NUMBER);
    rcutils_time_point_value_t * callback_let_subscriber3 = create_time_array(NODE3_SUBSCRIBER_NUMBER);
    rcutils_time_point_value_t * callback_let_timer4 = create_time_array(NODE4_TIMER_NUMBER);
    rcutils_time_point_value_t * callback_let_subscriber4 = create_time_array(NODE4_SUBSCRIBER_NUMBER);

    callback_let_timer1[0] = RCUTILS_MS_TO_NS(10);
    callback_let_subscriber2[0] = RCUTILS_MS_TO_NS(20);
    callback_let_subscriber3[0] = RCUTILS_MS_TO_NS(60);
    callback_let_subscriber4[0] = RCUTILS_MS_TO_NS(60);

    unsigned int num_handles = 1;
    
    rcutils_time_point_value_t * executor_period = create_time_array(num_executor);
    executor_period[0] = RCUTILS_MS_TO_NS(10);
    executor_period[1] = RCUTILS_MS_TO_NS(50);
    executor_period[2] = RCUTILS_MS_TO_NS(50);
    executor_period[3] = RCUTILS_MS_TO_NS(10);

    executor_semantics[0] = semantics;
    executor_semantics[1] = semantics;
    executor_semantics[2] = semantics;
    executor_semantics[3] = semantics;

    const int max_number_per_callback = 3; // Max number of calls per publisher per callback
    const int num_let_handles = 1; // max number of let handles per callback
    const int max_intermediate_handles = 5;

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

    for (i = 0; i < NODE1_TIMER_NUMBER; i++)
    {
      RCCHECK(rclc_executor_add_timer(&executor[0], &node1->timer[i], callback_let_timer1[i]));
    }
    for (i = 0; i < NODE2_SUBSCRIBER_NUMBER; i++)
    {
      RCCHECK(rclc_executor_add_subscription(
        &executor[1], &node2->subscriber[i], &node2->sub_msg[i], node2->subscriber_callback[i],
        ON_NEW_DATA, callback_let_subscriber2[i], sizeof(custom_interfaces__msg__Message)));
    }
    for (i = 0; i < NODE3_SUBSCRIBER_NUMBER; i++)
    {
      RCCHECK(rclc_executor_add_subscription(
        &executor[2], &node3->subscriber[i], &node3->sub_msg[i], node3->subscriber_callback[i],
        ON_NEW_DATA, callback_let_subscriber3[i], sizeof(custom_interfaces__msg__Message)));
    }
    for (i = 0; i < NODE4_SUBSCRIBER_NUMBER; i++)
    {
      RCCHECK(rclc_executor_add_subscription(
        &executor[3], &node4->subscriber[i], &node4->sub_msg[i], node4->subscriber_callback[i],
        ON_NEW_DATA, callback_let_subscriber4[i], sizeof(custom_interfaces__msg__Message)));
    }

    if (let)
    {
      for (i = 0; i < NODE1_PUBLISHER_NUMBER; i++)
      {
        RCCHECK(rclc_executor_add_publisher_LET(&executor[0], &node1->publisher[i], sizeof(custom_interfaces__msg__Message),
          max_number_per_callback, &node1->timer[0], RCLC_TIMER));
      }
      for (i = 0; i < NODE2_PUBLISHER_NUMBER; i++)
      {
        RCCHECK(rclc_executor_add_publisher_LET(&executor[1], &node2->publisher[i], sizeof(custom_interfaces__msg__Message),
          max_number_per_callback, &node2->subscriber[0], RCLC_SUBSCRIPTION));
      }
      for (i = 0; i < NODE3_PUBLISHER_NUMBER; i++)
      {
        RCCHECK(rclc_executor_add_publisher_LET(&executor[2], &node3->publisher[i], sizeof(custom_interfaces__msg__Message),
          max_number_per_callback, &node3->subscriber[0], RCLC_SUBSCRIPTION));
      }
    }

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
    printf("StartTime %ld\n", now);

    if (executor_period_input > 0)
    {
        struct arg_spin_period ex1 = {executor_period[0], &executor[0], &support};
        struct arg_spin_period ex2 = {executor_period[1], &executor[1], &support};
        struct arg_spin_period ex3 = {executor_period[2], &executor[2], &support};
        struct arg_spin_period ex4 = {executor_period[3], &executor[3], &support};
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
        thread_create(&thread1, policy, 49, 0, rclc_executor_spin_wrapper, &executor[0]);
        thread_create(&thread1, policy, 49, 0, rclc_executor_spin_wrapper, &executor[1]);
        thread_create(&thread1, policy, 49, 0, rclc_executor_spin_wrapper, &executor[2]);
        thread_create(&thread1, policy, 49, 1, rclc_executor_spin_wrapper, &executor[3]);
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
    for (i = 0; i < num_executor; i++)
    {
      RCCHECK(rclc_executor_let_fini(&executor[i]));
      RCCHECK(rclc_executor_fini(&executor[i]));
    }

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

    destroy_time_array(callback_let_timer1);
    destroy_time_array(callback_let_timer2);
    destroy_time_array(callback_let_timer3);
    destroy_time_array(callback_let_timer4);
    destroy_time_array(callback_let_subscriber1);
    destroy_time_array(callback_let_subscriber2);
    destroy_time_array(callback_let_subscriber3);
    destroy_time_array(callback_let_subscriber4);

    destroy_time_array(executor_period);
    free(executor);
    free(executor_semantics);

    printf("%s", stat1);
    printf("%s", stat2);
    printf("%s", stat3);
    printf("%s", stat4);
 
    return 0;
}
