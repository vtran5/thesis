#ifndef RCLC__BUFFER_H_
#define RCLC__BUFFER_H_

#include <stddef.h>
#include <stdbool.h>
#include <rcl/types.h>
// Circular Queue
typedef struct rclc_circular_queue_s {
    int front, rear;
    int elem_size;
    int capacity;
    void* buffer;
} rclc_circular_queue_t;

rcl_ret_t rclc_init_circular_queue(rclc_circular_queue_t * queue, int elem_size, int capacity);
rcl_ret_t rclc_fini_circular_queue(rclc_circular_queue_t * queue);
rcl_ret_t rclc_enqueue(rclc_circular_queue_t * queue, const void * item);
rcl_ret_t rclc_dequeue(rclc_circular_queue_t * queue, void * item);
bool rclc_is_empty_circular_queue(rclc_circular_queue_t * queue);
bool rclc_is_full_circular_queue(rclc_circular_queue_t * queue);

#endif // RCLC__BUFFER_H_
