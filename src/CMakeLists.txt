add_library(vmak_lib INTERFACE builder.hpp builders.hpp project.hpp result.hpp work_directory.hpp builders/cmake.hpp builders/cmake_preset.hpp builders/conan.hpp builders/ninja.hpp)

target_link_libraries(vmak_lib INTERFACE basic_prj::utils nlohmann_json::nlohmann_json fmt::fmt)
target_include_directories(vmak_lib INTERFACE ${CMAKE_CURRENT_LIST_DIR})

add_executable(vmak main.cpp)
target_link_libraries(vmak PRIVATE vmak_lib)

setup_target(vmak PUBLIC)
install(TARGETS vmak DESTINATION ${CMAKE_INSTALL_BINDIR})

enable_testing()

find_package(fmt REQUIRED)
find_package(nlohmann_json 3 REQUIRED)


find_package(Catch2)

if (Catch2_FOUND)
    add_executable(vmak_test tests/test.cpp)

    setup_target(vmak_test PRIVATE)
    target_link_libraries(vmak_test PRIVATE vmak_lib Catch2::Catch2WithMain)

    add_test(NAME vmak COMMAND vmak_test)
endif()
