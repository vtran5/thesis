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
        fields = len(line.strip().split(' '))
        max_fields = max(max_fields, fields)

# Read the file using pandas with the maximum number of fields
df = pd.read_csv(input_file, sep=' ', header=None, usecols=range(max_fields), engine='python')

# Filter the data to keep only rows with the first column value equal to 'node4'
df = df[df.iloc[:, 0] == 'node4']

df.dropna(axis=1, inplace=True, how='all')

column_count = df.shape[1]
column_names = ['node', 'frame'] + [str(i) for i in range(1, column_count - 2)] + ['latency']
df.columns = column_names

df['latency'] = df['latency']/1000

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

print(f"Median latency: {median_latency:.2f}")
print(f"Range from median (±{equal_percentage * 100:.0f}%): {lower_bound:.2f} - {upper_bound:.2f}")

ax = df.plot.scatter(x='frame', y='latency')
ax.set_ylabel('latency (ms)')  # Change the y-axis label
plt.axhline(median_latency, color='r', linestyle='--', label='Median latency')
plt.axhline(lower_bound, color='g', linestyle=':', label=f'±{equal_percentage * 100:.0f}% from median')
plt.axhline(upper_bound, color='g', linestyle=':',)
plt.legend()

# Save the figure with the same name as the input file and a '.png' extension
figure_name = os.path.splitext(input_file)[0] + '.png'
plt.savefig(figure_name)

# Show the figure
plt.show()
