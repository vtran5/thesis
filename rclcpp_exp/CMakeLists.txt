cmake_minimum_required(VERSION 3.8)
project(rclcpp_exp)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(custom_interfaces REQUIRED)     

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

include_directories(include)

add_executable(exp1 src/exp_cb_execute_order.cpp src/my_node_utilities.cpp)
ament_target_dependencies(exp1 rclcpp custom_interfaces)

add_executable(exp2 src/exp_trigger_all.cpp src/my_node_utilities.cpp)
ament_target_dependencies(exp2 rclcpp custom_interfaces)

add_executable(exp3 src/experiment3.cpp src/my_node_utilities.cpp)
ament_target_dependencies(exp3 rclcpp custom_interfaces)

add_executable(exp4 src/exp_single_thread_single_core.cpp src/my_node_utilities.cpp)
ament_target_dependencies(exp4 rclcpp custom_interfaces)

add_executable(exp5 src/exp_multi_thread_single_core.cpp src/my_node_utilities.cpp)
ament_target_dependencies(exp5 rclcpp custom_interfaces)

install(TARGETS
  exp1
  exp2
  exp3
  exp4
  exp5
  DESTINATION lib/${PROJECT_NAME})

install(
  DIRECTORY include/
  DESTINATION include
)
ament_package()
