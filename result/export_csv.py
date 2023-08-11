import sys
import pandas as pd
import plot_utils as pu
import numpy as np
import json
import matplotlib.pyplot as plt

input_file = './result/temp1_data.txt'
input_file_nolet = './result/temp1_data_nolet.txt'
json_file = './result/test1.json'

# input_file = './result/automated_test_result/data1.txt'
# input_file_nolet = './result/automated_test_result/data1_nolet.txt'
# json_file = './result/automated_test_json/automated_test1.json'

with open(json_file, 'r') as f:
    json_data = json.load(f)

# Extract executors data
executors = json_data.get('executors', {})
print(len(executors))
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
df_nolet = pu.read_input_file(input_file_nolet)

start_time = pu.find_start_time(df)
start_program_time = 0

subscriber_map = pu.find_map(df, 'Subscriber')
timer_map = pu.find_map(df, 'Timer')
executor_map = pu.find_map(df, 'Executor')
publisher_map = pu.find_map(df, 'Publisher')
timer_subscriber_map = timer_map | subscriber_map

subscriber = pu.process_dataframe(df, 'Subscriber', subscriber_map, start_time, frame_id=True)
timer = pu.process_dataframe(df, 'Timer', timer_map, start_time, frame_id=True)
output_write = pu.process_dataframe(df, 'Output', publisher_map, start_time, frame_id=True)
input_read = pu.process_dataframe(df, 'Input', timer_subscriber_map, start_time, frame_id=True)
period = pu.process_dataframe(df, 'Period', executor_map, start_time, frame_id=True)
input_overhead = pu.process_dataframe(df, 'OverheadInput', executor_map, frame_id=True)
input_taking = pu.process_dataframe(df, 'TakingInput', executor_map, start_time, frame_id=True)
output_overhead = pu.process_dataframe(df, 'OverheadOutput', executor_map, frame_id=True)
output_start = pu.process_dataframe(df, 'StartOutput', executor_map, start_time, frame_id=True)
output_write = pu.process_dataframe(df, 'OutputData', executor_map, start_time, frame_id=True)
publish_internal_overhead = pu.process_dataframe(df, 'PublishInternalOverhead', publisher_map, frame_id=False)
publish_overhead = pu.process_dataframe(df, 'PublishOverhead', executor_map, frame_id=True)
input_triggered = pu.process_dataframe(df, 'InputTriggered', executor_map, frame_id=True)
total_overhead_input = pu.process_dataframe(df, 'OverheadTotalInput', executor_map, frame_id=False)
total_overhead_output = pu.process_dataframe(df, 'OverheadTotalOutput', executor_map, frame_id=False)
total_publish_overhead = pu.process_dataframe(df, 'OverheadTotalPublish', publisher_map, frame_id=False)
total_publish_internal_overhead = pu.process_dataframe(df, 'OverheadTotalInternalPublish', publisher_map, frame_id=False)
# wait_thread = pu.process_dataframe(df, 'WaitThread', executor_map, frame_id=True)
# wait_all = pu.process_dataframe(df, 'WaitAll', executor_map, frame_id=True)

subscriber_map_nolet = pu.find_map(df_nolet, 'Subscriber')
timer_map_nolet = pu.find_map(df_nolet, 'Timer')
executor_map_nolet = pu.find_map(df_nolet, 'Executor')
publisher_map_nolet = pu.find_map(df_nolet, 'Publisher')
timer_subscriber_map_nolet = timer_map_nolet | subscriber_map_nolet

subscriber_nolet = pu.process_dataframe(df_nolet, 'Subscriber', subscriber_map_nolet, start_time, frame_id=True)
timer_nolet = pu.process_dataframe(df_nolet, 'Timer', timer_map_nolet, start_time, frame_id=True)
output_write_nolet = pu.process_dataframe(df_nolet, 'Output', publisher_map_nolet, start_time, frame_id=True)
input_read_nolet = pu.process_dataframe(df_nolet, 'Input', timer_subscriber_map_nolet, start_time, frame_id=True)
period_nolet = pu.process_dataframe(df_nolet, 'Period', executor_map_nolet, start_time, frame_id=True)
input_overhead_nolet = pu.process_dataframe(df_nolet, 'OverheadInput', executor_map_nolet, frame_id=True)
input_taking_nolet = pu.process_dataframe(df_nolet, 'TakingInput', executor_map_nolet, start_time, frame_id=True)
publish_internal_overhead_nolet = pu.process_dataframe(df_nolet, 'PublishInternalOverhead', publisher_map_nolet, frame_id=False)
publish_overhead_nolet = pu.process_dataframe(df_nolet, 'PublishOverhead', publisher_map_nolet, frame_id=False)
input_read_nolet = pu.process_dataframe(df_nolet, 'ReadInput', executor_map_nolet, frame_id=True)
total_overhead_input_nolet = pu.process_dataframe(df_nolet, 'OverheadTotalInput', executor_map_nolet, frame_id=False)
total_overhead_output_nolet = pu.process_dataframe(df_nolet, 'OverheadTotalOutput', executor_map_nolet, frame_id=False)
total_publish_overhead_nolet = pu.process_dataframe(df_nolet, 'OverheadTotalPublish', publisher_map_nolet, frame_id=False)
total_publish_internal_overhead_nolet = pu.process_dataframe(df_nolet, 'OverheadTotalInternalPublish', publisher_map_nolet, frame_id=False)
  
