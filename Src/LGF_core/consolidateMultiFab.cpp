#include <AMReX_MultiFab.H>
#include <AMReX_ParallelDescriptor.H>
#include <AMReX_BLProfiler.H>

#include <LGFCore.H>

using namespace amrex;

ConsolidatedData consolidateMultiFab(const amrex::MultiFab& phifab, const amrex::Geometry& geom, amrex::Vector<int> box_tag_arr)
{
    // adding profiling blocks for Tiny/Base profilers
    BL_PROFILE("<Communicate> consolidateMultiFab()");


    const amrex::GpuArray<amrex::Real, AMREX_SPACEDIM> dx = geom.CellSizeArray();

    // Each MPI rank stores its data and metadata into a sinle linearized vector
    Vector<Real> local_data;
    Vector<FabMetaData> local_meta;

    // declaring a CPU side host_mf to pack source data
    amrex::MultiFab host_mf(phifab.boxArray(), phifab.DistributionMap(), phifab.nComp(), phifab.nGrow(), amrex::MFInfo().SetArena(amrex::The_Pinned_Arena()));

    // copying data in bulk only once
    amrex::dtoh_memcpy(host_mf, phifab);

    for (MFIter mfi(host_mf); mfi.isValid(); ++mfi) 
    {
        // if the current box is not tagged, skip
        if (box_tag_arr[mfi.LocalIndex()] == 0)
        {
            continue;
        }

        // pack box values if empty
        const Box& bx = mfi.validbox();

        // phi_arr extracted from the Box that was copied to the CPU
        auto const& phi_arr = host_mf.const_array(mfi);

        // Metadata includes offset for each box and global indices for box lo and box hi
        local_meta.push_back({static_cast<int>(local_data.size()), bx.smallEnd(), bx.bigEnd(), geom.CellSizeArray()});

        // Fills the linear vector with data
        for (int k = AMREX_D_PICK(0, 0, bx.smallEnd()[2]); k <= AMREX_D_PICK(0, 0, bx.bigEnd()[2]); ++k) 
        {
            for (int j = AMREX_D_PICK(0, bx.smallEnd()[1], bx.smallEnd()[1]); j <= AMREX_D_PICK(0, bx.bigEnd()[1], bx.bigEnd()[1]); ++j) 
            {
                for (int i = bx.smallEnd()[0]; i <= bx.bigEnd()[0]; ++i) 
                {
                    local_data.push_back(phi_arr(i, j, k));
                }
            }
        }
    }

    // Setting up data for an AllGather() operation
    // Find number of processes and initialize data counts and index markers
    int nprocs = ParallelDescriptor::NProcs();

    // Fix the IO Processor as the one that retrieves and broadcasts
    int root = ParallelDescriptor::IOProcessorNumber();

    // Initialize data counts and index markers
    Vector<int> data_counts(nprocs), data_displs(nprocs + 1, 0);
    Vector<int> meta_counts(nprocs), meta_displs(nprocs + 1, 0);

    int my_data_size = static_cast<int>(local_data.size());
    int my_meta_size = static_cast<int>(local_meta.size());

    // Exchange data sizes across all processes
    ParallelDescriptor::Gather(&my_data_size, 1, data_counts.data(), 1, root);
    ParallelDescriptor::Gather(&my_meta_size, 1, meta_counts.data(), 1, root);
    ParallelDescriptor::Bcast(data_counts.data(), nprocs, root);
    ParallelDescriptor::Bcast(meta_counts.data(), nprocs, root);

    // Calculate displacements for the unified global buffers
    for (int i = 0; i < nprocs; ++i) 
    {
        data_displs[i+1] = data_displs[i] + data_counts[i];
        meta_displs[i+1] = meta_displs[i] + meta_counts[i];
    }

    // Allocate global host buffers for the full dataset
    Vector<Real> global_data(data_displs[nprocs]);
    Vector<FabMetaData> global_meta(meta_displs[nprocs]);

    ParallelDescriptor::Gatherv(local_data.data(), my_data_size, global_data.data(), data_counts, data_displs, root);
    ParallelDescriptor::Bcast(global_data.data(), global_data.size(), root);

    // Calculate byte-wise counts for the metadata struct
    Vector<int> m_byte_counts(nprocs);
    Vector<int> m_byte_displs(nprocs + 1, 0);
    for (int i = 0; i < nprocs; ++i) 
    {
        m_byte_counts[i] = meta_counts[i] * sizeof(FabMetaData);
        m_byte_displs[i+1] = m_byte_displs[i] + m_byte_counts[i];
    }

    // Treat metadata as char* (bytes) to avoid Mpi_typemap lookup
    // The char* cast is a workaround for the fact that MPI does not know FabMetaData (struct that I built)
    ParallelDescriptor::Gatherv(reinterpret_cast<char*>(local_meta.data()), 
                                static_cast<int>(my_meta_size * sizeof(FabMetaData)), 
                                reinterpret_cast<char*>(global_meta.data()), 
                                m_byte_counts, m_byte_displs, root);
    
    ParallelDescriptor::Bcast(reinterpret_cast<char*>(global_meta.data()), 
                                static_cast<int>(global_meta.size() * sizeof(FabMetaData)), root);

    // Adjust the local offsets to point correctly into the unified global_data array
    for (int p = 0; p < nprocs; ++p) 
    {
        for (int b = meta_displs[p]; b < meta_displs[p+1]; ++b) 
        {
            global_meta[b].offset += data_displs[p];
        }
    }

    // Pack data and return
    ConsolidatedData consolPhi;
    consolPhi.data = std::move(global_data);
    consolPhi.metadata = std::move(global_meta);

    return consolPhi;
}