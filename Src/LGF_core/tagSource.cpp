#include <LGFCore.H>

using namespace amrex;

void tagSource(amrex::Gpu::DeviceVector<int>& d_is_box_tagged, const amrex::MultiFab& phifab, const amrex::Real source_threshold)
{
    // perform grid tagging by assigning an int to each box
    // 0 = to be excluded during packing
    // 1 = to be packed and shipped

    // adding profiling blocks for Tiny/Base profilers
    BL_PROFILE("<Communicate> tagSource()");

    // resetting all values in d_is_box_tagged to zero asynchronously
    amrex::Gpu::fill_async(d_is_box_tagged.begin(), d_is_box_tagged.end(), 0);

    // obtaining raw pointers for GPU
    int* d_flags_ptr = d_is_box_tagged.dataPtr();

#ifdef AMREX_USE_OMP
    #pragma omp parallel if (amrex::Gpu::notInLaunchRegion())
#endif
    for(MFIter mfi(phifab, amrex::TilingIfNotGPU()); mfi.isValid(); ++mfi)
    {
        const Box& bx = mfi.tilebox();
        auto const& phi_arr = phifab.const_array(mfi);
        const int local_idx = mfi.LocalIndex();

        // check every box for threshold breach. if any true obtained, write d_is_box_tagged as 1
        // race condition might happen here (?), find out
        amrex::ParallelFor(bx, [=] AMREX_GPU_DEVICE (int i, int j, int k)
        {
            if (amrex::Math::abs(phi_arr(i,j,k)) > source_threshold)
            {
                amrex::Gpu::Atomic::Max(&d_flags_ptr[local_idx], 1);
            }
        });
    }
}