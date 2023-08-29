import json
import os
import glob

config_files_path = './rclc_exp/src/config_files/varied_let'
result_path = './result/automated_test_json/varied_let'
current_path = os.getcwd()
print(f"Current working directory: {current_path}")
file_list = glob.glob(os.path.join(config_files_path, 'config*.txt'))
print(file_list)
for config_file in glob.glob(os.path.join(config_files_path, 'config*.txt')):
  # Read the txt file
  with open(config_file, 'r') as file:
    lines = file.readlines()

  # Parsing the nodes
  nodes = int(lines[0].strip().split(': ')[1])

  # Storing the data
  json_data = {
    "executors": {},
    "callback_chain": {},
    "publisher_mapping": {}
  }

  executor_data = {}

  # Parse the executor data
  for i in range(1, nodes + 1):
    executor_name = f"Executor{i}"
    publishers, subscribers, timers = map(int, lines[i + 4].strip().split(': ')[1].split())
    executor_data[executor_name] = {
        'publishers': publishers,
        'subscribers': subscribers,
        'timers': timers
    }

  publishers_counter = 1
  subscribers_counter = 1
  timers_counter = 1
  chain_counter = 1

  # Create executors and callback chains
  for executor_name, data in executor_data.items():
    json_data["executors"][executor_name] = [executor_name]

    for _ in range(data['publishers']):
        json_data["executors"][executor_name].append(f"Publisher{publishers_counter}")
        publishers_counter += 1

    for _ in range(data['subscribers']):
        json_data["executors"][executor_name].append(f"Subscriber{subscribers_counter}")
        subscribers_counter += 1

    for _ in range(data['timers']):
        json_data["executors"][executor_name].append(f"Timer{timers_counter}")

        # Add callback chain
        json_data["callback_chain"][f"chain{chain_counter}"] = [f"Timer{timers_counter}"]
        chain_counter += 1

        timers_counter += 1

  # Create mapping and complete callback chains
  # publisher_mapped = 1
  # subscribers_mapped = 1
  # timers_mapped = 1

  # for executor_name, data in executor_data.items():
  #   publisher_one_ex_mapped = 0
  #   for _ in range(data['timers']):
  #     if publisher_one_ex_mapped <= executor_data[executor_name]['publishers']:
  #       json_data["publisher_mapping"][f"Timer{timers_mapped}"] = [f"Publisher{publisher_mapped}"]
  #       json_data["callback_chain"][f"chain{timers_mapped}"].extend([f"Publisher{publisher_mapped}"])
  #       publisher_mapped += 1
  #       publisher_one_ex_mapped += 1
  #     timers_mapped += 1

  #   subscriber_one_ex_mapped = 1
  #   for _ in range(data['subscribers']):
  #     if publisher_one_ex_mapped < executor_data[executor_name]['publishers']:
  #       json_data["publisher_mapping"][f"Subscriber{subscribers_mapped}"] = [f"Publisher{publisher_mapped}"]
  #       json_data["callback_chain"][f"chain{subscriber_one_ex_mapped}"].extend([f"Subscriber{subscribers_mapped}", f"Publisher{publisher_mapped}"])
  #       publisher_mapped += 1
  #       publisher_one_ex_mapped += 1
  #       subscriber_one_ex_mapped += 1
  #     subscribers_mapped += 1

  # Determine the output filename based on the input filename
  config_number = os.path.basename(config_file).split('.')[0].replace('config', '')
  json_filename = os.path.join(result_path, f'automated_test{config_number}.json')
  print(json_filename)
  # Write the JSON file
  with open(json_filename, 'w') as file:
      json.dump(json_data, file, indent=4)

print("Conversion completed!")
