from collections import deque
import math
import heapq
# Define the ActualTimer structure
class ActualTimer:
    def __init__(self, start_time, period):
        self.start_time = start_time
        self.period = period
        self.next_trigger_time = start_time+period
        self.triggered = False

    def update_trigger(self, current_time):
        if current_time >= self.next_trigger_time:
            self.next_trigger_time += self.period
            self.triggered = True 

    def check_trigger(self):
        if self.triggered == True: 
            self.triggered = False
            return True
        return False

# Adjust Task to include task name and record more detailed events
class Task:
    def __init__(self, name, task_type, deadline, id, actual_timer=None):
        self.name = name
        self.task_type = task_type
        self.deadline = deadline
        self.actual_timer = actual_timer
        self.data = -1
        # List of tasks that are supposed to read this task's output
        self.subscribers = []
        # Dictionary to track if the output has been read by each subscriber
        self.read_by = {}
        # Priority queue for output written times
        self.output_written_time_buffer = []  
        self.output = None
        self.id = id

    def add_subscriber(self, task):
        self.subscribers.append(task)
        self.read_by[task.name] = False

        
    def read_input(self, current_time, events_buffer):
        buffer_index = (current_time // self.thread_period) % self.buffer_size
        if self.task_type == "Timer":
            if self.actual_timer.check_trigger():
                heapq.heappush(self.output_written_time_buffer, current_time + self.deadline)
                self.data += 1
                self.input_buffer[buffer_index] = self.data
                events_buffer.append((current_time, f"{self.name} Task Read Input", self.data))
                #print("Timer", self.id, self.data, current_time*1000000)
                #print((current_time, f"{self.name} Task Read Input", self.data, "at index", buffer_index))
                
        elif self.task_type == "Subscriber":
            prev_task = self.prev_task
            if prev_task.output is not None and not prev_task.read_by[self.name]:
                heapq.heappush(self.output_written_time_buffer, current_time + self.deadline)
                prev_task.read_by[self.name] = True
                # Check if all read_by flags for the buffer_index are true
                self.input_buffer[buffer_index] = prev_task.output
                events_buffer.append((current_time, f"{self.name} Task Read Input"))
                #print((current_time, f"{self.name} Task Read Input", self.input_buffer[buffer_index], "at index", buffer_index))
                print("Subscriber", self.id, self.input_buffer[buffer_index], current_time*1000000)
                if all(prev_task.read_by.values()):
                    # Clear the output if all subscribers have read the output
                    prev_task.output = None

    def write_output(self, current_time):
        if self.output_written_time_buffer and self.output_written_time_buffer[0] == current_time:
            buffer_index = ((current_time - self.deadline) // self.thread_period) % self.buffer_size
            self.output = self.input_buffer[buffer_index]
            self.input_buffer[buffer_index] = None
            if self.task_type == "Timer":
                print("Timer", self.id, self.data, current_time*1000000)
            #print((current_time, f"{self.name} Task Write Output", self.output, "from index", buffer_index))
            for subscriber in self.subscribers:
                # Mark the output as unread for each subscriber
                self.read_by[subscriber.name] = False
            # Remove the earliest output_written_time from the priority queue
            heapq.heappop(self.output_written_time_buffer)

# Adjust the TaskChain structure
class TaskChain:
    def __init__(self, tasks):
        self.tasks = tasks
        for i in range(1, len(tasks)):
            tasks[i].prev_task = tasks[i-1]
            tasks[i-1].add_subscriber(tasks[i])

    def compute_latency(self):
        if self.tasks[-1].output_available:
            return self.tasks[-1].output_written_time - self.tasks[0].output_written_time
        return None

# Define threads with priority and adjust the simulation to run threads together along a timeline
class Thread:
    def __init__(self, period, priority, tasks, start_time, name, ex_id, actual_timers=None):
        self.period = period
        self.priority = priority
        self.tasks = tasks
        self.timers = actual_timers if actual_timers is not None else []
        self.start_time = start_time
        self.id = ex_id
        self.name = name
        print("ExecutorID", self.name, self.id)
        for task in self.tasks:
            task.buffer_size = math.ceil(task.deadline / self.period)
            task.thread_period = self.period
            task.input_buffer = [None] * task.buffer_size
            print(f"{task.task_type}ID", task.name, task.id)

def simulate_threads(threads, duration):
    events_buffer = []
    current_time = 0

    # Organize threads by their priority
    threads = sorted(threads, key=lambda t: t.priority)

    while current_time <= duration:
        #print("Current time: ", current_time)
        for thread in threads:
            if (current_time - thread.start_time) % thread.period == 0:
                print("Period", thread.id, current_time*1000000)
            for timer in thread.timers:
                timer.update_trigger(current_time)
            if current_time >= thread.start_time:
                for task in thread.tasks:
                    task.write_output(current_time)
                    if (current_time - thread.start_time) % thread.period == 0:
                        task.read_input(current_time, events_buffer)                
        current_time += 1

    return events_buffer


# Simple Test Setup

# Initialize the actual timer
actual_timer1 = ActualTimer(-10, 200)
actual_timer2 = ActualTimer(-10, 420)
actual_timer3 = ActualTimer(-10, 20)
actual_timer4 = ActualTimer(-7, 160)

timer1 = Task("Timer1", "Timer", 10, 1, actual_timer1)
timer2 = Task("Timer2", "Timer", 10, 2, actual_timer2)
timer3 = Task("Timer3", "Timer", 10, 3, actual_timer3)
timer4 = Task("Timer4", "Timer", 80, 4, actual_timer4)

subscriber1 = Task("Subscriber1", "Subscriber", 80, 5)
subscriber2 = Task("Subscriber2", "Subscriber", 80, 6)
subscriber3 = Task("Subscriber3", "Subscriber", 200, 7)
subscriber4 = Task("Subscriber4", "Subscriber", 200, 8)
subscriber5 = Task("Subscriber5", "Subscriber", 200, 9)
subscriber6 = Task("Subscriber6", "Subscriber", 200, 10)
subscriber7 = Task("Subscriber7", "Subscriber", 5, 11)
subscriber8 = Task("Subscriber8", "Subscriber", 5, 12)
subscriber9 = Task("Subscriber9", "Subscriber", 5, 13)
subscriber10 = Task("Subscriber10", "Subscriber", 5, 14)
subscriber11 = Task("Subscriber11", "Subscriber", 5, 15)

tasks1 = [timer1, subscriber1, subscriber3, subscriber7]
tasks2 = [timer2, subscriber2, subscriber4, subscriber8]
tasks3 = [timer4, subscriber5, subscriber9]
tasks4 = [timer2, subscriber2, subscriber6, subscriber10]
tasks5 = [timer3, subscriber11]

executor1 = [timer1, timer2, timer3]
executor2 = [subscriber1, subscriber2, timer4]
executor3 = [subscriber3, subscriber4, subscriber5, subscriber6]
executor4 = [subscriber7, subscriber8, subscriber9, subscriber10, subscriber11]

chain1 = TaskChain(tasks1)
chain2 = TaskChain(tasks2)
chain3 = TaskChain(tasks3)
chain4 = TaskChain(tasks4)
chain5 = TaskChain(tasks5)

# Define tasks for the two threads
timers1 = [actual_timer1, actual_timer2, actual_timer3]
timers2 = [actual_timer4]

# Initialize threads with their respective tasks and priorities
thread1 = Thread(10, 1, executor1, 1, "Executor1", 16, timers1)
thread2 = Thread(20, 2, executor2, 4, "Executor2", 17, timers2)
thread3 = Thread(50, 3, executor3, 6, "Executor3", 18)
thread4 = Thread(10, 4, executor4, 8, "Executor4", 19)

# Run the simulation and capture events
test_events = simulate_threads([thread1, thread2, thread3, thread4], 16500)
print("StartTime", 0)
#print(test_events)


