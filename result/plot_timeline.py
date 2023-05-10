import sys
import pandas as pd
import matplotlib.pyplot as plt
import math
import os

if len(sys.argv) != 2:
    print("Usage: python plot.py <input_file>")
    sys.exit(1)

input_file = sys.argv[1]

# Read the file line by line and determine the maximum number of fields
max_fields = 0
with open(input_file, 'r') as f:
    for line in f:
        fields = len(line.split(' '))
        max_fields = max(max_fields, fields)

column_names = [i for i in range(0, max_fields)]

# Read the file using pandas with the maximum number of fields
df = pd.read_csv(input_file, sep=' ', header=None, names=column_names, engine='python')

startTime_df = df[df.iloc[:, 0] == 'StartTime']
startTime_df = startTime_df.dropna(axis=1)
start_time = startTime_df.iloc[0,1]
start_time = float(start_time)

executorID = df[df.iloc[:, 0] == 'ExecutorID']
executorID = executorID.dropna(axis=1)
executorID = executorID.drop(df.columns[0], axis=1)
exID_col_names = ['ExecutorName', 'ExecutorID']
executorID.columns = exID_col_names

executor_map = executorID.set_index('ExecutorID')['ExecutorName'].to_dict()
executor_map = {int(key): value for key, value in executor_map.items()}
executor_map = {str(key): value for key, value in executor_map.items()}

publisherID = df[df.iloc[:, 0] == 'PublisherID']
publisherID = publisherID.dropna(axis=1)
publisherID = publisherID.drop(df.columns[0], axis=1)
pubID_col_names = ['PublisherName', 'publisherID']
publisherID.columns = pubID_col_names

publisher_map = publisherID.set_index('publisherID')['PublisherName'].to_dict()
publisher_map = {int(key): value for key, value in publisher_map.items()}
publisher_map = {str(key): value for key, value in publisher_map.items()}

publisher = df[df.iloc[:, 0] == 'Publisher']
publisher = publisher.dropna(axis=1)
pub_col_names = ['Publisher','ExecutorID','Time']
publisher.columns = pub_col_names

publisher['Time'] = pd.to_numeric(publisher['Time'])
publisher['Time'] = (publisher['Time'] - start_time)/1000000
publisher['ExecutorID'] = publisher['ExecutorID'].replace(publisher_map)
publisher = publisher.drop(publisher.columns[0], axis=1)
publisher = publisher.round(1)

executor = df[df.iloc[:, 0] == 'Executor']
executor = executor.dropna(axis=1)
ex_col_names = ['Executor','ExecutorID','Time']
executor.columns = ex_col_names

executor['Time'] = pd.to_numeric(executor['Time'])
executor['Time'] = (executor['Time'] - start_time)/1000000
executor['ExecutorID'] = executor['ExecutorID'].replace(executor_map)
executor = executor.drop(executor.columns[0], axis=1)
executor = executor.round(1)

# Get data
data = df[df.iloc[:, 0] == 'Frame']
data = data.dropna(axis=1)
data = data.reset_index(drop=True)
data = data.drop(data.columns[0], axis=1)

column_count = data.shape[1]
column_names = ['frame'] + [str(i) for i in range(1, column_count - 1)] + ['latency']
data.columns = column_names

# get a random start index
start_index = data.sample(n=1).index[0]
#start_index = 0
# get the 3 consecutive rows starting from the random start index
data = data.iloc[start_index:start_index+4]
data.iloc[:, 1:] = data.iloc[:, 1:].astype(float)
data.iloc[:, 1:] = data.iloc[:, 1:] / 1000
data = data.round(1)

# Adjust the DataFrame values
for col in ['2', '3', '4', '5', 'latency']:
    data[col] = data['1'] + data[col]

# Get the min and max values
data = data.apply(pd.to_numeric, errors='coerce')
data = data.drop(data.columns[0], axis=1)
min_time = data.min().min()
max_time = data.max().max()
filtered_executor = executor[(executor['Time'] >= min_time) & (executor['Time'] <= max_time)]
filtered_publisher = publisher[(publisher['Time'] >= min_time) & (publisher['Time'] <= max_time)]
# Plotting function
def plot_timeline(data, input_file):
    fig, ax = plt.subplots(4, figsize=(10, 6), sharex=True)

    for i, node in enumerate(['Executor1', 'Executor2', 'Executor3', 'Executor4']):
        ax[i].set_title(node)
        ax[i].set_yticks([])

        # Add time values from node_time DataFrame
        node_times = filtered_executor[filtered_executor['ExecutorID'] == node]['Time']
        for time in node_times:
            ax[i].axvline(time, color='k', linestyle='--', linewidth=1)

        pub_times = filtered_publisher[filtered_publisher['ExecutorID'] == node]['Time']
        for time in pub_times:
            ax[i].axvline(time, color='cyan', linestyle='-.', linewidth=2)

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
    # Save the figure with the same name as the input file and a '.png' extension
    figure_name = os.path.splitext(input_file)[0] + '_timeline.png'
    plt.savefig(figure_name)

    plt.show()


plot_timeline(data, input_file)

