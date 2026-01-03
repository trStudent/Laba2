# ===========================================================================
# 1. PROJECT CONFIGURATION
# ===========================================================================
cmake_minimum_required(VERSION 3.24)

# Project metadata
project(Laba2 
    VERSION 1.0.0 
    LANGUAGES CXX
)

# ===========================================================================
# 2. COMPILER & LANGUAGE STANDARDS
# ===========================================================================
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disable compiler-specific extensions (e.g., gnu++17) to ensure 
# portability and strict standard compliance.
set(CMAKE_CXX_EXTENSIONS OFF)

# Ensure Position Independent Code is used (important for libraries)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ===========================================================================
# 3. THIRD-PARTY DEPENDENCIES (GoogleTest)
# ===========================================================================

# CRITICAL: Disable GoogleMock to prevent build failures on modern MinGW 
# caused by attribute placement bugs in older GMock headers.
set(BUILD_GMOCK OFF CACHE BOOL "Builds only GTest, skipping GMock" FORCE)

# Prevents GTest from installing its own binaries during project installation.
set(INSTALL_GTEST OFF CACHE BOOL "Do not install GTest files system-wide" FORCE)

# ===========================================================================
# 4. HELPER MACROS
# ===========================================================================

# Scans a directory for subdirectories and returns them as a list.
# This facilitates automated test discovery and reduces code duplication (Rule 1).
macro(SUBDIRLIST result curdir)
  file(GLOB children RELATIVE ${curdir} ${curdir}/*)
  set(dirlist "")
  foreach(child ${children})
    if(IS_DIRECTORY ${curdir}/${child})
      list(APPEND dirlist ${child})
    endif()
  endforeach()
  set(${result} ${dirlist})
endmacro()

# ===========================================================================
# 5. DIRECTORY STRUCTURE & BUILD TARGETS
# ===========================================================================

# Enable the CTest framework for automated validation
enable_testing()

# Third-party libraries
add_subdirectory(libs/googletest)

# Core internal libraries
add_subdirectory(libs/core)

# Main application executables
add_subdirectory(apps)  

# Unit test suites
add_subdirectory(tests)

# ===========================================================================
# 6. DIAGNOSTIC OUTPUT
# ===========================================================================
message(STATUS "-----------------------------------------------------------")
message(STATUS " Configuration Summary:")
message(STATUS " Project:  ${CMAKE_PROJECT_NAME} v${CMAKE_PROJECT_VERSION}")
message(STATUS " Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS " Standard: C++${CMAKE_CXX_STANDARD} (Strict)")
message(STATUS "-----------------------------------------------------------")