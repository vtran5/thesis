import sys
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os
import plot_utils as pu
import numpy as np
import json
from matplotlib.lines import Line2D
import matplotlib.colors as mcolors

def print_dataframe(df, df_name):
    pd.set_option('display.max_rows', None)
    pd.set_option('display.max_columns', None)
    pd.set_option('display.width', None)
    print(f"Dataframe name: {df_name}\n")
    print(df)
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

callback_chains = json_data["callback_chain"]
if not callback_chains:
    print("No 'callback_chains' data found in the JSON file.")
    sys.exit(1)

publisher_mapping = json_data["publisher_mapping"]
if not publisher_mapping:
    print("No 'publisher_mapping' data found in the JSON file.")
    sys.exit(1)

# Read the file line by line and determine the maximum number of fields
df = pu.read_input_file(input_file)

start_time = pu.find_start_time(df)
start_program_time = 0

subscriber_map = pu.find_map(df, 'Subscriber')
timer_map = pu.find_map(df, 'Timer')
executor_map = pu.find_map(df, 'Executor')
publisher_map = pu.find_map(df, 'Publisher')
timer_subscriber_map = timer_map | subscriber_map

subscriber = pu.process_dataframe(df, 'Subscriber', subscriber_map, start_time, frame_id=True)
timer = pu.process_dataframe(df, 'Timer', timer_map, start_time, frame_id=True)
#executor = pu.process_dataframe(df, 'Executor', executor_map, start_time, frame_id=False)
output_write = pu.process_dataframe(df, 'Output', publisher_map, start_time, frame_id=True)
input_read = pu.process_dataframe(df, 'Input', timer_subscriber_map, start_time, frame_id=True)
#writer = pu.process_dataframe(df, 'Writer', publisher_map, start_time, frame_id=True)
period = pu.process_dataframe(df, 'Period', executor_map, start_time, frame_id=True)
# print_dataframe(period, "period")
# print_dataframe(output_write, "output")
# print_dataframe(input_read, "input")
# Get 20 random consecutive rows from sub dataframe
#start_index = timer.sample(n=1).index[0]
# start_index = 5
# # get the 3 consecutive rows starting from the random start index
# filtered_timer = timer.iloc[start_index:start_index+20]

# Find min and max time from these random rows
# min_time = np.min(filtered_timer['Time'])
# max_time = np.max(filtered_timer['Time'])

# if start_index == 0:
#     x_min = start_program_time
# else:
#     x_min = min_time - 20
#x_min = 3600
#x_min = 7400
#x_min = 11600
x_min = 0
max_time = x_min + 5000*1000000

dataframes = {}
dataframes['filtered_timer']  = pu.get_filtered_times(timer, x_min, max_time)
dataframes['filtered_subscriber'] = pu.get_filtered_times(subscriber, x_min, max_time)
#dataframes['filtered_executor'] = pu.get_filtered_times(executor, x_min, max_time)
dataframes['filtered_output'] = pu.get_filtered_times(output_write, x_min, max_time)
dataframes['filtered_input'] = pu.get_filtered_times(input_read, x_min, max_time)
#dataframes['filtered_writer'] = pu.get_filtered_times(writer, x_min, max_time)
dataframes['filtered_period'] = pu.get_filtered_times(period, x_min, max_time)

print_dataframe(dataframes['filtered_period'], "period")
print_dataframe(dataframes['filtered_input'], "input")
print_dataframe(dataframes['filtered_output'], "output")

# executors = {
#  'Executor1': ['Timer1', 'Executor1', 'Publisher1', 'Timer2', 'Publisher2'],
#  'Executor2': ['Timer3', 'Subscriber1', 'Subscriber2', 'Executor2', 'Publisher3', 'Publisher4', 'Publisher5', 'Publisher6'],
#  'Executor3': ['Subscriber3', 'Subscriber4', 'Subscriber5', 'Subscriber6', 'Executor3', 'Publisher7', 'Publisher8', 'Publisher9', 'Publisher10'],
#  'Executor4': ['Executor4', 'Subscriber7', 'Subscriber8', 'Subscriber9', 'Subscriber10']
# }

# publisher_mapping = {
#     'Timer1' : ['Publisher1'],
#     'Timer2' : ['Publisher2'],
#     'Subscriber1' : ['Publisher3'],
#     'Subscriber2' : ['Publisher4', 'Publisher5'],
#     'Timer3' : ['Publisher6'],
#     'Subscriber3' : ['Publisher7'],
#     'Subscriber4' : ['Publisher8'],
#     'Subscriber5' : ['Publisher9'],
#     'Subscriber6' : ['Publisher10']
# }

colors = {
    'Period': '#7f7f7f',
    'Input': '#e377c2'
}

linestyle = '-'

y_positions = {
    'Output': 0.1,
    'Subscriber': 0.3,
    'Timer': 0.5,
    'Input': 0.7,
    'Period': 0.9
}

sequence = ['Output', 'Subscriber', 'Timer', 'Input', 'Period']

# Defining a darker color palette using the specified xkcd colors
darker_palette = [
    '#1f77b4',  # Blue
    '#ff7f0e',  # Orange
    '#40b440',  # Green
    '#d62728',  # Red
    '#9467bd',  # Purple
    '#8c564b',  # Brown
    '#e377c2',  # Pink
    '#7f7f7f'   # Grey
]

# Assign unique colors to each callback chain using the darker palette
color_mapping_callback_chain = {chain: darker_palette[i % len(darker_palette)] for i, chain in enumerate(callback_chains.keys())}

