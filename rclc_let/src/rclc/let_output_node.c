#include "rclc/let_output_node.h"
#include <rcutils/time.h>
#include "rclc/rclc.h"
#include "rclc/executor.h"

rcl_ret_t _rclc_create_intermediate_topic(
	char ** intermediate_topic, 
	char * original_topic, 
	int index)
{
	char num_str[12];
	sprintf(num_str, "%d", index);
	strcpy(*intermediate_topic, original_topic);
	strcat(*intermediate_topic, num_str);	
	return RCL_RET_OK;
}

rcl_ret_t _rclc_output_handle_zero_init(rclc_let_output_t * output)
{
	output->data_consumed = NULL;
	output->initialized = false;
	output->subscriber_arr = NULL;
	output->callback_info = NULL;
	return RCL_RET_OK;
}

// The callback needs to be added before initialize the output handle (requires num_period_per_let)
rcl_ret_t _rclc_output_handle_init(
	rclc_let_output_t * output, 
	rcl_allocator_t * allocator,
	rclc_executor_let_handle_t handle)
{
	int num_period_per_let = output->callback_info->num_period_per_let;
	output->handle = handle;
	output->first_run = true;
	output->timer_triggered = false;
	output->period_index = 0;
	char * intermediate_topic;

	switch(output->handle.type)
	{
	case RCLC_PUBLISHER:
		CHECK_RCL_RET(rclc_allocate(allocator, (void **) &output->subscriber_arr, num_period_per_let*sizeof(rcl_subscription_t)),
									 (unsigned long) output);

		CHECK_RCL_RET(rclc_allocate(allocator, (void **) &output->data_consumed, num_period_per_let*sizeof(bool)),
									 (unsigned long) output);

		CHECK_RCL_RET(rclc_allocate(allocator, (void **) &output->handle.publisher->let_publisher->let_publishers, num_period_per_let*sizeof(rcl_publisher_t)),
									 (unsigned long) output);

		CHECK_RCL_RET(rclc_allocate(allocator, (void **) &intermediate_topic, strlen(output->handle.publisher->let_publisher->topic_name)+ 13),
									 (unsigned long) output);

		rcl_publisher_options_t option = rcl_publisher_get_default_options();
		option.qos = *output->handle.publisher->let_publisher->qos_profile;

		for(int i = 0; i < num_period_per_let; i++)
		{
			// Initialize the publisher to the intermediate topic, one for each period of its callback's LET
			CHECK_RCL_RET(_rclc_create_intermediate_topic(&intermediate_topic, output->handle.publisher->let_publisher->topic_name, i),
										(unsigned long) output);

			output->handle.publisher->let_publisher->let_publishers[i] = rcl_get_zero_initialized_publisher();
			CHECK_RCL_RET(rcl_publisher_init(&output->handle.publisher->let_publisher->let_publishers[i], 
				output->handle.publisher->let_publisher->node,
				output->handle.publisher->let_publisher->type_support,
				intermediate_topic,
				&option),
				(unsigned long) output);

			// Initialize the subscriber for the intermediate topic
			CHECK_RCL_RET(rclc_subscription_init(&output->subscriber_arr[i],
				output->handle.publisher->let_publisher->node,
				output->handle.publisher->let_publisher->type_support,
				intermediate_topic,
				output->handle.publisher->let_publisher->qos_profile),
				(unsigned long) output);
			
			output->data_consumed[i] = true;
		}

		allocator->deallocate(intermediate_topic, allocator->state);

		// Allocate memory to store subscriber data
		CHECK_RCL_RET(rclc_init_array(&output->data_arr, output->handle.publisher->let_publisher->message_size, num_period_per_let), 
										(unsigned long) output);

		// Initialize the intermediate publisher to the original topic
		CHECK_RCL_RET(rclc_publisher_init(&output->publisher, 
				handle.publisher->let_publisher->node, 
				handle.publisher->let_publisher->type_support,
				handle.publisher->let_publisher->topic_name,
				handle.publisher->let_publisher->qos_profile,
				RCLCPP_EXECUTOR),
				(unsigned long) output);

		// Disconnect the original publisher to the original topic
		CHECK_RCL_RET(rclc_publisher_fini(handle.publisher, handle.publisher->let_publisher->node), (unsigned long) output);
		break;
	default:
		break;
	}
	output->initialized = true;
	return RCL_RET_OK;
}

