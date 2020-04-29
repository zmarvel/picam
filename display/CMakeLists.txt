cmake_minimum_required(VERSION 3.7.0)

project(helloworld)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(QT_COMPONENTS
  Core Widgets Quick Network)

find_package(Qt5 COMPONENTS ${QT_COMPONENTS} REQUIRED)
find_package(Qt5QuickCompiler)

qtquick_compiler_add_resources(RESOURCES qml.qrc icons/icons.qrc)

add_executable(picam_display
    main.cpp ${RESOURCES}
)

qt5_use_modules(picam_display ${QT_COMPONENTS})

#target_link_libraries(picam_display Qt5::Widgets)
