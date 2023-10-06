# Add all the source files needed to build the executable

set(LIBETC_INCLUDEDIR include)
set(LIBETC_SRCDIR     src)

set(CMAKE_CXX_STANDARD 17)

include(FindPkgConfig)
pkg_check_modules(YAMLCPP yaml-cpp>=0.7.0)

set(LIBETC_LIBDIR "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)

set(LIBETC_SOURCES
  ${LIBETC_SRCDIR}/Curve.cpp
  ${LIBETC_SRCDIR}/ConfigManager.cpp
  ${LIBETC_SRCDIR}/DataFileManager.cpp
  ${LIBETC_SRCDIR}/Detector.cpp
  ${LIBETC_SRCDIR}/InstrumentModel.cpp
  ${LIBETC_SRCDIR}/Simulation.cpp
  ${LIBETC_SRCDIR}/SkyModel.cpp
  ${LIBETC_SRCDIR}/Spectrum.cpp)

set(LIBETC_HEADERS
  ${LIBETC_INCLUDEDIR}/Curve.h
  ${LIBETC_INCLUDEDIR}/ConfigManager.h
  ${LIBETC_INCLUDEDIR}/DataFileManager.h
  ${LIBETC_INCLUDEDIR}/Detector.h
  ${LIBETC_INCLUDEDIR}/Helpers.h
  ${LIBETC_INCLUDEDIR}/InstrumentModel.h
  ${LIBETC_INCLUDEDIR}/Simulation.h
  ${LIBETC_INCLUDEDIR}/SkyModel.h
  ${LIBETC_INCLUDEDIR}/Spectrum.h)

add_library(
  ETC
  SHARED
  ${LIBETC_SOURCES}
  ${LIBETC_HEADERS})

target_include_directories(
  ETC 
  PRIVATE
  ${LIBETC_INCLUDEDIR} ${YAMLCPP_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

target_link_directories(
  ETC
  PRIVATE
  ${YAMLCPP_LIBRARY_DIRS})

target_link_libraries(ETC PRIVATE ${YAMLCPP_LIBRARIES})

if(APPLE)
  # Required to retrieve bundle path
  target_link_libraries(ETC "-framework CoreFoundation")
endif()

install(TARGETS ETC LIBRARY DESTINATION lib)
