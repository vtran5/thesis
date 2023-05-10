#include "custom_interfaces/msg/message.h"
#include "utilities.h"
#include <stdlib.h>

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


const char * topic_name[TOPIC_NUMBER];

// struct to store node variables
typedef struct my_node1
{
  rcl_node_t rcl_node;
  volatile bool first_run;
  volatile int64_t count1;
#if NODE1_PUBLISHER_NUMBER > 0
  rcl_publisher_t publisher[NODE1_PUBLISHER_NUMBER];
#endif

#if NODE1_SUBSCRIBER_NUMBER > 0
  rcl_subscription_t subscriber[NODE1_SUBSCRIBER_NUMBER];
  custom_interfaces__msg__Message sub_msg[NODE1_SUBSCRIBER_NUMBER];
  void (*subscriber_callback[NODE1_SUBSCRIBER_NUMBER])(const void * msgin);
#endif

#if NODE1_TIMER_NUMBER > 0
  rcl_timer_t timer[NODE1_TIMER_NUMBER];
  void (*timer_callback[NODE1_TIMER_NUMBER])(rcl_timer_t * timer, int64_t last_call_time);
#endif
} my_node1_t;

typedef struct my_node2
{
  rcl_node_t rcl_node;
#if NODE2_PUBLISHER_NUMBER > 0
  rcl_publisher_t publisher[NODE2_PUBLISHER_NUMBER];
#endif

#if NODE2_SUBSCRIBER_NUMBER > 0
  rcl_subscription_t subscriber[NODE2_SUBSCRIBER_NUMBER];
  custom_interfaces__msg__Message sub_msg[NODE2_SUBSCRIBER_NUMBER];
  void (*subscriber_callback[NODE2_SUBSCRIBER_NUMBER])(const void * msgin);
#endif

#if NODE2_TIMER_NUMBER > 0
  rcl_timer_t timer[NODE2_TIMER_NUMBER];
  void (*timer_callback[NODE2_TIMER_NUMBER])(rcl_timer_t * timer, int64_t last_call_time);
#endif
} my_node2_t;

typedef struct my_node3
{
  rcl_node_t rcl_node;
#if NODE3_PUBLISHER_NUMBER > 0
  rcl_publisher_t publisher[NODE3_PUBLISHER_NUMBER];
#endif

#if NODE3_SUBSCRIBER_NUMBER > 0
  rcl_subscription_t subscriber[NODE3_SUBSCRIBER_NUMBER];
  custom_interfaces__msg__Message sub_msg[NODE3_SUBSCRIBER_NUMBER];
  void (*subscriber_callback[NODE3_SUBSCRIBER_NUMBER])(const void * msgin);
#endif

#if NODE3_TIMER_NUMBER > 0
  rcl_timer_t timer[NODE3_TIMER_NUMBER];
  void (*timer_callback[NODE3_TIMER_NUMBER])(rcl_timer_t * timer, int64_t last_call_time);
#endif
} my_node3_t;

typedef struct my_node4
{
  rcl_node_t rcl_node;
#if NODE4_PUBLISHER_NUMBER > 0
  rcl_publisher_t publisher[NODE4_PUBLISHER_NUMBER];
#endif

#if NODE4_SUBSCRIBER_NUMBER > 0
  rcl_subscription_t subscriber[NODE4_SUBSCRIBER_NUMBER];
  custom_interfaces__msg__Message sub_msg[NODE4_SUBSCRIBER_NUMBER];
  void (*subscriber_callback[NODE4_SUBSCRIBER_NUMBER])(const void * msgin);
#endif

#if NODE4_TIMER_NUMBER > 0
  rcl_timer_t timer[NODE4_TIMER_NUMBER];
  void (*timer_callback[NODE4_TIMER_NUMBER])(rcl_timer_t * timer, int64_t last_call_time);
#endif
} my_node4_t;
//custom_interfaces__msg__Message pub_msg2;

rclc_support_t support;
volatile rcl_time_point_value_t start_time;
my_node1_t node1;
my_node2_t node2;
my_node3_t node3;
my_node4_t node4;

rcl_time_point_value_t *timestamp[LOGGER_DIM0];

