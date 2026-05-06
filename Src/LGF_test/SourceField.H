#ifndef SOURCEFIELD_H_
#define SOURCEFIELD_H_

#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>
#include <AMReX_BCRec.H>
#include <AMReX_MultiFab.H>
#include <AMReX_Math.H>


// function for computing lgf kernel inside parallelFor
AMREX_GPU_DEVICE AMREX_FORCE_INLINE
amrex::Real sourceField(amrex::Real x, amrex::Real y, amrex::Real z)
    {
        // parameters of the gaussian distribution
        amrex::Real cent_x = 0.0;
        #if AMREX_SPACEDIM == 2
            amrex::Real cent_y = 0.25;
        #endif    
        #if AMREX_SPACEDIM == 3
            amrex::Real cent_y = 0.25;
            amrex::Real cent_z = 0.0;
        #endif
        amrex::Real variance = 0.5;

        amrex::Real r_sq = AMREX_D_TERM((x - cent_x)*(x - cent_x),
                                           + (y - cent_y)*(y - cent_y),
                                           + (z - cent_z)*(z - cent_z));
        
        amrex::Real f = std::exp(-r_sq / variance); 
        
        return f;                                
    }
#endif