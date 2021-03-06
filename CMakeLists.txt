########################################################################
# Build Soapy SDR remote support
########################################################################
cmake_minimum_required(VERSION 2.8.7)
project(SoapyRemote CXX C)

# select build type to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

#find soapy sdr
find_package(SoapySDR "0.3.0" NO_MODULE)

#enable c++11 features
if(CMAKE_COMPILER_IS_GNUCXX)

    #C++11 is a required language feature for this project
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

    #Thread support enabled (not the same as -lpthread)
    list(APPEND SoapySDR_LIBRARIES -pthread)

endif(CMAKE_COMPILER_IS_GNUCXX)

#common headers used by client and server
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common)

add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)
