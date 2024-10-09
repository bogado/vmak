add_executable(mak
    main.cpp
    arguments.hpp
    environment.hpp
    project.hpp
    stage.hpp
)

target_link_libraries(mak PRIVATE vbconf_lib basic_prj::utils)

find_package(Catch2 REQUIRED)

add_executable(mak_test
    test.cpp)

target_link_libraries(mak_test Catch2::Catch2WithMain)
