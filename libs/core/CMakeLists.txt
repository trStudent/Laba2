# ===========================================================================
# CORE LIBRARIES CONFIGURATION
# ===========================================================================

# Scan the 'src' directory for modular sub-libraries
subdirlist(SUBDIRS "${CMAKE_CURRENT_SOURCE_DIR}/src")

foreach(subdir ${SUBDIRS})
    # Define a unique internal name for the library
    set(LibName "${subdir}Core")

    # Collect source and header files for the current module
    file(GLOB SRC_FILES "src/${subdir}/*.cpp")
    file(GLOB HDR_FILES "include/core/${subdir}/*.h")
    
    if(SRC_FILES)
        # 1. Define the static library target
        add_library(${LibName} STATIC)  

        # 2. Create a namespaced alias for internal linking (e.g., core::General)
        # This prevents accidental linking outside the project scope.
        add_library(core::${subdir} ALIAS ${LibName})

        # 3. Assign source files
        target_sources(${LibName} PRIVATE ${SRC_FILES})

        # 4. Configure Modern CMake File Sets
        # This automatically handles header installation and IDE visibility.
        target_sources(${LibName} 
            PUBLIC 
                FILE_SET HEADERS 
                BASE_DIRS include 
                FILES ${HDR_FILES}
        )

        # 5. Export include directories
        # 'PUBLIC' ensures that any target linking to this library 
        # automatically gets access to the 'include' folder.
        target_include_directories(${LibName} PUBLIC include)

        # Visual feedback during the configuration phase
        message(STATUS "Configured core module: core::${subdir}")
    endif()
endforeach()