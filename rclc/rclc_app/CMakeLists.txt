cmake_minimum_required(VERSION 3.8)
project(rclc_app)

# Default to C11
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 11)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(ament_cmake_ros REQUIRED)
find_package(rcl REQUIRED)
find_package(rcl_lifecycle REQUIRED)
find_package(rclc REQUIRED)
find_package(rclc_lifecycle REQUIRED)
find_package(std_msgs REQUIRED)
find_package(lifecycle_msgs REQUIRED)
find_package(example_interfaces REQUIRED)
find_package(custom_interfaces REQUIRED)  
find_package(rclc_parameter REQUIRED)
find_package(Threads REQUIRED)

include_directories(include)

add_compile_definitions(RCUTILS_LOG_MIN_SEVERITY=RCUTILS_LOG_MIN_SEVERITY_WARN)

add_executable(exp1 src/experiment1.c src/utilities.c)
ament_target_dependencies(exp1 rcl rclc custom_interfaces)

add_executable(exp2 src/experiment2.c src/utilities.c)
ament_target_dependencies(exp2 rcl rclc custom_interfaces)

add_executable(exp3 src/experiment3.c src/utilities.c)
ament_target_dependencies(exp3 rcl rclc custom_interfaces)

add_executable(exp4 src/experiment4.c src/utilities.c)
ament_target_dependencies(exp4 rcl rclc custom_interfaces)

add_executable(exp4_1 src/experiment4_1.c src/utilities.c)
ament_target_dependencies(exp4_1 rcl rclc custom_interfaces)

add_executable(mytest src/test.c src/utilities.c)
ament_target_dependencies(mytest rcl rclc custom_interfaces)

# find dependencies
# find_package(ament_cmake REQUIRED)
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)

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



install(TARGETS
  exp1
  mytest
  exp2
  exp3
  exp4
  exp4_1
  DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY include/
  DESTINATION include
)

ament_package()
