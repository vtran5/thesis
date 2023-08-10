#ifndef RCLC__EXECUTOR__LET_H_
#define RCLC__EXECUTOR__LET_H_

#if __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdarg.h>

#include <rcl/error_handling.h>
#include <rcutils/logging_macros.h>

#include "rclc/executor_handle.h"
#include "rclc/types.h"
#include "rclc/sleep.h"
#include "rclc/visibility_control.h"
#include "rclc/publisher.h"

/// Enumeration for publisher, server, client, etc that will send messages
typedef enum
{
  RCLC_PUBLISHER,
  // RCLC_SERVICE,
  // RCLC_CLIENT,
  // RCLC_ACTION_CLIENT,
  // RCLC_ACTION_SERVER,
  RCLC_LET_NONE
} rclc_executor_let_handle_type_t;

/// Container for handles that will send messages
typedef struct
{
  /// Type of handle
  rclc_executor_let_handle_type_t type;
  union {
    rclc_publisher_t * publisher;
    rcl_client_t * client;
    rcl_service_t * service;
    rclc_action_client_t * action_client;
    rclc_action_server_t * action_server;
  };
  /// Stores the let (i.e deadline) of the callback
  rcutils_time_point_value_t callback_let;
} rclc_executor_let_handle_t;

typedef struct 
{
  rclc_executor_let_handle_t handle;
	rcl_timer_t timer;
	bool first_run;
  bool timer_triggered;
  bool * data_consumed;
  bool initialized;
	uint64_t period_index;
  int max_msg_per_period;
	rcl_subscription_t * subscriber_arr;
  rclc_array_t data_arr;
	rclc_publisher_t publisher;
	rclc_callback_let_info_t * callback_info;
} rclc_let_output_t;

typedef struct
{
  /// TO DO: change to pointer
  rclc_support_t support;
  /// Container to memory allocator for array handles
  rcl_allocator_t * allocator;
  rclc_let_output_t * output_arr;
  // Maximum size of the output handles 
  size_t max_output_handles;
  // Maximum total number of intermediate handles (timer + subscriber)
  size_t max_intermediate_handles;
  size_t num_intermediate_handles;
  // Index to the next free element in array handles
  size_t index;
  /// Mutex to protect callback state variable (private)
  pthread_mutex_t mutex;
  //int output_index;
  //int output_callback_id;
} rclc_let_output_node_t;

typedef struct
{
  rclc_let_output_t * output;
  uint64_t period_ns;
} rclc_let_timer_callback_context_t;

typedef struct 
{
  rclc_let_output_t * output;
  int subscriber_period_id;
  pthread_mutex_t * mutex;
} rclc_let_data_subscriber_callback_context_t;

rcl_ret_t
rclc_let_output_node_init(
  rclc_let_output_node_t * let_output_node,
  const size_t max_number_of_let_handles,
  const size_t max_intermediate_handles,
  rcl_allocator_t * allocator);

rcl_ret_t
rclc_let_output_node_fini(rclc_let_output_node_t * let_output_node);

rcl_ret_t
rclc_let_output_node_add_publisher(
  rclc_let_output_node_t * let_output_node,
  rclc_executor_handle_t * handles,
  size_t max_handles,
  rclc_publisher_t * publisher,
  const int max_number_per_callback,
  void * handle_ptr,
  rclc_executor_handle_type_t type);

rcl_ret_t
rclc_executor_let_run(rclc_let_output_node_t * let_output_node, bool * exit_flag, uint64_t period_ns);

#if __cplusplus
}
#endif

#endif  // RCLC__EXECUTOR_H_
