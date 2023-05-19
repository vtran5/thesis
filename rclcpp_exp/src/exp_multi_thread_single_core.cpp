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
    publisher1 = this->create_publisher<custom_interfaces::msg::Message>("topic1", 10);
		timer1 = this->create_wall_timer(timer_period*1ms, std::bind(&Node1::timer1_callback, this));
		timer1_ptr = reinterpret_cast<uintptr_t>(timer1.get());
	}
	uintptr_t timer1_ptr;

private:
	void timer1_callback()
	{
		auto message = custom_interfaces::msg::Message();
		rclcpp::Time now = this->now();
		if(first_run)
		{
			start_time = now;
			first_run = 0;
			stat += "StartTime " + std::to_string(start_time.nanoseconds()) + "\n";
		}
		message.frame_id = count1++;
		message.stamp = now.nanoseconds();
		stat += "Timer " + std::to_string(timer1_ptr) + " " + std::to_string(message.frame_id) + " " + std::to_string(now.nanoseconds()) + "\n";
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
	subscriber1 = this->create_subscription<custom_interfaces::msg::Message>(
			"topic1", 10, std::bind(&Node2::sub1_callback, this, _1));

	publisher1 = this->create_publisher<custom_interfaces::msg::Message>("topic2", 10);
	sub1_ptr = reinterpret_cast<uintptr_t>(subscriber1.get());
	}
	uintptr_t sub1_ptr;
private:
	void sub1_callback(const custom_interfaces::msg::Message & msg)
  	{
  		rclcpp::Time now = this->now();
  		stat += "Subscriber " + std::to_string(sub1_ptr) + " " + std::to_string(msg.frame_id) + " " + std::to_string(now.nanoseconds()) + "\n";
    	busy_wait_random(5ms, 15ms);
    	now = this->now();
  		stat += "Subscriber " + std::to_string(sub1_ptr) + " " + std::to_string(msg.frame_id) + " " + std::to_string(now.nanoseconds()) + "\n";
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
		subscriber1 = this->create_subscription<custom_interfaces::msg::Message>(
			"topic2", 10, std::bind(&Node3::sub1_callback, this, _1));

		publisher1 = this->create_publisher<custom_interfaces::msg::Message>("topic3", 10);
		sub1_ptr = reinterpret_cast<uintptr_t>(subscriber1.get());
	}
	uintptr_t sub1_ptr;

private:
	void sub1_callback(const custom_interfaces::msg::Message & msg)
  	{
  		rclcpp::Time now = this->now();
  		stat += "Subscriber " + std::to_string(sub1_ptr) + " " + std::to_string(msg.frame_id) + " " + std::to_string(now.nanoseconds()) + "\n";
    	busy_wait_random(5ms, 20ms);
    	now = this->now();
  		stat += "Subscriber " + std::to_string(sub1_ptr) + " " + std::to_string(msg.frame_id) + " " + std::to_string(now.nanoseconds()) + "\n";
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
		subscriber1 = this->create_subscription<custom_interfaces::msg::Message>(
			"topic3", 10, std::bind(&Node4::sub1_callback, this, _1));
		sub1_ptr = reinterpret_cast<uintptr_t>(subscriber1.get());
	}
	uintptr_t sub1_ptr;

private:
	void sub1_callback(const custom_interfaces::msg::Message & msg)
  	{
  		rclcpp::Time now = this->now();
  		stat += "Subscriber " + std::to_string(sub1_ptr) + " " + std::to_string(msg.frame_id) + " " + std::to_string(now.nanoseconds()) + "\n";
  	}

	rclcpp::Subscription<custom_interfaces::msg::Message>::SharedPtr subscriber1;
	rclcpp::Time start_time;
};

int main(int argc, char const *argv[])
{
	unsigned int timer_period = 100;
	unsigned int experiment_duration = 10000;
	std::chrono::milliseconds executor_period = 100ms;
	bool periodic = true;
	bool exit_flag = false;

	parse_arguments(argc, argv, timer_period, experiment_duration, executor_period, periodic);

	rclcpp::init(argc, argv);
	
	// Create 1 executor for each node
	auto executor1 = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
	auto executor2 = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
	auto executor3 = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
	auto executor4 = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
	auto node1 = std::make_shared<Node1>(timer_period);
	auto node2 = std::make_shared<Node2>();
	auto node3 = std::make_shared<Node3>();
	auto node4 = std::make_shared<Node4>();
 	executor1->add_node(node1);
 	executor2->add_node(node2);
 	executor3->add_node(node3);
 	executor4->add_node(node4);
 	rclcpp::Logger logger1 = node1->get_logger();
 	rclcpp::Logger logger2 = node2->get_logger();
 	rclcpp::Logger logger3 = node3->get_logger();
 	rclcpp::Logger logger4 = node4->get_logger();
 	std::cout << "TimerID Timer1 " << node1->timer1_ptr << "\n";
 	std::cout << "SubscriberID Subscriber1 " << node2->sub1_ptr << "\n";
 	std::cout << "SubscriberID Subscriber2 " << node3->sub1_ptr << "\n";
 	std::cout << "SubscriberID Subscriber3 " << node4->sub1_ptr << "\n";

 	std::thread thread1, thread2, thread3, thread4;
 	// Create a thread for each executor
 	if (periodic)
 	{
	 	thread1 = std::thread([&]() {spin_period(executor1, executor_period, &exit_flag);});
	 	thread2 = std::thread([&]() {spin_period(executor2, executor_period, &exit_flag);});
	 	thread3 = std::thread([&]() {spin_period(executor3, executor_period, &exit_flag);});
	  thread4 = std::thread([&]() {spin_period(executor4, executor_period, &exit_flag);}); 		
 	}
 	else
 	{
 		thread1 = std::thread([&]() {executor1->spin();});
 		thread2 = std::thread([&]() {executor2->spin();});
 		thread3 = std::thread([&]() {executor3->spin();});
  	thread4 = std::thread([&]() {executor4->spin();});
 	}

    // ... and configure them accordinly as high and low prio and pin them to the
	// first CPU. Hence, the two executors compete about this computational resource.
	bool ret = configure_thread(thread1, ThreadPriority::HIGH, 0);
	std::this_thread::sleep_for(1ms);
	ret = configure_thread(thread2, ThreadPriority::HIGH, 0);
	std::this_thread::sleep_for(1ms);
	ret = configure_thread(thread3, ThreadPriority::HIGH, 0);
	std::this_thread::sleep_for(1ms);
	ret = configure_thread(thread4, ThreadPriority::HIGH, 1);
	if (!ret) {
	  RCLCPP_WARN(logger1, "Failed to configure high priority thread, are you root?");
	}

  	//RCLCPP_INFO_STREAM(
    //	logger1, "Running experiment from now on for " << EXPERIMENT_DURATION.count() << " seconds ...");
  	std::this_thread::sleep_for(experiment_duration*1ms);
  	exit_flag = true;
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