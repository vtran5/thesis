// Copyright (c) 2019 - for information on the respective copyright owner
// see the NOTICE file and/or the repository https://github.com/ros2/rclc.
// Copyright 2014 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <std_msgs/msg/int32.h>
#include <gtest/gtest.h>
#include <rclc/rclc.h>

TEST(Test, rclc_publisher_init_default) {
  rclc_support_t support;
  rcl_ret_t rc;

  // preliminary setup
  rcl_allocator_t allocator = rcl_get_default_allocator();
  rc = rclc_support_init(&support, 0, nullptr, &allocator);
  const char * my_name = "test_pub";
  const char * my_namespace = "test_namespace";
  rcl_node_t node = rcl_get_zero_initialized_node();
  rc = rclc_node_init_default(&node, my_name, my_namespace, &support);
  EXPECT_EQ(RCL_RET_OK, rc);

  // test with valid arguments
  rcl_publisher_t rcl_publisher = rcl_get_zero_initialized_publisher();
  const rosidl_message_type_support_t * type_support =
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32);

  rclc_publisher_t publisher;
  publisher.rcl_publisher = rcl_publisher;
  rc = rclc_publisher_init_default(&publisher, &node, type_support, "topic1", sizeof(int), 5);
  EXPECT_EQ(RCL_RET_OK, rc);

  // tests with invalid arguments
  rc = rclc_publisher_init_default(nullptr, &node, type_support, "topic1", sizeof(int), 5);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, rc);
  rcutils_reset_error();
  rc = rclc_publisher_init_default(&publisher, nullptr, type_support, "topic1", sizeof(int), 5);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, rc);
  rcutils_reset_error();
  rc = rclc_publisher_init_default(&publisher, &node, nullptr, "topic1", sizeof(int), 5);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, rc);
  rcutils_reset_error();
  rc = rclc_publisher_init_default(&publisher, &node, type_support, nullptr, sizeof(int), 5);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, rc);
  rcutils_reset_error();
  rc = rclc_publisher_init_default(&publisher, &node, type_support, "topic1", -1, 5);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, rc);
  rcutils_reset_error();
  rc = rclc_publisher_init_default(&publisher, &node, type_support, "topic1", sizeof(int), -1);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, rc);
  rcutils_reset_error();

  // clean up
  rc = rclc_publisher_fini(&publisher, &node);
  EXPECT_EQ(RCL_RET_OK, rc);
  rc = rcl_node_fini(&node);
  EXPECT_EQ(RCL_RET_OK, rc);
  rc = rclc_support_fini(&support);
  EXPECT_EQ(RCL_RET_OK, rc);
}

TEST(Test, rclc_publisher_init_best_effort) {
  rclc_support_t support;
  rcl_ret_t rc;

  // preliminary setup
  rcl_allocator_t allocator = rcl_get_default_allocator();
  rc = rclc_support_init(&support, 0, nullptr, &allocator);
  const char * my_name = "test_pub_be";
  const char * my_namespace = "test_namespace";
  rcl_node_t node = rcl_get_zero_initialized_node();
  rc = rclc_node_init_default(&node, my_name, my_namespace, &support);

  // test with valid arguments
  rcl_publisher_t rcl_publisher = rcl_get_zero_initialized_publisher();
  const rosidl_message_type_support_t * type_support =
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32);

  rclc_publisher_t publisher;
  publisher.rcl_publisher = rcl_publisher;
  rc = rclc_publisher_init_best_effort(&publisher, &node, type_support, "topic1", sizeof(int), 5);
  EXPECT_EQ(RCL_RET_OK, rc);

  // check for qos-option best effort
  const rcl_publisher_options_t * pub_options = rcl_publisher_get_options(&(publisher.rcl_publisher));
  EXPECT_EQ(pub_options->qos.reliability, RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT);

  // tests with invalid arguments
  rc = rclc_publisher_init_best_effort(nullptr, &node, type_support, "topic1", sizeof(int), 5);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, rc);
  rcutils_reset_error();
  rc = rclc_publisher_init_best_effort(&publisher, nullptr, type_support, "topic1", sizeof(int), 5);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, rc);
  rcutils_reset_error();
  rc = rclc_publisher_init_best_effort(&publisher, &node, nullptr, "topic1", sizeof(int), 5);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, rc);
  rcutils_reset_error();
  rc = rclc_publisher_init_best_effort(&publisher, &node, type_support, nullptr, sizeof(int), 5);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, rc);
  rcutils_reset_error();
  rc = rclc_publisher_init_best_effort(&publisher, &node, type_support, "topic1", -1, 5);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, rc);
  rcutils_reset_error();
  rc = rclc_publisher_init_best_effort(&publisher, &node, type_support, "topic1", sizeof(int), -1);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, rc);
  rcutils_reset_error();

  // clean up
  rc = rclc_publisher_fini(&publisher, &node);
  EXPECT_EQ(RCL_RET_OK, rc);
  rc = rcl_node_fini(&node);
  EXPECT_EQ(RCL_RET_OK, rc);
  rc = rclc_support_fini(&support);
  EXPECT_EQ(RCL_RET_OK, rc);
}

