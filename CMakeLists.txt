cmake_minimum_required(VERSION 3.16)

project(ctsimulator VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_GENERATE_QMLLS_INI ON)
set(CMAKE_PREFIX_PATH "C:/Qt6/6.11.0/mingw_64")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Quick Widgets Concurrent)

qt_policy(SET QTP0004 NEW)
qt_policy(SET QTP0001 NEW)

# Collect all backend source files
set(BACKEND_SOURCES
    source/CTController.cpp
    source/CTImageProvider.cpp
    source/drawingarea.cpp
    source/matrix.cpp
    source/utility.cpp
    source/artthread.cpp
    source/sirtthread.cpp
)

set(BACKEND_HEADERS
    source/CTController.h
    source/CTImageProvider.h
    source/drawingarea.h
    source/matrix.h
    source/utility.h
    source/artthread.h
    source/sirtthread.h
)

qt_add_executable(appuntitled
    main.cpp
    src/visualization/ColormapManager.cpp
    src/visualization/ImageViewer.cpp
    src/visualization/MetricsCalculator.cpp
    src/visualization/ROITools.cpp
    src/visualization/RayPathVisualizer.cpp
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
        qml/tutorials/TutorialManager.qml
        qml/tutorials/IntroductionTutorial.qml
        qml/tutorials/ProjectionsTutorial.qml
        qml/tutorials/AlgorithmsTutorial.qml
        qml/tutorials/NoiseTutorial.qml
        qml/tutorials/OptimizationTutorial.qml
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
