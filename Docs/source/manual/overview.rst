======================
Overview
======================

The **LGF_Accelerate** library provides a high-performance framework for solving
elliptic partial differential equations (PDEs), specifically the Poisson
equation, using **Lattice Green’s Functions (LGFs)**. This approach is
particularly effective for unbounded domain problems where the transcient
physics is confined to *small* regions, but its effect is felt *everywhere* in
the domain.

The library is developed for use in an **incompressible Navier-Stokes** solver,
specifically for solving the pressure Poisson equation. It is developed using
**AMReX** in C++, inheriting most of the key AMReX features like perforamnce
portability, dimension-agnostic source code and  out-of-the-box support for
hybrid computing architectures. It uses other open source C++ libraries
(Eg: **BBFMM2D**) for advanced methods to accelerate convolutions.

The library was originally written by Akhil, as part of a Navier-Stokes
solver project for ME282: Computational Heat Transfer and Fluid Flow, offered in
the Mechanical Engineering department at Indian Institute of Science (IISc).

Features
=========

* Naive summation implementation can run on both CPU and GPU systems. BBFMM2D
  implementation is limited to single-core CPU functionality.
* Naive summation implementation can use MPI+CUDA (or OpenMP) for parallel
  computing.
* The tagging threshold (cutoff for :math:`\mathrm{supp}(f)`) can be used for
  controlling error.
* 