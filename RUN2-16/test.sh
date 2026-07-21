#!/bin/bash

# Array with values of N to test
VALORI_N=(4 200 500 800 1000 2000 4000 5000)

# Name of the log file (deletes the old file if it exists to start from scratch)
LOG_FILE="test.log"
rm -f "$LOG_FILE"

echo "=== Starting Benchmarks for Matrix Multiplication ==="
echo "Results will be saved in: $LOG_FILE"
echo "------------------------------------------------------------"

# Loop through all values of N
for N in "${VALORI_N[@]}"; do
    echo "Running test for N = $N..."
    
    # Write the separator and the current value of N to the log file
    echo "==================================================" >> "$LOG_FILE"
    echo "N = $N" >> "$LOG_FILE"
    echo "==================================================" >> "$LOG_FILE"
    echo "" >> "$LOG_FILE"

    # 1. Generation of matrices A and B
    echo "[1/4] Generating matrices..." >> "$LOG_FILE"
    ./build/generator "$N" >> "$LOG_FILE" 2>&1
    echo "--------------------------------------------------" >> "$LOG_FILE"

    # 2. Execution of the sequential algorithm
    echo "[2/4] Running sequential algorithm..." >> "$LOG_FILE"
    ./build/mul_sequential >> "$LOG_FILE" 2>&1
    echo "--------------------------------------------------" >> "$LOG_FILE"

    # 3. Execution of the parallel algorithm (with 4 processes)
    echo "[3/4] Running parallel algorithm (MPI -n $SLURM_NTASKS)..." >> "$LOG_FILE"
    mpirun -np $SLURM_NTASKS ./build/mul_parallel >> "$LOG_FILE" 2>&1

    # 4. Correctness check of results (Python script)
    echo "[4/4] Verifying result correctness..." >> "$LOG_FILE" 
    python3 mul_check.py >> "$LOG_FILE" 2>&1
    
    # Add empty lines at the end of each N block to separate from the next one
    echo "" >> "$LOG_FILE"
    echo "" >> "$LOG_FILE"
done

echo "------------------------------------------------------------"
echo "✅ All tests completed! Open '$LOG_FILE' to view the logs."
