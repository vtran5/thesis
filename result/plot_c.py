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

# Process the dataframe for the 'Frame' keyword
df = pu.process_dataframe(df, 'Frame')

# Remove the last 3 rows
df = df.iloc[3:-3]

# Calculate the latency range
median_latency, lower_bound, upper_bound, equal_percentage = pu.calculate_latency_range(df)

print(f"Median latency: {median_latency:.2f}")
print(f"Range from median (±{equal_percentage * 100:.0f}%): {lower_bound:.2f} - {upper_bound:.2f}")

# Save the figure with the same name as the input file and a '.png' extension
figure_name = os.path.splitext(input_file)[0] + '.png'

# Plot the latency
pu.plot_latency(df, median_latency, lower_bound, upper_bound, equal_percentage, figure_name)


