//#include<iostream>
//#include<functional>
#include <stdio.h>
#include "custom_interfaces/msg/message.h"
#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d. Aborting.\n",__LINE__,(int)temp_rc); return 1;}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d. Continuing.\n",__LINE__,(int)temp_rc);}}

#define EXPERIMENT_DURATION 5000 //ms

#define NODE1_PUBLISHER_NUMBER 2
#define NODE1_SUBSCRIBER_NUMBER 2
#define NODE1_TIMER_NUMBER 1

#define NODE2_PUBLISHER_NUMBER 0
#define NODE2_SUBSCRIBER_NUMBER 1

#define TOPIC_NUMBER 2

rcl_publisher_t node1_publisher[NODE1_PUBLISHER_NUMBER];
rcl_subscription_t node1_subscriber[NODE1_SUBSCRIBER_NUMBER];
rcl_timer_t node1_timer[NODE1_TIMER_NUMBER];

//rcl_publisher_t node2_publisher[NODE1_PUBLISHER_NUMBER];
rcl_subscription_t node2_subscriber[NODE1_SUBSCRIBER_NUMBER];

const char * topic_name[TOPIC_NUMBER];
custom_interfaces__msg__Message pub_msg;
//custom_interfaces__msg__Message pub_msg2;
custom_interfaces__msg__Message sub_msg1;
custom_interfaces__msg__Message sub_msg2;
custom_interfaces__msg__Message sub_msg3;
rclc_support_t support;
rcl_time_point_value_t start_time;
volatile uint8_t first_run;
volatile int64_t count;
volatile bool exit_flag = false;
struct arg_spin_period {
  uint64_t period; //nanoseconds
  rclc_executor_t * executor;
};
rcl_time_point_value_t timestamp[4][100] = {{0}};

// Wrapper function for pthread compatibility
void *rclc_executor_spin_wrapper(void *arg)
{
  rclc_executor_t * executor = (rclc_executor_t *) arg;
  rcl_ret_t ret = RCL_RET_OK;
  while (!exit_flag) {
    ret = rclc_executor_spin_some(executor, executor->timeout_ns);
    if (!((ret == RCL_RET_OK) || (ret == RCL_RET_TIMEOUT))) {
      printf("Executor spin failed\n");
    }
  }
  printf("Executor stops\n");
  return 0;
}

void *rclc_executor_spin_period_wrapper(void *arg)
{
  struct arg_spin_period *arguments = arg;
  rclc_executor_t * executor = arguments->executor;
  const uint64_t period = arguments->period;
  rcl_ret_t ret = RCL_RET_OK;
  while (!exit_flag) {
    ret = rclc_executor_spin_one_period(executor, period);
    if (!((ret == RCL_RET_OK) || (ret == RCL_RET_TIMEOUT))) {
      printf("Executor spin failed\n");
    }
  }
  printf("Executor stops\n");
  return 0;
}

// Function to create and configure thread
void thread_create(pthread_t thread_id, int policy, int priority, void *(*function)(void *), void * arg)
{
    struct sched_param param;
    int ret;
    pthread_attr_t attr;

    ret = pthread_attr_init (&attr);
    ret += pthread_attr_setinheritsched(&attr,PTHREAD_EXPLICIT_SCHED);
    ret += pthread_attr_setschedpolicy(&attr, policy);
    ret += pthread_attr_getschedparam (&attr, &param);
    param.sched_priority = priority;
    ret += pthread_attr_setschedparam (&attr, &param);
    ret += pthread_create(&thread_id, &attr, function, arg);
    if(ret!=0)
      printf("Create thread %lu failed\n", thread_id);
}

// return value in nanoseconds
rcl_time_point_value_t rclc_now(rclc_support_t * support)
{
  rcl_time_point_value_t now = 0;
  RCSOFTCHECK(rcl_clock_get_now(&support->clock, &now));
  return now;
}

/* sleep_ms(): Sleep for the requested number of milliseconds. */
int sleep_ms(int msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    res = nanosleep(&ts, NULL);

    return res;
}

void print_timestamp(int dim0, int dim1, rcl_time_point_value_t timestamp[dim0][dim1])
{
    char line[20000] = "";
    uint8_t end_of_file = 0;
    for (int i = 0; i < dim1; i++)
    {
        int printZero = ((i == 0) ? 1 : 0);
        sprintf(line, "%d ", i);
        for (int j = 0; j < dim0; j++)
        {
            if (timestamp[j][i] == 0 && !printZero)
            {
                end_of_file = 1;
                break;
            }
            printZero = 0;
            char temp[32];
            sprintf(temp, "%ld ", timestamp[j][i]);
            strcat(line, temp);
        }
        strcat(line, "\n");
        if (end_of_file)
          break;
        printf("%s", line);
    }  
}

