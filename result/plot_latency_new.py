import sys
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import os
import json
import plot_utils as pu
from matplotlib.lines import Line2D

def print_dataframe(df, df_name):
    pd.set_option('display.max_rows', None)
    pd.set_option('display.max_columns', None)
    pd.set_option('display.width', None)
    print(f"Dataframe name: {df_name}\n")
    print(df)

def adjust_frame_id_ms(df, df_receive, period):
    missed_period = 0
    for i in range(1, len(df)):
        duration = df.at[i, 'Time'] - df.at[i-1, 'Time']
        if not (period - 21 <= duration <= period + 21):
            missed_period = round(duration/period) - 1
            for j in range(i, len(df)):
                df.at[j, 'FrameID'] += missed_period
                if j < df_receive.shape[0]:
                    df_receive.at[j, 'FrameID'] += missed_period
    
    return df, df_receive

def adjust_frame_id(publish, receive):
    publish_ms = publish.copy()
    publish_ms['Time'] = publish_ms['Time']/1000000
    receive_ms = receive.copy()
    receive_ms['Time'] = receive_ms['Time']/1000000
    period = publish_ms.at[1, 'Time'] - publish_ms.at[0, 'Time']
    new_publish_ms, new_receive_ms = adjust_frame_id_ms(publish_ms, receive_ms, period)
    publish['FrameID'] = new_publish_ms['FrameID']
    receive['FrameID'] = new_receive_ms['FrameID']
    return publish, receive

def get_start_time_difference(df, callback_chains, executors, start_time, simulation=False):
    time_difference = []
    executor_map = pu.find_map(df, 'Executor')
    if simulation:
        period = pu.process_dataframe(df, 'Period', executor_map, start_time, frame_id=False)
    else:
        period = pu.process_dataframe(df, 'Period', executor_map, start_time, frame_id=True)

    for chain, entities in callback_chains.items():
        first_entity = entities[0]
        last_entity = entities[-1]
        first_entity_executor = next((executor for executor, items in executors.items() if first_entity in items), None)
        last_entity_executor = next((executor for executor, items in executors.items() if last_entity in items), None)
        start_chain_time = period[period['ExecutorID'] == first_entity_executor]['Time'].iloc[0]
        end_chain_time = period[period['ExecutorID'] == last_entity_executor]['Time'].iloc[0]
        time_difference.append(end_chain_time-start_chain_time)

    return time_difference

def get_latencies(df, callback_chains, start_time, timer_map, subscriber_map, publisher_map=None, simulation=False):
    latencies = []
    timer_sub_map = subscriber_map | timer_map
    subscriber = pu.process_dataframe(df, 'Subscriber', subscriber_map, start_time, frame_id=True)
    timer = pu.process_dataframe(df, 'Timer', timer_map, start_time, frame_id=True)

    for _, (chain_name, chain) in enumerate(callback_chains.items()):
        if not simulation:
            input = pu.process_dataframe(df, 'Input', timer_sub_map, start_time, frame_id=True)
            output = pu.process_dataframe(df, 'Output', publisher_map, start_time, frame_id=True)
            publisher = output[output['ExecutorID'] == chain[1]].reset_index(drop=True)
            publish_time = publisher[['FrameID', 'Time']]
            receive_time = input[input['ExecutorID'] == chain[-1]][['FrameID', 'Time']].reset_index(drop=True)
            publish_time['FrameID'] = publish_time['FrameID'].astype(int)
            receive_time['FrameID'] = receive_time['FrameID'].astype(int)
            #print(publish_time)
            #print(receive_time)
            publish_time, receive_time = adjust_frame_id(publish_time, receive_time)
        else:
            publish_time = timer[timer['ExecutorID'] == chain[0]][['FrameID', 'Time']].reset_index(drop=True)
            receive_time = subscriber[subscriber['ExecutorID'] == chain[-1]][['FrameID', 'Time']].reset_index(drop=True)
            #print(publish_time)
            #print(receive_time)

        merged_df = pd.merge(publish_time, receive_time, how='left', on='FrameID', suffixes=('_publish', '_receive'))
        merged_df['latency'] = merged_df['Time_receive'] - merged_df['Time_publish']
        merged_df = merged_df.dropna()
        latency = merged_df[['FrameID', 'latency']]
        latency.columns = ['frame','latency']
        print(chain_name)
        print(latency)
        #print_dataframe(latency, "latency")
        latencies.append(latency)

    return latencies

def process_data(input_file, json_data, simulation=False):
    callback_chains = json_data["callback_chain"]
    executors = json_data["executors"]
    df = pu.read_input_file(input_file)
    start_time = pu.find_start_time(df)
    subscriber_map = pu.find_map(df, 'Subscriber')
    timer_map = pu.find_map(df, 'Timer')

    publisher_map = None
    if not simulation:
        publisher_map = pu.find_map(df, 'Publisher')
    time_difference = get_start_time_difference(df, callback_chains, executors, start_time, simulation)
    latencies = get_latencies(df, callback_chains, start_time, timer_map, subscriber_map, publisher_map, simulation)
    return time_difference, latencies


