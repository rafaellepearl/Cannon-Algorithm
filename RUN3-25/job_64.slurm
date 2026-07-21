#!/bin/bash
#SBATCH -J cannon_64
#SBATCH -o cannon_64_%j.out
#SBATCH -e cannon_64_%j.err
#SBATCH -n 64
#SBATCH -p allgroups
#SBATCH -t 01:00:00
#SBATCH --mem 16G

cd $SLURM_SUBMIT_DIR

chmod +x test.sh
./test.sh
