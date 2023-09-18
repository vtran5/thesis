import ast
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import json

def count_publishers_per_executor(json_file_path):
    # Load the JSON file
    with open(json_file_path, "r") as f:
        data = json.load(f)

    # Extract the number of publishers for each executor
    publishers_count_per_executor = {}
    for executor, entities in data["executors"].items():
        publishers_count = sum(1 for entity in entities if "Publisher" in entity)
        publishers_count_per_executor[executor] = publishers_count

    return publishers_count_per_executor

def extract_data_for_section(content, start_section, end_section=None):
    """Extract data between start and end section markers."""
    data = {}
    in_section = False
    current_subsection = None
    executor_id = None
    
    for line in content:
        line = line.strip()
        if line == start_section:
            in_section = True
            continue
        if end_section and line == end_section:
            in_section = False
            continue
        
        if in_section:
            if 'ExecutorID' in line:
                executor_id = line.split(": ")[1]
            elif line.startswith("{"):
                stats = ast.literal_eval(line)
                data[(current_subsection, executor_id)] = stats
            elif line in ["Without data", "With data"]:
                current_subsection = line
            else:
                break
    return data

def extract_table_data(content, start_section):
    """Extract table data starting from the given section."""
    in_section = False
    headers = []
    rows = []
    
    for line in content:
        line = line.strip()
        if line == start_section:
            in_section = True
            continue
        
        if in_section:
            if not headers:
                headers = [header.strip() for header in line.split()]
                continue
            row_data = [item.strip() for item in line.split()]
            # Ensure the row data has the expected format
            if len(row_data) == 5:  # If there are 5 items in the row, including the row index
                rows.append(row_data[1:])  # Append only the 4 desired columns of data
            else:
                break
    
    # Convert the collected rows to a dataframe
    df = pd.DataFrame(rows, columns=headers[0:])
    
    return df

def extract_all_data_from_content(file_path):
    with open(file_path, "r") as file:
        content = file.readlines()
    """Extract data for all callbackLETs from the content."""
    all_data = []
    current_data = {}
    current_section = None
    
    idx = 0
    while idx < len(content):
        line = content[idx].strip()
        if line.startswith("Period:"):
        # if line.startswith("CallbackLET:"):
        # if line.startswith("MessageSize:"):
            # Save the previous callbackLET data and start a new one
            if current_data:
                all_data.append(current_data)
                current_data = {}
            current_data["CallbackLET"] = line.split(":")[1].strip()
        elif line in ["Input Overhead per wakeup", 
                      "Output Overhead per wakeup"]:
            current_section = line
            data = extract_data_for_section(content[idx:], current_section)
            current_data[current_section] = data
        elif line in ["Total input overhead vs default", "Total Output overhead vs default", "Total input overhead vs old let", "Total output overhead vs old let"]:
            current_section = line
            data = extract_table_data(content[idx:], current_section)
            current_data[current_section] = data
        idx += 1
    
    # Append the last callbackLET data
    if current_data:
        all_data.append(current_data)
    
    return all_data

def print_dataframe(df, df_name):
    pd.set_option('display.max_rows', None)
    pd.set_option('display.max_columns', None)
    pd.set_option('display.width', None)
    print(f"Dataframe name: {df_name}\n")
    print(df)

# Defining a darker color palette using the specified xkcd colors
colors = [
    '#1f77b4',  # Blue
    '#ff7f0e',  # Orange
    '#40b440',  # Green
    '#e377c2',  # Pink
    '#9467bd',  # Purple
    '#8c564b',  # Brown
    '#7f7f7f'   # Grey
]

