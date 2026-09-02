#ifndef LGFPOISSONSOLVER_H_
#define LGFPOISSONSOLVER_H_

#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>
#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_Math.H>
#include <AMReX_ParallelDescriptor.H>
#include <AMReX_BLProfiler.H>

class LGFPoissonSolver
{
public:
    virtual ~LGFPoissonSolver() = default;

    // cell-centered solve. tag_ba selects which source boxes participate.
    virtual void solvePoisson(const amrex::MultiFab& source, amrex::MultiFab& target,
                              const amrex::BoxArray& tag_ba) = 0;

    // nodal solve. Backends that cannot do nodal data inherit this.
    virtual void solveNodalPoisson(const amrex::MultiFab& source, amrex::MultiFab& target,
                                   const amrex::BoxArray& tag_ba)
    {
        amrex::ignore_unused(source, target, tag_ba);
        amrex::Abort("solveNodalPoisson() not implemented for this backend");
    }

    // rebuild any cached state after the snug domain changes. Backends that
    // hold nothing across solves (e.g. direct summation) need do nothing;
    // FFT backends must rebuild their cached plan here.
    virtual void regridOnto(const amrex::Geometry& new_geom, const amrex::BoxArray& new_ba,
                            const amrex::DistributionMapping& new_dm)
    {
        amrex::ignore_unused(new_geom, new_ba, new_dm);
    }
};

// free tagSource function that retuns a boxArray, common to all solver types
void tagSource(amrex::BoxArray& tag_ba, const amrex::MultiFab& phi, const amrex::Real tag_thresh);

#endif