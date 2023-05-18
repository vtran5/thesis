//#include<iostream>
//#include<functional>
#include "custom_interfaces/msg/message.h"
#include "custom_interfaces/srv/service.h"
#include "utilities.h"
#include <stdlib.h>

#define EXPERIMENT_DURATION 10000 //ms

#define NODE1_PUBLISHER_NUMBER 1
#define NODE1_SUBSCRIBER_NUMBER 1
#define NODE1_TIMER_NUMBER 1
#define NODE1_CLIENT_NUMBER 1
#define NODE1_SERVER_NUMBER 0

#define NODE2_PUBLISHER_NUMBER 0
#define NODE2_SUBSCRIBER_NUMBER 1
#define NODE2_TIMER_NUMBER 0
#define NODE2_CLIENT_NUMBER 0
#define NODE2_SERVER_NUMBER 1

#define TOPIC_NUMBER 2

#define LOGGER_DIM0 5

const char * topic_name[TOPIC_NUMBER];

// struct to store node variables
typedef struct my_node1
{
  rcl_node_t rcl_node;
  volatile bool first_run;
  volatile int64_t count;
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

#if NODE1_CLIENT_NUMBER > 0
  rcl_client_t client[NODE1_CLIENT_NUMBER];
  custom_interfaces__msg__Message res[NODE1_CLIENT_NUMBER];
  void (*client_callback[NODE1_CLIENT_NUMBER])(const void * msg);
#endif

#if NODE1_SERVER_NUMBER > 0
  rcl_service_t server[NODE1_SERVER_NUMBER];
  custom_interfaces__msg__Message res[NODE1_SERVER_NUMBER];
  custom_interfaces__msg__Message req[NODE1_SERVER_NUMBER];
  void (*server_callback[NODE1_SERVER_NUMBER])(const void * req, void * res);
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

#if NODE2_CLIENT_NUMBER > 0
  rcl_client_t client[NODE2_CLIENT_NUMBER];
  custom_interfaces__srv__Service_Response res[NODE2_CLIENT_NUMBER];
  void (*client_callback[NODE2_CLIENT_NUMBER])(const void * msg);
#endif
  
#if NODE2_SERVER_NUMBER > 0
  rcl_service_t server[NODE2_SERVER_NUMBER];
  custom_interfaces__srv__Service_Response res[NODE2_SERVER_NUMBER];
  custom_interfaces__srv__Service_Request req[NODE2_SERVER_NUMBER];
  void (*server_callback[NODE2_SERVER_NUMBER])(const void * req, void * res);
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
  if (timer == NULL)
  {
    printf("timer_callback Error: timer parameter is NULL\n");
    return;
  }

  RCLC_UNUSED(last_call_time);
  custom_interfaces__srv__Service_Request req;
  int64_t seq;
  rcl_time_point_value_t now = rclc_now(&support);
  if (node1.first_run)
  {
    start_time = now;
    node1.first_run = false;
  }
  req.frame_id = node1.count;
  req.stamp = now;
  timestamp[0][req.frame_id] = (now - start_time)/1000;
  RCSOFTCHECK(rcl_send_request(&node1.client[0], &req, &seq));
  printf("Timer1: Send request %ld at time %ld \n", req.frame_id, now-start_time);
  busy_wait(15);
}

void node1_subscriber1_callback(const void * msgin)
{
  // This subscriber is set to ALWAYS so it doesn't care if there is a new message or not
  custom_interfaces__msg__Message msg;
  rcl_time_point_value_t now;
  now = rclc_now(&support);
  msg.frame_id = node1.count++;
  msg.stamp = now;
  RCSOFTCHECK(rcl_publish(&node1.publisher[0], &msg, NULL));
  timestamp[1][msg.frame_id] = (now - msg.stamp)/1000;
  printf("Node1_Sub1_Callback: Published message %ld at time %ld\n", msg.frame_id, now/1000000);
}

void node1_client1_callback(const void * msgin)
{
  const custom_interfaces__srv__Service_Response * msg = (const custom_interfaces__srv__Service_Response *)msgin;
  rcl_time_point_value_t now;
  now = rclc_now(&support);
  timestamp[2][msg->frame_id] = (now - msg->stamp)/1000; 
  printf("Client received response %ld at time %ld\n", msg->frame_id, now); 
}