/***************************** CALLBACKS ***********************************/
void node1_timer1_callback(rcl_timer_t * timer, int64_t last_call_time)
{
  RCLC_UNUSED(last_call_time);
  if (timer == NULL)
  {
    printf("timer_callback Error: timer parameter is NULL\n");
    return;
  }
  custom_interfaces__msg__Message pub_msg;
  rcl_time_point_value_t now = rclc_now(&support);
  if (node1.first_run)
  {
    start_time = now;
    node1.first_run = false;
    printf("StartTime %ld\n", start_time);
  }
  pub_msg.frame_id = node1.count1++;
  pub_msg.stamp = now;

  //printf("Timer: Timer start\n");
  timestamp[0][pub_msg.frame_id] = (now - start_time)/1000;
  RCSOFTCHECK(rcl_publish(&node1.publisher[0], &pub_msg, NULL));
  //printf("Timer1: Published message %ld at time %ld \n", pub_msg.frame_id, now-start_time);
}

void node2_subscriber1_callback(const void * msgin)
{
  const custom_interfaces__msg__Message * msg = (const custom_interfaces__msg__Message *)msgin;
  rcl_time_point_value_t now;
  if (msgin == NULL) {
    printf("Callback: msg NULL\n");
    return;
  }
  now = rclc_now(&support);
  timestamp[1][msg->frame_id] = (now - msg->stamp)/1000;
  //printf("Node1_Sub1_Callback: I heard: %ld at time %ld\n", msg->frame_id, timestamp[1][msg->frame_id]);
  busy_wait_random(5, 15);
  now = rclc_now(&support);
  timestamp[2][msg->frame_id] = (now - msg->stamp)/1000;  
  RCSOFTCHECK(rcl_publish(&node2.publisher[0], msg, NULL));
  //printf("Node1_Sub1_Callback: Published message %ld at time %ld\n", msg->frame_id, msg->stamp);
}

void node3_subscriber1_callback(const void * msgin)
{
  const custom_interfaces__msg__Message * msg = (const custom_interfaces__msg__Message *)msgin;
  rcl_time_point_value_t now;
  if (msgin == NULL) {
    printf("Callback: msg NULL\n");
    return;
  }
  now = rclc_now(&support);
  timestamp[3][msg->frame_id] = (now - msg->stamp)/1000;
  //printf("Node1_Sub2_Callback: I heard: %ld at time %ld\n", msg->frame_id, timestamp[2][msg->frame_id]);
  busy_wait_random(5, 20);
  now = rclc_now(&support);
  timestamp[4][msg->frame_id] = (now - msg->stamp)/1000;  
  RCSOFTCHECK(rcl_publish(&node3.publisher[0], msg, NULL));
  //printf("Node1_Sub1_Callback: Published message %ld at time %ld\n", msg->frame_id, msg->stamp);
}