rcl_ret_t _rclc_output_handle_fini(
	rclc_let_output_t * output, 
	rcl_allocator_t * allocator)
{
	int num_period_per_let = output->callback_info->num_period_per_let;
	switch(output->handle.type)
	{
	case RCLC_PUBLISHER:	
		for(int i = 0; i < num_period_per_let; i++)
		{
			CHECK_RCL_RET(rcl_publisher_fini(&output->handle.publisher->let_publisher->let_publishers[i], 
										 output->handle.publisher->let_publisher->node),
										 (unsigned long) output);
			CHECK_RCL_RET(rcl_subscription_fini(&output->subscriber_arr[i], output->handle.publisher->let_publisher->node), 
										 (unsigned long) output);
		}
		CHECK_RCL_RET(rclc_fini_array(&output->data_arr), (unsigned long) output);
		// Initialize the intermediate publisher to the original topic
		CHECK_RCL_RET(rclc_publisher_fini(&output->publisher, output->handle.publisher->let_publisher->node), 
										 (unsigned long) output); 

		// Re-initialize the original publisher
		rcl_publisher_options_t option = rcl_publisher_get_default_options();
  	option.qos = *output->handle.publisher->let_publisher->qos_profile;
		CHECK_RCL_RET(rcl_publisher_init(&output->handle.publisher->rcl_publisher, 
				output->handle.publisher->let_publisher->node, 
				output->handle.publisher->let_publisher->type_support,
				output->handle.publisher->let_publisher->topic_name,
				&option),
				(unsigned long) output);	

		allocator->deallocate(output->subscriber_arr, allocator->state);
		output->subscriber_arr = NULL;
		allocator->deallocate(output->data_consumed, allocator->state);
		output->data_consumed = NULL;
		allocator->deallocate(output->handle.publisher->let_publisher->let_publishers, allocator->state);
		output->handle.publisher->let_publisher->let_publishers = NULL;
		break;
	default:
		break;
	}
	output->initialized = false;
	return RCL_RET_OK;
}

RCLC_PUBLIC
rcl_ret_t
rclc_let_output_node_init(
	rclc_let_output_node_t * let_output_node,
	const size_t max_number_of_let_handles,
	const size_t max_intermediate_handles,
	rcl_allocator_t * allocator)
{
	let_output_node->allocator = allocator;
	CHECK_RCL_RET(rclc_support_init(&let_output_node->support, 0, NULL, allocator), 
									(unsigned long) let_output_node);
	let_output_node->max_output_handles = max_number_of_let_handles;
	let_output_node->max_intermediate_handles = max_intermediate_handles;
	let_output_node->num_intermediate_handles = 0;
	let_output_node->index = 0;

	CHECK_RCL_RET(rclc_allocate(allocator, (void **) &let_output_node->output_arr, max_number_of_let_handles*sizeof(rclc_let_output_t)), 
									(unsigned long) let_output_node);
	for (size_t i = 0; i < max_number_of_let_handles; i++)
	{
		_rclc_output_handle_zero_init(&let_output_node->output_arr[i]);
	}
	pthread_mutex_init(&(let_output_node->mutex), NULL);
  return RCL_RET_OK;
}

RCLC_PUBLIC
rcl_ret_t
rclc_let_output_node_fini(rclc_let_output_node_t * let_output_node)
{
  for (size_t i = 0; (i < let_output_node->max_output_handles && let_output_node->output_arr[i].initialized); i++)
  {
  	CHECK_RCL_RET(_rclc_output_handle_fini(&let_output_node->output_arr[i], let_output_node->allocator),
  									(unsigned long) let_output_node);
  }
  let_output_node->allocator->deallocate(let_output_node->output_arr, let_output_node->allocator->state);
  let_output_node->output_arr = NULL;
  let_output_node->index = 0;
  let_output_node->max_output_handles = 0;
  pthread_mutex_destroy(&(let_output_node->mutex));
  return RCL_RET_OK;
}

