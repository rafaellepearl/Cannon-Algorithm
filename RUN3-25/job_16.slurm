#!/bin/bash
#SBATCH -J cannon_16
#SBATCH -o cannon_16_%j.out
#SBATCH -e cannon_16_%j.err
#SBATCH -n 16
#SBATCH -p allgroups
#SBATCH -t 01:45:00
#SBATCH --mem 8G

cd $SLURM_SUBMIT_DIR

# Make sure the script is executable
chmod +x test.sh

# Run the benchmark
./test.sh