# Put the dataframes into a list
dfs = [subscriber, timer, output_write, input_read, period, 
       input_overhead, input_taking, output_overhead, output_start, output_write, 
       publish_internal_overhead, publish_overhead, input_triggered, total_overhead_input,
       total_overhead_output, total_publish_overhead, total_publish_internal_overhead]

dfs_nolet = [subscriber_nolet, timer_nolet, output_write_nolet, input_read_nolet, period_nolet, 
       input_overhead_nolet, input_taking_nolet, output_write_nolet, 
       publish_internal_overhead_nolet, publish_overhead_nolet, input_read_nolet, total_overhead_input_nolet,
        total_overhead_output_nolet, total_publish_overhead_nolet, total_publish_internal_overhead_nolet]

# dfs = [subscriber, output_overhead]
# Find the maximum number of rows among the dataframes
max_rows = max(df.shape[0] for df in dfs)

# Resize dataframes to have max_rows, filling with NaNs if necessary
#dfs = [df.reindex(range(max_rows)) for df in dfs]

# Concatenate the DataFrames side by side
df_final = pd.concat(dfs, axis=1)

# # Define the path where you want to save the file
my_path = "./result/data.csv"

# Write DataFrame to CSV
#df_final.to_csv(my_path, index=False)
# with open(my_path, 'w') as file:
#     for df in dfs:
#         df.to_csv(file, index=False)
#         file.write('---\n')
#     for df in dfs_nolet:
#         df.to_csv(file, index=False)
#         file.write('---\n')

# Function to plot the scatter plot for each ExecutorID (updated to use FrameID on x-axis)
def plot_input_overhead_one_executor(executor_id, ax, input_overhead_with_data, input_overhead_without_data):
    # Filter DataFrames based on ExecutorID
    with_data = input_overhead_with_data[input_overhead_with_data['ExecutorID'] == executor_id]
    without_data = input_overhead_without_data[input_overhead_without_data['ExecutorID'] == executor_id]
    with_data.loc[:, 'FrameID'] = with_data['FrameID'].astype(int)
    without_data.loc[:, 'FrameID'] = without_data['FrameID'].astype(int)
    with_data.loc[:, 'Time'] = with_data['Time'].astype(int)
    without_data.loc[:, 'Time'] = without_data['Time'].astype(int)
    with_data.loc[:, 'Time'] = with_data['Time']/1000
    without_data.loc[:, 'Time'] = without_data['Time']/1000    

    # Plot 'Time' values with red dots for with_data and blue dots for without_data
    ax.scatter(with_data['FrameID'], with_data['Time'], color='red', label='With Data', zorder=1)
    ax.scatter(without_data['FrameID'], without_data['Time'], color='blue', label='Without Data', zorder=1)
    
    # Labels and title
    ax.set_xlabel('Period Number')
    ax.set_ylabel('Time (us)')
    ax.set_title(f'ExecutorID: {executor_id}')
    ax.legend()
    ax.grid(True, zorder=0)

