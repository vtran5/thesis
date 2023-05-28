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

rcl_ret_t rclc_enqueue_circular_queue(rclc_circular_queue_t * queue, const void* item) {
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

rcl_ret_t rclc_dequeue_circular_queue(rclc_circular_queue_t * queue, void* item) {
    rcl_ret_t ret = RCL_RET_OK;
    ret = rclc_peek(queue, item);

    if (queue->front == queue->rear) {
        queue->front = queue->rear = -1;
    } else {
        queue->front = (queue->front + 1) % queue->capacity;
    }
    return ret;
}

rcl_ret_t rclc_peek_circular_queue(rclc_circular_queue_t * queue, void* item) {
    rcl_ret_t ret = RCL_RET_OK;

    // Check if the queue is empty
    if (rclc_is_empty_circular_queue(queue)) {
        return ret;
    }

    // Copy the first item to the provided memory location without modifying the queue.
    memcpy(item, (char*)queue->buffer + queue->front * queue->elem_size, queue->elem_size);

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
    }

    queue->nodes[capacity - 1].in_use = false;
    queue->nodes[capacity - 1].next = NULL;
    queue->head = NULL;

    return ret;
}

rcl_ret_t rclc_enqueue_priority_queue(rclc_priority_queue_t* queue, const void* item, int priority) {
    rcl_ret_t ret = RCL_RET_OK;

    if (queue->size == queue->capacity) {
        return RCL_RET_OVERFLOW;
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
    node->item = item;
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
    map->entries.key = NULL;
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
