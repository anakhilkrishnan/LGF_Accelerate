#include <LGFOpenBC.H>

#include <AMReX_ParmParse.H>

using namespace amrex;

LGFOpenBC::LGFOpenBC (const Geometry& geom_in, int n_look_in, int box_quantum_in)
    : geom(geom_in), n_lookup(n_look_in), box_quantum(std::max(1, box_quantum_in)),
      m_cached_domain(IntVect(AMREX_D_DECL(0,0,0)), IntVect(AMREX_D_DECL(-1,-1,-1)))
{
    // m_cached_domain starts empty (hi < lo) so the first solve always builds.
}


amrex::IntVect LGFOpenBC::paddedLength () const
{
    return m_solver ? m_solver->PaddedLength() : IntVect(AMREX_D_DECL(0,0,0));
}


Box LGFOpenBC::makeDomain (const MultiFab& target, const BoxArray& tag_ba) const
{
    // The transform domain must cover BOTH:
    //   (a) the tagged sources -- otherwise ParallelCopy silently drops them
    //   (b) the region where u is wanted, including ghosts if backward_doit is
    //       to fill them -- otherwise the solution is silently truncated
    //
    // In the testbench (a) and (b) coincide, so getting this wrong would not
    // show up here. In NSE_GPU the evaluation region is generally larger than
    // the tagged support, and the failure mode is a solution that looks right
    // in the interior and is zero near the edges.
    Box bx = amrex::grow(target.boxArray().minimalBox(), target.nGrowVect());
    if (!tag_ba.empty()) {
        bx.minBox(tag_ba.minimalBox());
    }

    // Quantise so that a snug domain jittering by a few cells between regrids
    // keeps hitting the same cached solver. Growing only on the high side
    // keeps the lo corner fixed, which matters because the Green's function
    // functor is written relative to domain.smallEnd().
    if (box_quantum > 1) {
        IntVect len = bx.length();
        for (int idim = 0; idim < AMREX_SPACEDIM; ++idim) {
            int q = ((len[idim] + box_quantum - 1) / box_quantum) * box_quantum;
            bx.growHi(idim, q - len[idim]);
        }
    }
    return bx;
}


void LGFOpenBC::ensureSolver (const Box& domain)
{
    // BoxArray/Box comparison here is a plain value compare, not the
    // permutation-sensitive BoxArray::operator== that caused the latching
    // regrid bug -- Box has no distribution to disagree about.
    if (m_solver && domain == m_cached_domain) { return; }

    BL_PROFILE("<Setup> LGFOpenBC::ensureSolver");

    FFT::Info info;
    info.setOpenBCPadding(true);   // default, but be explicit: rounds the
                                   // one-sided length via FFT::nextFastLen
                                   // before doubling, which is what keeps a
                                   // prime-sized snug domain from falling into
                                   // a cuFFT Bluestein path.

    m_solver = std::make_unique<FFT::OpenBCSolver<Real>>(domain, info);
    m_cached_domain = domain;

    // ---- the Green's function functor -------------------------------------
    //
    // setGreensFunction calls f(ii+lo.x, jj+lo.y, kk+lo.z) where ii,jj,kk are
    // ZERO-BASED offsets into the (padded, doubled) domain. So the lattice
    // offset is (arg - domain.smallEnd()). AMReX handles the mirroring and the
    // zeroed middle planes internally -- we never touch the wrap-around layout.
    //
    // Scaling: with L_h = h^-2 L,  u = h^2 * sum_m G(n-m) f(m). The prefactor
    // is h^2 in EVERY dimension, NOT the cell volume. (In 2D h^2 == dvol, which
    // is why directSumLGF's dvol works there by coincidence and not in 3D.)
    // solve()'s internal scaling_factor is only the 1/N FFT normalisation, so
    // h^2 has to ride along here.
    //
    // Everything captured is POD; the functor is copied into a device lambda.
    const auto  glo = domain.smallEnd().dim3();
    const auto  dx  = geom.CellSizeArray();
    const Real  h2  = dx[0] * dx[1];
    const int   nlk = n_lookup;

    m_solver->setGreensFunction(
        [=] AMREX_GPU_DEVICE (int i, int j, int k) -> Real
        {
            // Reuse computeLGF() unchanged rather than adding an offset-based
            // entry point. It takes physical coordinates and rounds internally,
            // so feeding it (offset*dx, 0) reproduces the integer offset
            // exactly. The point is that DirectSumLGF and LGFOpenBC then call
            // bit-identical kernel code -- any discrepancy between the two
            // backends is in the plumbing, not the kernel.
            AMREX_D_TERM(const Real xt = Real(i - glo.x) * dx[0];,
                         const Real yt = Real(j - glo.y) * dx[1];,
                         const Real zt = Real(k - glo.z) * dx[2];)

            return h2 * computeLGF(nlk,
                                   AMREX_D_DECL(xt, yt, zt),
                                   AMREX_D_DECL(Real(0.0), Real(0.0), Real(0.0)),
                                   AMREX_D_DECL(dx[0], dx[1], dx[2]));
        });

    amrex::Print() << "LGFOpenBC: built solver on " << domain
                   << ", padded one-sided length " << m_solver->PaddedLength() << "\n";
}


