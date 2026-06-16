import yt
import numpy as np
import matplotlib.pyplot as plt
from scipy.special import exp1
import os
import re

# ==========================================
# User Defined Inputs (Matching ParmParse)
# ==========================================
n_cell = 512
dom_lo = -5.0
dom_hi = 5.0
gauss_cen_x = 0.0
gauss_cen_y = 0.25
variance = 0.5  # From SourceField.H

dir = "."
descr = ""
plotfile = dir + "/plt" + descr + "00" + str(n_cell)
adaptiveGrid = False

# ==========================================
# Directory Setup
# ==========================================
basename = os.path.basename(plotfile.rstrip('/'))
match = re.search(r'\d+', basename)
num_str = match.group(0) if match else "XXXXX"

out_dir = dir + "/post" + descr + f"{num_str}"
os.makedirs(out_dir, exist_ok=True)
print(f"Output directory established: {os.path.abspath(out_dir)}")

# ==========================================
# 1. Load AMReX Data
# ==========================================
print(f"Loading {plotfile}...")
ds = yt.load(plotfile)

grid = ds.covering_grid(level=0, left_edge=ds.domain_left_edge, dims=ds.domain_dimensions)

phi_num_raw = grid['Target_Phi'].squeeze().v 
mlmg_phi_num_raw = grid['MLMG_Target_Phi'].squeeze().v

if adaptiveGrid:
    tag_mask = grid['Active_Box_Tag'].squeeze().v

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
mlmg_shift_val = mlmg_phi_num_raw[cen_idx_x, cen_idx_y] - phi_exact[cen_idx_x, cen_idx_y]
phi_num = phi_num_raw - shift_val
mlmg_phi_num = mlmg_phi_num_raw - mlmg_shift_val

print(f"Applied constant shifts of {shift_val:.4e} and {mlmg_shift_val:.4e} to match analytical center.")

# Compute Absolute Error
abs_error = np.abs(phi_num - phi_exact)
max_error = np.max(abs_error)

# Compute Relative L2 Norm (The "Shape Check")
rmse_error = np.sqrt(np.mean(np.square(phi_num - phi_exact)))

# Compute errors for MLMG results as well
mlmg_abs_error = np.abs(mlmg_phi_num - phi_exact)
mlmg_max_error = np.max(abs_error)

# Compute Relative L2 Norm (The "Shape Check")
mlmg_rmse_error = np.sqrt(np.mean(np.square(mlmg_phi_num - phi_exact)))

print(f"Maximum Absolute Error: {max_error:.4e}")
print(f"RMS Error: {rmse_error:.4e}")

print(f"MLMG Maximum Absolute Error: {mlmg_max_error:.4e}")
print(f"MLMG RMS Error: {mlmg_rmse_error:.4e}")

# Compute solver - MLMG results
discrete_compare = phi_num - mlmg_phi_num
discrete_max_abs_err = np.max(np.abs(discrete_compare))

# ==========================================
# 5. Plotting Results (2D)
# ==========================================
extent = [dom_lo, dom_hi, dom_lo, dom_hi]
fig, axs = plt.subplots(1, 3, figsize=(16, 4.5))

im0 = axs[0].imshow(abs_error.T, extent=extent, origin='lower', cmap='magma')
axs[0].set_title(f"Absolute Error\n(Max: {max_error:.2e} | RMS: {rmse_error:.2e})")
fig.colorbar(im0, ax=axs[0])

im1 = axs[1].imshow(mlmg_abs_error.T, extent=extent, origin='lower', cmap='magma')
axs[1].set_title(f"MLMG Absolute Error\n(Max: {mlmg_max_error:.2e} | RMS: {mlmg_rmse_error:.2e})")
fig.colorbar(im1, ax=axs[1])

im2 = axs[2].imshow(discrete_compare.T, extent=extent, origin='lower', cmap='magma')
axs[2].set_title(f"Discrete comparison of solver\n(Max: {discrete_max_abs_err:.2e})")
fig.colorbar(im2, ax=axs[2])

plt.tight_layout()

if adaptiveGrid:
    out_path_2d = os.path.join(out_dir, "error_maps_2D_adapt.png")
else:
    out_path_2d = os.path.join(out_dir, "error_maps_2D.png")
plt.savefig(out_path_2d, dpi=300)
print(f"Saved 2D visual error map to: {os.path.abspath(out_path_2d)}")
