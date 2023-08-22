// Copyright (c) 2019 - for information on the respective copyright owner
// see the NOTICE file and/or the repository https://github.com/ros2/rclc.
// Copyright 2014 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "rclc/publisher.h"
#include <rcutils/time.h>
#include <rcl/error_handling.h>
#include <rcutils/logging_macros.h>
#include <rmw/qos_profiles.h>

rcl_ret_t
rclc_publisher_init_default(
  rclc_publisher_t * publisher,
  const rcl_node_t * node,
  const rosidl_message_type_support_t * type_support,
  const char * topic_name)
{
  return rclc_publisher_init(
    publisher, node, type_support, topic_name, &rmw_qos_profile_default);
}

rcl_ret_t
rclc_publisher_init_best_effort(
  rclc_publisher_t * publisher,
  const rcl_node_t * node,
  const rosidl_message_type_support_t * type_support,
  const char * topic_name)
{
  return rclc_publisher_init(
    publisher, node, type_support, topic_name, &rmw_qos_profile_sensor_data);
}

rcl_ret_t
rclc_publisher_init(
  rclc_publisher_t * publisher,
  const rcl_node_t * node,
  const rosidl_message_type_support_t * type_support,
  const char * topic_name,
  const rmw_qos_profile_t * qos_profile)
{
  RCL_CHECK_FOR_NULL_WITH_MSG(
    publisher, "publisher is a null pointer", return RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_FOR_NULL_WITH_MSG(
    node, "node is a null pointer", return RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_FOR_NULL_WITH_MSG(
    type_support, "type_support is a null pointer", return RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_FOR_NULL_WITH_MSG(
    topic_name, "topic_name is a null pointer", return RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_FOR_NULL_WITH_MSG(
    qos_profile, "qos_profile is a null pointer", return RCL_RET_INVALID_ARGUMENT);

  publisher->rcl_publisher = rcl_get_zero_initialized_publisher();
  rcl_publisher_options_t pub_opt = rcl_publisher_get_default_options();
  pub_opt.qos = *qos_profile;
  rcl_ret_t rc = rcl_publisher_init(
    &(publisher->rcl_publisher),
    node,
    type_support,
    topic_name,
    &pub_opt);
  if (rc != RCL_RET_OK) {
    PRINT_RCLC_ERROR(rclc_publisher_init_best_effort, rcl_publisher_init);
  }
  publisher->message_buffer = NULL;
  return rc;
}

rcl_ret_t
_rclc_publish_default(
  rclc_publisher_t * publisher,
  const void * ros_message,
  rmw_publisher_allocation_t * allocation)
{
  rcl_ret_t ret = rcl_publish(&(publisher->rcl_publisher), ros_message, allocation);
  return ret;
}

rcl_ret_t
_rclc_publish_LET(
  rclc_publisher_t * publisher,
  const void * ros_message,
  rmw_publisher_allocation_t * allocation)
{
  RCLC_UNUSED(allocation);
  uint64_t executor_index = *(publisher->executor_index);
  int buffer_size = publisher->message_buffer->size;
  int index = (int) (executor_index%buffer_size);
  printf("Publish %lu at index %d %ld %d\n", (unsigned long) publisher, index, executor_index, buffer_size);
  rcl_ret_t ret = rclc_enqueue_2d_circular_queue(publisher->message_buffer, 
                  ros_message, index);
  return ret;
}

rcl_ret_t
rclc_publish(
  rclc_publisher_t * publisher,
  const void * ros_message,
  rmw_publisher_allocation_t * allocation,
  rclc_executor_semantics_t semantics)
{
  rcl_ret_t ret = RCL_RET_OK;
  rcutils_time_point_value_t now;
  if (semantics == LET)
  {
    ret = _rclc_publish_LET(publisher, ros_message, allocation);
  }
  else if (semantics == RCLCPP_EXECUTOR)
  {
    ret = rcutils_steady_time_now(&now);
    printf("Output %lu %ld\n", (unsigned long) publisher, now);
    ret = _rclc_publish_default(publisher, ros_message, allocation);
  }
  return ret;
}
rcl_ret_t
rclc_publisher_let_init(
  rclc_publisher_t * publisher,
  const int message_size,
  const int _1d_capacity,
  const int _2d_capacity,
  uint64_t * executor_index_ptr,
  const rcl_allocator_t * allocator)
{
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "allocator is NULL", return RCL_RET_INVALID_ARGUMENT);
  if (message_size <= 0 || _1d_capacity <= 0 || _2d_capacity <= 0 || executor_index_ptr == NULL)
    return RCL_RET_INVALID_ARGUMENT;
  publisher->message_buffer = allocator->allocate(sizeof(rclc_2d_circular_queue_t), allocator->state);
  publisher->executor_index = executor_index_ptr;
  publisher->allocator = allocator;
  rcl_ret_t rc = rclc_init_2d_circular_queue(publisher->message_buffer, _2d_capacity, message_size, _1d_capacity, allocator); 
  if (rc != RCL_RET_OK)
    printf("Init 2d queue fail\n");
  return rc;
}


rcl_ret_t
rclc_LET_output(rclc_publisher_t * publisher, int queue_index)
{
  rcl_ret_t ret = RCL_RET_OK;
  rcutils_time_point_value_t now;
  ret = rcutils_steady_time_now(&now);

  while(!rclc_is_empty_circular_queue(rclc_get_queue(publisher->message_buffer, queue_index)))
  {
    void * array;
    rclc_dequeue_2d_circular_queue(publisher->message_buffer, &array, queue_index);
    int64_t * ptr = array;
    printf("Output %lu %ld %ld\n", (unsigned long) publisher, ptr[1], now);
    ret = rcl_publish(&(publisher->rcl_publisher), array, NULL);
  }
  return ret;
}

rcl_ret_t
rclc_publisher_fini(rclc_publisher_t * publisher, rcl_node_t * node)
{
  rcl_ret_t rc = RCL_RET_OK;
  if (publisher->message_buffer != NULL)
  {
    rc = rclc_fini_2d_circular_queue(publisher->message_buffer);
    publisher->allocator->deallocate(publisher->message_buffer, publisher->allocator->state);
  }
    
  rc = rcl_publisher_fini(&(publisher->rcl_publisher), node);
  return rc;
}

rcl_ret_t
rclc_publisher_check_buffer_state(rclc_publisher_t * publisher, 
  int queue_index, rclc_queue_state_t * state)
{
  rclc_circular_queue_t * queue = rclc_get_queue(publisher->message_buffer, queue_index);

  if (queue == NULL)
    return RCL_RET_ERROR;
  *state = queue->state;
  return RCL_RET_OK;
}

rcl_ret_t
rclc_publisher_flush_buffer(rclc_publisher_t * publisher,
  int queue_index)
{
  rclc_circular_queue_t * queue = rclc_get_queue(publisher->message_buffer, queue_index);
  rcl_ret_t ret = rclc_flush_circular_queue(queue);
  return ret;
}

