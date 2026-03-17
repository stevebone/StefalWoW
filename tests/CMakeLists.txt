# This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(tests)

CollectAndAddSourceFiles(
  tests
  ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(tests
  PRIVATE
    trinity-core-interface
    game
    Catch2::Catch2)

# Link playerbot module if enabled (for Phase5 tests)
if(BUILD_PLAYERBOT)
  target_link_libraries(tests PRIVATE playerbot)
  target_compile_definitions(tests PRIVATE BUILD_PLAYERBOT=1)
endif()

target_include_directories(tests
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src/modules/Playerbot
    ${CMAKE_SOURCE_DIR}/src/modules/Playerbot/AI
    ${CMAKE_SOURCE_DIR}/src/modules/Playerbot/AI/Decision)

catch_discover_tests(tests)

set_target_properties(tests
    PROPERTIES
      FOLDER
        "tests")
