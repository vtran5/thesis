import random
import copy

def generate_common_config():
    nodes = random.randint(4, 5)
    executor_period = random.choice(range(10, 200, 10))
    timer_period = random.choice(range(50, 1000, 10))
    callback_let = random.choice(range(5, 2000, 5))
    message_size = 512

    config = {
        "nodes": nodes,
        "executor_period": executor_period,
        "timer_period": timer_period,
        "callback_let": callback_let,
        "message_size": message_size,
        "node_config": []
    }

    previous_publisher = 0

    for node in range(1, nodes + 1):
        publishers = timers = subscribers = 0

        if node == 1:  # First Node
            timers = random.randint(2, 7)
            publishers = random.randint(timers, timers*random.randint(1, 2))
        elif node == nodes:  # Last Node
            subscribers = random.randint(round(previous_publisher/2), previous_publisher)
        else:  # Intermediate Nodes
            timers = random.randint(0, 2)
            subscribers = random.randint(round(previous_publisher/2), previous_publisher)
            publishers = random.randint(timers + subscribers, (timers + subscribers)*random.randint(2, 4))
            
        config["node_config"].append((publishers, subscribers, timers))
        previous_publisher = publishers

    return config

def modify_config(common_config, set_number):
    modified_config = copy.deepcopy(common_config)
    node_config = modified_config["node_config"]
    previous_publisher = 0
    for node_num, (publishers, subscribers, timers) in enumerate(node_config):
        # For the first set, vary publishers (except for the last node)
        if set_number == 1 and node_num != len(node_config) - 1:
            publishers = max(subscribers+timers, publishers + random.randint(0, 5))

        # For the second set, vary subscribers (except for the first node), ensuring they stay positive
        if set_number == 2 and node_num != 0:
            subscribers = min(previous_publisher, max(1, subscribers + random.randint(-15, 15)))

        # For the third set, vary timers (except for the last node)
        if set_number == 3 and node_num != len(node_config) - 1:
            timers += random.randint(0, 4)

        node_config[node_num] = (publishers, subscribers, timers)
        previous_publisher = publishers

    if set_number == 4:
        modified_config["executor_period"] = random.choice(range(10, 200, 10))
        modified_config["callback_let"] = random.choice(range(5, 2000, 5))

    return modified_config

def get_totals(config):
    total_publishers = 0
    total_subscribers = 0
    total_timers = 0

    for publishers, subscribers, timers in config["node_config"]:
        total_publishers += publishers
        total_subscribers += subscribers
        total_timers += timers

    return total_publishers, total_subscribers, total_timers

def generate_configs(common_config, set_number):
    configs = []
    total_varied_entities = []
    attempts = 0

    while len(configs) < 10 and attempts < 10000:
        modified_config = modify_config(common_config, set_number)
        total_publishers, total_subscribers, total_timers = get_totals(modified_config)
        range = 1
        if set_number == 1:
            total_varied_entity = total_publishers
        elif set_number == 2:
            total_varied_entity = total_subscribers
        else:  # set_number == 3
            total_varied_entity = total_timers
            range = 2

        # Check if the total number of the varied entity is within +/- 5 range of any previously recorded total

        if not any(abs(total - total_varied_entity) <= range for total in total_varied_entities):
            configs.append(modified_config)
            total_varied_entities.append(total_varied_entity)
            print("Total publishers:", total_publishers)
            print("Total subscribers:", total_subscribers)
            print("Total timers:", total_timers)
        
        attempts += 1

    if attempts == 2000:
        print("Warning: Maximum attempts reached. Could not generate all configurations.")

    return configs

def write_config(config, file_num):
    with open(f'config_files/config{file_num}.txt', 'w') as file:
        file.write(f"nodes: {config['nodes']}\n")
        file.write(f"executor_period: {config['executor_period']}\n")
        file.write(f"timer_period: {config['timer_period']}\n")
        file.write(f"message_size: {config['message_size']}\n")
        file.write(f"callback_let: {config['callback_let']}\n")

        for node_num, (publishers, subscribers, timers) in enumerate(config['node_config'], start=1):
            file.write(f"{node_num}: {publishers} {subscribers} {timers}\n")

    print(f"File '{file_num}' has been generated.")

def write_config_set(configs, file_num):
    for config in configs:
        write_config(config, file_num)
        file_num += 1
    return file_num

common_config = generate_common_config()
configs = generate_configs(common_config, 1)
filenum = 0
filenum = write_config_set(configs, filenum)
print(filenum)