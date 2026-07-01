#ifndef LGFPOISSONSOLVER_H_
#define LGFPOISSONSOLVER_H_

#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>
#include <AMReX_BCRec.H>
#include <AMReX_Math.H>
#include <AMReX_ParallelDescriptor.H>
#include <AMReX_BLProfiler.H>

class LGFPoissonSolver
{
public:
    virtual ~LGFPoissonSolver() = default;
    virtual void solvePoisson(const amrex::MultiFab& source, amrex::MultiFab& target,
                          const amrex::Gpu::DeviceVector<int>& source_box_tag_arr) = 0;
};

// free tagSource function, common to all solver types
void tagSource(amrex::Gpu::DeviceVector<int>& box_tag_arr, const amrex::MultiFab& phi, const amrex::Real tag_thresh);

#endif