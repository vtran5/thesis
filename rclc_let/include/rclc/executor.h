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
#ifndef RCLC__EXECUTOR_H_
#define RCLC__EXECUTOR_H_

#if __cplusplus
extern "C"
{
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef RCLC_LET
#define RCLC_LET
#endif

#include <stdio.h>
#include <stdarg.h>

#include <rcl/error_handling.h>
#include <rcutils/logging_macros.h>

#include "rclc/executor_handle.h"
#include "rclc/types.h"
#include "rclc/sleep.h"
#include "rclc/visibility_control.h"

#include "rclc/action_client.h"
#include "rclc/action_server.h"

#include <pthread.h>
#include <sched.h>

/*! \file executor.h
    \brief The RCLC-Executor provides an Executor based on RCL in which all callbacks are
    processed in a user-defined order.
*/

/// Type definition for trigger function. With the parameters:
/// - array of executor_handles
/// - size of array
/// - application specific struct used in the trigger function
typedef bool (* rclc_executor_trigger_t)(rclc_executor_handle_t *, unsigned int, void *, rclc_executor_semantics_t, uint64_t);

/// LET overrun handling options
typedef enum{
  NO_ERROR_HANDLING,
  CANCEL_CURRENT_PERIOD, // Cancel the callback if it's overrun, user is responsible for cleanup
  CANCEL_CURRENT_PERIOD_NO_OUTPUT, // Same as CANCEL_CURRENT_PERIOD, except no output is published
  CANCEL_NEXT_PERIOD, // Cancel the next instances of callback until the overrunning callback is finished
  RUN_AT_LOW_PRIORITY, // Let the overrun callback run in background (this option would change the temporal order of data)
  //RUN_AT_LOW_PRIORITY_NO_OUTPUT // Let the overrun callback run in background but skip the output
} rclc_executor_let_overrun_option_t;

typedef enum{
  IDLE,
  INPUT_READ,
  EXECUTING,
  WAIT_INPUT
} rclc_executor_state_t;

typedef struct 
{
  /// State of the executor
  rclc_executor_state_t state;  
  /// Overrun handling option
  rclc_executor_let_overrun_option_t overrun_option;
  /// Maximum number of 'let_handles' per callback (private)
  size_t max_let_handles_per_callback;
  /// Flag to signal overrun
  bool deadline_passed;
  /// Condition variables to signal callback status (private)
  pthread_cond_t cond_callback;
  /// Condition variables for LET scheduling (private)
  pthread_cond_t exec_period;
  pthread_cond_t let_input_done;
  /// Mutex to protect variable (private)
  pthread_mutex_t mutex;
  /// Mutex to protect callback state
  pthread_mutex_t mutex_state;
    /// Id of the next added handle (private)
  int next_callback_id;
  /// period index of the executor
  uint64_t spin_index;
  /// period index of the input thread
  uint64_t input_index;
  /// timepoint used for input thread wakeup
  rcutils_time_point_value_t input_invocation_time;
  /// Queue to store wakeup time for the LET output (private)
  rclc_priority_queue_t output_invocation_times;
} rclc_executor_let_t;

/// Container for RCLC-Executor
typedef struct rclc_executor_s
{
  /// Context (to get information if ROS is up-and-running)
  rcl_context_t * context;
  /// Container for dynamic array for DDS-handles
  rclc_executor_handle_t * handles;
  /// Maximum size of array 'handles'
  size_t max_handles;
  /// Index to the next free element in array handles
  size_t index;
  /// Container to memory allocator for array handles
  const rcl_allocator_t * allocator;
  /// Wait set (is initialized only in the first call of the rclc_executor_spin_some function)
  rcl_wait_set_t wait_set;
  /// Statistics objects about total number of subscriptions, timers, clients, services, etc.
  rclc_executor_handle_counters_t info;
  /// timeout in nanoseconds for rcl_wait() used in rclc_executor_spin_once(). Default 100ms
  uint64_t timeout_ns;
  /// timepoint used for spin_period()
  rcutils_time_point_value_t invocation_time;
  /// trigger function, when to process new data
  rclc_executor_trigger_t trigger_function;
  /// application specific data structure for trigger function
  void * trigger_object;
  /// data communication semantics
  rclc_executor_semantics_t data_comm_semantics;
  /// Period of the executor
  uint64_t period_ns;
  rclc_executor_let_t * let_executor;
  /// Variable to store the overhead
  rcutils_time_point_value_t input_overhead;
  rcutils_time_point_value_t output_overhead;
  uint64_t output_index;
} rclc_executor_t;

/**
 *  Return a rclc_executor_t struct with pointer members initialized to `NULL`
 *  and member variables to 0.
 */
RCLC_PUBLIC
rclc_executor_t
rclc_executor_get_zero_initialized_executor(void);

/**
 *  Initializes an executor.
 *  It creates a dynamic array with size \p number_of_handles using the
 *  \p allocator.
 * As the Executor is intended for embedded controllers, dynamic memory management is crucial.
 *  Therefore at initialization of the RCLC-Executor, the user defines the total \p number_of_handles.
 * A handle is a term for subscriptions, timers, services, clients and guard conditions. The
 * heap will be allocated only in this phase and no more memory will be allocated in the
 * running phase in the executor.
 *
 * Also in the XRCE-DDS middleware the maximum number are configured. See [Memory Management Tutorial](https://docs.vulcanexus.org/en/humble/rst/tutorials/micro/memory_management/memory_management.html#entity-creation)
 * for the default values. If you need larger values, you need to update your colcon.meta
 * configuration file and rebuild. To make sure that the changes were applied, you can check
 * the defined values in the following library include file:
 * build/rmw_microxrcedds/include/rmw_microxrcedds_c/config.h
 *
 * The heap memory of corresponding wait-set is
 * allocated in the first iteration of a spin-method, which calls internally rclc_executor_prepare.
 * Optionally, you can also call rclc_executor_prepare before calling any of the spin-methods.
 * Then all wait-set related memory allocation will be done in rclc_executor_prepare and not
 * in the first iteration of the spin-method.
 *
 * This makes this Executor static in
 * terms of memory allocation, in the sense, that during runtime no heap allocations occur.
 * You can add, however, at runtime as many handles, e.g. subscriptions, to the executor
 * until the maximum number of handles is reached. In this case, the wait-set needs to be
 * updated and rclc_executor_prepare is called again (with dynamic memory allocation in RCL).
 *
 *
 *  * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param[inout] e preallocated rclc_executor_t
 * \param[in] context RCL context
 * \param[in] number_of_handles is the total number of subscriptions, timers, services,
 *  clients and guard conditions. Do not include the number of nodes and publishers.
 * \param[in] allocator allocator for allocating memory
 * \return `RCL_RET_OK` if the executor was initialized successfully
 * \return `RCL_RET_INVALID_ARGUMENT` if any null pointer as argument
 * \return `RCL_RET_ERROR` in case of failure
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_init(
  rclc_executor_t * executor,
  rcl_context_t * context,
  const size_t number_of_handles,
  const rcl_allocator_t * allocator);

/**
 *  Set timeout in nanoseconds for rcl_wait (called during {@link rclc_executor_spin_once()}).
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [inout] executor pointer to an initialized executor
 * \param [in] timeout_ns  timeout in nanoseconds for the rcl_wait (DDS middleware)
 * \return `RCL_RET_OK` if timeout was set successfully
 * \return `RCL_RET_INVALID_ARGUMENT` if \p executor is a null pointer
 * \return `RCL_RET_ERROR` in an error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_set_timeout(
  rclc_executor_t * executor,
  const uint64_t timeout_ns);

/**
 *  Set data communication semantics
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [inout] executor pointer to an initialized executor
 * \param [in] valid semantics value as defined in enum type {@link rclc_executor_semantics_t}
 * \return `RCL_RET_OK` if semantics was set successfully
 * \return `RCL_RET_INVALID_ARGUMENT` if \p executor is a null pointer
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_set_semantics(
  rclc_executor_t * executor,
  rclc_executor_semantics_t semantics);

/**
 *  Cleans up executor.
 *  Deallocates dynamic memory of {@link rclc_executor_t.handles} and
 *  resets all other values of {@link rclc_executor_t}.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [inout] executor pointer to initialized executor
 * \return `RCL_RET_OK` if reset operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if \p executor is a null pointer
 * \return `RCL_RET_INVALID_ARGUMENT` if \p executor.handles is a null pointer
 * \return `RCL_RET_ERROR` in an error occured (aka executor was not initialized)
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_fini(rclc_executor_t * executor);

/**
 *  Adds a subscription to an executor.
 * * An error is returned, if {@link rclc_executor_t.handles} array is full.
 * * The total number_of_subscriptions field of {@link rclc_executor_t.info}
 *   is incremented by one.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [inout] executor pointer to initialized executor
 * \param [in] subscription pointer to an allocated subscription
 * \param [in] msg pointer to an allocated message
 * \param [in] callback    function pointer to a callback
 * \param [in] invocation  invocation type for the callback (ALWAYS or only ON_NEW_DATA)
 * \param [in] callback_let_ns the let duration of the associated callback
 * \return `RCL_RET_OK` if add-operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if any parameter is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_add_subscription(
  rclc_executor_t * executor,
  rcl_subscription_t * subscription,
  void * msg,
  rclc_subscription_callback_t callback,
  rclc_executor_handle_invocation_t invocation,
  rcutils_time_point_value_t callback_let_ns,
  int message_size);

/**
 *  Adds a subscription to an executor.
 * * An error is returned, if {@link rclc_executor_t.handles} array is full.
 * * The total number_of_subscriptions field of {@link rclc_executor_t.info}
 *   is incremented by one.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [inout] executor pointer to initialized executor
 * \param [in] subscription pointer to an allocated subscription
 * \param [in] msg pointer to an allocated message
 * \param [in] callback    function pointer to a callback
 * \param [in] context     type-erased ptr to additional callback context
 * \param [in] invocation  invocation type for the callback (ALWAYS or only ON_NEW_DATA)
 * \param [in] callback_let_ns the let duration of the associated callback
 * \return `RCL_RET_OK` if add-operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if any parameter is a null pointer (NULL context is ignored)
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_add_subscription_with_context(
  rclc_executor_t * executor,
  rcl_subscription_t * subscription,
  void * msg,
  rclc_subscription_callback_with_context_t callback,
  void * context,
  rclc_executor_handle_invocation_t invocation,
  rcutils_time_point_value_t callback_let_ns,
  int message_size);

/**
 *  Adds a timer with context to an executor.
 *  Because the callback is executed by executor in rclc instead of rcl, 
 *  there will be no last_time_call passed into the callback
 * * An error is returned, if {@link rclc_executor_t.handles} array is full.
 * * The total number_of_timers field of {@link rclc_executor_t.info} is
 *   incremented by one.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [inout] executor pointer to initialized executor
 * \param [in] timer pointer to an allocated timer
 * \param [in] callback    function pointer to a callback
 * \param [in] context     type-erased ptr to additional callback context
 * \param [in] callback_let_ns the let duration of the associated callback
 * \return `RCL_RET_OK` if add-operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if any parameter is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_add_timer_with_context(
  rclc_executor_t * executor,
  rcl_timer_t * timer,
  rclc_timer_callback_with_context_t callback,
  void * context,
  rcutils_time_point_value_t callback_let_ns);

/**
 *  Adds a timer to an executor.
 * * An error is returned, if {@link rclc_executor_t.handles} array is full.
 * * The total number_of_timers field of {@link rclc_executor_t.info} is
 *   incremented by one.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [inout] executor pointer to initialized executor
 * \param [in] timer pointer to an allocated timer
 * \param [in] callback_let_ns the let duration of the associated callback
 * \return `RCL_RET_OK` if add-operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if any parameter is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_add_timer(
  rclc_executor_t * executor,
  rcl_timer_t * timer,
  rcutils_time_point_value_t callback_let_ns);

/**
 *  Adds a client to an executor.
 * * An error is returned if {@link rclc_executor_t.handles} array is full.
 * * The total number_of_clients field of {@link rclc_executor_t.info}
 *   is incremented by one.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [inout] executor pointer to initialized executor
 * \param [in] client pointer to a allocated and initialized client
 * \param [in] request_msg type-erased ptr to an allocated request message
 * \param [in] callback    function pointer to a callback function
 * \return `RCL_RET_OK` if add-operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if any parameter is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_add_client(
  rclc_executor_t * executor,
  rcl_client_t * client,
  void * response_msg,
  rclc_client_callback_t callback);

/**
 *  Adds a client to an executor.
 * * An error is returned if {@link rclc_executor_t.handles} array is full.
 * * The total number_of_clients field of {@link rclc_executor_t.info}
 *   is incremented by one.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [inout] executor pointer to initialized executor
 * \param [in] client pointer to a allocated and initialized client
 * \param [in] request_msg type-erased ptr to an allocated request message
 * \param [in] callback    function pointer to a callback function with request_id
 * \return `RCL_RET_OK` if add-operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if any parameter is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_add_client_with_request_id(
  rclc_executor_t * executor,
  rcl_client_t * client,
  void * response_msg,
  rclc_client_callback_with_request_id_t callback);

/**
 *  Adds a service to an executor.
 * * An error is returned if {@link rclc_executor_t.handles} array is full.
 * * The total number_of_services field of {@link rclc_executor_t.info}
 *   is incremented by one.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [inout] executor pointer to initialized executor
 * \param [in] service pointer to an allocated and initialized service
 * \param [in] request_msg type-erased ptr to an allocated request message
 * \param [in] response_msg type-erased ptr to an allocated response message
 * \param [in] callback    function pointer to a callback function
 * \return `RCL_RET_OK` if add-operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if any parameter is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_add_service(
  rclc_executor_t * executor,
  rcl_service_t * service,
  void * request_msg,
  void * response_msg,
  rclc_service_callback_t callback);

/**
 *  Adds a action client to an executor.
 *  An error is returned if {@link rclc_executor_t.handles} array is full.
 *  The total number_of_action_clients field of {@link rclc_executor_t.info}
 *  is incremented by one.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [inout] executor pointer to initialized executor
 * \param [in] action_client pointer to a allocated and initialized action client
 * \param [in] handles_number max number of goals to handle with the client
 * \param [in] ros_result_response type-erased ptr to an allocated ROS result message
 * \param [in] ros_feedback type-erased ptr to an allocated ROS feedback message
 * \param [in] goal_callback function pointer to a goal callback
 * \param [in] feedback_callback function pointer to a feedback callback
 * \param [in] result_callback function pointer to a result callback
 * \param [in] cancel_callback function pointer to a result cancel callback
 * \param [in] context context to pass to the callback functions
 * \return `RCL_RET_OK` if add-operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if any parameter is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */
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
  void * context);

