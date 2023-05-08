#include "my_node_utilities.hpp"
#include <random>
#include "thread_utilities.hpp"

static std::random_device rd;
static std::mt19937 gen(rd());

using namespace my_thread_utilities;

MyNode::MyNode(std::string node_name) : Node(node_name){};

void MyNode::print_statistics()
{
	print_timestamp(timestamp);
}

void MyNode::print_timestamp(std::vector<std::vector<int64_t>>& timestamp) 
{
    const size_t m = timestamp.size();
    const size_t n = timestamp[0].size();
    bool end_of_file = false;
    std::string node_name(MyNode::get_name());
    for (size_t i = 0; i < n; i++) {
        std::string line = "";
        line += node_name + " " + std::to_string(i) + " ";
        for (size_t j = 0; j < m; j++) {
            if (j == 0 && timestamp[j][i] == 0)
            {
            	int sum = 0;
            	for (size_t k = 0; k < m; k++)
            		sum += timestamp[k][i];
            	if (sum == 0 && i > 0)
            	{
            		end_of_file = true;
            		break;
            	}
            }
            line += std::to_string(timestamp[j][i]) + " ";
        }
        if(end_of_file)
        	break;
        line += "\n";
        std::cout << line << std::flush;
    }
}

void MyNode::busy_wait(std::chrono::milliseconds duration)
{
	if (duration > std::chrono::nanoseconds::zero()) {
	  auto end_time = get_current_thread_time() + duration;
	  int x = 0;
	  bool do_again = true;
	  while (do_again) {
	  	x++;
	    do_again = (get_current_thread_time() < end_time);
	  }
	}	
}

void MyNode::busy_wait_random(std::chrono::milliseconds min_time, std::chrono::milliseconds max_time) 
{
	// Use the random_device to seed the random number generator
	std::uniform_int_distribution<> dist(min_time.count(), max_time.count());
	
	// Calculate the random wait time
	std::chrono::milliseconds wait_time{dist(gen)};
	
	// Busy wait for the random amount of time
	MyNode::busy_wait(wait_time);
}
