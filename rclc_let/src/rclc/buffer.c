#include "rclc/buffer.h"
#include <string.h>

rcl_ret_t
rclc_allocate(rcl_allocator_t * allocator, void ** ptr, size_t size)
{
  *ptr = allocator->allocate(size, allocator->state);
  if (*ptr == NULL)
  {
    // try again
    *ptr = allocator->allocate(size, allocator->state);
    if (*ptr == NULL)
      return RCL_RET_BAD_ALLOC;
  }
  return RCL_RET_OK;
}

void print_ret(rcl_ret_t ret, unsigned long ptr)
{
  switch(ret)
  {
  case RCL_RET_OK:
    printf("RCL_RET_OK %ld\n", (long)ptr);
    break;
  case RCL_RET_ERROR:
    printf("RCL_RET_ERROR %ld\n", (long)ptr);
    break;
  case RCL_RET_TIMEOUT:
    printf("RCL_RET_TIMEOUT %ld\n", (long)ptr);
    break;
  case RCL_RET_BAD_ALLOC:
    printf("RCL_RET_BAD_ALLOC %ld\n", (long)ptr);
    break;
  case RCL_RET_INVALID_ARGUMENT:
    printf("RCL_RET_INVALID_ARGUMENT %ld\n", (long)ptr);
    break;
  case RCL_RET_UNSUPPORTED:
    printf("RCL_RET_UNSUPPORTED %ld\n", (long)ptr);
    break;
  case RCL_RET_ALREADY_INIT:
    printf("RCL_RET_ALREADY_INIT %ld\n", (long)ptr);
    break;
  case RCL_RET_NOT_INIT:
    printf("RCL_RET_NOT_INIT %ld\n", (long)ptr);
    break;
  case RCL_RET_MISMATCHED_RMW_ID:
    printf("RCL_RET_MISMATCHED_RMW_ID %ld\n", (long)ptr);
    break;
  case RCL_RET_TOPIC_NAME_INVALID:
    printf("RCL_RET_TOPIC_NAME_INVALID %ld\n", (long)ptr);
    break;
  case RCL_RET_SERVICE_NAME_INVALID:
    printf("RCL_RET_SERVICE_NAME_INVALID %ld\n", (long)ptr);
    break;
  case RCL_RET_UNKNOWN_SUBSTITUTION:
    printf("RCL_RET_UNKNOWN_SUBSTITUTION %ld\n", (long)ptr);
    break;
  case RCL_RET_ALREADY_SHUTDOWN:
    printf("RCL_RET_ALREADY_SHUTDOWN %ld\n", (long)ptr);
    break;
  case RCL_RET_NODE_INVALID:
    printf("RCL_RET_NODE_INVALID %ld\n", (long)ptr);
    break;
  case RCL_RET_NODE_INVALID_NAME:
    printf("RCL_RET_NODE_INVALID_NAME %ld\n", (long)ptr);
    break;
  case RCL_RET_NODE_INVALID_NAMESPACE:
    printf("RCL_RET_NODE_INVALID_NAMESPACE %ld\n", (long)ptr);
    break;
  case RCL_RET_NODE_NAME_NON_EXISTENT:
    printf("RCL_RET_NODE_NAME_NON_EXISTENT %ld\n", (long)ptr);
    break;
  case RCL_RET_PUBLISHER_INVALID:
    printf("RCL_RET_PUBLISHER_INVALID %ld\n", (long)ptr);
    break;
  case RCL_RET_SUBSCRIPTION_INVALID:
    printf("RCL_RET_SUBSCRIPTION_INVALID %ld\n", (long)ptr);
    break;
  case RCL_RET_SUBSCRIPTION_TAKE_FAILED:
    printf("RCL_RET_SUBSCRIPTION_TAKE_FAILED %ld\n", (long)ptr);
    break;
  case RCL_RET_CLIENT_INVALID:
    printf("RCL_RET_CLIENT_INVALID %ld\n", (long)ptr);
    break;
  case RCL_RET_CLIENT_TAKE_FAILED:
    printf("RCL_RET_CLIENT_TAKE_FAILED %ld\n", (long)ptr);
    break;
  case RCL_RET_SERVICE_INVALID:
    printf("RCL_RET_SERVICE_INVALID %ld\n", (long)ptr);
    break;
  case RCL_RET_SERVICE_TAKE_FAILED:
    printf("RCL_RET_SERVICE_TAKE_FAILED %ld\n", (long)ptr);
    break;
  case RCL_RET_TIMER_INVALID:
    printf("RCL_RET_TIMER_INVALID %ld\n", (long)ptr);
    break;
  case RCL_RET_TIMER_CANCELED:
    printf("RCL_RET_TIMER_CANCELED %ld\n", (long)ptr);
    break;
  case RCL_RET_WAIT_SET_INVALID:
    printf("RCL_RET_WAIT_SET_INVALID %ld\n", (long)ptr);
    break;
  case RCL_RET_WAIT_SET_EMPTY:
    printf("RCL_RET_WAIT_SET_EMPTY %ld\n", (long)ptr);
    break;
  case RCL_RET_WAIT_SET_FULL:
    printf("RCL_RET_WAIT_SET_FULL %ld\n", (long)ptr);
    break;
  case RCL_RET_INVALID_REMAP_RULE:
    printf("RCL_RET_INVALID_REMAP_RULE %ld\n", (long)ptr);
    break;
  case RCL_RET_WRONG_LEXEME:
    printf("RCL_RET_WRONG_LEXEME %ld\n", (long)ptr);
    break;
  case RCL_RET_INVALID_ROS_ARGS:
    printf("RCL_RET_INVALID_ROS_ARGS %ld\n", (long)ptr);
    break;
  case RCL_RET_INVALID_PARAM_RULE:
    printf("RCL_RET_INVALID_PARAM_RULE %ld\n", (long)ptr);
    break;
  case RCL_RET_INVALID_LOG_LEVEL_RULE:
    printf("RCL_RET_INVALID_LOG_LEVEL_RULE %ld\n", (long)ptr);
    break;
  case RCL_RET_EVENT_INVALID:
    printf("RCL_RET_EVENT_INVALID %ld\n", (long)ptr);
    break;
  case RCL_RET_EVENT_TAKE_FAILED:
    printf("RCL_RET_EVENT_TAKE_FAILED %ld\n", (long)ptr);
    break;
  case RCL_RET_LIFECYCLE_STATE_REGISTERED:
    printf("RCL_RET_LIFECYCLE_STATE_REGISTERED %ld\n", (long)ptr);
    break;
  case RCL_RET_LIFECYCLE_STATE_NOT_REGISTERED:
    printf("RCL_RET_LIFECYCLE_STATE_NOT_REGISTERED %ld\n", (long)ptr);
    break;
  default:
    printf("Unknown case %ld\n", (long)ptr);
  }
}

