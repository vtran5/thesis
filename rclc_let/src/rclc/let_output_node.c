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

// The callback needs to be added before initialize the output handle (requires num_period_per_let)
rcl_ret_t _rclc_output_handle_init(
	rclc_let_output_t * output, 
	rcl_allocator_t * allocator,
	rclc_executor_let_handle_t handle)
{
	rcl_ret_t ret = RCL_RET_OK;
	int num_period_per_let = output->callback_info->num_period_per_let;
	output->handle = handle;
	output->first_run = true;
	output->timer_triggered = false;
	output->data_consumed = true;
	output->period_index = 0;

	switch(output->handle.type)
	{
	case RCLC_PUBLISHER:
		output->subscriber_arr = allocator->allocate(num_period_per_let*sizeof(rcl_subscription_t), allocator->state);
		output->handle.publisher->let_publishers = allocator->allocate(num_period_per_let*sizeof(rcl_publisher_t), allocator->state);
		char * intermediate_topic = allocator->allocate(strlen(output->handle.publisher->topic_name)+ 13, allocator->state);
		if (intermediate_topic == NULL)
			return RCL_RET_BAD_ALLOC;

		for(int i = 0; i < num_period_per_let; i++)
		{
			// Initialize the publisher to the intermediate topic, one for each period of its callback's LET
			ret = _rclc_create_intermediate_topic(&intermediate_topic, 
				output->handle.publisher->topic_name, i);

			output->handle.publisher->let_publishers[i] = rcl_get_zero_initialized_publisher();
			ret = rcl_publisher_init(&output->handle.publisher->let_publishers[i], 
				output->handle.publisher->node,
				output->handle.publisher->type_support,
				intermediate_topic,
				&output->handle.publisher->option);

			// Initialize the subscriber for the intermediate topic
			ret = rclc_subscription_init(&output->subscriber_arr[i],
				output->handle.publisher->node,
				output->handle.publisher->type_support,
				intermediate_topic,
				&output->handle.publisher->option.qos);
		}

		allocator->deallocate(intermediate_topic, allocator->state);
		// Allocate memory to store subscriber data
		ret = rclc_init_array(&output->data_arr, output->callback_info->data.elem_size, num_period_per_let);
		// Initialize the intermediate publisher to the original topic

		ret = rclc_publisher_init(&output->publisher, 
				handle.publisher->node, 
				handle.publisher->type_support,
				handle.publisher->topic_name,
				&(handle.publisher->option.qos));	
		// Disconnect the original publisher to the original topic
		ret = rclc_publisher_fini(handle.publisher);
		output->handle.publisher->num_period_per_let = num_period_per_let;
		break;
	default:
		break;
	}
	output->initialized = true;
	return ret;
}

rcl_ret_t _rclc_output_handle_fini(
	rclc_let_output_t * output, 
	rcl_allocator_t * allocator)
{
	rcl_ret_t ret = RCL_RET_OK;
	int num_period_per_let = output->callback_info->num_period_per_let;
	switch(output->handle.type)
	{
	case RCLC_PUBLISHER:	
		for(int i = 0; i < num_period_per_let; i++)
		{
			ret = rcl_publisher_fini(&output->handle.publisher->let_publishers[i], output->handle.publisher->node);
			ret = rcl_subscription_fini(&output->subscriber_arr[i], output->handle.publisher->node);
		}
		ret = rclc_fini_array(&output->data_arr);
		// Initialize the intermediate publisher to the original topic
		ret = rclc_publisher_fini(&output->publisher); 
		allocator->deallocate(output->subscriber_arr, allocator->state);
		output->subscriber_arr = NULL;
		rclc_fini_array(&output->data_arr);
		allocator->deallocate(output->handle.publisher->let_publishers, allocator->state);
		output->handle.publisher->let_publishers = NULL;
		break;
	default:
		break;
	}
	output->initialized = false;
	return ret;
}

RCLC_PUBLIC
rcl_ret_t
rclc_let_output_node_init(
	rclc_let_output_node_t * let_output_node,
	const size_t max_number_of_let_handles,
	const size_t max_intermediate_handles,
	rcl_allocator_t * allocator)
{
	rcl_ret_t ret = RCL_RET_OK;
	let_output_node->allocator = allocator;
	ret = rclc_support_init(&let_output_node->support, 0, NULL, allocator);
	let_output_node->max_output_handles = max_number_of_let_handles;
	let_output_node->max_intermediate_handles = max_intermediate_handles;
	let_output_node->index = 0;
	let_output_node->output_arr = allocator->allocate(max_number_of_let_handles*sizeof(rclc_let_output_t), allocator->state);
  return ret;
}

