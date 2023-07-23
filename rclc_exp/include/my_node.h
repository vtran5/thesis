#include <stdlib.h>
#include <rclc/rclc.h>
#include <stdio.h>
#include <stdint.h>
#include "utilities.h"
typedef struct 
{
  rclc_executor_handle_type_t type;
  void * handle_ptr;
} callback_t;
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
  void (**subscriber_callback)(const void *);
  rcl_timer_t * timer;
  void (**timer_callback)(rcl_timer_t * timer, int64_t last_call_time);
} my_node_t;

my_node_t * create_node(
  int timer_num, 
  int pub_num, 
  int sub_num, 
  int callback_chain_num)
{
  my_node_t * node = malloc(sizeof(my_node_t));
  if(node == NULL)
  {
    return NULL;
  }

  node->first_run = true;
  node->timer_num = timer_num;
  node->pub_num = pub_num;
  node->sub_num = sub_num;

  if(callback_chain_num > 0)
  {
    node->count = malloc(sizeof(int64_t)*callback_chain_num);
    if (node->count == NULL)
    {
      free(node);
      return NULL;
    }
    for (int i = 0; i < callback_chain_num; i++)
    {
      node->count[i] = 0;
    }    
  }

  if(pub_num > 0)
  {
    node->publisher = malloc(sizeof(rclc_publisher_t)*pub_num);
    if(node->publisher == NULL)
    {
      if(node->count != NULL)
        free(node->count);
      free(node);
      return NULL;
    }
    node->callback = malloc(sizeof(callback_t)*pub_num);
    if (node->callback == NULL)
      return NULL;
  }

  if(sub_num > 0)
  {
    node->subscriber = malloc(sizeof(rcl_subscription_t)*sub_num);
    if(node->subscriber == NULL)
    {
      if(node->publisher != NULL)
        free(node->publisher);
      if(node->count != NULL)
        free(node->count);
      free(node);
      return NULL;
    }

    node->sub_msg = malloc(sizeof(custom_interfaces__msg__Message)*sub_num);
    if(node->sub_msg == NULL)
    {
      if(node->subscriber != NULL)
        free(node->subscriber);
      if(node->publisher != NULL)
        free(node->publisher);
      if(node->count != NULL)
        free(node->count);
      free(node);
      return NULL;
    } 

    node->subscriber_callback = malloc(sizeof(void (*)(const void *))*sub_num);
    if(node->subscriber_callback == NULL)
    {
      if(node->sub_msg != NULL)
        free(node->sub_msg);
      if(node->subscriber != NULL)
        free(node->subscriber);
      if(node->publisher != NULL)
        free(node->publisher);
      if(node->count != NULL)
        free(node->count);
      free(node);
      return NULL;
    }    
  }

  if(timer_num > 0)
  {
    node->timer = malloc(sizeof(rcl_timer_t)*timer_num);
    if(node->timer == NULL)
    {
      if (node->subscriber_callback != NULL)
      {
        free(node->subscriber_callback);
        free(node->sub_msg);
        free(node->subscriber);        
      }
      if(node->publisher != NULL)
        free(node->publisher);
      if(node->count != NULL)
        free(node->count);
      free(node);
      return NULL;
    }  

    node->timer_callback = malloc(sizeof(void (*)(rcl_timer_t *, int64_t))*timer_num);
    if(node->timer_callback == NULL)
    {
      if(node->timer != NULL)
        free(node->timer);  
      if (node->subscriber_callback != NULL)
      {
        free(node->subscriber_callback);
        free(node->sub_msg);
        free(node->subscriber);        
      }
      if(node->publisher != NULL)
        free(node->publisher);
      if(node->count != NULL)
        free(node->count);
      free(node);
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
  rclc_executor_semantics_t semantics)
{
  RCLC_UNUSED(semantics);
  if((node == NULL) || (my_type_support == NULL) | (topic_name == NULL) | (profile == NULL))
    return;
  for (int i = 0; i < node->pub_num; i++)
  {
    VOID_RCCHECK(rclc_publisher_init(&node->publisher[i], 
      &node->rcl_node, my_type_support, topic_name[i], profile));
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

void destroy_node(my_node_t * node)
{
  if(node == NULL)
    return;

  if(node->timer != NULL)
  {
    for (int i = 0; i < node->timer_num; i++)
    {
      VOID_RCCHECK(rcl_timer_fini(&node->timer[i]));
    }
    free(node->timer);
  }

  if(node->publisher != NULL)
  {
    for (int i = 0; i < node->pub_num; i++)
    {
      VOID_RCCHECK(rclc_publisher_fini(&node->publisher[i], &node->rcl_node));
      //VOID_RCCHECK(rclc_publisher_let_fini(&node->publisher[i]));
    }    
    free(node->publisher);
    free(node->callback);
  }

  if(node->count != NULL)
    free(node->count);

  if(node->subscriber != NULL)
  {
    for (int i = 0; i < node->sub_num; i++)
    {
      VOID_RCCHECK(rcl_subscription_fini(&node->subscriber[i], &node->rcl_node));
    }    
    free(node->subscriber);    
  }

  if(node->sub_msg != NULL)
    free(node->sub_msg);

  if(node->timer_callback != NULL)
    free(node->timer_callback);

  if(node->subscriber_callback != NULL)
    free(node->subscriber_callback);

  VOID_RCCHECK(rcl_node_fini(&node->rcl_node));
}

char** create_topic_name_array(size_t array_size)
{
  if(array_size <= 0)
    return NULL;

  char ** arr = malloc(array_size * sizeof(char *));
  if(arr == NULL)
    return NULL;

  for(size_t i = 0; i < array_size; i++)
  {
    arr[i] = malloc(8*sizeof(char)); // "topicXX" plus null terminator
    if (arr[i] == NULL) {
      for (size_t j = 0; j < i; ++j) {
          free(arr[j]);
      }
      free(arr);
      return NULL;
    }
  }
  return arr;
}

void destroy_topic_name_array(char** arr, size_t array_size) {
    if (arr != NULL) {
        // Free each string
        for (size_t i = 0; i < array_size; ++i) {
            free(arr[i]);
        }
        
        // Then free the array itself
        free(arr);
        arr = NULL;
    }
}

rcutils_time_point_value_t * create_time_array(size_t array_size)
{
  if(array_size <= 0)
    return NULL;

  rcutils_time_point_value_t * arr = malloc(array_size * sizeof(rcutils_time_point_value_t));
  if(arr == NULL)
    printf("Fail to allocate memory for callback let array\n");
  return arr;
}

void destroy_time_array(rcutils_time_point_value_t * array)
{
  if(array != NULL)
    free(array);
  array = NULL;
}