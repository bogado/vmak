add_executable(mak
    main.cpp
    arguments.hpp
    environment.hpp
    project.hpp
    stage.hpp
)

target_link_libraries(mak PRIVATE basic_prj::utils)
setup_target(mak)

find_package(Catch2 REQUIRED)

enable_testing()

add_executable(mak_test
    test.cpp)

add_test(NAME mak COMMAND mak_test)

target_link_libraries(mak_test Catch2::Catch2WithMain)
