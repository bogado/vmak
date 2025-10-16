add_library(vmake_lib INTERFACE builder.hpp builders.hpp project.hpp result.hpp work_directory.hpp builders/cmake.hpp builders/cmake_preset.hpp builders/conan.hpp builders/ninja.hpp)

target_link_libraries(vmake_lib INTERFACE basic_prj::utils nlohmann_json::nlohmann_json)
target_include_directories(vmake_lib INTERFACE ${CMAKE_CURRENT_LIST_DIR})

add_executable(vmk main.cpp)
target_link_libraries(vmk PRIVATE vmake_lib)

setup_target(vmk PUBLIC)
install(TARGETS vmk DESTINATION ${CMAKE_INSTALL_BINDIR})

enable_testing()

find_package(nlohmann_json 3 REQUIRED)

find_package(Catch2)

if (Catch2_FOUND)
    add_executable(vmak_test
        tests/arguments_tests.cpp
        tests/test.cpp
    )

    setup_target(vmak_test PRIVATE)
    target_link_libraries(vmak_test PRIVATE vmake_lib Catch2::Catch2WithMain)

    add_test(NAME vmak COMMAND vmak_test)
endif()
