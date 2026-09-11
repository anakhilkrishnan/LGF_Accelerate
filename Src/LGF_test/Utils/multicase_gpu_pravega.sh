#!/bin/bash

#SBATCH --job-name=LGF_Accelerate_run
#SBATCH --partition=gpusinglenode
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=2
#SBATCH --cpus-per-task=20
#SBATCH --gres=gpu:2
#SBATCH --time=00:10:00
#SBATCH --error=job.%J.err
#SBATCH --output=job.%J.out

set -o pipefail

# Environment
module purge
module load cmake/3.27.7
module load spack/0.17
. /home-ext/apps/spack/share/spack/setup-env.sh
module load nvhpc/23.9-gcc-13.1.0-go44
module load gcc/11.2.0-gcc-4.8.5-yqde

export OMPI_CC=gcc OMPI_CXX=g++
NVROOT=$(dirname $(dirname $(which nvc++)))
MPIROOT=$NVROOT/../comm_libs/12.2/openmpi4/openmpi-4.1.5
export PATH=$MPIROOT/bin:$PATH
export LD_LIBRARY_PATH=$MPIROOT/lib:$LD_LIBRARY_PATH
CUDALIBS=$NVROOT/../math_libs/12.2/targets/x86_64-linux/lib
CUDART=$NVROOT/../cuda/12.2/targets/x86_64-linux/lib
export LD_LIBRARY_PATH=$MPIROOT/lib:$CUDALIBS:$CUDART:$LD_LIBRARY_PATH

# MPI / UCX
export OMP_NUM_THREADS=1

# AMReX Arguments
AMREX_ARGS="amrex.abort_on_out_of_gpu_memory=1 amrex.use_gpu_aware_mpi=1"

ulimit -s unlimited
cd $SLURM_SUBMIT_DIR
mkdir -p Logs Results

EXEC="./lgfpoisson"
INPUTS_FILE="inputs"

if ldd $EXEC | grep -q "not found"; then
    echo "UNRESOLVED:"; ldd $EXEC | grep "not found"; exit 1
fi

for n in 1024 2048 4096 8192; do
    echo "=== n_cell = $n | 2D ==="
    mpiexec -n $SLURM_NTASKS --map-by socket --bind-to socket --report-bindings $EXEC $INPUTS_FILE \
        n_cell=$n max_grid_size=512 \
        tagging_threshold=0.0 \
        plot_prefix=./Results/plt \
        $AMREX_ARGS \
        2>&1 | tee ./Logs/log_n${n}.txt
done