def plot_box_whiskier(filtered_df, title):
    # List of unique ExecutorIDs
    executors = filtered_df["ExecutorID"].unique()
    
    # Define positions for each callbackLET
    positions = sorted(filtered_df["callbackLET"].unique())
    # fig, axes = plt.subplots(nrows=len(executors), figsize=(12, 3 * len(executors)))
    fig, axes = plt.subplots(round((len(executors))/2), 2, figsize=(12, 1.8 * len(executors)))
    # Plot data for each executor
    for idx, executor in enumerate(executors):
        if executor == "Executor7":
            continue
        ax = axes[(idx) // 2, (idx) % 2]
        executor_data = filtered_df[filtered_df["ExecutorID"] == executor]
        for pos in positions:
            data_row = executor_data[executor_data["callbackLET"] == pos]
            if not data_row.empty:
                stats = [{
                    "label": pos,
                    "whislo": data_row["Whisker Minimum"].values[0],
                    "q1": data_row["Q1"].values[0],
                    "med": data_row["Median"].values[0],
                    "q3": data_row["Q3"].values[0],
                    "whishi": data_row["Whisker Maximum"].values[0],
                    "fliers": []
                }]
                ax.bxp(stats, positions=[positions.index(pos)], widths=0.6, vert=True, patch_artist=True,
                      boxprops=dict(facecolor=colors[idx]),
                      whiskerprops=dict(color=colors[idx]),
                      capprops=dict(color=colors[idx]),
                      medianprops=dict(color='yellow'),
                      flierprops=dict(markeredgecolor=colors[idx]))

        ax.set_xticks(range(len(positions)))
        ax.set_xticklabels(sorted(executor_data["callbackLET"].astype(int)))
        ax.set_xlabel('Message Size (B)')
        ax.set_ylabel('Time (µs)')
        ax.set_title(executor)

    # plt.suptitle(title)
    plt.tight_layout()
    plt.subplots_adjust(hspace = 0.3)

def plot_total_overhead(df,title):
    # List of unique ExecutorIDs
    unique_executors = df["ExecutorID"].unique()
    executors = sorted(unique_executors)
    print(executors)
    # Create bar plots for each executor
    # fig, axes = plt.subplots(nrows=len(executors), figsize=(12, 2.5 * len(executors)), sharex=True)
    fig, axes = plt.subplots(round((len(executors)-1)/2), 2, figsize=(10, 8))
    bar_width = 0.35  # width of the bars
    index_positions = range(len(df["callbackLET"].unique()))

    for idx, executor in enumerate(executors):
        # Filter data for the current executor
        executor_data = df[df["ExecutorID"] == executor].sort_values(by="callbackLET")
        executor_data["Input Overhead Total"] = executor_data["Input Overhead Total"]/1200000000
        executor_data["Output Overhead Total"] = executor_data["Output Overhead Total"]/1200000000 
        if executor == "Executor7":
            continue
        ax = axes[(idx) // 2, (idx) % 2]
        bars1 = ax.bar(index_positions, executor_data["Input Overhead Total"], bar_width, label="Input Overhead Total", alpha=0.8)
        bars2 = ax.bar([i + bar_width for i in index_positions], executor_data["Output Overhead Total"], bar_width, label="Output Overhead Total", alpha=0.8)
        
        ax.set_title(f'{executor}')
        ax.set_xticks([i + bar_width/2 for i in index_positions])
        ax.set_xticklabels(sorted(executor_data["callbackLET"].astype(int)))
        ax.legend()
        ax.set_xlabel('CallbackLET (B)')
        ax.set_ylabel('Overhead Total (%)')

    plt.tight_layout()
    plt.subplots_adjust(hspace = 0.3)
    
def plot_side_by_side_boxes(df1, df2, title):
    # Assuming both dataframes have the same ExecutorIDs
    executors = df1["ExecutorID"].unique()
    if title == "output":
        executors = [e for e in executors if e != 'Executor7']
    if title == "input":
        executors = [e for e in executors if e != 'Executor1']
    
    positions = sorted(df1["callbackLET"].unique())
    
    fig, axes = plt.subplots(round((len(executors))/2), 2, figsize=(12, 1.8 * len(executors)))

    # Ensure axes is always a list for consistent indexing
    if len(executors) == 1:
        axes = [axes]

    # Define a function to get box stats from the data row
    def get_box_stats(data_row):
        return {
            "whislo": data_row["Whisker Minimum"].values[0],
            "q1": data_row["Q1"].values[0],
            "med": data_row["Median"].values[0],
            "q3": data_row["Q3"].values[0],
            "whishi": data_row["Whisker Maximum"].values[0],
            "fliers": []
        }

    for idx, executor in enumerate(executors):
        
        ax = axes[(idx) // 2, (idx) % 2]
        
        for pos_idx, pos in enumerate(positions):
            data_row1 = df1[(df1["ExecutorID"] == executor) & (df1["callbackLET"] == pos)]
            data_row2 = df2[(df2["ExecutorID"] == executor) & (df2["callbackLET"] == pos)]

            stats = []
            colors = []
            if not data_row1.empty:
                stats.append(get_box_stats(data_row1))
                colors.append('orange')
                
            if not data_row2.empty:
                stats.append(get_box_stats(data_row2))
                colors.append('blue')

            for i, stat in enumerate(stats):
                ax.bxp([stat], positions=[pos_idx + i * 0.3], widths=0.25, vert=True, 
                       patch_artist=True, boxprops=dict(facecolor=colors[i]))
                
        # xtick = [round(x/1000) for x in positions]
        xtick = [round(x) for x in positions]
        ax.set_xticks([i for i in range(len(positions))])
        ax.set_xticklabels(xtick, fontsize=12)
        # ax.set_xlabel('Message Size (kB)',  fontsize=14)
        ax.set_xlabel('Period (ms)',  fontsize=14)
        ax.set_xlabel('Callback LET (ms)',  fontsize=14)
        ax.set_xlabel('Message Size (kB)',  fontsize=14)
        ax.set_ylabel('Time (µs)',  fontsize=14)
        ax.set_title(executor, fontsize=15)

    plt.tight_layout()
    plt.subplots_adjust(hspace = 0.5)

# Custom formatter function
def percent_formatter(x, pos):
    if 100*x > 8 or 100*x <-8 or x == 0:
        return f"{100 * x:.0f}%"
    else:
        return f"{100 * x:.2f}%"

def plot_grouped_barchart(dfs_in: dict, dfs_out: dict, title):
    """Plots a grouped bar chart from a dictionary of dataframes.
    
    Args:
    - dfs (dict): Dictionary where the keys are callbackLET values and 
                  the values are corresponding dataframes.
    """
    
    # Extract unique ExecutorIDs (assuming all dataframes have the same ExecutorIDs)
    executor_ids = dfs_in[next(iter(dfs_in))]['ExecutorID'].unique()
    executor_ids = [e for e in executor_ids if e != 'Executor7' and e != 'Executor1']
    callbackLETs = sorted([int(let) for let in dfs_in.keys()])
    num_callbackLETs = len(callbackLETs)
    width = 0.2  # Width of the bars
    index_positions = range(num_callbackLETs)
    fig, axes = plt.subplots(round((len(executor_ids)+1)/2), 2, figsize=(12, 1.8 * len(executor_ids)))

    # Create a bar for each ExecutorID's value per callbackLET
    for idx, executor in enumerate(executor_ids):
        ax = axes[(idx) // 2, (idx) % 2]
        values = []
        values2 = []

        for callbackLET in callbackLETs:
            value_str = dfs_in[callbackLET].loc[dfs_in[callbackLET]['ExecutorID'] == executor, 'Difference'].values[0]
            input_old = dfs_in[callbackLET].loc[dfs_in[callbackLET]['ExecutorID'] == executor, dfs_in[callbackLET].columns[3]].values[0]
            input_old_int = int(input_old)
            value_str2 = dfs_out[callbackLET].loc[dfs_out[callbackLET]['ExecutorID'] == executor, 'Difference'].values[0]
            output_old = dfs_out[callbackLET].loc[dfs_out[callbackLET]['ExecutorID'] == executor, dfs_out[callbackLET].columns[3]].values[0]
            output_old_int = int(output_old)
            value_int = int(value_str)  # Convert the string value to integer
            value_int2 = int(value_str2)
            if title == "original":
                result = value_int / input_old_int# 180000000000
                result2 = value_int2 / output_old_int# 180000000000
            if title == "runtime":
                result = value_int / 180000000000
                result2 = value_int2 / 180000000000                
            values.append(result)
            values2.append(result2)

        a1 = ax.bar([x - width/2 for x in range(num_callbackLETs)], values, width=width, label="Input Overhead", color='blue')
        a2 = ax.bar([x + width/2 for x in range(num_callbackLETs)], values2, width=width, label="Output Overhead", color='orange')

        
        ax.set_xticks(range(num_callbackLETs))
        ax.set_xticklabels(callbackLETs, fontsize = 12)
        ax.yaxis.set_major_formatter(mticker.FuncFormatter(percent_formatter))
        for label in ax.get_yticklabels():
            label.set_fontsize(12)  # Change 12 to the desired font size
        ax.set_xlabel('callbackLET', fontsize = 14)
        ax.set_ylabel('Relative Overhead', fontsize = 14)
        ax.set_title(executor, fontsize = 15)
        # Removing individual legends
        # ax.legend()

    # Adding a single legend outside the subplots
    labels = ["Input Overhead", "Output Overhead"]
    fig.legend([a1, a2], labels = labels, loc='lower right', fontsize=16)

    plt.tight_layout()
    # plt.show()

def plot_twin_barchart(dfs_in, dfs_out, title):
    """Plots a grouped bar chart with twin y-axes from a dictionary of dataframes."""
    # Extract unique ExecutorIDs (assuming all dataframes have the same ExecutorIDs)
    executor_ids = dfs_in[next(iter(dfs_in))]['ExecutorID'].unique()
    executor_ids = [e for e in executor_ids if e != 'Executor7' and e != 'Executor1']
    callbackLETs = sorted([int(let) for let in dfs_in.keys()])
    num_callbackLETs = len(callbackLETs)
    width = 0.2  # Width of the bars
    index_positions = range(num_callbackLETs)
    fig, axes = plt.subplots(round((len(executor_ids)+1)/2), 2, figsize=(12, 1.8 * len(executor_ids)))

    for idx, executor in enumerate(executor_ids):
        ax = axes[idx // 2, idx % 2]
        ax2 = ax.twinx()  # Create a twin y-axis

        values = []
        values2 = []
        ticks = []

        for callbackLET in callbackLETs:
            value_str = dfs_in[callbackLET].loc[dfs_in[callbackLET]['ExecutorID'] == executor, 'Difference'].values[0]
            input_old = dfs_in[callbackLET].loc[dfs_in[callbackLET]['ExecutorID'] == executor, dfs_in[callbackLET].columns[3]].values[0]
            input_old_int = int(input_old)
            value_str2 = dfs_out[callbackLET].loc[dfs_out[callbackLET]['ExecutorID'] == executor, 'Difference'].values[0]
            output_old = dfs_out[callbackLET].loc[dfs_out[callbackLET]['ExecutorID'] == executor, dfs_out[callbackLET].columns[3]].values[0]
            output_old_int = int(output_old)
            value_int = int(value_str)  # Convert the string value to integer
            value_int2 = int(value_str2)
            if title == "original":
                result = value_int / input_old_int# 180000000000
                result2 = value_int2 / output_old_int# 180000000000
            if title == "runtime":
                result = (value_int*1) / 60000000000
                result2 = (value_int2*1) / 60000000000               
            values.append(result)
            values2.append(result2)
            ticks.append(callbackLET)

        # Determine limits to align zeros
        if (min(values) < 0):
            min_val = min(values)*1.1
        else:
            min_val = 0
        lims1 = (min_val, max(values)*2.5)
        lims2 = (min(values2)*0.9, max(values2)*1.1)

        # Compute the range ratio
        ratio = lims1[1] / lims2[1]

        # Set axis limits
        ax.set_ylim(lims1)
        ax2.set_ylim([lims1[0]/ratio, lims1[1]/ratio])

        a1 = ax.bar([x - width/2 for x in range(num_callbackLETs)], values, width=width, label="Input Overhead", color='blue')
        a2 = ax2.bar([x + width/2 for x in range(num_callbackLETs)], values2, width=width, label="Output Overhead", color='orange')
        
        # tick_label = [round(x/1000) for x in callbackLETs]
        tick_label = [round(x) for x in callbackLETs]
        ax.set_xticks(range(num_callbackLETs))
        ax.set_xticklabels(ticks, fontsize = 12)
        ax.yaxis.set_major_formatter(mticker.FuncFormatter(percent_formatter))
        ax2.yaxis.set_major_formatter(mticker.FuncFormatter(percent_formatter))
        for label in ax.get_yticklabels():
            label.set_fontsize(12)
        for label in ax2.get_yticklabels():
            label.set_fontsize(12)
        ax.set_xlabel('Period (ms)', fontsize=14)
        # ax.set_xlabel('Callback LET (ms)', fontsize=14)
        # ax.set_xlabel('Message Size (kB)', fontsize=14)
        ax.set_ylabel('Input Overhead', fontsize=14)
        ax2.set_ylabel('Output Overhead', fontsize=14)
        ax.set_title(executor, fontsize=15)

    # Get the legend handles and labels from both axes
    handles1, labels1 = ax.get_legend_handles_labels()
    handles2, labels2 = ax2.get_legend_handles_labels()

    # Combine the handles and labels
    all_handles = handles1 + handles2
    all_labels = labels1 + labels2

    # Set the legend using fig.legend()
    fig.legend(all_handles, all_labels, loc='lower right', fontsize=16)

    plt.tight_layout()

if __name__ == "__main__":
    file_path = "./result/overhead_profile_varied_period.txt"  # You can replace with your file path
    # file_path = "./result/overhead_profile_varied_let.txt" 
    # file_path = "./result/overhead_profile_varied_message_size.txt" 
    extracted_data = extract_all_data_from_content(file_path)
    # Creating the first table
    table_1_data = []
    total_ip_default = {}
    total_op_default = {}
    total_ip_old = {}
    total_op_old = {}
    for data in extracted_data:
        callbackLET = float(data["CallbackLET"])
        
        for data_type, data_values in data.items():
            if data_type not in ["CallbackLET", "Total input overhead vs default", "Total Output overhead vs default", "Total input overhead vs old let", "Total output overhead vs old let"]:
                for (data_availability, executor_id), stats in data_values.items():
                    row = {
                        "callbackLET": callbackLET,
                        "data type": data_type,
                        "ExecutorID": executor_id,
                        "data available": True if data_availability == "With data" else False,
                        "Minimum": stats["Minimum"],
                        "Q1": stats["Q1"],
                        "Median": stats["Median"],
                        "Q3": stats["Q3"],
                        "Maximum": stats["Maximum"],
                        "Whisker Minimum": stats["Whisker Minimum"],
                        "Whisker Maximum": stats["Whisker Maximum"]
                    }
                    table_1_data.append(row)
            if data_type == "Total input overhead vs default":
                total_ip_default[callbackLET] = data_values
            if data_type == "Total Output overhead vs default":
                total_op_default[callbackLET] = data_values
            if data_type == "Total input overhead vs old let":
                total_ip_old[callbackLET] = data_values
            if data_type == "Total output overhead vs old let":
                total_op_old[callbackLET] = data_values

    table1_df = pd.DataFrame(table_1_data)

    filtered_df_ip_no_data = table1_df[
        (table1_df["data type"] == "Input Overhead per wakeup") & 
        (table1_df["data available"] == False)
        ]

    filtered_df_ip_data = table1_df[
        (table1_df["data type"] == "Input Overhead per wakeup") & 
        (table1_df["data available"] == True)
        ]

    filtered_df_op_no_data = table1_df[
        (table1_df["data type"] == "Output Overhead per wakeup") & 
        (table1_df["data available"] == False)
        ]

    filtered_df_op_data = table1_df[
        (table1_df["data type"] == "Output Overhead per wakeup") & 
        (table1_df["data available"] == True)
        ]

    plot_twin_barchart(total_ip_old, total_op_old, "runtime")
    plot_twin_barchart(total_ip_old, total_op_old, "original")
    plot_twin_barchart(total_ip_default, total_op_default, "runtime")
    plot_twin_barchart(total_ip_default, total_op_default, "original")

    plot_side_by_side_boxes(filtered_df_ip_no_data, filtered_df_ip_data, "input")
    plot_side_by_side_boxes(filtered_df_op_no_data, filtered_df_op_data, "output")
    # plot_box_whiskier(filtered_df_ip_no_data, "Input Overhead with LET")
    # plot_box_whiskier(filtered_df_input_data, "Input Overhead with LET")
    # plot_box_whiskier(filtered_df_input_nodata, "Input Overhead with LET")
    # plot_total_overhead(table2_df, "Total Overhead")
    # plot_box_whiskier(filtered_df_ip_no_data, "Input Overhead with LET")
    plt.show()
    # print_dataframe(table2_df, "table2")