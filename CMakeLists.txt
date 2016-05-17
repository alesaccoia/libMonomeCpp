cmake_minimum_required(VERSION 2.8)

project(libMonomeCpp)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(SetTargetOptions)

find_package (monome)
if (NOT MONOME_FOUND)
  message(FATAL_ERROR "Impossible to find monome library: download from https://github.com/monome/libmonome or use your OS package manager")
endif ()


# Configure the TPCircularBuffer lib
set(TPCircularBuffer_src
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/TPCircularBuffer/TPCircularBuffer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/TPCircularBuffer/TPCircularBuffer+AudioBufferList.c)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/TPCircularBuffer)
add_library(TPCircularBuffer ${TPCircularBuffer_src})


# Configure the libMonomeCpp lib
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/)
include_directories(${MONOME_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/TPCircularBuffer)
add_library(monomeCpp ${CMAKE_CURRENT_SOURCE_DIR}/src/MonomeGrid.cpp)
set_target_properties(monomeCpp PROPERTIES COMPILE_FLAGS "-std=c++11")

# build the examples

add_executable(01-grid ${CMAKE_CURRENT_SOURCE_DIR}/examples/01-grid.cpp)
target_link_libraries(01-grid monomeCpp TPCircularBuffer ${MONOME_LIBRARIES})
set_target_properties(monomeCpp PROPERTIES COMPILE_FLAGS "-std=c++11")

add_executable(02-sequencer ${CMAKE_CURRENT_SOURCE_DIR}/examples/02-sequencer.cpp)
target_link_libraries(02-sequencer monomeCpp TPCircularBuffer ${MONOME_LIBRARIES})
set_target_properties(monomeCpp PROPERTIES COMPILE_FLAGS "-std=c++11")

add_executable(03-externalSequencer ${CMAKE_CURRENT_SOURCE_DIR}/examples/03-externalSequencer.cpp)
target_link_libraries(03-externalSequencer monomeCpp TPCircularBuffer ${MONOME_LIBRARIES})
set_target_properties(monomeCpp PROPERTIES COMPILE_FLAGS "-std=c++11")






