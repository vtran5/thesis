#include <stdlib.h>
#include <rclc/rclc.h>
#include <stdio.h>
#include <stdint.h>
#include "utilities.h"
rclc_support_t support;
custom_interfaces__msg__Message pub_msg;
typedef struct 
{
  rclc_executor_handle_type_t type;
  void * handle_ptr;
} callback_t;

typedef struct timer_callback_context_t timer_callback_context_t;
typedef struct subscriber_callback_context_t subscriber_callback_context_t;

typedef struct
{
  rcl_node_t rcl_node;
  bool first_run;
  int64_t * count;
  int timer_num;
  int pub_num;
  int sub_num;
  rclc_publisher_t * publisher;
  callback_t * callback; // callbacks that calls the publishers
  rcl_subscription_t * subscriber;
  custom_interfaces__msg__Message * sub_msg;
  char** pub_topic_name;
  char** sub_topic_name;
  subscriber_callback_context_t * sub_context;
  timer_callback_context_t * timer_context;
  void (**subscriber_callback)(const void *, void *);
  rcl_timer_t * timer;
  void (**timer_callback)(rcl_timer_t *, void *);
} my_node_t;

struct subscriber_callback_context_t
{
    int * pub_index;
    int pub_num;
    int sub_index;
    my_node_t * node;
    int max_execution_time_ms;
    int min_execution_time_ms;
    bool error;
    int error_time;
};

struct timer_callback_context_t
{
    int * pub_index;
    int pub_num;
    int timer_index;
    my_node_t * node;
    int max_execution_time_ms;
    int min_execution_time_ms;
    bool error;
    int error_time;
};

my_node_t * create_node(
  int timer_num, 
  int pub_num, 
  int sub_num,
  const rcl_allocator_t * allocator)
{
  my_node_t * node = allocator->allocate(sizeof(my_node_t), allocator->state);
  if(node == NULL)
  {
    return NULL;
  }

  node->first_run = true;
  node->timer_num = timer_num;
  node->pub_num = pub_num;
  node->sub_num = sub_num;

  if(timer_num > 0)
  {
    node->count = allocator->allocate(sizeof(int64_t)*timer_num, allocator->state);
    if (node->count == NULL)
    {
      allocator->deallocate(node, allocator->state);
      return NULL;
    }
    for (int i = 0; i < timer_num; i++)
    {
      node->count[i] = 0;
    }    
  }

  if(pub_num > 0)
  {
    node->publisher = allocator->allocate(sizeof(rclc_publisher_t)*pub_num, allocator->state);
    if(node->publisher == NULL)
    {
      if(node->count != NULL)
        allocator->deallocate(node->count, allocator->state);
      allocator->deallocate(node, allocator->state);
      return NULL;
    }
    node->callback = allocator->allocate(sizeof(callback_t)*pub_num, allocator->state);
    if (node->callback == NULL)
      return NULL;
  }

  if(sub_num > 0)
  {
    node->subscriber = allocator->allocate(sizeof(rcl_subscription_t)*sub_num, allocator->state);
    if(node->subscriber == NULL)
    {
      if(node->publisher != NULL)
        allocator->deallocate(node->publisher, allocator->state);
      if(node->count != NULL)
        allocator->deallocate(node->count, allocator->state);
      allocator->deallocate(node, allocator->state);
      return NULL;
    }

    node->sub_msg = allocator->allocate(sizeof(custom_interfaces__msg__Message)*sub_num, allocator->state);
    if(node->sub_msg == NULL)
    {
      if(node->subscriber != NULL)
        allocator->deallocate(node->subscriber, allocator->state);
      if(node->publisher != NULL)
        allocator->deallocate(node->publisher, allocator->state);
      if(node->count != NULL)
        allocator->deallocate(node->count, allocator->state);
      allocator->deallocate(node, allocator->state);
      return NULL;
    } 

    node->subscriber_callback = allocator->allocate(sizeof(void (*)(const void *))*sub_num, allocator->state);
    if(node->subscriber_callback == NULL)
    {
      if(node->sub_msg != NULL)
        allocator->deallocate(node->sub_msg, allocator->state);
      if(node->subscriber != NULL)
        allocator->deallocate(node->subscriber, allocator->state);
      if(node->publisher != NULL)
        allocator->deallocate(node->publisher, allocator->state);
      if(node->count != NULL)
        allocator->deallocate(node->count, allocator->state);
      allocator->deallocate(node, allocator->state);
      return NULL;
    }    
  }

  if(timer_num > 0)
  {
    node->timer = allocator->allocate(sizeof(rcl_timer_t)*timer_num, allocator->state);
    if(node->timer == NULL)
    {
      if (node->subscriber_callback != NULL)
      {
        allocator->deallocate(node->subscriber_callback, allocator->state);
        allocator->deallocate(node->sub_msg, allocator->state);
        allocator->deallocate(node->subscriber, allocator->state);        
      }
      if(node->publisher != NULL)
        allocator->deallocate(node->publisher, allocator->state);
      if(node->count != NULL)
        allocator->deallocate(node->count, allocator->state);
      allocator->deallocate(node, allocator->state);
      return NULL;
    }  

    node->timer_callback = allocator->allocate(sizeof(void (*)(rcl_timer_t *, int64_t))*timer_num, allocator->state);
    if(node->timer_callback == NULL)
    {
      if(node->timer != NULL)
        allocator->deallocate(node->timer, allocator->state);  
      if (node->subscriber_callback != NULL)
      {
        allocator->deallocate(node->subscriber_callback, allocator->state);
        allocator->deallocate(node->sub_msg, allocator->state);
        allocator->deallocate(node->subscriber, allocator->state);        
      }
      if(node->publisher != NULL)
        allocator->deallocate(node->publisher, allocator->state);
      if(node->count != NULL)
        allocator->deallocate(node->count, allocator->state);
      allocator->deallocate(node, allocator->state);
      return NULL;
    }    
  }

  return node;
}

