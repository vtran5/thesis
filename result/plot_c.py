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

df = df[df.iloc[:, 0] == 'Frame']
df = df.dropna(axis=1)
df = df.reset_index(drop=True)
df = df.drop(df.columns[0], axis=1)

column_count = df.shape[1]
column_names = ['frame'] + [str(i) for i in range(1, column_count - 1)] + ['latency']
df.columns = column_names

# Remove the last 3 rows
df = df.iloc[3:-3]

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

df.plot.scatter(x='frame', y='latency')
plt.axhline(median_latency, color='r', linestyle='--', label='Median latency')
plt.axhline(lower_bound, color='g', linestyle=':', label=f'±{equal_percentage * 100:.0f}% from median')
plt.axhline(upper_bound, color='g', linestyle=':',)
plt.legend()

# Save the figure with the same name as the input file and a '.png' extension
figure_name = os.path.splitext(input_file)[0] + '.png'
plt.savefig(figure_name)

# Show the figure
plt.show()


