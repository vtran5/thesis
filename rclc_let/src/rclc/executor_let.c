#include "rclc/executor_let.h"
#include <rcutils/time.h>

RCLC_PUBLIC
rcl_ret_t
rclc_executor_let_init(rclc_executor_let_t * executor_let)
{
  return RCL_RET_OK;
}

RCLC_PUBLIC
rcl_ret_t
rclc_executor_let_init(rclc_executor_let_t * executor_let)
{
  return RCL_RET_OK;
}

void _rclc_let_deadline_timer_callback(rclc_let_timer_t * let_timer, rclc_executor_let_t * let_executor)
{
	int64_t old_period;
	let_interfaces__msg__Timer_Msg msg;
	if (let_timer->first_run)
	{
		rcl_timer_exchange_period(let_timer->timer, let_executor->period, old_period);
	}
	msg.callback_id = let_timer->callback_info->callback_id;
	msg.index = let_timer->index;
	let_timer.index++;
	rcl_publish(&let_executor->let_deadline_node.publisher, &msg, NULL);
	return;
}

void _rclc_let_deadline_subscriber_callback(const void * msgin, rclc_executor_let_t * let_executor)
{
	const let_interfaces__msg__Timer_Msg * msg = (const let_interfaces__msg__Timer_Msg *) msgin;
	if (msgin == NULL)
	{
		printf("LET deadline timer: msg NULL\n");
		return;
	}
	let_executor->output_index = msg->index;
	let_executor->output_callback_id = msg->callback_id;
	return;
}

void _rclc_let_data_subscriber_callback(const void * msgin, 
	rclc_executor_let_t * let_executor, 
	rclc_let_data_channel_t * channel,
	bool * data_consumed)
{
	if(channel->callback_info->callback_id == let_executor->output_callback_id &&
		channel->period_id == let_executor->output_index%channel->callback_info->num_period_per_let)
	{
		rcl_publish(&channel->publisher, msgin, NULL);
		*data_consumed = true;		
	}
	else
		*data_consumed = false;
	return;
}