# Map entities to their corresponding callback chain color
# If an entity belongs to multiple chains, it will take the color of the first chain it's found in
color_mapping_entities = {}
for chain, entities in callback_chains.items():
    for entity in entities:
        if entity not in color_mapping_entities:
            color_mapping_entities[entity] = color_mapping_callback_chain[chain]

# Update the original colors dictionary with these new colors based on callback chains
colors.update(color_mapping_entities)

# Improved plotting function
def improved_plot_v2(ax, filtered_data, label, linestyle, color, y_position, custom_handles, custom_labels, frame_id=False):
    subset = filtered_data[filtered_data['ExecutorID'] == label]
    label_type = filtered_data['Keyword'].iloc[0]
    if subset.empty:
        return
    times = subset['Time'].astype(float) 
    frameIDs = subset['FrameID'] if (frame_id and 'FrameID' in subset.columns) else [None for _ in range(len(times))]
    if "Executor" in label:
        label_name = None
    else:
        label_name = label
    # For Subscriber data, we need to check if two timestamps have the same FrameID (start and end of a callback)
    if ("Subscriber" in label or "Timer" in label) and (label_type != "Input"):
        seen_frameIDs = {}
        for idx, (time, frameID) in enumerate(zip(times, frameIDs)):
            if frameID in seen_frameIDs:
                start_time = seen_frameIDs[frameID]
                ax.plot([start_time, time], [y_position, y_position], color=color, label=label_name, linewidth=4, solid_capstyle='butt')
                ax.text(start_time + 1, y_position + 0.07, '{}'.format(int(frameID)), verticalalignment='center', horizontalalignment='center', fontsize=10, color=color)
                # Remove the FrameID from the dictionary as we've plotted its duration bar
                del seen_frameIDs[frameID]
            else:
                seen_frameIDs[frameID] = time
        # Plotting remaining timestamps as lines (those without matching end timestamps)
        for frameID, remaining_time in seen_frameIDs.items():
            ax.axvline(remaining_time, color=color, linestyle=linestyle, linewidth=2, ymin=y_position - 0.05, ymax=y_position + 0.02)
            vline_legend_handle = Line2D([0], [0], color=color, linestyle='None', marker='|', markersize=10, markeredgewidth=2)
            vline_legend_label = label_name
            custom_handles.append(vline_legend_handle)
            custom_labels.append(vline_legend_label)
            ax.text(remaining_time, y_position + 0.14, '{}'.format(int(frameID)), verticalalignment='center', horizontalalignment='center', fontsize=10, color=color)
    else:
        for time in times:
            ax.axvline(time, color=color, linestyle=linestyle, linewidth=2, ymin=y_position - 0.02, ymax=y_position + 0.05)
            if label_name == None:
                continue
            vline_legend_handle = Line2D([0], [0], color=color, linestyle='None', marker='|', markersize=8, markeredgewidth=2)
            vline_legend_label = label_name
            custom_handles.append(vline_legend_handle)
            custom_labels.append(vline_legend_label)

# Plotting
fig, axs = plt.subplots(len(executors), figsize=(14, 10), sharex=True, gridspec_kw={'hspace': 0.5})

for i, (executor, labels) in enumerate(executors.items()):
    custom_handles = []
    custom_labels = []
    for label_type in sequence:
        data = dataframes[f"filtered_{label_type.lower()}"]
        for label in labels:
            if (label_type == "Period") and "Executor" in label :
                improved_plot_v2(axs[i], data, label, linestyle, colors.get(label_type, 'black'), y_positions[label_type], custom_handles, custom_labels)
            else:
                improved_plot_v2(axs[i], data, label, linestyle, colors.get(label, 'black'), y_positions[label_type], custom_handles, custom_labels, frame_id=True)


    axs[i].set_title(executor, fontsize=14)
    axs[i].set_yticks(list(y_positions.values()))
    axs[i].set_yticklabels(sequence)
    axs[i].grid(axis='x', linestyle='--', alpha=0.5)
    axs[i].set_facecolor('#F5F5F5')  # Background color for differentiation

    # Get existing handles and labels
    handles, labels = axs[i].get_legend_handles_labels()
    
    # Append custom handles and labels
    handles.extend(custom_handles)
    labels.extend(custom_labels)

    # Filter out duplicate labels while keeping the order
    unique_handles, unique_labels = [], []
    seen_labels = set()
    for handle, label in zip(handles, labels):
        if label not in seen_labels:
            seen_labels.add(label)
            unique_handles.append(handle)
            unique_labels.append(label)
            
    #axs[i].legend(unique_handles, unique_labels, loc='upper right', bbox_to_anchor=(1.1, 1), ncol=1, fontsize=10, title=executor)
    axs[i].legend(unique_handles, unique_labels, loc='center', bbox_to_anchor=(1.05, 0.5), bbox_transform=axs[i].transAxes)



def round_to_tens(n):
    return round(n / 10) * 10

for ax in axs:
    ax.set_xlim(left=x_min, right=max_time+20)
    ax.set_ylim(0, 1)  # Adjusting y-limit
    step_size = round_to_tens((max_time-x_min)/20)
    x_ticks = np.arange(round_to_tens(x_min), round_to_tens(max_time) + step_size, step_size)
    plt.xticks(x_ticks)
    ax.set_xlabel("Time (ms)", fontsize=12)

fig.suptitle("Final Enhanced Executors' Behavior Timeline", fontsize=18, y=1.05)

# Setting legend
custom_lines = [Line2D([0], [0], color=colors[key], lw=2) for key in colors.keys()]
#fig.legend(custom_lines, list(colors.keys()), loc='upper right', ncol=1, fontsize=10, title="Event Types")

fig.tight_layout()
plt.show()