// Circular Queue
rcl_ret_t rclc_init_circular_queue(
  rclc_circular_queue_t* queue, 
  int elem_size, 
  int capacity,
  const rcl_allocator_t * allocator) {
  if (elem_size <= 0 || capacity <= 0 || !queue) 
    return RCL_RET_INVALID_ARGUMENT;

  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "allocator is NULL", return RCL_RET_INVALID_ARGUMENT);

  queue->allocator = allocator;
  queue->buffer = NULL;

  queue->buffer = queue->allocator->allocate(capacity*elem_size, queue->allocator->state);
  if (NULL == queue->buffer)
      return RCL_RET_BAD_ALLOC;

  queue->front = queue->rear = -1;
  queue->elem_size = elem_size;
  queue->capacity = capacity;
  queue->state = RCLC_QUEUE_EMPTY;

  return RCL_RET_OK;
}

rcl_ret_t rclc_fini_circular_queue(rclc_circular_queue_t* queue) {
  if (!queue) 
    return RCL_RET_INVALID_ARGUMENT;

  if (queue->buffer == NULL)
    return RCL_RET_OK;

  queue->allocator->deallocate(queue->buffer, queue->allocator->state);
  queue->front = queue->rear = -1;
  queue->elem_size = 0;
  queue->capacity = 0;
  queue->state = RCLC_QUEUE_EMPTY;

  return RCL_RET_OK;
}

