cmake_minimum_required(VERSION 3.25)
project(VMake LANGUAGES CXX VERSION 0.1)

find_package(basic_prj 1.0 REQUIRED GLOBAL)

message(STATUS compiler : "${CMAKE_CXX_COMPILER_ID}")
message(STATUS package : "${${basic}_PACKAGE}")

if (Catch2_FOUND)
    enable_testing()
endif()

add_subdirectory(src)
