#!/bin/bash

# Directory containing the config files
CONFIG_DIR1="./rclc_exp/src/config_files/varied_let"
# CONFIG_DIR1="./rclc_exp/src/config_files/varied_pub_num"
# CONFIG_DIR1="./rclc_exp/src/config_files/varied_timer_num"

# Directory to save the results
RESULT_DIR1="./result/automated_test_result/memory/varied_let"
# RESULT_DIR1="./result/automated_test_result/memory/varied_pub_num"
# RESULT_DIR1="./result/automated_test_result/memory/varied_timer_num"

# Loop through all config files matching the pattern "config*.txt" in the specified directory
for CONFIG_FILE1 in $CONFIG_DIR1/config*.txt
do
  # Extract the number from the filename (e.g., "1" from "config1.txt")
  NUMBER=$(basename $CONFIG_FILE1 | sed -e 's/^config//' -e 's/\.txt$//')

  # Define the result file corresponding to the config file
  RESULT_FILE_LET="$RESULT_DIR1/data${NUMBER}.txt"
  RESULT_FILE_NOLET="$RESULT_DIR1/data${NUMBER}_nolet.txt"

  echo "Running with $CONFIG_FILE1..."
  # ./../build/rclc_exp/auto -let true -ed 120000 $CONFIG_FILE1 > $RESULT_FILE_LET 2>&1
  # ./../build/rclc_exp/auto -let false -ed 120000 $CONFIG_FILE1 > $RESULT_FILE_NOLET 2>&1
  stackusage ./../build/rclc_exp/auto -let true -ed 1000 $CONFIG_FILE1 > $RESULT_FILE_LET 2>&1
  stackusage ./../build/rclc_exp/auto -let false -ed 1000 $CONFIG_FILE1 > $RESULT_FILE_NOLET 2>&1
  echo "Saved results to $RESULT_FILE_LET and $RESULT_FILE_NOLET"
done

echo "All configurations have been executed."
