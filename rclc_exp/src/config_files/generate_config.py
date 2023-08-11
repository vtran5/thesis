import random

for file_num in range(1, 11):
  # Rule 1: Number of nodes less than 10
  nodes = random.randint(2, 9)

  # Rule 2: executor period less than 1000, and is multiple of 10
  executor_period = random.choice(range(10, 1000, 10))

  # Rule 3: timer period less than 2000, and is multiple of 10
  timer_period = random.choice(range(20, 2000, 10))

  # Commonly used values
  message_size = 512

  # Rule 4: callback let less than 5000, and is multiple of 10
  callback_let = random.choice(range(100, 5000, 10))

  previous_publisher = 0

  with open(f'config_files/config{file_num}.txt', 'w') as file:
    file.write(f"nodes: {nodes}\n")
    file.write(f"executor_period: {executor_period}\n")
    file.write(f"timer_period: {timer_period}\n")
    file.write(f"message_size: {message_size}\n")
    file.write(f"callback_let: {callback_let}\n")

    for node in range(1, nodes + 1):
      publishers = timers = subscribers = 0

      # Rule 5: first node doesn't have subscriber
      if node == 1:
        timers = random.randint(1, 7)  # Rule 8: less than 20
        subscribers = 0
        publishers = timers
      # Rule 6: last node doesn't have timer and publisher
      elif node == nodes:
        publishers = timers = 0
        subscribers = previous_publisher  # Rule 8: less than 20
      # Rule 7: for each node except last node: number of publisher = number of timer + number of subscriber
      else:
        timers = random.randint(1, 7)  # Rule 8: less than 20
        subscribers = previous_publisher  # Rule 9: subscriber = previous node's publisher
        publishers = timers + subscribers

      file.write(f"{node}: {publishers} {subscribers} {timers}\n")
      previous_publisher = publishers

  print(f"File 'config{file_num}.txt' has been generated.")
