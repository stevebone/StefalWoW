# Test Module - Demonstrates Shared Module API Access
#
# This module tests that the shared Common module infrastructure
# can be accessed by any module in the system.

add_library(test-module STATIC)

CollectAndAddSourceFiles(
  test-module
  ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(test-module
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/modules/Common
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src/server
    ${CMAKE_SOURCE_DIR}/src/server/shared
    ${CMAKE_SOURCE_DIR}/src/server/database
    ${CMAKE_SOURCE_DIR}/src/common
)

target_link_libraries(test-module
  PUBLIC
    trinity-core-interface
    modules-common
  PRIVATE
    shared
    common
)

set_target_properties(test-module
  PROPERTIES
    FOLDER "src/modules"
)
