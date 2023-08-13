#!/bin/bash

# Check if enough arguments are provided
if [ $# -ne 3 ]; then
  echo "Usage: $0 <path_to_executable> <let> <duration>"
  exit 1
fi

# Assigning command-line arguments to variables
PATH_TO_EXECUTABLE="$1"
LET="$2"
DURATION="$3"

# Start the given command in the background
"$PATH_TO_EXECUTABLE" -let "$LET" -ed "$DURATION" &
EXECUTABLE_PID=$!

# Wait for duration minus 1000 milliseconds
sleep $((DURATION - 1000))ms

# Record the data for every 100ms until the executable finishes running
while kill -0 "$EXECUTABLE_PID" 2>/dev/null; do
  for TID in /proc/$EXECUTABLE_PID/task/*/sched; do
    cat "$TID" >> "output_$EXECUTABLE_PID.txt"
  done
  # Sleep for 100ms
  sleep 0.1
done

echo "Data recorded to output_$EXECUTABLE_PID.txt"

# Wait for the command to complete (optional)
wait "$EXECUTABLE_PID"
