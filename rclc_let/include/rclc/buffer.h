#ifndef RCLC__BUFFER_H_
#define RCLC__BUFFER_H_

#include <stddef.h>
#include <stdbool.h>
#include <rcl/types.h>
#include <rcl/allocator.h>

#define CHECK_RCL_RET(FUNCTION_CALL, ADDITIONAL_ARG)    \
{                                                       \
    rcl_ret_t ret = FUNCTION_CALL;                      \
    if (ret != RCL_RET_OK) {                            \
        printf("Error occurred at %s:%d\n", __FILE__, __LINE__); \
        print_ret(ret, ADDITIONAL_ARG);                 \
        return ret;                                     \
    }                                                   \
}

#define VOID_CHECK_RCL_RET(FUNCTION_CALL, ADDITIONAL_ARG)    \
{                                                       \
    rcl_ret_t ret = FUNCTION_CALL;                      \
    if (ret != RCL_RET_OK) {                            \
        printf("Error occurred at %s:%d\n", __FILE__, __LINE__); \
        print_ret(ret, ADDITIONAL_ARG);                 \
        return;                                         \
    }                                                   \
}
rcl_ret_t
rclc_allocate(rcl_allocator_t * allocator, void ** ptr, size_t size);
void print_ret(rcl_ret_t ret, unsigned long ptr);
// Circular Queue
typedef enum {
    RCLC_QUEUE_EMPTY,
    RCLC_QUEUE_FULL,
    RCLC_QUEUE_NORMAL
} rclc_queue_state_t;

typedef struct rclc_circular_queue_s {
    /// Container to memory allocator for array handles
    const rcl_allocator_t * allocator;
    int front, rear;
    int elem_size;
    int capacity;
    void* buffer;
    rclc_queue_state_t state;
} rclc_circular_queue_t;

rcl_ret_t rclc_init_circular_queue(rclc_circular_queue_t * queue, int elem_size, int capacity, const rcl_allocator_t * allocator);
rcl_ret_t rclc_fini_circular_queue(rclc_circular_queue_t * queue);
rcl_ret_t rclc_enqueue_circular_queue(rclc_circular_queue_t * queue, const void* item);
rcl_ret_t rclc_dequeue_circular_queue(rclc_circular_queue_t * queue, void ** item);
rcl_ret_t rclc_peek_circular_queue(rclc_circular_queue_t * queue, void** item);
int rclc_num_elements_circular_queue(rclc_circular_queue_t * queue);
bool rclc_is_empty_circular_queue(rclc_circular_queue_t * queue);
bool rclc_is_full_circular_queue(rclc_circular_queue_t * queue);
rcl_ret_t rclc_flush_circular_queue(rclc_circular_queue_t * queue);
rcl_ret_t rclc_lock_queue(rclc_circular_queue_t * queue);
rcl_ret_t rclc_unlock_queue(rclc_circular_queue_t * queue);

// Priority Queue (fixed size - implemented with linked list - smallest priority goes first) 
typedef struct rclc_priority_node_s {
    void* item;
    int64_t priority;
    struct rclc_priority_node_s* next;
    bool in_use;
} rclc_priority_node_t;

typedef struct rclc_priority_queue_s {
    /// Container to memory allocator for array handles
    const rcl_allocator_t * allocator;
    rclc_priority_node_t* nodes;
    rclc_priority_node_t* head;
    int size;
    int capacity;
    int elem_size;
} rclc_priority_queue_t;

rcl_ret_t rclc_init_priority_queue(rclc_priority_queue_t* queue, int elem_size, int capacity, const rcl_allocator_t * allocator);
rcl_ret_t rclc_enqueue_priority_queue(rclc_priority_queue_t* queue, const void* item, int64_t priority);
rcl_ret_t rclc_dequeue_priority_queue(rclc_priority_queue_t* queue, void* item, int64_t * priority);
rcl_ret_t rclc_peek_priority_queue(rclc_priority_queue_t* queue, void* item, int64_t * priority);
bool rclc_is_empty_priority_queue(rclc_priority_queue_t* queue);
bool rclc_is_full_priority_queue(rclc_priority_queue_t* queue);
rcl_ret_t rclc_fini_priority_queue(rclc_priority_queue_t* queue);

