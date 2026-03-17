# FindPhmap.cmake - CMake module for finding parallel-hashmap (phmap)
#
# This module finds the phmap header-only library and defines the following:
#
#  Phmap_FOUND          - True if phmap headers are found
#  Phmap_INCLUDE_DIRS   - Include directories for phmap
#  Phmap::phmap         - Imported interface target
#
# The module searches in the following order:
#  1. Vendored phmap in deps/phmap/ (relative to this module)
#  2. System-installed phmap (via PHMAP_ROOT or standard paths)
#  3. Error if not found
#
# Usage:
#   find_package(Phmap REQUIRED)
#   target_link_libraries(mytarget PRIVATE Phmap::phmap)
#
# License: Apache 2.0
# Maintainer: TrinityCore Playerbot Team

# Option 1: Try vendored phmap first (recommended for Playerbot)
get_filename_component(PLAYERBOT_MODULE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(VENDORED_PHMAP_DIR "${PLAYERBOT_MODULE_DIR}/deps/phmap")

if(EXISTS "${VENDORED_PHMAP_DIR}/parallel_hashmap/phmap.h")
    set(PHMAP_INCLUDE_DIR "${VENDORED_PHMAP_DIR}")
    set(PHMAP_FOUND TRUE)
    set(PHMAP_SOURCE "vendored")
    message(STATUS "Found vendored phmap: ${VENDORED_PHMAP_DIR}")
else()
    # Option 2: Search for system-installed phmap
    find_path(PHMAP_INCLUDE_DIR
        NAMES parallel_hashmap/phmap.h
        HINTS
            ${PHMAP_ROOT}
            $ENV{PHMAP_ROOT}
            ${CMAKE_PREFIX_PATH}
        PATH_SUFFIXES
            include
            include/phmap
        DOC "phmap include directory"
    )

    if(PHMAP_INCLUDE_DIR)
        set(PHMAP_FOUND TRUE)
        set(PHMAP_SOURCE "system")
        message(STATUS "Found system phmap: ${PHMAP_INCLUDE_DIR}")
    endif()
endif()

# Standard find_package result handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Phmap
    REQUIRED_VARS PHMAP_INCLUDE_DIR
    FAIL_MESSAGE "Could not find phmap headers. Either:
    1. Initialize git submodules: git submodule update --init --recursive
    2. Install phmap system-wide and set PHMAP_ROOT
    3. Download from https://github.com/greg7mdp/parallel-hashmap"
)

# Create imported target if found
if(PHMAP_FOUND AND NOT TARGET Phmap::phmap)
    add_library(Phmap::phmap INTERFACE IMPORTED)
    set_target_properties(Phmap::phmap PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${PHMAP_INCLUDE_DIR}"
    )

    # Set output variables
    set(Phmap_INCLUDE_DIRS "${PHMAP_INCLUDE_DIR}")

    # Informative message about source
    if(PHMAP_SOURCE STREQUAL "vendored")
        message(STATUS "Using vendored phmap (zero installation required)")
    else()
        message(STATUS "Using system-installed phmap from ${PHMAP_INCLUDE_DIR}")
    endif()
endif()

# Mark variables as advanced
mark_as_advanced(
    PHMAP_INCLUDE_DIR
    PHMAP_FOUND
    PHMAP_SOURCE
)
