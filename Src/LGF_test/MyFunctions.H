#ifndef MYFUNCTIONS_H_
#define MYFUNCTIONS_H_

#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>
#include <AMReX_BCRec.H>
#include <AMReX.H>
#include <AMReX_Print.H>
#include <AMReX_PlotFileUtil.H>
#include <AMReX_ParmParse.H>
#include <AMReX_BLProfiler.H>
#include <string>

#include <LGFPoissonSolver.H>
#include <directSumLGF.H>

using namespace amrex;

void extendedMain();

void initializeSourceMultiFab(amrex::MultiFab& phifab, amrex::Geometry& geom);

void syncBCs(amrex::MultiFab& phi_mf_dest, amrex::MultiFab& phi_mf_src, amrex::Geometry& phi_geom, int n_ghost);
void solveMLMG(const amrex::MultiFab& source, amrex::MultiFab& target, const amrex::Geometry& geom);

amrex::MultiFab computeResidual(const amrex::MultiFab& source, const amrex::MultiFab& target, const amrex::Geometry& geom);

// function for computing 5-pt laplacian for residual
AMREX_FORCE_INLINE AMREX_GPU_DEVICE
amrex::Real discreteLaplacian(int i, int j, int k,
                            amrex::Array4<amrex::Real const> const& tar,
                            amrex::GpuArray<amrex::Real, AMREX_SPACEDIM> const& dx)
{
    amrex::Real lap = 0.0;

    lap += (tar(i+1,j,k) - 2 * tar(i,j,k) + tar(i-1,j,k)) / (dx[0] * dx[0]);
#if AMREX_SPACEDIM >= 2
    lap += (tar(i,j+1,k) - 2 * tar(i,j,k) + tar(i,j-1,k)) / (dx[1] * dx[1]);
#endif
#if AMREX_SPACEDIM == 3
    lap += (tar(i,j,k+1) - 2 * tar(i,j,k) + tar(i,j,k-1)) / (dx[2] * dx[2]);
#endif
    return lap;
}

#endif