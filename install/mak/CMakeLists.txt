add_executable(mak
    main.cpp
    builder.hpp
    builders.hpp
    project.hpp
    work_directory.hpp
)

target_link_libraries(mak PRIVATE basic_prj::utils)
setup_target(mak PUBLIC)
install(TARGETS mak DESTINATION ${CMAKE_INSTALL_BINDIR})

enable_testing()

find_package(fmt REQUIRED)

target_link_libraries(mak PRIVATE fmt::fmt)

find_package(Catch2)

if (Catch2_FOUND)
    add_executable(mak_test
        test.cpp)

    setup_target(mak_test PRIVATE)
        target_link_libraries(mak_test PUBLIC basic_prj::utils Catch2::Catch2WithMain fmt::fmt)

    add_test(NAME mak COMMAND mak_test)
endif()
