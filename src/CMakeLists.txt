add_executable(vmak
    main.cpp
    builder.hpp
    builders.hpp
    project.hpp
    work_directory.hpp
)

target_link_libraries(vmak PRIVATE basic_prj::utils)
setup_target(vmak PUBLIC)
install(TARGETS vmak DESTINATION ${CMAKE_INSTALL_BINDIR})

enable_testing()

find_package(fmt REQUIRED)

target_link_libraries(vmak PRIVATE fmt::fmt)

find_package(Catch2)

if (Catch2_FOUND)
    add_executable(vmak_test
        test.cpp)

    setup_target(vmak_test PRIVATE)
        target_link_libraries(vmak_test PUBLIC basic_prj::utils Catch2::Catch2WithMain fmt::fmt)

    add_test(NAME vmak COMMAND vmak_test)
endif()