void init_node(
  my_node_t * node,
  rclc_support_t * support, 
  char * node_name)
{
  if((node == NULL) || (node_name == NULL))
    return;
  node->rcl_node = rcl_get_zero_initialized_node();
  VOID_RCCHECK(rclc_node_init_default(&node->rcl_node, node_name, "rclc_app", support));  
}

void init_node_publisher(
  my_node_t * node, 
  const rosidl_message_type_support_t * my_type_support, 
  char ** topic_name,
  rmw_qos_profile_t * profile,
  rclc_executor_semantics_t semantics,
  const rcl_allocator_t * allocator)
{
  RCLC_UNUSED(semantics);
  if((node == NULL) || (my_type_support == NULL) | (topic_name == NULL) | (profile == NULL))
    return;
  for (int i = 0; i < node->pub_num; i++)
  {
    VOID_RCCHECK(rclc_publisher_init(&node->publisher[i], 
      &node->rcl_node, my_type_support, topic_name[i], profile, semantics, allocator));
  }
}


void init_node_timer(
  my_node_t * node,
  rclc_support_t * support,
  const uint64_t * timeout_ns)
{
  if((node == NULL) || (support == NULL) | (timeout_ns == NULL))
    return;
  for (int i = 0; i < node->timer_num; i++)
  {
    node->timer[i] = rcl_get_zero_initialized_timer();
    VOID_RCCHECK(rclc_timer_init_default(&node->timer[i], support, timeout_ns[i], node->timer_callback[i]));
  }  
}

void init_node_subscriber(
  my_node_t * node,
  const rosidl_message_type_support_t * my_type_support,
  char ** topic_name,
  rmw_qos_profile_t * profile)
{
  if((node == NULL) || (my_type_support == NULL) | (topic_name == NULL) | (profile == NULL))
    return;
  for (int i = 0; i < node->sub_num; i++)
  {
    node->subscriber[i] = rcl_get_zero_initialized_subscription();
    VOID_RCCHECK(rclc_subscription_init(&node->subscriber[i], &node->rcl_node, my_type_support, topic_name[i], profile));
    custom_interfaces__msg__Message__init(&node->sub_msg[i]);
  }  
}

