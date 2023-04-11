from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    ld = LaunchDescription()

    node1 = Node(
        package="rclcpp_app",
        executable="mycppapp",
    )

    ld.add_action(node1)

    return ld