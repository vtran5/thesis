import sys
import pandas as pd
import plot_utils as pu
import matplotlib.pyplot as plt
import os
import numpy as np

if len(sys.argv) != 2:
    print("Usage: python plot.py <input_file>")
    sys.exit(1)

input_file = sys.argv[1]
df = pu.read_input_file(input_file)

start_time = pu.find_start_time(df)

publisher_map = pu.find_map(df, 'Publisher')
subscriber_map = pu.find_map(df, 'Subscriber')
executor_map = pu.find_map(df, 'Executor')

publisher = pu.process_dataframe(df, 'Publisher', publisher_map, start_time, frame_id=True)
subscriber = pu.process_dataframe(df, 'Subscriber', subscriber_map, start_time, frame_id=True)
executor = pu.process_dataframe(df, 'Executor', executor_map, start_time)

# Get 20 random consecutive rows from sub dataframe
start_index = subscriber.sample(n=1).index[0]
#start_index = 0
# get the 3 consecutive rows starting from the random start index
filtered_subscriber = subscriber.iloc[start_index:start_index+80]

# Find min and max time from these random rows
min_time = np.min(filtered_subscriber['Time'])
max_time = np.max(filtered_subscriber['Time'])
filtered_publisher = pu.get_filtered_times(publisher, min_time, max_time)
filtered_executor = pu.get_filtered_times(executor, min_time, max_time)
executor2 = ['Subscriber1','Subscriber2','Subscriber3']
executor3 = ['Subscriber4','Subscriber5','Subscriber6']
filtered_subscriber2 = filtered_subscriber[filtered_subscriber['ExecutorID'].isin(executor2)]
filtered_subscriber3 = filtered_subscriber[filtered_subscriber['ExecutorID'].isin(executor3)]
# Function to plot timeline
def plot_timeline(df, executor_color_map, ax, keyword):
    unique_executors = df["ExecutorID"].unique()
    # Create a mapping of executors to integers
    executor_to_int = {executor: i for i, executor in enumerate(unique_executors)}

    for executor in unique_executors:
        executor_df = df[df["ExecutorID"] == executor]
        color = executor_color_map[executor]
        times = executor_df["Time"]

        ax.scatter(times, [executor]*len(times), color=color)

        for i in range(len(executor_df) - 1):
            if keyword == 'Publisher':
                ax.text(executor_df.iloc[i]["Time"], executor_to_int[executor]+0.2, executor_df.iloc[i]["FrameID"], ha='center')
            elif keyword == 'Subscriber':
                if executor_df.iloc[i]["FrameID"] == executor_df.iloc[i+1]["FrameID"]:
                    mid_time = (executor_df.iloc[i]["Time"] + executor_df.iloc[i+1]["Time"]) / 2
                    ax.text(mid_time, executor_to_int[executor], executor_df.iloc[i]["FrameID"], ha='center')


# Mapping of executors to colors
executor_color_map = {"Publisher1": "red", "Publisher2": "blue", "Publisher3": "green",
                      "Subscriber1": "red", "Subscriber2": "blue", "Subscriber3": "green",
                      "Subscriber4": "red", "Subscriber5": "blue", "Subscriber6": "green",
                      "Executor2": "cyan", "Executor3": "yellow"}

fig, axs = plt.subplots(3, sharex=True)

# Plot publisher timeline
plot_timeline(filtered_publisher, executor_color_map, axs[0], 'Publisher')
axs[0].set_title('Executor 1')

# Plot subscriber timeline
plot_timeline(filtered_subscriber2, executor_color_map, axs[1], 'Subscriber')
axs[0].set_title('Executor 2')
plot_timeline(filtered_subscriber3, executor_color_map, axs[2], 'Subscriber')
axs[0].set_title('Executor 3')

#sub1 = subscriber[subscriber.iloc[:,0] == "Subscriber3"]
#data = sub1['FrameID']
#data = data.reset_index(drop=True)
#plt.plot(data)
plt.show()
