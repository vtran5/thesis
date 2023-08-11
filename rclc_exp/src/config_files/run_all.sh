#!/bin/bash

# Directory containing the config files
CONFIG_DIR="./src/rclc_exp/src/config_files"

# Directory to save the results
RESULT_DIR="./src/result/automated_test_result"

# Loop through all config files matching the pattern "config*.txt" in the specified directory
for CONFIG_FILE in $CONFIG_DIR/config*.txt
do
  # Extract the number from the filename (e.g., "1" from "config1.txt")
  NUMBER=$(basename $CONFIG_FILE | sed -e 's/^config//' -e 's/\.txt$//')

  # Define the result file corresponding to the config file
  RESULT_FILE_LET="$RESULT_DIR/data${NUMBER}.txt"
  RESULT_FILE_NOLET="$RESULT_DIR/data${NUMBER}_nolet.txt"

  echo "Running with $CONFIG_FILE..."
  ros2 run rclc_exp auto -let true $CONFIG_FILE > $RESULT_FILE_LET 2>&1
  ros2 run rclc_exp auto -let false $CONFIG_FILE > $RESULT_FILE_NOLET 2>&1

  echo "Saved results to $RESULT_FILE"
done

echo "All configurations have been executed."
