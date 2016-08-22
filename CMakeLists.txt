cmake_minimum_required (VERSION 2.8)
project(ocl_examples)

set(OCL_EXAMPLES_SOURCES ${OCL_EXAMPLES_SOURCES}
    3rdparty/utils/bmp_save.cpp
    3rdparty/utils/dump_utils.cpp
    3rdparty/utils/stream.cpp
    3rdparty/utils/stream_utils.cpp
    examples/example1_reduce.cpp
    main.cpp
    ocl_hist.cpp
    ocl_radix_sort.cpp
    ocl_stream_compact.cpp
    ocl_test.cpp
    ocl_radix_sort2.cpp
)

set(PATH_TO_3RDPARTIES "../../3rdparty")
set(PATH_TO_OCL ${PATH_TO_3RDPARTIES}/ocl)
set(PATH_TO_LOCAL_3RDPARTIES "./3rdparty")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -ggdb")
add_definitions(-DDATA_ROOT="./data/")

add_subdirectory(${PATH_TO_OCL} "./out/ocl" )

find_library(OPENGL NAMES "opengl32" "GL")
find_library(OPENCL NAMES "OpenCL" PATHS /opt/AMDAPPSDK-2.9-1/lib/x86_64)

set (INC_DIRS ${INC_DIRS} ".")
set (INC_DIRS ${INC_DIRS} "/opt/AMDAPPSDK-2.9-1/include")
set (INC_DIRS ${INC_DIRS} ${PATH_TO_3RDPARTIES})
set (INC_DIRS ${INC_DIRS} ${PATH_TO_LOCAL_3RDPARTIES})

#set (INC_DIRS ${INC_DIRS} ${CMAKE_PREFIX_PATH}/include)

message("OPENCL library found in ${OPENCL}")
message("OPENGL library found in ${OPENGL}")

include_directories(${INC_DIRS})

add_executable(ocl_examples ${OCL_EXAMPLES_SOURCES})

target_link_libraries(ocl_examples ${OPENGL} ${OPENCL} ocl)
