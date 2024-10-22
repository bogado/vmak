add_executable(mak
    main.cpp
    arguments.hpp
    environment.hpp
    project.hpp
    stage.hpp
)

target_link_libraries(mak PRIVATE basic_prj::utils)
setup_target(mak)

enable_testing()

add_executable(mak_test
    test.cpp)
setup_target(mak_test)

find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)

target_link_libraries(mak_test PUBLIC Catch2::Catch2WithMain fmt::fmt)

add_test(NAME mak COMMAND mak_test)
