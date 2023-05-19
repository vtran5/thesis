#ifndef MY_NODE_UTILITIES_HPP_
#define MY_NODE_UTILITIES_HPP_

#include <vector>
#include <string>
#include "rclcpp/rclcpp.hpp"

class MyNode : public rclcpp::Node
{
public:
	MyNode(std::string node_name);
	void print_statistics();

private:
	void print_timestamp(std::vector<std::vector<int64_t>>& timestamp);

protected:
	void busy_wait_random(std::chrono::milliseconds min_time, std::chrono::milliseconds max_time);
	void busy_wait(std::chrono::milliseconds duration);
	std::vector<std::vector<int64_t>> timestamp;
	std::string stat;
};
#endif