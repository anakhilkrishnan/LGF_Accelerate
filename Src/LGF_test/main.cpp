#include <MyFunctions.H>

int main(int argc, char* argv[])
{
    amrex::Initialize(argc,argv);

    amrex::Print() << "Launching test run for LGF Core...\n";
    extendedMain();

    amrex::Finalize();
    return 0;
}

void extendedMain()
{
    // marking start to track runtimes
    auto start_time = amrex::second();

    // variables to be read from ParmParse
    int n_cell, max_grid_size, n_chebyshev, n_lookup, solver_type;
    amrex::Real source_tag_thresh;
    amrex::Array<amrex::Real,AMREX_SPACEDIM> phy_dom_lo, phy_dom_hi;
    bool nodal_compute = false;
    bool write_plot = false;
    bool compare_MLMG = false;

    // setting a default plotfile prefix in case not specified in inputs
    std::string plot_prefix = "./Results/plt";

    // reading inputs file
    amrex::ParmParse pp;
    pp.get("n_cell",n_cell);
    pp.get("max_grid_size",max_grid_size);
    pp.get("domain_lo", phy_dom_lo);
    pp.get("domain_hi", phy_dom_hi);
    pp.get("tagging_threshold", source_tag_thresh);
    pp.get("n_chebyshev", n_chebyshev);
    pp.get("n_lookup", n_lookup);

    pp.query("solver_type", solver_type);
    pp.query("nodal_compute", nodal_compute);
    pp.query("write_plot", write_plot);
    pp.query("plot_prefix", plot_prefix);
    pp.query("compare_MLMG", compare_MLMG);

    // initializing parameters for MultiFabs
    int n_ghost = 1;
    int n_comp = 1;

    amrex::BoxArray ba;
    amrex::Geometry geom;

    // Define the computational domain
    amrex::IntVect dom_lo(AMREX_D_DECL(       0,        0,        0));
    amrex::IntVect dom_hi(AMREX_D_DECL(n_cell-1, n_cell-1, n_cell-1));
    amrex::Box domain(dom_lo, dom_hi);

    // Define the periodicity
    amrex::Vector<int> is_periodic(AMREX_SPACEDIM, 0); // 0 = not periodic

    // Initialize the boxarray "ba" from the single box "bx"
    ba.define(domain);
    // Break up boxarray "ba" into chunks no larger than "max_grid_size" along a direction
    ba.maxSize(max_grid_size);

    // branch for nodal compute, beyond this, always use ba_in_use
    amrex::BoxArray ba_in_use = (nodal_compute) ? amrex::convert(ba, amrex::IntVect::TheNodeVector()) : ba;

    // This defines the physical box, [0,1] in each direction.
    amrex::RealBox real_box(phy_dom_lo, phy_dom_hi);

    // This defines a Geometry object
    geom.define(domain,&real_box,amrex::CoordSys::cartesian,is_periodic.data());

    // How Boxes are distrubuted among MPI processes
    amrex::DistributionMapping dm(ba_in_use);

    
    // creating source and target multifabs
    amrex::MultiFab sourceMF(ba_in_use, dm, n_comp, n_ghost);
    amrex::MultiFab targetMF(ba_in_use, dm, n_comp, n_ghost);

    // initializing multifabs
    initializeSourceMultiFab(sourceMF, geom);
    targetMF.setVal(0.0);

    // creating a PoissonSolver object
    std::unique_ptr<LGFPoissonSolver> poisson_solver;

    if (solver_type == 1)
    {
        // create object for direct summation solver
        poisson_solver = std::make_unique<DirectSumLGF>(geom, n_lookup);    
    }
    else if (solver_type == 2)
    {
        // create object for bbfmm2d solver
        poisson_solver = std::make_unique<bbfmm2dLGF>(geom, n_lookup, n_chebyshev);
    }
    else if (solver_type == 3)
    {   
        // create object for advanced FMM library
        amrex::Abort("Solver_type requested not built yet.");
    }
    else
    {
        amrex::Abort("Solver_type requested does not exist.");
    }

    auto compute_start_time = amrex::second();

    // running the tagging algorithmn and obtaining the box tags as an array of 0s and 1s
    amrex::BoxArray supp_ba;
    tagSource(supp_ba, sourceMF, source_tag_thresh);

    if (!nodal_compute)
    {
        poisson_solver->solvePoisson(sourceMF, targetMF, supp_ba);
    }
    else
    {
        poisson_solver->solveNodalPoisson(sourceMF, targetMF, supp_ba);
    }

    // this line is needed for residual computations
    targetMF.FillBoundary(geom.periodicity());

    // marking end time and elapsed time
    auto compute_end_time = amrex::second();
    auto compute_time = compute_end_time - compute_start_time;
    amrex::Print() << "Time taken for computation: " << compute_time << "\n";

    amrex::MultiFab residual = computeResidual(sourceMF, targetMF, geom);
    amrex::Print() << "Max. Abs. Residual: " << residual.norminf() << "\n";

    amrex::MultiFab targetMLMG(ba_in_use, dm, n_comp, n_ghost);
    targetMLMG.setVal(0.0);
    if (compare_MLMG && !nodal_compute)
    {
        auto mlmg_start_time = amrex::second();
        syncBCs(targetMLMG, targetMF, geom, n_ghost);
        solveMLMG(sourceMF, targetMLMG, geom);
        auto mlmg_end_time = amrex::second();
        auto mlmg_compute_time = mlmg_end_time - mlmg_start_time;
        amrex::Print() << "Time taken for MLMG solve: " << mlmg_compute_time << "\n";
    }
    
    // building a MultiFab to visualize the cells that are being tagged
    amrex::MultiFab tagRegion(ba_in_use, sourceMF.DistributionMap(), 1, 0);

    for (amrex::MFIter mfi(tagRegion); mfi.isValid(); ++mfi)
    {
        const amrex::Box& bx = mfi.validbox();
        tagRegion[mfi].setVal<amrex::RunOn::Device>(supp_ba.intersects(amrex::enclosedCells(bx)) ? 1.0 : 0.0);
    }

    if (write_plot)
    {
        // adding profiling blocks for Tiny/Base profilers
        BL_PROFILE("<I/O> writingPlotfile");

        // building a multiFab with 4 + MLMG components for plotting
        int numPlotComp = (compare_MLMG && !nodal_compute) ? 5 : 4;
        amrex::MultiFab plotFab(targetMF.boxArray(), targetMF.DistributionMap(), numPlotComp, 0);
        amrex::MultiFab::Copy(plotFab, sourceMF, 0, 0, 1, 0);
        amrex::MultiFab::Copy(plotFab, targetMF, 0, 1, 1, 0);
        amrex::MultiFab::Copy(plotFab, tagRegion, 0, 2, 1, 0);
        amrex::MultiFab::Copy(plotFab, residual, 0, 3, 1, 0);

        // exporting the names of the MultiFabs
        amrex::Vector<std::string> varnames = {"Source_Phi", "Target_Phi", "Active_Box_Tag", "Residual"};

        if (compare_MLMG && !nodal_compute)
        {
            amrex::MultiFab::Copy(plotFab, targetMLMG, 0, (numPlotComp-1), 1, 0);
            varnames.push_back("MLMG_Target_Phi");
        }
        
        // writing a single leve plotfile
        const std::string& plotfile_name = amrex::Concatenate(plot_prefix, n_cell);
        amrex::Print() << "Writing plotfile to: " << plotfile_name << "\n";
        WriteSingleLevelPlotfile(plotfile_name, plotFab, varnames, geom, 0.0, 0);
        amrex::Print() << "Plotfile written to: " << plotfile_name << "\n";
    }
    

    auto end_time = amrex::second();
    auto elapsed_time = end_time - start_time;

    // making copies to track slowest and fastest processor
    amrex::Real max_time = elapsed_time;
    amrex::Real min_time = elapsed_time;

    // performing a reduction over all the processors to track the slowest and
    // fastest MPI rank
    const int IOProc = amrex::ParallelDescriptor::IOProcessorNumber();
    amrex::ParallelDescriptor::ReduceRealMax(max_time, IOProc);
    amrex::ParallelDescriptor::ReduceRealMin(min_time, IOProc);

    amrex::Print() << "Max compute time (Slowest Rank): " << max_time << " s\n"
                   << "Min compute time (Fastest Rank): " << min_time << " s\n"
                   << "Time spread (Load Imbalance)   : " << (max_time - min_time) << " s\n";
}
