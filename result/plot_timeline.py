import sys
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os
import plot_utils as pu
import numpy as np

if len(sys.argv) != 2:
    print("Usage: python plot.py <input_file>")
    sys.exit(1)

input_file = sys.argv[1]

# Read the file line by line and determine the maximum number of fields
df = pu.read_input_file(input_file)

start_time = pu.find_start_time(df)

subscriber_map = pu.find_map(df, 'Subscriber')
timer_map = pu.find_map(df, 'Timer')

subscriber = pu.process_dataframe(df, 'Subscriber', subscriber_map, start_time, frame_id=True)
timer = pu.process_dataframe(df, 'Timer', timer_map, start_time, frame_id=True)


# Get 20 random consecutive rows from sub dataframe
start_index = timer.sample(n=1).index[0]
#start_index = 0
# get the 3 consecutive rows starting from the random start index
filtered_timer = timer.iloc[start_index:start_index+10]

# Find min and max time from these random rows
min_time = np.min(filtered_timer['Time'])
max_time = np.max(filtered_timer['Time'])
filtered_subscriber = pu.get_filtered_times(subscriber, min_time, max_time)
executor2 = ['Subscriber1']
executor3 = ['Subscriber2']
executor4 = ['Subscriber3']
filtered_subscriber2 = filtered_subscriber[filtered_subscriber['ExecutorID'].isin(executor2)]
filtered_subscriber3 = filtered_subscriber[filtered_subscriber['ExecutorID'].isin(executor3)]
filtered_subscriber4 = filtered_subscriber[filtered_subscriber['ExecutorID'].isin(executor4)]

fig, axs = plt.subplots(4, sharex=False)

pu.plot_filtered_data(axs[0], filtered_timer, 'Timer1', '--', 'k')
axs[0].set_title('Executor 1')
pu.plot_filtered_data(axs[1], filtered_subscriber2, 'Subscriber1', '--', 'cyan')
axs[1].set_title('Executor 2')
pu.plot_filtered_data(axs[2], filtered_subscriber3, 'Subscriber2', '--', 'magenta')
axs[2].set_title('Executor 3')
pu.plot_filtered_data(axs[3], filtered_subscriber4, 'Subscriber3', '--', 'yellow')
axs[3].set_title('Executor 4')

for ax in axs:
    ax.set_xlim(left=min_time-20)
    ax.set_xlim(right=max_time+20)

fig.subplots_adjust(hspace = 0.5)
figure_name = os.path.splitext(input_file)[0] + '_timeline.png'
plt.savefig(figure_name)
plt.show()