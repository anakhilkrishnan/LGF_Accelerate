#include <AMReX_MLMG.H>
#include <AMReX_MLABecLaplacian.H>
#include <AMReX_MultiFabUtil.H>
#include <MyFunctions.H>

void solveMLMG(const amrex::MultiFab& source, amrex::MultiFab& target, const amrex::Geometry& geom)
{
    BL_PROFILE("<Compute> solveMLMG()");

    amrex::LPInfo info;
    amrex::MLABecLaplacian mlabec({geom}, {target.boxArray()}, {target.DistributionMap()}, info);

    // 1. Set Boundary Condition Types (Dirichlet all around)
    std::array<amrex::LinOpBCType, AMREX_SPACEDIM> lo_bc;
    std::array<amrex::LinOpBCType, AMREX_SPACEDIM> hi_bc;
    for (int idim = 0; idim < AMREX_SPACEDIM; ++idim) {
        lo_bc[idim] = amrex::LinOpBCType::Dirichlet;
        hi_bc[idim] = amrex::LinOpBCType::Dirichlet;
    }
    mlabec.setDomainBC(lo_bc, hi_bc);

    // 2. Set the Operator Coefficients (alpha = 0, beta = -1 for Poisson)
    mlabec.setScalars(0.0, -1.0);

    // Set B coefficients to 1.0 across all faces
    amrex::Array<amrex::MultiFab, AMREX_SPACEDIM> face_bcoef;
    for (int idim = 0; idim < AMREX_SPACEDIM; ++idim) {
        const amrex::BoxArray& edgeba = amrex::convert(target.boxArray(), amrex::IntVect::TheDimensionVector(idim));
        face_bcoef[idim].define(edgeba, target.DistributionMap(), 1, 0);
        face_bcoef[idim].setVal(1.0);
    }
    mlabec.setBCoeffs(0, amrex::GetArrOfConstPtrs(face_bcoef));

    // 3. Apply Boundary Values
    // MLMG reads the ghost cells of the 'target' MultiFab to enforce the Dirichlet values.
    // Ensure you have populated target's ghost cells with the analytical exact potential before calling this!
    mlabec.setLevelBC(0, &target);

    // 4. Initialize and Run Solver
    amrex::MLMG mlmg(mlabec);
    mlmg.setVerbose(1);       // Prints step-by-step iteration residuals
    mlmg.setBottomVerbose(0);

    // Target a residual tolerance of 1e-12 to match your FMM goal
    const amrex::Real tol_rel = 1.0e-12;
    const amrex::Real tol_abs = 0.0;
    
    mlmg.solve({&target}, {&source}, tol_rel, tol_abs);
}