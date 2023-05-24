import sys
import pandas as pd
import matplotlib.pyplot as plt
import os
import plot_utils as pu
import numpy as np

if len(sys.argv) != 2:
    print("Usage: python plot.py <input_file>")
    sys.exit(1)

input_file = sys.argv[1]

# Read the file line by line and determine the maximum number of fields
df = pu.read_input_file(input_file)

start_time = pu.find_start_time(df)
start_program_time = pu.find_program_start_time(df)
start_program_time = (start_program_time - start_time)/1000000

subscriber_map = pu.find_map(df, 'Subscriber')
subscriber = pu.process_dataframe(df, 'Subscriber', subscriber_map, start_time, frame_id=True)
subscriber3 = subscriber[subscriber.iloc[:, 0] == 'Subscriber3']
subscriber3['FrameID_difference'] = subscriber3['FrameID'].diff()
subscriber3 = subscriber3.dropna()

plt.figure(figsize=(10, 6)) # Define the plot size
plt.scatter(subscriber3.index, subscriber3['FrameID_difference']) # Plot the differences
plt.title('The number of data is skipped between 2 consecutive event chains') # Give it a title
plt.xlabel('Index') # Label for x-axis
plt.ylabel('Number of skipped data') # Label for y-axis
plt.show() # Display the plot