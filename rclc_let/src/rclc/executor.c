// Copyright (c) 2020 - for information on the respective copyright owner
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

#include "rclc/executor.h"
#include <rcutils/time.h>

#include "./action_generic_types.h"
#include "./action_goal_handle_internal.h"
#include "./action_client_internal.h"
#include "./action_server_internal.h"

#include "rclc/publisher.h"

// Include backport of function 'rcl_wait_set_is_valid' introduced in Foxy
// in case of building for Dashing and Eloquent. This pre-processor macro
// is defined in CMakeLists.txt.
#if defined (USE_RCL_WAIT_SET_IS_VALID_BACKPORT)
#include "rclc/rcl_wait_set_is_valid_backport.h"
#endif

// default timeout for rcl_wait() is 1000ms
#define DEFAULT_WAIT_TIMEOUT_NS 1000000000

// declarations of helper functions
/*
/// get new data from DDS queue for handle i
static
rcl_ret_t
_rclc_check_for_new_data(rclc_executor_t * executor, rcl_wait_set_t * wait_set, size_t i);

/// get new data from DDS queue for handle i
static
rcl_ret_t
_rclc_take_new_data(rclc_executor_t * executor, rcl_wait_set_t * wait_set, size_t i);


/// execute callback of handle i
static
rcl_ret_t
_rclc_execute(rclc_executor_t * executor, rcl_wait_set_t * wait_set, size_t i);

static
rcl_ret_t
_rclc_let_scheduling(rclc_executor_t * executor, rcl_wait_set_t * wait_set);
*/
static rcl_ret_t _rclc_set_state_let_buffer(rclc_executor_handle_t * handle, int index, rclc_queue_state_t state);
static void * _rclc_let_scheduling_output_wrapper(void * arg);
static void * _rclc_let_scheduling_input_wrapper(void * arg);
static void * _rclc_let_spin_some_wrapper(void * arg);
static rcl_ret_t _rclc_add_handles_to_waitset(rclc_executor_t * executor);
static rcl_ret_t _rclc_wait_for_new_input(rclc_executor_t * executor, const uint64_t timeout_ns);
static bool _rclc_executor_check_overrun_error(rclc_executor_t * executor);
void _rclc_thread_create(pthread_t *thread_id, int policy, int priority, int cpu_id, void *(*function)(void *), void * arg);

// rationale: user must create an executor with:
// executor = rclc_executor_get_zero_initialized_executor();
// then handles==NULL or not (e.g. properly initialized)
static
bool
_rclc_executor_is_valid(rclc_executor_t * executor)
{
  RCL_CHECK_FOR_NULL_WITH_MSG(executor, "executor pointer is invalid", return false);
  RCL_CHECK_FOR_NULL_WITH_MSG(
    executor->handles, "handle pointer is invalid", return false);
  RCL_CHECK_FOR_NULL_WITH_MSG(
    executor->allocator, "allocator pointer is invalid", return false);
  if (executor->max_handles == 0) {
    return false;
  }

  return true;
}

// wait_set and rclc_executor_handle_size_t are structs and cannot be statically
// initialized here.
rclc_executor_t
rclc_executor_get_zero_initialized_executor()
{
  static rclc_executor_t null_executor = {
    .context = NULL,
    .handles = NULL,
    .max_handles = 0,
    .index = 0,
    .allocator = NULL,
    .timeout_ns = 0,
    .invocation_time = 0,
    .trigger_function = NULL,
    .trigger_object = NULL,
    .period_ns = 0
  };
  return null_executor;
}

