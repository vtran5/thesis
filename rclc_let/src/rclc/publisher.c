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
  const char * topic_name,
  const int message_size,
  const int buffer_capacity)
{
  return rclc_publisher_init(
    publisher, node, type_support, topic_name, message_size, buffer_capacity,
    &rmw_qos_profile_default);
}

rcl_ret_t
rclc_publisher_init_best_effort(
  rclc_publisher_t * publisher,
  const rcl_node_t * node,
  const rosidl_message_type_support_t * type_support,
  const char * topic_name,
  const int message_size,
  const int buffer_capacity)
{
  return rclc_publisher_init(
    publisher, node, type_support, topic_name, message_size, buffer_capacity,
    &rmw_qos_profile_sensor_data);
}

rcl_ret_t
rclc_publisher_init(
  rclc_publisher_t * publisher,
  const rcl_node_t * node,
  const rosidl_message_type_support_t * type_support,
  const char * topic_name,
  const int message_size,
  const int buffer_capacity,
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
  if (message_size <= 0 || buffer_capacity <= 0)
    return RCL_RET_INVALID_ARGUMENT;
  rcl_ret_t rc = rclc_init_circular_queue(&(publisher->message_buffer), message_size, buffer_capacity);
  if (rc != RCL_RET_OK)
    return rc;
  publisher->rcl_publisher = rcl_get_zero_initialized_publisher();
  rcl_publisher_options_t pub_opt = rcl_publisher_get_default_options();
  pub_opt.qos = *qos_profile;
  rc = rcl_publisher_init(
    &(publisher->rcl_publisher),
    node,
    type_support,
    topic_name,
    &pub_opt);
  if (rc != RCL_RET_OK) {
    PRINT_RCLC_ERROR(rclc_publisher_init_best_effort, rcl_publisher_init);
  }
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
  rcl_ret_t ret = rclc_enqueue(&(publisher->message_buffer), ros_message);
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
  if (semantics == LET)
  {
    ret = _rclc_publish_LET(publisher, ros_message, allocation);
  }
  else if (semantics == RCLCPP_EXECUTOR)
  {
    ret = _rclc_publish_default(publisher, ros_message, allocation);
  }
  return ret;
}

rcl_ret_t
rclc_LET_output(rclc_publisher_t * publisher)
{
  rcl_ret_t ret = RCL_RET_OK;
  rcutils_time_point_value_t now;
  while(!rclc_is_empty_circular_queue(&(publisher->message_buffer)))
  {
    ret = rcutils_steady_time_now(&now);
    printf("Publisher %lu %ld\n", (unsigned long) publisher, now);
    unsigned char array[publisher->message_buffer.elem_size];
    rclc_dequeue(&(publisher->message_buffer), array);
    ret = rcl_publish(&(publisher->rcl_publisher), array, NULL);
  }
  return ret;
}

rcl_ret_t
rclc_publisher_fini(rclc_publisher_t * publisher, rcl_node_t * node)
{
  rcl_ret_t rc = rclc_fini_circular_queue(&(publisher->message_buffer));
  rc = rcl_publisher_fini(&(publisher->rcl_publisher), node);
  return rc;
}
