cmake_minimum_required(VERSION 3.7.0)

project(helloworld)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(picam_display
    main.cpp
)

target_link_libraries(picam_display Qt5::Widgets)
