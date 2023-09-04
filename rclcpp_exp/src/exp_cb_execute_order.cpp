#include <chrono>
#include <functional>
#include <memory>
#include <iostream>

#include "custom_interfaces/msg/message.hpp"
#include "thread_utilities.hpp"
#include "my_node_utilities.hpp"

using std::placeholders::_1;
using namespace std::chrono_literals;
using namespace my_thread_utilities;

/*
 * This is the example for sequential execution
 */

class Node1 : public MyNode
{
public:
	Node1() : MyNode("node1"), count(0), first_run(1)
	{
   		// Initialize the vector with 4 vectors of size 100 filled with 0's.
    	timestamp = std::vector<std::vector<int64_t>>(4, std::vector<int64_t>(100, 0));

		publisher1 = this->create_publisher<custom_interfaces::msg::Message>("topic1", 10);
		subscriber1 = this->create_subscription<custom_interfaces::msg::Message>(
			"topic1", 10, std::bind(&Node1::sub1_callback, this, _1));
		timer1 = this->create_wall_timer(100ms, std::bind(&Node1::timer1_callback, this));
		timer2 = this->create_wall_timer(500ms, std::bind(&Node1::timer2_callback, this));
	}

private:
	void timer1_callback()
	{
		auto message = custom_interfaces::msg::Message();
		rclcpp::Time now = this->now();
		if(first_run)
		{
			start_time = now;
			first_run = 0;
		}
		//message.data = "Hello" + std::to_string(count++);
		message.frame_id = count++;
		message.stamp = now.nanoseconds();
		std::cout << "Publishing: " << message.frame_id << "\n";
		//timestamp.at(0).at(message.frame_id) = (now - start_time).nanoseconds()/1000;
		//RCLCPP_INFO(this->get_logger(), "Publishing: '%s' at %f", message.header.frame_id.c_str(), t);
		publisher1->publish(message);
	}

	void timer2_callback()
	{
		rclcpp::Time now = this->now();
		busy_wait(80ms);
		//timestamp.at(1).at(count) = (now - start_time).nanoseconds()/1000;
		//RCLCPP_INFO(this->get_logger(), "Publishing: '%s' at %f", message.header.frame_id.c_str(), t);
	}
	
	void sub1_callback(const custom_interfaces::msg::Message & msg)
  	{
  		rclcpp::Time now = this->now();
			std::cout << "Receiving: " << msg.frame_id << "\n";
  		//timestamp.at(2).at(msg.frame_id) = (now.nanoseconds() - msg.stamp)/1000;
    	//RCLCPP_INFO(this->get_logger(), "I heard: '%s' at %f", msg.header.frame_id.c_str(), t);
    	//msg.header.stamp = now - start_time;
    	busy_wait(80ms);
    	now = this->now();
    	//timestamp.at(3).at(msg.frame_id) = (now.nanoseconds() - msg.stamp)/1000;
  	}

	rclcpp::TimerBase::SharedPtr timer1;
	rclcpp::TimerBase::SharedPtr timer2;
	rclcpp::Publisher<custom_interfaces::msg::Message>::SharedPtr publisher1;
	rclcpp::Subscription<custom_interfaces::msg::Message>::SharedPtr subscriber1;
	rclcpp::Time start_time;
	size_t count;
	int first_run;
};


int main(int argc, char const *argv[])
{
	rclcpp::init(argc, argv);
	
	// Create 1 executor for each node
	rclcpp::executors::SingleThreadedExecutor executor;
	auto node1 = std::make_shared<Node1>();
 	executor.add_node(node1);
 	rclcpp::Logger logger1 = node1->get_logger();

 	// Create a thread for each executor
 	auto thread1 = std::thread([&]() 
 	{
     executor.spin();
  });

	const int CPU_ZERO = 0;
	bool ret = configure_thread(thread1, ThreadPriority::HIGH, CPU_ZERO);
	if (!ret) {
	  RCLCPP_WARN(logger1, "Failed to configure high priority thread, are you root?");
	}

	const std::chrono::seconds EXPERIMENT_DURATION = 100s;
  	RCLCPP_INFO_STREAM(
    	logger1, "Running experiment from now on for " << EXPERIMENT_DURATION.count() << " seconds ...");
  	std::this_thread::sleep_for(EXPERIMENT_DURATION);
  	
  rclcpp::shutdown();
  thread1.join();
  node1->print_statistics();
	return 0;
}