// this function waits a random amount of time without suspending 
// min_time and max_time are in miliseconds
void busy_wait(int min_time, int max_time, rclc_support_t * support_)
{
  int wait_time = (rand() % (max_time - min_time + 1)) + min_time;
  rcl_time_point_value_t start_wait = rclc_now(support_);
  while(rclc_now(support_) < start_wait + wait_time*1000000)
    asm("nop"); // to prevent optimization
}

/***************************** CALLBACKS ***********************************/
void node1_timer_callback(rcl_timer_t * timer, int64_t last_call_time)
{
  rcl_time_point_value_t now = rclc_now(&support);
  if (first_run)
  {
    start_time = now;
    first_run = 0;
  }
  pub_msg.frame_id = count++;
  pub_msg.stamp = now;

  RCLC_UNUSED(last_call_time);

  if (timer != NULL) {
    //printf("Timer: Timer start\n");
    timestamp[0][pub_msg.frame_id] = (now - start_time)/1000;
    RCSOFTCHECK(rcl_publish(&node1_publisher[0], &pub_msg, NULL));
    //printf("Published message %ld at time %ld \n", pub_msg.frame_id, now-start_time);
    } else {
      printf("timer_callback Error: timer parameter is NULL\n");
    }
}

void node1_subscriber1_callback(const void * msgin)
{
    const custom_interfaces__msg__Message * msg = (const custom_interfaces__msg__Message *)msgin;
    rcl_time_point_value_t now;
    if (msgin == NULL) {
      printf("Callback: msg NULL\n");
    } else {
      now = rclc_now(&support);
      timestamp[1][msg->frame_id] = (now - msg->stamp)/1000;
      //printf("Node1_Sub1_Callback: I heard: %ld at time %ld\n", msg->frame_id, now-start_time);
    }
  busy_wait(100,300, &support);
  RCSOFTCHECK(rcl_publish(&node1_publisher[1], msg, NULL));
  now = rclc_now(&support);
  timestamp[2][msg->frame_id] = (now - msg->stamp)/1000;
  //printf("Node1_Sub1_Callback: Published message %ld at time %ld\n", msg->frame_id, msg->stamp);
}

void node1_subscriber2_callback(const void * msgin)
{
  const custom_interfaces__msg__Message * msg = (const custom_interfaces__msg__Message *)msgin;
  if (msg == NULL) {
    printf("Callback: msg NULL\n");
  } else {
    rcl_time_point_value_t now = rclc_now(&support);
    timestamp[3][msg->frame_id] = (now - msg->stamp)/1000;
    printf("Node1_Sub2_Callback: I heard: %ld at time %ld\n", msg->frame_id, msg->stamp);
  }
}

void node2_subscriber1_callback(const void * msgin)
{
  const custom_interfaces__msg__Message * msg = (const custom_interfaces__msg__Message *)msgin;
  if (msg == NULL) {
    printf("Callback: msg NULL\n");
  } else {
    //rcl_time_point_value_t now = rclc_now(&support);
    //printf("Node2_Sub1_Callback: I heard: %ld at time %ld\n", msg->frame_id, now);
  }
  busy_wait(100,150,&support);
}

