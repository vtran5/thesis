import math

class Task:
    def __init__(self, entity, period, execution_time, priority):
        self.entity = entity
        self.period = period
        self.execution_time = execution_time
        self.priority = priority
        self.LET = execution_time

    def update_LET(self, executors):
        for executor in executors:
            if executor.priority > self.priority:
                self.LET += executor.total_execution_time()
    def update_LET(self, executors):
        added_time = 0
        previous_added_time = -1
        while previous_added_time != added_time:  # Continue until no more time is added
            previous_added_time = added_time
            added_time = 0
            for executor in executors:
                if executor.priority > self.priority:
                    for task in executor.tasks:
                        # Calculate the multiplier based on the current LET
                        multiplier = math.floor((self.LET + added_time) / task.period)
                        if ((self.LET + added_time) - task.period*multiplier) > task.execution_time:
                            added_time += task.execution_time
                        else:
                            added_time += ((self.LET + added_time) - task.period*multiplier)
                        added_time += multiplier * task.execution_time 
            self.LET += added_time

class Executor:
    def __init__(self, name, period, priority):
        self.name = name
        self.period = period
        self.priority = priority
        self.tasks = []

    def add_task(self, task):
        self.tasks.append(task)

    def total_execution_time(self):
        return sum(task.execution_time for task in self.tasks)

    def update_tasks_LET(self, executors):
        total_execution_time = self.total_execution_time()
        cumulative_execution_time = 0
        for task in self.tasks:
            #task.LET += total_execution_time - task.execution_time
            #task.LET += cumulative_execution_time
            #cumulative_execution_time += task.execution_time
            task.update_LET(executors)


executors = [
    Executor("Executor 1", 10, 3),
    Executor("Executor 2", 20, 2),
    Executor("Executor 3", 50, 1)
]

# Adding tasks
executors[0].add_task(Task("Timer1", 150, 0, 3))
executors[0].add_task(Task("Timer2", 420, 0, 3))
executors[1].add_task(Task("Subscriber1", 150, 10, 2))
executors[1].add_task(Task("Subscriber2", 520, 45, 2))
executors[1].add_task(Task("Timer3", 160, 5, 2))
executors[2].add_task(Task("Subscriber3", 150, 65, 1))
executors[2].add_task(Task("Subscriber4", 520, 10, 1))
executors[2].add_task(Task("Subscriber5", 160, 10, 1))
executors[2].add_task(Task("Subscriber6", 520, 5, 1))

utilization = 0.00
# Update LET for each task
for executor in executors:
    executor.update_tasks_LET(executors)

# Print updated LET for each task
for executor in executors:
    for task in executor.tasks:
        print(f"{task.entity}'s new LET: {task.LET}")
        utilization += task.execution_time/task.period

print(utilization)