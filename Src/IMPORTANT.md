There is a bug in AMReX source code, as of version 26.09-12-g07ebbee51f42.

In Src/FFT/AMReX_FFT_OpenBCSolver.H, lines 192-193:

auto const len3d = m_padded_length.dim3();
GpuArray<int,3> len{len3d.x, len3d.y, len3d.z};

Default len3d.z to 0 for 2D builds, evading the kernel evaluation and forcing 0.
(in 3D, this behavior is for removing contribution of that point on its own
position)

For correct behavior in 2D, change lines to:
GpuArray<int,3> len{1,1,1};
for (int idim = 0; idim < AMREX_SPACEDIM; ++idim) { len[idim] = m_padded_length[idim]; }