def combined_plot(input_file1, input_file2, json_data):
    callback_chains = json_data["callback_chain"]
    time_difference1, latencies_1 = process_data(input_file1, json_data, simulation=False) if input_file1 else []
    time_difference2, latencies_2 = process_data(input_file2, json_data, simulation=True) if input_file2 else [],[]
    print(time_difference1)
    print(time_difference2)
    #print(latencies_2[0])
    #print(latencies_1[0])

    fig, axs = plt.subplots(max(len(latencies_1), len(latencies_2)), 1, figsize=(10, max(len(latencies_1), len(latencies_2))*5))
    axs = np.array(axs).flatten()

    for idx, (chain_name, _) in enumerate(callback_chains.items()):
        print(chain_name)       
        latency1 = latencies_1[idx]    
        latency1['frame'] = latency1['frame'].astype(int)

        if latencies_2:
            latency2 = latencies_2[idx]
            latency2['frame'] = latency2['frame'].astype(int)
            latency2['latency'] = latency2['latency'] - (time_difference2[idx] - time_difference1[idx])
            error_latency = latency1.merge(latency2, on='frame', suffixes=('_actual', '_simulated'))
            error_latency['error'] = error_latency['latency_actual'] - error_latency['latency_simulated']
            avg_error = error_latency['error'].mean()
            residual_error = error_latency
            residual_error['error'] = error_latency['error'] - avg_error
            max_error = error_latency['error'].max()
            min_error = error_latency['error'].min()
            error_text = f"Average Error: {round(avg_error)} ns\nMax Error: {round(max_error)} ns\nMin Error: {round(min_error)} ns"
            print(error_text)
            axs[idx].annotate(error_text, xy=(1.0, 0.0), xycoords='axes fraction',
               fontsize=10, xytext=(-5, 5), textcoords='offset points',
               bbox=dict(boxstyle="square", fc="white", alpha=0.5),
               ha="right")


        latency1['latency'] = latency1['latency']/1000000
        axs[idx].scatter(latency1['frame'], latency1['latency'], color='blue', marker='o', s=40)
        if latencies_2:
            latency2['latency'] = latency2['latency']/1000000
            axs[idx].scatter(latency2['frame'], latency2['latency'], color='orange', marker='x', s=40)
        #median_latency, lower_bound, upper_bound, equal_percentage = pu.calculate_latency_range(latency)
        #pu.plot_latency(axs[idx], latency, median_latency, lower_bound, upper_bound, equal_percentage, color='orange', marker='x')
        
        
        # axs[idx].scatter(residual_error['frame'], residual_error['error'], color='orange', marker='x', s=40)
        #print("Simulation Data")
        #print(f"Median latency: {median_latency:.2f}")
        #print(f"Range (±{equal_percentage * 100:.0f}%): {lower_bound:.2f} - {upper_bound:.2f}")
        #axs[idx].scatter(latency['FrameID'], latency['latency'], color='red', label='Type 2', marker='x')
        #axs[idx].legend()
        # latency1['frame'] = latency1['frame'].astype(float)
        # latency['frame'] = latency['frame'].astype(float)
        
        # # Calculate error for each frame
        
        # # Calculate average, max and min error
        # avg_error = error_latency['error'].mean()
        # residual_error = error_latency['error'] - avg_error
        #max_error = residual_error['error'].max()
        #min_error = residual_error['error'].min()

        # # Add the text box with avg, max and min error values
        #


        median_latency, lower_bound, upper_bound, equal_percentage = pu.calculate_latency_range(latency1)
        
        #pu.plot_latency(axs[idx], latency, median_latency, lower_bound, upper_bound, equal_percentage, color='blue', marker='o')
        #axs[idx].scatter(latency['FrameID'], latency['latency'], color='blue', label='Type 1', marker='o')
        #axs[idx].legend(loc='lower left', bbox_to_anchor=(1.1, 0.5), bbox_transform=axs[idx].transAxes)
        
        print("Actual Data")
        print(f"Median latency: {median_latency:.2f}")
        print(f"Range (±{equal_percentage * 100:.0f}%): {lower_bound:.2f} - {upper_bound:.2f}")
        x_min = latency1['frame'].min()-1
        x_max = latency1['frame'].max()+1
        axs[idx].set_xlim(x_min, x_max)
        #axs[idx].set_xticks(np.arange(x_min, x_max + 1, round((x_max-x_min)/50)))
        axs[idx].set_title(chain_name)
        axs[idx].axhline(median_latency, color='r', linestyle='--', label=f"Median latency: {median_latency:.2f} ms")
        axs[idx].axhline(lower_bound, color='g', linestyle=':', label=f"Range (±{equal_percentage * 100:.0f}%): {lower_bound:.2f} ms - {upper_bound:.2f} ms")
        axs[idx].axhline(upper_bound, color='g', linestyle=':')
        axs[idx].set_ylabel('End-to-end latency (ms)')
        axs[idx].set_xlabel('Chain number')
        axs[idx].set_ylim(bottom=round(lower_bound*0.5))
        axs[idx].set_ylim(top=round(upper_bound*1.1))
        axs[idx].legend(loc='lower left')

    # Create custom legend entries
    legend_elements = [
    Line2D([0], [0], color='blue', marker='o', linestyle='', label='Actual data'),
    Line2D([0], [0], color='orange', marker='x', linestyle='', label='Simulated data')
    ]
    fig.legend(handles=legend_elements, loc='upper right')
    plt.tight_layout()
    plt.subplots_adjust(hspace = 0.3)
    plt.show()

if __name__ == "__main__":
    # if len(sys.argv) < 3 or len(sys.argv) > 4:
    #     print("Usage: python combined_plot.py <config_file> <input_file2> [<input_file1>]")
    #     sys.exit(1)
    # json_file = sys.argv[1]
    # input_file1 = sys.argv[2]
    # input_file2 = sys.argv[3] if len(sys.argv) == 4 else None
    json_file = './result/test1.json'
    input_file1 = './result/temp3_data.txt'
    #input_file2 = './result/simulation_test1.txt'
    with open(json_file, 'r') as f:
        json_data = json.load(f)
    
    combined_plot(input_file1, None, json_data)