# ===========================================================================
# APPLICATION EXECUTABLES CONFIGURATION
# ===========================================================================

# Automatically discover all application modules in the 'src' directory
# This ensures new apps are added to the build system just by creating a folder.
subdirlist(SUBDIRS "${CMAKE_CURRENT_SOURCE_DIR}/src")

foreach(subdir ${SUBDIRS})
    set(AppName "${subdir}")

    # Collect implementation files for the current application
    file(GLOB SRC_FILES "src/${subdir}/*.cpp")
    
    if(SRC_FILES)
        # 1. Initialize the executable target
        add_executable(${AppName})  

        # 2. Assign source files
        # Using PRIVATE to encapsulate implementation details
        target_sources(${AppName} PRIVATE ${SRC_FILES})

        # Visual feedback for the configuration log
        message(STATUS "Created application target: ${AppName}")
    endif()
endforeach()

# ===========================================================================
# TARGET-SPECIFIC LINKAGE
# ===========================================================================

# Link the primary 'Main' executable to the core utility library.
# We check if the target exists to prevent errors if the 'Main' folder is missing.
if(TARGET Main)
    target_link_libraries(Main 
        PRIVATE 
            core::General
    )
    message(STATUS "Linked 'Main' to core::General")
endif()