This folder contains the experiments to test the performance of the rclc executor performance. The experiments are:
1. exp_cb_execute_order.c: This experiment along with exp_cb_execute_order.cpp in the rclcpp_exp folder shows the differences in callback execution order between the C executor and C++ executor.
2. exp_trigger_all.c: This experiment along with exp_trigger_all.cpp in the rclcpp_exp folder demonstrates the benefit of using the trigger condition. By setting the trigger condition to ALL, the high frequency callback is not called every time it is triggered as in exp_trigger_all.cpp.
3. TBD
4. exp_single_thread_single_core.c: This experiment demonstrates the benefit of using LET semantics in the current C executor as the end-to-end latency jitter is very low.
5. exp_multi_thread_single_core.c: This experiment demonstrates the problem of the LET semantics in the current C executor as the end-to-end latency jitter is very high in multi-threaded application.
6. exp_single_thread_multi_core.c: This experiment also demonstrates the problem of the LET semantics in the current C executor as the end-to-end latency jitter is very high in multi-core application.
7. exp_multi_thread_single_core_let.c: This experiment shows the effect of the newly implemented LET feature in C executor.

In experiment 4,5,6,7, the user can input the experience duration with -ed, the timer period with -tp, the executor period with -ep, and the semantics with -let. For example, the following set the experiment duration to 10000 ms, timer period to 100 ms, executor period to 20 ms and the let semantics to true:
		
		ros2 run rclc_exp exp4 -ed 10000 -tp 100 -let true -ep 20