void LGFOpenBC::doSolve (const MultiFab& source, MultiFab& target,
                         const BoxArray& tag_ba)
{
    BL_PROFILE("<Compute> LGFOpenBC::solve()");

    AMREX_ALWAYS_ASSERT_WITH_MESSAGE(source.ixType() == target.ixType(),
        "LGFOpenBC: source and target must share an index type");

    // backward_doit writes only where phi overlaps the solver domain, so
    // anything outside would otherwise retain stale values.
    target.setVal(0.0);

    if (tag_ba.empty()) { return; }   // no sources => u == 0 everywhere

    // Restrict the RHS to the tagged support. OpenBCSolver has no notion of
    // tagging, so the truncation has to happen before it sees the data.
    // Cheap: one masked copy on the existing layout.
    MultiFab rhs(source.boxArray(), source.DistributionMap(), 1, 0);
    rhs.setVal(0.0);
    for (MFIter mfi(rhs, TilingIfNotGPU()); mfi.isValid(); ++mfi) {
        const Box& bx = mfi.tilebox();
        if (!tag_ba.intersects(amrex::enclosedCells(mfi.validbox()))) { continue; }
        auto const& src = source.const_array(mfi);
        auto const& dst = rhs.array(mfi);
        ParallelFor(bx, [=] AMREX_GPU_DEVICE (int i, int j, int k)
        {
            dst(i,j,k) = src(i,j,k);
        });
    }

    ensureSolver(makeDomain(target, tag_ba));
    m_solver->solve(target, rhs);
}


void LGFOpenBC::solvePoisson (const MultiFab& source, MultiFab& target,
                              const BoxArray& tag_ba)
{
    doSolve(source, target, tag_ba);
}


void LGFOpenBC::solveNodalPoisson (const MultiFab& source, MultiFab& target,
                                   const BoxArray& tag_ba)
{
    // OpenBCSolver preserves domain.ixType() through make_grown_domain, and the
    // lattice offsets are node-to-node exactly as they are cell-to-cell, so the
    // same path serves both. No OwnerMask is needed: ParallelCopy resolves
    // shared seam nodes, unlike the manual packing in consolidateMultiFab.
    doSolve(source, target, tag_ba);
}


void LGFOpenBC::regridOnto (const Geometry& new_geom, const BoxArray& new_ba,
                            const DistributionMapping& new_dm)
{
    amrex::ignore_unused(new_ba, new_dm);
    geom = new_geom;

    // Drop the cache. ensureSolver() rebuilds lazily on the next solve, and
    // only if the quantised domain actually changed -- so a regrid that keeps
    // the same bounding-box dimensions costs nothing.
    m_cached_domain = Box(IntVect(AMREX_D_DECL(0,0,0)),
                          IntVect(AMREX_D_DECL(-1,-1,-1)));
}
