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
  rcl_timer_t timer;
  rclc_callback_let_info_t * callback_info;
  bool first_run;
  uint64_t index;
} rclc_let_timer_t;

typedef struct 
{
  rcl_subscription_t subscriber;
  rcl_publisher_t publisher;
  rclc_callback_let_info_t * callback_info;
  // Index to identify each callback instance
  int period_id;
} rclc_let_data_channel_t;

typedef struct 
{
	rcl_timer_t timer;
	bool first_run;
	uint64_t index;
	rcl_subscription_t * subscriber_arr;
	rcl_publisher_t * publisher_arr;
	rclc_callback_let_info_t * callback_info;
} rclc_let_output_callback_t;


typedef struct
{
  rcl_node_t rcl_node;
  rcl_publisher_t publisher;
  // Container to store let timer, each let handle should have 1 timer
  rclc_let_timer_t * timer_arr;
  // Index to the next free element in timer array
  size_t index;
} rclc_let_deadline_tracker_node_t;

typedef struct
{
  rcl_node_t rcl_node;
  rcl_subscription_t timer_subscriber;
  rclc_let_data_channel_t * data_channel_arr;
  size_t index;
}rclc_let_time_sync_node_t;

typedef struct
{
  // Container for dynamic array for DDS-let-handles
  rclc_executor_let_handle_t * handles;
  // Maximum size of array 'handles'
  size_t max_handles;
  // Index to the next free element in array handles
  size_t index;
  /// Container to memory allocator for array handles
  const rcl_allocator_t * allocator;
  rclc_let_output_callback_t * callback_arr;
  //rclc_let_deadline_tracker_node_t let_deadline_node;
  //rclc_let_time_sync_node_t time_sync_node;
  //int output_index;
  //int output_callback_id;
} rclc_executor_let_t;

#if __cplusplus
}
#endif

#endif  // RCLC__EXECUTOR_H_