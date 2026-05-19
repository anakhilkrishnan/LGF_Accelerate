#ifndef LGFKERNEL_HPP
#define LGFKERNEL_HPP

#include <cmath>
#include "kernel_Types.hpp" // Include BBFMM2D's built-in logarithmic kernel

class kernel_LGF : public kernel_Logarithm {
private:
    double cell_dx;
    double cell_dy;

public:
    
    kernel_LGF(double dx, double dy) : cell_dx(dx), cell_dy(dy) {}
    // -------------------------------------------------------------------------
    // THE INTERCEPTOR: Overriding the point-to-point evaluation
    // -------------------------------------------------------------------------
    virtual double kernel_Func(Point r0, Point r1) {
        double dx = r0.x - r1.x;
        double dy = r0.y - r1.y;

        // --- B. The Near-Field (Discrete LGF) ---
        // If the cells are close together, intercept the call and use your lookup table
        int n = std::abs(std::round(dx / cell_dx));
        int m = std::abs(std::round(dy / cell_dy));

        // If within 7 cells, intercept the FMM and use your exact volume integration
        if (n * n + m * m <= 49) 
        {
            AMREX_ASSERT(n < 8 && m < 8); // guard against table resize without updating cutoff
            static constexpr double exact_core[8][8] = {
                {-0.000000000000, 0.250000000000, 0.363380227632, 0.430281365795, 0.476993647394, 0.512902329079, 0.542115430423, 0.566760291019},
                {0.250000000000, 0.318309886184, 0.386619772368, 0.440375794076, 0.482395447352, 0.516250119249, 0.544399550798, 0.568421922546},
                {0.363380227632, 0.386619772368, 0.424413181578, 0.462206590789, 0.495962228688, 0.525303149768, 0.550810730973, 0.573181291838},
                {0.430281365795, 0.440375794076, 0.462206590789, 0.488075158815, 0.513943726841, 0.538189520163, 0.560358931488, 0.580479669282},
                {0.476993647394, 0.482395447352, 0.495962228688, 0.513943726841, 0.533547999699, 0.553152272556, 0.571955805532, 0.589632760981},
                {0.512902329079, 0.516250119249, 0.525303149768, 0.538189520163, 0.553152272556, 0.568915764830, 0.584679257104, 0.599992430844},
                {0.542115430423, 0.544399550798, 0.550810730973, 0.560358931488, 0.571955805532, 0.584679257104, 0.597853027210, 0.611026797317},
                {0.566760291019, 0.568421922546, 0.573181291838, 0.580479669282, 0.589632760981, 0.599992430844, 0.611026797317, 0.622338403071},
            };
            
            double lgf_exact = exact_core[n][m];
            
            // Reverse-engineer the physical value into pure-log space
            double C = 0.2573420803;
            double C_prime = C - (1.0 / (4.0 * M_PI)) * std::log(cell_dx * cell_dx);
            return 2.0 * M_PI * (lgf_exact - C_prime);
        }

        // --- C. The Far-Field (Continuous) ---
        // If the cells are far apart, hand control back to BBFMM2D's native log function.
        // This is what it will use to build the Chebyshev matrices.
        return kernel_Logarithm::kernel_Func(r0, r1);
    }

    // NOTICE: We completely omit kernel_Scale().
    // The C++ compiler will automatically route tree-translation scaling calls 
    // to kernel_Log::kernel_Scale(), completely saving us from the logarithmic trap!
};

#endif