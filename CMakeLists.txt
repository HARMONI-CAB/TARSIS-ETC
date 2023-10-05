cmake_minimum_required(VERSION 3.20.0)

set(TARSIS_ETC_VERSION 0.1)

# Define the project
project(
  TarsisETC
  VERSION ${TARSIS_ETC_VERSION}
  LANGUAGES CXX)

# The library code is in src
add_subdirectory(LibETC)
add_subdirectory(Calculator)
add_subdirectory(CalGUI)
