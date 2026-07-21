#!/bin/bash
#SBATCH -J cannon_25
#SBATCH -o cannon_25_%j.out
#SBATCH -e cannon_25_%j.err
#SBATCH -n 25
#SBATCH -p allgroups
#SBATCH -t 01:30:00
#SBATCH --mem 16G

cd $SLURM_SUBMIT_DIR

chmod +x test.sh
./test.sh
