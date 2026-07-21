#!/bin/bash
#SBATCH -J cannon_4
#SBATCH -o cannon_4_%j.out
#SBATCH -e cannon_4_%j.err
#SBATCH -n 4
#SBATCH -p allgroups
#SBATCH -t 01:00:00
#SBATCH --mem 4G

cd $SLURM_SUBMIT_DIR

# Make sure the script is executable
chmod +x test.sh

# Run the benchmark
./test.sh
