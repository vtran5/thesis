#ifndef UTILITIES_H_
#define UTILITIES_H_ 
#define _GNU_SOURCE
#include <stdio.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>


#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d. Aborting.\n",__LINE__,(int)temp_rc); return 1;}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d. Continuing.\n",__LINE__,(int)temp_rc);}}
#define VOID_RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d. Aborting.\n",__LINE__,(int)temp_rc); return;}}

extern volatile bool exit_flag;

struct arg_spin_period {
  uint64_t period; //nanoseconds
  rclc_executor_t * executor;
  rclc_support_t * support;
};

struct arg_spin {
  rclc_executor_t * executor;
  rclc_support_t * support;
};

/**
 *  pthread_create-compatible wrapper function of rclc_executor_spin.
 *
 * \param[inout] arg is the passing argument from pthread_create, 
 * should be a pointer to an executor
 */
void *rclc_executor_spin_wrapper(void *arg);

/**
 *  pthread_create-compatible wrapper function of rclc_executor_spin_period.
 *
 * \param[inout] arg is the passing argument from pthread_create, 
 * should be a pointer to a struct arg_spin_period
 */
void *rclc_executor_spin_period_wrapper(void *arg);

#ifdef RCLC_LET
/**
 *  pthread_create-compatible wrapper function of rclc_executor_spin_period_with_exit.
 *
 * \param[inout] arg is the passing argument from pthread_create, 
 * should be a pointer to a struct arg_spin_period
 */
void *rclc_executor_spin_period_with_exit_wrapper(void *arg);
#endif

/**
 *  Creates and configures a thread
 *
 * \param[out] thread_id the pointer to the created thread id
 * \param[in] policy the scheduling policy of the thread (e.g SCHED_FIFO)
 * \param[in] priority the priority of the thread
 * \param[in] cpu_id the id of CPU core(s) that the thread will run on
 * \param[in] function the function that will be executed in the thread
 * \param[inout] arg the argument that will be passed into the thread function
 */
void thread_create(pthread_t *thread_id, int policy, int priority, int cpu_id, void *(*function)(void *), void * arg);

/**
 *  Gives the current value of the associated clock.
 *
 * \param[in] support a pointer to the support struct that contains 
 * the clock used for the application
 * \return time value in nanoseconds
 */
rcl_time_point_value_t rclc_now(rclc_support_t * support);

/**
 *  Sleeps for the requested number of miliseconds.
 *
 * \param[in] msec the sleep duration in miliseconds
 * \return 0 if successful
 * \return -1 if there is an error
 */
int sleep_ms(int msec);

/**
 *  Get the thread time of the thread that calls this function.
 *
 * \return time value in milliseconds
 */
int get_current_thread_time();

/**
 *  Get the thread time of the thread with thread_id.
 * 
 * \param[in] thread_id is the id of the thread
 * \return time value in milliseconds
 */
int get_thread_time(pthread_t thread_id);

/**
 *  Busy-wait for a random amount of time between a minimum and maximum value.
 *	Use this function to simulate a task varied execution time
 * 
 * \param[in] min_time is minimum time value (best case execution time) in miliseconds
 * \param[in] max_time is maximum time value (worst case execution time) in miliseconds
 * \param[in] support_ is a pointer to the support struct that contains 
 * the clock used for the application
 */
void busy_wait_random(int min_time, int max_time);

/**
 *  Busy-wait for a random amount of time between a minimum and maximum value.
 *  Inject an error with the busy-wait duration greater than max_time while error flag is true
 *  Use this function to simulate a task varied execution time
 * 
 * \param[in] min_time is minimum time value (best case execution time) in miliseconds
 * \param[in] max_time is maximum time value (worst case execution time) in miliseconds
 * \param[in] error is a flag to signal the error injection
 * \param[in] error_time is the busy-wait duration during the error
 * \param[in] support_ is a pointer to the support struct that contains 
 * the clock used for the application
 */
void busy_wait_random_error(int min_time, int max_time, bool error, int error_time);

/**
 *  Busy-wait for a specified amount of time.
 *  Use this function to simulate a task varied execution time
 * 
 * \param[in] duration is waiting duration in miliseconds
 * \param[in] support_ is a pointer to the support struct that contains 
 * the clock used for the application
 */
void busy_wait(int duration);

/**
 *  Print the timestamp 2D array
 *
 * \param[in] dim0 is the number of arrays in timestamp
 * \param[in] dim1 is the length of each array in timestamp
 * \param[in] timestamp is the 2D array that needs to be printed
 */
void print_timestamp(int dim0, int dim1, rcl_time_point_value_t *timestamp[dim0]);

/**
 *  Allocate memory and initialize timestamp array to 0
 *
 * \param[in] dim0 is the number of arrays in timestamp
 * \param[in] dim1 is the length of each array in timestamp
 * \param[in] timestamp is the pointer to the 2D array
 */
void init_timestamp(int dim0, int dim1, rcl_time_point_value_t *timestamp[dim0]);

/**
 *  Free memory of the 2D array
 *
 * \param[in] dim0 is the number of arrays in timestamp
 * \param[in] timestamp is the pointer to the 2D array
 */
void fini_timestamp(int dim0, rcl_time_point_value_t *timestamp[dim0]);

/**
 *  Find the minimum value in an array
 *
 * \param[in] size is the size of the array
 * \param[in] array is the array that contains element
 */
unsigned int min_period(int size, const unsigned int array[size]);

/**
 *  Parse the command line arguments
 *
 * \param[in] argc is the number of command line arguments
 * \param[in] argv is the array storing the arguments
 * \param[out] executor_period is the executor period
 * \param[out] timer_period is the timer_period
 * \param[out] experiment_duration is the experiment duration
 * \param[out] let indicates if the executor will use LET semantics
 */
void parse_user_arguments(int argc, char const *argv[], unsigned int *executor_period, unsigned int *timer_period, unsigned int *experiment_duration, bool *let);

#endif