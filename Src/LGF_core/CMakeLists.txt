# add files from BBFMM2d submodule
file(GLOB BBFMM2D_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/BBFMM2D/src/*.cpp")
file(GLOB BBFMM2D_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/BBFMM2D/header/*.hpp" )

# Define the library and its source files
add_library(LGF_core_lib STATIC
    LGFPoissonSolver.H
    tagSource.cpp
    LGFKernels.H
    directSum/directSumKernel.H
    directSum/directSumLGF.H
    directSum/directSumLGF.cpp
    bbfmm2d/bbfmm2dKernel.H
    bbfmm2d/bbfmm2dLGF.H
    bbfmm2d/bbfmm2dLGF.cpp
    ${BBFMM2D_SOURCES}
    ${BBFMM2D_HEADERS}
)

# setup_target_for_cuda_compilation(LGF_core_lib)

# Tell CMake where the headers from local sources and submodules are
target_include_directories(LGF_core_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                                ${CMAKE_CURRENT_SOURCE_DIR}/directSum
                                                ${CMAKE_CURRENT_SOURCE_DIR}/bbfmm2d
                                                ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/BBFMM2D/header)

# Link to AMReX so the core knows about MultiFabs and MPI
# add Eigen3::Eigen for bbfmm2d
target_link_libraries(LGF_core_lib PUBLIC AMReX::amrex Eigen3::Eigen)