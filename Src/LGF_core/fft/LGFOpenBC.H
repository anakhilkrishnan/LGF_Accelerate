#ifndef LGFOPENBC_H_
#define LGFOPENBC_H_

#include <LGFPoissonSolver.H>
#include <directSumKernel.H>

#include <AMReX_FFT.H>

#include <memory>

/**
 * FFT-based free-space LGF Poisson solver.
 *
 *   u(n) = h^2 * sum_m G(n-m) f(m)
 *
 * evaluated as a single zero-padded convolution using amrex::FFT::OpenBCSolver.
 * This is the "FFT Conv." case of Liska & Colonius (2014) -- the special case
 * where the entire active grid is one block and the FLGF method reduces to a
 * single FFT-based convolution. Their Table 2 puts it at 3.54e5 pts/s against
 * 1.19e5 for the full multilevel FMM at 1e8 points, i.e. FASTER than the tree
 * on a box, so this is not a stepping stone -- it is the fastest configuration
 * for boxed domains, and O(N log N) rather than O(N^2).
 *
 * WHY THIS NEEDS ALMOST NO PLUMBING
 *
 *   OpenBCSolver::solve() does
 *       inmf.setVal(0); inmf.ParallelCopy(rho, 0, 0, 1);   ... forward
 *       ... multiply by cached spectral G ...
 *       m_r2c.backward_doit(phi, phi.nGrowVect());
 *   so rho and phi may live on ANY BoxArray overlapping the solver domain.
 *   The snug BoxArray goes straight in and out -- no marshalling layer, no
 *   intermediate bounding-box MultiFabs, nothing like consolidateMultiFab().
 *
 * THE EXPENSIVE PART IS CACHED
 *
 *   setGreensFunction() evaluates G over the doubled domain, forward-transforms
 *   it, and stores the result in the solver's m_G_fft. solve() only reads it.
 *   So the kernel transform must happen once per DOMAIN SIZE, not once per
 *   solve. Since G is translation invariant, the spectral kernel depends only
 *   on the box LENGTHS -- a snug domain that drifts without changing its
 *   bounding-box dimensions needs no recompute at all.
 */
class LGFOpenBC : public LGFPoissonSolver
{
private:

    amrex::Geometry geom;
    int n_lookup;

    //! Round each bounding-box dimension up to a multiple of this before
    //! building a solver. Purely a cache-hit optimisation: FFT length padding
    //! is already handled internally by Info::openbc_padding (default true),
    //! which rounds to a fast length via FFT::nextFastLen. Set to 1 to disable.
    int box_quantum;

    //! Domain the cached solver was built for. Invalid => must (re)build.
    amrex::Box m_cached_domain;

    std::unique_ptr<amrex::FFT::OpenBCSolver<amrex::Real>> m_solver;

    //! Smallest domain covering both the tagged sources and the evaluation
    //! region, quantised. See the .cpp for why both are needed.
    amrex::Box makeDomain (const amrex::MultiFab& target,
                           const amrex::BoxArray& tag_ba) const;


public:

    LGFOpenBC (const amrex::Geometry& geom_in, int n_look_in, int box_quantum_in = 1);

    void solvePoisson (const amrex::MultiFab& source, amrex::MultiFab& target,
                       const amrex::BoxArray& tag_ba) override;

    void solveNodalPoisson (const amrex::MultiFab& source, amrex::MultiFab& target,
                            const amrex::BoxArray& tag_ba) override;

    void regridOnto (const amrex::Geometry& new_geom, const amrex::BoxArray& new_ba,
                     const amrex::DistributionMapping& new_dm) override;

    //! Build the solver and its spectral Green's function if the domain changed.
    void ensureSolver (const amrex::Box& domain);

    void doSolve (const amrex::MultiFab& source, amrex::MultiFab& target,
                  const amrex::BoxArray& tag_ba);

    //! Padded one-sided length of the cached solver, for logging. Zero if none.
    [[nodiscard]] amrex::IntVect paddedLength () const;
};

#endif