void node4_subscriber1_callback(const void * msgin)
{
  const custom_interfaces__msg__Message * msg = (const custom_interfaces__msg__Message *)msgin;
  rcl_time_point_value_t now;
  if (msgin == NULL) {
    printf("Callback: msg NULL\n");
    return;
  }
  now = rclc_now(&support);
  timestamp[5][msg->frame_id] = (now - msg->stamp)/1000;
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
    node1.first_run = true;
    node1.count1 = 0;

    node1.timer_callback[0] = &node1_timer1_callback;

    node2.subscriber_callback[0] = &node2_subscriber1_callback;
    node3.subscriber_callback[0] = &node3_subscriber1_callback;
    node4.subscriber_callback[0] = &node4_subscriber1_callback;

    srand(time(NULL));
    exit_flag = false;

    const unsigned int timer_timeout[NODE1_TIMER_NUMBER] = {timer_period};
    int logger_dim1 =  (experiment_duration/min_period(NODE1_TIMER_NUMBER, timer_timeout)) + 1;
    init_timestamp(LOGGER_DIM0, logger_dim1, timestamp);

    // create init_options
    RCCHECK(rclc_support_init(&support, argc, argv, &allocator));

    // create rcl_node
    node1.rcl_node = rcl_get_zero_initialized_node();
    RCCHECK(rclc_node_init_default(&node1.rcl_node, "node_1", "rclc_app", &support));

    node2.rcl_node = rcl_get_zero_initialized_node();
    RCCHECK(rclc_node_init_default(&node2.rcl_node, "node_2", "rclc_app", &support));   

    node3.rcl_node = rcl_get_zero_initialized_node();
    RCCHECK(rclc_node_init_default(&node3.rcl_node, "node_3", "rclc_app", &support));

    node4.rcl_node = rcl_get_zero_initialized_node();
    RCCHECK(rclc_node_init_default(&node4.rcl_node, "node_4", "rclc_app", &support)); 
  
    topic_name[0] = "topic1";
    topic_name[1] = "topic2";
    topic_name[2] = "topic3";

    const rosidl_message_type_support_t * my_type_support =
      ROSIDL_GET_MSG_TYPE_SUPPORT(custom_interfaces, msg, Message);  
    
    int i;
    
    for (i = 0; i < NODE1_PUBLISHER_NUMBER; i++)
    {
      RCCHECK(rclc_publisher_init_default(&node1.publisher[i], &node1.rcl_node, my_type_support, topic_name[0]));    
    }


    for (i = 0; i < NODE2_PUBLISHER_NUMBER; i++)
    {
      RCCHECK(rclc_publisher_init_default(&node2.publisher[i], &node2.rcl_node, my_type_support, topic_name[1]));    
    }

    // create a timer, which will call the publisher with period=`timer_timeout` ms in the 'node1_timer_callback'

    for (i = 0; i < NODE1_TIMER_NUMBER; i++)
    {
      node1.timer[i] = rcl_get_zero_initialized_timer();
      RCCHECK(rclc_timer_init_default(&node1.timer[i], &support, RCL_MS_TO_NS(timer_timeout[i]), node1.timer_callback[i]));
    }

    for (i = 0; i < NODE2_SUBSCRIBER_NUMBER; i++)
    {
      node2.subscriber[i] = rcl_get_zero_initialized_subscription();
      RCCHECK(rclc_subscription_init_default(&node2.subscriber[i], &node2.rcl_node, my_type_support, topic_name[0]));
      //printf("Created subscriber %s:\n", topic_name[i]);
      custom_interfaces__msg__Message__init(&node2.sub_msg[i]);
    }

    for (i = 0; i < NODE3_SUBSCRIBER_NUMBER; i++)
    {
      node3.subscriber[i] = rcl_get_zero_initialized_subscription();
      RCCHECK(rclc_subscription_init_default(&node3.subscriber[i], &node3.rcl_node, my_type_support, topic_name[1]));
      //printf("Created subscriber %s:\n", topic_name[i]);
      custom_interfaces__msg__Message__init(&node3.sub_msg[i]);
    }

    for (i = 0; i < NODE3_PUBLISHER_NUMBER; i++)
    {
      RCCHECK(rclc_publisher_init_default(&node3.publisher[i], &node3.rcl_node, my_type_support, topic_name[2]));    
    }

    for (i = 0; i < NODE4_SUBSCRIBER_NUMBER; i++)
    {
      node4.subscriber[i] = rcl_get_zero_initialized_subscription();
      RCCHECK(rclc_subscription_init_default(&node4.subscriber[i], &node4.rcl_node, my_type_support, topic_name[2]));
      //printf("Created subscriber %s:\n", topic_name[i]);
      custom_interfaces__msg__Message__init(&node4.sub_msg[i]);
    }

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
    
    unsigned int num_handles = 1;
    //printf("Debug: number of DDS handles: %u\n", num_handles);
    RCCHECK(rclc_executor_init(&executor1, &support.context, num_handles, &allocator));
    RCCHECK(rclc_executor_init(&executor2, &support.context, num_handles, &allocator));
    RCCHECK(rclc_executor_init(&executor3, &support.context, num_handles, &allocator));
    RCCHECK(rclc_executor_init(&executor4, &support.context, num_handles, &allocator));


    for (i = 0; i < NODE1_TIMER_NUMBER; i++)
    {
      RCCHECK(rclc_executor_add_timer(&executor1, &node1.timer[i]));
    }

    for (i = 0; i < NODE2_SUBSCRIBER_NUMBER; i++)
    {
      RCCHECK(rclc_executor_add_subscription(
        &executor2, &node2.subscriber[i], &node2.sub_msg[i], node2.subscriber_callback[i],
        ON_NEW_DATA));
    }

    for (i = 0; i < NODE3_SUBSCRIBER_NUMBER; i++)
    {
      RCCHECK(rclc_executor_add_subscription(
        &executor3, &node3.subscriber[i], &node3.sub_msg[i], node3.subscriber_callback[i],
        ON_NEW_DATA));
    }

    for (i = 0; i < NODE4_SUBSCRIBER_NUMBER; i++)
    {
      RCCHECK(rclc_executor_add_subscription(
        &executor4, &node4.subscriber[i], &node4.sub_msg[i], node4.subscriber_callback[i],
        ON_NEW_DATA));
    }
    RCCHECK(rclc_executor_set_timeout(&executor1,timeout_ns));
    RCCHECK(rclc_executor_set_timeout(&executor2,timeout_ns));
    RCCHECK(rclc_executor_set_timeout(&executor3,timeout_ns));
    RCCHECK(rclc_executor_set_timeout(&executor4,timeout_ns));

    if(let)
    {
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
    printf("ExecutorID Executor1 %lu\n", (unsigned long) &executor1);
    printf("ExecutorID Executor2 %lu\n", (unsigned long) &executor2);
    printf("ExecutorID Executor3 %lu\n", (unsigned long) &executor3);
    printf("ExecutorID Executor4 %lu\n", (unsigned long) &executor4);

    ////////////////////////////////////////////////////////////////////////////
    // Configuration of Linux threads
    ////////////////////////////////////////////////////////////////////////////
    pthread_t thread1 = 0;
    pthread_t thread2 = 0;
    pthread_t thread3 = 0;
    pthread_t thread4 = 0;
    int policy = SCHED_FIFO;

    if (executor_period > 0)
    {
        struct arg_spin_period ex1 = {executor_period*1000*1000, &executor1, &support};
        struct arg_spin_period ex2 = {executor_period*1000*1000, &executor2, &support};
        struct arg_spin_period ex3 = {executor_period*1000*1000, &executor3, &support};
        struct arg_spin_period ex4 = {executor_period*1000*1000, &executor4, &support};
        thread_create(&thread1, policy, 49, 0, rclc_executor_spin_period_wrapper, &ex1);
        thread_create(&thread2, policy, 48, 0, rclc_executor_spin_period_wrapper, &ex2);
        thread_create(&thread3, policy, 47, 0, rclc_executor_spin_period_wrapper, &ex3);
        thread_create(&thread4, policy, 46, 0, rclc_executor_spin_period_wrapper, &ex4);
    }
    else
    {
        thread_create(&thread1, policy, 49, 0, rclc_executor_spin_wrapper, &executor1);
        thread_create(&thread2, policy, 48, 0, rclc_executor_spin_wrapper, &executor2);
        thread_create(&thread3, policy, 47, 0, rclc_executor_spin_wrapper, &executor3);
        thread_create(&thread4, policy, 46, 0, rclc_executor_spin_wrapper, &executor4);
    }

    //thread_create(&thread1, policy, 49, 0, rclc_executor_spin_wrapper, &executor1);

    //printf("Running experiment from now on for %ds\n", EXPERIMENT_DURATION);
    sleep_ms(experiment_duration);
    exit_flag = true;

    // Wait for threads to finish
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    pthread_join(thread4, NULL);

    // clean up 
    RCCHECK(rclc_executor_fini(&executor1));
    RCCHECK(rclc_executor_fini(&executor2));
    RCCHECK(rclc_executor_fini(&executor3));
    RCCHECK(rclc_executor_fini(&executor4));

#if (NODE1_PUBLISHER_NUMBER > 0)
      for (i = 0; i < NODE1_PUBLISHER_NUMBER; i++)
      {
        RCCHECK(rcl_publisher_fini(&node1.publisher[i], &node1.rcl_node));
      }
#endif

#if (NODE1_TIMER_NUMBER > 0)
      for (i = 0; i < NODE1_TIMER_NUMBER; i++)
      {
        RCCHECK(rcl_timer_fini(&node1.timer[i]));
      }
#endif


    RCCHECK(rcl_node_fini(&node1.rcl_node));

#if (NODE2_SUBSCRIBER_NUMBER > 0)
    for (i = 0; i < NODE2_SUBSCRIBER_NUMBER; i++)
    {
      RCCHECK(rcl_subscription_fini(&node2.subscriber[i], &node2.rcl_node));
    }
#endif

#if (NODE2_PUBLISHER_NUMBER > 0)
    for (i = 0; i < NODE2_PUBLISHER_NUMBER; i++)
    {
      RCCHECK(rcl_publisher_fini(&node2.publisher[i], &node2.rcl_node));
    }
#endif

    RCCHECK(rcl_node_fini(&node2.rcl_node));

#if (NODE3_SUBSCRIBER_NUMBER > 0)
    for (i = 0; i < NODE3_SUBSCRIBER_NUMBER; i++)
    {
      RCCHECK(rcl_subscription_fini(&node3.subscriber[i], &node3.rcl_node));
    }
#endif

    RCCHECK(rcl_node_fini(&node3.rcl_node));

#if (NODE4_SUBSCRIBER_NUMBER > 0)
    for (i = 0; i < NODE4_SUBSCRIBER_NUMBER; i++)
    {
      RCCHECK(rcl_subscription_fini(&node4.subscriber[i], &node4.rcl_node));
    }
#endif

    RCCHECK(rcl_node_fini(&node4.rcl_node));
    RCCHECK(rclc_support_fini(&support));  

    print_timestamp(LOGGER_DIM0,logger_dim1, timestamp);
    fini_timestamp(LOGGER_DIM0, timestamp);
 
  	return 0;
}
