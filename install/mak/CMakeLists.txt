add_executable(vbmak
    main.cpp
    project.hpp
)

target_link_libraries(vbmak PRIVATE project_options basic_prj)