rcl_ret_t rclc_enqueue_circular_queue(rclc_circular_queue_t* queue, const void* item) {
  if (!queue || !item)
  {
    printf("Invalid argument\n");
    return RCL_RET_INVALID_ARGUMENT;
  }
    

  if(rclc_is_full_circular_queue(queue))
  {
    printf("Queue is full\n");
    return RCL_RET_ERROR;
  }

  queue->rear = (queue->rear + 1) % queue->capacity;
  memcpy((char*)queue->buffer + queue->rear * queue->elem_size, item, queue->elem_size);

  if (queue->front == -1) queue->front = 0;

  queue->state = RCLC_QUEUE_NORMAL;

  return RCL_RET_OK;
}

rcl_ret_t rclc_dequeue_circular_queue(rclc_circular_queue_t* queue, void** item) {
  if (!queue || !item)
    return RCL_RET_INVALID_ARGUMENT;

  if(rclc_is_empty_circular_queue(queue))
    return RCL_RET_ERROR;
  
  *item = (char*)queue->buffer + queue->front * queue->elem_size;

  if (queue->front == queue->rear) {
      queue->front = queue->rear = -1;
      queue->state = RCLC_QUEUE_EMPTY;
  } else {
      queue->front = (queue->front + 1) % queue->capacity;
  }

  return RCL_RET_OK;
}

rcl_ret_t rclc_peek_circular_queue(rclc_circular_queue_t* queue, void** item) {
  if (!queue || !item)
    return RCL_RET_INVALID_ARGUMENT;

  if(rclc_is_empty_circular_queue(queue))
    return RCL_RET_ERROR;

  *item = (char*)queue->buffer + queue->front * queue->elem_size;

  return RCL_RET_OK;
}

int rclc_num_elements_circular_queue(rclc_circular_queue_t* queue) {
  if (!queue) return 0;
  if (queue->front == -1) return 0;
  if (queue->rear >= queue->front) return queue->rear - queue->front + 1;
  return queue->rear + queue->capacity - queue->front + 1;
}

bool rclc_is_empty_circular_queue(rclc_circular_queue_t* queue) {
  return (!queue || queue->state == RCLC_QUEUE_EMPTY);
}

bool rclc_is_full_circular_queue(rclc_circular_queue_t* queue) {
  if (!queue) return false;
  if (queue->state == RCLC_QUEUE_FULL) return true;
  return (queue->rear + 1) % queue->capacity == queue->front;
}

rcl_ret_t rclc_flush_circular_queue(rclc_circular_queue_t * queue) {
    rcl_ret_t ret = RCL_RET_OK;
    if (queue == NULL) {
        printf("Queue is NULL\n");
        return RCL_RET_INVALID_ARGUMENT;
    }

    queue->front = -1;
    queue->rear = -1;
    return ret;
}

