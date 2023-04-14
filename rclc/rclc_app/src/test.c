//#include<iostream>
//#include<functional>
#include <stdio.h>
#include "custom_interfaces/msg/message.h"
#include <rclc/executor.h>
#include <rclc/rclc.h>

rcl_publisher_t node1_publisher1;
rcl_publisher_t node1_publisher2;
custom_interfaces__msg__Message pub_msg;
custom_interfaces__msg__Message sub_msg;
rclc_support_t support;
rcl_time_point_value_t start_time;
uint8_t first_run;
int64_t count;

rcl_time_point_value_t rclc_now(rclc_support_t * support)
{
	rcl_time_point_value_t now = 0;
	rcl_ret_t rc = rcl_clock_get_now(&support->clock, &now);
   	if (rc != RCL_RET_OK) {
    	printf("rcl_clock_get_now: Error getting clock\n");
  	}
  	return now;
}

/******************** MAIN PROGRAM ****************************************/
int main(int argc, char const *argv[])
{
	   rcl_allocator_t allocator = rcl_get_default_allocator();
  	rcl_ret_t rc;
  	first_run = 1;
  	count = 0;

    // create init_options
    rc = rclc_support_init(&support, argc, argv, &allocator);
    if (rc != RCL_RET_OK) {
      printf("Error rclc_support_init.\n");
      return -1;
    }  

    rcl_time_point_value_t now = rclc_now(&support);
    printf("rcl_clock_get_now: now is %d and %f ns\n", now, now);

    // create rcl_node
    rcl_node_t node1 = rcl_get_zero_initialized_node();
    rc = rclc_node_init_default(&node1, "node_1", "rclc_app", &support);
    if (rc != RCL_RET_OK) {
      printf("Error in rclc_node_init_default\n");
      return -1;
    }  	
	
    rc += rcl_node_fini(&node1);
    rc += rclc_support_fini(&support);  

    if (rc != RCL_RET_OK) {
      printf("Error while cleaning up!\n");
      return -1;
    }
	return 0;
}