#!/bin/bash

#SBATCH --job-name=LGF_Accelerate_run
#SBATCH --partition=gpusinglenode
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=2
#SBATCH --gres=gpu:2
#SBATCH --time=00:30:00
#SBATCH --error=job.%J.err
#SBATCH --output=job.%J.out

# Environment
module purge
module load spack/0.17
module load cmake/3.27.7
module load nvhpc/23.9-gcc-13.1.0-go44
module load gcc/11.2.0-gcc-4.8.5-yqde
module load openmpi/openmpi-nvhpc

export NVHPC_ROOT=$(dirname $(dirname $(dirname $(which nvcc))))
CURAND_DIR=$(dirname $(find "$NVHPC_ROOT" -name "libcurand.so.10" 2>/dev/null | head -1))
export LD_LIBRARY_PATH="$CURAND_DIR:$NVHPC_ROOT/cuda/12.2/lib64:$LD_LIBRARY_PATH"

# MPI / UCX
export OMPI_MCA_btl_openib_allow_ib=1
export OMPI_MCA_btl_openib_if_include="mlx5_0:1"
export OMP_NUM_THREADS=1

export OMPI_MCA_pml=ucx
export OMPI_MCA_osc=ucx
export UCX_TLS=rc,sm,cuda_copy,cuda_ipc
export UCX_MEMTYPE_CACHE=n
export UCX_RNDV_THRESH=8192

AMREX_ARGS="amrex.use_gpu_aware_mpi=0 amrex.abort_on_out_of_gpu_memory=1"

ulimit -s unlimited
cd $SLURM_SUBMIT_DIR
mkdir -p Logs Results

EXEC="./lgfpoisson"
INPUTS_FILE="inputs"

echo "=== environment ==="
module list 2>&1
gcc --version | head -1
nvcc --version | tail -2
echo "NVHPC_ROOT = $NVHPC_ROOT"
echo "CURAND_DIR = $CURAND_DIR"
echo

echo "=== ldd $EXEC ==="
ldd $EXEC | grep -E "not found|curand|cufft|libmpi" || true
if ldd $EXEC | grep -q "not found"; then
    echo "*** unresolved shared libraries -- aborting before launch ***"
    ldd $EXEC | grep "not found"
    exit 1
fi
echo

# One GPU per rank
cat > select_gpu.sh <<'EOF'
#!/bin/bash
export CUDA_VISIBLE_DEVICES=$OMPI_COMM_WORLD_LOCAL_RANK
exec "$@"
EOF
chmod +x select_gpu.sh

# Cases
for n in 64 128 256; do
    echo "=== n_cell = $n ==="
    mpiexec -n $SLURM_NTASKS ./select_gpu.sh $EXEC $INPUTS_FILE \
        n_cell=$n max_grid_size=16 \
        plot_prefix=./Results/plt \
        $AMREX_ARGS \
        2>&1 | tee ./Logs/log_n${n}.txt
done