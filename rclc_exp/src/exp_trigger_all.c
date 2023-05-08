//#include<iostream>
//#include<functional>
#include "custom_interfaces/msg/message.h"
#include "utilities.h"
#include <stdlib.h>


#define EXPERIMENT_DURATION 10000 //ms

#define NODE1_PUBLISHER_NUMBER 2
#define NODE1_SUBSCRIBER_NUMBER 0
#define NODE1_TIMER_NUMBER 2

#define NODE2_PUBLISHER_NUMBER 0
#define NODE2_SUBSCRIBER_NUMBER 2
#define NODE2_TIMER_NUMBER 0

#define TOPIC_NUMBER 2
#define LOGGER_DIM0 4

const char * topic_name[TOPIC_NUMBER];

// struct to store node variables
typedef struct my_node1
{
  rcl_node_t rcl_node;
  volatile bool first_run;
  volatile int64_t count1;
  volatile int64_t count2;
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
  volatile bool sub1_new_data;
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

//custom_interfaces__msg__Message pub_msg2;

rclc_support_t support;
volatile rcl_time_point_value_t start_time;
my_node1_t node1;
my_node2_t node2;

rcl_time_point_value_t *timestamp[LOGGER_DIM0];

/***************************** CALLBACKS ***********************************/
void node1_timer1_callback(rcl_timer_t * timer, int64_t last_call_time)
{
  RCLC_UNUSED(last_call_time);
  custom_interfaces__msg__Message pub_msg;
	rcl_time_point_value_t now = rclc_now(&support);
	if (node1.first_run)
	{
		start_time = now;
		node1.first_run = false;
	}
	pub_msg.frame_id = node1.count1++;
	pub_msg.stamp = now;

	if (timer != NULL) {
		//printf("Timer: Timer start\n");
		timestamp[0][pub_msg.frame_id] = (now - start_time)/1000;
    RCSOFTCHECK(rcl_publish(&node1.publisher[0], &pub_msg, NULL));
    printf("Timer1: Published message %ld at time %ld \n", pub_msg.frame_id, now-start_time);
  	} else {
    	printf("timer_callback Error: timer parameter is NULL\n");
  	}
}

void node1_timer2_callback(rcl_timer_t * timer, int64_t last_call_time)
{
  RCLC_UNUSED(last_call_time);
  custom_interfaces__msg__Message pub_msg;
  rcl_time_point_value_t now = rclc_now(&support);
  if (node1.first_run)
  {
    start_time = now;
    node1.first_run = false;
  }
  pub_msg.frame_id = node1.count2++;
  pub_msg.stamp = now;

  if (timer != NULL) {
    //printf("Timer: Timer start\n");
    timestamp[1][pub_msg.frame_id] = (now - start_time)/1000;
    RCSOFTCHECK(rcl_publish(&node1.publisher[1], &pub_msg, NULL));
    printf("Timer2: Published message %ld at time %ld \n", pub_msg.frame_id, now-start_time);
    } else {
      printf("timer_callback Error: timer parameter is NULL\n");
    }
}

void node2_subscriber1_callback(const void * msgin)
{
	const custom_interfaces__msg__Message * msg = (const custom_interfaces__msg__Message *)msgin;
	rcl_time_point_value_t now;
	if (msgin == NULL) {
  	printf("Callback: msg NULL\n");
    return;
	}
  node2.sub1_new_data = true; 
  now = rclc_now(&support);
	timestamp[2][msg->frame_id] = (now - msg->stamp)/1000;
  printf("Node1_Sub1_Callback: I heard: %ld at time %ld\n", msg->frame_id, now-start_time);
  busy_wait_random(100,150);
  //printf("Node1_Sub1_Callback: Published message %ld at time %ld\n", msg->frame_id, msg->stamp);
}

void node2_subscriber2_callback(const void * msgin)
{
  const custom_interfaces__msg__Message * msg = (const custom_interfaces__msg__Message *)msgin;
  if (msg == NULL) {
    printf("Callback: msg NULL\n");
    return;
  }

  if (!node2.sub1_new_data) // ignore if not received new topic1 message
  {
    timestamp[3][msg->frame_id] = 1;
    return;
  }
  rcl_time_point_value_t now = rclc_now(&support);
  timestamp[3][msg->frame_id] = (now - msg->stamp)/1000;
  node2.sub1_new_data = false;
  printf("Node1_Sub2_Callback: I heard: %ld at time %ld\n", msg->frame_id, msg->stamp);
  busy_wait_random(100,150);
}

/******************** MAIN PROGRAM ****************************************/
int main(int argc, char const *argv[])
{
	  rcl_allocator_t allocator = rcl_get_default_allocator();
  	node1.first_run = true;
  	node1.count1 = 0;
    node1.count2 = 0;
    node2.sub1_new_data = false;

    node1.timer_callback[0] = &node1_timer1_callback;
    node1.timer_callback[1] = &node1_timer2_callback;

    node2.subscriber_callback[0] = &node2_subscriber1_callback;
    node2.subscriber_callback[1] = &node2_subscriber2_callback;

  	srand(time(NULL));
    exit_flag = false;

    const unsigned int timer_timeout[NODE1_TIMER_NUMBER] = {500,100};
    int logger_dim1 =  (EXPERIMENT_DURATION/min_period(NODE1_TIMER_NUMBER, timer_timeout)) + 1;
    init_timestamp(LOGGER_DIM0, logger_dim1, timestamp);

    // create init_options
    RCCHECK(rclc_support_init(&support, argc, argv, &allocator));

    // create rcl_node
    node1.rcl_node = rcl_get_zero_initialized_node();
    RCCHECK(rclc_node_init_default(&node1.rcl_node, "node_1", "rclc_app", &support));

    node2.rcl_node = rcl_get_zero_initialized_node();
    RCCHECK(rclc_node_init_default(&node2.rcl_node, "node_2", "rclc_app", &support));		
	
    topic_name[0] = "topic1";
    topic_name[1] = "topic2";
    const rosidl_message_type_support_t * my_type_support =
      ROSIDL_GET_MSG_TYPE_SUPPORT(custom_interfaces, msg, Message);  
    int i;
    for (i = 0; i < NODE1_PUBLISHER_NUMBER; i++)
    {
      RCCHECK(rclc_publisher_init_default(&node1.publisher[i], &node1.rcl_node, my_type_support, topic_name[i]));    
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
      RCCHECK(rclc_subscription_init_default(&node2.subscriber[i], &node2.rcl_node, my_type_support, topic_name[i]));
      //printf("Created subscriber %s:\n", topic_name[i]);
      custom_interfaces__msg__Message__init(&node2.sub_msg[i]);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Configuration of RCL Executor
    ////////////////////////////////////////////////////////////////////////////
    const uint64_t timeout_ns = 100000000;
    rclc_executor_t executor1;
    executor1 = rclc_executor_get_zero_initialized_executor();
    
    unsigned int num_handles = 1 + 2; // 1 timer + 2 subs
    //printf("Debug: number of DDS handles: %u\n", num_handles);
    RCCHECK(rclc_executor_init(&executor1, &support.context, num_handles, &allocator));


    for (i = 0; i < NODE1_TIMER_NUMBER; i++)
    {
      RCCHECK(rclc_executor_add_timer(&executor1, &node1.timer[i]));
    }

    RCCHECK(rclc_executor_set_timeout(&executor1,timeout_ns));

    //RCCHECK(rclc_executor_set_semantics(&executor, LET));

    rclc_executor_t executor2;
	  executor2 = rclc_executor_get_zero_initialized_executor();
    RCCHECK(rclc_executor_init(&executor2, &support.context, num_handles, &allocator)); 

    // add subscription to executor
    for (i = 0; i < NODE2_SUBSCRIBER_NUMBER; i++)
    {
      RCCHECK(rclc_executor_add_subscription(
        &executor2, &node2.subscriber[i], &node2.sub_msg[i], node2.subscriber_callback[i],
        ON_NEW_DATA));
    }


    RCCHECK(rclc_executor_set_timeout(&executor2,timeout_ns));
    //RCCHECK(rclc_executor_set_semantics(&executor2, LET));
    RCCHECK(rclc_executor_set_trigger(&executor2, rclc_executor_trigger_all, NULL));

    ////////////////////////////////////////////////////////////////////////////
    // Configuration of Linux threads
    ////////////////////////////////////////////////////////////////////////////
    pthread_t thread1 = 0;
    pthread_t thread2 = 0;
    int policy = SCHED_FIFO;

    //struct arg_spin_period ex1 = {500*1000*1000, &executor1};
    //struct arg_spin_period ex2 = {500*1000*1000, &executor2};
    //thread_create(&thread1, policy, 10, rclc_executor_spin_period_wrapper, &ex1);
    //thread_create(&thread2, policy, 49, rclc_executor_spin_period_wrapper, &ex2);
    thread_create(&thread1, policy, 10, 0, rclc_executor_spin_wrapper, &executor1);
    thread_create(&thread2, policy, 49, 0, rclc_executor_spin_wrapper, &executor2);

    //printf("Running experiment from now on for %ds\n", EXPERIMENT_DURATION);
    sleep_ms(EXPERIMENT_DURATION);
    exit_flag = true;
    printf("Stop experiment\n");
    // Wait for threads to finish
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    printf("Start clean up\n");
    // clean up 
    RCCHECK(rclc_executor_fini(&executor1));
    for (i = 0; i < NODE1_PUBLISHER_NUMBER; i++)
    {
      RCCHECK(rcl_publisher_fini(&node1.publisher[i], &node1.rcl_node));
    }

    for (i = 0; i < NODE1_TIMER_NUMBER; i++)
    {
      RCCHECK(rcl_timer_fini(&node1.timer[i]));
    }

    RCCHECK(rcl_node_fini(&node1.rcl_node));


    RCCHECK(rclc_executor_fini(&executor2));

    for (i = 0; i < NODE2_SUBSCRIBER_NUMBER; i++)
    {
      RCCHECK(rcl_subscription_fini(&node2.subscriber[i], &node2.rcl_node));
    }

    RCCHECK(rcl_node_fini(&node2.rcl_node));
    RCCHECK(rclc_support_fini(&support));  

    print_timestamp(LOGGER_DIM0,logger_dim1, timestamp);
    fini_timestamp(LOGGER_DIM0, timestamp);
 
  	return 0;
}