void print_id(my_node_t * node, 
  int * sub_count,
  int * pub_count,
  int * timer_count)
{
  int j;
  for (j = 0; j < node->timer_num; j++)
  {
    printf("TimerID Timer%d %lu\n", *timer_count, (unsigned long) &node->timer[j]);
    (*timer_count)++;
  }

  for (j = 0; j < node->pub_num; j++)
  {
    printf("PublisherID Publisher%d %lu\n", *pub_count, (unsigned long) &node->publisher[j]);
    (*pub_count)++;
  }

  for (j = 0; j < node->sub_num; j++)
  {
    printf("SubscriberID Subscriber%d %lu\n", *sub_count, (unsigned long) &node->subscriber[j]);
    (*sub_count)++;
  }
}

void destroy_node(my_node_t * node, const rcl_allocator_t * allocator)
{
  if(node == NULL)
    return;

  if(node->timer != NULL)
  {
    for (int i = 0; i < node->timer_num; i++)
    {
      VOID_RCCHECK(rcl_timer_fini(&node->timer[i]));
    }
    allocator->deallocate(node->timer, allocator->state);
  }

  if(node->publisher != NULL)
  {
    for (int i = 0; i < node->pub_num; i++)
    {
      VOID_RCCHECK(rclc_publisher_fini(&node->publisher[i], &node->rcl_node));
      //VOID_RCCHECK(rclc_publisher_let_fini(&node->publisher[i]));
    }    
    allocator->deallocate(node->publisher, allocator->state);
    allocator->deallocate(node->callback, allocator->state);
  }

  if(node->count != NULL)
    allocator->deallocate(node->count, allocator->state);

  if(node->subscriber != NULL)
  {
    for (int i = 0; i < node->sub_num; i++)
    {
      VOID_RCCHECK(rcl_subscription_fini(&node->subscriber[i], &node->rcl_node));
    }    
    allocator->deallocate(node->subscriber, allocator->state);    
  }

  if(node->sub_msg != NULL)
    allocator->deallocate(node->sub_msg, allocator->state);

  if(node->timer_callback != NULL)
    allocator->deallocate(node->timer_callback, allocator->state);

  if(node->subscriber_callback != NULL)
    allocator->deallocate(node->subscriber_callback, allocator->state);

  VOID_RCCHECK(rcl_node_fini(&node->rcl_node));
}

char** create_topic_name_array(size_t array_size, const rcl_allocator_t * allocator)
{
  if(array_size <= 0)
  {
    printf("Array size negative\n");
    return NULL;
  }

  char ** arr = allocator->allocate(array_size * sizeof(char *), allocator->state);
  if(arr == NULL)
  {
    printf("Fail to allocate memory to array\n");
    return NULL;
  }

  for(size_t i = 0; i < array_size; i++)
  {
    arr[i] = allocator->allocate(10*sizeof(char), allocator->state); // "topicXXYY" plus null terminator
    if (arr[i] == NULL) {
      printf("Fail to allocate memory to string\n");
      for (size_t j = 0; j < i; ++j) {
          allocator->deallocate(arr[j], allocator->state);
      }
      allocator->deallocate(arr, allocator->state);
      return NULL;
    }
  }
  return arr;
}

void destroy_topic_name_array(char** arr, size_t array_size, const rcl_allocator_t * allocator) {
    if (arr != NULL) {
        // Free each string
        for (size_t i = 0; i < array_size; ++i) {
            allocator->deallocate(arr[i], allocator->state);
        }
        
        // Then free the array itself
        allocator->deallocate(arr, allocator->state);
        arr = NULL;
    }
}

rcutils_time_point_value_t * create_time_array(size_t array_size, const rcl_allocator_t * allocator)
{
  if(array_size <= 0)
    return NULL;

  rcutils_time_point_value_t * arr = allocator->allocate(array_size * sizeof(rcutils_time_point_value_t), allocator->state);
  if(arr == NULL)
    printf("Fail to allocate memory for callback let array\n");
  return arr;
}