/******************** MAIN PROGRAM ****************************************/
int main(int argc, char const *argv[])
{
    rcl_allocator_t allocator = rcl_get_default_allocator();
    first_run = 1;
    count = 0;
    srand(time(NULL));

    // create init_options
    RCCHECK(rclc_support_init(&support, argc, argv, &allocator));

    // create rcl_node
    rcl_node_t node1 = rcl_get_zero_initialized_node();
    RCCHECK(rclc_node_init_default(&node1, "node_1", "rclc_app", &support));
  
    // create a publisher to publish topic 'topic1' with type std_msg::msg::String
    // node1_publisher1 is global, so that the callback can access this publisher.
    topic_name[0] = "topic1";
    topic_name[1] = "topic2";
    const rosidl_message_type_support_t * my_type_support =
      ROSIDL_GET_MSG_TYPE_SUPPORT(custom_interfaces, msg, Message);  
    int i;
    for (i = 0; i < NODE1_PUBLISHER_NUMBER; i++)
    {
      RCCHECK(rclc_publisher_init_default(&node1_publisher[i], &node1, my_type_support, topic_name[i]));    
    }

    // create a timer, which will call the publisher with period=`timer_timeout` ms in the 'node1_timer_callback'
    const unsigned int timer_timeout = 500;
    for (i = 0; i < NODE1_TIMER_NUMBER; i++)
    {
      node1_timer[i] = rcl_get_zero_initialized_timer();
      RCCHECK(rclc_timer_init_default(&node1_timer[i], &support, RCL_MS_TO_NS(timer_timeout), node1_timer_callback));

    }

    // assign message to publisher
    custom_interfaces__msg__Message__init(&pub_msg);

    // create subscription
    for (i = 0; i < NODE1_SUBSCRIBER_NUMBER; i++)
    {
      node1_subscriber[i] = rcl_get_zero_initialized_subscription();
      RCCHECK(rclc_subscription_init_default(&node1_subscriber[i], &node1, my_type_support, topic_name[i]));
      //printf("Created subscriber %s:\n", topic_name[i]);
    }


    // one string message for subscriber
    custom_interfaces__msg__Message__init(&sub_msg1);
    custom_interfaces__msg__Message__init(&sub_msg2);
    custom_interfaces__msg__Message__init(&sub_msg3);

    ////////////////////////////////////////////////////////////////////////////
    // Configuration of RCL Executor
    ////////////////////////////////////////////////////////////////////////////
    const uint64_t timeout_ns = 100000000;
    rclc_executor_t executor;
    executor = rclc_executor_get_zero_initialized_executor();
    
    unsigned int num_handles = 1 + 2; // 1 timer + 2 subs
    //printf("Debug: number of DDS handles: %u\n", num_handles);
    RCCHECK(rclc_executor_init(&executor, &support.context, num_handles, &allocator));

    // add subscription to executor
    RCCHECK(rclc_executor_add_subscription(
      &executor, &node1_subscriber[0], &sub_msg1, &node1_subscriber1_callback,
      ON_NEW_DATA));
    RCCHECK(rclc_executor_add_subscription(
      &executor, &node1_subscriber[1], &sub_msg2, &node1_subscriber2_callback,
      ON_NEW_DATA));

    RCCHECK(rclc_executor_add_timer(&executor, &node1_timer[0]));

    RCCHECK(rclc_executor_set_timeout(&executor,timeout_ns));

    ////////////////////////////////////////////////////////////////////////////
    // Configuration of Linux threads
    ////////////////////////////////////////////////////////////////////////////
    pthread_t thread1 = 0;
    int policy = SCHED_FIFO;

    /*
    struct arg_spin_period *ex1 = malloc(sizeof(struct arg_spin_period));
    ex1->period = 500*1000*1000;
    ex1->executor = &executor;

    struct arg_spin_period *ex2 = malloc(sizeof(struct arg_spin_period));
    ex2->period = 500*1000*1000;
    ex2->executor = &executor2;
*/
    struct arg_spin_period ex1 = {1000*1000*1000, &executor};
    thread_create(thread1, policy, 10, rclc_executor_spin_period_wrapper, &ex1);
    //thread_create(thread2, policy, 49, rclc_executor_spin_period_wrapper, &ex2);
    //thread_create(thread1, policy, 10, rclc_executor_spin_wrapper, &executor);
    //thread_create(thread2, policy, 49, rclc_executor_spin_wrapper, &executor2);

    //printf("Running experiment from now on for %ds\n", EXPERIMENT_DURATION);
    sleep_ms(EXPERIMENT_DURATION);
    exit_flag = true;
    printf("Stop experiment\n");
    void * status;
    // Wait for threads to finish
    pthread_join(thread1, &status);
    if (status !=0)
      printf("thread failed");
    else
    //pthread_join(thread2, NULL);
      printf("Start clean up\n");
    // clean up 
    //free(ex1);
    //free(ex2);
    RCCHECK(rclc_executor_fini(&executor));
    for (i = 0; i < NODE1_PUBLISHER_NUMBER; i++)
    {
      RCCHECK(rcl_publisher_fini(&node1_publisher[i], &node1));
    }
    printf("Start clean up\n");
    for (i = 0; i < NODE1_SUBSCRIBER_NUMBER; i++)
    {
      RCCHECK(rcl_subscription_fini(&node1_subscriber[i], &node1));
    }
    printf("Start clean up\n");
    for (i = 0; i < NODE1_TIMER_NUMBER; i++)
    {
      RCCHECK(rcl_timer_fini(&node1_timer[i]));
    }
    printf("Start clean up1\n");
    RCCHECK(rcl_node_fini(&node1));
printf("Start clean up2\n");
    RCCHECK(rclc_support_fini(&support));  
printf("Start clean up6\n");
    custom_interfaces__msg__Message__fini(&pub_msg);
    printf("Start clean up7\n");
    custom_interfaces__msg__Message__fini(&sub_msg1);  
    printf("Start clean up8\n");
    custom_interfaces__msg__Message__fini(&sub_msg2); 
    printf("Start clean up9\n");
    custom_interfaces__msg__Message__fini(&sub_msg3);  
    printf("Start clean up10\n");
    //print_timestamp(4,100, timestamp);

    
    // Start Executor
    //rclc_executor_spin(&executor);  
  return 0;
}
