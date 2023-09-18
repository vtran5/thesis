import sys
import pandas as pd
import plot_utils as pu
import numpy as np
import json
import matplotlib.pyplot as plt

if len(sys.argv) != 5:
    print("Usage: python export_csv.py <input_file> <input_file_nolet> <json_file>")
    sys.exit(1)

input_file = sys.argv[1]
input_file_nolet = sys.argv[2]
input_file_old_let = sys.argv[3]
json_file = sys.argv[4]

# input_file = './result/automated_test_result/varied_period/data1.txt'
# input_file_nolet = './result/automated_test_result/varied_period/data1_nolet.txt'
# input_file_old_let = './result/automated_test_result/varied_period/data1_old_let.txt'
# json_file = './result/automated_test_json/varied_let/automated_test1.json'

# input_file = './result/dump.txt'
# input_file = './result/data_let_200000B.txt'
# input_file_nolet = './result/data_nolet_200000B.txt'
# json_file = './result/test1.json'

with open(json_file, 'r') as f:
    json_data = json.load(f)

# Extract executors data
executors = json_data.get('executors', {})
if not executors:
    print("No 'executors' data found in the JSON file.")
    sys.exit(1)

def find_let(df):
    start_time_df = df[df.iloc[:, 0] == 'TimerPeriod']
    start_time_df = start_time_df.dropna(axis=1)
    start_time = float(start_time_df.iloc[0, 1])
    return start_time

# callback_chains = json_data["callback_chain"]
# if not callback_chains:
#     print("No 'callback_chains' data found in the JSON file.")
#     sys.exit(1)

# publisher_mapping = json_data["publisher_mapping"]
# if not publisher_mapping:
#     print("No 'publisher_mapping' data found in the JSON file.")
#     sys.exit(1)

# Read the file line by line and determine the maximum number of fields
df = pu.read_input_file(input_file)
df_nolet = pu.read_input_file(input_file_nolet)
df_old_let = pu.read_input_file(input_file_old_let)

start_time = pu.find_start_time(df)
start_program_time = 0
callback_let = find_let(df)
print(f"TimerPeriod: {callback_let}")
subscriber_map = pu.find_map(df, 'Subscriber')
timer_map = pu.find_map(df, 'Timer')
executor_map = pu.find_map(df, 'Executor')
publisher_map = pu.find_map(df, 'Publisher')
timer_subscriber_map = timer_map | subscriber_map

# subscriber = pu.process_dataframe(df, 'Subscriber', subscriber_map, start_time, frame_id=True)
# timer = pu.process_dataframe(df, 'Timer', timer_map, start_time, frame_id=True)
# output_write = pu.process_dataframe(df, 'Output', publisher_map, start_time, frame_id=True)
# input_read = pu.process_dataframe(df, 'Input', timer_subscriber_map, start_time, frame_id=True)
period = pu.process_dataframe(df, 'Period', executor_map, start_time, frame_id=True)
# input_overhead = pu.process_dataframe(df, 'OverheadInput', executor_map, frame_id=True)
# input_taking = pu.process_dataframe(df, 'TakingInput', executor_map, start_time, frame_id=True)
let_overhead = pu.process_dataframe(df, 'OverheadLET', executor_map, frame_id=True)
# output_start = pu.process_dataframe(df, 'StartOutput', executor_map, start_time, frame_id=True)
output_write = pu.process_dataframe(df, 'OutputData', executor_map, start_time, frame_id=True)
publish_internal_overhead = pu.process_dataframe(df, 'PublishInternalOverhead', publisher_map, frame_id=False)
# publish_overhead = pu.process_dataframe(df, 'PublishOverhead', executor_map, frame_id=True)
input_triggered = pu.process_dataframe(df, 'InputTriggered', executor_map, frame_id=True)
total_overhead_input = pu.process_dataframe(df, 'OverheadTotalInput', executor_map, frame_id=False)
total_overhead_output = pu.process_dataframe(df, 'OverheadTotalOutput', executor_map, frame_id=False)
total_overhead = pu.process_dataframe(df, 'OverheadTotalOverhead', executor_map, frame_id=False)
# total_publish_overhead = pu.process_dataframe(df, 'OverheadTotalPublish', publisher_map, frame_id=False)
total_publish_internal_overhead = pu.process_dataframe(df, 'OverheadTotalInternalPublish', publisher_map, frame_id=False)
# total_trigger_overhead = pu.process_dataframe(df, 'TriggerOverhead', executor_map, frame_id=False)
# wait_thread = pu.process_dataframe(df, 'WaitThread', executor_map, frame_id=True)
# wait_all = pu.process_dataframe(df, 'WaitAll', executor_map, frame_id=True)