RCLC_PUBLIC
rcl_ret_t
rclc_let_output_node_add_publisher(
	rclc_let_output_node_t * let_output_node,
	rclc_executor_handle_t * handles,
	size_t max_handles,
	rclc_publisher_t * publisher,
	const int max_number_per_callback,
	void * handle_ptr,
	rclc_executor_handle_type_t type)
{
	RCL_CHECK_ARGUMENT_FOR_NULL(let_output_node, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(publisher, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(handle_ptr, RCL_RET_INVALID_ARGUMENT);
  const char* error_message = NULL;
  bool handle_found = false;
  printf("Adding publisher\n");
  switch(type)
  {
    case RCLC_SUBSCRIPTION:
    case RCLC_SUBSCRIPTION_WITH_CONTEXT:
      error_message = "Subscription has not been added to the executor";
    case RCLC_TIMER:
      if (error_message == NULL) {
          error_message = "Timer has not been added to the executor";
      }
      for (size_t i = 0; (i < max_handles && handles[i].initialized); i++)
      {
        if (handles[i].type == type && 
          (handles[i].timer == handle_ptr || handles[i].subscription == handle_ptr))
        {
        	let_output_node->output_arr[let_output_node->index].callback_info = handles[i].callback_info;
        	publisher->let_publisher->num_period_per_let = handles[i].callback_info->num_period_per_let;
        	let_output_node->num_intermediate_handles += handles[i].callback_info->num_period_per_let;
        	let_output_node->num_intermediate_handles++;
        	handle_found = true;
        	break;
        }
      }
      if (!handle_found)
      {
      	printf(error_message);
	      RCL_SET_ERROR_MSG(error_message);
	      return RCL_RET_ERROR;        	
      }
    	break;
    default:
      return RCL_RET_ERROR;
  }
  rclc_executor_let_handle_t let_handle;
  let_handle.type = RCLC_PUBLISHER;
  let_handle.publisher = publisher;
  let_output_node->output_arr[let_output_node->index].max_msg_per_period = max_number_per_callback;
  printf("num_intermediate_handles %ld\n", let_output_node->num_intermediate_handles);
  CHECK_RCL_RET(_rclc_output_handle_init(&let_output_node->output_arr[let_output_node->index], 
								let_output_node->allocator, let_handle), (unsigned long) let_output_node);
  let_output_node->index++;
  return RCL_RET_OK;
}

void _rclc_let_deadline_timer_callback(rcl_timer_t * timer, void * context)
{
	rclc_let_timer_callback_context_t * context_obj = context;
	rclc_let_output_t * output = context_obj->output;
	uint64_t period_ns = context_obj->period_ns;
	int64_t old_period_ns;
	rcutils_time_point_value_t now;
	if (output->first_run)
	{
		VOID_CHECK_RCL_RET(rcl_timer_exchange_period(timer, (int64_t) period_ns, &old_period_ns), (unsigned long) timer);
		output->first_run = false;
	}
	output->period_index++;
	output->timer_triggered = true;
	VOID_CHECK_RCL_RET(rcutils_steady_time_now(&now), (unsigned long) timer);
	printf("Writer %lu %lu %ld\n", (unsigned long) output->handle.publisher, output->period_index, now);
	return;
}

void _rclc_let_data_subscriber_callback(const void * msgin, void * context, bool data_available)
{
	rclc_let_data_subscriber_callback_context_t * context_obj = context;
	rclc_let_output_t * output = context_obj->output;
	if (!output->timer_triggered)
		return;

	int subscriber_period_id = context_obj->subscriber_period_id;
	pthread_mutex_t * mutex = context_obj->mutex;
	int period_id = (output->period_index - 1)%output->callback_info->num_period_per_let; // Minus 1 because output->period_index is incremented before output send
	rcutils_time_point_value_t now;
	rclc_callback_state_t state;
	VOID_CHECK_RCL_RET(rclc_get_array(&output->callback_info->state, &state, subscriber_period_id), 
											(unsigned long) output->handle.publisher);
	printf("Sub output callback index %d\n", subscriber_period_id);

	if (subscriber_period_id == period_id)
	{
		printf("Sub output callback triggered\n");
		if (state == RUNNING || state == RELEASED)
		{
			pthread_mutex_lock(mutex);
			rclc_overrun_status_t overrun_status = (state == RUNNING) ? OVERRUN : HANDLING_ERROR;
			output->callback_info->overrun_status = overrun_status;
			// Set all other active instances of this callback to inactive
			for(int i = 0; i < output->callback_info->num_period_per_let; i++)
			{
	      if(i == period_id && overrun_status == OVERRUN)
	        continue;
				state = INACTIVE;
        VOID_CHECK_RCL_RET(rclc_set_array(&(output->callback_info->state), &state, i), 
        										(unsigned long) output->handle.publisher);
			}
			
			pthread_mutex_unlock(mutex);
			printf("deadline passed %lu\n", (unsigned long) output->handle.publisher);
			return;			
		}

		if (!data_available && output->data_consumed[subscriber_period_id])
		{
			// Finish executing without new data
			printf("No LET output\n");
			return;
		}

		VOID_CHECK_RCL_RET(rcl_publish(&output->publisher.rcl_publisher, msgin, NULL), 
												(unsigned long) output->handle.publisher);
		output->data_consumed[subscriber_period_id] = true;
		rcutils_steady_time_now(&now);
		int64_t * temp = (int64_t *) msgin;
		printf("Output %lu %ld %ld\n", (unsigned long) output->handle.publisher, temp[1], now);
	}
	else
	{
		printf("Sub output callback not triggered\n");
		if (data_available || (output->data_consumed[subscriber_period_id] == false))
			{
				// have new data but not for this deadline
				output->data_consumed[subscriber_period_id] = false;
				printf("state %d\n", (int) state);
				// if the overrun callback finish executing, publish the data
				if (output->callback_info->overrun_status == HANDLING_ERROR)
				{
					VOID_CHECK_RCL_RET(rcl_publish(&output->publisher.rcl_publisher, msgin, NULL), 
															(unsigned long) output);
					output->data_consumed[subscriber_period_id] = true;
					rcutils_steady_time_now(&now);
					int64_t * temp = (int64_t *) msgin;
					printf("Output %lu %ld %ld\n", (unsigned long) output->handle.publisher, temp[1], now);			
				}
			}		
	} 
	return;
}

void _rclc_check_overrun_callback_finishes(rclc_let_output_node_t * let_output_node)
{
	rclc_let_output_t * output;
	rclc_callback_state_t state;
	int period_id;
	for (size_t i = 0; i < let_output_node->max_output_handles && let_output_node->output_arr[i].initialized; i++)
	{
	  output = &let_output_node->output_arr[i];
	  if (!output->timer_triggered)
	    continue;

	  period_id = (output->period_index - 1)%output->callback_info->num_period_per_let; 
	  VOID_CHECK_RCL_RET(rclc_get_array(&output->callback_info->state, &state, period_id), 
	  										(unsigned long) let_output_node);

	  if (output->callback_info->overrun_status == HANDLING_ERROR)
	  {
	  	pthread_mutex_lock(&let_output_node->mutex);
	    output->callback_info->overrun_status = NO_ERROR;
	    pthread_mutex_unlock(&let_output_node->mutex);
	  }

	  output->timer_triggered = false;
	}
}

RCLC_PUBLIC
rcl_ret_t
rclc_executor_let_run(rclc_let_output_node_t * let_output_node, bool * exit_flag, uint64_t period_ns)
{
	rclc_executor_t output_executor = rclc_executor_get_zero_initialized_executor();
	CHECK_RCL_RET((rclc_executor_init(&output_executor, &let_output_node->support.context, 
						let_output_node->num_intermediate_handles, let_output_node->allocator)), (unsigned long) &output_executor);
	CHECK_RCL_RET(rclc_executor_set_timeout(&output_executor, RCL_MS_TO_NS(5000)),
									(unsigned long) &output_executor);
	CHECK_RCL_RET(rclc_executor_set_semantics(&output_executor, LET_OUTPUT),
									(unsigned long) &output_executor);
	CHECK_RCL_RET(rclc_executor_set_trigger(&output_executor, _rclc_executor_trigger_any_let_timer, NULL),
									(unsigned long) &output_executor);
	rclc_let_timer_callback_context_t * timer_context;
	rclc_let_data_subscriber_callback_context_t * sub_context;
	CHECK_RCL_RET(rclc_allocate(let_output_node->allocator, (void **) &timer_context, 
									sizeof(rclc_let_timer_callback_context_t)*let_output_node->index),
									(unsigned long) &output_executor);

	CHECK_RCL_RET(rclc_allocate(let_output_node->allocator, (void **) &sub_context, 
									sizeof(rclc_let_data_subscriber_callback_context_t)*let_output_node->num_intermediate_handles),
									(unsigned long) &output_executor);

	int intermediate_handles_count = 0;
  for (size_t i = 0; (i < let_output_node->max_output_handles && let_output_node->output_arr[i].initialized); i++)
  {
  	let_output_node->output_arr[i].timer = rcl_get_zero_initialized_timer();
  	CHECK_RCL_RET(rclc_timer_init_default(&let_output_node->output_arr[i].timer, 
  		&let_output_node->support, 
  		let_output_node->output_arr[i].callback_info->callback_let_ns, 
  		NULL), (unsigned long) &output_executor);

  	timer_context[i].output = &let_output_node->output_arr[i];
  	timer_context[i].period_ns = period_ns;

  	CHECK_RCL_RET(rclc_executor_add_timer_with_context(&output_executor,
  		&let_output_node->output_arr[i].timer, &_rclc_let_deadline_timer_callback,
  		&timer_context[i], 0), (unsigned long) &output_executor);

  	for (int j = 0; j < let_output_node->output_arr[i].callback_info->num_period_per_let; j++)
  	{
  		printf("Number of period per let %d\n", let_output_node->output_arr[i].callback_info->num_period_per_let);
  		void * msg;
  		rclc_array_element_status_t status;
  		CHECK_RCL_RET(rclc_get_pointer_array(&let_output_node->output_arr[i].data_arr, j, &msg, &status),
  										(unsigned long) &output_executor);

  		sub_context[intermediate_handles_count].output = &let_output_node->output_arr[i];
  		sub_context[intermediate_handles_count].subscriber_period_id = j;
  		sub_context[intermediate_handles_count].mutex = &let_output_node->mutex;
  		
	  	CHECK_RCL_RET(rclc_executor_add_subscription_for_let_data(&output_executor, 
	  		&let_output_node->output_arr[i].subscriber_arr[j],
	  		msg, &_rclc_let_data_subscriber_callback, 
	  		&sub_context[intermediate_handles_count], ALWAYS),
	  		(unsigned long) &output_executor);		
	  	
	  	intermediate_handles_count++;
  	}
  }
  rcl_ret_t ret = RCL_RET_OK;
  printf("OutEx start %lu\n", (unsigned long) &output_executor);
  while(!(*exit_flag))
  {
  	ret = rclc_executor_spin_some(&output_executor, output_executor.timeout_ns);
    if (!((ret == RCL_RET_OK) || (ret == RCL_RET_TIMEOUT))) {
      printf("LET output Executor spin failed\n");
      return RCL_RET_ERROR;
    }
    _rclc_check_overrun_callback_finishes(let_output_node);
  }
  printf("OutEx stop %lu\n", (unsigned long) &output_executor);
  let_output_node->allocator->deallocate(timer_context, let_output_node->allocator->state);
  let_output_node->allocator->deallocate(sub_context, let_output_node->allocator->state);
  return ret;
}

