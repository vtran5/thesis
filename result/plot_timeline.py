import sys
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os
import plot_utils as pu
import numpy as np
from matplotlib.lines import Line2D

if len(sys.argv) != 2:
    print("Usage: python plot.py <input_file>")
    sys.exit(1)

input_file = sys.argv[1]

# Read the file line by line and determine the maximum number of fields
df = pu.read_input_file(input_file)

start_time = pu.find_start_time(df)
start_program_time = pu.find_program_start_time(df)
start_program_time = (start_program_time - start_time)/1000000

subscriber_map = pu.find_map(df, 'Subscriber')
timer_map = pu.find_map(df, 'Timer')
executor_map = pu.find_map(df, 'Executor')
publisher_map = pu.find_map(df, 'Publisher')

subscriber = pu.process_dataframe(df, 'Subscriber', subscriber_map, start_time, frame_id=True)
timer = pu.process_dataframe(df, 'Timer', timer_map, start_time, frame_id=True)
executor = pu.process_dataframe(df, 'Executor', executor_map, start_time, frame_id=False)
publisher = pu.process_dataframe(df, 'Publisher', publisher_map, start_time, frame_id=False)
listener = pu.process_dataframe(df, 'Listener', executor_map, start_time, frame_id=False)
reader = pu.process_dataframe(df, 'Reader', executor_map, start_time, frame_id=False)

# Get 20 random consecutive rows from sub dataframe
#start_index = timer.sample(n=1).index[0]
start_index = 24
# get the 3 consecutive rows starting from the random start index
filtered_timer = timer.iloc[start_index:start_index+6]

# Find min and max time from these random rows
min_time = np.min(filtered_timer['Time'])
max_time = np.max(filtered_timer['Time'])
if start_index == 0:
    x_min = start_program_time
else:
    x_min = min_time - 20
filtered_subscriber = pu.get_filtered_times(subscriber, x_min, max_time)
filtered_executor = pu.get_filtered_times(executor, x_min, max_time)
filtered_publisher = pu.get_filtered_times(publisher, x_min, max_time)
filtered_listener = pu.get_filtered_times(listener, x_min, max_time)
filtered_reader = pu.get_filtered_times(reader, x_min, max_time)

executor1 = ['Timer1', 'Executor1', 'Publisher1']
executor2 = ['Subscriber1', 'Executor2', 'Publisher2']
executor3 = ['Subscriber2', 'Executor3', 'Publisher3']
executor4 = ['Subscriber3', 'Executor4']
filtered_subscriber2 = filtered_subscriber[filtered_subscriber['ExecutorID'].isin(executor2)]
filtered_subscriber3 = filtered_subscriber[filtered_subscriber['ExecutorID'].isin(executor3)]
filtered_subscriber4 = filtered_subscriber[filtered_subscriber['ExecutorID'].isin(executor4)]

filtered_executor1 = filtered_executor[filtered_executor['ExecutorID'].isin(executor1)]
filtered_executor2 = filtered_executor[filtered_executor['ExecutorID'].isin(executor2)]
filtered_executor3 = filtered_executor[filtered_executor['ExecutorID'].isin(executor3)]
filtered_executor4 = filtered_executor[filtered_executor['ExecutorID'].isin(executor4)]

filtered_publisher1 = filtered_publisher[filtered_publisher['ExecutorID'].isin(executor1)]
filtered_publisher2 = filtered_publisher[filtered_publisher['ExecutorID'].isin(executor2)]
filtered_publisher3 = filtered_publisher[filtered_publisher['ExecutorID'].isin(executor3)]

filtered_listener2 = filtered_listener[filtered_listener['ExecutorID'].isin(executor2)]
filtered_listener3 = filtered_listener[filtered_listener['ExecutorID'].isin(executor3)]
filtered_listener4 = filtered_listener[filtered_listener['ExecutorID'].isin(executor4)]

filtered_reader2 = filtered_reader[filtered_reader['ExecutorID'].isin(executor2)]
filtered_reader3 = filtered_reader[filtered_reader['ExecutorID'].isin(executor3)]
filtered_reader4 = filtered_reader[filtered_reader['ExecutorID'].isin(executor4)]

fig, axs = plt.subplots(4, sharex=False)
pu.plot_filtered_data(axs[0], filtered_publisher1, 'Publisher1', ':', 'black')
pu.plot_filtered_data(axs[0], filtered_timer, 'Timer1', '--', 'cyan', frame_id=True)
#pu.plot_filtered_data(axs[0], filtered_executor1, 'Executor1', ':', 'red')
axs[0].set_title('Executor 1')

pu.plot_filtered_data(axs[1], filtered_listener2, 'Executor2', '-.', 'green')
pu.plot_filtered_data(axs[1], filtered_reader2, 'Executor2', '-.', 'black')
pu.plot_filtered_data(axs[1], filtered_publisher2, 'Publisher2', ':', 'black')
pu.plot_filtered_data(axs[1], filtered_subscriber2, 'Subscriber1', '--', 'cyan', frame_id=True)
#pu.plot_filtered_data(axs[1], filtered_executor2, 'Executor2', ':', 'red')
axs[1].set_title('Executor 2')

pu.plot_filtered_data(axs[2], filtered_listener3, 'Executor3', '-.', 'green')
pu.plot_filtered_data(axs[2], filtered_reader3, 'Executor3', '-.', 'black')
pu.plot_filtered_data(axs[2], filtered_publisher3, 'Publisher3', ':', 'black')
pu.plot_filtered_data(axs[2], filtered_subscriber3, 'Subscriber2', '--', 'cyan', frame_id=True)
#pu.plot_filtered_data(axs[2], filtered_executor3, 'Executor3', ':', 'red')
axs[2].set_title('Executor 3')

pu.plot_filtered_data(axs[3], filtered_listener4, 'Executor4', '-.', 'green')
pu.plot_filtered_data(axs[3], filtered_subscriber4, 'Subscriber3', '--', 'cyan', frame_id=True)
#pu.plot_filtered_data(axs[3], filtered_executor4, 'Executor4', ':', 'red')
axs[3].set_title('Executor 4')

print(f"Start Program at: {start_program_time:.2f}")

for ax in axs:
    ax.set_xlim(left=x_min)
    ax.set_xlim(right=max_time+20)

# Creating custom lines
custom_lines = [Line2D([0], [0], color='black', lw=2, linestyle=':'),
                Line2D([0], [0], color='cyan', lw=2, linestyle='--'),
                Line2D([0], [0], color='green', lw=2, linestyle='-.'), 
                Line2D([0], [0], color='black', lw=2, linestyle='-.')]

fig.legend(custom_lines, ['Publisher', 'Callback', 'Reader', 'Writer'], loc='upper right')

fig.subplots_adjust(hspace = 0.5)
figure_name = os.path.splitext(input_file)[0] + '_timeline.png'
plt.savefig(figure_name)
plt.show()