/**
 *  Adds a action server to an executor.
 * * An error is returned if {@link rclc_executor_t.handles} array is full.
 * * The total number_of_action_servers field of {@link rclc_executor_t.info}
 *   is incremented by one.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [inout] executor pointer to initialized executor
 * \param [in] action_server pointer to a allocated and initialized action server
 * \param [in] handles_number max number of goals to handle with the server
 * \param [in] ros_goal_request type-erased ptr to an allocated ROS goal request message
 * \param [in] ros_goal_request_size size of the ROS goal request message type
 * \param [in] goal_callback    function pointer to a goal request callback
 * \param [in] cancel_callback    function pointer to a cancel request callback
 * \param [in] context context to pass to the callback functions
 * \return `RCL_RET_OK` if add-operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if any parameter is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */

rcl_ret_t
rclc_executor_add_action_server(
  rclc_executor_t * executor,
  rclc_action_server_t * action_server,
  size_t handles_number,
  void * ros_goal_request,
  size_t ros_goal_request_size,
  rclc_action_server_handle_goal_callback_t goal_callback,
  rclc_action_server_handle_cancel_callback_t cancel_callback,
  void * context);

/**
 *  Adds a service to an executor.
 * * An error is returned if {@link rclc_executor_t.handles} array is full.
 * * The total number_of_services field of {@link rclc_executor_t.info}
 *   is incremented by one.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [inout] executor pointer to initialized executor
 * \param [in] service pointer to an allocated and initialized service
 * \param [in] request_msg type-erased ptr to an allocated request message
 * \param [in] response_msg type-erased ptr to an allocated response message
 * \param [in] callback    function pointer to a callback function with request_id
 * \return `RCL_RET_OK` if add-operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if any parameter is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_add_service_with_request_id(
  rclc_executor_t * executor,
  rcl_service_t * service,
  void * request_msg,
  void * response_msg,
  rclc_service_callback_with_request_id_t callback);

/**
 *  Adds a service to an executor.
 * * An error is returned if {@link rclc_executor_t.handles} array is full.
 * * The total number_of_services field of {@link rclc_executor_t.info}
 *   is incremented by one.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [inout] executor pointer to initialized executor
 * \param [in] service pointer to an allocated and initialized service
 * \param [in] request_msg type-erased ptr to an allocated request message
 * \param [in] response_msg type-erased ptr to an allocated response message
 * \param [in] callback function pointer to a callback function with request_id
 * \param [in] context type-erased ptr to additional service context
 * \return `RCL_RET_OK` if add-operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if any parameter is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_add_service_with_context(
  rclc_executor_t * executor,
  rcl_service_t * service,
  void * request_msg,
  void * response_msg,
  rclc_service_callback_with_context_t callback,
  void * context);

/**
 *  Adds a guard_condition to an executor.
 * * An error is returned if {@link rclc_executor_t.handles} array is full.
 * * The total number_of_guard_conditions field of {@link rclc_executor_t.info}
 *   is incremented by one.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [inout] executor pointer to initialized executor
 * \param [in] gc pointer to an allocated and initialized guard_condition
 * \param [in] callback    function pointer to a callback function
 * \return `RCL_RET_OK` if add-operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if any parameter is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_add_guard_condition(
  rclc_executor_t * executor,
  rcl_guard_condition_t * gc,
  rclc_gc_callback_t callback);


/**
 *  Removes a subscription from an executor.
 * * An error is returned if {@link rclc_executor_t.handles} array is empty.
 * * An error is returned if subscription is not found in {@link rclc_executor_t.handles}.
 * * The total number_of_subscriptions field of {@link rclc_executor_t.info}
 *   is decremented by one.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [inout] executor pointer to initialized executor
 * \param [in] subscription pointer to an allocated and initialized subscription previously added to executor
 * \return `RCL_RET_OK` if add-operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if any parameter is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_remove_subscription(
  rclc_executor_t * executor,
  const rcl_subscription_t * subscription);


/**
 *  Removes a timer from an executor.
 * * An error is returned if {@link rclc_executor_t.handles} array is empty.
 * * An error is returned if timer is not found in {@link rclc_executor_t.handles}.
 * * The total number_of_timers field of {@link rclc_executor_t.info}
 *   is incremented by one.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [inout] executor pointer to initialized executor
 * \param [in] timer pointer to an allocated and initialized timer previously added to executor
 * \return `RCL_RET_OK` if add-operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if any parameter is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_remove_timer(
  rclc_executor_t * executor,
  const rcl_timer_t * timer);


/**
 *  Removes a client from an executor.
 * * An error is returned if {@link rclc_executor_t.handles} array is empty.
 * * An error is returned if client is not found in {@link rclc_executor_t.handles}.
 * * The total number_of_clients field of {@link rclc_executor_t.info}
 *   is incremented by one.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [inout] executor pointer to initialized executor
 * \param [in] client pointer to an allocated and initialized client previously added to executor
 * \return `RCL_RET_OK` if add-operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if any parameter is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_remove_client(
  rclc_executor_t * executor,
  const rcl_client_t * client);


/**
 *  Removes a service from an executor.
 * * An error is returned if {@link rclc_executor_t.handles} array is empty.
 * * An error is returned if service is not found in {@link rclc_executor_t.handles}.
 * * The total number_of_services field of {@link rclc_executor_t.info}
 *   is incremented by one.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [inout] executor pointer to initialized executor
 * \param [in] service pointer to an allocated and initialized service previously added to executor
 * \return `RCL_RET_OK` if add-operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if any parameter is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_remove_service(
  rclc_executor_t * executor,
  const rcl_service_t * service);

/**
 *  Removes a guard_condition from an executor.
 * * An error is returned if {@link rclc_executor_t.handles} array is empty.
 * * An error is returned if guard_condition is not found in {@link rclc_executor_t.handles}.
 * * The total number_of_guard_conditions field of {@link rclc_executor_t.info}
 *   is incremented by one.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [inout] executor pointer to initialized executor
 * \param [in] guard_condition pointer to an allocated and initialized guard_condition previously added to executor
 * \return `RCL_RET_OK` if add-operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if any parameter is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_remove_guard_condition(
  rclc_executor_t * executor,
  const rcl_guard_condition_t * guard_condition);

/**
 *  The executor prepare function prepare the waitset of the executor if
 *  it is invalid. Does nothing if a valid waitset is already prepared.
 *
 * Memory is dynamically allocated within rcl-layer, when DDS queue is accessed with rcl_wait_set_init()
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 *
 * \param [inout] executor pointer to initialized executor
 * \return `RCL_RET_OK` if executor prepare operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if any parameter is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_prepare(
  rclc_executor_t * executor);

/**
 *  The spin-some function checks one-time for new data from the DDS-queue.
 * * the timeout is defined in {@link rclc_executor_t.timeout_ns} and can
 *   be set by calling {@link rclc_executor_set_timeout()} function (default value is 100ms)
 *
 * The static-LET executor performs the following actions:
 * * initializes the wait_set with all handle of the array executor->handles
 * * waits for new data from DDS queue with rcl_wait() with timeout executor->timeout_ns
 * * takes all ready handles from the wait_set with rcl_take()
 * * processes all handles in the order, how they were added to the executor with the respective add-functions
 *   by calling respective callback (thus implementing first-read, process, semantic of LET)
 *
 * Memory is dynamically allocated within rcl-layer, when DDS queue is accessed with rcl_wait_set_init()
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 *
 * \param [inout] executor pointer to initialized executor
 * \param[in] timeout_ns  timeout in nanoseconds
 * \return `RCL_RET_OK` if spin_once operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if any parameter is a null pointer
 * \return `RCL_RET_TIMEOUT` if rcl_wait() returned timeout (aka no data is avaiable during until the timeout)
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_spin_some(
  rclc_executor_t * executor,
  const uint64_t timeout_ns);

/**
 *  The spin function checks for new data at DDS queue as long as ros context is available.
 *  It calls {@link rclc_executor_spin_some()} as long as rcl_is_context_is_valid() returns true.
 *
 *  Memory is dynamically allocated within rcl-layer, when DDS queue is accessed with rcl_wait_set_init()
 *  (in spin_some function)
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 *
 * \param [inout] executor pointer to initialized executor
 * \return `RCL_RET_OK` if spin operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if executor is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_spin(rclc_executor_t * executor);

/**
 *  The spin_period function checks for new data at DDS queue as long as ros context is available.
 *  It is called every period nanoseconds.
 *  It calls {@link rclc_executor_spin_some()} as long as rcl_is_context_is_valid() returns true.
 *
 *  Memory is dynamically allocated within rcl-layer, when DDS queue is accessed with rcl_wait_set_init()
 *  (in spin_some function)
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 *
 * \param [inout] executor pointer to initialized executor
 * \param [in] period in nanoseconds
 * \return `RCL_RET_OK` if spin operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if executor is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_spin_period(
  rclc_executor_t * executor,
  const uint64_t period);

/**
 *  The spin_period function with exit condition.
 *  It calls {@link rclc_executor_spin_some()} as long as rcl_is_context_is_valid() returns true.
 *
 *  Memory is dynamically allocated within rcl-layer, when DDS queue is accessed with rcl_wait_set_init()
 *  (in spin_some function)
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 *
 * \param [inout] executor pointer to initialized executor
 * \param [in] period in nanoseconds
 * \param [in] exit_flag set to true to exit
 * \return `RCL_RET_OK` if spin operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if executor is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_spin_period_with_exit(rclc_executor_t * executor, const uint64_t period, volatile bool * exit_flag);

/**
 * The reason for splitting up the rclc_executor_spin_period function, is only to write a
 * unit test for testing the accuracy of the period duration.
 *
 * The rclc_executor_spin_period is an endless loop, therefore it is not possible to stop
 * after x iterations. The function rclc_executor_spin_one_period implements one iteration.
 * The unit test for rclc_executor_spin_period covers only rclc_executor_spin_one_period.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 *
 * \param [inout] executor pointer to initialized executor
 * \param [in] period in nanoseconds
 * \return `RCL_RET_OK` if spin operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if executor is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_spin_one_period(
  rclc_executor_t * executor,
  const uint64_t period);

/**
 * Set the trigger condition.
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 *
 * \param [inout] executor pointer to initialized executor
 * \param [in] trigger_function function of the trigger condition
 * \param [in] trigger_object  pointer to a rcl-handle used in the trigger
 * \return `RCL_RET_OK` if spin operation was successful
 * \return `RCL_RET_INVALID_ARGUMENT` if executor is a null pointer
 * \return `RCL_RET_ERROR` if any other error occured
 */
