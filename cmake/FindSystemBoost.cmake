# FindSystemBoost.cmake - Locate system Boost 1.89+ with fallback support
cmake_minimum_required(VERSION 3.24)

# Check for environment variable or CMake variable first
if(DEFINED ENV{BOOST_ROOT})
    set(BOOST_ROOT $ENV{BOOST_ROOT})
    message(STATUS "Using BOOST_ROOT from environment: ${BOOST_ROOT}")
elseif(NOT DEFINED BOOST_ROOT)
    # Fallback: Try common installation locations
    if(WIN32)
        if(EXISTS "C:/libs/boost_1_89_0-bin-msvc-all-32-64/boost_1_89_0")
            set(BOOST_ROOT "C:/libs/boost_1_89_0-bin-msvc-all-32-64/boost_1_89_0")
            message(STATUS "Using default Boost location: ${BOOST_ROOT}")
        elseif(EXISTS "C:/local/boost_1_89_0")
            set(BOOST_ROOT "C:/local/boost_1_89_0")
            message(STATUS "Using Boost at C:/local: ${BOOST_ROOT}")
        elseif(EXISTS "C:/Program Files/boost/boost_1_89_0")
            set(BOOST_ROOT "C:/Program Files/boost/boost_1_89_0")
            message(STATUS "Using Boost at Program Files: ${BOOST_ROOT}")
        endif()
    else()
        # Linux/Unix fallback paths
        if(EXISTS "/usr/local/boost_1_89_0")
            set(BOOST_ROOT "/usr/local/boost_1_89_0")
        elseif(EXISTS "/opt/boost_1_89_0")
            set(BOOST_ROOT "/opt/boost_1_89_0")
        endif()
    endif()
endif()

# Only proceed with custom logic if BOOST_ROOT is set
if(DEFINED BOOST_ROOT AND EXISTS "${BOOST_ROOT}")
    message(STATUS "Attempting to use system Boost from: ${BOOST_ROOT}")

    # Clear vcpkg interference only when using custom Boost
    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG FALSE)

    set(BOOST_INCLUDEDIR "${BOOST_ROOT}")

    # Auto-detect library directory based on compiler
    if(MSVC)
        # Detect MSVC toolset version (e.g., 14.3 for VS2022)
        if(MSVC_TOOLSET_VERSION)
            string(LENGTH "${MSVC_TOOLSET_VERSION}" _TOOLSET_LEN)
            math(EXPR _TOOLSET_LEN "${_TOOLSET_LEN} - 1")
            string(SUBSTRING "${MSVC_TOOLSET_VERSION}" 0 ${_TOOLSET_LEN} _TOOLSET_MAJOR)
            string(SUBSTRING "${MSVC_TOOLSET_VERSION}" ${_TOOLSET_LEN} -1 _TOOLSET_MINOR)
            set(_MSVC_VER "${_TOOLSET_MAJOR}.${_TOOLSET_MINOR}")
        else()
            # Fallback for older CMake versions
            set(_MSVC_VER "14.3")
        endif()

        # Try different library directory naming conventions
        if(EXISTS "${BOOST_ROOT}/lib64-msvc-${_MSVC_VER}")
            set(BOOST_LIBRARYDIR "${BOOST_ROOT}/lib64-msvc-${_MSVC_VER}")
        elseif(EXISTS "${BOOST_ROOT}/lib")
            set(BOOST_LIBRARYDIR "${BOOST_ROOT}/lib")
        elseif(EXISTS "${BOOST_ROOT}/stage/lib")
            set(BOOST_LIBRARYDIR "${BOOST_ROOT}/stage/lib")
        else()
            message(WARNING "Could not auto-detect Boost library directory. Tried: lib64-msvc-${_MSVC_VER}, lib, stage/lib")
        endif()
    else()
        # Non-MSVC (GCC, Clang, etc.)
        if(EXISTS "${BOOST_ROOT}/lib")
            set(BOOST_LIBRARYDIR "${BOOST_ROOT}/lib")
        elseif(EXISTS "${BOOST_ROOT}/stage/lib")
            set(BOOST_LIBRARYDIR "${BOOST_ROOT}/stage/lib")
        endif()
    endif()