/************Priority Queue********************/
rcl_ret_t rclc_init_priority_queue(
  rclc_priority_queue_t* queue, 
  int elem_size, 
  int capacity,
  const rcl_allocator_t * allocator) 
{
    if (queue == NULL | elem_size <= 0 | capacity <= 0)
    {
      printf("Invalid priority queue argument\n");
      return RCL_RET_INVALID_ARGUMENT;
    }

    RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "allocator is NULL", return RCL_RET_INVALID_ARGUMENT);

    rcl_ret_t ret = RCL_RET_OK;
    queue->allocator = allocator;
    queue->size = 0;
    queue->capacity = capacity;
    queue->elem_size = elem_size;
    queue->nodes = queue->allocator->allocate(capacity * sizeof(rclc_priority_node_t), queue->allocator->state);

    if (queue->nodes == NULL) {
        return RCL_RET_BAD_ALLOC;
    }

    // Initialize all nodes as unused and link them together
    for (int i = 0; i < capacity - 1; i++) {
        queue->nodes[i].in_use = false;
        queue->nodes[i].next = &queue->nodes[i+1];
        queue->nodes[i].item = queue->allocator->allocate(elem_size, queue->allocator->state);
        if (queue->nodes[i].item == NULL) {
            return RCL_RET_BAD_ALLOC;
        }
    }

    queue->nodes[capacity - 1].in_use = false;
    queue->nodes[capacity - 1].next = NULL;
    queue->nodes[capacity - 1].item = queue->allocator->allocate(elem_size, queue->allocator->state);
    if (queue->nodes[capacity - 1].item == NULL) {
        return RCL_RET_BAD_ALLOC;
    }    
    queue->head = NULL;

    return ret;
}

rcl_ret_t rclc_enqueue_priority_queue(rclc_priority_queue_t* queue, const void* item, int64_t priority) {
    rcl_ret_t ret = RCL_RET_OK;

    if (queue->size == queue->capacity) {
        return RCL_RET_ERROR;
    }

    // Find an unused node
    rclc_priority_node_t* node = NULL;
    for (int i = 0; i < queue->capacity; i++) {
        if (!queue->nodes[i].in_use) {
            node = &queue->nodes[i];
            break;
        }
    }

    // Initialize the node
    memcpy(node->item, item, queue->elem_size);
    node->priority = priority;
    node->in_use = true;

    // Insert the node into the queue
    if (queue->head == NULL || queue->head->priority > priority) {
        node->next = queue->head;
        queue->head = node;
    } else {
        rclc_priority_node_t* current = queue->head;
        while (current->next != NULL && current->next->priority <= priority) {
            current = current->next;
        }
        node->next = current->next;
        current->next = node;
    }

    queue->size++;

    return ret;
}

rcl_ret_t rclc_dequeue_priority_queue(rclc_priority_queue_t* queue, void* item, int64_t * priority) {
    rcl_ret_t ret = RCL_RET_OK;

    if (queue->size == 0) {
        return RCL_RET_ERROR;
    }

    // Copy the item and priority, and mark the node as unused
    rclc_priority_node_t* node = queue->head;
    memcpy(item, node->item, queue->elem_size);
    *priority = node->priority;
    node->in_use = false;

    // Remove the node from the queue
    queue->head = node->next;

    queue->size--;

    return ret;
}

rcl_ret_t rclc_peek_priority_queue(rclc_priority_queue_t* queue, void* item, int64_t * priority) {
    rcl_ret_t ret = RCL_RET_OK;

    if (queue->size == 0) {
        return RCL_RET_ERROR;
    }

    // Copy the item and priority of the highest-priority node
    memcpy(item, queue->head->item, queue->elem_size);
    *priority = queue->head->priority;

    return ret;
}

bool rclc_is_empty_priority_queue(rclc_priority_queue_t* queue) {
    return queue->size == 0;
}

bool rclc_is_full_priority_queue(rclc_priority_queue_t* queue) {
    return queue->size == queue->capacity;
}

rcl_ret_t rclc_fini_priority_queue(rclc_priority_queue_t* queue) {
    rcl_ret_t ret = RCL_RET_OK;

    for (int i = 0; i < queue->capacity; i++) {
        queue->allocator->deallocate(queue->nodes[i].item, queue->allocator->state);
        queue->nodes[i].item = NULL;
    }
    queue->allocator->deallocate(queue->nodes, queue->allocator->state);
    queue->nodes = NULL;
    queue->head = NULL;
    queue->size = 0;

    return ret;
}

