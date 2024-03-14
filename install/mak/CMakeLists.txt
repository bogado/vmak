find_package(fmt REQUIRED)

add_executable(vbmak
    main.cpp
    project.hpp
)

target_link_libraries(vbmak PRIVATE fmt::fmt project_options basic_prj)
