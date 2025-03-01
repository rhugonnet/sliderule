# SlideRule top-level CMake build script

project (SLIDERULE LANGUAGES CXX)

#################
# CMake Options #
#################

set (CMAKE_LEGACY_CYGWIN_WIN32 0) # Squelch a warning when building on Win32/Cygwin
cmake_minimum_required (VERSION 3.13.0) # The minimum CMake version is chosen to enable policy CMP0079
cmake_policy(SET CMP0079 NEW) # add link library to target which is not built in this directory
cmake_policy(SET CMP0053 NEW) # simplified variable escape processing (recommended by cmake)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(default_build_type "Release")
    if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
      set(default_build_type "Debug")
    endif()
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
endif()

# Set a default platform
if(NOT CMAKE_BUILD_PLATFORM)
    set(CMAKE_BUILD_PLATFORM "Linux" CACHE STRING "Choose the target platform." FORCE)
endif()

###################
# Project Options #
###################

# Project Options #

option (PYTHON_BINDINGS "Create Python bindings, including h5lite module" OFF)
option (SHARED_LIBRARY "Create shared library instead of sliderule binary" OFF)
option (SERVER_APP "Create sliderule server binary" ON)

# Library Options #

option (ENABLE_COMPAT "Use C++11 for compatibility with older compilers" OFF)
option (ENABLE_ADDRESS_SANITIZER "Instrument code with AddressSanitizer for memory error detection" OFF)
option (ENABLE_TIME_HEARTBEAT "Instruct TimeLib to use a 1KHz heart beat timer to set millisecond time resolution" OFF)
option (ENABLE_CUSTOM_ALLOCATOR "Override new and delete operators globally for debug purposes" OFF)
option (ENABLE_H5CORO_ATTRIBUTE_SUPPORT "H5Coro will read and process attribute messages" OFF)
option (ENABLE_APACHE_ARROW_10_COMPAT "Use Apache Arrow 11 interface" OFF)
option (ENABLE_BEST_EFFORT_CONDA_ENV "Attempt to alleviate some issues with running in a conda environment")

# Package Options #

option (USE_ARROW_PACKAGE "Include the Apache Arrow package" OFF)
option (USE_AWS_PACKAGE "Include the AWS package" OFF)
option (USE_CCSDS_PACKAGE "Include the CCSDS package" ON)
option (USE_GEO_PACKAGE "Include the GEO package" OFF)
option (USE_H5_PACKAGE "Include the HDF5 package" ON)
option (USE_LEGACY_PACKAGE "Include the Legacy package" ON)
option (USE_NETSVC_PACKAGE "Include the Network Services package" OFF)
option (USE_PISTACHE_PACKAGE "Include the Pistache package" OFF)

# Platform Options #

option (ENABLE_TRACING "Instantiate trace points" OFF)
option (ENABLE_TERMINAL "Instantiate terminal messages" ON)

#####################
# Target: SlideRule #
#####################

# SlideRule Library #

if(${SHARED_LIBRARY})
    add_library(slideruleLib SHARED "")
else()
    add_library(slideruleLib STATIC "")
endif()

# Version Information #

file(STRINGS ${PROJECT_SOURCE_DIR}/version.txt TGTVER)
string(REPLACE "v" "" LIBVER ${TGTVER})

# C++ Version #

if(${ENABLE_COMPAT})
    set(CXX_VERSION 11)
else()
    set(CXX_VERSION 17) # required if using pistache package
endif()

# Platform #

set_property(GLOBAL PROPERTY platform "")

if(CMAKE_BUILD_PLATFORM MATCHES "Linux")

    # Prefer libraries installed in /usr/local
    INCLUDE_DIRECTORIES(/usr/local/include)
    LINK_DIRECTORIES(/usr/local/lib)

    # Set Environment Variables
    set (INSTALLDIR /usr/local CACHE STRING "Installation directory for library and executables")
    set (CONFDIR ${INSTALLDIR}/etc/sliderule)
    set (INCDIR ${INSTALLDIR}/include/sliderule)

    # Include Linux Platform #
    add_subdirectory (platforms/linux)

elseif(CMAKE_BUILD_PLATFORM MATCHES "Windows")

    # Set Environment Variables
    set (INSTALLDIR "C:\\Program Files\\SlideRule" CACHE STRING "Installation directory for library and executables")
    set (CONFDIR ${INSTALLDIR}\etc\sliderule)
    set (INCDIR ${INSTALLDIR}\include\sliderule)

    # Include Windows Platform #
    add_subdirectory (platforms/windows)

endif()

# Target Properties #

set_target_properties (slideruleLib PROPERTIES VERSION ${LIBVER})
set_target_properties (slideruleLib PROPERTIES OUTPUT_NAME sliderule)
set_target_properties (slideruleLib PROPERTIES ENABLE_EXPORTS true)
set_target_properties (slideruleLib PROPERTIES CXX_STANDARD ${CXX_VERSION})

if(${PYTHON_BINDINGS})
    set_target_properties (slideruleLib PROPERTIES POSITION_INDEPENDENT_CODE 1)
