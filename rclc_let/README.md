This is the improved version of the official rclc package. A full Logical Execution Time (LET) has been implemented by adding the functionality to write the output at the end of the executor period. The gist of the change is instead of publishing the message directly, the message is stored in an internal buffer before it is published at the end of the executor period.

The API changes are described below:
1. [include/rclc/buffer.h]
    - Implement circular queue rclc_circular_queue_t to use in the publisher. The queue use dynamic memory allocation.
2. [include/rclc/publisher.h] 
    - Introduced a rclc_publisher_t wrapper around rcl_publisher_t that also contains a buffer to store the messages.
    - Split the rcl_publish() into 2 functions: rclc_publish_LET() that adds the message to the internal buffer and rclc_LET_output() that takes message from the internal buffer and publishes it with rcl_publish().
    - Added functions to initialize and deinitialize the publisher. 
3. [include/rclc/executor_handle.h]
    - Introduced an enum (rclc_executor_let_handle_type_t) to distinguish different types of output-producing mechanisms (publisher, service, action, etc). Currently only the publisher is implemented.
    - Introduced a struct (rclc_executor_let_handle_t) that contains the pointer to the output-producing entity and its type.
    - Added (function rclc_executor_let_handle_init()) to initialize the entity handle.
4. [include/rclc/executor.h]
    - Added a dynamic array (let_handles) for output-producing DDS-handles and its related variables (index and capacity) into the executor struct (rclc_executor_t).
    - Added functions to initialize and deinitialize the array above.
    - Added a function to add the publisher to the array above.
5. [src/rclc/executor.c]
    - Added a local function to go through all the handles in the let_handles array and publish all the messages in the internal buffer of the handles.
    - Added call to the function above at the end of rclc_executor_spin_one_period().