/********************* Multimap **************************/
rcl_ret_t rclc_init_map(
  rclc_map_t *map, 
  int key_size, 
  int value_size, 
  int max_values_per_key, 
  int capacity,
  const rcl_allocator_t * allocator)
{
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "allocator is NULL", return RCL_RET_INVALID_ARGUMENT);
  map->allocator = allocator;
  map->entries = map->allocator->allocate(capacity*sizeof(rclc_map_entry_t), map->allocator->state);
  if (NULL == map->entries)
    return RCL_RET_BAD_ALLOC;

  for (int i = 0; i < capacity; i++) {
    map->entries[i].key = map->allocator->allocate(key_size, map->allocator->state);
    if (NULL == map->entries[i].key)
        return RCL_RET_BAD_ALLOC;
    map->entries[i].values = map->allocator->allocate(max_values_per_key*value_size, map->allocator->state);
    if (NULL == map->entries[i].values)
        return RCL_RET_BAD_ALLOC;
  }
  map->key_size = key_size;
  map->value_size = value_size;
  map->max_values_per_key = max_values_per_key;
  map->num_keys = 0;
  map->capacity = capacity;
  return RCL_RET_OK;
}

rcl_ret_t rclc_insert_map(rclc_map_t *map, const void *key, const void *value) {
  if (rclc_is_full_key_map(map)) {
    return RCL_RET_ERROR; // map is full
  }

  for (int i = 0; i < map->capacity; i++) {
    if (memcmp(map->entries[i].key, key, map->key_size) == 0) {
      if (map->entries[i].num_values == map->max_values_per_key) {
        return RCL_RET_OK; // max values for key reached
      }
      memcpy((char*)map->entries[i].values + map->value_size * map->entries[i].num_values, value, map->value_size);
      map->entries[i].num_values++;
      return RCL_RET_OK;
    }
  }

  for (int i = 0; i < map->capacity; i++) {
    if (map->entries[i].num_values == 0) {
      memcpy(map->entries[i].key, key, map->key_size);
      memcpy(map->entries[i].values, value, map->value_size);
      map->entries[i].num_values = 1;
      map->num_keys++;
      return RCL_RET_OK;
    }
  }

  return RCL_RET_ERROR; // something went wrong
}

rcl_ret_t rclc_get_values_map(rclc_map_t *map, const void *key, void *values, int *num_values) {
  for (int i = 0; i < map->capacity; i++) {
    if (memcmp(map->entries[i].key, key, map->key_size) == 0) {
      memcpy(values, map->entries[i].values, map->value_size * map->entries[i].num_values);
      *num_values = map->entries[i].num_values;
      return RCL_RET_OK;
    }
  }
  return RCL_RET_ERROR; // key not found
}

rcl_ret_t rclc_get_entry_map(rclc_map_t *map, const void *key, rclc_map_entry_t *entry) {
  for (int i = 0; i < map->capacity; i++) {
    if (memcmp(map->entries[i].key, key, map->key_size) == 0) {
      entry->key = map->entries[i].key;
      entry->values = map->entries[i].values;
      entry->num_values = map->entries[i].num_values;
      return RCL_RET_OK;
    }
  }
  return RCL_RET_ERROR; // key not found
}

rcl_ret_t rclc_remove_key_value_map(rclc_map_t *map, const void *key, const void *value) {
  for (int i = 0; i < map->capacity; i++) {
    if (memcmp(map->entries[i].key, key, map->key_size) == 0) {
      for (int j = 0; j < map->entries[i].num_values; j++) {
        if (memcmp((char*)map->entries[i].values + map->value_size * j, value, map->value_size) == 0) {
          if (map->entries[i].num_values == 1) {
            memset(map->entries[i].key, 0, map->key_size);
          } else {
            memcpy((char*)map->entries[i].values + map->value_size * j, (char*)map->entries[i].values + map->value_size * (map->entries[i].num_values - 1), map->value_size);
          }
          map->entries[i].num_values--;
          return RCL_RET_OK;
        }
      }
    }
  }
  return RCL_RET_ERROR; // key-value pair not found
}