endif()

# Compile Definitions #

set (RUNTIMEDIR ${CONFDIR} CACHE STRING "Runtime directory for plugins and configuration scripts")

if (${ENABLE_TIME_HEARTBEAT})
    message (STATUS "Enabling time heartbeat")
    target_compile_definitions (slideruleLib PUBLIC TIME_HEARTBEAT)
endif ()

if (${ENABLE_CUSTOM_ALLOCATOR})
    message (STATUS "Enabling custom allocator")
    target_compile_definitions (slideruleLib PUBLIC CUSTOM_ALLOCATOR)
endif ()

if (DEFINED MAX_FREE_STACK_SIZE)
    message (STATUS "Setting MAX_FREE_STACK_SIZE to " ${MAX_FREE_STACK_SIZE})
    target_compile_definitions (slideruleLib PUBLIC MAX_FREE_STACK_SIZE=${MAX_FREE_STACK_SIZE})
endif ()

if (DEFINED H5CORO_THREAD_POOL_SIZE)
    message (STATUS "Setting H5CORO_THREAD_POOL_SIZE to " ${H5CORO_THREAD_POOL_SIZE})
    target_compile_definitions (slideruleLib PUBLIC H5CORO_THREAD_POOL_SIZE=${H5CORO_THREAD_POOL_SIZE})
endif ()

if (DEFINED H5CORO_MAXIMUM_NAME_SIZE)
    message (STATUS "Setting H5CORO_MAXIMUM_NAME_SIZE to " ${H5CORO_MAXIMUM_NAME_SIZE})
    target_compile_definitions (slideruleLib PUBLIC H5CORO_MAXIMUM_NAME_SIZE=${H5CORO_MAXIMUM_NAME_SIZE})
endif ()

if (${ENABLE_H5CORO_ATTRIBUTE_SUPPORT})
    message (STATUS "Enabling support in H5Coro for attribute messages")
    target_compile_definitions (slideruleLib PUBLIC H5CORO_ATTRIBUTE_SUPPORT)
endif ()

if (${ENABLE_APACHE_ARROW_10_COMPAT})
    message (STATUS "Enabling Apache Arrow 10 compatibility")
    target_compile_definitions (slideruleLib PUBLIC APACHE_ARROW_10_COMPAT)
endif ()

if (${ENABLE_BEST_EFFORT_CONDA_ENV})
    message (STATUS "Attempting best effort at running in a mixed system and conda environment")
    target_compile_definitions (slideruleLib PUBLIC BEST_EFFORT_CONDA_ENV)
endif ()

# Platform File #

set_property(GLOBAL APPEND PROPERTY platform "#define LIBID \"${TGTVER}\"\n")
set_property(GLOBAL APPEND PROPERTY platform "#define CONFDIR \"${RUNTIMEDIR}\"\n")

if(${ENABLE_TRACING})
    message (STATUS "Enabling trace points")
    set_property(GLOBAL APPEND PROPERTY platform "#define __tracing__\n")
endif()

if(${ENABLE_TERMINAL})
    message (STATUS "Enabling terminal messages")
    set_property(GLOBAL APPEND PROPERTY platform "#define __terminal__\n")
endif()

include (TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
    message (STATUS "Enabling code compilation for big endian machine")
    set_property(GLOBAL APPEND PROPERTY platform "#define __be__\n")
else()
    message (STATUS "Enabling code compilation for little endian machine")
    set_property(GLOBAL APPEND PROPERTY platform "#define __le__\n")
endif()

get_property(PLATFORM GLOBAL PROPERTY platform)
file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/auto/platform.h ${PLATFORM})

target_include_directories (slideruleLib PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/auto)

install (FILES ${CMAKE_CURRENT_BINARY_DIR}/auto/platform.h DESTINATION ${INCDIR})

# Add Packages #

add_subdirectory (packages/core)

if(${USE_ARROW_PACKAGE})
    add_subdirectory (packages/arrow)
endif()

if(${USE_AWS_PACKAGE})
    add_subdirectory (packages/aws)
endif()

if(${USE_CCSDS_PACKAGE})
    add_subdirectory (packages/ccsds)
endif()

if(${USE_GEO_PACKAGE})
    add_subdirectory (packages/geo)
endif()

if(${USE_H5_PACKAGE})
    add_subdirectory (packages/h5)
endif()

if(${USE_LEGACY_PACKAGE})
    add_subdirectory (packages/legacy)
endif()

if(${USE_NETSVC_PACKAGE})
    add_subdirectory (packages/netsvc)
endif()

if(${USE_PISTACHE_PACKAGE})
    add_subdirectory (packages/pistache)
endif()

# Scripts #

add_subdirectory (scripts)

# Executables #

install (TARGETS slideruleLib DESTINATION ${INSTALLDIR}/lib)

if(${PYTHON_BINDINGS})

    add_subdirectory (targets/binding-python)

elseif(${SERVER_APP} AND (CMAKE_BUILD_PLATFORM MATCHES "Linux"))

    add_subdirectory (targets/server-linux)

endif()
