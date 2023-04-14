#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <random>

#include "rclcpp/rclcpp.hpp"
#include "custom_interfaces/msg/message.hpp"
#include "thread_utilities.hpp"

static std::random_device rd;
static std::mt19937 gen(rd());

using std::placeholders::_1;
using namespace std::chrono_literals;
using namespace my_app;

class MyNode : public rclcpp::Node
{
public:
	MyNode(std::string node_name) : Node(node_name){};
	void print_statistics()
	{
		print_timestamp(timestamp);
	}

private:
	void print_timestamp(std::vector<std::vector<int64_t>>& timestamp) 
	{
	    const size_t m = timestamp.size();
	    const size_t n = timestamp[0].size();

	    for (size_t i = 0; i < n; i++) {
	        bool printZero = (i == 0); // print the first element of the first line even if it's 0
	        std::string line = "";
	        line += "frame " + std::to_string(i) + ": ";
	        for (size_t j = 0; j < m; j++) {
	            if (timestamp[j][i] == 0 && !printZero)
	                break;
	            printZero = false;
	            line += std::to_string(timestamp[j][i]) + " ";
	        }
	        line += "\n";
	        RCLCPP_INFO(this->get_logger(), line.c_str());
	    }
	}

protected:
	// min_time and max_time are in miliseconds
	void busy_wait(int min_time, int max_time) 
	{
		// Use the high_resolution_clock to get a precise timepoint
		std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();

		// Use the random_device to seed the random number generator
		
		std::uniform_int_distribution<> dist(min_time, max_time);

		// Calculate the random wait time
		int wait_time = dist(gen);

		// Busy wait for the random amount of time
		while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count() < wait_time) {
		  // Do nothing
		}
	}

	std::vector<std::vector<int64_t>> timestamp;
};

class Node1 : public MyNode
{
public:
	Node1() : MyNode("node1"), count(0), first_run(1)
	{
   		// Initialize the vector with 4 vectors of size 100 filled with 0's.
    	timestamp = std::vector<std::vector<int64_t>>(4, std::vector<int64_t>(40, 0));

		publisher1 = this->create_publisher<custom_interfaces::msg::Message>("topic1", 10);
		publisher2 = this->create_publisher<custom_interfaces::msg::Message>("topic2", 10);
		subscriber1 = this->create_subscription<custom_interfaces::msg::Message>(
			"topic1", 10, std::bind(&Node1::sub1_callback, this, _1));
		subscriber2 = this->create_subscription<custom_interfaces::msg::Message>(
			"topic2", 10, std::bind(&Node1::sub2_callback, this, _1));
		timer = this->create_wall_timer(500ms, std::bind(&Node1::timer_callback, this));
	}

private:
	void timer_callback()
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
		message.stamp = start_time.nanoseconds();
		timestamp.at(0).at(message.frame_id) = (now - start_time).nanoseconds()/1000;
		//RCLCPP_INFO(this->get_logger(), "Publishing: '%s' at %f", message.header.frame_id.c_str(), t);
		publisher1->publish(message);
	}
	
	void sub1_callback(const custom_interfaces::msg::Message & msg)
  	{
  		rclcpp::Time now = this->now();
  		timestamp.at(1).at(msg.frame_id) = (now - start_time).nanoseconds()/1000;
    	//RCLCPP_INFO(this->get_logger(), "I heard: '%s' at %f", msg.header.frame_id.c_str(), t);
    	//msg.header.stamp = now - start_time;
    	busy_wait(100,300);
    	publisher2->publish(msg);
    	now = this->now();
    	timestamp.at(2).at(msg.frame_id) = (now - start_time).nanoseconds()/1000;
  	}

  	void sub2_callback(const custom_interfaces::msg::Message & msg)
  	{
  		rclcpp::Time now = this->now();
  		timestamp.at(3).at(msg.frame_id) = (now - start_time).nanoseconds()/1000;
    	//RCLCPP_INFO(this->get_logger(), "I heard: '%s' at %f", msg.header.frame_id.c_str(), t);
  	}

	rclcpp::TimerBase::SharedPtr timer;
	rclcpp::Publisher<custom_interfaces::msg::Message>::SharedPtr publisher1;
	rclcpp::Publisher<custom_interfaces::msg::Message>::SharedPtr publisher2;
	rclcpp::Subscription<custom_interfaces::msg::Message>::SharedPtr subscriber1;
	rclcpp::Subscription<custom_interfaces::msg::Message>::SharedPtr subscriber2;
	rclcpp::Time start_time;
	int first_run;
	size_t count;
};

class Node2 : public MyNode
{
public:
	Node2() : MyNode("node2"), count(0), first_run(1)
	{
   		// Initialize the vector with 4 vectors of size 100 filled with 0's.
    	timestamp = std::vector<std::vector<int64_t>>(1, std::vector<int64_t>(40, 0));

		subscriber1 = this->create_subscription<custom_interfaces::msg::Message>(
			"topic1", 10, std::bind(&Node2::sub1_callback, this, _1));
	}

private:
	void sub1_callback(const custom_interfaces::msg::Message & msg)
  	{
  		rclcpp::Time now = this->now();
  		timestamp.at(0).at(msg.frame_id) = (now.nanoseconds() - msg.stamp)/1000;
    	//RCLCPP_INFO(this->get_logger(), "I heard: '%s' at %f", msg.header.frame_id.c_str(), t);
    	//msg.header.stamp = now - start_time;
    	busy_wait(100,150);
  	}

	rclcpp::Subscription<custom_interfaces::msg::Message>::SharedPtr subscriber1;
	rclcpp::Time start_time;
	int first_run;
	size_t count;
};

int main(int argc, char const *argv[])
{
	rclcpp::init(argc, argv);
	
	// Create executors
	rclcpp::executors::SingleThreadedExecutor high_prio_executor;
	rclcpp::executors::SingleThreadedExecutor low_prio_executor;
	auto node1 = std::make_shared<Node1>();
	auto node2 = std::make_shared<Node2>();
  	high_prio_executor.add_node(node2);
  	low_prio_executor.add_node(node1);
  	rclcpp::Logger logger1 = node1->get_logger();
  	rclcpp::Logger logger2 = node2->get_logger();
  	// Create a thread for each executor
  	auto high_prio_thread = std::thread([&]() 
  	{
      high_prio_executor.spin();
    });
    auto low_prio_thread = std::thread([&]() 
  	{
      low_prio_executor.spin();
    });
    // ... and configure them accordinly as high and low prio and pin them to the
	// first CPU. Hence, the two executors compete about this computational resource.
	const int CPU_ZERO = 0;
	bool ret = configure_thread(high_prio_thread, ThreadPriority::HIGH, CPU_ZERO);
	ret = configure_thread(low_prio_thread, ThreadPriority::LOW, CPU_ZERO);
	if (!ret) {
	  RCLCPP_WARN(logger1, "Failed to configure high priority thread, are you root?");
	}

	const std::chrono::seconds EXPERIMENT_DURATION = 10s;
  	RCLCPP_INFO_STREAM(
    	logger1, "Running experiment from now on for " << EXPERIMENT_DURATION.count() << " seconds ...");
  	std::this_thread::sleep_for(EXPERIMENT_DURATION);
  	
  	rclcpp::shutdown();
  	high_prio_thread.join();
  	low_prio_thread.join();
  	node1->print_statistics();
  	node2->print_statistics();
	return 0;
}