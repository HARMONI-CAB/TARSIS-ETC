cmake_minimum_required(VERSION 3.5)

project(CalGUI VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets Charts)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Charts)

include(FindPkgConfig)
pkg_check_modules(YAMLCPP yaml-cpp>=0.6.0)

set(PROJECT_SOURCES
        CalculationWorker.h CalculationWorker.cpp
        GUIHelpers.h GUIHelpers.cpp
        icons.qrc
        main.cpp
        MainWindow.cpp
        MainWindow.h
        MainWindow.ui
        RangeLimitedValueAxis.h RangeLimitedValueAxis.cpp
        ZoomableChart.h ZoomableChart.cpp
        ZoomableChartView.h ZoomableChartView.cpp
        ZoomableChartWidget.h ZoomableChartWidget.cpp ZoomableChartWidget.ui
        res/zoomable_chart_widget_resources.qrc
)

# Define application icons
set(MACOSX_BUNDLE_ICON_FILE CalGUI.icns)
set(app_icon_macos "${CMAKE_CURRENT_SOURCE_DIR}/icons/CalGUI.icns")
set_source_files_properties(
    ${app_icon_macos}
    PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources")
           
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(CalGUI
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )

# Define target properties for Android with Qt 6 as:
#    set_property(TARGET CalGUI APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(CalGUI SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(CalGUI
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_directories(CalGUI PRIVATE ${LIBETC_LIBDIR} ${YAMLCPP_LIBRARY_DIRS})
target_link_libraries(CalGUI PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Charts ETC ${YAMLCPP_LIBRARIES})
target_include_directories(
  CalGUI
  PRIVATE ../LibETC/include ${YAMLCPP_INCLUDE_DIRS})

set_target_properties(CalGUI PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER calgui.tarsisetc.cab.inta-csic.es
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(
    TARGETS CalGUI
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION bundles)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(CalGUI)
endif()
