#ifndef BBFMM2DLGF_H_
#define BBFMM2DLGF_H_

// headers directly from bbfmm2d library
#include <BBFMM2D.hpp>
#include <kernel_Types.hpp>

// headers for class shell and kernel function
#include <LGFPoissonSolver.H>
#include <bbfmm2dKernel.H>

// headers for cpp features
#include <vector>

class bbfmm2dLGF : public LGFPoissonSolver
{
    private:

    // variables for instantiation
    amrex::Geometry geom;
    int n_lookup;
    int n_chebyshev;
    

    public:

    bbfmm2dLGF(const amrex::Geometry& geom_in, const int n_look_in, const int n_cheb_in);
    // bbfmm2d does not support source truncation, but argument source_box_tag_arr is maintained to be consistent across solvers
    void solvePoisson(const amrex::MultiFab& source, amrex::MultiFab& target, const amrex::Gpu::DeviceVector<int>& source_box_tag_arr) override;
};
#endif