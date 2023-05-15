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

def process_dataframe(df, keyword, keyword_map=None, start_time=None):
    filtered_df = df[df.iloc[:, 0] == keyword]
    filtered_df = filtered_df.dropna(axis=1)

    if keyword == 'Frame':
        filtered_df = filtered_df.reset_index(drop=True)
        filtered_df = filtered_df.drop(filtered_df.columns[0], axis=1)

        column_count = filtered_df.shape[1]
        column_names = ['frame'] + [str(i) for i in range(1, column_count - 1)] + ['latency']
        filtered_df.columns = column_names

    else:
        col_names = ['Keyword', 'ExecutorID', 'Time']
        filtered_df.columns = col_names
        if start_time is not None:
            filtered_df['Time'] = pd.to_numeric(filtered_df['Time'])
            filtered_df['Time'] = (filtered_df['Time'] - start_time) / 1000000
        if keyword_map is not None:
            filtered_df['ExecutorID'] = filtered_df['ExecutorID'].replace(keyword_map)
        filtered_df = filtered_df.drop(filtered_df.columns[0], axis=1)
        filtered_df = filtered_df.round(1)

    return filtered_df

def get_filtered_times(df, min_time, max_time):
    return df[(df['Time'] >= min_time) & (df['Time'] <= max_time)]

def plot_filtered_data(ax, filtered_data, node, linestyle, color):
    times = filtered_data[filtered_data['ExecutorID'] == node]['Time']
    for time in times:
        ax.axvline(time, color=color, linestyle=linestyle, linewidth=2)

def plot_timeline(data, figure_name, filtered_executor=None, filtered_publisher=None, filtered_listener=None, filtered_writer=None):
    fig, ax = plt.subplots(4, figsize=(10, 6), sharex=True)

    for i, node in enumerate(['Executor1', 'Executor2', 'Executor3', 'Executor4']):
        ax[i].set_title(node)
        ax[i].set_yticks([])

        if filtered_executor is not None:
            plot_filtered_data(ax[i], filtered_executor, node, '--', 'k')
        if filtered_publisher is not None:
            plot_filtered_data(ax[i], filtered_publisher, node, '-.', 'cyan')
        if filtered_listener is not None:
            plot_filtered_data(ax[i], filtered_listener, node, ':', 'magenta')
        if filtered_writer is not None:
            plot_filtered_data(ax[i], filtered_writer, node, '--', 'yellow')
        
        if node == 'Executor1':
            for val in data['1']:
                ax[i].axvline(val, color='b', linestyle='-', linewidth=2)
                ax[i].text(val, 0.5, f"{val:.1f}", ha='center', va='bottom', fontsize=8)
        elif node == 'Executor2':
            for index, row in data.iterrows():
                ax[i].axvspan(row['2'], row['3'], color='r', alpha=0.3)
                ax[i].text(row['2'], 0.5, f"{row['2']:.1f}", ha='center', va='bottom', fontsize=8)
                ax[i].text(row['3'], 0.5, f"{row['3']:.1f}", ha='center', va='bottom', fontsize=8)
        elif node == 'Executor3':
            for index, row in data.iterrows():
                ax[i].axvspan(row['4'], row['5'], color='g', alpha=0.3)
                ax[i].text(row['4'], 0.5, f"{row['4']:.1f}", ha='center', va='bottom', fontsize=8)
                ax[i].text(row['5'], 0.5, f"{row['5']:.1f}", ha='center', va='bottom', fontsize=8)
        elif node == 'Executor4':
            for val in data['latency']:
                ax[i].axvline(val, color='m', linestyle='-', linewidth=2)
                ax[i].text(val, 0.5, f"{val:.1f}", ha='center', va='bottom', fontsize=8)

    plt.xlabel("Time (microseconds)")
    plt.tight_layout()

    plt.savefig(figure_name)

    plt.show()

def calculate_latency_range(df):
    df['latency'] = df['latency'] / 1000
    median_latency = df['latency'].median()
    min_latency = df['latency'].min()
    max_latency = df['latency'].max()

    lower_percentage = (median_latency - min_latency) / median_latency
    upper_percentage = (max_latency - median_latency) / median_latency

    rounded_lower_percentage = math.ceil(lower_percentage * 100) / 100
    rounded_upper_percentage = math.ceil(upper_percentage * 100) / 100
    equal_percentage = max(rounded_lower_percentage, rounded_upper_percentage)

    lower_bound = median_latency * (1 - equal_percentage)
    upper_bound = median_latency * (1 + equal_percentage)

    return median_latency, lower_bound, upper_bound, equal_percentage


def plot_latency(df, median_latency, lower_bound, upper_bound, equal_percentage, figure_name):
    df.plot.scatter(x='frame', y='latency')
    plt.axhline(median_latency, color='r', linestyle='--', label='Median latency')
    plt.axhline(lower_bound, color='g', linestyle=':', label=f'±{equal_percentage * 100:.0f}% from median')
    plt.axhline(upper_bound, color='g', linestyle=':',)
    plt.legend()

    plt.savefig(figure_name)
    plt.show()