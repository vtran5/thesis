# thesis

This repo contains the the code for my graduation project. The aim of the project is to improve the predictability of the rclc executor in ROS2/microROS. The repo includes:
1. custom_interfaces: a ROS2 package that defines a custom-type message format to be used in the experiments communication in this project.
2. my_app_startup: a launch system to launch the rclcpp experiments 
3. rclc: the official rclc package (Humble release) from Micro-ROS project 
4. rclc_let: the improved version of rclc with Logical Execution Time implemented
5. rclc_exp: a package that contains the experiments to test rclc executor performance
6. rclcpp_exp: a package that contains the experiments to test the rclcpp executor performance
7. result: stores the raw and visualized results from the rclc and rclcpp experiments

The experiments in rclc_exp and rclcpp_exp are run like a node in ROS2. To run it, first ROS2 (Humble release) and all its relevant tools needs to be installed. The installation guide can be found here: https://docs.ros.org/en/humble/Installation.html. The source code of ROS2 can also be downloaded using the following command (taken from the build from source guide):
		
		mkdir -p ~/ros2_humble/src
		cd ~/ros2_humble
		vcs import --input https://raw.githubusercontent.com/ros2/ros2/humble/ros2.repos src


