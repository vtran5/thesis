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
 * This is the example for showing unpredictability. This experiment simulates the sense-plan-act pipeline of 
 * the robot with 3 nodes. All three will run in a single core processor. The experiment will run for both 
 * single thread (non-preemptive) and multi thread (preemptive).
 */
class Node1 : public MyNode
{
public:
	Node1(unsigned int timer_period) : MyNode("node1"), count1(0), first_run(1)
	{
   		// Initialize the vector with 4 vectors of size 100 filled with 0's.
    	timestamp = std::vector<std::vector<int64_t>>(1, std::vector<int64_t>(500, 0));
    	publisher1 = this->create_publisher<custom_interfaces::msg::Message>("topic1", 10);
		timer1 = this->create_wall_timer(timer_period*1ms, std::bind(&Node1::timer1_callback, this));
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
		message.frame_id = count1++;
		message.stamp = now.nanoseconds();
		timestamp.at(0).at(message.frame_id) = (now - start_time).nanoseconds()/1000;
		//RCLCPP_INFO(this->get_logger(), "Publishing: '%d' at %ld and %ld", message.frame_id, now.nanoseconds(), start_time.nanoseconds());
		publisher1->publish(message);
	}
	

	rclcpp::TimerBase::SharedPtr timer1;
	rclcpp::Publisher<custom_interfaces::msg::Message>::SharedPtr publisher1;
	rclcpp::Time start_time;
	size_t count1;
	int first_run;
};

class Node2 : public MyNode
{
public:
	Node2() : MyNode("node2")
	{
   		// Initialize the vector with 4 vectors of size 100 filled with 0's.
    timestamp = std::vector<std::vector<int64_t>>(2, std::vector<int64_t>(500, 0));

	subscriber1 = this->create_subscription<custom_interfaces::msg::Message>(
			"topic1", 10, std::bind(&Node2::sub1_callback, this, _1));

	publisher1 = this->create_publisher<custom_interfaces::msg::Message>("topic2", 10);

	}

private:
	void sub1_callback(const custom_interfaces::msg::Message & msg)
  	{
  		rclcpp::Time now = this->now();
  		timestamp.at(0).at(msg.frame_id) = (now.nanoseconds() - msg.stamp)/1000;
    	//RCLCPP_INFO(this->get_logger(), "I heard: '%d' at %ld", msg.frame_id, timestamp.at(0).at(msg.frame_id));
    	//msg.header.stamp = now - start_time;
    	busy_wait_random(5ms, 15ms);
    	now = this->now();
  		timestamp.at(1).at(msg.frame_id) = (now.nanoseconds() - msg.stamp)/1000;
  		publisher1->publish(msg);
  	}

	rclcpp::Subscription<custom_interfaces::msg::Message>::SharedPtr subscriber1;
	rclcpp::Publisher<custom_interfaces::msg::Message>::SharedPtr publisher1;
	rclcpp::Time start_time;
};

class Node3 : public MyNode
{
public:
	Node3() : MyNode("node3")
	{
   		// Initialize the vector with 4 vectors of size 100 filled with 0's.
    	timestamp = std::vector<std::vector<int64_t>>(2, std::vector<int64_t>(500, 0));

		subscriber1 = this->create_subscription<custom_interfaces::msg::Message>(
			"topic2", 10, std::bind(&Node3::sub1_callback, this, _1));

		publisher1 = this->create_publisher<custom_interfaces::msg::Message>("topic3", 10);

	}

private:
	void sub1_callback(const custom_interfaces::msg::Message & msg)
  	{
  		rclcpp::Time now = this->now();
  		timestamp.at(0).at(msg.frame_id) = (now.nanoseconds() - msg.stamp)/1000;
    	//RCLCPP_INFO(this->get_logger(), "I heard: '%d' at %ld", msg.frame_id, timestamp.at(0).at(msg.frame_id));
    	//msg.header.stamp = now - start_time;
    	busy_wait_random(5ms, 20ms);
    	now = this->now();
  		timestamp.at(1).at(msg.frame_id) = (now.nanoseconds() - msg.stamp)/1000;
  		publisher1->publish(msg);
  	}

