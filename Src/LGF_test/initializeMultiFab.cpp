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
    
    for (MFIter mfi(phi_mf); mfi.isValid(); ++mfi)
    {
        const amrex::Box& vbx = mfi.validbox();
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