rcl_ret_t rclc_remove_key_map(rclc_map_t *map, const void *key) {
  for (int i = 0; i < map->capacity; i++) {
    if (memcmp(map->entries[i].key, key, map->key_size) == 0) {
      memset(map->entries[i].key, 0, map->key_size);
      map->entries[i].num_values = 0;
      map->num_keys--;
      return RCL_RET_OK;
    }
  }
  return RCL_RET_ERROR; // key not found
}

rcl_ret_t rclc_fini_map(rclc_map_t *map) {

  for (int i = 0; i < map->capacity; i++) {
    map->allocator->deallocate(map->entries[i].key, map->allocator->state);
    map->allocator->deallocate(map->entries[i].values, map->allocator->state);
    map->entries[i].key = NULL;
    map->entries[i].values = NULL;
  }
  map->allocator->deallocate(map->entries, map->allocator->state);
  map->entries = NULL;
  return RCL_RET_OK;
}

bool rclc_is_empty_map(rclc_map_t *map) {
  return map->num_keys == 0;
}

bool rclc_is_full_key_map(rclc_map_t *map) {
  return map->num_keys == map->capacity;
}

bool rclc_is_full_value_map(rclc_map_t *map, const void *key) {
  for (int i = 0; i < map->capacity; i++) {
    if (memcmp(map->entries[i].key, key, map->key_size) == 0) {
      return map->entries[i].num_values == map->max_values_per_key;
    }
  }
  return false; // key not found
}

bool rclc_contains_key_map(rclc_map_t *map, const void *key) {
  for (int i = 0; i < map->capacity; i++) {
    if (memcmp(map->entries[i].key, key, map->key_size) == 0) {
      return true;
    }
  }
  return false;
}

/************2D Circular Queue********************/
rcl_ret_t rclc_init_2d_circular_queue(
  rclc_2d_circular_queue_t * queue2d, 
  int _2d_capacity,
  int elem_size, 
  int _1d_capacity,
  const rcl_allocator_t * allocator) 
  {
    if (queue2d == NULL | elem_size <= 0 | _2d_capacity <= 0 | _1d_capacity <= 0)
    {
      printf("Invalid priority queue argument\n");
      return RCL_RET_INVALID_ARGUMENT;
    }
    RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "allocator is NULL", return RCL_RET_INVALID_ARGUMENT);
    queue2d->size = _2d_capacity;
    queue2d->allocator = allocator;

    queue2d->queues = queue2d->allocator->allocate(_2d_capacity*sizeof(rclc_circular_queue_t), queue2d->allocator->state);
    if (queue2d->queues == NULL)
        return RCL_RET_BAD_ALLOC;
    rcl_ret_t rc;
    for (int i = 0; i < _2d_capacity; i++) {
        rc = rclc_init_circular_queue(&(queue2d->queues[i]), elem_size, _1d_capacity, queue2d->allocator);
        if (rc != RCL_RET_OK)
          return rc;
    }

    return RCL_RET_OK;
}

rcl_ret_t rclc_fini_2d_circular_queue(rclc_2d_circular_queue_t * queue2d) {
    for (int i = 0; i < queue2d->size; ++i) {
        rclc_fini_circular_queue(&(queue2d->queues[i]));
    }

    queue2d->allocator->deallocate(queue2d->queues, queue2d->allocator->state);
    queue2d->queues = NULL;
    queue2d->size = 0;

    return RCL_RET_OK;
}

rcl_ret_t rclc_enqueue_2d_circular_queue(rclc_2d_circular_queue_t * queue2d, const void* item, int queue_index) {
    if (queue_index >= queue2d->size || queue_index < 0) {
        printf("Incorrect queue_index");
        return RCL_RET_INVALID_ARGUMENT;
    }

    return rclc_enqueue_circular_queue(&(queue2d->queues[queue_index]), item);
}

