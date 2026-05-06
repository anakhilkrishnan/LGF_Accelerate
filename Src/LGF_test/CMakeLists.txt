add_executable(lgfpoisson
                main.cpp
                initializeMultiFab.cpp
                MyFunctions.H
                SourceField.H)

setup_target_for_cuda_compilation(lgfpoisson)

target_include_directories(lgfpoisson PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lgfpoisson PRIVATE LGF_core_lib AMReX::amrex)

# Direct the binary to Exec/LGF_test
set_target_properties(lgfpoisson PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Exec/LGF_test")

# Update the input file sync command to follow the binary
add_custom_command(TARGET lgfpoisson POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/inputs"
    "${CMAKE_SOURCE_DIR}/Exec/LGF_test/inputs"
    COMMENT "Syncing inputs to Exec/LGF_test..."
)

# Update the jobscript file sync command to follow the binary
add_custom_command(TARGET lgfpoisson POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/debug.sh"
    "${CMAKE_SOURCE_DIR}/Exec/LGF_test/debug.sh"
    COMMENT "Syncing jobscript to Exec/LGF_test..."
)