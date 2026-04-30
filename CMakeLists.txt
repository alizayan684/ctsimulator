cmake_minimum_required(VERSION 3.16)

project(untitled VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_GENERATE_QMLLS_INI ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Widgets Concurrent)

qt_standard_project_setup(REQUIRES 6.5)

# Collect all backend source files
set(BACKEND_SOURCES
    source/CTController.cpp
    source/CTImageProvider.cpp
    source/drawingarea.cpp
    source/matrix.cpp
    source/matrixwidget.cpp
    source/utility.cpp
    source/artthread.cpp
    source/sirtthread.cpp
    source/imagewidget.cpp
    source/sinogramwidget.cpp
)

set(BACKEND_HEADERS
    source/CTController.h
    source/CTImageProvider.h
    source/drawingarea.h
    source/matrix.h
    source/matrixwidget.h
    source/utility.h
    source/artthread.h
    source/sirtthread.h
    source/imagewidget.h
    source/sinogramwidget.h
)

qt_add_executable(appuntitled
    main.cpp
    ${BACKEND_SOURCES}
    ${BACKEND_HEADERS}
)

# Add include directory for source files
target_include_directories(appuntitled PRIVATE source)

qt_add_qml_module(appuntitled
    URI untitled
    QML_FILES
        Main.qml
        MainView.qml
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appuntitled PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appuntitled
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appuntitled
    PRIVATE Qt6::Quick Qt6::Widgets Qt6::Concurrent
)

install(TARGETS appuntitled
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