rcl_ret_t rclc_dequeue_2d_circular_queue(rclc_2d_circular_queue_t * queue2d, void ** item, int queue_index) {
    if (queue_index >= queue2d->size || queue_index < 0) {
        return RCL_RET_INVALID_ARGUMENT;
    }

    return rclc_dequeue_circular_queue(&(queue2d->queues[queue_index]), item);
}

rclc_circular_queue_t* rclc_get_queue(rclc_2d_circular_queue_t * queue2d, int queue_index) {
    if (queue_index >= queue2d->size || queue_index < 0) {
        return NULL; // return NULL or handle the error appropriately
    }

    return &(queue2d->queues[queue_index]);
}
/******************** Array *************************/
rcl_ret_t rclc_init_array(
  rclc_array_t * array, 
  int elem_size, 
  int capacity,
  const rcl_allocator_t * allocator) 
{
    if (array == NULL | elem_size <= 0 | capacity <= 0)
    {
      printf("Invalid priority queue argument\n");
      return RCL_RET_INVALID_ARGUMENT;
    }
    RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "allocator is NULL", return RCL_RET_INVALID_ARGUMENT);
    array->capacity = capacity;
    array->elem_size = elem_size;
    array->allocator = allocator;

    array->buffer = array->allocator->allocate(capacity * sizeof(rclc_array_element_t), array->allocator->state);
    if (array->buffer == NULL) {
        return RCL_RET_BAD_ALLOC;
    }
    for(int i = 0; i < capacity; i++) {
        array->buffer[i].status = UNAVAILABLE;
        array->buffer[i].item = array->allocator->allocate(elem_size, array->allocator->state);
        if (array->buffer[i].item == NULL) {
            return RCL_RET_BAD_ALLOC;
        }
    }
    return RCL_RET_OK;
}

rcl_ret_t rclc_fini_array(rclc_array_t * array) {
    if (array->buffer == NULL || array->capacity == 0)
        return RCL_RET_OK;

    for(int i = 0; i < array->capacity; i++) {
        if (array->buffer[i].item == NULL)
            continue;
        array->allocator->deallocate(array->buffer[i].item, array->allocator->state);
        array->buffer[i].item = NULL;
    }
    array->allocator->deallocate(array->buffer, array->allocator->state);
    array->buffer = NULL;
    array->capacity = 0;
    return RCL_RET_OK;
}

rcl_ret_t rclc_set_array(rclc_array_t * array, const void* item, int index) {
    if(index < 0 || index >= array->capacity) {
        return RCL_RET_ERROR;
    }
    memcpy(array->buffer[index].item, item, array->elem_size);
    array->buffer[index].status = AVAILABLE;
    return RCL_RET_OK;
}

rcl_ret_t rclc_get_array(rclc_array_t * array, void * item, int index) {
    if(index < 0 || index >= array->capacity) {
        return RCL_RET_ERROR;
    }
    memcpy(item, array->buffer[index].item, array->elem_size);
    return RCL_RET_OK;
}

rcl_ret_t rclc_take_array(rclc_array_t * array, void * item, int index) {
    rcl_ret_t ret = rclc_get_array(array, item, index);
    if(ret == RCL_RET_OK) {
        array->buffer[index].status = UNAVAILABLE;
    }
    return ret;
}

rcl_ret_t rclc_get_pointer_array(rclc_array_t * array, int index, void** item_ptr, rclc_array_element_status_t * status)
{
    if(array == NULL || item_ptr == NULL || status == NULL || index < 0 || index >= array->capacity) {
        return RCL_RET_INVALID_ARGUMENT;
    }

    // Get the pointer to the data and status
    *item_ptr = array->buffer[index].item;
    status = &(array->buffer[index].status);

    return RCL_RET_OK;
}

int rclc_capacity_array(rclc_array_t * array) {
    return array->capacity;
}