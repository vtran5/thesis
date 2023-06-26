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
    queue->state = UNLOCKED;
    rcl_allocator_t allocator = rcl_get_default_allocator();
    queue->buffer = allocator.allocate(capacity*elem_size, allocator.state);
    if (NULL == queue->buffer)
        return RCL_RET_BAD_ALLOC;
    return ret;
}

rcl_ret_t rclc_enqueue_circular_queue(rclc_circular_queue_t * queue, const void* item, int index) {
    rcl_ret_t ret = RCL_RET_OK;

    if (queue->state == LOCKED || queue->state == POP_ONLY)
    {
        printf("Queue is locked\n");
        return RCL_RET_ERROR;
    }

    if (rclc_is_full_circular_queue(queue)) {
        printf("Queue is full \n");
        return RCL_RET_ERROR;
    }

    if (queue->front == -1) {
        queue->front = queue->rear = 0;
        if (index < 0)
            index = queue->rear;
    } else {
        int num_elems = (queue->rear - queue->front + queue->capacity) % queue->capacity + 1;
        if (index >= num_elems || index < 0) {
            queue->rear = (queue->rear + 1) % queue->capacity;
            index = queue->rear;
        } else {
            for (int i = queue->rear; i >= index; i--) {
                memcpy((char*)queue->buffer + ((i + 1) % queue->capacity) * queue->elem_size,
                       (char*)queue->buffer + i * queue->elem_size,
                       queue->elem_size);
            }
            queue->rear = (queue->rear + 1) % queue->capacity;
        }
    }

    memcpy((char*)queue->buffer + index * queue->elem_size, item, queue->elem_size);
    return ret;
}

rcl_ret_t rclc_dequeue_circular_queue(rclc_circular_queue_t * queue, void* item, int index) {
    rcl_ret_t ret = RCL_RET_OK;

    if (queue->state == LOCKED || queue->state == PUSH_ONLY)
    {
        printf("Queue is locked\n");
        return RCL_RET_ERROR;
    }

    if (rclc_is_empty_circular_queue(queue)) {
        return RCL_RET_ERROR;
    }

    int num_elems = (queue->rear - queue->front + queue->capacity) % queue->capacity + 1;
    if (index >= num_elems) {
        return RCL_RET_ERROR;
    }

    if (index < 0) {
        index = queue->front;
    }

    memcpy(item, (char*)queue->buffer + ((queue->front + index) % queue->capacity) * queue->elem_size, queue->elem_size);

    for (int i = index; i != queue->rear; i = (i + 1) % queue->capacity) {
        memcpy((char*)queue->buffer + i * queue->elem_size,
               (char*)queue->buffer + ((i + 1) % queue->capacity) * queue->elem_size,
               queue->elem_size);
    }
    if (queue->front == queue->rear) {
        queue->front = queue->rear = -1;
    } else {
        queue->rear = (queue->rear - 1 + queue->capacity) % queue->capacity;
    }
    return ret;
}

rcl_ret_t rclc_peek_circular_queue(rclc_circular_queue_t * queue, void* item, int index) {
    rcl_ret_t ret = RCL_RET_OK;

    if (rclc_is_empty_circular_queue(queue)) {
        return RCL_RET_ERROR;
    }

    int num_elems = (queue->rear - queue->front + queue->capacity) % queue->capacity + 1;
    if (index >= num_elems) {
        return RCL_RET_ERROR;
    }

    if (index < 0) {
        index = queue->front;
    }

    memcpy(item, (char*)queue->buffer + ((queue->front + index) % queue->capacity) * queue->elem_size, queue->elem_size);

    return ret;
}

rcl_ret_t rclc_get_circular_queue(rclc_circular_queue_t * queue, void** item, int index) {
    rcl_ret_t ret = RCL_RET_OK;

    if (queue->state == LOCKED)
    {
        printf("Queue is locked\n");
        return RCL_RET_ERROR;
    }

    if (rclc_is_empty_circular_queue(queue)) {
        return RCL_RET_ERROR;
    }

    int num_elems = (queue->rear - queue->front + queue->capacity) % queue->capacity + 1;
    if (index >= num_elems) {
        return RCL_RET_ERROR;
    }

    if (index < 0) {
        index = queue->front;
    }

    *item = (char*)queue->buffer + ((queue->front + index) % queue->capacity) * queue->elem_size;

    return ret;
}


