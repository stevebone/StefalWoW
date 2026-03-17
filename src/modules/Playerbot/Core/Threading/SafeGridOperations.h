/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Safe Grid Operations - Thread-safe wrappers for grid operations
 *
 * Problem: Grid operations (GetCreatureListWithEntryInGrid, etc.) are NOT thread-safe
 * when called from worker threads. The Map data can change during iteration,
 * causing access violations (SEH exceptions, not C++ exceptions).
 *
 * Solution: Use Windows SEH to catch access violations and return empty results
 * instead of crashing the server. This is a workaround, not a fix for the
 * underlying thread-safety issue.
 */

#ifndef PLAYERBOT_SAFE_GRID_OPERATIONS_H
#define PLAYERBOT_SAFE_GRID_OPERATIONS_H

#include "Define.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "Log.h"
#include <list>

#ifdef _WIN32
#include <windows.h>
#include <excpt.h>
#endif

namespace Playerbot
{

/**
 * @brief Safe wrapper for grid operations that catches access violations
 *
 * On Windows, grid operations from worker threads can cause ACCESS_VIOLATION
 * when the Map is modified during iteration. This wrapper uses SEH to catch
 * these exceptions and return false/empty instead of crashing.
 */
class SafeGridOperations
{
public:
    /**
     * @brief Safely get creatures in grid with SEH protection
     * @param bot The player/bot to search around
     * @param result Output list of creatures found
     * @param entry Creature entry to search for (0 = all creatures)
     * @param radius Search radius in yards
     * @return true if operation completed safely, false if an exception occurred
     */
    static bool GetCreatureListSafe(Player* bot, std::list<Creature*>& result, uint32 entry, float radius)
    {
        if (!bot)
            return false;

        // Pre-flight checks
        if (!bot->IsInWorld())
        {
            TC_LOG_TRACE("playerbot.grid", "SafeGridOperations: Bot {} not in world", bot->GetName());
            return false;
        }

        Map* map = bot->FindMap();
        if (!map)
        {
            TC_LOG_TRACE("playerbot.grid", "SafeGridOperations: Bot {} has no valid map", bot->GetName());
            return false;
        }

        // Validate position
        float x = bot->GetPositionX();
        float y = bot->GetPositionY();
        if (std::isnan(x) || std::isnan(y) || std::isinf(x) || std::isinf(y))
        {
            TC_LOG_ERROR("playerbot.grid", "SafeGridOperations: Bot {} has invalid position", bot->GetName());
            return false;
        }

#ifdef _WIN32
        // Use SEH on Windows to catch access violations
        __try
        {
            bot->GetCreatureListWithEntryInGrid(result, entry, radius);
            return true;
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            TC_LOG_ERROR("playerbot.grid", "SafeGridOperations: ACCESS_VIOLATION in GetCreatureListWithEntryInGrid for bot {} (entry={}, radius={})",
                         bot->GetName(), entry, radius);
            result.clear();
            return false;
        }
#else
        // On non-Windows, just do the operation with try-catch (won't catch SIGSEGV)
        try
        {
            bot->GetCreatureListWithEntryInGrid(result, entry, radius);
            return true;
        }
        catch (...)
        {
            TC_LOG_ERROR("playerbot.grid", "SafeGridOperations: Exception in GetCreatureListWithEntryInGrid for bot {}", bot->GetName());
            result.clear();
            return false;
        }
#endif
    }

    /**
     * @brief Safely get game objects in grid with SEH protection
     */
    static bool GetGameObjectListSafe(Player* bot, std::list<GameObject*>& result, uint32 entry, float radius)
    {
        if (!bot)
            return false;

        // Pre-flight checks
        if (!bot->IsInWorld())
        {
            TC_LOG_TRACE("playerbot.grid", "SafeGridOperations: Bot {} not in world", bot->GetName());
            return false;
        }

        Map* map = bot->FindMap();
        if (!map)
        {
            TC_LOG_TRACE("playerbot.grid", "SafeGridOperations: Bot {} has no valid map", bot->GetName());
            return false;
        }

        // Validate position
        float x = bot->GetPositionX();
        float y = bot->GetPositionY();
        if (std::isnan(x) || std::isnan(y) || std::isinf(x) || std::isinf(y))
        {
            TC_LOG_ERROR("playerbot.grid", "SafeGridOperations: Bot {} has invalid position", bot->GetName());
            return false;
        }

#ifdef _WIN32
        // Use SEH on Windows to catch access violations
        __try
        {
            bot->GetGameObjectListWithEntryInGrid(result, entry, radius);
            return true;
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            TC_LOG_ERROR("playerbot.grid", "SafeGridOperations: ACCESS_VIOLATION in GetGameObjectListWithEntryInGrid for bot {} (entry={}, radius={})",
                         bot->GetName(), entry, radius);
            result.clear();
            return false;
        }
#else
        // On non-Windows, just do the operation with try-catch (won't catch SIGSEGV)
        try
        {
            bot->GetGameObjectListWithEntryInGrid(result, entry, radius);
            return true;
        }
        catch (...)
        {
            TC_LOG_ERROR("playerbot.grid", "SafeGridOperations: Exception in GetGameObjectListWithEntryInGrid for bot {}", bot->GetName());
            result.clear();
            return false;
        }
#endif
    }

    /**
     * @brief Safely get creatures in grid around a creature (for add detection, etc.)
     */
    static bool GetCreatureListFromCreatureSafe(Creature* creature, std::list<Creature*>& result, uint32 entry, float radius)
    {
        if (!creature)
            return false;

        // Pre-flight checks
        if (!creature->IsInWorld())
        {
            return false;
        }

        Map* map = creature->FindMap();
        if (!map)
        {
            return false;
        }

#ifdef _WIN32
        // Use SEH on Windows to catch access violations
        __try
        {
            creature->GetCreatureListWithEntryInGrid(result, entry, radius);
            return true;
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            TC_LOG_ERROR("playerbot.grid", "SafeGridOperations: ACCESS_VIOLATION in GetCreatureListWithEntryInGrid for creature entry {}",
                         creature->GetEntry());
            result.clear();
            return false;
        }
#else
        try
        {
            creature->GetCreatureListWithEntryInGrid(result, entry, radius);
            return true;
        }
        catch (...)
        {
            result.clear();
            return false;
        }
#endif
    }

    /**
     * @brief Safely get players in grid with SEH protection
     */
    static bool GetPlayerListSafe(Player* bot, std::list<Player*>& result, float radius)
    {
        if (!bot)
            return false;

        // Pre-flight checks
        if (!bot->IsInWorld())
        {
            return false;
        }

        Map* map = bot->FindMap();
        if (!map)
        {
            return false;
        }

#ifdef _WIN32
        __try
        {
            bot->GetPlayerListInGrid(result, radius);
            return true;
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            TC_LOG_ERROR("playerbot.grid", "SafeGridOperations: ACCESS_VIOLATION in GetPlayerListInGrid for bot {}",
                         bot->GetName());
            result.clear();
            return false;
        }
#else
        try
        {
            bot->GetPlayerListInGrid(result, radius);
            return true;
        }
        catch (...)
        {
            result.clear();
            return false;
        }
#endif
    }
};

} // namespace Playerbot

#endif // PLAYERBOT_SAFE_GRID_OPERATIONS_H