# Put the dataframes into a list
# dfs = [subscriber, timer, output_write, input_read, period, 
#        input_overhead, input_taking, output_overhead, output_start, output_write, 
#        publish_internal_overhead, publish_overhead, input_triggered, 


dfs = [period, let_overhead, input_triggered, output_write, 
       publish_internal_overhead, total_overhead_input,
       total_overhead_output, total_overhead, total_publish_internal_overhead]
####################
subscriber_map_nolet = pu.find_map(df_nolet, 'Subscriber')
timer_map_nolet = pu.find_map(df_nolet, 'Timer')
executor_map_nolet = pu.find_map(df_nolet, 'Executor')
publisher_map_nolet = pu.find_map(df_nolet, 'Publisher')
timer_subscriber_map_nolet = timer_map_nolet | subscriber_map_nolet
########################

# input_overhead_nolet = pu.process_dataframe(df_nolet, 'OverheadInput', executor_map_nolet, frame_id=True)
# input_triggered_nolet = pu.process_dataframe(df_nolet, 'InputTriggered', executor_map_nolet, frame_id=True)
# publish_overhead_nolet = pu.process_dataframe(df_nolet, 'PublishOverhead', publisher_map_nolet, frame_id=False)
# total_publish_overhead_nolet = pu.process_dataframe(df_nolet, 'OverheadTotalPublish', publisher_map_nolet, frame_id=False)
# total_publish_internal_overhead_nolet = pu.process_dataframe(df_nolet, 'OverheadTotalInternalPublish', publisher_map_nolet, frame_id=False)

#################
total_overhead_input_nolet = pu.process_dataframe(df_nolet, 'OverheadTotalInput', executor_map_nolet, frame_id=False)
total_overhead_output_nolet = pu.process_dataframe(df_nolet, 'OverheadTotalOutput', executor_map_nolet, frame_id=False)
total_overhead_nolet = pu.process_dataframe(df_nolet, 'OverheadTotalOverhead', executor_map_nolet, frame_id=False)
total_publish_overhead_nolet = pu.process_dataframe(df_nolet, 'OverheadTotalPublish', publisher_map_nolet, frame_id=False)

dfs_nolet = [total_overhead_input_nolet, total_overhead_output_nolet, total_overhead_nolet, total_publish_overhead_nolet]

subscriber_map_old_let = pu.find_map(df_old_let, 'Subscriber')
timer_map_old_let = pu.find_map(df_old_let, 'Timer')
executor_map_old_let = pu.find_map(df_old_let, 'Executor')
publisher_map_old_let = pu.find_map(df_old_let, 'Publisher')
timer_subscriber_map_old_let = timer_map_old_let | subscriber_map_old_let


total_overhead_input_old_let = pu.process_dataframe(df_old_let, 'OverheadTotalInput', executor_map_old_let, frame_id=False)
total_overhead_output_old_let = pu.process_dataframe(df_old_let, 'OverheadTotalOutput', executor_map_old_let, frame_id=False)
total_overhead_old_let = pu.process_dataframe(df_old_let, 'OverheadTotalOverhead', executor_map_old_let, frame_id=False)


dfs_old_let = [total_overhead_input_old_let, total_overhead_output_old_let, total_overhead_old_let]
########################3
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
def print_dataframe(df):
    pd.set_option('display.max_rows', None)
    pd.set_option('display.max_columns', None)
    pd.set_option('display.width', None)
    pd.set_option('display.float_format', '{:.0f}'.format)
    # print(f"Dataframe name: {df_name}\n")
    print(df)

def calculate_time_statistics(df):
    df.loc[:, 'Time'] = df['Time'].astype(int)
    Q1 = df['Time'].quantile(0.25)
    Q3 = df['Time'].quantile(0.75)
    IQR = Q3 - Q1
    minimum = df['Time'].min()
    median = df['Time'].median()
    maximum = df['Time'].max()
    lower_bound = Q1 - 1.5 * IQR  # Potential lower bound for outliers
    upper_bound = Q3 + 1.5 * IQR  # Potential upper bound for outliers

    # Adjust minimum and maximum if they are beyond whiskers
    whisker_min = df['Time'][df['Time'] > lower_bound].min()
    whisker_max = df['Time'][df['Time'] < upper_bound].max()

    box_plot_data = {
        'Minimum': minimum,
        'Q1': Q1,
        'Median': median,
        'Q3': Q3,
        'Maximum': maximum,
        'Whisker Minimum': whisker_min,
        'Whisker Maximum': whisker_max
    }

    return box_plot_data

def print_time_statistics(df, title):
    box_plot_data = calculate_time_statistics(df)
    print(title)
    print(box_plot_data)

