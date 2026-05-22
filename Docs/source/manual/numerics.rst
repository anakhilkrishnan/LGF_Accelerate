==========================
Numerical Implementation
==========================

.. important::

    * Only 2D problems supported (3D kernel computing function yet to be
      developed).
    * :math:`dx = dy` must be ensured.
    * :code:`max_grid_size` must be a factor of :code:`n_cell`.

Box-wise domain decomposition
================================

Using AMReX data strucutres, the domain is broken down into cells. Both target
and source fields are stored as **cell-centered** data. Many cells are grouped
to form non-overlapping boxes.

Boxes serve 2 roles:

#. **Enabling parallel-computations** - each box is assigned a certain MPI rank
   and by use of ghost cells, computations within each box can be performed
   independent of each other.
#. **Box-wise tagging and convolutions** - each box is tagged as active/inactive
   based on whether the source field value in each cell is greater than a
   threshold or not (numerical equivalent of finding :math:`\mathrm{supp}(f)`).
   The convolution is implemented as a loop over all the boxes, with subloops
   over all cells within each box. The number of computations is drastically
   lowered at a small error cost by skipping inactive boxes.

.. note::

    The present solver only tags boxes on the source side. The solution is still
    evaluated at every target box. Liska and Colonius' method tags both source
    and target cells. What their solver does when it tags a previously untagged
    target box is a mystery.

The Lattice Green's Function (LGF)
=====================================

In a discretized domain, the actual Poisson equation becomes a difference
equation of the form

.. math::

    \mathcal{L}\Phi = F

Where

* :math:`\mathcal{L}` is a discrete Laplacian operator (a 5-pt operator in a 2\
  :sup:`nd` order discretization)
* :math:`\Phi` is the cell-centered target field data
* :math:`F` is the cell-centered source field data

Using the continuous Green's function to solve the discrete Poisson equation
introduces discretization errors that blow up for small values of
:math:`\abs{\vec{r} - \vec{r}_0}`.

The Lattice Green's Function :math:`G_L` is the exact solution to the
**discrete** Laplacian operator on a structured grid. The solution computed
using the LGF exactly satisfies the discrete Poisson equation and does not
introduce any new errors. The LGF also remains finite for all values of
:math:`\abs{\vec{n} - \vec{n}_0}`.

The solution :math:`\Phi(\vec{n})` at any target cell :math:`\vec{n}` is
obtained via discrete convolution:

.. math::

    \Phi(\vec{n}) = \sum_{\vec{n}_0} G_L(\vec{n} - \vec{n}_0) f(\vec{n}_0)


An comprehensive explanation of the method used to compute the LGF for a given
source-target cell pair is presented in Liska and Colonius' work
:footcite:`LISKA201476`. In summary, the discrete Laplacian :math:`\mathcal{L}`
operator is inverted using Fourier integrals, yielding the form

.. math::

    G_L(\vec{n}) = \frac{1}{(2\pi)^2} \int\int_{[-\pi, \pi]^2} \frac{1 - \cos{(\vec{\xi} \cdot \vec{n})}}{4 - 2\cos{(\xi_1)}- 2\cos{(\xi_2)}} \odif{\xi_1}\odif{\xi_2}

Where :math:`\vec{\xi} = (\xi_1, \xi_2)` are Fourier modes.

To compute the LGF more efficiently for a desired order of error, the
source-target cell pairs are split into Near-Field and Far-Field type, and LGFs
are evaluated differently for each.

Near-Field: Exact Integration
-----------------------------

Near-Field contributions are computed by direct evaluation of the aforementioned
Fourier Integral form. Liska and Colonius :footcite:`LISKA201476` use symmetry
to reduce the integral to a 1D integral, which is precomputed using Gauss-Kronod
quadrature and stored as a lookup table. The code developed by Hou and Colonius
:footcite:`hou2024lattice` was used to generate the lookup table.

Far-Field: Asymptotic Expansion
-------------------------------

Far-Field behavior is modeled using asymptotic expansions of the form

.. math::

   G_L(\mathbf{r}) \approx \frac{1}{4\pi} \ln(r^2) + C - \frac{n^4 + m^4 -
   6n^2m^2}{24\pi r^6}

Where :math:`C \approx 0.2573420803` is the constant asymptotic shift.

A continuous far-field representation is adopted to enable the use of fast
convolution algorithms, such as KI-FMMs, originally designed for continuous
kernels.

Convolution Accelerating Methods
=================================

The far-field contributions are handled by **BBFMM2D** (Black Box Fast Multipole
Method in 2 Dimensions), an open-source KI-FMM implementation in C++. Its
simplicity and lack of parallelizing/memory managing layers made it an idea
choice to test with the problem at hand.

BBFMM2D performs the following steps:

#. Constructs a quadtree representation of the domain based on the positions of
   the source points.
#. Replaces the source points with equivalent source distributions on known
   proxy surfaces using Chebyshev interpolation.
#. Computes multipole expansions of the proxy distribution rapidly.
#. Performs an upward pass of the tree, computing local expansions for solutions
   at proxy surfaces using the multipole expansions of the proxy distributions.
#. Performs a downward pass of the tree, extracting solutions at target
   locations.

BBFMM2D has the following limitations:

* The code is fully **serial**, it can only use a single CPU core.
* The target points **cannot** be different from the source points, negating the
  advantage from limiting computations to :math:`\mathrm{supp}(f)` by box
  tagging.
* The inbuilt :math:`\log{\vec{r}}` kernel represents the continuous Green's
  function, not the LGF. This introduces a systematic far-field error that would
  otherwise have been corrected for when using an asymptotic expansion to
  approximate the LGF in the far-field.

.. footbibliography::

