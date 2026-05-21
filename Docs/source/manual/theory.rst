==========================
Green's Function Approach
==========================

In the context of the incompressible Navier-Stokes solver, the Poisson equation
to be solved takes the general form

.. math::

   \nabla^2 \phi = f

Where

* :math:`\phi` is the target scalar field to be computed (e.g., pressure).
* :math:`f` is the source field, (e.g., the divergence of an intermediate
  velocity field :math:`\nabla \cdot \vec{u}^*`).

For an unbounded domain problem, provided :math:`\phi \to 0` as
:math:`\abs{\vec{r}} \to \infty`, the solution can be computed by performing a
convolution with the free space Green's function as

.. math::

   \phi = G(\vec{r}, \vec{r}_0) \ast f(\vec{r}_0) = \int G(\vec{r}, \vec{r}_0)
   f(\vec{r}_0) \odif{\vec{r}_0}

Where

.. math::

   \nabla^2 G(\vec{r}, \vec{r}_0) = \delta(\vec{r} - \vec{r}_0)

For a 2D problem, the free space Green's function is

.. math::

   G(\vec{r}, \vec{r}_0) = \frac{1}{2\pi}\log{\abs{\vec{r} - \vec{r}_0}}

The snug-domain capability of the solver is due to the fact that the convolution
can be restricted to a subset of the overall domain, specifically,
:math:`\mathrm{supp}(f)`.