int rclc_num_elements_circular_queue(rclc_circular_queue_t * queue)
{
    if (queue == NULL) {
        printf("Queue is NULL\n");
        return -1;
    }

    if (queue->front == -1) {
        return 0;
    }

    if (queue->rear >= queue->front) {
        return queue->rear - queue->front + 1;
    } else {
        return queue->capacity - queue->front + queue->rear + 1;
    }
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

rcl_ret_t rclc_flush_circular_queue(rclc_circular_queue_t * queue) {
    rcl_ret_t ret = RCL_RET_OK;
    if (queue == NULL) {
        printf("Queue is NULL\n");
        return RCL_RET_INVALID_ARGUMENT;
    }

    if (queue->state == LOCKED || queue->state == PUSH_ONLY)
    {
        printf("Queue is locked\n");
        return RCL_RET_ERROR;
    }

    queue->front = -1;
    queue->rear = -1;
    return ret;
}

rcl_ret_t rclc_set_state_queue(rclc_circular_queue_t * queue, rclc_queue_state_t state) {
    if (queue == NULL) {
        printf("Queue is NULL\n");
        return RCL_RET_INVALID_ARGUMENT;
    }
    queue->state = state;
    return RCL_RET_OK;
}

/************Priority Queue********************/
rcl_ret_t rclc_init_priority_queue(rclc_priority_queue_t* queue, int elem_size, int capacity) {
    rcl_ret_t ret = RCL_RET_OK;
    rcl_allocator_t allocator = rcl_get_default_allocator();

    queue->size = 0;
    queue->capacity = capacity;
    queue->elem_size = elem_size;
    queue->nodes = allocator.allocate(capacity * sizeof(rclc_priority_node_t), allocator.state);

    if (queue->nodes == NULL) {
        return RCL_RET_BAD_ALLOC;
    }

    // Initialize all nodes as unused and link them together
    for (int i = 0; i < capacity - 1; i++) {
        queue->nodes[i].in_use = false;
        queue->nodes[i].next = &queue->nodes[i+1];
        queue->nodes[i].item = allocator.allocate(elem_size, allocator.state);
        if (queue->nodes[i].item == NULL) {
            return RCL_RET_BAD_ALLOC;
        }
    }

    queue->nodes[capacity - 1].in_use = false;
    queue->nodes[capacity - 1].next = NULL;
    queue->nodes[capacity - 1].item = allocator.allocate(elem_size, allocator.state);
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
    if (queue->head == NULL || queue->head->priority < priority) {
        node->next = queue->head;
        queue->head = node;
    } else {
        rclc_priority_node_t* current = queue->head;
        while (current->next != NULL && current->next->priority >= priority) {
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
    rcl_allocator_t allocator = rcl_get_default_allocator();
    for (int i = 0; i < queue->capacity; i++) {
        allocator.deallocate(queue->nodes[i].item, allocator.state);
        queue->nodes[i].item = NULL;
    }
    allocator.deallocate(queue->nodes, allocator.state);
    queue->nodes = NULL;
    queue->head = NULL;
    queue->size = 0;

    return ret;
}

/********************* Multimap **************************/
rcl_ret_t rclc_init_map(rclc_map_t *map, int key_size, int value_size, int max_values_per_key, int capacity) {
  rcl_allocator_t allocator = rcl_get_default_allocator();
  map->entries = allocator.allocate(capacity*sizeof(rclc_map_entry_t), allocator.state);
  if (NULL == map->entries)
    return RCL_RET_BAD_ALLOC;

  for (int i = 0; i < capacity; i++) {
    map->entries[i].key = allocator.allocate(key_size, allocator.state);
    if (NULL == map->entries[i].key)
        return RCL_RET_BAD_ALLOC;
    map->entries[i].values = allocator.allocate(max_values_per_key*value_size, allocator.state);
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
  rcl_allocator_t allocator = rcl_get_default_allocator();
  for (int i = 0; i < map->capacity; i++) {
    allocator.deallocate(map->entries[i].key, allocator.state);
    allocator.deallocate(map->entries[i].values, allocator.state);
    map->entries[i].key = NULL;
    map->entries[i].values = NULL;
  }
  allocator.deallocate(map->entries, allocator.state);
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
rcl_ret_t rclc_init_2d_circular_queue(rclc_2d_circular_queue_t * queue2d, int _2d_capacity, int elem_size, int _1d_capacity) {
    queue2d->size = _2d_capacity;
    rcl_allocator_t allocator = rcl_get_default_allocator();
    queue2d->queues = allocator.allocate(_2d_capacity*sizeof(rclc_circular_queue_t), allocator.state);
    if (queue2d->queues == NULL)
        return RCL_RET_BAD_ALLOC;

    for (int i = 0; i < _2d_capacity; i++) {
        rclc_init_circular_queue(&(queue2d->queues[i]), elem_size, _1d_capacity);
    }

    return RCL_RET_OK;
}

rcl_ret_t rclc_fini_2d_circular_queue(rclc_2d_circular_queue_t * queue2d) {
    for (int i = 0; i < queue2d->size; ++i) {
        rclc_fini_circular_queue(&(queue2d->queues[i]));
    }

    rcl_allocator_t allocator = rcl_get_default_allocator();
    allocator.deallocate(queue2d->queues, allocator.state);
    queue2d->queues = NULL;
    queue2d->size = 0;

    return RCL_RET_OK;
}

rcl_ret_t rclc_enqueue_2d_circular_queue(rclc_2d_circular_queue_t * queue2d, const void* item, int queue_index, int item_index) {
    if (queue_index >= queue2d->size || queue_index < 0) {
        printf("Incorrect queue_index");
        return RCL_RET_INVALID_ARGUMENT;
    }

    return rclc_enqueue_circular_queue(&(queue2d->queues[queue_index]), item, item_index);
}

rcl_ret_t rclc_dequeue_2d_circular_queue(rclc_2d_circular_queue_t * queue2d, void * item, int queue_index) {
    if (queue_index >= queue2d->size || queue_index < 0) {
        return RCL_RET_INVALID_ARGUMENT;
    }

    return rclc_dequeue_circular_queue(&(queue2d->queues[queue_index]), item, -1);
}

rclc_circular_queue_t* rclc_get_queue(rclc_2d_circular_queue_t * queue2d, int queue_index) {
    if (queue_index >= queue2d->size || queue_index < 0) {
        return NULL; // return NULL or handle the error appropriately
    }

    return &(queue2d->queues[queue_index]);
}
/******************** Array *************************/
rcl_ret_t rclc_init_array(rclc_array_t * array, int elem_size, int capacity) {
    array->capacity = capacity;
    array->elem_size = elem_size;
    rcl_allocator_t allocator = rcl_get_default_allocator();
    array->buffer = allocator.allocate(capacity * sizeof(rclc_array_element_t), allocator.state);
    if (array->buffer == NULL) {
        return RCL_RET_BAD_ALLOC;
    }
    for(int i = 0; i < capacity; i++) {
        array->buffer[i].status = UNAVAILABLE;
        array->buffer[i].item = allocator.allocate(elem_size, allocator.state);
        if (array->buffer[i].item == NULL) {
            return RCL_RET_BAD_ALLOC;
        }
    }
    return RCL_RET_OK;
}

rcl_ret_t rclc_fini_array(rclc_array_t * array) {
    rcl_allocator_t allocator = rcl_get_default_allocator();
    for(int i = 0; i < array->capacity; i++) {
        allocator.deallocate(array->buffer[i].item, allocator.state);
        array->buffer[i].item = NULL;
    }
    allocator.deallocate(array->buffer, allocator.state);
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
    if(index < 0 || index >= array->capacity || array->buffer[index].status == UNAVAILABLE) {
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