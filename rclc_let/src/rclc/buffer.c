#include "rclc/buffer.h"
#include <string.h>
#include <rcl/allocator.h>
// Circular Queue
rcl_ret_t rclc_init_circular_queue(rclc_circular_queue_t * queue, int elem_size, int capacity) {
    rcl_ret_t ret = RCL_RET_OK;
    queue->front = -1;
    queue->rear = -1;
    queue->elem_size = elem_size;
    queue->capacity = capacity;
    queue->buffer = NULL;
    rcl_allocator_t allocator = rcl_get_default_allocator();
    queue->buffer = allocator.allocate(capacity*elem_size, allocator.state);
    if (NULL == queue->buffer)
        return RCL_RET_BAD_ALLOC;
    return ret;
}

rcl_ret_t rclc_enqueue(rclc_circular_queue_t * queue, const void* item) {
    rcl_ret_t ret = RCL_RET_OK;
    if (rclc_is_full_circular_queue(queue)) {
        return ret;
    }

    if (queue->front == -1) {
        queue->front = queue->rear = 0;
    } else {
        queue->rear = (queue->rear + 1) % queue->capacity;
    }

    memcpy((char*)queue->buffer + queue->rear * queue->elem_size, item, queue->elem_size);
    return ret;
}

rcl_ret_t rclc_dequeue(rclc_circular_queue_t * queue, void* item) {
    rcl_ret_t ret = RCL_RET_OK;
    if (rclc_is_empty_circular_queue(queue)) {
        return ret;
    }

    memcpy(item, (char*)queue->buffer + queue->front * queue->elem_size, queue->elem_size);

    if (queue->front == queue->rear) {
        queue->front = queue->rear = -1;
    } else {
        queue->front = (queue->front + 1) % queue->capacity;
    }
    return ret;
}

rcl_ret_t rclc_fini_circular_queue(rclc_circular_queue_t * queue)
{
    rcl_ret_t ret = RCL_RET_OK;
    rcl_allocator_t allocator = rcl_get_default_allocator();
    allocator.deallocate(queue->buffer, allocator.state);
    queue->buffer = NULL;
    return ret;
}

bool rclc_is_empty_circular_queue(rclc_circular_queue_t * queue) {
    return queue->front == -1;
}

bool rclc_is_full_circular_queue(rclc_circular_queue_t * queue) {
    return (queue->rear + 1) % queue->capacity == queue->front;
}
