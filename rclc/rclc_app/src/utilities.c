#include "utilities.h"

volatile bool exit_flag = false;
void *rclc_executor_spin_wrapper(void *arg)
{
  rclc_executor_t * executor = (rclc_executor_t *) arg;
  rcl_ret_t ret = RCL_RET_OK;
  while (!exit_flag) {
    ret = rclc_executor_spin_some(executor, executor->timeout_ns);
    if (!((ret == RCL_RET_OK) || (ret == RCL_RET_TIMEOUT))) {
      printf("Executor spin failed\n");
    }
  }
  printf("Executor stops\n");
  return 0;
}

void *rclc_executor_spin_period_wrapper(void *arg)
{
  struct arg_spin_period *arguments = arg;
  rclc_executor_t * executor = arguments->executor;
  const uint64_t period = arguments->period;
  rcl_ret_t ret = RCL_RET_OK;
  while (!exit_flag) {
    ret = rclc_executor_spin_one_period(executor, period);
    if (!((ret == RCL_RET_OK) || (ret == RCL_RET_TIMEOUT))) {
      printf("Executor spin failed\n");
    }
  }
  printf("Executor stops\n");
  return 0;
}

void thread_create(pthread_t *thread_id, int policy, int priority, int cpu_id, void *(*function)(void *), void * arg)
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

rcl_time_point_value_t rclc_now(rclc_support_t * support)
{
	rcl_time_point_value_t now = 0;
	RCSOFTCHECK(rcl_clock_get_now(&support->clock, &now));
  return now;
}

int sleep_ms(int msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    res = nanosleep(&ts, NULL);

    return res;
}

int get_thread_time(pthread_t thread_id)
{
  clockid_t id;
  pthread_getcpuclockid(thread_id, &id);
  struct timespec spec;
  clock_gettime(id, &spec);
  return spec.tv_sec*1000 + spec.tv_nsec/1000000;
}

int get_current_thread_time()
{
  return get_thread_time(pthread_self());
}

void busy_wait(int duration)
{
  if (duration > 0)
  {
    int end_time = get_current_thread_time() + duration;
    int x = 0;
    bool do_again = true;
    while (do_again)
    {
      x++;
      do_again = (get_current_thread_time() < end_time);
    }
  }
}

void busy_wait_random(int min_time, int max_time)
{
	int duration = (rand() % (max_time - min_time + 1)) + min_time;
  busy_wait(duration);
}

void print_timestamp(int dim0, int dim1, rcl_time_point_value_t timestamp[dim0][dim1])
{
	  char line[20000] = "";
	  uint8_t end_of_file = 0;
    for (int i = 0; i < dim1; i++)
    {
        sprintf(line, "%d ", i);
        for (int j = 0; j < dim0; j++)
        {
            if (j == 0 && timestamp[j][i] == 0)
            {
                int sum = 0;
                for (int k = 1; k < dim0; k++)
                    sum += timestamp[k][i];
                if(sum == 0)
                {
                    end_of_file = 1;
                    break;
                }
            }
            char temp[32];
            sprintf(temp, "%ld ", timestamp[j][i]);
            strcat(line, temp);
        }
        strcat(line, "\n");
        if (end_of_file)
        	break;
        printf("%s", line);
    }  
}

