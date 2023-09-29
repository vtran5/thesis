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
  const char * topic_name,
  rclc_executor_semantics_t semantics,
  const rcl_allocator_t * allocator)
{
  return rclc_publisher_init(
    publisher, node, type_support, topic_name,
    &rmw_qos_profile_default, semantics, allocator);
}

rcl_ret_t
rclc_publisher_init_best_effort(
  rclc_publisher_t * publisher,
  const rcl_node_t * node,
  const rosidl_message_type_support_t * type_support,
  const char * topic_name,
  rclc_executor_semantics_t semantics,
  const rcl_allocator_t * allocator)
{
  return rclc_publisher_init(
    publisher, node, type_support, topic_name,
    &rmw_qos_profile_sensor_data, semantics, allocator);
}

rcl_ret_t
rclc_publisher_init(
  rclc_publisher_t * publisher,
  const rcl_node_t * node,
  const rosidl_message_type_support_t * type_support,
  const char * topic_name,
  const rmw_qos_profile_t * qos_profile,
  rclc_executor_semantics_t semantics,
  const rcl_allocator_t * allocator)
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
  rcl_publisher_options_t option = rcl_publisher_get_default_options();
  option.qos = *qos_profile;

  rcl_ret_t rc = rcl_publisher_init(
    &publisher->rcl_publisher,
    node,
    type_support,
    topic_name,
    &option);
  if (rc != RCL_RET_OK) {
    PRINT_RCLC_ERROR(rclc_publisher_init_best_effort, rcl_publisher_init);
  }

  if (semantics == LET)
  {
    publisher->allocator = allocator;
    if(publisher->let_publisher != NULL)
      return RCL_RET_OK;
    publisher->let_publisher = allocator->allocate(sizeof(rclc_publisher_let_t), allocator->state);
    if(publisher->let_publisher == NULL)
      return RCL_RET_BAD_ALLOC;    

    publisher->let_publisher->let_publishers = NULL;
    publisher->let_publisher->executor_index = NULL;
    publisher->let_publisher->type_support = type_support;
    publisher->let_publisher->node = node;
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
  if (publisher->let_publisher == NULL)
  {
    printf("Invalid let_publisher\n");
    return RCL_RET_ERROR;
  }

  if (publisher->let_publisher->executor_index == NULL)
  {
    printf("Invalid executor_index\n");
    return RCL_RET_ERROR;
  }

  uint64_t executor_index = *(publisher->let_publisher->executor_index);
  int index = (int) (executor_index%publisher->let_publisher->num_period_per_let);

  if((publisher->let_publisher->let_publishers == NULL) || !rcl_publisher_is_valid(&(publisher->let_publisher->let_publishers[index])))
  {
    printf("Invalid publisher at index %d\n", index);
    return RCL_RET_ERROR;
  }

  rcl_ret_t ret = rcl_publish(&(publisher->let_publisher->let_publishers[index]), ros_message, allocation);
  printf("Publish %lu at index %d %ld\n", (unsigned long) publisher, index, executor_index);
  return ret;
}

rcl_ret_t
rclc_publish(
  rclc_publisher_t * publisher,
  const void * ros_message,
  rmw_publisher_allocation_t * allocation,
  rclc_executor_semantics_t semantics)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(publisher, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(ros_message, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret = RCL_RET_OK;
  rcutils_time_point_value_t now;
  if (semantics == LET)
  {
    ret = _rclc_publish_LET(publisher, ros_message, allocation);
  }
  else if (semantics == RCLCPP_EXECUTOR)
  {
    int64_t * temp = (int64_t *) ros_message;
    ret = rcutils_steady_time_now(&now);
    printf("Output %lu %ld %ld\n", (unsigned long) publisher, temp[1], now);
    ret = _rclc_publish_default(publisher, ros_message, allocation);
  }
  return ret;
}

rcl_ret_t
rclc_publisher_fini(rclc_publisher_t * publisher, rcl_node_t * node)
{
  rcl_ret_t ret = rcl_publisher_fini(&(publisher->rcl_publisher), node);
  return ret;
}

rcl_ret_t
rclc_publisher_let_fini(rclc_publisher_t * publisher)
{
  if (publisher->let_publisher != NULL)
  {
    if (publisher->let_publisher->let_publishers != NULL)
      publisher->allocator->deallocate(publisher->let_publisher->let_publishers, publisher->allocator->state);
    publisher->allocator->deallocate(publisher->let_publisher, publisher->allocator->state);
    publisher->let_publisher = NULL;
  }  
  return RCL_RET_OK;
}