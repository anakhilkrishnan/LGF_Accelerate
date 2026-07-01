#include <AMReX.H>
#include <AMReX_MultiFab.H>
#include <AMReX_BLProfiler.H>

#include <MyFunctions.H>
#include <SourceField.H>

using namespace amrex;

void initializeSourceMultiFab(amrex::MultiFab& phi_mf, amrex::Geometry& phi_geom)
{
    // adding profiling blocks for Tiny/Base profilers
    BL_PROFILE("<Setup> initializeSourceMultiFab");

    // extracting physical dx, physical domain lo for computing x,y,z
    GpuArray<amrex::Real, AMREX_SPACEDIM> dx = phi_geom.CellSizeArray();
    GpuArray<amrex::Real, AMREX_SPACEDIM> prob_lo = phi_geom.ProbLoArray();
    
#ifdef AMREX_USE_OMP
    #pragma omp parallel if (amrex::Gpu::notInLaunchRegion())
#endif
    for (MFIter mfi(phi_mf, amrex::TilingIfNotGPU()); mfi.isValid(); ++mfi)
    {
        const amrex::Box& vbx = mfi.tilebox();
        auto const& phiarr = phi_mf.array(mfi);
        ParallelFor(vbx, [=] AMREX_GPU_DEVICE(int i, int j, int k)
            {
                // compute the physical coordinates of the cell center
                amrex::Real x = prob_lo[0] + (i + 0.5) * dx[0];
                amrex::Real y = (AMREX_SPACEDIM >= 2) ? prob_lo[1] + (j + 0.5) * dx[1] : 0.0;
                amrex::Real z = (AMREX_SPACEDIM == 3) ? prob_lo[2] + (k + 0.5) * dx[2] : 0.0;

                phiarr(i,j,k) = sourceField(x,y,z);
                
            });
    }
}

void syncBCs(amrex::MultiFab& phi_mf_dest, amrex::MultiFab& phi_mf_src, amrex::Geometry& phi_geom, int n_ghost)
{
    // adding profiling blocks for Tiny/Base profilers
    BL_PROFILE("<Setup> Extracting FMM Boundaries for MLMG");
        
    for (MFIter mfi(phi_mf_dest, amrex::TilingIfNotGPU()); mfi.isValid(); ++mfi) 
    {
        const Box& gbx = mfi.growntilebox(n_ghost);
        const Box& vbx = mfi.validbox(); // The strict interior box
        auto const& mlmg_arr = phi_mf_dest.array(mfi);
        auto const& fmm_arr = phi_mf_src.array(mfi);
        
        ParallelFor(gbx, [=] AMREX_GPU_DEVICE(int i, int j, int k) 
        {
            // Only populate the ghost cells (the Dirichlet boundaries)
            // The interior remains 0.0 to force MLMG to solve it
            if (!vbx.contains(i, j, k)) {
                mlmg_arr(i,j,k) = fmm_arr(i,j,k);
            }
        });
    }
}