void node2_subscriber1_callback(const void * msgin)
{
  const custom_interfaces__msg__Message * msg = (const custom_interfaces__msg__Message *)msgin;
  rcl_time_point_value_t now;
  now = rclc_now(&support);
  if (msg == NULL)
    printf("Node2_Sub1_Callback: receives NULL msg\n");
  else
  //timestamp[3][msg->frame_id] = (now - msg->stamp)/1000; 
  printf("Node2_Sub1_Callback: I heard: %ld at time %ld\n", msg->frame_id, now);
}

void node2_server1_callback(const void * req, void * res)
{
  const custom_interfaces__srv__Service_Request * req_in = (const custom_interfaces__srv__Service_Request *) req;
  custom_interfaces__srv__Service_Response * res_in = (custom_interfaces__srv__Service_Response *) res;
  res_in->frame_id = req_in->frame_id;
  res_in->stamp = req_in->stamp;
  rcl_time_point_value_t now;
  now = rclc_now(&support);
  timestamp[4][req_in->frame_id] = (now - req_in->stamp)/1000;  
  printf("Server receives request %ld at time %ld\n", req_in->frame_id, now);    
}


/******************** MAIN PROGRAM ****************************************/
int main(int argc, char const *argv[])
{
    rcl_allocator_t allocator = rcl_get_default_allocator();
    node1.first_run = true;
    node1.count = 0;

    node1.timer_callback[0] = &node1_timer1_callback;
    node1.subscriber_callback[0] = &node1_subscriber1_callback;
    node1.client_callback[0] = &node1_client1_callback;

    node2.subscriber_callback[0] = &node2_subscriber1_callback;
    node2.server_callback[0] = &node2_server1_callback;

    srand(time(NULL));
    exit_flag = false;

    const unsigned int timer_timeout[NODE1_TIMER_NUMBER] = {500};
    int logger_dim1 =  (EXPERIMENT_DURATION/min_period(NODE1_TIMER_NUMBER, timer_timeout)) + 1;
    init_timestamp(LOGGER_DIM0, logger_dim1, timestamp);
    
    // create init_options
    RCCHECK(rclc_support_init(&support, argc, argv, &allocator));

    // create rcl_node
    node1.rcl_node = rcl_get_zero_initialized_node();
    RCCHECK(rclc_node_init_default(&node1.rcl_node, "node_1", "rclc_exp", &support));
    node2.rcl_node = rcl_get_zero_initialized_node();
    RCCHECK(rclc_node_init_default(&node2.rcl_node, "node_2", "rclc_exp", &support));

    topic_name[0] = "topic1";
    topic_name[1] = "topic2";
    const rosidl_message_type_support_t * my_type_support =
      ROSIDL_GET_MSG_TYPE_SUPPORT(custom_interfaces, msg, Message);  
    const rosidl_service_type_support_t * my_type_support_srv =
      ROSIDL_GET_SRV_TYPE_SUPPORT(custom_interfaces, srv, Service);  

    int i;
    for (i = 0; i < NODE1_PUBLISHER_NUMBER; i++)
    {
      RCCHECK(rclc_publisher_init_default(&node1.publisher[i], &node1.rcl_node, my_type_support, topic_name[1]));    
    }
    // create a timer, which will call the publisher with period=`timer_timeout` ms in the 'node1_timer_callback'
    
    for (i = 0; i < NODE1_TIMER_NUMBER; i++)
    {
      node1.timer[i] = rcl_get_zero_initialized_timer();
      RCCHECK(rclc_timer_init_default(&node1.timer[i], &support, RCL_MS_TO_NS(timer_timeout[i]), node1.timer_callback[i]));
    }
    for (i = 0; i < NODE1_SUBSCRIBER_NUMBER; i++)
    {
      node1.subscriber[i] = rcl_get_zero_initialized_subscription();
      RCCHECK(rclc_subscription_init_default(&node1.subscriber[i], &node1.rcl_node, my_type_support, topic_name[0]));
      custom_interfaces__msg__Message__init(&node1.sub_msg[i]);
    }
    for (i = 0; i < NODE1_CLIENT_NUMBER; i++)
    {
      node1.client[i] = rcl_get_zero_initialized_client();
      RCCHECK(rclc_client_init_default(&node1.client[i], &node1.rcl_node, my_type_support_srv, "/test_service"));
    }
    for (i = 0; i < NODE2_SUBSCRIBER_NUMBER; i++)
    {
      node2.subscriber[i] = rcl_get_zero_initialized_subscription();
      RCCHECK(rclc_subscription_init_default(&node2.subscriber[i], &node2.rcl_node, my_type_support, topic_name[1]));
      custom_interfaces__msg__Message__init(&node2.sub_msg[i]);
    }
    for (i = 0; i < NODE2_SERVER_NUMBER; i++)
    {
      node2.server[i] = rcl_get_zero_initialized_service();
      RCCHECK(rclc_service_init_default(&node2.server[i], &node2.rcl_node, my_type_support_srv, "/test_service"));
    }
    printf("Finish Init\n");
    ////////////////////////////////////////////////////////////////////////////
    // Configuration of RCL Executor
    ////////////////////////////////////////////////////////////////////////////
    const uint64_t timeout_ns = 400000000;
    rclc_executor_t executor1;
    executor1 = rclc_executor_get_zero_initialized_executor();

    unsigned int num_handles = 7; // 1 timer + 2 subs + 1 client + 1 server
    //printf("Debug: number of DDS handles: %u\n", num_handles);
    RCCHECK(rclc_executor_init(&executor1, &support.context, num_handles, &allocator));
    RCCHECK(rclc_executor_set_semantics(&executor1, LET));
    RCCHECK(rclc_executor_add_subscription(
        &executor1, &node1.subscriber[0], &node1.sub_msg[0], node1.subscriber_callback[0],
        ALWAYS));
    RCCHECK(rclc_executor_add_timer(&executor1, &node1.timer[0]));
    RCCHECK(rclc_executor_add_service(&executor1, &node2.server[0], &node2.req[0], &node2.res[0], node2.server_callback[0]));
    RCCHECK(rclc_executor_add_client(&executor1, &node1.client[0], &node1.res[0], node1.client_callback[0]));
    RCCHECK(rclc_executor_add_subscription(
        &executor1, &node2.subscriber[0], &node2.sub_msg[0], node2.subscriber_callback[0],
        ALWAYS));

    RCCHECK(rclc_executor_set_timeout(&executor1,timeout_ns));
    RCCHECK(rclc_executor_set_trigger(&executor1, rclc_executor_trigger_one, &node1.timer[0]));


    printf("Finish configure executor\n");
    ////////////////////////////////////////////////////////////////////////////
    // Configuration of Linux threads
    ////////////////////////////////////////////////////////////////////////////
    pthread_t thread1 = 0;
    int policy = SCHED_FIFO;
    struct arg_spin arg ={&executor1, &support};
    thread_create(&thread1, policy, 49, 0, rclc_executor_spin_wrapper, &arg);


    printf("Running experiment from now on for %ds\n", EXPERIMENT_DURATION);
    sleep_ms(EXPERIMENT_DURATION);
    exit_flag = true;
    printf("Stop experiment\n");
    // Wait for threads to finish
    pthread_join(thread1, NULL);

    printf("Start clean up\n");
    // clean up 
    RCCHECK(rclc_executor_fini(&executor1));
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

#if (NODE1_SUBSCRIBER_NUMBER > 0)
    for (i = 0; i < NODE1_SUBSCRIBER_NUMBER; i++)
    {
      RCCHECK(rcl_subscription_fini(&node1.subscriber[i], &node1.rcl_node));
    }
#endif

#if (NODE1_SERVER_NUMBER > 0)
    for (i = 0; i < NODE1_SERVER_NUMBER; i++)
    {
      RCCHECK(rcl_service_fini(&node1.server[i], &node1.rcl_node));
    }
#endif

#if (NODE1_CLIENT_NUMBER > 0)
    for (i = 0; i < NODE1_CLIENT_NUMBER; i++)
    {
      RCCHECK(rcl_client_fini(&node1.client[i], &node1.rcl_node));
    }
#endif

    RCCHECK(rcl_node_fini(&node1.rcl_node));

#if (NODE2_TIMER_NUMBER > 0)
      for (i = 0; i < NODE2_TIMER_NUMBER; i++)
      {
        RCCHECK(rcl_timer_fini(&node2.timer[i]));
      }
#endif

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

#if (NODE2_SERVER_NUMBER > 0)
    for (i = 0; i < NODE2_SERVER_NUMBER; i++)
    {
      RCCHECK(rcl_service_fini(&node2.server[i], &node2.rcl_node));
    }
#endif

#if (NODE2_CLIENT_NUMBER > 0)
    for (i = 0; i < NODE2_CLIENT_NUMBER; i++)
    {
      RCCHECK(rcl_client_fini(&node2.client[i], &node2.rcl_node));
    }
#endif

    RCCHECK(rcl_node_fini(&node2.rcl_node));
    RCCHECK(rclc_support_fini(&support));  

    print_timestamp(LOGGER_DIM0,logger_dim1, timestamp);
    fini_timestamp(LOGGER_DIM0, timestamp);
 
    return 0;
}