void destroy_time_array(rcutils_time_point_value_t * array, const rcl_allocator_t * allocator)
{
  if(array != NULL)
    allocator->deallocate(array, allocator->state);
  array = NULL;
}

void timer_callback(
  my_node_t * node, 
  char * stat, 
  int timer_index, 
  int pub_index, 
  int min_run_time_ms,
  int max_run_time_ms,
  rclc_executor_semantics_t pub_semantics)
{
  char temp[1000] = "";
  rcl_time_point_value_t now = rclc_now(&support);
  pub_msg.frame_id = node->count[timer_index]++;
  pub_msg.stamp = now;
  sprintf(temp, "Timer %lu %ld %ld\n", (unsigned long) &node->timer[timer_index], pub_msg.frame_id, now);
  strcat(stat,temp);
  busy_wait_random(min_run_time_ms, max_run_time_ms);
  RCSOFTCHECK(rclc_publish(&node->publisher[pub_index], &pub_msg, NULL, pub_semantics));
  now = rclc_now(&support);
  sprintf(temp, "Timer %lu %ld %ld\n", (unsigned long) &node->timer[timer_index], pub_msg.frame_id, now);
  strcat(stat,temp);
}

void subscriber_callback(
  my_node_t * node, 
  char * stat,
  const custom_interfaces__msg__Message * msg,
  int sub_index,
  int pub_index,
  int min_run_time_ms,
  int max_run_time_ms,
  rclc_executor_semantics_t pub_semantics)
{
  char temp[1000] = "";  
  rcl_time_point_value_t now;
  now = rclc_now(&support);
  sprintf(temp, "Subscriber %lu %ld %ld\n", (unsigned long) &node->subscriber[sub_index], msg->frame_id, now);
  strcat(stat,temp);
  busy_wait_random(min_run_time_ms, max_run_time_ms);
  now = rclc_now(&support);
  if (pub_index >= 0)
  {
    RCSOFTCHECK(rclc_publish(&node->publisher[pub_index], msg, NULL, pub_semantics));
    sprintf(temp, "Subscriber %lu %ld %ld\n", (unsigned long) &node->subscriber[sub_index], msg->frame_id, now);
    strcat(stat,temp);     
  }
}

void timer_callback_print(
  my_node_t * node,
  int timer_index, 
  int pub_index, 
  int min_run_time_ms,
  int max_run_time_ms,
  rclc_executor_semantics_t pub_semantics)
{
  rcl_time_point_value_t now = rclc_now(&support);
  pub_msg.frame_id = node->count[timer_index]++;
  pub_msg.stamp = now;
  printf("Timer %lu %ld %ld\n", (unsigned long) &node->timer[timer_index], pub_msg.frame_id, now);
  busy_wait_random(min_run_time_ms, max_run_time_ms);
  RCSOFTCHECK(rclc_publish(&node->publisher[pub_index], &pub_msg, NULL, pub_semantics));
  now = rclc_now(&support);
  printf("Timer %lu %ld %ld\n", (unsigned long) &node->timer[timer_index], pub_msg.frame_id, now);
}

void subscriber_callback_print(
  my_node_t * node,
  const custom_interfaces__msg__Message * msg,
  int sub_index,
  int pub_index,
  int min_run_time_ms,
  int max_run_time_ms,
  rclc_executor_semantics_t pub_semantics)
{
  rcl_time_point_value_t now;
  now = rclc_now(&support);
  printf("Subscriber %lu %ld %ld\n", (unsigned long) &node->subscriber[sub_index], msg->frame_id, now);
  busy_wait_random(min_run_time_ms, max_run_time_ms);
  now = rclc_now(&support);
  if (pub_index >= 0)
  {
    RCSOFTCHECK(rclc_publish(&node->publisher[pub_index], msg, NULL, pub_semantics));
    printf("Subscriber %lu %ld %ld\n", (unsigned long) &node->subscriber[sub_index], msg->frame_id, now);     
  }
}