RCLC_PUBLIC
rcl_ret_t
rclc_executor_set_trigger(
  rclc_executor_t * executor,
  rclc_executor_trigger_t trigger_function,
  void * trigger_object);

/**
 * Trigger condition: all, returns true if all handles are ready.
 *
 * Parameter obj is not used.
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [in] handles pointer to array of handles
 * \param [in] size size of array
 * \param [in] obj trigger_object set by rclc_executor_set_trigger (not used)
 * \return true - if all handles are ready (subscriptions have new data, timers are ready)
 * \return false - otherwise
 */
RCLC_PUBLIC
bool
rclc_executor_trigger_all(
  rclc_executor_handle_t * handles,
  unsigned int size,
  void * obj,
  rclc_executor_semantics_t semantics,
  uint64_t index);

/**
 * Trigger condition: any, returns true if at least one handles is ready.
 *
 * Parameter obj is not used.
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [in] handles pointer to array of handles
 * \param [in] size size of array
 * \param [in] obj trigger_object set by rclc_executor_set_trigger (not used)
 * \return true - if at least one handles is ready (subscriptions have new data, timers are ready)
 * \return false - otherwise
 */
RCLC_PUBLIC
bool
rclc_executor_trigger_any(
  rclc_executor_handle_t * handles,
  unsigned int size,
  void * obj,
  rclc_executor_semantics_t semantics,
  uint64_t index);

