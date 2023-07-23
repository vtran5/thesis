import pandas as pd
import matplotlib.pyplot as plt
import math

def read_input_file(input_file):
    max_fields = 0
    with open(input_file, 'r') as f:
        for line in f:
            fields = len(line.split(' '))
            max_fields = max(max_fields, fields)

    column_names = [i for i in range(0, max_fields)]
    df = pd.read_csv(input_file, sep=' ', header=None, names=column_names, engine='python')
    return df

def find_map(df, keyword):
    id_df = df[df.iloc[:, 0] == f"{keyword}ID"]
    id_df = id_df.dropna(axis=1).drop(df.columns[0], axis=1)
    id_df.columns = [f"{keyword}Name",f"{keyword}ID"]
    id_map = id_df.set_index(f"{keyword}ID")[f"{keyword}Name"].to_dict()
    id_map = {int(key): value for key, value in id_map.items()}
    id_map = {str(key): value for key, value in id_map.items()}
    return id_map

def find_start_time(df):
    start_time_df = df[df.iloc[:, 0] == 'StartTime']
    start_time_df = start_time_df.dropna(axis=1)
    start_time = float(start_time_df.iloc[0, 1])
    return start_time

def find_program_start_time(df):
    start_time_df = df[df.iloc[:, 0] == 'StartProgram']
    start_time_df = start_time_df.dropna(axis=1)
    start_time = float(start_time_df.iloc[0, 1])
    return start_time

def process_dataframe(df, keyword, keyword_map=None, start_time=None, frame_id=False):
    filtered_df = df[df.iloc[:, 0] == keyword]
    filtered_df = filtered_df.dropna(axis=1)

    if keyword == 'Frame':
        filtered_df = filtered_df.reset_index(drop=True)
        filtered_df = filtered_df.drop(filtered_df.columns[0], axis=1)

        column_count = filtered_df.shape[1]
        column_names = ['frame'] + [str(i) for i in range(1, column_count - 1)] + ['latency']
        filtered_df.columns = column_names

    else:
        if frame_id:
            assert filtered_df.shape[1] == 4, f"DataFrame doesn't have the correct number of columns (4). It has {filtered_df.shape[1]} columns"
            col_names = ['Keyword', 'ExecutorID','FrameID', 'Time']
        else:
            assert filtered_df.shape[1] == 3, f"DataFrame doesn't have the correct number of columns (3). It has {filtered_df.shape[1]} columns"
            col_names = ['Keyword', 'ExecutorID', 'Time']

        filtered_df.columns = col_names
        if start_time is not None:
            filtered_df['Time'] = pd.to_numeric(filtered_df['Time'])
            filtered_df['Time'] = (filtered_df['Time'] - start_time) / 1000000
        if keyword_map is not None:
            filtered_df['ExecutorID'] = filtered_df['ExecutorID'].replace(keyword_map)
        #filtered_df = filtered_df.drop(filtered_df.columns[0], axis=1)
        filtered_df = filtered_df.round(1)

    filtered_df = filtered_df.reset_index(drop=True)
    filtered_df = filtered_df.sort_values(by='Time')
    return filtered_df


def get_filtered_times(df, min_time, max_time):
    return df[(df['Time'] >= min_time) & (df['Time'] <= max_time)]

def plot_filtered_data(ax, filtered_data, label, linestyle, color, frame_id=False):
    subset = filtered_data[filtered_data['ExecutorID'] == label]
    times = subset['Time']
    frameIDs = subset['FrameID'] if frame_id else ["" for _ in range(len(times))]

    vertical_position = {'Subscriber': 0, 'Timer': 0.2, 'Executor': 0.4, 'Publisher': 0.6, 'Listener': 0.8, 'Writer': 0.8}
    value = vertical_position[filtered_data.iloc[0, 0]]  # Get the value from the first column

    for time, frameID in zip(times, frameIDs):
        ax.axvline(time, color=color, linestyle=linestyle, linewidth=2, ymin=value, ymax=value + 0.2)
        if frame_id:  # Add this check
            ax.text(time, 1, '{}'.format(int(frameID)), verticalalignment='center')  # Cast to int here
        else:
            ax.text(time, 1, frameID, verticalalignment='center')

    ax.set_xticks(times.round())
    ax.tick_params(axis='x', labelbottom=True)
    for label in ax.get_xticklabels():
        label.set_rotation(30)

def calculate_latency_range(df):
    median_latency = df['latency'].median()
    min_latency = df['latency'].min()
    max_latency = df['latency'].max()

    lower_percentage = (median_latency - min_latency) / median_latency
    upper_percentage = (max_latency - median_latency) / median_latency

    if math.isnan(lower_percentage):
        print("Warning: lower_percentage is NaN. Check your data.")
        # Set it to some default value or handle it in a different way, as per your logic
        lower_percentage = 0.0

    if math.isnan(upper_percentage):
        print("Warning: lower_percentage is NaN. Check your data.")
        # Set it to some default value or handle it in a different way, as per your logic
        upper_percentage = 0.0
            
    rounded_lower_percentage = math.ceil(lower_percentage * 100) / 100
    rounded_upper_percentage = math.ceil(upper_percentage * 100) / 100
    equal_percentage = max(rounded_lower_percentage, rounded_upper_percentage)

    lower_bound = median_latency * (1 - equal_percentage)
    upper_bound = median_latency * (1 + equal_percentage)

    return median_latency, lower_bound, upper_bound, equal_percentage


def plot_latency(ax, df, median_latency, lower_bound, upper_bound, equal_percentage, title):
    ax.scatter(df['frame'], df['latency'])
    ax.axhline(median_latency, color='r', linestyle='--', label=f"Median latency: {median_latency:.2f}")
    ax.axhline(lower_bound, color='g', linestyle=':', label=f"Range from median (±{equal_percentage * 100:.0f}%): {lower_bound:.2f} - {upper_bound:.2f}")
    ax.axhline(upper_bound, color='g', linestyle=':')
    ax.set_ylabel('End-to-end latency (ms)')
    ax.set_xlabel('Chain number')
    ax.set_ylim(bottom=100)
    ax.set_ylim(top=400)
    ax.legend(loc='lower left')
    ax.set_title(title)