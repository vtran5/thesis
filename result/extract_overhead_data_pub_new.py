import ast
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import json

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
        if line.startswith("path:"):
            # Save the previous callbackLET data and start a new one
            if current_data:
                all_data.append(current_data)
                current_data = {}
            current_data["path"] = line.split(":")[1].strip()
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

def plot_side_by_side_boxes(df1, df2, title):
    # Assuming both dataframes have the same ExecutorIDs
    executors = df1["ExecutorID"].unique()
    executors = [e for e in executors if e != 'Executor7' and e != 'Executor1']
  
    fig, axes = plt.subplots(round((len(executors)+1)/2), 2, figsize=(12, 1.8 * len(executors)))

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
        executor_data = df1[df1["ExecutorID"] == executor]
        positions = sorted(executor_data["number of publishers"].unique().astype(int))
        
        ax = axes[(idx) // 2, (idx) % 2]
        
        for pos_idx, pos in enumerate(positions):
            data_row1 = df1[(df1["ExecutorID"] == executor) & (df1["number of publishers"] == pos)]
            data_row2 = df2[(df2["ExecutorID"] == executor) & (df2["number of publishers"] == pos)]

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

        ax.set_xticks([i for i in range(len(positions))])
        ax.set_xticklabels(positions, fontsize=12)
        ax.set_xlabel('Number of Publishers',  fontsize=14)
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

def plot_twin_barchart(dfs_in, dfs_out, title):
    """Plots a grouped bar chart with twin y-axes from a dictionary of dataframes."""
    width = 0.2
    executor_ids = set()
    for _, df in dfs_in:
        executor_ids.update(df['ExecutorID'].values)
    executor_ids = sorted(list(executor_ids))
    executor_ids = [e for e in executor_ids if e != 'Executor7' and e != 'Executor1']
    fig, axes = plt.subplots(round((len(executor_ids)+1)/2), 2, figsize=(12, 1.8 * len(executor_ids)))

    for idx, executor in enumerate(executor_ids):
        ax = axes[idx // 2, idx % 2]
        ax2 = ax.twinx()  # Create a twin y-axis

        pub_set1 = []
        pub_set2 = []
        
        values = []
        values2 = []

        for pub_count, df in dfs_in:
            pub_set1.append(pub_count[executor])
            overhead_value = df.loc[df['ExecutorID'] == executor, 'Difference'].values[0]
            overhead_old = df.loc[df['ExecutorID'] == executor, df.columns[3]].values[0]
            overhead_old_int = int(overhead_old)
            overhead_value_int = int(overhead_value)
            if title == "original":
                values.append(overhead_value_int/overhead_old_int)
            else:
                values.append(overhead_value_int/180000000000)
            
        for pub_count, df in dfs_out:
            pub_set2.append(pub_count[executor])
            overhead_value2 = df.loc[df['ExecutorID'] == executor, 'Difference'].values[0]
            overhead_old2 = df.loc[df['ExecutorID'] == executor, df.columns[3]].values[0]
            overhead_old_int2 = int(overhead_old2)
            overhead_value_int2 = int(overhead_value2)
            if title == "original":
                values2.append(overhead_value_int2/overhead_old_int2)
            else:
                values2.append(overhead_value_int2/180000000000)
        
        pubs1 = [int(pub) for pub in pub_set1]
        pubs2 = [int(pub) for pub in pub_set2]
        combined1 = list(zip(pubs1, values))
        sorted_combined1 = sorted(combined1, key=lambda x: x[0])
        pubs1_sorted, values1_sorted = zip(*sorted_combined1)

        combined2 = list(zip(pubs2, values2))
        sorted_combined2 = sorted(combined2, key=lambda x: x[0])
        pubs2_sorted, values2_sorted = zip(*sorted_combined2)

        pubs_num = len(pubs1_sorted)

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

        a1 = ax.bar([x - width/2 for x in range(pubs_num)], values1_sorted, width=width, label="Input Overhead", color='blue')
        a2 = ax2.bar([x + width/2 for x in range(pubs_num)], values2_sorted, width=width, label="Output Overhead", color='orange')
        
        ax.set_xticks(range(pubs_num))
        ax.set_xticklabels(pubs1_sorted, fontsize=12)
        ax.yaxis.set_major_formatter(mticker.FuncFormatter(percent_formatter))
        ax2.yaxis.set_major_formatter(mticker.FuncFormatter(percent_formatter))
        for label in ax.get_yticklabels():
            label.set_fontsize(12)
        for label in ax2.get_yticklabels():
            label.set_fontsize(12)
        ax.set_xlabel('Number of Publishers', fontsize=14)
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

def count_entity_per_executor(json_file_path, name):
    # Load the JSON file
    with open(json_file_path, "r") as f:
        data = json.load(f)

    # Extract the number of publishers for each executor
    publishers_count_per_executor = {}
    for executor, entities in data["executors"].items():
        publishers_count = sum(1 for entity in entities if name in entity)
        publishers_count_per_executor[executor] = publishers_count

    return publishers_count_per_executor

if __name__ == "__main__":
    file_path = "./result/overhead_profile_varied_pub_num.txt"  # You can replace with your file path
    extracted_data = extract_all_data_from_content(file_path)
    # Creating the first table
    table_1_data = []
    total_ip_default = []
    total_op_default = []
    total_ip_old = []
    total_op_old = []
    for data in extracted_data:
        json_path = data["path"]
        pub_count = count_entity_per_executor(json_path, "Publisher")
        
        for data_type, data_values in data.items():
            if data_type not in ["path", "Total input overhead vs default", "Total Output overhead vs default", "Total input overhead vs old let", "Total output overhead vs old let"]:
                for (data_availability, executor_id), stats in data_values.items():
                    row = {
                        "number of publishers": pub_count[executor_id],
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
            else:
                pair = (pub_count, data_values)
                if data_type == "Total input overhead vs default":
                    total_ip_default.append(pair)
                if data_type == "Total Output overhead vs default":
                    total_op_default.append(pair)
                if data_type == "Total input overhead vs old let":
                    total_ip_old.append(pair)
                if data_type == "Total output overhead vs old let":
                    total_op_old.append(pair)

    # print(total_ip_default)

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

    # pub_set1 = []
    # executor = 'Executor4'
    # for pub_count, df in total_ip_old:
    #     print(pub_count)
    #     print(pub_count[executor])
    #     pub_set1.append(pub_count[executor])
    # pubs1 = [int(pub) for pub in pub_set1]
    # print(pubs1)
    
    plot_twin_barchart(total_ip_old, total_op_old, "runtime")
    plot_twin_barchart(total_ip_old, total_op_old, "original")
    plot_twin_barchart(total_ip_default, total_op_default, "runtime")
    plot_twin_barchart(total_ip_default, total_op_default, "original")

    plot_side_by_side_boxes(filtered_df_ip_no_data, filtered_df_ip_data, "input")
    plot_side_by_side_boxes(filtered_df_op_no_data, filtered_df_op_data, "output")
    plt.show()
    # print_dataframe(table2_df, "table2")