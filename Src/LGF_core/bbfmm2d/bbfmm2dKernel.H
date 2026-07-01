#ifndef BBFMM2DKERNEL_H_
#define BBFMM2DKERNEL_H_

#include <kernel_Types.hpp>
#include <LGFKernels.H>

class kernel_LGF : public kernel_Base
{
    private:

        double cell_dx;
        double cell_dy;
        int n_lookup;

    public:
        
        kernel_LGF(double dx, double dy, int nLookup) : cell_dx(dx), cell_dy(dy), n_lookup(nLookup) 
        {
            // class object initialization
        }
        
        virtual double kernel_Func(Point r0, Point r1) 
        {
            // non-dimensionalize the distances against dx
            double deltax = (r1.x - r0.x) / cell_dx;
            double deltay = (r1.y - r0.y) / cell_dy;

            // computing integer distances to check for lookup table            
            int n = std::round(std::abs(deltax));
            int m = std::round(std::abs(deltay));

            bool is_physical_grid = (std::abs(n - std::abs(deltax)) < 1e-8) && (std::abs(m - std::abs(deltay)) < 1e-8);

            // checking if within lookup table range to bypass any further computations
            if (is_physical_grid && (n * n + m * m <= ((n_lookup - 1) * (n_lookup - 1))))
            {
                return nearField2D(n, m);
            }

            return farField2D(deltax, deltay);
        }
};

#endif