cmake_minimum_required(VERSION 3.15)

project(NNPDFDriver VERSION 1.0.8 LANGUAGES CXX Fortran)

include(CTest)
include(CheckCXXCompilerFlag)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(${CMAKE_CXX_COMPILER_ID}_CXX_FLAGS_TO_CHECK "")
set(MSVC_CXX_FLAGS_TO_CHECK
  "/bigobj;/O2;/W4;/permissive-;/w14242;/w14254;/w14263;/w14265;/w14287;/w14296;/w14311;/w14545;/w14546;/w14547;/w14549;/w14555;/w14619;/w14640;/w14826;/w14905;/w14906;/w14928"
)
set(NVHPC_CXX_FLAGS_TO_CHECK "-O3;-Wall;-Wextra;-Wpedantic")
set(Intel_CXX_FLAGS_TO_CHECK "-O3;-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow")
set(IntelLLVM_CXX_FLAGS_TO_CHECK "-O3;-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow")
set(GNU_CXX_FLAGS_TO_CHECK
  "-O3;-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow;-Wcast-align=strict;-Wdouble-promotion;-Wformat=2;-Wnull-dereference;-Wduplicated-cond;-Wduplicated-branches;-Wlogical-op;-Wuseless-cast;-Wold-style-cast;-Woverloaded-virtual;-Wnon-virtual-dtor;-Wpessimizing-move;-Wredundant-move"
)
set(Clang_CXX_FLAGS_TO_CHECK
  "-O3;-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow;-Wcast-align;-Wdouble-promotion;-Wformat=2;-Wnull-dereference;-Wold-style-cast;-Woverloaded-virtual;-Wnon-virtual-dtor;-Wpessimizing-move;-Wredundant-move"
)
set(AppleClang_CXX_FLAGS_TO_CHECK
  "-O3;-Wall;-Wextra;-Wpedantic;-Wconversion;-Wsign-conversion;-Wshadow;-Wcast-align;-Wdouble-promotion;-Wformat=2;-Wnull-dereference;-Wold-style-cast;-Woverloaded-virtual;-Wnon-virtual-dtor;-Wpessimizing-move;-Wredundant-move"
)
set(XL_CXX_FLAGS_TO_CHECK "-O3;-Wall;-Wextra;-Wpedantic")

foreach(flag ${${CMAKE_CXX_COMPILER_ID}_CXX_FLAGS_TO_CHECK})
  string(MAKE_C_IDENTIFIER "${flag}" flag_identifier)
  check_cxx_compiler_flag("${flag}" CXX_COMPILER_SUPPORTS_${flag_identifier})
  if(CXX_COMPILER_SUPPORTS_${flag_identifier})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
  endif()
endforeach()

add_library(nnpdfdriver STATIC
  source/NNPDFDriver.cc
)

target_include_directories(nnpdfdriver
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/source
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")
find_package(LHAPDF REQUIRED)

add_executable(testcode
  source/testcode.cc
)

target_link_libraries(testcode PRIVATE nnpdfdriver LHAPDF::LHAPDF)

add_executable(test1
  source/test1.cc
)

target_link_libraries(test1 PRIVATE nnpdfdriver LHAPDF::LHAPDF)

add_executable(testcode_fortran
  source/testcode.f
)

target_link_libraries(testcode_fortran PRIVATE nnpdfdriver)

set(NNPDF_TEST_GRID "${CMAKE_CURRENT_SOURCE_DIR}/NNPDF30_nlo_as_0118" CACHE FILEPATH
  "LHAPDF grid or grid directory used by the comparison test"
)

if(BUILD_TESTING AND NNPDF_TEST_GRID)
  add_test(
    NAME cpp_driver_comparison
    COMMAND testcode ${NNPDF_TEST_GRID} 0
  )
  add_test(
    NAME cpp_driver_timing
    COMMAND test1 ${NNPDF_TEST_GRID} 0 1000
  )
endif()
