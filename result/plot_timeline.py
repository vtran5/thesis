import sys
import pandas as pd
import plot_utils as pu
import os

if len(sys.argv) != 2:
    print("Usage: python plot.py <input_file>")
    sys.exit(1)

input_file = sys.argv[1]
df = pu.read_input_file(input_file)

start_time = pu.find_start_time(df)

executor_map = pu.find_map(df, 'Executor')
publisher_map = pu.find_map(df, 'Publisher')

publisher = pu.process_dataframe(df, 'Publisher', publisher_map, start_time)
listener = pu.process_dataframe(df, 'Listener', executor_map, start_time)
writer = pu.process_dataframe(df, 'Writer', executor_map, start_time)
executor = pu.process_dataframe(df, 'Executor', executor_map, start_time)
data = pu.process_dataframe(df, 'Frame')

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

# Get the executor, publisher, listener and writer values during the random duration
data = data.apply(pd.to_numeric, errors='coerce')
data = data.drop(data.columns[0], axis=1)
min_time = data.min().min()
max_time = data.max().max()
filtered_executor = pu.get_filtered_times(executor, min_time, max_time)
filtered_publisher = pu.get_filtered_times(publisher, min_time, max_time)
filtered_listener = pu.get_filtered_times(listener, min_time, max_time)
filtered_writer = pu.get_filtered_times(writer, min_time, max_time)

# Save the figure with the same name as the input file and a '.png' extension
figure_name = os.path.splitext(input_file)[0] + '_timeline.png'
pu.plot_timeline(data, figure_name, filtered_executor, filtered_publisher, filtered_listener, filtered_writer)
