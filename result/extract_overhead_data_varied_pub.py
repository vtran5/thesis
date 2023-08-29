import ast
import pandas as pd
import matplotlib.pyplot as plt
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
    data = {}
    in_section = False
    headers = []
    
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
            if len(row_data) == 3:
                executor_id = row_data[1]
                data_values = row_data[2:]
                data[executor_id] = dict(zip(headers[1:], data_values))
            else:
                break
    return data

def extract_all_data_from_content(file_path):
    with open(file_path, "r") as file:
        content = file.readlines()
    """Extract data for all number of publisherss from the content."""
    all_data = []
    current_data = {}
    current_section = None
    
    idx = 0
    while idx < len(content):
        line = content[idx].strip()
        if line.startswith("path:"):
            # Save the previous path data and start a new one
            if current_data:
                all_data.append(current_data)
                current_data = {}
            current_data["path"] = line.split(":")[1].strip()
        elif line in ["Input Overhead with LET", 
                      "Input Overhead without LET", 
                      "Output Thread Overhead"]:
            current_section = line
            data = extract_data_for_section(content[idx:], current_section)
            current_data[current_section] = data
        elif line in ["input overhead increase total", "output overhead increase total"]:
            current_section = line
            data = extract_table_data(content[idx:], current_section)
            current_data[current_section] = data
        idx += 1
    
    # Append the last number of publishers data
    if current_data:
        all_data.append(current_data)
    
    return all_data

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
    
    # Define positions for each number of publishers
    
    # fig, axes = plt.subplots(nrows=len(executors)-1, figsize=(12, 4 * len(executors)))
    fig, axes = plt.subplots(round((len(executors)-1)/2), 2, figsize=(12, 1.8 * len(executors)))
    # Plot data for each executor
    for idx, executor in enumerate(executors):
        if executor == "Executor7":
            continue
        ax = axes[idx // 2, idx % 2]
        executor_data = filtered_df[filtered_df["ExecutorID"] == executor]
        positions = sorted(executor_data["number of publishers"].unique())
        for pos in positions:
            data_row = executor_data[executor_data["number of publishers"] == pos]
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
        ax.set_xticklabels(positions)
        ax.set_xlabel('Number of publishers')
        ax.set_ylabel('Time (µs)')
        ax.set_title(executor)

    # plt.suptitle(title)
    plt.tight_layout()
    plt.subplots_adjust(hspace = 0.3)

def plot_total_overhead(df,title):
    # List of unique ExecutorIDs
    unique_executors = df["ExecutorID"].unique()
    executors = sorted(unique_executors)

    # Create bar plots for each executor
    # fig, axes = plt.subplots(nrows=len(executors), figsize=(12, 4 * len(executors)), sharex=True)
    fig1, axes1 = plt.subplots(round((len(executors)-1)/2), 2, figsize=(10, 8))
    plt.tight_layout()
    plt.subplots_adjust(hspace = 0.3)
    fig2, axes2 = plt.subplots(round((len(executors)-1)/2), 2, figsize=(12, 10))
    plt.tight_layout()
    plt.subplots_adjust(hspace = 0.3)
    bar_width = 0.35  # width of the bars

    for idx, executor in enumerate(executors):
        # Filter data for the current executor
        executor_data = df[df["ExecutorID"] == executor].sort_values(by="number of publishers")
        executor_data["Input Overhead Total"] = executor_data["Input Overhead Total"]/1200000000
        executor_data["Output Overhead Total"] = executor_data["Output Overhead Total"]/1200000000 
        # executor_data["number of publishers"] = executor_data["number of publishers"] .sort_values()
        index_positions = range(len(executor_data["number of publishers"].unique()))    
        if executor == "Executor7":
            continue
        ax1 = axes1[idx // 2, idx % 2]
        ax2 = axes2[idx // 2, idx % 2]
        bars1 = ax1.bar(index_positions, executor_data["Input Overhead Total"], bar_width, label="Input Overhead Total", alpha=0.8)
        bars2 = ax2.bar(index_positions, executor_data["Output Overhead Total"], bar_width, label="Output Overhead Total", alpha=0.8)
        
        ax1.set_title(f'{executor}')
        ax1.set_xticks([i for i in index_positions])
        ax1.set_xticklabels(executor_data["number of publishers"].unique())
        ax1.set_xlabel('Number of Publishers')
        ax1.set_ylabel('Overhead Total (%)')

        ax2.set_title(f'{executor}')
        ax2.set_xticks([i for i in index_positions])
        ax2.set_xticklabels(executor_data["number of publishers"].unique())
        ax2.set_xlabel('Number of Publishers')
        ax2.set_ylabel('Overhead Total (%)')

    plt.tight_layout()
    plt.subplots_adjust(hspace = 0.3)
    

if __name__ == "__main__":
    file_path = "./result/overhead_profile_varied_pub_num.txt"  # You can replace with your file path
    extracted_data = extract_all_data_from_content(file_path)
    # Creating the first table
    table_1_data = []

    for data in extracted_data:
        json_path = data["path"]
        pub_count = count_publishers_per_executor(json_path)
        
        for data_type, data_values in data.items():
            if data_type not in ["path", "input overhead increase total", "output overhead increase total"]:
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

    table1_df = pd.DataFrame(table_1_data)

    # Creating the second table
    table_2_data = []

    for data in extracted_data:
        json_path = data["path"]
        pub_count = count_publishers_per_executor(json_path)
        # Extracting input and output overhead increase total data
        input_overhead_data = data.get("input overhead increase total", {})
        output_overhead_data = data.get("output overhead increase total", {})
        
        executor_ids = set(input_overhead_data.keys()) | set(output_overhead_data.keys())
        
        for executor_id in executor_ids:
            row = {
                "number of publishers": pub_count[executor_id],
                "ExecutorID": executor_id,
                "Input Overhead Total": float(input_overhead_data.get(executor_id, {}).get("Difference", None)),
                "Output Overhead Total": float(output_overhead_data.get(executor_id, {}).get("Difference", None))
            }
            table_2_data.append(row)

    table2_df = pd.DataFrame(table_2_data)
    print(table2_df.to_csv(index=False, sep=','))
    filtered_df = table1_df[
      (table1_df["data type"] == "Output Thread Overhead") & 
      (table1_df["data available"] == False)
    ]

    filtered_df_output_data = table1_df[
      (table1_df["data type"] == "Output Thread Overhead") & 
      (table1_df["data available"] == True)
    ]

    filtered_df_input_data = table1_df[
      (table1_df["data type"] == "Input Overhead with LET") & 
      (table1_df["data available"] == True)
    ]

    filtered_df_input_nodata = table1_df[
      (table1_df["data type"] == "Input Overhead with LET") & 
      (table1_df["data available"] == False)
    ]

    # plot_total_overhead(table2_df, "Total Overhead")
    plot_box_whiskier(filtered_df, "Input Overhead with LET")
    # plot_box_whiskier(filtered_df_output_data, "Input Overhead with LET")
    # plot_box_whiskier(filtered_df_input_data, "Input Overhead with LET")
    # plot_box_whiskier(filtered_df_input_nodata, "Input Overhead with LET")
    plt.show()
    # print_dataframe(table2_df, "table2")