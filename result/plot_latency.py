import sys
import pandas as pd
import matplotlib.pyplot as plt
import os
import plot_utils as pu

if len(sys.argv) != 2:
    print("Usage: python plot.py <input_file>")
    sys.exit(1)

input_file = sys.argv[1]

# Read the file line by line and determine the maximum number of fields
df = pu.read_input_file(input_file)

start_time = pu.find_start_time(df)

subscriber_map = pu.find_map(df, 'Subscriber')
timer_map = pu.find_map(df, 'Timer')

subscriber = pu.process_dataframe(df, 'Subscriber', subscriber_map, start_time, frame_id=True)
timer = pu.process_dataframe(df, 'Timer', timer_map, start_time, frame_id=True)

publish_time = timer[['FrameID','Time']]
receive_time = subscriber[subscriber['ExecutorID'] == 'Subscriber3'][['FrameID', 'Time']]

# Merge the dataframes on 'FrameID'
merged_df = pd.merge(publish_time, receive_time, on='FrameID', suffixes=('_publish', '_receive'))

merged_df['latency'] = merged_df['Time_receive'] - merged_df['Time_publish']

latency = merged_df[['FrameID', 'latency']]
latency.columns = ['frame','latency']

latency = latency.iloc[1:]
# Calculate the latency range
median_latency, lower_bound, upper_bound, equal_percentage = pu.calculate_latency_range(latency)

print(f"Median latency: {median_latency:.2f}")
print(f"Range from median (±{equal_percentage * 100:.0f}%): {lower_bound:.2f} - {upper_bound:.2f}")

# Save the figure with the same name as the input file and a '.png' extension
figure_name = os.path.splitext(input_file)[0] + '.png'

# Plot the latency
pu.plot_latency(latency, median_latency, lower_bound, upper_bound, equal_percentage, figure_name)