#include "custom_interfaces/msg/message.h"
#include "utilities.h"
#include <stdlib.h>
#include "my_node.h"
bool exiting_flag;
typedef struct test_sleep_time
{
  uint64_t period_ns;
  /* data */
} thread_arg_t;

void * _rclc_let_scheduling_input_wrapper(void * arg)
{
  thread_arg_t * arg_thread = (thread_arg_t *) arg;
  uint64_t period_ns = arg_thread->period_ns;
  rcutils_time_point_value_t end_time_point, now, start_time;
  rcutils_duration_value_t sleep_time;
  rcl_ret_t ret = rcutils_system_time_now(&start_time);

  while(!(exiting_flag))
  {
      ret = rcutils_steady_time_now(&now);
      printf("Period %ld\n", now);
      busy_wait_random(2, 4);
      ret = rcutils_system_time_now(&end_time_point);
      start_time += period_ns;
      sleep_time = start_time - end_time_point;
      if (sleep_time > 0) {
        rclc_sleep_ns(sleep_time);
      }
  }
  return 0;
}

int main(int argc, char const *argv[])
{
  pthread_t thread_id;
  exiting_flag = false;
  uint64_t period_ns = RCL_MS_TO_NS(10);
  thread_arg_t arg = {period_ns};
  thread_create(&thread_id, SCHED_FIFO, 99, 1, _rclc_let_scheduling_input_wrapper, &arg);
  sleep_ms(15000);
  exiting_flag = true;
  pthread_join(thread_id, NULL);
  return 0;
}