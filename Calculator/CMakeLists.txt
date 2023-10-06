set(CALCULATOR_INCLUDEDIR include)
set(CALCULATOR_SRCDIR src)

set(CMAKE_CXX_STANDARD 17)

include(FindPkgConfig)
pkg_check_modules(YAMLCPP yaml-cpp>=0.7.0)

# Add source files
file(GLOB_RECURSE SOURCE_FILES 
	${CALCULATOR_SRCDIR}/*.c
	${CALCULATOR_SRCDIR}/*.cpp)
	
# Add header files
file(GLOB_RECURSE HEADER_FILES 
	${CALCULATOR_INCLUDEDIR}/*.h
	${CALCULATOR_INCLUDEDIR}/*.hpp)

add_executable(Calculator ${HEADER_FILES} ${SOURCE_FILES})

target_link_directories(
  Calculator 
  PRIVATE
  ${LIBETC_LIBDIR}
  ${YAMLCPP_LIBRARY_DIRS})

  target_link_libraries(Calculator PRIVATE ETC ${YAMLCPP_LIBRARIES})

target_include_directories(
  Calculator
  PRIVATE 
  ../LibETC/include
  ${CALCULATOR_INCLUDEDIR}
  ${YAMLCPP_INCLUDE_DIRS})

install(TARGETS Calculator)