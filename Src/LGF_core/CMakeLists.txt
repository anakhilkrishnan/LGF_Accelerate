add_library(LGF_core_lib STATIC
    LGFPoissonSolver.H
    tagSource.cpp
    LGFKernels.H
    directSum/directSumKernel.H
    directSum/DirectSumLGF.H
    directSum/directSumLGF.cpp
)

if(AMReX_SPACEDIM EQUAL 2)
    file(GLOB BBFMM2D_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/BBFMM2D/src/*.cpp")
    file(GLOB BBFMM2D_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/BBFMM2D/header/*.hpp")
    target_sources(LGF_core_lib PRIVATE
        bbfmm2d/bbfmm2dKernel.H bbfmm2d/bbfmm2dLGF.H bbfmm2d/bbfmm2dLGF.cpp
        ${BBFMM2D_SOURCES} ${BBFMM2D_HEADERS})
    target_include_directories(LGF_core_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/bbfmm2d
        ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/BBFMM2D/header)
    target_link_libraries(LGF_core_lib PUBLIC Eigen3::Eigen)
endif()

if(AMReX_GPU_BACKEND STREQUAL "CUDA")
    setup_target_for_cuda_compilation(LGF_core_lib)
endif()

# Tell CMake where the headers from local sources and submodules are
target_include_directories(LGF_core_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                                ${CMAKE_CURRENT_SOURCE_DIR}/directSum
                                                ${CMAKE_CURRENT_SOURCE_DIR}/bbfmm2d
                                                ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/BBFMM2D/header)

# Link to AMReX so the core knows about MultiFabs and MPI
# add Eigen3::Eigen for bbfmm2d
target_link_libraries(LGF_core_lib PUBLIC AMReX::amrex Eigen3::Eigen)