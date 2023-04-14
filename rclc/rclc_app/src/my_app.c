//#include<iostream>
//#include<functional>
#include <stdio.h>
#include "custom_interfaces/msg/message.h"
#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <stdlib.h>
#include <time.h>

rcl_publisher_t node1_publisher1;
rcl_publisher_t node1_publisher2;
custom_interfaces__msg__Message pub_msg;
custom_interfaces__msg__Message sub_msg1;
custom_interfaces__msg__Message sub_msg2;
rclc_support_t support;
rcl_time_point_value_t start_time;
uint8_t first_run;
int64_t count;
//rcl_time_point_value_t timestamp[4][100] = {{0}};


// return value in nanoseconds
rcl_time_point_value_t rclc_now(rclc_support_t * support)
{
	rcl_time_point_value_t now = 0;
	rcl_ret_t rc = rcl_clock_get_now(&support->clock, &now);
   	if (rc != RCL_RET_OK) {
    	printf("rcl_clock_get_now: Error getting clock\n");
  	}
  	return now;
}

void print_timestamp(int dim0, int dim1, rcl_time_point_value_t timestamp[dim0][dim1])
{
    for (int i = 0; i < dim1; i++)
    {
        int printZero = ((i == 0) ? 1 : 0);
        char line[20000] = "";
        sprintf(line, "frame %zu: ", i);
        for (size_t j = 0; j < dim0; j++)
        {
            if (timestamp[j][i] == 0 && !printZero)
                break;
            printZero = 0;
            char temp[32];
            sprintf(temp, "%ld ", timestamp[j][i]);
            strcat(line, temp);
        }
        strcat(line, "\n");
        printf("%s", line);
    }
}

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
	rcl_ret_t rc;
	rcl_time_point_value_t now = rclc_now(&support);
	if (first_run)
	{
		start_time = now;
		first_run = 0;
	}
	pub_msg.frame_id = count++;
	pub_msg.stamp = start_time;

	RCLC_UNUSED(last_call_time);

	if (timer != NULL) {
		printf("Timer: Timer start\n");
    	//printf("Timer: time since last call %ld\n", (int) last_call_time);
    	rc = rcl_publish(&node1_publisher1, &pub_msg, NULL);
    	if (rc == RCL_RET_OK) {
      		printf("Published message %ld at time %ld \n", pub_msg.frame_id, now-start_time);
    	} else {
      		printf("timer_callback: Error publishing message %ld\n", pub_msg.frame_id);
    	}
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
    	printf("Node1_Sub1_Callback: I heard: %ld at time %ld\n", msg->frame_id, now-start_time);
  	}
  	busy_wait(100,300, &support);
	rcl_ret_t rc;
	rc = rcl_publish(&node1_publisher2, &msg, NULL);
	now = rclc_now(&support);
    if (rc == RCL_RET_OK) {
      	printf("Node1_Sub1_Callback: Published message %ld at time %ld\n", msg->frame_id, msg->stamp);
    } else {
      	printf("node1_sub1_callback: Error publishing message %ld\n", msg->frame_id);
    }
}

void node1_subscriber2_callback(const void * msgin)
{
  const custom_interfaces__msg__Message * msg = (const custom_interfaces__msg__Message *)msgin;
  if (msg == NULL) {
    printf("Callback: msg NULL\n");
  } else {
  	rcl_time_point_value_t now = rclc_now(&support);
    printf("Node1_Sub2_Callback: I heard: %ld at time %ld\n", msg->frame_id, msg->stamp);
  }
}

