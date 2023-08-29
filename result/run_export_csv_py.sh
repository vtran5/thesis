#!/bin/bash

# Directory containing the config files
INPUT_DIR="./result/automated_test_result/varied_let"
JSON_DIR="./result/automated_test_json/varied_let"
# Directory to save the results
RESULT_DIR="./result/"

# Loop through all config files matching the pattern "config*.txt" in the specified directory
for JSON_FILE in $JSON_DIR/automated_test*.json
do
  # Extract the number from the filename (e.g., "1" from "automated_test.json")
  NUMBER=$(basename $JSON_FILE | sed -e 's#^automated_test##' -e 's#\.json$##')

  # Define the result file corresponding to the config file
  INPUT_FILE_LET="$INPUT_DIR/data${NUMBER}.txt"
  INPUT_FILE_NOLET="$INPUT_DIR/data${NUMBER}_nolet.txt"
  RESULT_FILE="./result/overhead_profile_varied_let.txt"

  echo "Running with $JSON_FILE $INPUT_FILE_LET $INPUT_FILE_NOLET..."
  echo "$JSON_FILE" >> $RESULT_FILE
  /bin/python3 result/export_csv.py $INPUT_FILE_LET $INPUT_FILE_NOLET $JSON_FILE >> $RESULT_FILE

  echo "Saved results to $RESULT_FILE"
done

echo "All configurations have been executed."
