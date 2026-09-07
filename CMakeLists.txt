cmake_minimum_required(VERSION 3.16)

# custom options for convenience
option(ENABLE_BBFMM2D "Enable bbfmm2d library" NO)

project(LGF_Accelerate LANGUAGES C CXX)

# typical values (included for ease of building)
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "cmake build type")
set(AMReX_SPACEDIM "3" CACHE STRING "Spatial Dimension")
set(AMReX_MPI YES CACHE BOOL "Enable MPI")
set(AMReX_TINY_PROFILE YES CACHE BOOL "Enable Tiny Profiling")
set(AMReX_MEM_PROFILE YES CACHE BOOL "Enable Memory Profiling")

if(AMReX_OMP)
  find_package(OpenMP REQUIRED)
endif()

if(AMReX_GPU_BACKEND STREQUAL "CUDA")
  enable_language(CUDA)
  # always specify CMAKE_CUDA_ARCHITECTURES at compile time on every machine
endif()

if (ENABLE_BBFMM2D)
  # find Eigen3 installation on the system
  find_package(Eigen3 REQUIRED)
endif()

if(NOT TARGET AMReX::amrex)
  message(STATUS "LGF_Accelerate: AMReX not found in parent. Fetching local copy...")
  
  if(NOT DEFINED AMReX_ROOT)
    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)
    FetchContent_Declare(
        amrex_code
        GIT_REPOSITORY https://github.com/AMReX-Codes/amrex.git/
        GIT_TAG        origin/development
    )
    FetchContent_MakeAvailable(amrex_code)
  else()
    find_package(AMReX REQUIRED PATHS ${AMReX_ROOT}/lib/cmake/AMReX/AMReXConfig.cmake)
  endif()
else()
  message(STATUS "LGF_Accelerate: Using AMReX provided by parent project.")
endif()

# building as static library
add_subdirectory(Src/LGF_core)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

  # building executable for testing and development
  message(STATUS "Building LGF_Accelerate for testing...")
  add_subdirectory(Src/LGF_test)

  # make Exec directories in case they're not present
  file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/Exec/LGF_test/Results")
  file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/Exec/LGF_test/Logs")
  
else()

  # no additional building, only static library for use as a submodule
  message(STATUS "Building LGF_Accelerate as a submodule library...")
endif()

