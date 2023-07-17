import sys
import pandas as pd
import matplotlib.pyplot as plt
import os
import plot_utils as pu
import json

if len(sys.argv) != 3:
    print("Usage: python plot.py <input_file> <config_file>")
    sys.exit(1)

input_file = sys.argv[1]
json_file = sys.argv[2]
with open(json_file, 'r') as f:
    json_data = json.load(f)

# Extract callback chain data
callback_chains = json_data["callback_chain"]
if not callback_chains:
    print("No 'callback_chains' data found in the JSON file.")
    sys.exit(1)

# Read the file line by line and determine the maximum number of fields
df = pu.read_input_file(input_file)

start_time = pu.find_start_time(df)

subscriber_map = pu.find_map(df, 'Subscriber')
timer_map = pu.find_map(df, 'Timer')

subscriber = pu.process_dataframe(df, 'Subscriber', subscriber_map, start_time, frame_id=True)
timer = pu.process_dataframe(df, 'Timer', timer_map, start_time, frame_id=True)

# Create the figure with subplots.
fig, axs = plt.subplots(len(callback_chains), 1, figsize=(10, len(callback_chains)*5))

# Loop over each chain.
for idx, (chain_name, chain) in enumerate(callback_chains.items()):
    # Get the publish and receive times.
    publish_time = timer[timer['ExecutorID'] == chain[0]][['FrameID','Time']]
    receive_time = subscriber[subscriber['ExecutorID'] == chain[-1]][['FrameID', 'Time']]

    # Merge the dataframes on 'FrameID'
    merged_df = pd.merge(publish_time, receive_time, on='FrameID', suffixes=('_publish', '_receive'))

    # Calculate latency
    merged_df['latency'] = merged_df['Time_receive'] - merged_df['Time_publish']
    merged_df = merged_df.dropna()

    # Calculate the latency range
    latency = merged_df[['FrameID', 'latency']]
    latency.columns = ['frame','latency']

    #latency = latency.iloc[1:]
    latency = latency.dropna()
    median_latency, lower_bound, upper_bound, equal_percentage = pu.calculate_latency_range(latency)

    # Plot the latency
    pu.plot_latency(axs[idx], latency, median_latency, lower_bound, upper_bound, equal_percentage, chain_name)
    axs[idx].set_title(chain_name)

    print(chain_name)
    print(f"Median latency: {median_latency:.2f}")
    print(f"Range from median (±{equal_percentage * 100:.0f}%): {lower_bound:.2f} - {upper_bound:.2f}")

# Adjust layout for better visibility
plt.tight_layout()
plt.subplots_adjust(hspace = 0.4)
# Save the figure with the same name as the input file and a '.png' extension
figure_name = os.path.splitext(input_file)[0] + '_latency.png'

plt.savefig(figure_name)
plt.show()