#include <AMReX.H>
#include <AMReX_Print.H>
#include <AMReX_PlotFileUtil.H>
#include <AMReX_ParmParse.H>
#include <AMReX_BLProfiler.H>
#include <string>

#include <MyFunctions.H>

using namespace amrex;

int main(int argc, char* argv[])
{
    amrex::Initialize(argc,argv);

    test_print();
    extendedMain();

    amrex::Finalize();
    return 0;
}

void test_print()
{
    amrex::Print() << "This is finally the start of my AMReX code." << "\n";
}

void extendedMain()
{
    // marking start to track runtimes
    auto start_time = amrex::second();

    // variables to be read from ParmParse
    int n_cell, max_grid_size;
    amrex::Real source_tag_thresh;
    amrex::Array<amrex::Real,AMREX_SPACEDIM> phy_dom_lo, phy_dom_hi;
    bool write_plot = false;
    bool use_FMM = false;

    // setting a default plotfile prefix in case not specified in inputs
    std::string plot_prefix = "./Results/plt";

    // reading inputs file
    amrex::ParmParse pp;
    pp.get("n_cell",n_cell);
    pp.get("max_grid_size",max_grid_size);
    pp.get("domain_lo", phy_dom_lo);
    pp.get("domain_hi", phy_dom_hi);
    pp.get("tagging_threshold", source_tag_thresh);

    pp.query("use_FMM", use_FMM);
    pp.query("write_plot", write_plot);
    pp.query("plot_prefix", plot_prefix);

    // initializing parameters for MultiFabs
    int n_ghost = 1;
    int n_comp = 1;

    BoxArray ba;
    Geometry geom;

    // Define the computational domain
    IntVect dom_lo(AMREX_D_DECL(       0,        0,        0));
    IntVect dom_hi(AMREX_D_DECL(n_cell-1, n_cell-1, n_cell-1));
    Box domain(dom_lo, dom_hi);

    // Define the periodicity
    Vector<int> is_periodic(AMREX_SPACEDIM, 0); // 0 = not periodic

    // Initialize the boxarray "ba" from the single box "bx"
    ba.define(domain);
    // Break up boxarray "ba" into chunks no larger than "max_grid_size" along a direction
    ba.maxSize(max_grid_size);

    // This defines the physical box, [0,1] in each direction.
    RealBox real_box(phy_dom_lo, phy_dom_hi);

    // This defines a Geometry object
    geom.define(domain,&real_box,CoordSys::cartesian,is_periodic.data());

    // How Boxes are distrubuted among MPI processes
    DistributionMapping dm(ba);

    // creating source and target multifabs
    amrex::MultiFab sourceMF(ba, dm, n_comp, n_ghost);
    amrex::MultiFab targetMF(ba, dm, n_comp, n_ghost);

    // initializing multifabs
    initializeSourceMultiFab(sourceMF, geom);
    targetMF.setVal(0.0);

    auto compute_start_time = amrex::second();

    // running the tagging algorithmn and obtaining the box tags as an array of 0s and 1s
    amrex::Vector<int> box_tag_arr = tagSource(sourceMF, source_tag_thresh);

    if (use_FMM)
    {
        // using BBFMM2d to compute the result of the LGF problem
        solveFMM(sourceMF, targetMF, geom);
    }
    else
    {
        // performing addition of box values
        addEverySourceBox(sourceMF, targetMF, geom, box_tag_arr);

        // this line is not needed because the code doesn't use the MF again at all, and the plot doesn't use ghost cells
        // targetMF.FillBoundary(geom.periodicity());
    }

    // marking end time and elapsed time
    auto compute_end_time = amrex::second();
    auto compute_time = compute_end_time - compute_start_time;
    amrex::Print() << "Time taken for computation: " << compute_time << "\n";

    // building a MultiFab to visualize the cells that are being tagged
    amrex::MultiFab tagRegion(sourceMF.boxArray(), sourceMF.DistributionMap(), 1, 0);

    for (MFIter mfi(tagRegion); mfi.isValid(); ++mfi) 
    {
        amrex::Real val = (box_tag_arr[mfi.LocalIndex()] == 1) ? 1.0 : 0.0;
        tagRegion[mfi].setVal<RunOn::Device>(val);
    }

    if (write_plot)
    {
        // adding profiling blocks for Tiny/Base profilers
        BL_PROFILE("<I/O> writingPlotfile");

        // building a multiFab with 3 components for plotting
        amrex::MultiFab plotFab(targetMF.boxArray(), targetMF.DistributionMap(), 3, 0);
        amrex::MultiFab::Copy(plotFab, sourceMF, 0, 0, 1, 0); // Component 0
        amrex::MultiFab::Copy(plotFab, targetMF, 0, 1, 1, 0); // Component 1
        amrex::MultiFab::Copy(plotFab, tagRegion, 0, 2, 1, 0); // Component 2

        // exporting the names of the MultiFabs
        amrex::Vector<std::string> varnames = {"Source_Phi", "Target_Phi", "Active_Box_Tag"};

        // writing a simple plotfile
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
