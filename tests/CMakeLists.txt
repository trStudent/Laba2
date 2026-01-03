# ===========================================================================
# AUTOMATED TEST DISCOVERY
# ===========================================================================

# Scan the current directory for subfolders (Rule 1: No manual duplication)
subdirlist(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR})

foreach(subdir ${SUBDIRS})
    set(TestName "${subdir}Test")

    # Collect all C++ source files within the subdirectory
    file(GLOB SRC_FILES "${subdir}/*.cpp")
    
    if(SRC_FILES)
        # 1. Define the test executable
        add_executable(${TestName})  

        # 2. Assign sources
        target_sources(${TestName} PRIVATE ${SRC_FILES})

        # 3. Link dependencies
        # Links against the GoogleTest entry point and the corresponding core library
        target_link_libraries(${TestName} 
            PRIVATE 
                gtest_main 
                core::${subdir}
        )

        # 4. Register with the CTest runner
        add_test(
            NAME ${TestName} 
            COMMAND ${TestName}
        )

        # Visual feedback during configuration
        message(STATUS "Registered test suite: ${TestName}")
    endif()
endforeach()