import subprocess
import csv
import re
import os

# Point to the executable in the neighboring directory
EXECUTABLE = "../LGF_test/lgfpoisson"
INPUTS = "bench_inputs"
OUTPUT_FILE = "lgf_scaling_results.csv"
MPI_CMD = "mpiexec"
NUM_RANKS = "1"

# Manual overrides
TAGGING_THRESH = 1.0e-8

# Define the test suites
suites = {
    # Type 1: Vary n_cell, keep max_grid_size constant
    "Scale_Domain": {
        "n_cells": [128, 256, 512], # Careful going higher without checking RAM!
        "max_grid_sizes": [32]
    },
    # Type 2: Vary box sizes, keep n_cell constant
    "Scale_Boxes": {
        "n_cells": [512],
        "max_grid_sizes": [16, 32, 64, 128]
    }
}

def run_suite():
    # Ensure the executable exists
    if not os.path.exists(EXECUTABLE):
        print(f"Error: Executable not found at {EXECUTABLE}")
        return

    with open(OUTPUT_FILE, mode='w', newline='') as file:
        writer = csv.writer(file)
        # We only have one combined compute time in the current main.cpp
        writer.writerow(["Test_Type", "n_cell", "max_grid_size", "total_compute_time_s", "max_rank_time_s"])

        for test_name, params in suites.items():
            print(f"\n--- Running Suite: {test_name} ---")
            for cell in params["n_cells"]:
                for grid in params["max_grid_sizes"]:
                    print(f"  Testing n_cell={cell}, max_grid_size={grid}...", end=" ", flush=True)

                    NUM_THREADS = "16"
                    # Set the OpenMP environment variable for the child process
                    os.environ["OMP_NUM_THREADS"] = NUM_THREADS

                    # AMReX arguments: force write_plot=0 and turn off amrex verbosity
                    cmd = [
                        MPI_CMD, "-n", NUM_RANKS,  # <--- MPI Wrapper
                        EXECUTABLE, 
                        INPUTS,      # <--- Base Inputs
                        f"n_cell={cell}",          # <--- Overrides
                        f"max_grid_size={grid}",
                        f"tagging_threshold={TAGGING_THRESH}"
                    ]
                    
                    try:
                        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
                        output = result.stdout
                        
                        # Use Regex to scrape the exact numbers from your C++ Print() statements
                        compute_match = re.search(r"Time taken for computation:\s*([\d\.]+)", output)
                        max_time_match = re.search(r"Max compute time \(Slowest Rank\):\s*([\d\.]+)", output)
                        
                        if compute_match and max_time_match:
                            calc_time = compute_match.group(1)
                            max_time = max_time_match.group(1)
                            writer.writerow([test_name, cell, grid, calc_time, max_time])
                            print(f"Done! ({calc_time} s)")
                        else:
                            print(f"Failed! Could not parse output.")
                            print("Output was:\n", output)
                    
                    except subprocess.CalledProcessError as e:
                        print(f"Failed! (Crash)")
                        print(e.stderr)

    print(f"\nAll benchmarks complete! Results saved to {OUTPUT_FILE}")

if __name__ == "__main__":
    run_suite()