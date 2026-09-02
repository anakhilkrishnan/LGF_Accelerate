#include <LGFPoissonSolver.H>

using namespace amrex;

void tagSource(amrex::BoxArray& tag_ba, const amrex::MultiFab& phi, const amrex::Real tag_thresh)
{
    // creating boxArray that signifies support region, always cell-centered

    // adding profiling blocks for Tiny/Base profilers
    BL_PROFILE("<Compute> tagSource()");

    // normalize threshold based on incoming field
    const amrex::Real phi_max = phi.norm0(0, 0, false);
    if (phi_max <= 0.0) { tag_ba = amrex::BoxArray(); return; }
    const amrex::Real abs_thresh = tag_thresh * phi_max;

    // ensure correct sized device vectors
    const int num_local_boxes = phi.local_size();
    amrex::Gpu::DeviceVector<int> supp_tag_arr(num_local_boxes, 0);

    // obtain raw pointers for GPU
    int* d_flags_ptr = supp_tag_arr.dataPtr();
    
    auto const& phi_arrs = phi.const_arrays();
    amrex::ParallelFor(phi, [=] AMREX_GPU_DEVICE (int box_no, int i, int j, int k)
    {
        if (amrex::Math::abs(phi_arrs[box_no](i,j,k)) > abs_thresh) 
        {
            amrex::Gpu::Atomic::Max(&d_flags_ptr[box_no], 1);
        }
    });

    amrex::Vector<int> h_tag_arr(num_local_boxes);
    amrex::Gpu::copy(amrex::Gpu::deviceToHost, supp_tag_arr.begin(), supp_tag_arr.end(), h_tag_arr.begin());

    // tag_ba is ALWAYS cell-centered, whatever the index type of phi.
    // Both consumers assume this: consolidateMultiFab tests
    // tag_ba.intersects(enclosedCells(bx)), and main.cpp tests against a
    // cell-centered tagRegion.
    amrex::Vector<amrex::Box> local_tagged_boxes;
    for (amrex::MFIter mfi(phi); mfi.isValid(); ++mfi) 
    {
        if (h_tag_arr[mfi.LocalIndex()] != 0) 
        {
            local_tagged_boxes.push_back(amrex::enclosedCells(mfi.validbox()));
        }
    }

    // gather across all ranks for boxes, updates in place and create box list
    amrex::AllGatherBoxes(local_tagged_boxes);
    tag_ba = amrex::BoxArray(amrex::BoxList(std::move(local_tagged_boxes)));
}