def plot_one_type_overhead_one_executor(executor_id, ax, output_overhead, color):
    # Filter DataFrames based on ExecutorID
    data = output_overhead[output_overhead['ExecutorID'] == executor_id]
    data.loc[:, 'FrameID'] = data['FrameID'].astype(int)
    data.loc[:, 'Time'] = data['Time'].astype(int)
    data.loc[:, 'Time'] = data['Time']/1000

    print_time_statistics(data, f'ExecutorID: {executor_id}')
    # Plot 'Time' values with red dots for with_data and blue dots for without_data
    ax.scatter(data['FrameID'], data['Time'], color=color, zorder=1)
    
    # Labels and title
    ax.set_xlabel('Instance Number')
    ax.set_ylabel('Time (us)')
    ax.set_title(f'ExecutorID: {executor_id}')
    ax.grid(True, zorder=0)

def plot_one_type_overhead(output_overhead, title, color):
    # Get unique ExecutorIDs
    unique_executors = output_overhead['ExecutorID'].unique()
    executors = sorted(unique_executors)

    # Create 2x2 subplots
    fig, axs = plt.subplots(round((len(executors)+1)/2), 2, figsize=(12, 10))
    axs = np.atleast_2d(axs)
    # Plot scatter plot for each unique ExecutorID (using updated function)
    for i, executor_id in enumerate(executors):
        ax = axs[i // 2, i % 2]
        # ax = axs[1]
        plot_one_type_overhead_one_executor(executor_id, ax, output_overhead, color)
    plt.suptitle(title, fontsize=16)
    plt.tight_layout()

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

def split_data(data, matching_data):
    # Creating a set of tuples representing the FrameID and ExecutorID pairs in input_triggered
    common_pairs = set(tuple(row) for row in matching_data[['FrameID', 'ExecutorID']].values)

    # Function to determine if a row has a matching pair in common_pairs
    def has_matching_pair(row):
        return (row['FrameID'], row['ExecutorID']) in common_pairs

    # Applying the function to split the input_overhead DataFrame
    data_matched = data[data.apply(has_matching_pair, axis=1)]
    data_not_matched = data[~data.apply(has_matching_pair, axis=1)]
    return data_matched, data_not_matched


def plot_overhead(input_overhead, input_triggered, title):
     # Applying the function to split the input_overhead DataFrame
    input_overhead_with_data, input_overhead_without_data = split_data(input_overhead, input_triggered)
    # input_overhead_with_data = input_overhead_with_data.iloc[5:]
    # input_overhead_without_data = input_overhead_without_data.iloc[5:]
    # Get unique ExecutorIDs
    unique_executors = input_overhead['ExecutorID'].unique()
    executors = sorted(unique_executors)
    print("Without data")
    plot_one_type_overhead(input_overhead_without_data, title+" without data", 'blue')
    print("With data")
    plot_one_type_overhead(input_overhead_with_data, title+" with data", 'red')

    # Create 2x2 subplots
    fig, axs = plt.subplots(round((len(executors)+1)/2), 2, figsize=(12, 10))

    # Plot scatter plot for each unique ExecutorID (using updated function)
    for i, executor_id in enumerate(executors):
        ax = axs[i // 2, i %2]
        plot_input_overhead_one_executor(executor_id, ax, input_overhead_with_data, input_overhead_without_data)
    
    plt.suptitle(title, fontsize=16)
    plt.tight_layout()
    
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

def plot_total_input_overhead(total_overhead_input_nolet, total_overhead_input, title_nolet, title_newlet):
    total_overhead_input_nolet.loc[:, 'Time'] = total_overhead_input_nolet['Time'].astype(int)
    total_overhead_input.loc[:, 'Time'] = total_overhead_input['Time'].astype(int)
    merged_df = pd.merge(total_overhead_input_nolet[['ExecutorID', 'Time']], total_overhead_input[['ExecutorID', 'Time']], how='left', on='ExecutorID', suffixes=(title_nolet, title_newlet)).sort_values(by="ExecutorID")
    # Plotting the bar chart
    fig, ax = plt.subplots(figsize=(10, 6))

    bar_width = 0.1
    index = range(len(merged_df['ExecutorID']))

    merged_df['Difference'] = merged_df['Time'+title_newlet] - merged_df['Time'+title_nolet]
    clean_df = merged_df.dropna()
    print_dataframe(clean_df[['ExecutorID','Difference','Time'+title_newlet,'Time'+title_nolet]])
    # print(merged_df[['ExecutorID','Difference','Time'+title_newlet,'Time'+title_nolet]])
    bar = plt.bar(index, merged_df['Difference']/1000000, bar_width, label='Difference')
    bar_nolet = plt.bar([i + 2*bar_width for i in index], merged_df['Time'+title_nolet]/1000000, bar_width, label=title_nolet)
    bar_let = plt.bar([i + bar_width for i in index], merged_df['Time'+title_newlet]/1000000, bar_width, label=title_newlet)

    plt.xlabel('Executor')
    plt.ylabel('Time (ms)')
    plt.title('Comparison of total input overhead without let and with let')
    plt.xticks([i + 3*bar_width / 2 for i in index], merged_df['ExecutorID'])
    plt.legend()
    plt.tight_layout()

def calculate_output_overhead_let(executors, total_overhead_output, total_publish_internal_overhead):
    # Creating a mapping dictionary to replace ExecutorID values
    mapping_dict = {value: key for key, values in executors.items() for value in values}

    # Replacing the ExecutorID values in the DataFrames
    total_publish_internal_overhead['ExecutorID'].replace(mapping_dict, inplace=True)

    # Grouping by 'ExecutorID' and summing the 'Time' values for each DataFrame
    total_publish_internal_overhead_summed = total_publish_internal_overhead.groupby('ExecutorID')['Time'].sum().reset_index()
    # Merge the two dataframes on 'ExecutorID'
    merged = pd.merge(total_overhead_output[['ExecutorID','Time']], total_publish_internal_overhead_summed, on='ExecutorID', suffixes=('_df1', '_df2'))

    # Create a new column with the same name as the second column of the input dataframes, which is the sum of the respective columns from df1 and df2
    column_name = total_overhead_output.columns[2]
    merged[column_name] = merged[column_name + '_df1'] + merged[column_name + '_df2']

    # Keep only 'ExecutorID' and the summed values column
    result = merged[['ExecutorID', column_name]]
    return result

def calculate_output_overhead_default(executors, total_publish_overhead):
    # Creating a mapping dictionary to replace ExecutorID values
    mapping_dict = {value: key for key, values in executors.items() for value in values}

    # Replacing the ExecutorID values in the DataFrames
    total_publish_overhead['ExecutorID'].replace(mapping_dict, inplace=True)

    # Grouping by 'ExecutorID' and summing the 'Time' values for each DataFrame
    total_publish_overhead_summed = total_publish_overhead.groupby('ExecutorID')['Time'].sum().reset_index()

    return total_publish_overhead_summed

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

    bar_width = 0.15
    index = range(len(output_overhead_merged['ExecutorID']))
    output_overhead_merged['Difference'] = output_overhead_merged['Time_let'] - output_overhead_merged['Time_nolet']
    print("output overhead increase total")
    pd.set_option('display.float_format', '{:.0f}'.format)
    print(output_overhead_merged[['ExecutorID', 'Difference']])
    bar = plt.bar(index, output_overhead_merged['Difference'] /1000000, bar_width, label='Difference')
    bar_nolet = plt.bar([i + 2*bar_width for i in index], output_overhead_merged['Time_nolet']/1000000, bar_width, label='WithoutLET')
    bar_let = plt.bar([i + bar_width for i in index], output_overhead_merged['Time_let']/1000000, bar_width, label='WithLET')

    plt.xlabel('Executor')
    plt.ylabel('Time (ms)')
    plt.title('Comparison of total output overhead without let and with let')
    plt.xticks([i + 3*bar_width / 2 for i in index], output_overhead_merged['ExecutorID'])
    plt.legend()
    plt.tight_layout()



input_overhead, output_overhead = split_data(let_overhead, period)

new_total_overhead_input = input_overhead.groupby('ExecutorID')['Time'].sum().reset_index()
# print(new_total_overhead_input)
# print(total_overhead_input)
print("Input Overhead per wakeup")
plot_overhead(input_overhead, input_triggered, "Input Thread Overhead per One Run with LET")
# # # print("Input Overhead without LET")
# # # plot_overhead(input_overhead_nolet, input_triggered_nolet, "Input Thread Overhead per Executor Period without LET")
# # # #calculate_publish_overhead(publish_overhead, publish_internal_overhead)
print("Output Overhead per wakeup")
plot_overhead(output_overhead, output_write, "Output Thread Overhead per One Run with LET")
total_output_overhead = calculate_output_overhead_let(executors, total_overhead_output, total_publish_internal_overhead)
total_output_overhead_default = calculate_output_overhead_default(executors, total_publish_overhead_nolet)
print("Total input overhead vs default")
plot_total_input_overhead(total_overhead_input_nolet, new_total_overhead_input,"_overhead_input_default", "_overhead_input_newlet")
print("space")
print("Total Output overhead vs default")
plot_total_input_overhead(total_output_overhead_default, total_output_overhead,"_overhead_output_default", "_overhead_output_newlet")
print("space")
print("Total input overhead vs old let")
plot_total_input_overhead(total_overhead_input_old_let, new_total_overhead_input,"_overhead_input_oldlet", "_overhead_input_newlet")
print("space")
print("Total output overhead vs old let")
plot_total_input_overhead(total_overhead_output_old_let, total_output_overhead,"_overhead_output_oldlet", "_overhead_output_newlet")
print("space")

# plt.show()