void timer_callback_error(
  my_node_t * node, 
  char * stat, 
  int timer_index, 
  int pub_index, 
  int min_run_time_ms,
  int max_run_time_ms,
  bool error,
  int error_time,
  rclc_executor_semantics_t pub_semantics)
{
  char temp[1000] = "";
  rcl_time_point_value_t now = rclc_now(&support);
  pub_msg.frame_id = node->count[timer_index]++;
  pub_msg.stamp = now;
  sprintf(temp, "Timer %lu %ld %ld\n", (unsigned long) &node->timer[timer_index], pub_msg.frame_id, now);
  strcat(stat,temp);
  busy_wait_random_error(min_run_time_ms, max_run_time_ms, error, error_time);
  RCSOFTCHECK(rclc_publish(&node->publisher[pub_index], &pub_msg, NULL, pub_semantics));
  now = rclc_now(&support);
  sprintf(temp, "Timer %lu %ld %ld\n", (unsigned long) &node->timer[timer_index], pub_msg.frame_id, now);
  strcat(stat,temp);
}

void subscriber_callback_error(
  my_node_t * node, 
  char * stat,
  const custom_interfaces__msg__Message * msg,
  int sub_index,
  int pub_index,
  int min_run_time_ms,
  int max_run_time_ms,
  bool error,
  int error_time,
  rclc_executor_semantics_t pub_semantics)
{
  char temp[1000] = "";  
  rcl_time_point_value_t now;
  now = rclc_now(&support);
  sprintf(temp, "Subscriber %lu %ld %ld\n", (unsigned long) &node->subscriber[sub_index], msg->frame_id, now);
  strcat(stat,temp);
  busy_wait_random_error(min_run_time_ms, max_run_time_ms, error, error_time);
  now = rclc_now(&support);
  if (pub_index >= 0)
  {
    RCSOFTCHECK(rclc_publish(&node->publisher[pub_index], msg, NULL, pub_semantics));
    sprintf(temp, "Subscriber %lu %ld %ld\n", (unsigned long) &node->subscriber[sub_index], msg->frame_id, now);
    strcat(stat,temp);     
  }
}

// void timer_callback_us(
//   my_node_t * node, 
//   char * stat, 
//   int timer_index, 
//   int pub_index, 
//   uint64_t min_run_time_us,
//   uint64_t max_run_time_us,
//   rclc_executor_semantics_t pub_semantics)
// {
//   char temp[1000] = "";
//   rcl_time_point_value_t now = rclc_now(&support);
//   pub_msg.frame_id = node->count[timer_index]++;
//   pub_msg.stamp = now;
//   sprintf(temp, "Timer %lu %ld %ld\n", (unsigned long) &node->timer[timer_index], pub_msg.frame_id, now);
//   strcat(stat,temp);
//   busy_wait_random_us(min_run_time_us, max_run_time_us);
//   RCSOFTCHECK(rclc_publish(&node->publisher[pub_index], &pub_msg, NULL, pub_semantics));
//   now = rclc_now(&support);
//   sprintf(temp, "Timer %lu %ld %ld\n", (unsigned long) &node->timer[timer_index], pub_msg.frame_id, now);
//   strcat(stat,temp);
// }

// void subscriber_callback_us(
//   my_node_t * node, 
//   char * stat,
//   const custom_interfaces__msg__Message * msg,
//   int sub_index,
//   int pub_index,
//   uint64_t min_run_time_us,
//   uint64_t max_run_time_us,
//   rclc_executor_semantics_t pub_semantics)
// {
//   char temp[1000] = "";  
//   rcl_time_point_value_t now;
//   now = rclc_now(&support);
//   sprintf(temp, "Subscriber %lu %ld %ld\n", (unsigned long) &node->subscriber[sub_index], msg->frame_id, now);
//   strcat(stat,temp);
//   busy_wait_random_us(min_run_time_us, max_run_time_us);
//   now = rclc_now(&support);
//   if (pub_index >= 0)
//   {
//     RCSOFTCHECK(rclc_publish(&node->publisher[pub_index], msg, NULL, pub_semantics));
//     sprintf(temp, "Subscriber %lu %ld %ld\n", (unsigned long) &node->subscriber[sub_index], msg->frame_id, now);
//     strcat(stat,temp);     
//   }
// }

