cmake_minimum_required(VERSION 3.16)
project(BeatOven VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Network)
qt_standard_project_setup()

qt_add_executable(BeatOven
    main.cpp
    beatoven.conf

)

target_link_libraries(BeatOven PRIVATE Qt6::Widgets)
target_link_libraries(BeatOven PRIVATE Qt6::Network)
