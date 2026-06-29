cmake_minimum_required(VERSION 3.31.0)
project(opendds-try VERSION 0.1.0 LANGUAGES C CXX)

# Compiler options
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

# Specify the libraries and headers directory
# TODO(wesly): Please adjust with your system configuration 
include_directories(
    "/opt/OpenDDS-3.29.1"
    "/opt/OpenDDS-3.29.1/ACE_wrappers"
    "/opt/OpenDDS-3.29.1/ACE_wrappers/TAO"
)
link_directories(
    "/opt/OpenDDS-3.29.1/lib"
    "/opt/OpenDDS-3.29.1/ACE_wrappers/lib"
)

# Specify the source code
file(GLOB_RECURSE SOURCES src/*.cpp src/*.cc src/*.h src/*.hpp)
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/sender.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/receiver.cpp)
add_library(shared-objects OBJECT ${SOURCES})

add_executable(receiver ${CMAKE_CURRENT_SOURCE_DIR}/src/receiver.cpp $<TARGET_OBJECTS:shared-objects>)
add_executable(sender ${CMAKE_CURRENT_SOURCE_DIR}/src/sender.cpp $<TARGET_OBJECTS:shared-objects>)

# Specify the library
target_link_libraries(receiver PRIVATE OpenDDS_Dcps TAO_Valuetype TAO ACE)
target_link_libraries(sender PRIVATE OpenDDS_Dcps TAO_Valuetype TAO ACE)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CMAKE_BUILD_TYPE Debug)
include(CPack)
