########################################################################
# Thread config support
########################################################################
find_library(
    RT_LIBRARIES
    NAMES rt
    PATHS /usr/lib /usr/lib64
)

if (RT_LIBRARIES)
    list(APPEND SoapySDR_LIBRARIES ${RT_LIBRARIES})
endif()

if(WIN32)
    list(APPEND SOAPY_SERVER_SOURCES ThreadPrioWindows.cpp)
elseif(UNIX)
    list(APPEND SOAPY_SERVER_SOURCES ThreadPrioUnix.cpp)
endif()

########################################################################
# Build the remote server application
########################################################################
list(APPEND SOAPY_SERVER_SOURCES
    SoapyServer.cpp
    ServerListener.cpp
    ClientHandler.cpp
    LogForwarding.cpp
    ServerStreamData.cpp
)
if (MSVC)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/msvc)
endif ()
include_directories(${SoapySDR_INCLUDE_DIRS})
add_executable(SoapySDRServer ${SOAPY_SERVER_SOURCES})
target_link_libraries(SoapySDRServer ${SoapySDR_LIBRARIES} SoapySDRRemoteCommon)
install(TARGETS SoapySDRServer DESTINATION bin)