def plot_input_overhead(input_overhead, input_triggered, text):
    # Creating a set of tuples representing the FrameID and ExecutorID pairs in input_triggered
    common_pairs = set(tuple(row) for row in input_triggered[['FrameID', 'ExecutorID']].values)

    # Function to determine if a row has a matching pair in common_pairs
    def has_matching_pair(row):
        return (row['FrameID'], row['ExecutorID']) in common_pairs

    # Applying the function to split the input_overhead DataFrame
    input_overhead_with_data = input_overhead[input_overhead.apply(has_matching_pair, axis=1)]
    input_overhead_without_data = input_overhead[~input_overhead.apply(has_matching_pair, axis=1)]

    # Get unique ExecutorIDs
    unique_executors = input_overhead['ExecutorID'].unique()
    executors = sorted(unique_executors)

    # Create 2x2 subplots
    fig, axs = plt.subplots(round(len(executors)/2), 2, figsize=(12, 10))

    # Plot scatter plot for each unique ExecutorID (using updated function)
    for i, executor_id in enumerate(executors):
        ax = axs[i // 2, i %2]
        plot_input_overhead_one_executor(executor_id, ax, input_overhead_with_data, input_overhead_without_data)

    plt.tight_layout()
    plt.suptitle(text, fontsize=16)


def calculate_time_statistics(df):
    df.loc[:, 'Time'] = df['Time'].astype(int)
    mean_time = df['Time'].mean()
    min_time = df['Time'].min()
    max_time = df['Time'].max()
    return mean_time, min_time, max_time

def calculate_publish_overhead(publish_overhead, publish_internal_overhead):
    publish_overhead.loc[:, 'Time'] = publish_overhead['Time'].astype(int)
    publish_internal_overhead.loc[:, 'Time'] = publish_internal_overhead['Time'].astype(int)

    publish_without_data_overhead = publish_overhead[publish_overhead['Time'] < 5000]
    publish_with_data_overhead = publish_overhead[publish_overhead['Time'] >= 5000]

    mean_time_internal, min_time_internal, max_time_internal = calculate_time_statistics(publish_internal_overhead)
    mean_time_publish_data, min_time_publish_data, max_time_publish_data = calculate_time_statistics(publish_with_data_overhead)
    mean_time_publish_no_data, min_time_publish_no_data, max_time_publish_no_data = calculate_time_statistics(publish_without_data_overhead)

    print("Publish with data overhead:")
    print(f"Mean: {round(mean_time_publish_data)} ns")
    print(f"Min: {min_time_publish_data} ns")
    print(f"Max: {max_time_publish_data} ns")
    print("Publish with no data overhead:")
    print(f"Mean: {round(mean_time_publish_no_data)} ns")
    print(f"Min: {min_time_publish_no_data} ns")
    print(f"Max: {max_time_publish_no_data} ns")
    print("Publish internal overhead:")
    print(f"Mean: {round(mean_time_internal)} ns")
    print(f"Min: {min_time_internal} ns")
    print(f"Max: {max_time_internal} ns")

# Function to plot the scatter plot for each ExecutorID (updated to use FrameID on x-axis)
def plot_output_overhead_one_executor(executor_id, ax, output_overhead_with_data, output_overhead_without_data):
    # Filter DataFrames based on ExecutorID
    with_data = output_overhead_with_data[output_overhead_with_data['ExecutorID'] == executor_id]
    without_data = output_overhead_without_data[output_overhead_without_data['ExecutorID'] == executor_id]
    with_data.loc[:, 'FrameID'] = with_data['FrameID'].astype(int)
    without_data.loc[:, 'FrameID'] = without_data['FrameID'].astype(int)
    with_data.loc[:, 'Time'] = with_data['Time'].astype(int)
    without_data.loc[:, 'Time'] = without_data['Time'].astype(int)
    with_data.loc[:, 'Time'] = with_data['Time']/1000
    without_data.loc[:, 'Time'] = without_data['Time']/1000

    # Plot 'Time' values with red dots for with_data and blue dots for without_data
    ax.scatter(with_data['FrameID'], with_data['Time'], color='red', label='With Data', zorder=1)
    ax.scatter(without_data['FrameID'], without_data['Time'], color='blue', label='Without Data', zorder=1)
    
    # Labels and title
    ax.set_xlabel('Instance Number')
    ax.set_ylabel('Time (us)')
    ax.set_title(f'ExecutorID: {executor_id}')
    ax.legend()
    ax.grid(True, zorder=0)

def plot_output_overhead(output_overhead, output_write):
    # Creating a set of tuples representing the FrameID and ExecutorID pairs in input_triggered
    common_pairs = set(tuple(row) for row in output_write[['FrameID', 'ExecutorID']].values)

    # Function to determine if a row has a matching pair in common_pairs
    def has_matching_pair(row):
        return (row['FrameID'], row['ExecutorID']) in common_pairs

    # Applying the function to split the input_overhead DataFrame
    output_overhead_with_data = output_overhead[output_overhead.apply(has_matching_pair, axis=1)]
    output_overhead_without_data = output_overhead[~output_overhead.apply(has_matching_pair, axis=1)]

    # Get unique ExecutorIDs
    unique_executors = output_overhead['ExecutorID'].unique()
    executors = sorted(unique_executors)

    # Create 2x2 subplots
    fig, axs = plt.subplots(round(len(executors)/2), 2, figsize=(12, 10))

    # Plot scatter plot for each unique ExecutorID (using updated function)
    for i, executor_id in enumerate(executors):
        ax = axs[i // 2, i % 2]
        plot_output_overhead_one_executor(executor_id, ax, output_overhead_with_data, output_overhead_without_data)
    plt.suptitle('Output Overhead', fontsize=16)
    plt.tight_layout()

def plot_total_input_overhead(total_overhead_input_nolet, total_overhead_input):
    total_overhead_input_nolet.loc[:, 'Time'] = total_overhead_input_nolet['Time'].astype(int)
    total_overhead_input.loc[:, 'Time'] = total_overhead_input['Time'].astype(int)
    merged_df = pd.merge(total_overhead_input_nolet[['ExecutorID', 'Time']], total_overhead_input[['ExecutorID', 'Time']], how='left', on='ExecutorID', suffixes=('_nolet', '_let')).sort_values(by="ExecutorID")
    # Plotting the bar chart
    fig, ax = plt.subplots(figsize=(10, 6))

    bar_width = 0.35
    index = range(len(merged_df['ExecutorID']))

    bar_nolet = plt.bar(index, merged_df['Time_nolet']/1000000, bar_width, label='WithoutLET')
    bar_let = plt.bar([i + bar_width for i in index], merged_df['Time_let']/1000000, bar_width, label='WithLET')

    plt.xlabel('Executor')
    plt.ylabel('Time (ms)')
    plt.title('Comparison of total input overhead without let and with let')
    plt.xticks([i + bar_width / 2 for i in index], merged_df['ExecutorID'])
    plt.legend()
    plt.tight_layout()

def plot_total_output_overhead(executors, total_overhead_output, total_publish_overhead, total_publish_internal_overhead, total_publish_overhead_nolet):
    # Creating a mapping dictionary to replace ExecutorID values
    mapping_dict = {value: key for key, values in executors.items() for value in values}

    # Replacing the ExecutorID values in the DataFrames
    total_publish_internal_overhead['ExecutorID'].replace(mapping_dict, inplace=True)
    total_publish_overhead_nolet['ExecutorID'].replace(mapping_dict, inplace=True)
    total_publish_overhead['ExecutorID'].replace(mapping_dict, inplace=True)

    # Grouping by 'ExecutorID' and summing the 'Time' values for each DataFrame
    total_publish_internal_overhead_summed = total_publish_internal_overhead.groupby('ExecutorID')['Time'].sum().reset_index()
    total_publish_overhead_nolet_summed = total_publish_overhead_nolet.groupby('ExecutorID')['Time'].sum().reset_index()
    total_publish_overhead_summed = total_publish_overhead.groupby('ExecutorID')['Time'].sum().reset_index()

    # Merging total_overhead_output_summed with total_publish_internal_overhead_summed on 'ExecutorID' using a left join
    merged_df = total_overhead_output.merge(total_publish_internal_overhead_summed, on='ExecutorID', how='left', suffixes=('_output', '_internal'))

    # Filling NaN values with 0 for the 'Time_internal' column
    merged_df['Time_internal'].fillna(0, inplace=True)

    # Calculating the new 'Time' values by summing 'Time_output' and 'Time_internal'
    merged_df['Time'] = merged_df['Time_output'] + merged_df['Time_internal']

    # Keeping only the 'ExecutorID' and new 'Time' columns
    output_overhead_merged = pd.merge(total_publish_overhead_nolet_summed[['ExecutorID', 'Time']], merged_df[['ExecutorID', 'Time']], how='right', on='ExecutorID', suffixes=('_nolet', '_let')).sort_values(by="ExecutorID")
    output_overhead_merged['Time_nolet'].fillna(0, inplace=True)
    fig, ax = plt.subplots(figsize=(10, 6))

    bar_width = 0.35
    index = range(len(output_overhead_merged['ExecutorID']))

    bar_nolet = plt.bar(index, output_overhead_merged['Time_nolet']/1000000, bar_width, label='WithoutLET')
    bar_let = plt.bar([i + bar_width for i in index], output_overhead_merged['Time_let']/1000000, bar_width, label='WithLET')

    plt.xlabel('Executor')
    plt.ylabel('Time (ms)')
    plt.title('Comparison of total output overhead without let and with let')
    plt.xticks([i + bar_width / 2 for i in index], output_overhead_merged['ExecutorID'])
    plt.legend()
    plt.tight_layout()

plot_input_overhead(input_overhead, input_triggered, "Input Overhead")
calculate_publish_overhead(publish_overhead, publish_internal_overhead)
plot_output_overhead(output_overhead, output_write)
plot_total_input_overhead(total_overhead_input_nolet, total_overhead_input)
plot_total_output_overhead(executors, total_overhead_output, total_publish_overhead, total_publish_internal_overhead, total_publish_overhead_nolet)
plt.show()