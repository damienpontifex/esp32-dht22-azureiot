cmake_minimum_required(VERSION 3.14)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
  espidflib
  GIT_REPOSITORY https://github.com/UncleRus/esp-idf-lib.git
)
FetchContent_MakeAvailable(espidflib)
set(EXTRA_COMPONENT_DIRS ${espidflib_SOURCE_DIR}/components)

FetchContent_Declare(
  espazure
  GIT_REPOSITORY https://github.com/espressif/esp-azure.git
)
FetchContent_MakeAvailable(espazure)
set(EXTRA_COMPONENT_DIRS ${EXTRA_COMPONENT_DIRS} ${espazure_SOURCE_DIR}/port)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32-dht22)
