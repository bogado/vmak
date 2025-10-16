cmake_minimum_required(VERSION 3.25)
project(VMake LANGUAGES CXX VERSION 0.1.0)

enable_testing()

find_package(basic_prj 1.0 REQUIRED GLOBAL)

message(STATUS compiler : "${CMAKE_CXX_COMPILER_ID}")
message(STATUS package : "${${basic}_PACKAGE}")

add_subdirectory(src)

# These are project-specific
set(CPACK_PACKAGE_NAME vmake)
set(CPACK_PACKAGE_VENDOR "Victor Bogado")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "vmk guesses the correct type of build and invokes it.")
# These lines tend to be the same for every project
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_VERBATIM_VARIABLES TRUE)

include(CPack)