/******************** MAIN PROGRAM ****************************************/
int main(int argc, char const *argv[])
{
	rcl_allocator_t allocator = rcl_get_default_allocator();
  	rcl_ret_t rc;
  	first_run = 1;
  	count = 0;
  	srand(time(NULL));

    // create init_options
    rc = rclc_support_init(&support, argc, argv, &allocator);
    if (rc != RCL_RET_OK) {
      printf("Error rclc_support_init.\n");
      return -1;
    }  

    // create rcl_node
    rcl_node_t node1 = rcl_get_zero_initialized_node();
    rc = rclc_node_init_default(&node1, "node_1", "rclc_app", &support);
    if (rc != RCL_RET_OK) {
      printf("Error in rclc_node_init_default\n");
      return -1;
    }  	
	
    // create a publisher to publish topic 'topic1' with type std_msg::msg::String
    // node1_publisher1 is global, so that the callback can access this publisher.
    const char * topic_name1 = "topic1";
    const char * topic_name2 = "topic2";
    const rosidl_message_type_support_t * my_type_support =
      ROSIDL_GET_MSG_TYPE_SUPPORT(custom_interfaces, msg, Message);  

    rc = rclc_publisher_init_default(&node1_publisher1, &node1, my_type_support, topic_name1);
    if (RCL_RET_OK != rc) {
      printf("Error in rclc_publisher_init_default %s.\n", topic_name1);
      return -1;
    }

    rc = rclc_publisher_init_default(&node1_publisher2, &node1, my_type_support, topic_name2);
    if (RCL_RET_OK != rc) {
      printf("Error in rclc_publisher_init_default %s.\n", topic_name2);
      return -1;
    }

    // create a timer, which will call the publisher with period=`timer_timeout` ms in the 'node1_timer_callback'
    rcl_timer_t node1_timer = rcl_get_zero_initialized_timer();
    const unsigned int timer_timeout = 500;
    rc = rclc_timer_init_default(&node1_timer, &support, RCL_MS_TO_NS(timer_timeout), node1_timer_callback);
    if (rc != RCL_RET_OK) {
      printf("Error in rcl_timer_init_default.\n");
      return -1;
    } else {
      printf("Created timer with timeout %ld ms.\n", timer_timeout);
    }

    // assign message to publisher
    custom_interfaces__msg__Message__init(&pub_msg);

    // create subscription
    rcl_subscription_t node1_subscriber1 = rcl_get_zero_initialized_subscription();
    rc = rclc_subscription_init_default(&node1_subscriber1, &node1, my_type_support, topic_name1);
    if (rc != RCL_RET_OK) {
      printf("Failed to create subscriber %s.\n", topic_name1);
      return -1;
    } else {
      printf("Created subscriber %s:\n", topic_name1);
    }	

    rcl_subscription_t node1_subscriber2 = rcl_get_zero_initialized_subscription();
    rc = rclc_subscription_init_default(&node1_subscriber2, &node1, my_type_support, topic_name2);
    if (rc != RCL_RET_OK) {
      printf("Failed to create subscriber %s.\n", topic_name2);
      return -1;
    } else {
      printf("Created subscriber %s:\n", topic_name2);
    }	
    // one string message for subscriber
    custom_interfaces__msg__Message__init(&sub_msg1);
    custom_interfaces__msg__Message__init(&sub_msg2);

    ////////////////////////////////////////////////////////////////////////////
    // Configuration of RCL Executor
    ////////////////////////////////////////////////////////////////////////////
    rclc_executor_t executor;
    executor = rclc_executor_get_zero_initialized_executor();

    unsigned int num_handles = 1 + 2; // 1 timer + 2 subs
    printf("Debug: number of DDS handles: %u\n", num_handles);
    rclc_executor_init(&executor, &support.context, num_handles, &allocator);  

    // add subscription to executor
    rc = rclc_executor_add_subscription(
      &executor, &node1_subscriber1, &sub_msg1, &node1_subscriber1_callback,
      ON_NEW_DATA);
    rc += rclc_executor_add_subscription(
      &executor, &node1_subscriber2, &sub_msg2, &node1_subscriber2_callback,
      ON_NEW_DATA);
    if (rc != RCL_RET_OK) {
      printf("Error in rclc_executor_add_subscription. \n");
    }  

    rclc_executor_add_timer(&executor, &node1_timer);
    if (rc != RCL_RET_OK) {
      printf("Error in rclc_executor_add_timer.\n");
    }  

    // Start Executor
    rclc_executor_spin(&executor);  

    // clean up (never called in this example)
    rc = rclc_executor_fini(&executor);
    rc += rcl_publisher_fini(&node1_publisher1, &node1);
    rc += rcl_publisher_fini(&node1_publisher2, &node1);
    rc += rcl_timer_fini(&node1_timer);
    rc += rcl_subscription_fini(&node1_subscriber1, &node1);
    rc += rcl_subscription_fini(&node1_subscriber2, &node1);
    rc += rcl_node_fini(&node1);
    rc += rclc_support_fini(&support);  

    custom_interfaces__msg__Message__fini(&pub_msg);
    custom_interfaces__msg__Message__fini(&sub_msg1);  
    custom_interfaces__msg__Message__fini(&sub_msg2);  

    if (rc != RCL_RET_OK) {
      printf("Error while cleaning up!\n");
      return -1;
    }
	return 0;
}