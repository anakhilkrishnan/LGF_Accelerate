======================
Theoretical Background
======================

The **LGF_Accelerate** library provides a high-performance framework for solving elliptic partial differential equations (PDEs), specifically the Poisson equation, using **Lattice Green’s Functions (LGF)**. This approach is particularly effective for unbounded domain problems where traditional boundary conditions are difficult to implement.

1. Governing Equations
======================

In the context of an incompressible Navier-Stokes solver, we solve the pressure Poisson equation derived from the projection method:

.. math::

   \nabla^2 \phi = f

Where:
* :math:`\phi` represents the scalar potential (e.g., pressure).
* :math:`f` is the source term, typically the divergence of an intermediate velocity field :math:`\nabla \cdot \vec{u}^*`.

2. The Lattice Green's Function (LGF)
=====================================

Unlike the continuous Green’s function, the LGF is the fundamental solution to the **discrete** Laplacian operator on a structured grid. For a 2D Cartesian grid with spacing :math:`h`, the discrete Poisson equation is:

.. math::

   \Delta_h G(\mathbf{r}) = \delta_{\mathbf{r}, 0}

The solution :math:`\phi` at any target point :math:`\mathbf{x}_{tar}` is obtained via discrete convolution:

.. math::

   \phi(\mathbf{x}_{tar}) = \sum_{\mathbf{x}_{src}} G(\mathbf{x}_{tar} - \mathbf{x}_{src}) f(\mathbf{x}_{src})



3. Numerical Implementation
===========================

Near-Field: Exact Integration
-----------------------------
For small separations (:math:`\sqrt{n^2 + m^2} \le 7`), the library utilizes a precomputed lookup table to avoid singularities and high errors at short range.

Far-Field: Asymptotic Expansion
-------------------------------
For large separations, we apply a multipole correction to the continuous Green’s function:

.. math::

   G(\mathbf{r}) \approx \frac{1}{4\pi} \ln(r^2) + C - \frac{n^4 + m^4 - 6n^2m^2}{24\pi r^6}

Where :math:`C \approx 0.2573420803` is the constant asymptotic shift.



4. Computational Acceleration
=============================

The library integrates with **BBFMM2D** to handle complexity:

* **Tagging**: Identifies significant source terms using a user-defined threshold.
* **Consolidation**: Prepares data for FMM processing.
* **Kernel Interception**: The :cpp:class:`kernel_LGF` intercepts point-to-point interactions to inject discrete corrections.