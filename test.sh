#!/bin/bash

# Array with values of N to test
VALORI_N=(4 200 500 800 1000 2000 4000 5000)

# Use the SLURM Job ID for unique log files
LOG_FILE="test_job_${SLURM_JOB_ID}.log"
rm -f "$LOG_FILE"

echo "=== Starting Benchmarks for Matrix Multiplication ==="
echo "Results will be saved in: $LOG_FILE"
echo "Running with $SLURM_NTASKS MPI tasks."
echo "------------------------------------------------------------"

for N in "${VALORI_N[@]}"; do 
    echo "Running test for N = $N..."
    
    echo "==================================================" >> "$LOG_FILE"
    echo "N = $N" >> "$LOG_FILE"
    echo "==================================================" >> "$LOG_FILE"
    echo "" >> "$LOG_FILE"

    echo "[1/4] Generating matrices..." >> "$LOG_FILE"
    ./build/generator "$N" >> "$LOG_FILE" 2>&1
    
    echo "[2/4] Running sequential algorithm..." >> "$LOG_FILE"
    ./build/mul_sequential >> "$LOG_FILE" 2>&1
    
    # Updated to use SLURM_NTASKS dynamically
    echo "[3/4] Running parallel algorithm (MPI -n 4)..." >> "$LOG_FILE"
    mpirun -np 4 ./build/mul_parallel >> "$LOG_FILE" 2>&1
    
    echo "[4/4] Verifying result correctness..." >> "$LOG_FILE" 
    python3 mul_check.py >> "$LOG_FILE" 2>&1
    
    echo "" >> "$LOG_FILE"
    echo "" >> "$LOG_FILE"
done

echo "✅ Benchmark finished for $SLURM_NTASKS cores!"