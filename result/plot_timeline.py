import sys
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os
import plot_utils as pu
import numpy as np
import json
from matplotlib.lines import Line2D
#import os; os.chdir('/home/oem/thesis_ws/src/result'); input_file = '/home/oem/thesis_ws/src/result/temp.txt'; import plot_utils as pu; import numpy as np; import json; json_file = '/home/oem/thesis_ws/src/result/test1.json'
#os.chdir('/home/oem/thesis_ws/src/result')
#input_file = '/home/oem/thesis_ws/src/result/exp7_tp50_epdynamic_let_c.txt'
if len(sys.argv) != 3:
    print("Usage: python plot.py <input_file> <config_file>")
    sys.exit(1)

input_file = sys.argv[1]
json_file = sys.argv[2]
with open(json_file, 'r') as f:
    json_data = json.load(f)

# Extract executors data
executors = json_data.get('executors', {})
if not executors:
    print("No 'executors' data found in the JSON file.")
    sys.exit(1)

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
publisher = pu.process_dataframe(df, 'Publisher', publisher_map, start_time, frame_id=True)
listener = pu.process_dataframe(df, 'Listener', executor_map, start_time, frame_id=True)
writer = pu.process_dataframe(df, 'Writer', publisher_map, start_time, frame_id=True)

# Get 20 random consecutive rows from sub dataframe
#start_index = timer.sample(n=1).index[0]
start_index = 0
# get the 3 consecutive rows starting from the random start index
filtered_timer = timer.iloc[start_index:start_index+12]

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
filtered_writer = pu.get_filtered_times(writer, x_min, max_time)

#executors = {
#    'Executor1': ['Timer1', 'Executor1', 'Publisher1', 'Timer2', 'Publisher4'],
#    'Executor2': ['Subscriber1', 'Executor2', 'Publisher2', 'Subscriber4', 'Publisher5'],
#    'Executor3': ['Subscriber2', 'Executor3', 'Publisher3', 'Subscriber5', 'Publisher6'],
#    'Executor4': ['Subscriber3', 'Executor4', 'Subscriber6']
#}

data_mapping = {
    'Timer': [{'data': filtered_timer, 'line_style': '--', 'color': 'cyan', 'frame_id': True}],
    'Subscriber': [{'data': filtered_subscriber, 'line_style': '--', 'color': 'cyan', 'frame_id': True}],
    'Executor': [
        {'data': filtered_executor, 'line_style': ':', 'color': 'red'},
        {'data': filtered_listener, 'line_style': '-.', 'color': 'green'}
    ],
    'Publisher': [
        {'data': filtered_publisher, 'line_style': ':', 'color': 'black'},
#        {'data': filtered_writer, 'line_style': '-.', 'color': 'black'}
    ]
}

fig, axs = plt.subplots(len(executors), sharex=False)

for i, (executor, labels) in enumerate(executors.items()):
    # Filter data based on executor
    filtered_data_mapping = {}
    for data_type, data_details_list in data_mapping.items():
        for data_details in data_details_list:
            filtered_data = data_details['data'][data_details['data']['ExecutorID'].isin(labels)]
            filtered_data_mapping.setdefault(data_type, []).append({**data_details, 'data': filtered_data})
    
    for label in labels:
        # Extract type from the label
        label_type = ''.join(filter(str.isalpha, label))

        if label_type in filtered_data_mapping:
            properties_list = filtered_data_mapping[label_type]

            for properties in properties_list:
                data = properties['data']
                line_style = properties['line_style']
                color = properties['color']
                frame_id = properties.get('frame_id', False)

                pu.plot_filtered_data(axs[i], data, label, line_style, color, frame_id=frame_id)

    axs[i].set_title(executor)

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