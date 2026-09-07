#!/bin/sh

#SBATCH -N 2
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=48
#SBATCH --time=00:15:00
#SBATCH --job-name=LGF_solver_test
#SBATCH --error=job.%J.err_node_48
#SBATCH --output=job.%J.out_node_48
#SBATCH --partition=debug

module load gnu11/11.4.0
module load openmpi/4.1.2

export OMPI_MCA_btl_openib_allow_ib=1
export OMPI_MCA_btl_openib_if_include="mlx5_0:1"

ulimit -s unlimited

cd $SLURM_SUBMIT_DIR
export OMP_PROC_BIND=close
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
export OMP_DISPLAY_AFFINITY=true

EXEC="./lgfpoisson"
INPUTS_FILE="inputs"
for n in 64 128 256; do
    mpiexec --bind-to none -n $SLURM_NTASKS $EXEC $INPUTS_FILE n_cell=$n max_grid_size=32 plot_prefix=./Results/plt | tee log_n${n}.txt
done