else()
    message(STATUS "BOOST_ROOT not set or doesn't exist. Will attempt standard find_package.")
    # Let CMake's standard mechanism handle it
    set(USE_STANDARD_BOOST_SEARCH TRUE)
endif()

# Only use custom search if we have a valid BOOST_ROOT
if(NOT USE_STANDARD_BOOST_SEARCH)
    # Force CMake to use custom paths
    set(Boost_NO_SYSTEM_PATHS ON)
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_MULTITHREADED ON)
    set(Boost_USE_STATIC_RUNTIME OFF)

    # Auto-detect compiler and architecture
    if(MSVC)
        set(Boost_COMPILER "-vc${MSVC_TOOLSET_VERSION}")
    endif()
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(Boost_ARCHITECTURE "-x64")
    else()
        set(Boost_ARCHITECTURE "-x32")
    endif()

    # Disable boost auto-linking on Windows
    add_definitions(-DBOOST_ALL_NO_LIB)

    message(STATUS "Using custom Boost search from: ${BOOST_ROOT}")

    # For multi-config generators (Visual Studio), find both Release and Debug libraries
    # Manually find boost libraries using old-style approach
    find_path(Boost_INCLUDE_DIRS
        NAMES boost/version.hpp
        PATHS "${BOOST_ROOT}"
        NO_DEFAULT_PATH)

    # Note: Boost.System is header-only since 1.69, no separate library needed

    # Detect Boost version from version.hpp
    if(EXISTS "${Boost_INCLUDE_DIRS}/boost/version.hpp")
        file(STRINGS "${Boost_INCLUDE_DIRS}/boost/version.hpp" BOOST_VERSION_LINE REGEX "define BOOST_VERSION ")
        string(REGEX REPLACE ".*#define BOOST_VERSION ([0-9]+).*" "\\1" BOOST_VERSION_NUMBER "${BOOST_VERSION_LINE}")
        math(EXPR BOOST_VERSION_MAJOR "${BOOST_VERSION_NUMBER} / 100000")
        math(EXPR BOOST_VERSION_MINOR "(${BOOST_VERSION_NUMBER} / 100) % 1000")
        set(BOOST_VERSION_STR "${BOOST_VERSION_MAJOR}_${BOOST_VERSION_MINOR}")
        message(STATUS "Detected Boost version: ${BOOST_VERSION_MAJOR}.${BOOST_VERSION_MINOR}")
    else()
        # Fallback to 1.89 if detection fails
        set(BOOST_VERSION_STR "1_89")
        message(WARNING "Could not detect Boost version, assuming 1.89")
    endif()

    # Build library name patterns
    if(MSVC)
        set(_BOOST_LIB_PREFIX "libboost_")
        set(_BOOST_LIB_SUFFIX "-vc${MSVC_TOOLSET_VERSION}-mt${Boost_ARCHITECTURE}-${BOOST_VERSION_STR}")
        set(_BOOST_LIB_SUFFIX_DEBUG "-vc${MSVC_TOOLSET_VERSION}-mt-gd${Boost_ARCHITECTURE}-${BOOST_VERSION_STR}")
    else()
        set(_BOOST_LIB_PREFIX "libboost_")
        set(_BOOST_LIB_SUFFIX "")
        set(_BOOST_LIB_SUFFIX_DEBUG "")
    endif()

    # Find Release libraries
    find_library(Boost_THREAD_LIBRARY_RELEASE
        NAMES ${_BOOST_LIB_PREFIX}thread${_BOOST_LIB_SUFFIX} boost_thread-mt boost_thread
        PATHS "${BOOST_LIBRARYDIR}"
        NO_DEFAULT_PATH)

    find_library(Boost_FILESYSTEM_LIBRARY_RELEASE
        NAMES ${_BOOST_LIB_PREFIX}filesystem${_BOOST_LIB_SUFFIX} boost_filesystem-mt boost_filesystem
        PATHS "${BOOST_LIBRARYDIR}"
        NO_DEFAULT_PATH)

    find_library(Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE
        NAMES ${_BOOST_LIB_PREFIX}program_options${_BOOST_LIB_SUFFIX} boost_program_options-mt boost_program_options
        PATHS "${BOOST_LIBRARYDIR}"
        NO_DEFAULT_PATH)

    find_library(Boost_REGEX_LIBRARY_RELEASE
        NAMES ${_BOOST_LIB_PREFIX}regex${_BOOST_LIB_SUFFIX} boost_regex-mt boost_regex
        PATHS "${BOOST_LIBRARYDIR}"
        NO_DEFAULT_PATH)

    find_library(Boost_LOCALE_LIBRARY_RELEASE
        NAMES ${_BOOST_LIB_PREFIX}locale${_BOOST_LIB_SUFFIX} boost_locale-mt boost_locale
        PATHS "${BOOST_LIBRARYDIR}"
        NO_DEFAULT_PATH)

    # Find Debug libraries (-gd suffix for MSVC)
    find_library(Boost_THREAD_LIBRARY_DEBUG
        NAMES ${_BOOST_LIB_PREFIX}thread${_BOOST_LIB_SUFFIX_DEBUG} boost_thread-mt-gd boost_thread-gd
        PATHS "${BOOST_LIBRARYDIR}"
        NO_DEFAULT_PATH)

    find_library(Boost_FILESYSTEM_LIBRARY_DEBUG
        NAMES ${_BOOST_LIB_PREFIX}filesystem${_BOOST_LIB_SUFFIX_DEBUG} boost_filesystem-mt-gd boost_filesystem-gd
        PATHS "${BOOST_LIBRARYDIR}"
        NO_DEFAULT_PATH)

    find_library(Boost_PROGRAM_OPTIONS_LIBRARY_DEBUG
        NAMES ${_BOOST_LIB_PREFIX}program_options${_BOOST_LIB_SUFFIX_DEBUG} boost_program_options-mt-gd boost_program_options-gd
        PATHS "${BOOST_LIBRARYDIR}"
        NO_DEFAULT_PATH)

    find_library(Boost_REGEX_LIBRARY_DEBUG
        NAMES ${_BOOST_LIB_PREFIX}regex${_BOOST_LIB_SUFFIX_DEBUG} boost_regex-mt-gd boost_regex-gd
        PATHS "${BOOST_LIBRARYDIR}"
        NO_DEFAULT_PATH)

    find_library(Boost_LOCALE_LIBRARY_DEBUG
        NAMES ${_BOOST_LIB_PREFIX}locale${_BOOST_LIB_SUFFIX_DEBUG} boost_locale-mt-gd boost_locale-gd
        PATHS "${BOOST_LIBRARYDIR}"
        NO_DEFAULT_PATH)

    # Set the main library variables using Release versions for validation
    set(Boost_THREAD_LIBRARY ${Boost_THREAD_LIBRARY_RELEASE})
    set(Boost_FILESYSTEM_LIBRARY ${Boost_FILESYSTEM_LIBRARY_RELEASE})
    set(Boost_PROGRAM_OPTIONS_LIBRARY ${Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE})
    set(Boost_REGEX_LIBRARY ${Boost_REGEX_LIBRARY_RELEASE})
    set(Boost_LOCALE_LIBRARY ${Boost_LOCALE_LIBRARY_RELEASE})

    if(Boost_INCLUDE_DIRS AND Boost_THREAD_LIBRARY_RELEASE AND Boost_FILESYSTEM_LIBRARY_RELEASE AND Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE AND Boost_REGEX_LIBRARY_RELEASE AND Boost_LOCALE_LIBRARY_RELEASE)
        set(Boost_FOUND TRUE)

        # Use optimized library selection for multi-config generators
        # Use Debug libraries if available, otherwise fall back to Release for Debug builds
        if(Boost_THREAD_LIBRARY_DEBUG)
            set(Boost_LIBRARIES
                optimized ${Boost_THREAD_LIBRARY_RELEASE} debug ${Boost_THREAD_LIBRARY_DEBUG}
                optimized ${Boost_FILESYSTEM_LIBRARY_RELEASE} debug ${Boost_FILESYSTEM_LIBRARY_DEBUG}
                optimized ${Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE} debug ${Boost_PROGRAM_OPTIONS_LIBRARY_DEBUG}
                optimized ${Boost_REGEX_LIBRARY_RELEASE} debug ${Boost_REGEX_LIBRARY_DEBUG}
                optimized ${Boost_LOCALE_LIBRARY_RELEASE} debug ${Boost_LOCALE_LIBRARY_DEBUG})
        else()
            # No Debug libraries found, use Release for both configurations
            set(Boost_LIBRARIES
                ${Boost_THREAD_LIBRARY_RELEASE}
                ${Boost_FILESYSTEM_LIBRARY_RELEASE}
                ${Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE}
                ${Boost_REGEX_LIBRARY_RELEASE}
                ${Boost_LOCALE_LIBRARY_RELEASE})
        endif()

        message(STATUS "✅ Custom Boost found at: ${Boost_INCLUDE_DIRS}")
        message(STATUS "✅ Boost Release libraries found: ${Boost_THREAD_LIBRARY_RELEASE};${Boost_FILESYSTEM_LIBRARY_RELEASE};${Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE};${Boost_REGEX_LIBRARY_RELEASE};${Boost_LOCALE_LIBRARY_RELEASE}")
        if(Boost_THREAD_LIBRARY_DEBUG)
            message(STATUS "✅ Boost Debug libraries found: ${Boost_THREAD_LIBRARY_DEBUG};${Boost_FILESYSTEM_LIBRARY_DEBUG};${Boost_PROGRAM_OPTIONS_LIBRARY_DEBUG};${Boost_REGEX_LIBRARY_DEBUG};${Boost_LOCALE_LIBRARY_DEBUG}")
        else()
            message(STATUS "⚠️  Boost Debug libraries not found (Release libraries will be used for Debug builds)")
        endif()
    else()
        message(WARNING "Custom Boost search failed. Diagnostic information:")
        message(STATUS "  Include dirs: ${Boost_INCLUDE_DIRS}")
        message(STATUS "  BOOST_LIBRARYDIR: ${BOOST_LIBRARYDIR}")
        message(STATUS "  Thread lib (Release): ${Boost_THREAD_LIBRARY_RELEASE}")
        message(STATUS "  Filesystem lib (Release): ${Boost_FILESYSTEM_LIBRARY_RELEASE}")
        message(STATUS "  Program options lib (Release): ${Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE}")
        message(STATUS "  Regex lib (Release): ${Boost_REGEX_LIBRARY_RELEASE}")
        message(STATUS "  Locale lib (Release): ${Boost_LOCALE_LIBRARY_RELEASE}")
        message(STATUS "Falling back to standard find_package(Boost)...")
        set(USE_STANDARD_BOOST_SEARCH TRUE)
    endif()
endif()

# Fallback to standard CMake Boost finding if custom search failed or wasn't attempted
if(USE_STANDARD_BOOST_SEARCH)
    message(STATUS "Using standard CMake Boost finding mechanism")

    # Clear previous attempts
    unset(Boost_FOUND)
    unset(Boost_NO_SYSTEM_PATHS)

    # Use standard CMake FindBoost
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_MULTITHREADED ON)
    set(Boost_USE_STATIC_RUNTIME OFF)

    if(WIN32)
        set(BOOST_REQUIRED_VERSION 1.74)  # Minimum version
    else()
        set(BOOST_REQUIRED_VERSION 1.74)
    endif()

    find_package(Boost ${BOOST_REQUIRED_VERSION} REQUIRED
        COMPONENTS
            filesystem
            program_options
            regex
            locale
            thread)

    if(Boost_FOUND)
        message(STATUS "✅ Standard Boost ${Boost_VERSION} found at: ${Boost_INCLUDE_DIRS}")
        set(Boost_LIBRARIES ${Boost_LIBRARIES})
        set(Boost_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "❌ Boost not found. Please install Boost 1.74+ or set BOOST_ROOT environment variable.")
    endif()
endif()
