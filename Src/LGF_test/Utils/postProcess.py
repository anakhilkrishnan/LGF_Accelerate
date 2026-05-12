import yt
import numpy as np
import matplotlib.pyplot as plt
from scipy.special import exp1
import os
import re

# ==========================================
# User Defined Inputs (Matching ParmParse)
# ==========================================
n_cell = 256
dom_lo = -5.0
dom_hi = 5.0
gauss_cen_x = 0.0
gauss_cen_y = 0.25
variance = 0.5  # From SourceField.H
descr = "taggpu"
plotfile = "plt" + descr + "00" + str(n_cell)
adaptiveGrid = True

# ==========================================
# Directory Setup
# ==========================================
basename = os.path.basename(plotfile.rstrip('/'))
match = re.search(r'\d+', basename)
num_str = match.group(0) if match else "XXXXX"

out_dir = "post" + descr + f"{num_str}"
os.makedirs(out_dir, exist_ok=True)
print(f"Output directory established: {os.path.abspath(out_dir)}")

# ==========================================
# 1. Load AMReX Data
# ==========================================
print(f"Loading {plotfile}...")
ds = yt.load(plotfile)

grid = ds.covering_grid(level=0, left_edge=ds.domain_left_edge, dims=ds.domain_dimensions)

if adaptiveGrid:
    phi_num_raw = grid['Target_Phi'].squeeze().v 
    tag_mask = grid['Active_Box_Tag'].squeeze().v
else:
    phi_num_raw = grid['phi'].squeeze().v

# 2. Generate Physical Coordinate Grid
dx = (dom_hi - dom_lo) / n_cell
x_coords = np.linspace(dom_lo + dx/2.0, dom_hi - dx/2.0, n_cell)
y_coords = np.linspace(dom_lo + dx/2.0, dom_hi - dx/2.0, n_cell)

X, Y = np.meshgrid(x_coords, y_coords, indexing='ij')

# 3. Compute Exact Analytical Solution
R2 = (X - gauss_cen_x)**2 + (Y - gauss_cen_y)**2
R2 = np.where(R2 == 0, 1e-15, R2)

phi_exact = (variance / 4.0) * (np.log(R2) + exp1(R2 / variance))

# ==========================================
# 4. Apply Constant Shift & Shape Check
# ==========================================
# Find the array indices closest to the Gaussian center
cen_idx_x = np.argmin(np.abs(x_coords - gauss_cen_x))
cen_idx_y = np.argmin(np.abs(y_coords - gauss_cen_y))

# Calculate the difference at the center and shift the numerical array
shift_val = phi_num_raw[cen_idx_x, cen_idx_y] - phi_exact[cen_idx_x, cen_idx_y]
phi_num = phi_num_raw - shift_val

print(f"Applied constant shift of {shift_val:.4e} to match analytical center.")

# Compute Absolute Error
abs_error = np.abs(phi_num - phi_exact)
max_error = np.max(abs_error)

# Compute Relative L2 Norm (The "Shape Check")
l2_norm_rel = np.linalg.norm(phi_num - phi_exact) / np.linalg.norm(phi_exact)

print(f"Maximum Absolute Error: {max_error:.4e}")
print(f"Relative L2 Error Norm: {l2_norm_rel:.4e}")

# ==========================================
# 5. Plotting Results (2D)
# ==========================================
extent = [dom_lo, dom_hi, dom_lo, dom_hi]
fig, axs = plt.subplots(1, 3, figsize=(16, 4.5))

im0 = axs[0].imshow(phi_num.T, extent=extent, origin='lower', cmap='viridis')
axs[0].set_title(r"Shifted AMReX Numerical $\phi$")
fig.colorbar(im0, ax=axs[0])

im1 = axs[1].imshow(phi_exact.T, extent=extent, origin='lower', cmap='viridis')
axs[1].set_title(r"Exact Analytical $\phi$")
fig.colorbar(im1, ax=axs[1])

im2 = axs[2].imshow(abs_error.T, extent=extent, origin='lower', cmap='magma')

if adaptiveGrid:
    axs[2].contour(tag_mask.T, levels=[0.5], extent=extent, colors='cyan', 
               linewidths=1.5, linestyles='dashed')

axs[2].set_title(f"Absolute Error\n(Max: {max_error:.2e} | L2: {l2_norm_rel:.2e})")
fig.colorbar(im2, ax=axs[2])

plt.tight_layout()

if adaptiveGrid:
    out_path_2d = os.path.join(out_dir, "error_maps_2D_adapt.png")
else:
    out_path_2d = os.path.join(out_dir, "error_maps_2D.png")
plt.savefig(out_path_2d, dpi=300)
print(f"Saved 2D visual error map to: {os.path.abspath(out_path_2d)}")

# ==========================================
# 6. Plotting Results (1D)
# ==========================================
mid_idx = n_cell // 2  
plt.figure(figsize=(8, 5))

plt.plot(x_coords, phi_num[:, mid_idx], 'ro', label='Shifted AMReX Numerical', markersize=5)
plt.plot(x_coords, phi_exact[:, mid_idx], 'k-', label='Exact Analytical', linewidth=2)

plt.title("1D Cross-Section at $y = 0$")
plt.xlabel("$x$ coordinate")
plt.ylabel(r"Potential ($\phi$)")
plt.legend()
plt.grid(True, linestyle='--', alpha=0.6)
plt.tight_layout()

if adaptiveGrid:
    out_path_1d = os.path.join(out_dir, "slice_1D_adapt.png")
else:
    out_path_1d = os.path.join(out_dir, "slice_1D.png")
plt.savefig(out_path_1d, dpi=300)
print(f"Saved 1D slice comparison to: {os.path.abspath(out_path_1d)}")