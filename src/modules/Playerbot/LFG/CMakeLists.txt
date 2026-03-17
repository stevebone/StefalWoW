# LFG Bot Management System
# Handles automatic bot recruitment for Looking For Group system

set(LFG_SOURCES
    LFGBotManager.cpp
    LFGBotSelector.cpp
    LFGRoleDetector.cpp
    LFGGroupCoordinator.cpp
)

set(LFG_HEADERS
    LFGBotManager.h
    LFGBotSelector.h
    LFGRoleDetector.h
    LFGGroupCoordinator.h
)

# Add source files to parent scope
set(PLAYERBOT_LFG_SOURCES ${LFG_SOURCES} PARENT_SCOPE)
set(PLAYERBOT_LFG_HEADERS ${LFG_HEADERS} PARENT_SCOPE)
