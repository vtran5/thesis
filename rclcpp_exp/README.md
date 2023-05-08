This folder contains the experiments to test the performance of the rclcpp executor performance. The experiments are:
1. exp_cb_execute_order.cpp: This experiment along with exp_cb_execute_order.c in the rclc_exp folder shows the differences in callback execution order between the C executor and C++ executor.
2. exp_trigger_all.cpp: This experiment along with exp_trigger_all.c in the rclc_exp folder demonstrates the benefit of using the trigger condition in the C executor. Here the trigger condition is ANY, so the high frequency callback is called every time it is triggered.
3. TBD
4. exp_single_thread_single_core.cpp: This experiment demonstrates the unpredictability of the C++ executor as the end-to-end latency jitter is high.
5. exp_multi_thread_single_core.cpp: This experiment demonstrates the unpredictability of the C++ executor as the end-to-end latency jitter is very high.

In experiment 4,5, the user can input the experience duration with -ed, and the timer period with -tp. For example, the following set the experiment duration to 10000 ms, and timer period to 100 ms:
		
		ros2 run rclcpp_exp exp4 -ed 10000 -tp 100