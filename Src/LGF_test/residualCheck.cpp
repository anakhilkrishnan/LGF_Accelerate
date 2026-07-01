#include <MyFunctions.H>

amrex::MultiFab computeResidual(const amrex::MultiFab& source, const amrex::MultiFab& target, const amrex::Geometry& geom)
{
    BL_PROFILE("<Compute> computeResidual");

    // defining the residual MultiFab
    amrex::MultiFab residual(target.boxArray(), target.DistributionMap(), target.nComp(), 0);

    // evaluating discrete laplacian of target data and subtracting from source
    amrex::GpuArray<amrex::Real, AMREX_SPACEDIM> dx = geom.CellSizeArray();

    for (amrex::MFIter mfi(residual, amrex::TilingIfNotGPU()); mfi.isValid(); ++mfi)
    {
        const amrex::Box& bx = mfi.tilebox();
        auto const& tar_arr = target.const_array(mfi);
        auto const& src_arr = source.const_array(mfi);
        auto const& res_arr  = residual.array(mfi);

        amrex::ParallelFor(bx, [=] AMREX_GPU_DEVICE (int i, int j, int k)
        {
            amrex::Real laplacian = discreteLaplacian(i,j,k, tar_arr, dx);

            res_arr(i,j,k) = src_arr(i,j,k) - laplacian;            

        });
    }

    return residual;
}