/**
 * Trigger condition: always, returns always true.
 *
 * Parameter handles, size and obj are not used.
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [in] handles pointer to array of handles (not used)
 * \param [in] size size of array (not used)
 * \param [in] obj trigger_object set by rclc_executor_set_trigger (not used)
 * \return true always
 */
RCLC_PUBLIC
bool
rclc_executor_trigger_always(
  rclc_executor_handle_t * handles,
  unsigned int size,
  void * obj,
  rclc_executor_semantics_t semantics,
  uint64_t index);

/**
 * Trigger condition: one, returns true, if rcl handle obj is ready
 * (when obj is a subscription, if new data available,
 *  when obj is a timer, if the timer is ready)
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param [in] handles pointer to array of handles (not used)
 * \param [in] size size of array (not used)
 * \param [in] obj trigger_object set by rclc_executor_set_trigger
 * \return true if rcl-handle obj is ready
 * \return false otherwise
 */
RCLC_PUBLIC
bool
rclc_executor_trigger_one(
  rclc_executor_handle_t * handles,
  unsigned int size,
  void * obj,
  rclc_executor_semantics_t semantics,
  uint64_t index);

/********************* LET Implementation ************************/
RCLC_PUBLIC
rcl_ret_t
rclc_executor_set_period(rclc_executor_t * executor, 
  const uint64_t period);

RCLC_PUBLIC
rcl_ret_t
rclc_executor_let_init(
  rclc_executor_t * executor,
  const size_t number_of_let_handles,
  rclc_executor_let_overrun_option_t option);

RCLC_PUBLIC
rcl_ret_t
rclc_executor_let_fini(rclc_executor_t * executor);

RCLC_PUBLIC
rcl_ret_t
rclc_executor_set_overrun_option(
  rclc_executor_t * executor,
  rclc_executor_let_overrun_option_t option);

// Must call after adding all the callback handles
RCLC_PUBLIC
rcl_ret_t
rclc_executor_add_publisher_LET(
  rclc_executor_t * executor,
  rclc_publisher_t * publisher,
  const int message_size,
  const int buffer_capacity,
  void * handle_ptr,
  rclc_executor_handle_type_t type);

#if __cplusplus
}
#endif

#endif  // RCLC__EXECUTOR_H_