// Multimap (fixed size - implemented with linked list and node pool)
typedef struct {
  void* key;
  void* values;
  int num_values;
} rclc_map_entry_t;

typedef struct {
  /// Container to memory allocator for array handles
  const rcl_allocator_t * allocator;
  rclc_map_entry_t* entries;
  int key_size;
  int value_size;
  int max_values_per_key;
  int num_keys;
  int capacity;
} rclc_map_t;

// Initialize a map with the size of key, size of each value, max number of values per key, and max number of keys
rcl_ret_t rclc_init_map(rclc_map_t *map, int key_size, int value_size, int max_values_per_key, int capacity, const rcl_allocator_t * allocator);

// Insert a pair of key-value into the map
rcl_ret_t rclc_insert_map(rclc_map_t *map, const void *key, const void *value);

// Get all the values of a key
rcl_ret_t rclc_get_values_map(rclc_map_t *map, const void *key, void *values, int *num_values);

// Get the entry of a key
rcl_ret_t rclc_get_entry_map(rclc_map_t *map, const void *key, rclc_map_entry_t *entry);

// Remove a key-value pair
rcl_ret_t rclc_remove_key_value_map(rclc_map_t *map, const void *key, const void *value);

// Remove a key and all its values
rcl_ret_t rclc_remove_key_map(rclc_map_t *map, const void *key);

// Clean up the map
rcl_ret_t rclc_fini_map(rclc_map_t *map);

// Check if the map is empty
bool rclc_is_empty_map(rclc_map_t *map);

// Check if the map has reached the maximum number of keys
bool rclc_is_full_key_map(rclc_map_t *map);

// Check if a key has reached the maximum number of values
bool rclc_is_full_value_map(rclc_map_t *map, const void *key);

// Check if the map contains the key
bool rclc_contains_key_map(rclc_map_t *map, const void *key);

// 2D Circular Queue
typedef struct rclc_2d_circular_queue_s {
    /// Container to memory allocator for array handles
    const rcl_allocator_t * allocator;
    int size;
    rclc_circular_queue_t* queues;
} rclc_2d_circular_queue_t;

rcl_ret_t rclc_init_2d_circular_queue(rclc_2d_circular_queue_t * queue2d, int _2d_capacity, int elem_size, int _1d_capacity, const rcl_allocator_t * allocator);
rcl_ret_t rclc_fini_2d_circular_queue(rclc_2d_circular_queue_t * queue2d);
rcl_ret_t rclc_enqueue_2d_circular_queue(rclc_2d_circular_queue_t * queue2d, const void* item, int queue_index);
rcl_ret_t rclc_dequeue_2d_circular_queue(rclc_2d_circular_queue_t * queue2d, void ** item, int queue_index);
rclc_circular_queue_t* rclc_get_queue(rclc_2d_circular_queue_t * queue2d, int queue_index);

// Array
typedef enum {
    AVAILABLE,
    UNAVAILABLE
} rclc_array_element_status_t;

typedef struct rclc_array_element_s {
    rclc_array_element_status_t status;
    void * item;
} rclc_array_element_t;

typedef struct rclc_array_s {
    /// Container to memory allocator for array handles
    const rcl_allocator_t * allocator;
    int elem_size;
    int capacity;
    rclc_array_element_t* buffer;
} rclc_array_t;

rcl_ret_t rclc_init_array(rclc_array_t * array, int elem_size, int capacity, const rcl_allocator_t * allocator);
rcl_ret_t rclc_fini_array(rclc_array_t * array);
rcl_ret_t rclc_set_array(rclc_array_t * array, const void* item, int index);
rcl_ret_t rclc_get_array(rclc_array_t * array, void * item, int index);
rcl_ret_t rclc_take_array(rclc_array_t * array, void * item, int index);
rcl_ret_t rclc_get_pointer_array(rclc_array_t * array, int index, void** item_ptr, rclc_array_element_status_t * status);
int rclc_capacity_array(rclc_array_t * array);

#endif // RCLC__BUFFER_H_