rcl_ret_t
rclc_executor_init(
  rclc_executor_t * executor,
  rcl_context_t * context,
  const size_t number_of_handles,
  const rcl_allocator_t * allocator)
{
  RCL_CHECK_FOR_NULL_WITH_MSG(executor, "executor is NULL", return RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_FOR_NULL_WITH_MSG(context, "context is NULL", return RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "allocator is NULL", return RCL_RET_INVALID_ARGUMENT);

  if (number_of_handles == 0) {
    RCL_SET_ERROR_MSG("number_of_handles is 0. Must be larger or equal to 1");
    return RCL_RET_INVALID_ARGUMENT;
  }

  rcl_ret_t ret = RCL_RET_OK;
  (*executor) = rclc_executor_get_zero_initialized_executor();
  executor->context = context;
  executor->max_handles = number_of_handles;
  executor->index = 0;
  executor->wait_set = rcl_get_zero_initialized_wait_set();
  executor->allocator = allocator;
  executor->timeout_ns = DEFAULT_WAIT_TIMEOUT_NS;
  // allocate memory for the array
  executor->handles =
    executor->allocator->allocate(
    (number_of_handles * sizeof(rclc_executor_handle_t)),
    executor->allocator->state);
  if (NULL == executor->handles) {
    RCL_SET_ERROR_MSG("Could not allocate memory for 'handles'.");
    return RCL_RET_BAD_ALLOC;
  }

  // initialize handle
  for (size_t i = 0; i < number_of_handles; i++) {
    rclc_executor_handle_init(&executor->handles[i], number_of_handles);
  }

  // initialize #counts for handle types
  rclc_executor_handle_counters_zero_init(&executor->info);

  // default: trigger_any which corresponds to the ROS2 rclcpp Executor semantics
  //          start processing if any handle has new data/or is ready
  rclc_executor_set_trigger(executor, rclc_executor_trigger_any, NULL);

  // default semantics
  rclc_executor_set_semantics(executor, RCLCPP_EXECUTOR);

  return ret;
}

rcl_ret_t
rclc_executor_set_timeout(rclc_executor_t * executor, const uint64_t timeout_ns)
{
  RCL_CHECK_FOR_NULL_WITH_MSG(
    executor, "executor is null pointer", return RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret = RCL_RET_OK;
  if (_rclc_executor_is_valid(executor)) {
    executor->timeout_ns = timeout_ns;
  } else {
    RCL_SET_ERROR_MSG("executor not initialized.");
    return RCL_RET_ERROR;
  }
  return ret;
}

rcl_ret_t
rclc_executor_set_semantics(rclc_executor_t * executor, rclc_executor_semantics_t semantics)
{
  RCL_CHECK_FOR_NULL_WITH_MSG(
    executor, "executor is null pointer", return RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret = RCL_RET_OK;
  if (_rclc_executor_is_valid(executor)) {
    executor->data_comm_semantics = semantics;
  } else {
    RCL_SET_ERROR_MSG("executor not initialized.");
    return RCL_RET_ERROR;
  }
  return ret;
}


rcl_ret_t
rclc_executor_fini(rclc_executor_t * executor)
{
  if (_rclc_executor_is_valid(executor)) {
    executor->allocator->deallocate(executor->handles, executor->allocator->state);
    executor->handles = NULL;
    executor->max_handles = 0;
    executor->index = 0;
    rclc_executor_handle_counters_zero_init(&executor->info);
    // free memory of wait_set if it has been initialized
    // calling it with un-initialized wait_set will fail.
    if (rcl_wait_set_is_valid(&executor->wait_set)) {
      rcl_ret_t rc = rcl_wait_set_fini(&executor->wait_set);
      if (rc != RCL_RET_OK) {
        PRINT_RCLC_ERROR(rclc_executor_fini, rcl_wait_set_fini);
      }
    }
    executor->timeout_ns = DEFAULT_WAIT_TIMEOUT_NS;
  } else {
    // Repeated calls to fini or calling fini on a zero initialized executor is ok
  }
  return RCL_RET_OK;
}

rcl_ret_t
rclc_executor_add_subscription(
  rclc_executor_t * executor,
  rcl_subscription_t * subscription,
  void * msg,
  rclc_subscription_callback_t callback,
  rclc_executor_handle_invocation_t invocation,
  rcutils_time_point_value_t callback_let_ns,
  int message_size)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(subscription, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(msg, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(callback, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret = RCL_RET_OK;
  // array bound check
  if (executor->index >= executor->max_handles) {
    RCL_SET_ERROR_MSG("Buffer overflow of 'executor->handles'. Increase 'max_handles'");
    return RCL_RET_ERROR;
  }
  // assign data fields
  executor->handles[executor->index].type = RCLC_SUBSCRIPTION;
  executor->handles[executor->index].subscription = subscription;
  executor->handles[executor->index].data = msg;
  executor->handles[executor->index].subscription_callback = callback;
  executor->handles[executor->index].invocation = invocation;
  executor->handles[executor->index].initialized = true;
  executor->handles[executor->index].callback_context = NULL;
  executor->handles[executor->index].data_available = false;

  if(executor->data_comm_semantics == LET && executor->period_ns > 0)
  {
    executor->handles[executor->index].callback_info->callback_id = executor->let_executor->next_callback_id;
    executor->let_executor->next_callback_id++;
    executor->handles[executor->index].callback_info->callback_let_ns = callback_let_ns;
    int num_period_per_let = (callback_let_ns/executor->period_ns) + 1;
    bool data_available = false;
    rclc_callback_state_t state = INACTIVE;
    const rcl_allocator_t * allocator = executor->allocator;
    executor->handles[executor->index].callback_info->num_period_per_let = num_period_per_let;
    CHECK_RCL_RET(rclc_init_array(&(executor->handles[executor->index].callback_info->data), message_size, num_period_per_let, allocator),
                    (unsigned long) executor);
    CHECK_RCL_RET(rclc_init_array(&(executor->handles[executor->index].callback_info->data_available), sizeof(bool), num_period_per_let, allocator),
                    (unsigned long) executor);
    CHECK_RCL_RET(rclc_init_array(&(executor->handles[executor->index].callback_info->state), sizeof(rclc_callback_state_t), num_period_per_let, allocator),
                    (unsigned long) executor);

    for (int i = 0; i < num_period_per_let; i++)
    {
      CHECK_RCL_RET(rclc_set_array(&(executor->handles[executor->index].callback_info->state), &state, i),
                      (unsigned long) executor);
      CHECK_RCL_RET(rclc_set_array(&(executor->handles[executor->index].callback_info->data_available), &data_available, i),
                      (unsigned long) executor);
    }  
  }
  else if (executor->data_comm_semantics == LET)
  {
    executor->handles[executor->index].callback_info->data.buffer = NULL;
    executor->handles[executor->index].callback_info->data_available.buffer = NULL;
    executor->handles[executor->index].callback_info->state.buffer = NULL;
  }

  // increase index of handle array
  executor->index++;

  // invalidate wait_set so that in next spin_some() call the
  // 'executor->wait_set' is updated accordingly
  if (rcl_wait_set_is_valid(&executor->wait_set)) {
    ret = rcl_wait_set_fini(&executor->wait_set);
    if (RCL_RET_OK != ret) {
      RCL_SET_ERROR_MSG("Could not reset wait_set in rclc_executor_add_subscription.");
      return ret;
    }
  }

  executor->info.number_of_subscriptions++;

  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Added a subscription.");
  return ret;
}

rcl_ret_t
rclc_executor_add_subscription_with_context(
  rclc_executor_t * executor,
  rcl_subscription_t * subscription,
  void * msg,
  rclc_subscription_callback_with_context_t callback,
  void * context,
  rclc_executor_handle_invocation_t invocation,
  rcutils_time_point_value_t callback_let_ns,
  int message_size)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(subscription, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(msg, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(callback, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret = RCL_RET_OK;
  // array bound check
  if (executor->index >= executor->max_handles) {
    RCL_SET_ERROR_MSG("Buffer overflow of 'executor->handles'. Increase 'max_handles'");
    return RCL_RET_ERROR;
  }

  // assign data fields
  executor->handles[executor->index].type = RCLC_SUBSCRIPTION_WITH_CONTEXT;
  executor->handles[executor->index].subscription = subscription;
  executor->handles[executor->index].data = msg;
  executor->handles[executor->index].subscription_callback_with_context = callback;
  executor->handles[executor->index].invocation = invocation;
  executor->handles[executor->index].initialized = true;
  executor->handles[executor->index].callback_context = context;

  if(executor->data_comm_semantics == LET && executor->period_ns > 0)
  {
    executor->handles[executor->index].callback_info->callback_id = executor->let_executor->next_callback_id;
    executor->let_executor->next_callback_id++;
    executor->handles[executor->index].callback_info->callback_let_ns = callback_let_ns;
    int num_period_per_let = (callback_let_ns/executor->period_ns) + 1;
    
    bool data_available = false;
    rclc_callback_state_t state = INACTIVE;
    executor->handles[executor->index].callback_info->num_period_per_let = num_period_per_let;
    const rcl_allocator_t * allocator = executor->allocator;
    CHECK_RCL_RET(rclc_init_array(&(executor->handles[executor->index].callback_info->data), message_size, num_period_per_let, allocator),
                    (unsigned long) executor);
    CHECK_RCL_RET(rclc_init_array(&(executor->handles[executor->index].callback_info->data_available), sizeof(bool), num_period_per_let, allocator),
                    (unsigned long) executor);
    CHECK_RCL_RET(rclc_init_array(&(executor->handles[executor->index].callback_info->state), sizeof(rclc_callback_state_t), num_period_per_let, allocator),
                    (unsigned long) executor);

    for (int i = 0; i < num_period_per_let; i++)
    {
      CHECK_RCL_RET(rclc_set_array(&(executor->handles[executor->index].callback_info->state), &state, i),
                      (unsigned long) executor);
      CHECK_RCL_RET(rclc_set_array(&(executor->handles[executor->index].callback_info->data_available), &data_available, i),
                      (unsigned long) executor);
    }  
  }
  else if (executor->data_comm_semantics == LET)
  {
    executor->handles[executor->index].callback_info->data.buffer = NULL;
    executor->handles[executor->index].callback_info->data_available.buffer = NULL;
    executor->handles[executor->index].callback_info->state.buffer = NULL;
  }
  // increase index of handle array
  executor->index++;

  // invalidate wait_set so that in next spin_some() call the
  // 'executor->wait_set' is updated accordingly
  if (rcl_wait_set_is_valid(&executor->wait_set)) {
    ret = rcl_wait_set_fini(&executor->wait_set);
    if (RCL_RET_OK != ret) {
      RCL_SET_ERROR_MSG("Could not reset wait_set in rclc_executor_add_subscription_with_context.");
      return ret;
    }
  }
  
  executor->info.number_of_subscriptions++;
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Added a subscription.");
  return ret;
}

rcl_ret_t
rclc_executor_add_timer_with_context(
  rclc_executor_t * executor,
  rcl_timer_t * timer,
  rclc_timer_callback_with_context_t callback,
  void * context,
  rcutils_time_point_value_t callback_let_ns)
{
  rcl_ret_t ret = RCL_RET_OK;

  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(timer, RCL_RET_INVALID_ARGUMENT);

  // array bound check
  if (executor->index >= executor->max_handles) {
    rcl_ret_t ret = RCL_RET_ERROR;     // TODO(jst3si) better name : rclc_RET_BUFFER_OVERFLOW
    RCL_SET_ERROR_MSG("Buffer overflow of 'executor->handles'. Increase 'max_handles'");
    return ret;
  }

  // assign data fields
  executor->handles[executor->index].type = RCLC_TIMER_WITH_CONTEXT;
  executor->handles[executor->index].timer = timer;
  executor->handles[executor->index].invocation = ON_NEW_DATA;  // i.e. when timer elapsed
  executor->handles[executor->index].initialized = true;
  executor->handles[executor->index].timer_callback_with_context = callback;
  executor->handles[executor->index].callback_context = context;
  executor->handles[executor->index].data_available = false;

  if(executor->data_comm_semantics == LET && executor->period_ns > 0)
  {
    executor->handles[executor->index].callback_info->callback_id = executor->let_executor->next_callback_id;
    executor->let_executor->next_callback_id++;
    executor->handles[executor->index].callback_info->callback_let_ns = callback_let_ns;
    int num_period_per_let = (callback_let_ns/executor->period_ns) + 1;
    bool data_available = false;
    rclc_callback_state_t state = INACTIVE;
    executor->handles[executor->index].callback_info->num_period_per_let = num_period_per_let;
    executor->handles[executor->index].callback_info->data.buffer = NULL;
    const rcl_allocator_t * allocator = executor->allocator;
    CHECK_RCL_RET(rclc_init_array(&(executor->handles[executor->index].callback_info->data_available), sizeof(bool), num_period_per_let, allocator),
                    (unsigned long) executor);
    CHECK_RCL_RET(rclc_init_array(&(executor->handles[executor->index].callback_info->state), sizeof(rclc_callback_state_t), num_period_per_let, allocator),
                    (unsigned long) executor);
    for (int i = 0; i < num_period_per_let; i++)
    {
      CHECK_RCL_RET(rclc_set_array(&(executor->handles[executor->index].callback_info->state), &state, i),
                      (unsigned long) executor);
      CHECK_RCL_RET(rclc_set_array(&(executor->handles[executor->index].callback_info->data_available), &data_available, i),
                      (unsigned long) executor);
    }  
  }
  else if (executor->data_comm_semantics == LET)
  {
    executor->handles[executor->index].callback_info->data.buffer = NULL;
    executor->handles[executor->index].callback_info->data_available.buffer = NULL;
    executor->handles[executor->index].callback_info->state.buffer = NULL;
  }
  
  // increase index of handle array
  executor->index++;

  // invalidate wait_set so that in next spin_some() call the
  // 'executor->wait_set' is updated accordingly
  if (rcl_wait_set_is_valid(&executor->wait_set)) {
    ret = rcl_wait_set_fini(&executor->wait_set);
    if (RCL_RET_OK != ret) {
      RCL_SET_ERROR_MSG("Could not reset wait_set in rclc_executor_add_timer function.");
      return ret;
    }
  }
  executor->info.number_of_timers++;
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Added a timer.");
  return ret;
}

rcl_ret_t
rclc_executor_add_timer(
  rclc_executor_t * executor,
  rcl_timer_t * timer,
  rcutils_time_point_value_t callback_let_ns)
{
  rcl_ret_t ret = RCL_RET_OK;

  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(timer, RCL_RET_INVALID_ARGUMENT);

  // array bound check
  if (executor->index >= executor->max_handles) {
    rcl_ret_t ret = RCL_RET_ERROR;     // TODO(jst3si) better name : rclc_RET_BUFFER_OVERFLOW
    RCL_SET_ERROR_MSG("Buffer overflow of 'executor->handles'. Increase 'max_handles'");
    return ret;
  }

  // assign data fields
  executor->handles[executor->index].type = RCLC_TIMER;
  executor->handles[executor->index].timer = timer;
  executor->handles[executor->index].invocation = ON_NEW_DATA;  // i.e. when timer elapsed
  executor->handles[executor->index].initialized = true;
  executor->handles[executor->index].callback_context = NULL;
  executor->handles[executor->index].data_available = false;

  if(executor->data_comm_semantics == LET && executor->period_ns > 0)
  {
    executor->handles[executor->index].callback_info->callback_id = executor->let_executor->next_callback_id;
    executor->let_executor->next_callback_id++;
    executor->handles[executor->index].callback_info->callback_let_ns = callback_let_ns;
    int num_period_per_let = (callback_let_ns/executor->period_ns) + 1;
    bool data_available = false;
    rclc_callback_state_t state = INACTIVE;
    executor->handles[executor->index].callback_info->num_period_per_let = num_period_per_let;
    executor->handles[executor->index].callback_info->data.buffer = NULL;
    const rcl_allocator_t * allocator = executor->allocator;
    CHECK_RCL_RET(rclc_init_array(&(executor->handles[executor->index].callback_info->data_available), sizeof(bool), num_period_per_let, allocator),
                    (unsigned long) executor);
    CHECK_RCL_RET(rclc_init_array(&(executor->handles[executor->index].callback_info->state), sizeof(rclc_callback_state_t), num_period_per_let, allocator),
                    (unsigned long) executor);
    for (int i = 0; i < num_period_per_let; i++)
    {
      CHECK_RCL_RET(rclc_set_array(&(executor->handles[executor->index].callback_info->state), &state, i),
                      (unsigned long) executor);
      CHECK_RCL_RET(rclc_set_array(&(executor->handles[executor->index].callback_info->data_available), &data_available, i),
                      (unsigned long) executor);
    }  
  }
  else if (executor->data_comm_semantics == LET)
  {
    executor->handles[executor->index].callback_info->data.buffer = NULL;
    executor->handles[executor->index].callback_info->data_available.buffer = NULL;
    executor->handles[executor->index].callback_info->state.buffer = NULL;
  }

  // increase index of handle array
  executor->index++;

  // invalidate wait_set so that in next spin_some() call the
  // 'executor->wait_set' is updated accordingly
  if (rcl_wait_set_is_valid(&executor->wait_set)) {
    ret = rcl_wait_set_fini(&executor->wait_set);
    if (RCL_RET_OK != ret) {
      RCL_SET_ERROR_MSG("Could not reset wait_set in rclc_executor_add_timer function.");
      return ret;
    }
  }
  executor->info.number_of_timers++;
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Added a timer.");
  return ret;
}

rcl_ret_t
rclc_executor_add_client(
  rclc_executor_t * executor,
  rcl_client_t * client,
  void * response_msg,
  rclc_client_callback_t callback)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(client, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(response_msg, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(callback, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret = RCL_RET_OK;
  // array bound check
  if (executor->index >= executor->max_handles) {
    rcl_ret_t ret = RCL_RET_ERROR;
    RCL_SET_ERROR_MSG("Buffer overflow of 'executor->handles'. Increase 'max_handles'");
    return ret;
  }

  // assign data fields
  executor->handles[executor->index].type = RCLC_CLIENT;
  executor->handles[executor->index].client = client;
  executor->handles[executor->index].data = response_msg;
  executor->handles[executor->index].client_callback = callback;
  executor->handles[executor->index].invocation = ON_NEW_DATA;  // i.e. when request came in
  executor->handles[executor->index].initialized = true;
  executor->handles[executor->index].callback_context = NULL;

  // increase index of handle array
  executor->index++;

  // invalidate wait_set so that in next spin_some() call the
  // 'executor->wait_set' is updated accordingly
  if (rcl_wait_set_is_valid(&executor->wait_set)) {
    ret = rcl_wait_set_fini(&executor->wait_set);
    if (RCL_RET_OK != ret) {
      RCL_SET_ERROR_MSG("Could not reset wait_set in rclc_executor_add_client function.");
      return ret;
    }
  }

  executor->info.number_of_clients++;
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Added a client.");
  return ret;
}

rcl_ret_t
rclc_executor_add_client_with_request_id(
  rclc_executor_t * executor,
  rcl_client_t * client,
  void * response_msg,
  rclc_client_callback_with_request_id_t callback)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(client, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(response_msg, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(callback, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret = RCL_RET_OK;
  // array bound check
  if (executor->index >= executor->max_handles) {
    rcl_ret_t ret = RCL_RET_ERROR;
    RCL_SET_ERROR_MSG("Buffer overflow of 'executor->handles'. Increase 'max_handles'");
    return ret;
  }

  // assign data fields
  executor->handles[executor->index].type = RCLC_CLIENT_WITH_REQUEST_ID;
  executor->handles[executor->index].client = client;
  executor->handles[executor->index].data = response_msg;
  executor->handles[executor->index].client_callback_with_reqid = callback;
  executor->handles[executor->index].invocation = ON_NEW_DATA;  // i.e. when request came in
  executor->handles[executor->index].initialized = true;
  executor->handles[executor->index].callback_context = NULL;

  // increase index of handle array
  executor->index++;

  // invalidate wait_set so that in next spin_some() call the
  // 'executor->wait_set' is updated accordingly
  if (rcl_wait_set_is_valid(&executor->wait_set)) {
    ret = rcl_wait_set_fini(&executor->wait_set);
    if (RCL_RET_OK != ret) {
      RCL_SET_ERROR_MSG("Could not reset wait_set in rclc_executor_add_client function.");
      return ret;
    }
  }

  executor->info.number_of_clients++;
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Added a client.");
  return ret;
}

rcl_ret_t
rclc_executor_add_service(
  rclc_executor_t * executor,
  rcl_service_t * service,
  void * request_msg,
  void * response_msg,
  rclc_service_callback_t callback)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(service, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(request_msg, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(response_msg, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(callback, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret = RCL_RET_OK;
  // array bound check
  if (executor->index >= executor->max_handles) {
    rcl_ret_t ret = RCL_RET_ERROR;
    RCL_SET_ERROR_MSG("Buffer overflow of 'executor->handles'. Increase 'max_handles'");
    return ret;
  }

  // assign data fields
  executor->handles[executor->index].type = RCLC_SERVICE;
  executor->handles[executor->index].service = service;
  executor->handles[executor->index].data = request_msg;
  // TODO(jst3si) new type with req and resp message in data field.
  executor->handles[executor->index].data_response_msg = response_msg;
  executor->handles[executor->index].service_callback = callback;
  executor->handles[executor->index].invocation = ON_NEW_DATA;  // invoce when request came in
  executor->handles[executor->index].initialized = true;
  executor->handles[executor->index].callback_context = NULL;

  // increase index of handle array
  executor->index++;

  // invalidate wait_set so that in next spin_some() call the
  // 'executor->wait_set' is updated accordingly
  if (rcl_wait_set_is_valid(&executor->wait_set)) {
    ret = rcl_wait_set_fini(&executor->wait_set);
    if (RCL_RET_OK != ret) {
      RCL_SET_ERROR_MSG("Could not reset wait_set in rclc_executor_add_service function.");
      return ret;
    }
  }

  executor->info.number_of_services++;
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Added a service.");
  return ret;
}

rcl_ret_t
rclc_executor_add_service_with_context(
  rclc_executor_t * executor,
  rcl_service_t * service,
  void * request_msg,
  void * response_msg,
  rclc_service_callback_with_context_t callback,
  void * context)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(service, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(request_msg, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(response_msg, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(callback, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret = RCL_RET_OK;
  // array bound check
  if (executor->index >= executor->max_handles) {
    rcl_ret_t ret = RCL_RET_ERROR;
    RCL_SET_ERROR_MSG("Buffer overflow of 'executor->handles'. Increase 'max_handles'");
    return ret;
  }

  // assign data fields
  executor->handles[executor->index].type = RCLC_SERVICE_WITH_CONTEXT;
  executor->handles[executor->index].service = service;
  executor->handles[executor->index].data = request_msg;
  // TODO(jst3si) new type with req and resp message in data field.
  executor->handles[executor->index].data_response_msg = response_msg;
  executor->handles[executor->index].service_callback_with_context = callback;
  executor->handles[executor->index].invocation = ON_NEW_DATA;  // invoce when request came in
  executor->handles[executor->index].initialized = true;
  executor->handles[executor->index].callback_context = context;

  // increase index of handle array
  executor->index++;

  // invalidate wait_set so that in next spin_some() call the
  // 'executor->wait_set' is updated accordingly
  if (rcl_wait_set_is_valid(&executor->wait_set)) {
    ret = rcl_wait_set_fini(&executor->wait_set);
    if (RCL_RET_OK != ret) {
      RCL_SET_ERROR_MSG("Could not reset wait_set in rclc_executor_add_service function.");
      return ret;
    }
  }

  executor->info.number_of_services++;
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Added a service.");
  return ret;
}

rcl_ret_t
rclc_executor_add_service_with_request_id(
  rclc_executor_t * executor,
  rcl_service_t * service,
  void * request_msg,
  void * response_msg,
  rclc_service_callback_with_request_id_t callback)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(service, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(request_msg, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(response_msg, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(callback, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret = RCL_RET_OK;
  // array bound check
  if (executor->index >= executor->max_handles) {
    rcl_ret_t ret = RCL_RET_ERROR;
    RCL_SET_ERROR_MSG("Buffer overflow of 'executor->handles'. Increase 'max_handles'");
    return ret;
  }

  // assign data fields
  executor->handles[executor->index].type = RCLC_SERVICE_WITH_REQUEST_ID;
  executor->handles[executor->index].service = service;
  executor->handles[executor->index].data = request_msg;
  // TODO(jst3si) new type with req and resp message in data field.
  executor->handles[executor->index].data_response_msg = response_msg;
  executor->handles[executor->index].service_callback_with_reqid = callback;
  executor->handles[executor->index].invocation = ON_NEW_DATA;  // invoce when request came in
  executor->handles[executor->index].initialized = true;
  executor->handles[executor->index].callback_context = NULL;

  // increase index of handle array
  executor->index++;

  // invalidate wait_set so that in next spin_some() call the
  // 'executor->wait_set' is updated accordingly
  if (rcl_wait_set_is_valid(&executor->wait_set)) {
    ret = rcl_wait_set_fini(&executor->wait_set);
    if (RCL_RET_OK != ret) {
      RCL_SET_ERROR_MSG("Could not reset wait_set in rclc_executor_add_service function.");
      return ret;
    }
  }

  executor->info.number_of_services++;
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Added a service.");
  return ret;
}

rcl_ret_t
rclc_executor_add_guard_condition(
  rclc_executor_t * executor,
  rcl_guard_condition_t * gc,
  rclc_gc_callback_t callback)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(gc, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(callback, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret = RCL_RET_OK;
  // array bound check
  if (executor->index >= executor->max_handles) {
    ret = RCL_RET_ERROR;
    RCL_SET_ERROR_MSG("Buffer overflow of 'executor->handles'. Increase 'max_handles'");
    return ret;
  }

  // assign data fields
  executor->handles[executor->index].type = RCLC_GUARD_CONDITION;
  executor->handles[executor->index].gc = gc;
  executor->handles[executor->index].gc_callback = callback;
  executor->handles[executor->index].invocation = ON_NEW_DATA;  // invoce when request came in
  executor->handles[executor->index].initialized = true;
  executor->handles[executor->index].callback_context = NULL;

  // increase index of handle array
  executor->index++;

  // invalidate wait_set so that in next spin_some() call the
  // 'executor->wait_set' is updated accordingly
  if (rcl_wait_set_is_valid(&executor->wait_set)) {
    ret = rcl_wait_set_fini(&executor->wait_set);
    if (RCL_RET_OK != ret) {
      RCL_SET_ERROR_MSG("Could not reset wait_set in rclc_executor_add_guard_condition function.");
      return ret;
    }
  }

  executor->info.number_of_guard_conditions++;
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Added a guard_condition.");
  return ret;
}

static
rcl_ret_t
_rclc_executor_remove_handle(rclc_executor_t * executor, rclc_executor_handle_t * handle)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);

  rcl_ret_t ret = RCL_RET_OK;

  // NULL will be returned by _rclc_executor_find_handle if the handle is not found
  if (NULL == handle) {
    RCL_SET_ERROR_MSG("handle not found in rclc_executor_remove_handle");
    return RCL_RET_ERROR;
  }

  if ((handle >= &executor->handles[executor->index]) ||
    (handle < executor->handles))
  {
    RCL_SET_ERROR_MSG("Handle out of bounds");
    return RCL_RET_ERROR;
  }
  if (0 == executor->index) {
    RCL_SET_ERROR_MSG("No handles to remove");
    return RCL_RET_ERROR;
  }

  // shorten the list of handles without changing the order of remaining handles
  executor->index--;
  for (rclc_executor_handle_t * handle_dest = handle;
    handle_dest < &executor->handles[executor->index];
    handle_dest++)
  {
    *handle_dest = *(handle_dest + 1);
  }
  ret = rclc_executor_handle_init(&executor->handles[executor->index], executor->max_handles);

  // force a refresh of the wait set
  if (rcl_wait_set_is_valid(&executor->wait_set)) {
    ret = rcl_wait_set_fini(&executor->wait_set);
    if (RCL_RET_OK != ret) {
      RCL_SET_ERROR_MSG("Could not reset wait_set in _rclc_executor_remove_handle.");
      return ret;
    }
  }

  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Removed a handle.");
  return ret;
}

static
rclc_executor_handle_t *
_rclc_executor_find_handle(
  rclc_executor_t * executor,
  const void * rcl_handle)
{
  for (rclc_executor_handle_t * test_handle = executor->handles;
    test_handle < &executor->handles[executor->index];
    test_handle++)
  {
    if (rcl_handle == rclc_executor_handle_get_ptr(test_handle)) {
      return test_handle;
    }
  }
  return NULL;
}


rcl_ret_t
rclc_executor_remove_subscription(
  rclc_executor_t * executor,
  const rcl_subscription_t * subscription)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(subscription, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret = RCL_RET_OK;

  rclc_executor_handle_t * handle = _rclc_executor_find_handle(executor, subscription);
  ret = _rclc_executor_remove_handle(executor, handle);
  if (RCL_RET_OK != ret) {
    RCL_SET_ERROR_MSG("Failed to remove handle in rclc_executor_remove_subscription.");
    return ret;
  }
  executor->info.number_of_subscriptions--;
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Removed a subscription.");
  return ret;
}

rcl_ret_t
rclc_executor_remove_timer(
  rclc_executor_t * executor,
  const rcl_timer_t * timer)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(timer, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret = RCL_RET_OK;

  rclc_executor_handle_t * handle = _rclc_executor_find_handle(executor, timer);
  ret = _rclc_executor_remove_handle(executor, handle);
  if (RCL_RET_OK != ret) {
    RCL_SET_ERROR_MSG("Failed to remove handle in rclc_executor_remove_timer.");
    return ret;
  }
  executor->info.number_of_timers--;
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Removed a timer.");
  return ret;
}

rcl_ret_t
rclc_executor_remove_client(
  rclc_executor_t * executor,
  const rcl_client_t * client)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(client, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret = RCL_RET_OK;

  rclc_executor_handle_t * handle = _rclc_executor_find_handle(executor, client);
  ret = _rclc_executor_remove_handle(executor, handle);
  if (RCL_RET_OK != ret) {
    RCL_SET_ERROR_MSG("Failed to remove handle in rclc_executor_remove_client.");
    return ret;
  }
  executor->info.number_of_clients--;
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Removed a client.");
  return ret;
}

rcl_ret_t
rclc_executor_remove_service(
  rclc_executor_t * executor,
  const rcl_service_t * service)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(service, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret = RCL_RET_OK;

  rclc_executor_handle_t * handle = _rclc_executor_find_handle(executor, service);
  ret = _rclc_executor_remove_handle(executor, handle);
  if (RCL_RET_OK != ret) {
    RCL_SET_ERROR_MSG("Failed to remove handle in rclc_executor_remove_service.");
    return ret;
  }
  executor->info.number_of_services--;
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Removed a service.");
  return ret;
}


rcl_ret_t
rclc_executor_remove_guard_condition(
  rclc_executor_t * executor,
  const rcl_guard_condition_t * guard_condition)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(guard_condition, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret = RCL_RET_OK;

  rclc_executor_handle_t * handle = _rclc_executor_find_handle(executor, guard_condition);
  ret = _rclc_executor_remove_handle(executor, handle);
  if (RCL_RET_OK != ret) {
    RCL_SET_ERROR_MSG("Failed to remove handle in rclc_executor_remove_guard_condition.");
    return ret;
  }
  executor->info.number_of_guard_conditions--;
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Removed a guard condition.");
  return ret;
}

rcl_ret_t
rclc_executor_add_action_client(
  rclc_executor_t * executor,
  rclc_action_client_t * action_client,
  size_t handles_number,
  void * ros_result_response,
  void * ros_feedback,
  rclc_action_client_goal_callback_t goal_callback,
  rclc_action_client_feedback_callback_t feedback_callback,
  rclc_action_client_result_callback_t result_callback,
  rclc_action_client_cancel_callback_t cancel_callback,
  void * context)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(action_client, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(ros_result_response, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(goal_callback, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(result_callback, RCL_RET_INVALID_ARGUMENT);

  if (NULL != feedback_callback) {
    RCL_CHECK_ARGUMENT_FOR_NULL(ros_feedback, RCL_RET_INVALID_ARGUMENT);
  }

  rcl_ret_t ret = RCL_RET_OK;

  // array bound check
  if (executor->index >= executor->max_handles) {
    rcl_ret_t ret = RCL_RET_ERROR;
    RCL_SET_ERROR_MSG("Buffer overflow of 'executor->handles'. Increase 'max_handles'");
    return ret;
  }

  action_client->allocator = executor->allocator;

  // Init goal handles
  action_client->goal_handles_memory =
    executor->allocator->allocate(
    handles_number * sizeof(rclc_action_goal_handle_t),
    executor->allocator->state);
  if (NULL == action_client->goal_handles_memory) {
    return RCL_RET_ERROR;
  }
  action_client->goal_handles_memory_size = handles_number;
  rclc_action_init_goal_handle_memory(action_client);

  action_client->ros_feedback = ros_feedback;
  action_client->ros_result_response = ros_result_response;

  action_client->ros_cancel_response.goals_canceling.data =
    (action_msgs__msg__GoalInfo *) executor->allocator->allocate(
    handles_number *
    sizeof(action_msgs__msg__GoalInfo), executor->allocator->state);
  action_client->ros_cancel_response.goals_canceling.size = 0;
  action_client->ros_cancel_response.goals_canceling.capacity = handles_number;

  rclc_action_goal_handle_t * goal_handle = action_client->free_goal_handles;
  while (NULL != goal_handle) {
    goal_handle->action_client = action_client;
    goal_handle = goal_handle->next;
  }

  // assign data fields
  executor->handles[executor->index].type = RCLC_ACTION_CLIENT;
  executor->handles[executor->index].action_client = action_client;
  executor->handles[executor->index].action_client->goal_callback = goal_callback;
  executor->handles[executor->index].action_client->feedback_callback = feedback_callback;
  executor->handles[executor->index].action_client->result_callback = result_callback;
  executor->handles[executor->index].action_client->cancel_callback = cancel_callback;
  executor->handles[executor->index].invocation = ON_NEW_DATA;  // i.e. when request came in
  executor->handles[executor->index].initialized = true;
  executor->handles[executor->index].callback_context = context;

  executor->handles[executor->index].action_client->feedback_available = false;
  executor->handles[executor->index].action_client->status_available = false;
  executor->handles[executor->index].action_client->goal_response_available = false;
  executor->handles[executor->index].action_client->result_response_available = false;
  executor->handles[executor->index].action_client->cancel_response_available = false;

  // increase index of handle array
  executor->index++;

  // invalidate wait_set so that in next spin_some() call the
  // 'executor->wait_set' is updated accordingly
  if (rcl_wait_set_is_valid(&executor->wait_set)) {
    ret = rcl_wait_set_fini(&executor->wait_set);
    if (RCL_RET_OK != ret) {
      RCL_SET_ERROR_MSG("Could not reset wait_set in rclc_executor_add_action_client function.");
      return ret;
    }
  }

  size_t num_subscriptions = 0, num_guard_conditions = 0,
    num_timers = 0, num_clients = 0, num_services = 0;

  ret = rcl_action_client_wait_set_get_num_entities(
    &action_client->rcl_handle,
    &num_subscriptions,
    &num_guard_conditions,
    &num_timers,
    &num_clients,
    &num_services);

  executor->info.number_of_subscriptions += num_subscriptions;
  executor->info.number_of_guard_conditions += num_guard_conditions;
  executor->info.number_of_timers += num_timers;
  executor->info.number_of_clients += num_clients;
  executor->info.number_of_services += num_services;

  executor->info.number_of_action_clients++;
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Added an action client.");
  return ret;
}


rcl_ret_t
rclc_executor_add_action_server(
  rclc_executor_t * executor,
  rclc_action_server_t * action_server,
  size_t handles_number,
  void * ros_goal_request,
  size_t ros_goal_request_size,
  rclc_action_server_handle_goal_callback_t goal_callback,
  rclc_action_server_handle_cancel_callback_t cancel_callback,
  void * context)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(action_server, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(ros_goal_request, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(goal_callback, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(cancel_callback, RCL_RET_INVALID_ARGUMENT);

  if (ros_goal_request_size <= 0) {
    return RCL_RET_ERROR;
  }

  rcl_ret_t ret = RCL_RET_OK;

  action_server->allocator = executor->allocator;

  // array bound check
  if (executor->index >= executor->max_handles) {
    rcl_ret_t ret = RCL_RET_ERROR;
    RCL_SET_ERROR_MSG("Buffer overflow of 'executor->handles'. Increase 'max_handles'");
    return ret;
  }

  // Init goal handles
  action_server->goal_handles_memory =
    executor->allocator->allocate(
    handles_number * sizeof(rclc_action_goal_handle_t),
    executor->allocator->state);
  if (NULL == action_server->goal_handles_memory) {
    return RCL_RET_ERROR;
  }
  action_server->goal_handles_memory_size = handles_number;
  rclc_action_init_goal_handle_memory(action_server);

  rclc_action_goal_handle_t * goal_handle = action_server->free_goal_handles;
  size_t ros_goal_request_index = 0;
  while (NULL != goal_handle) {
    goal_handle->ros_goal_request =
      (void *) &((uint8_t *)ros_goal_request)[ros_goal_request_index * ros_goal_request_size]; // NOLINT()
    goal_handle->action_server = action_server;
    ros_goal_request_index++;
    goal_handle = goal_handle->next;
  }

  // assign data fields
  executor->handles[executor->index].type = RCLC_ACTION_SERVER;
  executor->handles[executor->index].action_server = action_server;
  executor->handles[executor->index].action_server->goal_callback = goal_callback;
  executor->handles[executor->index].action_server->cancel_callback = cancel_callback;
  executor->handles[executor->index].invocation = ON_NEW_DATA;
  executor->handles[executor->index].initialized = true;
  executor->handles[executor->index].callback_context = context;

  executor->handles[executor->index].action_server->goal_ended = false;
  executor->handles[executor->index].action_server->goal_request_available = false;
  executor->handles[executor->index].action_server->cancel_request_available = false;
  executor->handles[executor->index].action_server->result_request_available = false;
  executor->handles[executor->index].action_server->goal_expired_available = false;

  // increase index of handle array
  executor->index++;

  // invalidate wait_set so that in next spin_some() call the
  // 'executor->wait_set' is updated accordingly
  if (rcl_wait_set_is_valid(&executor->wait_set)) {
    ret = rcl_wait_set_fini(&executor->wait_set);
    if (RCL_RET_OK != ret) {
      RCL_SET_ERROR_MSG("Could not reset wait_set in rclc_executor_add_action_server function.");
      return ret;
    }
  }

  size_t num_subscriptions = 0, num_guard_conditions = 0,
    num_timers = 0, num_clients = 0, num_services = 0;

  ret = rcl_action_server_wait_set_get_num_entities(
    &action_server->rcl_handle,
    &num_subscriptions,
    &num_guard_conditions,
    &num_timers,
    &num_clients,
    &num_services);

  executor->info.number_of_subscriptions += num_subscriptions;
  executor->info.number_of_guard_conditions += num_guard_conditions;
  executor->info.number_of_timers += num_timers;
  executor->info.number_of_clients += num_clients;
  executor->info.number_of_services += num_services;

  executor->info.number_of_action_servers++;
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Added a action client.");
  return ret;
}

/***
 * operates on handle executor->handles[i]
 * - evaluates the status bit in the wait_set for this handle
 * - if new message is available or timer is ready, assigns executor->handles[i].data_available = true
 */

// todo refactor parameters: (rclc_executor_handle_t *, rcl_wait_set_t * wait_set)

static
rcl_ret_t
_rclc_check_for_new_data(rclc_executor_handle_t * handle, 
      rcl_wait_set_t * wait_set, 
      rclc_executor_semantics_t semantics, 
      uint64_t input_index)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(handle, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(wait_set, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t rc = RCL_RET_OK;

  switch (handle->type) {
    case RCLC_SUBSCRIPTION:
    case RCLC_SUBSCRIPTION_WITH_CONTEXT:
      handle->data_available = (NULL != wait_set->subscriptions[handle->index]); 
      if(semantics == LET)
      {
        rc = rclc_set_array(&(handle->callback_info->data_available), &handle->data_available, 
              input_index%handle->callback_info->num_period_per_let);
      }
      break;

    case RCLC_TIMER:
    case RCLC_TIMER_WITH_CONTEXT:
      handle->data_available = (NULL != wait_set->timers[handle->index]);
      if(semantics == LET)
      {
        rc = rclc_set_array(&(handle->callback_info->data_available), &handle->data_available, 
              input_index%handle->callback_info->num_period_per_let);
      }
      break;

    case RCLC_SERVICE:
    case RCLC_SERVICE_WITH_REQUEST_ID:
    case RCLC_SERVICE_WITH_CONTEXT:
      handle->data_available = (NULL != wait_set->services[handle->index]);
      break;

    case RCLC_CLIENT:
    case RCLC_CLIENT_WITH_REQUEST_ID:
      // case RCLC_CLIENT_WITH_CONTEXT:
      handle->data_available = (NULL != wait_set->clients[handle->index]);
      break;

    case RCLC_GUARD_CONDITION:
      // case RCLC_GUARD_CONDITION_WITH_CONTEXT:
      handle->data_available = (NULL != wait_set->guard_conditions[handle->index]);
      break;

    case RCLC_ACTION_CLIENT:
      rc = rcl_action_client_wait_set_get_entities_ready(
        wait_set,
        &handle->action_client->rcl_handle,
        &handle->action_client->feedback_available,
        &handle->action_client->status_available,
        &handle->action_client->goal_response_available,
        &handle->action_client->cancel_response_available,
        &handle->action_client->result_response_available
      );
      break;

    case RCLC_ACTION_SERVER:
      rc = rcl_action_server_wait_set_get_entities_ready(
        wait_set,
        &handle->action_server->rcl_handle,
        &handle->action_server->goal_request_available,
        &handle->action_server->cancel_request_available,
        &handle->action_server->result_request_available,
        &handle->action_server->goal_expired_available
      );
      break;

    default:
      RCUTILS_LOG_DEBUG_NAMED(
        ROS_PACKAGE_NAME, "Error in _rclc_check_for_new_data:wait_set unknwon handle type: %d",
        handle->type);
      return RCL_RET_ERROR;
  }    // switch-case
  return rc;
}

// call rcl_take for subscription
// todo change function signature (rclc_executor_handle_t * handle, rcl_wait_set_t * wait_set)

static
rcl_ret_t
_rclc_take_new_data(rclc_executor_handle_t * handle, rcl_wait_set_t * wait_set, rclc_executor_semantics_t semantics, uint64_t input_index)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(handle, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(wait_set, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t rc = RCL_RET_OK;

  switch (handle->type) {
    case RCLC_SUBSCRIPTION:
    case RCLC_SUBSCRIPTION_WITH_CONTEXT:
      if (wait_set->subscriptions[handle->index]) {
        rmw_message_info_t messageInfo;
        int64_t * data_to_print = NULL;
        if (semantics == LET)
        {
          rclc_array_element_status_t status;
          void * data;
          rc = rclc_get_pointer_array(&(handle->callback_info->data), input_index%handle->callback_info->num_period_per_let, &data, &status);
          if (rc != RCL_RET_OK)
            return rc;
          rc = rcl_take(handle->subscription, data, &messageInfo, NULL);
          data_to_print = data;
        }
        else
        {
          rc = rcl_take(handle->subscription, handle->data, &messageInfo, NULL);
          data_to_print = handle->data;          
        }

        if (rc != RCL_RET_OK) {
          // rcl_take might return this error even with successfull rcl_wait
          if (rc != RCL_RET_SUBSCRIPTION_TAKE_FAILED) {
            PRINT_RCLC_ERROR(rclc_take_new_data, rcl_take);
            RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Error number: %d", rc);
          }
          // invalidate that data is available, because rcl_take failed
          if (rc == RCL_RET_SUBSCRIPTION_TAKE_FAILED) {
            handle->data_available = false;
          }
          return rc;
        }
        rcutils_time_point_value_t now;
        rc = rcutils_steady_time_now(&now);
        printf("Input %lu %ld %ld\n", (unsigned long) handle->timer, data_to_print[1], now);
      }
      break;

    case RCLC_TIMER:
    case RCLC_TIMER_WITH_CONTEXT:
      // nothing to do
      // notification, that timer is ready already done in _rclc_evaluate_data_availability()
      break;

    case RCLC_ACTION_CLIENT:
      if (handle->action_client->goal_response_available) {
        Generic_SendGoal_Response aux_goal_response;
        rmw_request_id_t aux_goal_response_header;
        rc = rcl_action_take_goal_response(
          &handle->action_client->rcl_handle,
          &aux_goal_response_header,
          &aux_goal_response);
        if (rc != RCL_RET_OK) {
          PRINT_RCLC_ERROR(rclc_take_new_data, rcl_action_take_goal_response);
          RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Error number: %d", rc);
          return rc;
        }
        rclc_action_goal_handle_t * goal_handle =
          rclc_action_find_handle_by_goal_request_sequence_number(
          handle->action_client, aux_goal_response_header.sequence_number);
        if (NULL != goal_handle) {
          goal_handle->available_goal_response = true;
          goal_handle->goal_accepted = aux_goal_response.accepted;
        }
      }
      if (handle->action_client->feedback_callback != NULL &&
        handle->action_client->feedback_available)
      {
        rc = rcl_action_take_feedback(
          &handle->action_client->rcl_handle,
          handle->action_client->ros_feedback);

        if (rc != RCL_RET_OK) {
          PRINT_RCLC_ERROR(rclc_take_new_data, rcl_action_take_feedback);
          RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Error number: %d", rc);
          return rc;
        }
        rclc_action_goal_handle_t * goal_handle = rclc_action_find_goal_handle_by_uuid(
          handle->action_client,
          &handle->action_client->ros_feedback->goal_id);
        if (NULL != goal_handle) {
          goal_handle->available_feedback = true;
        }
      }
      if (handle->action_client->cancel_response_available) {
        rmw_request_id_t cancel_response_header;
        rc = rcl_action_take_cancel_response(
          &handle->action_client->rcl_handle,
          &cancel_response_header,
          &handle->action_client->ros_cancel_response);
        if (rc != RCL_RET_OK) {
          PRINT_RCLC_ERROR(rclc_take_new_data, rcl_action_take_cancel_response);
          RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Error number: %d", rc);
          return rc;
        }
        rclc_action_goal_handle_t * goal_handle =
          rclc_action_find_handle_by_cancel_request_sequence_number(
          handle->action_client,
          cancel_response_header.sequence_number);
        if (NULL != goal_handle) {
          goal_handle->available_cancel_response = true;
          goal_handle->goal_cancelled = false;
          for (size_t i = 0; i < handle->action_client->ros_cancel_response.goals_canceling.size;
            i++)
          {
            rclc_action_goal_handle_t * aux = rclc_action_find_goal_handle_by_uuid(
              handle->action_client,
              &handle->action_client->ros_cancel_response.goals_canceling.data[i].goal_id);
            if (NULL != aux) {
              goal_handle->goal_cancelled = true;
            }
          }
        }
      }
      if (handle->action_client->result_response_available) {
        rmw_request_id_t result_request_header;
        rc = rcl_action_take_result_response(
          &handle->action_client->rcl_handle,
          &result_request_header,
          handle->action_client->ros_result_response);
        if (rc != RCL_RET_OK) {
          PRINT_RCLC_ERROR(rclc_take_new_data, rcl_action_take_result_response);
          RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Error number: %d", rc);
          return rc;
        }
        rclc_action_goal_handle_t * goal_handle =
          rclc_action_find_handle_by_result_request_sequence_number(
          handle->action_client, result_request_header.sequence_number);
        if (NULL != goal_handle) {
          goal_handle->available_result_response = true;
        }
      }
      break;

    case RCLC_ACTION_SERVER:
      if (handle->action_server->goal_request_available) {
        rclc_action_goal_handle_t * goal_handle =
          rclc_action_take_goal_handle(handle->action_server);
        if (NULL != goal_handle) {
          goal_handle->action_server = handle->action_server;
          rc = rcl_action_take_goal_request(
            &handle->action_server->rcl_handle,
            &goal_handle->goal_request_header,
            goal_handle->ros_goal_request);
          if (rc != RCL_RET_OK) {
            rclc_action_remove_used_goal_handle(handle->action_server, goal_handle);
            PRINT_RCLC_ERROR(rclc_take_new_data, rcl_action_take_goal_request);
            RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Error number: %d", rc);
            return rc;
          }
          goal_handle->goal_id = goal_handle->ros_goal_request->goal_id;
          goal_handle->status = GOAL_STATE_UNKNOWN;
        }
      }
      if (handle->action_server->result_request_available) {
        Generic_GetResult_Request aux_result_request;
        rmw_request_id_t aux_result_request_header;
        rc = rcl_action_take_result_request(
          &handle->action_server->rcl_handle,
          &aux_result_request_header,
          &aux_result_request);
        if (rc != RCL_RET_OK) {
          PRINT_RCLC_ERROR(rclc_take_new_data, rcl_action_take_result_request);
          RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Error number: %d", rc);
          return rc;
        }
        rclc_action_goal_handle_t * goal_handle = rclc_action_find_goal_handle_by_uuid(
          handle->action_server, &aux_result_request.goal_id);
        if (NULL != goal_handle) {
          goal_handle->result_request_header = aux_result_request_header;
          goal_handle->status = GOAL_STATE_EXECUTING;
        }
        handle->action_server->result_request_available = false;
      }
      if (handle->action_server->cancel_request_available) {
        action_msgs__srv__CancelGoal_Request aux_cancel_request;
        rmw_request_id_t aux_cancel_request_header;

        rc = rcl_action_take_cancel_request(
          &handle->action_server->rcl_handle,
          &aux_cancel_request_header,
          &aux_cancel_request);
        if (rc != RCL_RET_OK) {
          PRINT_RCLC_ERROR(rclc_take_new_data, rcl_action_take_cancel_request);
          RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Error number: %d", rc);
          return rc;
        }
        rclc_action_goal_handle_t * goal_handle = rclc_action_find_goal_handle_by_uuid(
          handle->action_server, &aux_cancel_request.goal_info.goal_id);
        if (NULL != goal_handle) {
          if (GOAL_STATE_CANCELING == rcl_action_transition_goal_state(
              goal_handle->status, GOAL_EVENT_CANCEL_GOAL))
          {
            goal_handle->cancel_request_header = aux_cancel_request_header;
            goal_handle->status = GOAL_STATE_CANCELING;
          } else {
            rclc_action_server_goal_cancel_reject(
              handle->action_server, CANCEL_STATE_TERMINATED,
              aux_cancel_request_header);
          }
        } else {
          rclc_action_server_goal_cancel_reject(
            handle->action_server, CANCEL_STATE_UNKNOWN_GOAL,
            aux_cancel_request_header);
        }
      }
      break;

    case RCLC_SERVICE:
    case RCLC_SERVICE_WITH_REQUEST_ID:
    case RCLC_SERVICE_WITH_CONTEXT:
      if (wait_set->services[handle->index]) {
        rc = rcl_take_request(
          handle->service, &handle->req_id, handle->data);
        if (rc != RCL_RET_OK) {
          // rcl_take_request might return this error even with successfull rcl_wait
          if (rc != RCL_RET_SERVICE_TAKE_FAILED) {
            PRINT_RCLC_ERROR(rclc_take_new_data, rcl_take_request);
            RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Error number: %d", rc);
          }
          // invalidate that data is available, because rcl_take failed
          if (rc == RCL_RET_SERVICE_TAKE_FAILED) {
            handle->data_available = false;
          }
          return rc;
        }
      }
      break;

    case RCLC_CLIENT:
    case RCLC_CLIENT_WITH_REQUEST_ID:
      // case RCLC_CLIENT_WITH_CONTEXT:
      if (wait_set->clients[handle->index]) {
        rc = rcl_take_response(
          handle->client, &handle->req_id, handle->data);
        if (rc != RCL_RET_OK) {
          // rcl_take_response might return this error even with successfull rcl_wait
          if (rc != RCL_RET_CLIENT_TAKE_FAILED) {
            PRINT_RCLC_ERROR(rclc_take_new_data, rcl_take_response);
            RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Error number: %d", rc);
          }
          return rc;
        }
      }
      break;

    case RCLC_GUARD_CONDITION:
      // case RCLC_GUARD_CONDITION_WITH_CONTEXT:
      // nothing to do
      break;

    default:
      RCUTILS_LOG_DEBUG_NAMED(
        ROS_PACKAGE_NAME, "Error in _rclc_take_new_data:wait_set unknwon handle type: %d",
        handle->type);
      return RCL_RET_ERROR;
  }    // switch-case
  return rc;
}

bool _rclc_check_handle_data_available(rclc_executor_handle_t * handle,
  rclc_executor_semantics_t semantics, 
  uint64_t index)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(handle, false);

  switch (handle->type) {
    case RCLC_ACTION_CLIENT:
      if (handle->action_client->feedback_available ||
        handle->action_client->status_available ||
        handle->action_client->goal_response_available ||
        handle->action_client->cancel_response_available ||
        handle->action_client->result_response_available)
      {
        return true;
      }
      break;

    case RCLC_ACTION_SERVER:
      if (handle->action_server->goal_request_available ||
        handle->action_server->cancel_request_available ||
        handle->action_server->goal_expired_available ||
        handle->action_server->result_request_available ||
        handle->action_server->goal_ended)
      {
        return true;
      }
      break;
    case RCLC_SUBSCRIPTION:
    case RCLC_SUBSCRIPTION_WITH_CONTEXT:
    case RCLC_TIMER:
    case RCLC_TIMER_WITH_CONTEXT:
      if (semantics == LET)
      {
        bool data_available = false;
        rcl_ret_t ret = rclc_get_array(&(handle->callback_info->data_available), 
          &data_available, index%handle->callback_info->num_period_per_let);
        RCLC_UNUSED(ret);
        return data_available;
      }
      else if (handle->data_available)
        return true;
      break;
    default:
      if (handle->data_available) {
        return true;
      }
      break;
  }  // switch-case

  return false;
}

/***
 * operates on executor->handles[i] object
 * - calls every callback of each object depending on its type
 */

// todo change parametes (rclc_executor_handle_t * handle)

static
rcl_ret_t
_rclc_execute(
  rclc_executor_handle_t * handle,
  rclc_executor_t * executor)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(handle, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t rc = RCL_RET_OK;
  bool invoke_callback = false;
  rclc_executor_semantics_t semantics = executor->data_comm_semantics;
  uint64_t period_index = 0;
  if (semantics == LET)
    period_index = executor->let_executor->spin_index;
  bool data_available = _rclc_check_handle_data_available(handle, semantics, period_index);
  rclc_callback_state_t state;

  // determine, if callback shall be called
  if (handle->invocation == ON_NEW_DATA && data_available)
  {
    invoke_callback = true;
  }

  if (handle->invocation == ALWAYS) {
    invoke_callback = true;
  }

  // execute callback
  if (invoke_callback) {
    if (semantics == LET && executor->let_executor->overrun_option != NO_ERROR_HANDLING)
    {
      pthread_mutex_lock(&executor->let_executor->mutex_state);
      state = RUNNING;     
      rc = rclc_set_array(&(handle->callback_info->state), &state, 
              period_index%handle->callback_info->num_period_per_let);
      pthread_mutex_unlock(&executor->let_executor->mutex_state);
    }

    switch (handle->type) {
      case RCLC_SUBSCRIPTION:
        if (data_available) {
          handle->subscription_callback(handle->data);
        } else {
          handle->subscription_callback(NULL);
        }
        break;

      case RCLC_SUBSCRIPTION_WITH_CONTEXT:
        if (data_available) {
          handle->subscription_callback_with_context(
            handle->data,
            handle->callback_context);
        } else {
          handle->subscription_callback_with_context(
            NULL,
            handle->callback_context);
        }
        break;

      case RCLC_TIMER:
        // case RCLC_TIMER_WITH_CONTEXT:
        if (semantics == LET)
        {
          rcl_timer_callback_t callback = rcl_timer_get_callback(handle->timer);
          // Users must ignore the last_call_time value
          callback(handle->timer, 0);
        }
        else
        {
          rcutils_time_point_value_t now;
          rc = rcutils_steady_time_now(&now);
          printf("Input %lu %ld %ld\n", (unsigned long) handle->timer, 0, now);
          rc = rcl_timer_call(handle->timer);
          // cancled timer are not handled, return success
          if (rc == RCL_RET_TIMER_CANCELED) {
            rc = RCL_RET_OK;
            break;
          }

          if (rc != RCL_RET_OK) {
            PRINT_RCLC_ERROR(rclc_execute, rcl_timer_call);
            return rc;
          }
        }
        break;
      
      case RCLC_TIMER_WITH_CONTEXT:
        if (semantics != LET)
        {
          rc = rcl_timer_call(handle->timer);
          if (rc == RCL_RET_TIMER_CANCELED) {
            rc = RCL_RET_OK;
            break;
          }

          if (rc != RCL_RET_OK) {
            PRINT_RCLC_ERROR(rclc_execute, rcl_timer_call);
            return rc;
          }
          rcutils_time_point_value_t now;
          rc = rcutils_steady_time_now(&now);
          printf("Input %lu %ld %ld\n", (unsigned long) handle->timer, 0, now);
        }
        handle->timer_callback_with_context(handle->timer, handle->callback_context);
        break;

      case RCLC_SERVICE:
      case RCLC_SERVICE_WITH_REQUEST_ID:
      case RCLC_SERVICE_WITH_CONTEXT:
        // differentiate user-side service types
        switch (handle->type) {
          case RCLC_SERVICE:
            handle->service_callback(
              handle->data,
              handle->data_response_msg);
            break;
          case RCLC_SERVICE_WITH_REQUEST_ID:
            handle->service_callback_with_reqid(
              handle->data,
              &handle->req_id,
              handle->data_response_msg);
            break;
          case RCLC_SERVICE_WITH_CONTEXT:
            handle->service_callback_with_context(
              handle->data,
              handle->data_response_msg,
              handle->callback_context);
            break;
          default:
            break;  // flow can't reach here
        }
        // handle rcl-side services
        rc = rcl_send_response(handle->service, &handle->req_id, handle->data_response_msg);
        if (rc != RCL_RET_OK) {
          PRINT_RCLC_ERROR(rclc_execute, rcl_send_response);
          return rc;
        }
        break;

      case RCLC_CLIENT:
        handle->client_callback(handle->data);
        break;

      case RCLC_CLIENT_WITH_REQUEST_ID:
        handle->client_callback_with_reqid(handle->data, &handle->req_id);
        break;

      // case RCLC_CLIENT_WITH_CONTEXT:   //TODO
      //   break;

      case RCLC_GUARD_CONDITION:
        handle->gc_callback();
        break;

      // case RCLC_GUARD_CONDITION_WITH_CONTEXT:  //TODO
      //   break;

      case RCLC_ACTION_CLIENT:
        // TODO(pablogs9): Handle action client status
        if (handle->action_client->goal_response_available) {
          // Handle action client goal response messages
          //
          // Pre-condition:
          // - goal in action_client->used_goal_handles list
          // - goal->available_goal_response = true
          //
          // Post-condition:
          // - goal->available_goal_response = false
          rclc_action_goal_handle_t * goal_handle;
          while (goal_handle =
            rclc_action_find_first_handle_with_goal_response(handle->action_client),
            NULL != goal_handle)
          {
            // Set post-condition
            goal_handle->available_goal_response = false;
            handle->action_client->goal_callback(
              goal_handle, goal_handle->goal_accepted,
              handle->callback_context);
            if (!goal_handle->goal_accepted ||
              RCL_RET_OK != rclc_action_send_result_request(goal_handle))
            {
              rclc_action_remove_used_goal_handle(handle->action_client, goal_handle);
            } else {
              goal_handle->status = GOAL_STATE_ACCEPTED;
            }
          }
        }
        if (handle->action_client->feedback_available) {
          rclc_action_goal_handle_t * goal_handle;
          for (goal_handle = handle->action_client->used_goal_handles; NULL != goal_handle;
            goal_handle = goal_handle->next)
          {
            if (goal_handle->available_feedback) {
              goal_handle->available_feedback = false;

              if (handle->action_client->feedback_callback != NULL) {
                handle->action_client->feedback_callback(
                  goal_handle,
                  handle->action_client->ros_feedback,
                  handle->callback_context);
              }
            }
          }
        }
        if (handle->action_client->cancel_response_available) {
          rclc_action_goal_handle_t * goal_handle;
          for (goal_handle = handle->action_client->used_goal_handles; NULL != goal_handle;
            goal_handle = goal_handle->next)
          {
            if (goal_handle->available_cancel_response) {
              goal_handle->available_cancel_response = false;

              if (handle->action_client->cancel_callback != NULL) {
                handle->action_client->cancel_callback(
                  goal_handle,
                  goal_handle->goal_cancelled,
                  handle->callback_context);
              }
            }
          }
        }
        if (handle->action_client->result_response_available) {
          // Handle action client result response messages
          //
          // Pre-condition:
          // - goal in action_client->used_goal_handles list
          // - goal->available_result_response = true
          //
          // Post-condition:
          // - goal->available_result_response = false
          // - goal deleted from action_client->used_goal_handles list
          rclc_action_goal_handle_t * goal_handle;
          while (goal_handle =
            rclc_action_find_first_handle_with_result_response(handle->action_client),
            NULL != goal_handle)
          {
            // Set first post-condition:
            goal_handle->available_result_response = false;
            handle->action_client->result_callback(
              goal_handle,
              handle->action_client->ros_result_response,
              handle->callback_context);

            // Set second post-condition
            rclc_action_remove_used_goal_handle(handle->action_client, goal_handle);
          }
        }
        break;

      case RCLC_ACTION_SERVER:
        if (handle->action_server->goal_ended) {
          // Handle action server terminated goals (succeeded, canceled or aborted)
          //
          // Pre-condition:
          // - goal in action_server->used_goal_handles list
          // - goal->status > GOAL_STATE_CANCELING
          //
          // Post-condition:
          // - goal deleted from action_server->used_goal_handles list
          rclc_action_goal_handle_t * goal_handle;
          while (goal_handle =
            rclc_action_find_first_terminated_handle(handle->action_server),
            NULL != goal_handle)
          {
            // Set post-condition
            rclc_action_remove_used_goal_handle(goal_handle->action_server, goal_handle);
          }
          handle->action_server->goal_ended = false;
        }
        if (handle->action_server->goal_request_available) {
          // Handle action server goal request messages
          //
          // Pre-condition:
          // - goal in action_server->used_goal_handles list
          // - goal->status = GOAL_STATE_UNKNOWN
          //
          // Accepted post-condition:
          // - goal->status = GOAL_STATE_ACCEPTED
          // Rejected/Error post-condition:
          // - goal deleted from action_server->used_goal_handles list
          rclc_action_goal_handle_t * goal_handle;
          while (goal_handle =
            rclc_action_find_first_handle_by_status(handle->action_server, GOAL_STATE_UNKNOWN),
            NULL != goal_handle)
          {
            rcl_ret_t ret = handle->action_server->goal_callback(
              goal_handle,
              handle->callback_context);
            switch (ret) {
              case RCL_RET_ACTION_GOAL_ACCEPTED:
                rclc_action_server_response_goal_request(goal_handle, true);
                // Set accepted post-condition
                goal_handle->status = GOAL_STATE_ACCEPTED;
                break;
              case RCL_RET_ACTION_GOAL_REJECTED:
              default:
                rclc_action_server_response_goal_request(goal_handle, false);
                // Set rejected/error post-condition
                rclc_action_remove_used_goal_handle(handle->action_server, goal_handle);
                break;
            }
          }
          handle->action_server->goal_request_available = false;
        }
        if (handle->action_server->cancel_request_available) {
          rclc_action_goal_handle_t * goal_handle;
          for (goal_handle = handle->action_server->used_goal_handles; NULL != goal_handle;
            goal_handle = goal_handle->next)
          {
            if (GOAL_STATE_CANCELING == goal_handle->status) {
              goal_handle->goal_cancelled =
                handle->action_server->cancel_callback(goal_handle, handle->callback_context);
              if (goal_handle->goal_cancelled) {
                rclc_action_server_goal_cancel_accept(goal_handle);
              } else {
                rclc_action_server_goal_cancel_reject(
                  handle->action_server, CANCEL_STATE_REJECTED,
                  goal_handle->cancel_request_header);
                goal_handle->status = GOAL_STATE_EXECUTING;
              }
            }
          }
          handle->action_server->cancel_request_available = false;
        }
        break;

      default:
        RCUTILS_LOG_DEBUG_NAMED(
          ROS_PACKAGE_NAME, "Error in _rclc_execute: unknwon handle type: %d",
          handle->type);
        return RCL_RET_ERROR;
    }   // switch-case

    if (semantics == LET && executor->let_executor->overrun_option != NO_ERROR_HANDLING)
    {
      pthread_mutex_lock(&executor->let_executor->mutex_state);
      state = INACTIVE;
      rc = rclc_set_array(&(handle->callback_info->state), &state, 
              period_index%handle->callback_info->num_period_per_let);    
      if (handle->callback_info->overrun_status == OVERRUN)
          handle->callback_info->overrun_status = HANDLING_ERROR;
      pthread_mutex_unlock(&executor->let_executor->mutex_state);
    }
  }
  return rc;
}


static
rcl_ret_t
_rclc_default_scheduling(rclc_executor_t * executor)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t rc = RCL_RET_OK;

  for (size_t i = 0; (i < executor->max_handles && executor->handles[i].initialized); i++) {
    rc = _rclc_check_for_new_data(&executor->handles[i], &executor->wait_set, RCLCPP_EXECUTOR, 0);
    if ((rc != RCL_RET_OK) && (rc != RCL_RET_SUBSCRIPTION_TAKE_FAILED)) {
      return rc;
    }
  }
  // if the trigger condition is fullfilled, fetch data and execute
  if (executor->trigger_function(
      executor->handles, executor->max_handles,
      executor->trigger_object, executor->data_comm_semantics, executor->let_executor->spin_index))
  {
    // take new input data from DDS-queue and execute the corresponding callback of the handle
    for (size_t i = 0; (i < executor->max_handles && executor->handles[i].initialized); i++) {
      rc = _rclc_take_new_data(&executor->handles[i], &executor->wait_set, RCLCPP_EXECUTOR, 0);
      if ((rc != RCL_RET_OK) && (rc != RCL_RET_SUBSCRIPTION_TAKE_FAILED) &&
        (rc != RCL_RET_SERVICE_TAKE_FAILED))
      {
        return rc;
      }
      rc = _rclc_execute(&executor->handles[i], executor);
      if (rc != RCL_RET_OK) {
        return rc;
      }
    }
  }
  return rc;
}

static
rcl_ret_t
_rclc_let_scheduling(rclc_executor_t * executor)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t rc = RCL_RET_OK;

  // logical execution time
  // 1. read all input (in a separate thread)
  // 2. process
  // 3. write data (in a separate thread)

  // if the trigger condition is fullfilled, fetch data and execute
  // complexity: O(n) where n denotes the number of handles
  if (executor->trigger_function(
      executor->handles, executor->max_handles,
      executor->trigger_object, executor->data_comm_semantics, executor->let_executor->spin_index))
  {
    // step 2:  process (execute)
    int period_index;
    rclc_callback_state_t state;
    for (size_t i = 0; (i < executor->max_handles && executor->handles[i].initialized); i++) 
    {
      if (executor->let_executor->overrun_option != NO_ERROR_HANDLING)
      {
        period_index = executor->let_executor->spin_index%executor->handles[i].callback_info->num_period_per_let;
        rclc_get_array(&executor->handles[i].callback_info->state, &state, period_index);  
        // Skip if callback is not released or callback is overrun (released but not started before deadline)        
        rclc_overrun_status_t overrun_status = executor->handles[i].callback_info->overrun_status;
        if(overrun_status == OVERRUN || overrun_status == HANDLING_ERROR || state != RELEASED)
        {
          continue;
        }
      }

      // Copy data pointer from input buffer to handle data
      if(executor->handles[i].type == RCLC_SUBSCRIPTION || executor->handles[i].type == RCLC_SUBSCRIPTION_WITH_CONTEXT)
      {
        rclc_array_element_status_t status;
        rc = rclc_get_pointer_array(&(executor->handles[i].callback_info->data), 
                executor->let_executor->spin_index%executor->handles[i].callback_info->num_period_per_let, 
                &(executor->handles[i].data), &status);
        if (rc != RCL_RET_OK)
          return rc;
      }

      rc = _rclc_execute(&executor->handles[i], executor);
      if (rc != RCL_RET_OK) {
        return rc;
      }
    }
  }
  return rc;
}

rcl_ret_t
rclc_executor_prepare(rclc_executor_t * executor)
{
  rcl_ret_t rc = RCL_RET_OK;
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "executor_prepare");

  // initialize wait_set if
  // (1) this is the first invocation of executor_spin_some()
  // (2) executor_add_timer() or executor_add_subscription() has been called.
  //     i.e. a new timer or subscription has been added to the Executor.
  if (!rcl_wait_set_is_valid(&executor->wait_set)) {
    // calling wait_set on zero_initialized wait_set multiple times is ok.
    rcl_ret_t rc = rcl_wait_set_fini(&executor->wait_set);
    if (rc != RCL_RET_OK) {
      PRINT_RCLC_ERROR(rclc_executor_spin_some, rcl_wait_set_fini);
    }
    // initialize wait_set
    executor->wait_set = rcl_get_zero_initialized_wait_set();
    // create sufficient memory space for all handles in the wait_set
    rc = rcl_wait_set_init(
      &executor->wait_set, executor->info.number_of_subscriptions,
      executor->info.number_of_guard_conditions, executor->info.number_of_timers,
      executor->info.number_of_clients, executor->info.number_of_services,
      executor->info.number_of_events,
      executor->context,
      *executor->allocator);

    if (rc != RCL_RET_OK) {
      PRINT_RCLC_ERROR(rclc_executor_spin_some, rcl_wait_set_init);
      return rc;
    }
  }

  return rc;
}

rcl_ret_t
rclc_executor_spin_some(rclc_executor_t * executor, const uint64_t timeout_ns)
{
  rcl_ret_t rc = RCL_RET_OK;
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "spin_some");

  // based on semantics process input data
  switch (executor->data_comm_semantics) {
    case LET:
      rc = _rclc_let_scheduling(executor);
      break;
    case RCLCPP_EXECUTOR:
      rcutils_time_point_value_t now;
      rc = rcutils_steady_time_now(&now);
      printf("Period %lu %d %ld\n", (unsigned long) executor, 0, now);
      if (!rcl_context_is_valid(executor->context)) {
        PRINT_RCLC_ERROR(rclc_executor_spin_some, rcl_context_not_valid);
        return RCL_RET_ERROR;
      }

      rclc_executor_prepare(executor);

      // set rmw fields to NULL
      rc = rcl_wait_set_clear(&executor->wait_set);
      if (rc != RCL_RET_OK) {
        PRINT_RCLC_ERROR(rclc_executor_spin_some, rcl_wait_set_clear);
        return rc;
      }

      // add handles to wait_set
      rc = _rclc_add_handles_to_waitset(executor);
      if (rc != RCL_RET_OK) {
        PRINT_RCLC_ERROR(rclc_executor_spin_some, rcl_wait_set_clear);
        return rc;
      }

      // wait up to 'timeout_ns' to receive notification about which handles reveived
      // new data from DDS queue.
      rc = rcl_wait(&executor->wait_set, timeout_ns);
      RCLC_UNUSED(rc);
      rc = _rclc_default_scheduling(executor);
      break;
    default:
      PRINT_RCLC_ERROR(rclc_executor_spin_some, unknown_semantics);
      return RCL_RET_ERROR;
  }

  return rc;
}

rcl_ret_t
rclc_executor_spin(rclc_executor_t * executor)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret = RCL_RET_OK;
  RCUTILS_LOG_DEBUG_NAMED(
    ROS_PACKAGE_NAME,
    "INFO: rcl_wait timeout %ld ms",
    ((executor->timeout_ns / 1000) / 1000));
  executor->let_executor->state = EXECUTING;
  while (true) {
    ret = rclc_executor_spin_some(executor, executor->timeout_ns);
    if (!((ret == RCL_RET_OK) || (ret == RCL_RET_TIMEOUT))) {
      RCL_SET_ERROR_MSG("rclc_executor_spin_some error");
      return ret;
    }
  }
  return ret;
}


/*
 The reason for splitting this function up, is to be able to write a unit test.
 The spin_period is an endless loop, therefore it is not possible to stop after x iterations.
 rclc_executor_spin_period_ implements one iteration and the function
 rclc_executor_spin_period implements the endless while-loop. The unit test covers only
 rclc_executor_spin_period_.
*/
rcl_ret_t
rclc_executor_spin_one_period(rclc_executor_t * executor, const uint64_t period_ns)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret = RCL_RET_OK;
  rcutils_time_point_value_t end_time_point, now;
  rcutils_duration_value_t sleep_time;

  if (executor->data_comm_semantics == LET)
  {
    if(executor->period_ns != period_ns)
    {
      printf("Period is defined twice\n");
      return RCL_RET_ERROR;
    }

    pthread_mutex_lock(&executor->let_executor->mutex);
    while (executor->let_executor->state != INPUT_READ && executor->let_executor->spin_index >= executor->let_executor->input_index)
    {
      pthread_cond_wait(&executor->let_executor->let_input_done, &executor->let_executor->mutex);
    }
    executor->let_executor->state = EXECUTING;
    pthread_mutex_unlock(&executor->let_executor->mutex);

    switch (executor->let_executor->overrun_option)
    {
      case CANCEL_CURRENT_PERIOD:
      case RUN_AT_LOW_PRIORITY:
      case CANCEL_NEXT_PERIOD:
        ret = rclc_executor_spin_some(executor, executor->timeout_ns);
        if (!((ret == RCL_RET_OK) || (ret == RCL_RET_TIMEOUT))) {
          RCL_SET_ERROR_MSG("rclc_executor_spin_some error");
          return ret;
        }
      default:
        break;      
    }

    executor->let_executor->spin_index++;
    executor->invocation_time += period_ns;

    ret = rcutils_system_time_now(&end_time_point);
    
    pthread_mutex_lock(&executor->let_executor->mutex);
    if (executor->invocation_time >= end_time_point)
    {
      executor->let_executor->state = WAIT_INPUT;
    }
    else
    {
      executor->let_executor->state = INPUT_READ;      
    }
    pthread_mutex_unlock(&executor->let_executor->mutex);   
  }
  else 
  {
    if (executor->invocation_time == 0) {
      ret = rcutils_system_time_now(&executor->invocation_time);
      RCLC_UNUSED(ret);
    }
    executor->invocation_time += period_ns;    
    ret = rclc_executor_spin_some(executor, executor->timeout_ns);
    if (!((ret == RCL_RET_OK) || (ret == RCL_RET_TIMEOUT))) {
      RCL_SET_ERROR_MSG("rclc_executor_spin_some error");
      return ret;
    }
  }

  // sleep UNTIL next invocation time point
  ret = rcutils_system_time_now(&end_time_point);
  sleep_time = executor->invocation_time - end_time_point;
  if (sleep_time > 0) {
    rclc_sleep_ns(sleep_time);
  }

  return ret;
}

rcl_ret_t
rclc_executor_spin_period(rclc_executor_t * executor, const uint64_t period_ns)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret;
  pthread_t thread_id_output = 0;
  pthread_t thread_id_input = 0;
  

  if (executor->data_comm_semantics == LET)
  {
    executor->let_executor->state = WAIT_INPUT;
    ret = rcutils_system_time_now(&executor->invocation_time);
    executor->let_executor->input_invocation_time = executor->invocation_time;
    RCLC_UNUSED(ret);
    executor->timeout_ns = 0;
    // Add all callbacks to output_invocation_times queue
    for(size_t i = 0; (i < executor->max_handles && executor->handles[i].initialized); i++)
    {
      const int callback_id = executor->handles[i].callback_info->callback_id;
      rclc_enqueue_priority_queue(&(executor->let_executor->output_invocation_times), &callback_id, 
                                executor->invocation_time + executor->handles[i].callback_info->callback_let_ns);
    }
    const int input_id = -2;
    CHECK_RCL_RET(rclc_enqueue_priority_queue(&(executor->let_executor->output_invocation_times), &input_id, executor->invocation_time), 
                      (unsigned long) executor);
    executor->timeout_ns = 0;
    _rclc_thread_create(&thread_id_output, SCHED_FIFO, 99, -1, _rclc_let_scheduling_output_wrapper, executor);
    //_rclc_thread_create(&thread_id_input, SCHED_FIFO, 99, -1, _rclc_let_scheduling_input_wrapper, executor);    
  }

  while (true) {
    ret = rclc_executor_spin_one_period(executor, period_ns);
    if (!((ret == RCL_RET_OK) || (ret == RCL_RET_TIMEOUT))) {
      RCL_SET_ERROR_MSG("rclc_executor_spin_one_period error");
      break;
    }
  }

  if (executor->data_comm_semantics == LET)
  {
      // should never get here
    pthread_mutex_lock(&executor->let_executor->mutex);
    executor->let_executor->state = IDLE;
    pthread_mutex_unlock(&executor->let_executor->mutex);

    pthread_join(thread_id_output, NULL);
    //pthread_join(thread_id_input, NULL);
  }
  return ret;
}

rcl_ret_t
rclc_executor_spin_period_with_exit(rclc_executor_t * executor, const uint64_t period_ns, volatile bool * exit_flag)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret;
  pthread_t thread_id_output = 0;
  pthread_t thread_id_input = 0;

  if (executor->data_comm_semantics == LET)
  {
    executor->let_executor->state = WAIT_INPUT;
    ret = rcutils_system_time_now(&executor->invocation_time);
    executor->let_executor->input_invocation_time = executor->invocation_time;
    RCLC_UNUSED(ret);
    // Add all callbacks to output_invocation_times queue
    for(size_t i = 0; (i < executor->max_handles && executor->handles[i].initialized); i++)
    {
      const int callback_id = executor->handles[i].callback_info->callback_id;
      int64_t add_wakeup_time = executor->invocation_time + executor->handles[i].callback_info->callback_let_ns;
      CHECK_RCL_RET(rclc_enqueue_priority_queue(&(executor->let_executor->output_invocation_times), &callback_id, add_wakeup_time), 
                      (unsigned long) executor);
    }
    const int input_id = -2;
    CHECK_RCL_RET(rclc_enqueue_priority_queue(&(executor->let_executor->output_invocation_times), &input_id, executor->invocation_time), 
                      (unsigned long) executor);
    executor->timeout_ns = 0;
    _rclc_thread_create(&thread_id_output, SCHED_FIFO, 99, -1, _rclc_let_scheduling_output_wrapper, executor);
    //_rclc_thread_create(&thread_id_input, SCHED_FIFO, 99, -1, _rclc_let_scheduling_input_wrapper, executor);
  }

  while (!(*exit_flag)) {
    ret = rclc_executor_spin_one_period(executor, period_ns);
    if (!((ret == RCL_RET_OK) || (ret == RCL_RET_TIMEOUT))) {
      RCL_SET_ERROR_MSG("rclc_executor_spin_one_period error");
      return ret;
    }      
  }
  
  if (executor->data_comm_semantics == LET)
  {
    pthread_mutex_lock(&executor->let_executor->mutex);
    executor->let_executor->state = IDLE;
    pthread_cond_broadcast(&executor->let_executor->exec_period);
    pthread_cond_broadcast(&executor->let_executor->let_input_done);
    pthread_mutex_unlock(&executor->let_executor->mutex);
    pthread_join(thread_id_output, NULL);
   // pthread_join(thread_id_input, NULL);
  }
  printf("Exe exit %lu\n", (unsigned long) executor);
  return RCL_RET_OK;
}

rcl_ret_t
rclc_executor_set_trigger(
  rclc_executor_t * executor,
  rclc_executor_trigger_t trigger_function,
  void * trigger_object)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  executor->trigger_function = trigger_function;
  executor->trigger_object = trigger_object;
  return RCL_RET_OK;
}

bool rclc_executor_trigger_all(rclc_executor_handle_t * handles, 
  unsigned int size, 
  void * obj,
  rclc_executor_semantics_t semantics,
  uint64_t index)
{
  RCL_CHECK_FOR_NULL_WITH_MSG(handles, "handles is NULL", return false);
  // did not use (i<size && handles[i].initialized) as loop-condition
  // because for last index i==size this would result in out-of-bound access
  RCLC_UNUSED(obj);
  for (unsigned int i = 0; i < size; i++) {
    if (handles[i].initialized) {
      if (_rclc_check_handle_data_available(&handles[i], semantics, index) == false) {
        return false;
      }
    } else {
      break;
    }
  }
  return true;
}

bool rclc_executor_trigger_any(rclc_executor_handle_t * handles, 
  unsigned int size, 
  void * obj,
  rclc_executor_semantics_t semantics,
  uint64_t index)
{
  RCL_CHECK_FOR_NULL_WITH_MSG(handles, "handles is NULL", return false);
  RCLC_UNUSED(obj);
  // did not use (i<size && handles[i].initialized) as loop-condition
  // because for last index i==size this would result in out-of-bound access
  for (unsigned int i = 0; i < size; i++) {
    if (handles[i].initialized) {
      if (_rclc_check_handle_data_available(&handles[i], semantics, index)) {
        return true;
      }
    } else {
      break;
    }
  }
  return false;
}

bool rclc_executor_trigger_one(rclc_executor_handle_t * handles, 
  unsigned int size, 
  void * obj,
  rclc_executor_semantics_t semantics,
  uint64_t index)
{
  RCL_CHECK_FOR_NULL_WITH_MSG(handles, "handles is NULL", return false);
  // did not use (i<size && handles[i].initialized) as loop-condition
  // because for last index i==size this would result in out-of-bound access
  for (unsigned int i = 0; i < size; i++) {
    if (handles[i].initialized) {
      if (_rclc_check_handle_data_available(&handles[i], semantics, index)) {
        void * handle_obj_ptr = rclc_executor_handle_get_ptr(&handles[i]);
        if (NULL == handle_obj_ptr) {
          // rclc_executor_handle_get_ptr returns null for unsupported types
          return false;
        }
        if (obj == handle_obj_ptr) {
          return true;
        }
      }
    } else {
      break;
    }
  }
  return false;
}

bool rclc_executor_trigger_always(rclc_executor_handle_t * handles,
  unsigned int size,
  void * obj,
  rclc_executor_semantics_t semantics,
  uint64_t index)
{
  RCLC_UNUSED(handles);
  RCLC_UNUSED(size);
  RCLC_UNUSED(obj);
  RCLC_UNUSED(semantics);
  RCLC_UNUSED(index);
  return true;
}

/********************* LET Implementation ************************/
rcl_ret_t
rclc_executor_set_period(rclc_executor_t * executor, const uint64_t period_ns)
{
  executor->period_ns = period_ns;
  return RCL_RET_OK;
}

rcl_ret_t
rclc_executor_let_init(
  rclc_executor_t * executor,
  const size_t number_of_let_handles,
  rclc_executor_let_overrun_option_t option)
{
  RCL_CHECK_FOR_NULL_WITH_MSG(executor, "executor is NULL", return RCL_RET_INVALID_ARGUMENT);
  
  CHECK_RCL_RET(rclc_allocate(executor->allocator, (void **) &executor->let_executor, 
                sizeof(rclc_executor_let_t)), (unsigned long) executor);

  executor->let_executor->max_let_handles_per_callback = number_of_let_handles;
  executor->let_executor->overrun_option = option;
  executor->let_executor->state = IDLE;
  executor->let_executor->spin_index = 0;
  executor->let_executor->input_index = 0;
  executor->let_executor->input_invocation_time = 0;
  executor->let_executor->next_callback_id = 0;
  executor->let_executor->deadline_passed = false;
  pthread_cond_init(&(executor->let_executor->exec_period), NULL);
  pthread_mutex_init(&(executor->let_executor->mutex), NULL);
  pthread_mutex_init(&(executor->let_executor->mutex_state), NULL);
  pthread_cond_init(&(executor->let_executor->let_input_done), NULL);
  pthread_cond_init(&(executor->let_executor->cond_callback), NULL);
  CHECK_RCL_RET(rclc_init_priority_queue(&(executor->let_executor->output_invocation_times), sizeof(int), 
    executor->max_handles + 1, executor->allocator), (unsigned long) executor);
  // initialize let handle
  for (size_t i = 0; i < executor->max_handles; i++) {
    CHECK_RCL_RET(rclc_executor_let_handle_init(&executor->handles[i], number_of_let_handles, executor->allocator), (unsigned long) executor);
  }

  return RCL_RET_OK;
}

static
bool
_rclc_executor_let_is_valid(rclc_executor_t * executor)
{
  RCL_CHECK_FOR_NULL_WITH_MSG(executor, "executor pointer is invalid", return false);
  if (executor->let_executor == NULL) {
    return false;
  }
  return true;
}

rcl_ret_t
rclc_executor_set_overrun_option(rclc_executor_t * executor, rclc_executor_let_overrun_option_t option)
{
  RCL_CHECK_FOR_NULL_WITH_MSG(
    executor, "executor is null pointer", return RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret = RCL_RET_OK;
  if (_rclc_executor_is_valid(executor) && _rclc_executor_let_is_valid(executor)) {
    executor->let_executor->overrun_option = option;
  } else {
    RCL_SET_ERROR_MSG("executor not initialized.");
    return RCL_RET_ERROR;
  }
  return ret;
}

rcl_ret_t
rclc_executor_let_fini(rclc_executor_t * executor)
{
  if (_rclc_executor_let_is_valid(executor)) {
    CHECK_RCL_RET(rclc_fini_priority_queue(&(executor->let_executor->output_invocation_times)), (unsigned long) executor);
    executor->let_executor->max_let_handles_per_callback = 0;
    pthread_mutex_destroy(&(executor->let_executor->mutex));
    pthread_cond_destroy(&(executor->let_executor->exec_period));
    pthread_cond_destroy(&(executor->let_executor->let_input_done));
    pthread_cond_destroy(&(executor->let_executor->cond_callback));
    executor->allocator->deallocate(executor->let_executor, executor->allocator->state);
      // de-initialize let handle
    for (size_t i = 0; i < executor->max_handles && executor->handles[i].initialized; i++) {
      rclc_executor_let_handle_fini(&executor->handles[i], executor->allocator);
    }
  } else {
    // Repeated calls to fini or calling fini on a zero initialized executor is ok
  }
  return RCL_RET_OK;
}

rcl_ret_t
rclc_executor_add_publisher_LET(
  rclc_executor_t * executor,
  rclc_publisher_t * publisher,
  const int message_size,
  const int buffer_capacity,
  void * handle_ptr,
  rclc_executor_handle_type_t type)
{
  RCL_CHECK_ARGUMENT_FOR_NULL(executor, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(publisher, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(handle_ptr, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t ret = RCL_RET_OK;
  rclc_executor_let_handle_t let_handle;
  let_handle.type = RCLC_PUBLISHER;
  let_handle.publisher = publisher;
  const char* error_message = NULL;
  
  switch(type)
  {
    case RCLC_SUBSCRIPTION:
    case RCLC_SUBSCRIPTION_WITH_CONTEXT:
      error_message = "Subscription has not been added to the executor\n";
    case RCLC_TIMER:
    case RCLC_TIMER_WITH_CONTEXT:
      if (error_message == NULL) {
          error_message = "Timer has not been added to the executor\n";
      }

      for (size_t i = 0; (i < executor->max_handles && executor->handles[i].initialized); i++)
      {
        if(executor->handles[i].type == type && 
          (executor->handles[i].timer == handle_ptr || executor->handles[i].subscription == handle_ptr))
        {
          if (executor->handles[i].callback_info->let_num >= executor->let_executor->max_let_handles_per_callback)
          {
            printf("Number of publishers in this callback exceeds the pre-defined max let handles per callback\n");
            return RCL_RET_ERROR;
          }
          
          executor->handles[i].callback_info->let_handles[executor->handles[i].callback_info->let_num] = let_handle;
          executor->handles[i].callback_info->let_num++;
          ret = rclc_publisher_let_init(publisher, 
                      message_size, 
                      buffer_capacity, 
                      executor->handles[i].callback_info->num_period_per_let,
                      &(executor->let_executor->spin_index),
                      executor->allocator);
          return ret;
        }
      }
      RCL_SET_ERROR_MSG(error_message);
      printf(error_message);
      return RCL_RET_ERROR;      
    default:
      return ret;
  }
  return ret;
}

// Helper function to update output_invocation_times priority queue
static rcl_ret_t _rclc_update_output_invocation_times(
  rclc_executor_t * executor,
  int * callback_id,
  rcutils_time_point_value_t * wakeup_time) 
{
  rcl_ret_t rc = rclc_dequeue_priority_queue(&executor->let_executor->output_invocation_times, callback_id, (int64_t *) wakeup_time);
  if (rc != RCL_RET_OK) {
    printf("Failed to dequeue priority queue\n");
    return rc;
  }
  int64_t add_wakeup_time = *wakeup_time + executor->period_ns;
  rc = rclc_enqueue_priority_queue(&executor->let_executor->output_invocation_times, callback_id, add_wakeup_time);
  if (rc != RCL_RET_OK) {
    printf("Failed to enqueue priority queue\n");
    return rc;
  }
  return rc;
}

// Function to find the handle index from callback_id
static int _rclc_find_handle_index(
  rclc_executor_t * executor,
  int callback_id) 
{
  for (size_t i = 0; i < executor->max_handles && executor->handles[i].initialized; i++) {
    if(executor->handles[i].callback_info->callback_id == callback_id) {
      return i;
    }
  }
  return -1;
}

// Function to check if an executor has overrun error
bool _rclc_executor_check_overrun_error(rclc_executor_t * executor)
{
  for (size_t i = 0; (i < executor->max_handles && executor->handles[i].initialized); i++) {
    if (executor->handles[i].callback_info->overrun_status == HANDLING_ERROR || executor->handles[i].callback_info->overrun_status == OVERRUN)
    {
      return true;
    }
  }  
  return false;
}

static rcl_ret_t _rclc_get_next_callback_to_write_output(
  rclc_executor_t *executor, 
  rcutils_time_point_value_t *next_wakeup_time, 
  int *callback_id) 
{
  rcl_ret_t rc = RCL_RET_OK;
  rcutils_time_point_value_t current_wakeup_time, now;
  int next_callback_id = -1;
  rc = rcutils_steady_time_now(&now);
  int index = 0;

  rc = rclc_peek_priority_queue(&executor->let_executor->output_invocation_times, &next_callback_id, (int64_t *) &current_wakeup_time);
  callback_id[0] = next_callback_id;
  rc = rcutils_system_time_now(&now);

  if (now < current_wakeup_time) {
    // This is just a wakeup to check next wakeup time
    *next_wakeup_time = current_wakeup_time;
    callback_id[0] = -1;
    printf("Early wakeup %lu %ld\n", (unsigned long) executor, now);
    return rc;
  }

  while (true) {
      rc = _rclc_update_output_invocation_times(executor, &callback_id[index], next_wakeup_time);
      if (rc != RCL_RET_OK) {
          printf("Error update output_invocation_times %lu %ld\n", (unsigned long)executor, now);
          return rc;
      }
      index++;
      rc = rclc_peek_priority_queue(&executor->let_executor->output_invocation_times, &callback_id[index], (int64_t *)next_wakeup_time);
      if (*next_wakeup_time != current_wakeup_time || rc == RCL_RET_ERROR) {
          break;
      }
  }

  callback_id[index] = -1;
  return rc;
}

static rcl_ret_t _rclc_write_output_single_callback(
  rclc_executor_t *executor,
  int callback_id) 
{
  rcl_ret_t rc = RCL_RET_OK;
  rcutils_time_point_value_t now;

  int handle_index = _rclc_find_handle_index(executor, callback_id);
  if (handle_index < 0) 
  {
      return RCL_RET_OK; // Skip if handle_index is invalid.
  }
  int period_index = executor->handles[handle_index].callback_info->output_index;
  executor->handles[handle_index].callback_info->output_index = (period_index + 1) % executor->handles[handle_index].callback_info->num_period_per_let;

  if(executor->let_executor->overrun_option != NO_ERROR_HANDLING)
  {
    rclc_callback_state_t state;
    CHECK_RCL_RET(rclc_get_array(&executor->handles[handle_index].callback_info->state, &state, period_index), 
                                            (unsigned long) executor);
    bool deadline_passed = false;
    if (state == RUNNING || state == RELEASED)
    {
      printf("Callback isn't Finished %lu\n", (unsigned long) executor);
      pthread_mutex_lock(&executor->let_executor->mutex_state);
      rclc_overrun_status_t overrun_status = (state == RUNNING) ? OVERRUN : HANDLING_ERROR;
      executor->handles[handle_index].callback_info->overrun_status = overrun_status;
      // Set all other active instances of this callback to inactive
      for(int i = 0; i < executor->handles[handle_index].callback_info->num_period_per_let; i++)
      {
        if(i == period_index && overrun_status == OVERRUN)
          continue;
        state = INACTIVE;
        CHECK_RCL_RET(rclc_set_array(&(executor->handles[handle_index].callback_info->state), &state, i), 
                                          (unsigned long) executor);
      }
      pthread_mutex_unlock(&executor->let_executor->mutex_state);
      deadline_passed = true;
      printf("deadline passed %lu\n", (unsigned long) executor->handles[handle_index].subscription);
      return RCL_RET_OK;         
    }
  }

  rclc_executor_let_handle_t let_handle;
  for (int i = 0; i < executor->handles[handle_index].callback_info->let_num; i++) 
  {
    let_handle = executor->handles[handle_index].callback_info->let_handles[i];
    if (let_handle.type == RCLC_PUBLISHER) 
    {
      if (executor->let_executor->overrun_option == NO_ERROR_HANDLING)
      {
        rc = rclc_LET_output(let_handle.publisher, period_index);
        if (rc != RCL_RET_OK) {
          RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Error: unable to publish let output: %lu", (unsigned long)let_handle.publisher);
        }
        continue;        
      }

      // There should not be a case where overrun status is OVERRUN but added a guard just in case
      if (executor->handles[handle_index].callback_info->overrun_status == OVERRUN) 
        continue;   

      if (executor->handles[handle_index].callback_info->overrun_status != HANDLING_ERROR)
      {
        rc = rclc_LET_output(let_handle.publisher, period_index);
        if (rc != RCL_RET_OK) {
          printf("Error: unable to publish let output: %lu", (unsigned long)let_handle.publisher);
          RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Error: unable to publish let output: %lu", (unsigned long)let_handle.publisher);
        }
        continue;
      }

      // The overrun callback just finishes, publish output now
      for(int j = 0; j < executor->handles[handle_index].callback_info->num_period_per_let; j++)
      {
        rc = rclc_LET_output(let_handle.publisher, j);
        if (rc != RCL_RET_OK) {
            RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Error: unable to publish let output: %lu", (unsigned long)let_handle.publisher);
        }
      }
    } 
    else
    {
      printf("Error: unknown let handle type: %d", let_handle.type);
      RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Error: unknown let handle type: %d", let_handle.type);
      return RCL_RET_ERROR;
    }
  }

  if ((executor->let_executor->overrun_option != NO_ERROR_HANDLING) && (executor->handles[handle_index].callback_info->overrun_status == HANDLING_ERROR))
  {
    pthread_mutex_lock(&executor->let_executor->mutex_state);
    executor->handles[handle_index].callback_info->overrun_status = NO_ERROR;
    pthread_mutex_unlock(&executor->let_executor->mutex_state);
  } 

  return rc;
}

static rcl_ret_t _rclc_write_output(
  rclc_executor_t *executor,
  int *callback_id) 
{
  rcl_ret_t rc = RCL_RET_OK;

  for (size_t i = 0; callback_id[i] != -1; i++) {
    if (callback_id[i] == -2)
    {
      rcutils_time_point_value_t now;
      rc = rcutils_steady_time_now(&now);
      printf("Period %lu %ld %ld\n", (unsigned long) executor, executor->let_executor->input_index, now);
      rc = _rclc_let_scheduling_input(executor);
      if (rc != RCL_RET_OK)
        printf("Reading input failed \n");      
      pthread_mutex_lock(&executor->let_executor->mutex);
      // Increment input_index
      if (executor->let_executor->state != IDLE){
        executor->let_executor->input_index++;
        // Only update the executor state if it's not executing callbacks
        if (executor->let_executor->state != EXECUTING)
          executor->let_executor->state = INPUT_READ;
        pthread_cond_signal(&executor->let_executor->let_input_done);
      }
      pthread_mutex_unlock(&executor->let_executor->mutex); 
      continue;
    }
    rc = _rclc_write_output_single_callback(executor, callback_id[i]);
    if (rc != RCL_RET_OK) {
      return rc;
    }
  }
  return rc;
}

rcl_ret_t _rclc_let_scheduling_output(
  rclc_executor_t *executor,
  rcutils_time_point_value_t *next_wakeup_time) 
{
  rcl_ret_t rc = RCL_RET_OK;
  int callback_id[executor->max_handles];

  rc = rcutils_steady_time_now(next_wakeup_time);
  if (rclc_is_empty_priority_queue(&executor->let_executor->output_invocation_times)) {
    printf("Empty queue %lu %ld\n", (unsigned long)executor, *next_wakeup_time);
    *next_wakeup_time = -1;
    return rc;
  }

  rc = _rclc_get_next_callback_to_write_output(executor, next_wakeup_time, callback_id);
  if (rc != RCL_RET_OK || callback_id[0] == -1) {
    printf("Get next callback failed\n");
    return rc;
  }

  rc = _rclc_write_output(executor, callback_id);
  return rc;
}

rcl_ret_t _rclc_let_scheduling_input(rclc_executor_t * executor)
{
  rcl_ret_t rc = RCL_RET_OK;
  rcutils_time_point_value_t now;
  const uint64_t timeout_ns = 0;
  rc = _rclc_wait_for_new_input(executor, timeout_ns);

  // step 0: check for available input data from DDS queue
  // complexity: O(n) where n denotes the number of handles
  for (size_t i = 0; (i < executor->max_handles && executor->handles[i].initialized); i++) {
    rc = _rclc_check_for_new_data(&executor->handles[i], &executor->wait_set, 
          executor->data_comm_semantics, executor->let_executor->input_index);
    if ((rc != RCL_RET_OK) && (rc != RCL_RET_SUBSCRIPTION_TAKE_FAILED)) {
      return rc;
    }
  }

  // if the trigger condition is fullfilled, fetch data and execute
  // complexity: O(n) where n denotes the number of handles
  if (executor->trigger_function(
      executor->handles, executor->max_handles,
      executor->trigger_object, executor->data_comm_semantics, executor->let_executor->input_index))
  {  
    // step 1: read input data
    for (size_t i = 0; (i < executor->max_handles && executor->handles[i].initialized); i++) {
      rc = _rclc_take_new_data(&executor->handles[i], &executor->wait_set, LET, executor->let_executor->input_index);
      if ((rc != RCL_RET_OK) && (rc != RCL_RET_SUBSCRIPTION_TAKE_FAILED)) {
        return rc;
      }

      if ((executor->handles[i].type == RCLC_TIMER || executor->handles[i].type == RCLC_TIMER_WITH_CONTEXT) && executor->handles[i].data_available)
      {
        // Ideally only need to update the next call time of the timer.
        // However, because the rcl timer library doesn't support this, the timer callback is temporarily set to NULL
        // to update the next call time only
        rcl_timer_callback_t temp_callback = rcl_timer_exchange_callback(executor->handles[i].timer, NULL);
        rc = rcl_timer_call(executor->handles[i].timer);
        rcl_timer_callback_t temp_callback2 = rcl_timer_exchange_callback(executor->handles[i].timer, temp_callback);
        RCLC_UNUSED(temp_callback2);

        // cancled timer are not handled, return success
        if (rc == RCL_RET_TIMER_CANCELED) {
          rc = RCL_RET_OK;
          executor->handles[i].data_available = false;
        }
        else if (rc != RCL_RET_OK)
          return rc;
        
        rc = rcutils_steady_time_now(&now);
        printf("Input %lu %ld %ld\n", (unsigned long) executor->handles[i].timer, executor->let_executor->input_index, now);
      }

      if (executor->let_executor->overrun_option == NO_ERROR_HANDLING)
        continue;
      
      int period_index = executor->let_executor->input_index%executor->handles[i].callback_info->num_period_per_let;
      if (executor->handles[i].callback_info->overrun_status != OVERRUN && executor->handles[i].data_available)
      {
        pthread_mutex_lock(&executor->let_executor->mutex_state);
        rclc_callback_state_t state = RELEASED;
        CHECK_RCL_RET(rclc_set_array(&(executor->handles[i].callback_info->state), &state, period_index), (unsigned long) executor);
        pthread_mutex_unlock(&executor->let_executor->mutex_state);
      }
    }
  }
  return rc;
}

void * _rclc_let_scheduling_input_wrapper(void * arg)
{
  rclc_executor_t * executor = (rclc_executor_t *) arg;
  rcl_ret_t ret = RCL_RET_OK;
  rcutils_time_point_value_t end_time_point, now;
  rcutils_duration_value_t sleep_time;
  while(true)
  {
    if (executor->data_comm_semantics == LET)
    {
      ret = rcutils_steady_time_now(&now);
      printf("Period %lu %ld %ld\n", (unsigned long) executor, executor->let_executor->input_index, now);
      ret = _rclc_let_scheduling_input(executor);
      if (ret != RCL_RET_OK)
        printf("Reading input failed \n");

      pthread_mutex_lock(&executor->let_executor->mutex);

      // Increment input_index
      if (executor->let_executor->state != IDLE){
        executor->let_executor->input_index++;
        // Only update the executor state if it's not executing callbacks
        if (executor->let_executor->state != EXECUTING)
          executor->let_executor->state = INPUT_READ;
        pthread_cond_signal(&executor->let_executor->let_input_done);
        pthread_mutex_unlock(&executor->let_executor->mutex); 
      }
      else
      {
        // If executor is IDLE, exit thread
        pthread_mutex_unlock(&executor->let_executor->mutex);
        break;        
      }
      // Sleep to the end of the period
      ret = rcutils_system_time_now(&end_time_point);
      executor->let_executor->input_invocation_time += executor->period_ns;
      sleep_time = executor->let_executor->input_invocation_time - end_time_point;
      if (sleep_time > 0) {
        rclc_sleep_ns(sleep_time);
      }
    }
    else
    {
      pthread_mutex_lock(&executor->let_executor->mutex);
      if(executor->let_executor->state == IDLE)
      {
        pthread_mutex_unlock(&executor->let_executor->mutex);
        break;
      }
      pthread_mutex_unlock(&executor->let_executor->mutex);
      rclc_sleep_ms(5);
    }
  }
  return 0;
}

void * _rclc_let_scheduling_output_wrapper(void * arg)
{
  rclc_executor_t * executor = (rclc_executor_t *) arg;
  rcl_ret_t ret = RCL_RET_OK;
  rcutils_time_point_value_t next_wakeup_time, now;
  rcutils_duration_value_t sleep_time;

  while(true)
  {
    if (executor->data_comm_semantics == LET)
    {
      ret = _rclc_let_scheduling_output(executor, &next_wakeup_time);
      if (ret != RCL_RET_OK)
        printf("Writing output failed \n");
      ret = rcutils_system_time_now(&now);
      pthread_mutex_lock(&executor->let_executor->mutex);
      if (executor->let_executor->state == IDLE)
      {
        pthread_mutex_unlock(&executor->let_executor->mutex);
        break;
      }
      if (next_wakeup_time == -1)
      {
        while (executor->let_executor->state != IDLE && now > executor->invocation_time)
        {
          pthread_cond_wait(&executor->let_executor->exec_period, &executor->let_executor->mutex);
        }
        next_wakeup_time = executor->invocation_time; // sleep until next executor period
        ret = rcutils_system_time_now(&now);
      }
      pthread_mutex_unlock(&executor->let_executor->mutex);

      // Sleep to the next wake up point
      sleep_time = next_wakeup_time - now;
      if (sleep_time > 0) {
        rclc_sleep_ns(sleep_time);
      }
    }
    else
    {
      pthread_mutex_lock(&executor->let_executor->mutex);
      if(executor->let_executor->state == IDLE)
      {
        pthread_mutex_unlock(&executor->let_executor->mutex);
        break;
      }
      pthread_mutex_unlock(&executor->let_executor->mutex);
      rclc_sleep_ms(5);
    }
  }
  return 0;
}

void * _rclc_let_spin_some_wrapper(void * arg)
{
  rclc_executor_t * executor = (rclc_executor_t *) arg;
  rcl_ret_t ret = RCL_RET_OK;
  ret = rclc_executor_spin_some(executor, executor->timeout_ns);
  if (!((ret == RCL_RET_OK) || (ret == RCL_RET_TIMEOUT))) {
    RCL_SET_ERROR_MSG("rclc_executor_spin_some error");
    return 0;
  }

  if (executor->data_comm_semantics == LET)
  {
    pthread_mutex_lock(&executor->let_executor->mutex);
    executor->let_executor->state = WAIT_INPUT;
    pthread_cond_signal(&executor->let_executor->cond_callback);
    pthread_mutex_unlock(&executor->let_executor->mutex);
  }
  pthread_detach(pthread_self());
  return 0;
}

void _rclc_print_thread_info() 
{
    pthread_t thread_id = pthread_self();
    printf("Thread id: %lu\n", thread_id);

    int policy;
    struct sched_param param;
    pthread_getschedparam(pthread_self(), &policy, &param);
    printf("Thread priority: %d\n", param.sched_priority);

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    printf("CPU Affinity: ");
    for (int i = 0; i < CPU_SETSIZE; i++) {
        if (CPU_ISSET(i, &cpuset)) {
            printf("%d ", i);
        }
    }
    printf("\n");

    return;
}

void _rclc_thread_create(pthread_t *thread_id, int policy, int priority, int cpu_id, void *(*function)(void *), void * arg)
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
    if (cpu_id >= 0) {
      cpu_set_t cpuset;
      CPU_ZERO(&cpuset);
      CPU_SET(cpu_id, &cpuset);
      ret += pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);
    }
    ret += pthread_create(thread_id, &attr, function, arg);
    if(ret!=0)
      printf("Create thread %lu failed\n", *thread_id);
}

rcl_ret_t _rclc_add_handles_to_waitset(rclc_executor_t * executor)
{
  rcl_ret_t rc = RCL_RET_OK;
  for (size_t i = 0; (i < executor->max_handles && executor->handles[i].initialized); i++) {
    RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "wait_set_add_* %d", executor->handles[i].type);
    switch (executor->handles[i].type) {
      case RCLC_SUBSCRIPTION:
      case RCLC_SUBSCRIPTION_WITH_CONTEXT:
        // add subscription to wait_set and save index
        rc = rcl_wait_set_add_subscription(
          &executor->wait_set, executor->handles[i].subscription,
          &executor->handles[i].index);
        if (rc == RCL_RET_OK) {
          RCUTILS_LOG_DEBUG_NAMED(
            ROS_PACKAGE_NAME,
            "Subscription added to wait_set_subscription[%ld]",
            executor->handles[i].index);
        } else {
          PRINT_RCLC_ERROR(rclc_executor_spin_some, rcl_wait_set_add_subscription);
          return rc;
        }
        break;
      case RCLC_TIMER:
      case RCLC_TIMER_WITH_CONTEXT:
        // add timer to wait_set and save index
        rc = rcl_wait_set_add_timer(
          &executor->wait_set, executor->handles[i].timer,
          &executor->handles[i].index);
        if (rc == RCL_RET_OK) {
          RCUTILS_LOG_DEBUG_NAMED(
            ROS_PACKAGE_NAME, "Timer added to wait_set_timers[%ld]",
            executor->handles[i].index);
        } else {
          PRINT_RCLC_ERROR(rclc_executor_spin_some, rcl_wait_set_add_timer);
          return rc;
        }
        break;
      case RCLC_SERVICE:
      case RCLC_SERVICE_WITH_REQUEST_ID:
      case RCLC_SERVICE_WITH_CONTEXT:
        // add service to wait_set and save index
        rc = rcl_wait_set_add_service(
          &executor->wait_set, executor->handles[i].service,
          &executor->handles[i].index);
        if (rc == RCL_RET_OK) {
          RCUTILS_LOG_DEBUG_NAMED(
            ROS_PACKAGE_NAME, "Service added to wait_set_service[%ld]",
            executor->handles[i].index);
        } else {
          PRINT_RCLC_ERROR(rclc_executor_spin_some, rcl_wait_set_add_service);
          return rc;
        }
        break;
      case RCLC_CLIENT:
      case RCLC_CLIENT_WITH_REQUEST_ID:
        // case RCLC_CLIENT_WITH_CONTEXT:
        // add client to wait_set and save index
        rc = rcl_wait_set_add_client(
          &executor->wait_set, executor->handles[i].client,
          &executor->handles[i].index);
        if (rc == RCL_RET_OK) {
          RCUTILS_LOG_DEBUG_NAMED(
            ROS_PACKAGE_NAME, "Client added to wait_set_client[%ld]",
            executor->handles[i].index);
        } else {
          PRINT_RCLC_ERROR(rclc_executor_spin_some, rcl_wait_set_add_client);
          return rc;
        }
        break;
      case RCLC_GUARD_CONDITION:
        // case RCLC_GUARD_CONDITION_WITH_CONTEXT:
        // add guard_condition to wait_set and save index
        rc = rcl_wait_set_add_guard_condition(
          &executor->wait_set, executor->handles[i].gc,
          &executor->handles[i].index);
        if (rc == RCL_RET_OK) {
          RCUTILS_LOG_DEBUG_NAMED(
            ROS_PACKAGE_NAME, "Guard_condition added to wait_set_client[%ld]",
            executor->handles[i].index);
        } else {
          PRINT_RCLC_ERROR(rclc_executor_spin_some, rcl_wait_set_add_guard_condition);
          return rc;
        }
        break;
      case RCLC_ACTION_CLIENT:
        // add action client to wait_set and save index
        rc = rcl_action_wait_set_add_action_client(
          &executor->wait_set, &executor->handles[i].action_client->rcl_handle,
          &executor->handles[i].index, NULL);
        if (rc == RCL_RET_OK) {
          RCUTILS_LOG_DEBUG_NAMED(
            ROS_PACKAGE_NAME,
            "Action client added to wait_set_action_clients[%ld]",
            executor->handles[i].index);
        } else {
          PRINT_RCLC_ERROR(rclc_executor_spin_some, rcl_wait_set_add_action_client);
          return rc;
        }
        break;
      case RCLC_ACTION_SERVER:
        // add action server to wait_set and save index
        rc = rcl_action_wait_set_add_action_server(
          &executor->wait_set, &executor->handles[i].action_server->rcl_handle,
          &executor->handles[i].index);
        if (rc == RCL_RET_OK) {
          RCUTILS_LOG_DEBUG_NAMED(
            ROS_PACKAGE_NAME,
            "Action server added to wait_set_action_servers[%ld]",
            executor->handles[i].index);
        } else {
          PRINT_RCLC_ERROR(rclc_executor_spin_some, rcl_wait_set_add_action_server);
          return rc;
        }
        break;
      default:
        RCUTILS_LOG_DEBUG_NAMED(
          ROS_PACKAGE_NAME, "Error: unknown handle type: %d",
          executor->handles[i].type);
        PRINT_RCLC_ERROR(rclc_executor_spin_some, rcl_wait_set_add_unknown_handle);
        return RCL_RET_ERROR;
    }
  }
  return rc;
}

static rcl_ret_t _rclc_wait_for_new_input(rclc_executor_t * executor, const uint64_t timeout_ns)
{
  rcl_ret_t rc = RCL_RET_OK;
  if (!rcl_context_is_valid(executor->context)) {
    PRINT_RCLC_ERROR(rclc_executor_spin_some, rcl_context_not_valid);
    return RCL_RET_ERROR;
  }

  rclc_executor_prepare(executor);
  
  // set rmw fields to NULL
  rc = rcl_wait_set_clear(&executor->wait_set);
  if (rc != RCL_RET_OK) {
    PRINT_RCLC_ERROR(rclc_executor_spin_some, rcl_wait_set_clear);
    return rc;
  }
  
  // add handles to wait_set
  rc = _rclc_add_handles_to_waitset(executor);
  if (rc != RCL_RET_OK) {
    PRINT_RCLC_ERROR(rclc_executor_spin_some, rcl_wait_set_clear);
    return rc;
  }
  
  // wait up to 'timeout_ns' to receive notification about which handles reveived
  // new data from DDS queue.
  rc = rcl_wait(&executor->wait_set, timeout_ns);
  return rc;
}

