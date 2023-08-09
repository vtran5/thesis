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
#ifndef RCLC__PUBLISHER_H_
#define RCLC__PUBLISHER_H_

#if __cplusplus
extern "C"
{
#endif

#include <rcl/publisher.h>
// #include<rosidl_generator_c/message_type_support_struct.h>
// #include<rcl/node.h>
#include <rcl/allocator.h>
#include <rclc/types.h>
#include "rclc/visibility_control.h"
#include "rclc/buffer.h"


typedef struct rclc_publisher_s
{
  rcl_publisher_t rcl_publisher;
  rclc_2d_circular_queue_t message_buffer;
  uint64_t * executor_index; // This should point to the executor spin_index
} rclc_publisher_t;


/**
 *  Creates an rcl publisher.
 *
 *  * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes (in RCL)
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param[inout] publisher a zero_initialized rcl_publisher_t
 * \param[in] node the rcl node
 * \param[in] type_support the message data type
 * \param[in] topic_name the name of published topic
 * \return `RCL_RET_OK` if successful
 * \return `RCL_ERROR` (or other error code) if an error has occurred
 */
RCLC_PUBLIC
rcl_ret_t
rclc_publisher_init_default(
  rclc_publisher_t * publisher,
  const rcl_node_t * node,
  const rosidl_message_type_support_t * type_support,
  const char * topic_name);

/**
 *  Creates an rcl publisher with quality-of-service option best effort
 *
 *  * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes (in RCL)
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param[inout] publisher a zero_initialized rcl_publisher_t
 * \param[in] node the rcl node
 * \param[in] type_support the message data type
 * \param[in] topic_name the name of published topic
 * \return `RCL_RET_OK` if successful
 * \return `RCL_ERROR` (or other error code) if an error has occurred
 */
RCLC_PUBLIC
rcl_ret_t
rclc_publisher_init_best_effort(
  rclc_publisher_t * publisher,
  const rcl_node_t * node,
  const rosidl_message_type_support_t * type_support,
  const char * topic_name);

/**
 *  Creates an rcl publisher with defined QoS
 *
 *  * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes (in RCL)
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param[inout] publisher a zero_initialized rcl_publisher_t
 * \param[in] node the rcl node
 * \param[in] type_support the message data type
 * \param[in] topic_name the name of published topic
 * \param[in] qos_profile the qos of the topic
 * \return `RCL_RET_OK` if successful
 * \return `RCL_ERROR` (or other error code) if an error has occurred
 */
RCLC_PUBLIC
rcl_ret_t
rclc_publisher_init(
  rclc_publisher_t * publisher,
  const rcl_node_t * node,
  const rosidl_message_type_support_t * type_support,
  const char * topic_name,
  const rmw_qos_profile_t * qos_profile);

RCLC_PUBLIC
rcl_ret_t
rclc_publisher_let_init(
  rclc_publisher_t * publisher,
  const int message_size,
  const int _1d_capacity,
  const int _2d_capacity,
  uint64_t * executor_index_ptr);

RCLC_PUBLIC
rcl_ret_t
rclc_publish(
  rclc_publisher_t * publisher,
  const void * ros_message,
  rmw_publisher_allocation_t * allocation,
  rclc_executor_semantics_t semantics);

RCLC_PUBLIC
rcl_ret_t
rclc_LET_output(rclc_publisher_t * publisher,
  int queue_index);

RCLC_PUBLIC
rcl_ret_t
rclc_publisher_fini(rclc_publisher_t * publisher, 
  rcl_node_t * node);

RCLC_PUBLIC
rcl_ret_t
rclc_publisher_check_buffer_state(rclc_publisher_t * publisher, 
  int queue_index,
  rclc_queue_state_t * state);

RCLC_PUBLIC
rcl_ret_t
rclc_publisher_flush_buffer(rclc_publisher_t * publisher,
  int queue_index);


#if __cplusplus
}
#endif

#endif  // RCLC__PUBLISHER_H_
