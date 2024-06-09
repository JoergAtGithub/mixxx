#!/bin/bash

COMPILER=$1
FLAGS=$2
FILE=$3

# Compile the code with vectorization enabled and save the compiler's output
$COMPILER "$FLAGS" "$FILE" > output.txt

# Extract the names of all functions in the file
FUNCTIONS=$(grep -oP '(\w+)\s*\([^)]*\)\s*\{' "$FILE" | grep -oP '^\w+')

# Check the output for each function
for function in $FUNCTIONS; do
  if grep -q "$function.*loop vectorized" output.txt; then
    echo "$(uname -s), $COMPILER, $function, Yes"
  else
    echo "$(uname -s), $COMPILER, $function, No"
  fi
done
