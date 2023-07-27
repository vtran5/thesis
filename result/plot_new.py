import sys
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import os
import json
import plot_utils as pu
def print_dataframe(df, df_name):
    pd.set_option('display.max_rows', None)
    pd.set_option('display.max_columns', None)
    pd.set_option('display.width', None)
    print(f"Dataframe name: {df_name}\n")
    print(df)

def get_latencies(df, callback_chains, start_time, timer_map, subscriber_map, publisher_map=None, simulation=False):
    latencies = []
    
    subscriber = pu.process_dataframe(df, 'Subscriber', subscriber_map, start_time, frame_id=True)
    timer = pu.process_dataframe(df, 'Timer', timer_map, start_time, frame_id=True)

    for _, (chain_name, chain) in enumerate(callback_chains.items()):
        if not simulation:
            output = pu.process_dataframe(df, 'Output', publisher_map, start_time, frame_id=True)
            publisher = output[output['ExecutorID'] == chain[1]].reset_index(drop=True)
            #publish_time = timer[timer['ExecutorID'] == chain[0]][['FrameID', 'Time']]
            #publish_time = publish_time.iloc[1::2].reset_index(drop=True)
            #publish_time['Time'] = publisher['Time']
            publish_time = publisher[['FrameID', 'Time']]
        else:
            publish_time = timer[timer['ExecutorID'] == chain[0]][['FrameID', 'Time']].reset_index(drop=True)
        
        receive_time = subscriber[subscriber['ExecutorID'] == chain[-1]][['FrameID', 'Time']].reset_index(drop=True)
        merged_df = pd.merge(publish_time, receive_time, how='left', on='FrameID', suffixes=('_publish', '_receive'))
        merged_df['latency'] = merged_df['Time_receive'] - merged_df['Time_publish']
        merged_df = merged_df.dropna()
        latency = merged_df[['FrameID', 'latency']]
        latency.columns = ['frame','latency']
        latencies.append(latency)

    return latencies

def process_data(input_file, callback_chains, simulation=False):
    df = pu.read_input_file(input_file)
    start_time = pu.find_start_time(df)
    subscriber_map = pu.find_map(df, 'Subscriber')
    timer_map = pu.find_map(df, 'Timer')
    publisher_map = None
    if not simulation:
        publisher_map = pu.find_map(df, 'Publisher')

    return get_latencies(df, callback_chains, start_time, timer_map, subscriber_map, publisher_map, simulation)


def combined_plot(input_file1, input_file2, callback_chains):
    latencies_1 = process_data(input_file1, callback_chains, simulation=False) if input_file1 else []
    latencies_2 = process_data(input_file2, callback_chains, simulation=True) if input_file2 else []

    fig, axs = plt.subplots(max(len(latencies_1), len(latencies_2)), 1, figsize=(10, max(len(latencies_1), len(latencies_2))*5))
    axs = np.array(axs).flatten()

    for idx, (chain_name, _) in enumerate(callback_chains.items()):
        print(chain_name)

        if idx < len(latencies_1):
            latency = latencies_1[idx]
            median_latency, lower_bound, upper_bound, equal_percentage = pu.calculate_latency_range(latency)
            pu.plot_latency(axs[idx], latency, median_latency, lower_bound, upper_bound, equal_percentage, color='blue', marker='o')
            #axs[idx].scatter(latency['FrameID'], latency['latency'], color='blue', label='Type 1', marker='o')
            print("Actual Data")
            print(f"Median actual latency: {median_latency:.2f}")
            print(f"Range (±{equal_percentage * 100:.0f}%): {lower_bound:.2f} - {upper_bound:.2f}")

        if idx < len(latencies_2):
            latency = latencies_2[idx]
            median_latency, lower_bound, upper_bound, equal_percentage = pu.calculate_latency_range(latency)
            pu.plot_latency(axs[idx], latency, median_latency, lower_bound, upper_bound, equal_percentage, color='orange', marker='x')
            print("Simulation Data")
            print(f"Median simulation latency: {median_latency:.2f}")
            print(f"Range (±{equal_percentage * 100:.0f}%): {lower_bound:.2f} - {upper_bound:.2f}")
            #axs[idx].scatter(latency['FrameID'], latency['latency'], color='red', label='Type 2', marker='x')
            #axs[idx].legend()

        axs[idx].set_title(chain_name)

    plt.tight_layout()
    plt.subplots_adjust(hspace = 0.4)
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) < 3 or len(sys.argv) > 4:
        print("Usage: python combined_plot.py <config_file> <input_file2> [<input_file1>]")
        sys.exit(1)
    json_file = sys.argv[1]
    input_file1 = sys.argv[2]
    input_file2 = sys.argv[3] if len(sys.argv) == 4 else None
    with open(json_file, 'r') as f:
        json_data = json.load(f)
    callback_chains = json_data["callback_chain"]
    combined_plot(input_file1, input_file2, callback_chains)