typedef struct {
  size_t current_memory_size;
  size_t max_memory_size;
} rcl_allocator_state_t;

void *my_allocate(size_t size, void *state) {
    rcl_allocator_state_t *alloc_state = (rcl_allocator_state_t *)state;
    size_t *ptr = (size_t *)malloc(size + sizeof(size_t));
    if (!ptr) return NULL;
    *ptr = size;
    alloc_state->current_memory_size += size;
    if (alloc_state->current_memory_size > alloc_state->max_memory_size) {
        alloc_state->max_memory_size = alloc_state->current_memory_size;
    }
    return (void *)(ptr + 1);
}

void *main_allocate(size_t size, void *state) {
    void * ptr = my_allocate(size, state);
    return ptr;
}

void *support_allocate(size_t size, void *state) {
    void * ptr = my_allocate(size, state);
    return ptr;
}

void *executor_allocate(size_t size, void *state) {
    void * ptr = my_allocate(size, state);
    return ptr;
}

void my_deallocate(void *pointer, void *state) {
    if (!pointer) return;
    rcl_allocator_state_t *alloc_state = (rcl_allocator_state_t *)state;
    size_t *ptr = (size_t *)pointer - 1;
    alloc_state->current_memory_size -= *ptr;
    free(ptr);
}

void main_deallocate(void *pointer, void *state) {
    my_deallocate(pointer, state);
}

void support_deallocate(void *pointer, void *state) {
    my_deallocate(pointer, state);
}

void executor_deallocate(void *pointer, void *state) {
    my_deallocate(pointer, state);
}

void *my_reallocate(void *pointer, size_t size, void *state) {
    if (!pointer) return my_allocate(size, state);
    size_t *old_ptr = (size_t *)pointer - 1;
    size_t old_size = *old_ptr;
    size_t *new_ptr = (size_t *)realloc(old_ptr, size + sizeof(size_t));
    if (!new_ptr) return NULL;
    *new_ptr = size;
    rcl_allocator_state_t *alloc_state = (rcl_allocator_state_t *)state;
    alloc_state->current_memory_size += size - old_size;
    if (alloc_state->current_memory_size > alloc_state->max_memory_size) {
        alloc_state->max_memory_size = alloc_state->current_memory_size;
    }
    return (void *)(new_ptr + 1);
}

void *main_reallocate(void *pointer, size_t size, void *state) {
    void * ptr = my_reallocate(pointer, size, state);
    return ptr;
}

void *support_reallocate(void *pointer, size_t size, void *state) {
    void * ptr = my_reallocate(pointer, size, state);
    return ptr;
}

void *executor_reallocate(void *pointer, size_t size, void *state) {
    void * ptr = my_reallocate(pointer, size, state);
    return ptr;
}

void *my_zero_allocate(size_t number_of_elements, size_t size_of_element, void *state) {
    size_t total_size = number_of_elements * size_of_element;
    rcl_allocator_state_t *alloc_state = (rcl_allocator_state_t *)state;
    size_t *ptr = (size_t *)malloc(total_size + sizeof(size_t));
    if (!ptr) return NULL;
    *ptr = total_size; // Storing the total size
    alloc_state->current_memory_size += total_size;
    if (alloc_state->current_memory_size > alloc_state->max_memory_size) {
        alloc_state->max_memory_size = alloc_state->current_memory_size;
    }
    void *user_ptr = (void *)(ptr + 1);
    memset(user_ptr, 0, total_size); // Set all bytes to zero
    return user_ptr;
}

void *main_zero_allocate(size_t number_of_elements, size_t size_of_element, void *state) {
    void * ptr = my_zero_allocate(number_of_elements, size_of_element, state);
    return ptr;
}

void *support_zero_allocate(size_t number_of_elements, size_t size_of_element, void *state) {
    void * ptr = my_zero_allocate(number_of_elements, size_of_element, state);
    return ptr;
}

void *executor_zero_allocate(size_t number_of_elements, size_t size_of_element, void *state) {
    void * ptr = my_zero_allocate(number_of_elements, size_of_element, state);
    return ptr;
}