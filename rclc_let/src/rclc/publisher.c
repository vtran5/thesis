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
#include <string.h>
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
    publisher, node, type_support, topic_name,
    &rmw_qos_profile_default);
}

rcl_ret_t
rclc_publisher_init_best_effort(
  rclc_publisher_t * publisher,
  const rcl_node_t * node,
  const rosidl_message_type_support_t * type_support,
  const char * topic_name)
{
  return rclc_publisher_init(
    publisher, node, type_support, topic_name,
    &rmw_qos_profile_sensor_data);
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
  publisher->option = rcl_publisher_get_default_options();
  publisher->option.qos = *qos_profile;
  publisher->type_support = type_support;
  publisher->node = node;
  publisher->topic_name = publisher->option.allocator.allocate(strlen(topic_name) + 1, publisher->option.allocator.state);
  if (publisher->topic_name == NULL)
    return RCL_RET_BAD_ALLOC;
  strcpy(publisher->topic_name, topic_name);

  rcl_ret_t rc = rcl_publisher_init(
    &publisher->rcl_publisher,
    node,
    type_support,
    topic_name,
    &publisher->option);
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
  uint64_t executor_index = *(publisher->executor_index);
  int index = (int) (executor_index%publisher->num_period_per_let);
  if((publisher->let_publishers == NULL) || !rcl_publisher_is_valid(&(publisher->let_publishers[index])))
  {
    printf("Invalid publisher at index %d\n", index);
    return RCL_RET_ERROR;
  }
  rcl_ret_t ret = rcl_publish(&(publisher->let_publishers[index]), ros_message, allocation);
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
    printf("Publisher %lu %ld\n", (unsigned long) publisher, now);
    ret = _rclc_publish_default(publisher, ros_message, allocation);
  }
  return ret;
}

rcl_ret_t
rclc_publisher_fini(rclc_publisher_t * publisher)
{
  if (publisher->topic_name == NULL)
    return RCL_RET_OK;
  
  publisher->option.allocator.deallocate(publisher->topic_name, publisher->option.allocator.state);
  publisher->topic_name = NULL;
  rcl_ret_t ret = rcl_publisher_fini(&(publisher->rcl_publisher), publisher->node);
  return ret;
}