RCLC_PUBLIC
rcl_ret_t
rclc_let_output_node_fini(rclc_let_output_node_t * let_output_node)
{
	rcl_ret_t ret = RCL_RET_OK;
  for (size_t i = 0; (i < let_output_node->max_output_handles && let_output_node->output_arr[i].initialized); i++)
  {
  	ret = _rclc_output_handle_fini(&let_output_node->output_arr[i], let_output_node->allocator);
  }
  let_output_node->allocator->deallocate(let_output_node->output_arr, let_output_node->allocator->state);
  let_output_node->output_arr = NULL;
  let_output_node->index = 0;
  let_output_node->max_output_handles = 0;
  return ret;
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
  rcl_ret_t ret = RCL_RET_OK;
  const char* error_message = NULL;
  bool handle_found = false;
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
        if(handles[i].type == type && 
          (handles[i].timer == handle_ptr || handles[i].subscription == handle_ptr))
        {
        	let_output_node->output_arr[let_output_node->index].callback_info = handles[i].callback_info;
        	handle_found = true;
        	break;
        }
      }
      if(!handle_found)
      {
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
  ret = _rclc_output_handle_init(&let_output_node->output_arr[let_output_node->index], 
								let_output_node->allocator, let_handle);
  let_output_node->index++;
  return ret;
}

void _rclc_let_deadline_timer_callback(rcl_timer_t * timer, void * context)
{
	rclc_let_timer_callback_context_t * context_obj = context;
	rclc_let_output_t * output = context_obj->output;
	uint64_t period_ns = context_obj->period_ns;
	int64_t old_period_ns;
	rcutils_time_point_value_t now;
	rcl_ret_t ret = RCL_RET_OK;
	if (output->first_run)
	{
		ret = rcl_timer_exchange_period(timer, (int64_t) period_ns, &old_period_ns);
		RCLC_UNUSED(ret);
		output->first_run = false;
	}
	ret = rcutils_steady_time_now(&now);
	printf("Writer %ld\n", now);
	output->period_index++;
	output->timer_triggered = true;
	return;
}

void _rclc_let_data_subscriber_callback(const void * msgin, void * context)
{
	_rclc_let_data_subscriber_callback_context_t * context_obj = context;
	rclc_let_output_t * output = context_obj->output;
	rcl_subscription_t * subscriber = context_obj->subscriber;
	int period_id = output->period_index%output->callback_info->num_period_per_let;
	rcutils_time_point_value_t now;
	if(output->timer_triggered && subscriber == &(output->subscriber_arr[period_id]))
	{
		rcl_ret_t ret = rcl_publish(&output->publisher.rcl_publisher, msgin, NULL);
		RCLC_UNUSED(ret);
		output->data_consumed = true;
		output->timer_triggered = false;
		ret = rcutils_steady_time_now(&now);
		printf("Publisher %lu %ld\n", (unsigned long) output->handle.publisher, now);
	}
	else
		output->data_consumed = false;
	return;
}

RCLC_PUBLIC
rcl_ret_t
rclc_executor_let_run(rclc_let_output_node_t * let_output_node, bool * exit_flag, uint64_t period_ns)
{
	rcl_ret_t ret = RCL_RET_OK;
	rclc_executor_t output_executor = rclc_executor_get_zero_initialized_executor();
	ret = rclc_executor_init(&output_executor, &let_output_node->support.context, 
						let_output_node->max_intermediate_handles, let_output_node->allocator);
	ret = rclc_executor_set_timeout(&output_executor, RCL_MS_TO_NS(1000));
	ret = rclc_executor_set_semantics(&output_executor, LET_OUTPUT);
	ret = rclc_executor_set_trigger(&output_executor, _rclc_executor_trigger_any_let_timer, NULL);

  for (size_t i = 0; (i < let_output_node->max_output_handles && let_output_node->output_arr[i].initialized); i++)
  {
  	let_output_node->output_arr[i].timer = rcl_get_zero_initialized_timer();
  	rclc_timer_init_default(&let_output_node->output_arr[i].timer, 
  		&let_output_node->support, 
  		let_output_node->output_arr[i].callback_info->callback_let_ns, 
  		NULL);
  	rclc_let_timer_callback_context_t timer_context =
  	{
  		.output = &let_output_node->output_arr[i],
  		.period_ns = period_ns
  	};
  	ret = rclc_executor_add_timer_with_context(&output_executor,
  		&let_output_node->output_arr[i].timer, &_rclc_let_deadline_timer_callback,
  		&timer_context, 0);

  	for (int j = 0; j < let_output_node->output_arr[i].callback_info->num_period_per_let; j++)
  	{
  		void * msg;
  		rclc_array_element_status_t status;
  		ret = rclc_get_pointer_array(&let_output_node->output_arr[i].data_arr, j, &msg, &status);
  		_rclc_let_data_subscriber_callback_context_t sub_context = 
  		{
  			.output = &let_output_node->output_arr[i],
  			.subscriber = &let_output_node->output_arr[i].subscriber_arr[j]
  		};

	  	rclc_executor_add_subscription_with_context(&output_executor, 
	  		&let_output_node->output_arr[i].subscriber_arr[j],
	  		msg, &_rclc_let_data_subscriber_callback, 
	  		&sub_context, 0, ON_NEW_DATA, 0);  		
  	}
  }

  while(!(*exit_flag))
  {
  	ret = rclc_executor_spin_some(&output_executor, output_executor.timeout_ns);
    if (!((ret == RCL_RET_OK) || (ret == RCL_RET_TIMEOUT))) {
      printf("LET output Executor spin failed\n");
      return RCL_RET_ERROR;
    }
  }
  return ret;
}
