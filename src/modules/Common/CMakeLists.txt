# Common Module - Shared Infrastructure for All TrinityCore Modules
#
# This module provides shared APIs and utilities that can be used by all
# other modules to ensure consistency and avoid code duplication.

add_library(modules-common STATIC)

# CollectAndAddSourceFiles automatically recurses into subdirectories
CollectAndAddSourceFiles(
  modules-common
  ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(modules-common
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src/server
    ${CMAKE_SOURCE_DIR}/src/server/shared
    ${CMAKE_SOURCE_DIR}/src/server/database
    ${CMAKE_SOURCE_DIR}/src/common
)

target_link_libraries(modules-common
  PUBLIC
    trinity-core-interface
  PRIVATE
    shared
    common
)

set_target_properties(modules-common
  PROPERTIES
    FOLDER "src/modules"
)

# Install headers for other modules to use
install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
  DESTINATION include/modules/Common
  FILES_MATCHING PATTERN "*.h"
)