	rclcpp::Subscription<custom_interfaces::msg::Message>::SharedPtr subscriber1;
	rclcpp::Publisher<custom_interfaces::msg::Message>::SharedPtr publisher1;
	rclcpp::Time start_time;
};

class Node4 : public MyNode
{
public:
	Node4() : MyNode("node4")
	{
   		// Initialize the vector with 4 vectors of size 100 filled with 0's.
    timestamp = std::vector<std::vector<int64_t>>(1, std::vector<int64_t>(500, 0));

		subscriber1 = this->create_subscription<custom_interfaces::msg::Message>(
			"topic3", 10, std::bind(&Node4::sub1_callback, this, _1));

	}

private:
	void sub1_callback(const custom_interfaces::msg::Message & msg)
  	{
  		rclcpp::Time now = this->now();
  		timestamp.at(0).at(msg.frame_id) = (now.nanoseconds() - msg.stamp)/1000;
    	//RCLCPP_INFO(this->get_logger(), "I heard: '%d' at %ld", msg.frame_id, timestamp.at(0).at(msg.frame_id));
  	}

	rclcpp::Subscription<custom_interfaces::msg::Message>::SharedPtr subscriber1;
	rclcpp::Time start_time;
};

int main(int argc, char const *argv[])
{
	unsigned int timer_period = 100;
	unsigned int experiment_duration = 10000;

	parse_arguments(argc, argv, timer_period, experiment_duration);

	rclcpp::init(argc, argv);
	
	// Create 1 executor for each node
	rclcpp::executors::SingleThreadedExecutor executor1;
	rclcpp::executors::SingleThreadedExecutor executor2;
	rclcpp::executors::SingleThreadedExecutor executor3;
	rclcpp::executors::SingleThreadedExecutor executor4;
	auto node1 = std::make_shared<Node1>(timer_period);
	auto node2 = std::make_shared<Node2>();
	auto node3 = std::make_shared<Node3>();
	auto node4 = std::make_shared<Node4>();
 	executor1.add_node(node1);
 	executor2.add_node(node2);
 	executor3.add_node(node3);
 	executor4.add_node(node4);
 	rclcpp::Logger logger1 = node1->get_logger();
 	rclcpp::Logger logger2 = node2->get_logger();
 	rclcpp::Logger logger3 = node3->get_logger();
 	rclcpp::Logger logger4 = node4->get_logger();

 	// Create a thread for each executor
 	auto thread1 = std::thread([&]() 
 	{
     executor1.spin();
   });
 	 auto thread2 = std::thread([&]() 
 	{
     executor2.spin();
   });
 	 auto thread3 = std::thread([&]() 
 	{
     executor3.spin();
   });
    auto thread4 = std::thread([&]() 
 	{
     executor4.spin();
   });
    // ... and configure them accordinly as high and low prio and pin them to the
	// first CPU. Hence, the two executors compete about this computational resource.
	bool ret = configure_thread(thread1, ThreadPriority::HIGH, 0);
	ret = configure_thread(thread2, ThreadPriority::HIGH, 0);
	ret = configure_thread(thread3, ThreadPriority::HIGH, 0);
	ret = configure_thread(thread4, ThreadPriority::HIGH, 0);
	if (!ret) {
	  RCLCPP_WARN(logger1, "Failed to configure high priority thread, are you root?");
	}

  	//RCLCPP_INFO_STREAM(
    //	logger1, "Running experiment from now on for " << EXPERIMENT_DURATION.count() << " seconds ...");
  	std::this_thread::sleep_for(experiment_duration*1ms);
  	
  	rclcpp::shutdown();
  	thread1.join();
  	thread2.join();
  	thread3.join();
  	thread4.join();
  	node1->print_statistics();
  	node2->print_statistics();
  	node3->print_statistics();
  	node4->print_statistics();
	return 0;
}