TEST(Test, rclc_publisher_init_qos) {
  rclc_support_t support;
  rcl_ret_t rc;

  // preliminary setup
  rcl_allocator_t allocator = rcl_get_default_allocator();
  rc = rclc_support_init(&support, 0, nullptr, &allocator);
  const char * my_name = "test_pub_qos";
  const char * my_namespace = "test_namespace";
  rcl_node_t node = rcl_get_zero_initialized_node();
  rc = rclc_node_init_default(&node, my_name, my_namespace, &support);

  // test with valid arguments
  rcl_publisher_t rcl_publisher = rcl_get_zero_initialized_publisher();
  const rmw_qos_profile_t * qos_profile = &rmw_qos_profile_default;
  const rosidl_message_type_support_t * type_support =
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32);

  rclc_publisher_t publisher;
  publisher.rcl_publisher = rcl_publisher;
  rc = rclc_publisher_init(&publisher, &node, type_support, "topic1", sizeof(int), 5, qos_profile);
  EXPECT_EQ(RCL_RET_OK, rc);

  // check for qos-option best effort
  const rcl_publisher_options_t * pub_options = rcl_publisher_get_options(&(publisher.rcl_publisher));
  EXPECT_EQ(pub_options->qos.reliability, rmw_qos_profile_default.reliability);

  // tests with invalid arguments
  rc = rclc_publisher_init(nullptr, &node, type_support, "topic1", sizeof(int), 5, qos_profile);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, rc);
  rcutils_reset_error();
  rc = rclc_publisher_init(&publisher, nullptr, type_support, "topic1", sizeof(int), 5, qos_profile);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, rc);
  rcutils_reset_error();
  rc = rclc_publisher_init(&publisher, &node, nullptr, "topic1", sizeof(int), 5, qos_profile);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, rc);
  rcutils_reset_error();
  rc = rclc_publisher_init(&publisher, &node, type_support, nullptr, sizeof(int), 5, qos_profile);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, rc);
  rcutils_reset_error();
  rc = rclc_publisher_init(&publisher, &node, type_support, "topic1", sizeof(int), 5, nullptr);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, rc);
  rcutils_reset_error();
  rc = rclc_publisher_init(&publisher, &node, type_support, "topic1", -1, 5, qos_profile);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, rc);
  rcutils_reset_error();
  rc = rclc_publisher_init(&publisher, &node, type_support, "topic1", sizeof(int), -5, qos_profile);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, rc);
  rcutils_reset_error();

  // clean up
  rc = rclc_publisher_fini(&publisher, &node);
  EXPECT_EQ(RCL_RET_OK, rc);
  rc = rcl_node_fini(&node);
  EXPECT_EQ(RCL_RET_OK, rc);
  rc = rclc_support_fini(&support);
  EXPECT_EQ(RCL_RET_OK, rc);
}


TEST(Test, rclc_publish_LET)
{
  rcl_ret_t ret = RCL_RET_OK;
  rclc_publisher_t publisher;
}