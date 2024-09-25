add_executable(vbmak
    main.cpp
    project.hpp
)

target_link_libraries(vbmak PRIVATE basic_prj::utils)
