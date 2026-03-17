/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"

enum PlayerbotDatabaseStatements : uint32
{
    // Activity Patterns (PBDB_SEL_PATTERN_*)
    PBDB_SEL_PATTERN_BY_NAME,           // SELECT * FROM playerbot_activity_patterns WHERE pattern_name = ?
    PBDB_SEL_ALL_PATTERNS,              // SELECT * FROM playerbot_activity_patterns ORDER BY pattern_name
    PBDB_SEL_SYSTEM_PATTERNS,           // SELECT * FROM playerbot_activity_patterns WHERE is_system_pattern = 1
    PBDB_INS_ACTIVITY_PATTERN,          // INSERT INTO playerbot_activity_patterns (pattern_name, display_name, ...) VALUES (?, ?, ...)
    PBDB_UPD_ACTIVITY_PATTERN,          // UPDATE playerbot_activity_patterns SET ... WHERE pattern_name = ?
    PBDB_DEL_ACTIVITY_PATTERN,          // DELETE FROM playerbot_activity_patterns WHERE pattern_name = ? AND is_system_pattern = 0

    // Bot Schedules (PBDB_SCHEDULE_*)
    PBDB_SEL_SCHEDULE_BY_GUID,          // SELECT * FROM playerbot_schedules WHERE bot_guid = ?
    PBDB_SEL_ACTIVE_SCHEDULES,          // SELECT * FROM playerbot_schedules WHERE is_scheduled = 1
    PBDB_SEL_SCHEDULES_READY_LOGIN,     // SELECT * FROM playerbot_schedules WHERE next_login <= NOW() AND is_active = 0
    PBDB_SEL_SCHEDULES_READY_LOGOUT,    // SELECT * FROM playerbot_schedules WHERE next_logout <= NOW() AND is_active = 1
    PBDB_SEL_SCHEDULES_BY_PATTERN,      // SELECT * FROM playerbot_schedules WHERE pattern_name = ?
    PBDB_INS_BOT_SCHEDULE,              // INSERT INTO playerbot_schedules (bot_guid, pattern_name, ...) VALUES (?, ?, ...)
    PBDB_UPD_BOT_SCHEDULE,              // UPDATE playerbot_schedules SET ... WHERE bot_guid = ?
    PBDB_UPD_SCHEDULE_LOGIN_TIME,       // UPDATE playerbot_schedules SET next_login = ?, last_calculation = NOW() WHERE bot_guid = ?
    PBDB_UPD_SCHEDULE_LOGOUT_TIME,      // UPDATE playerbot_schedules SET next_logout = ?, last_calculation = NOW() WHERE bot_guid = ?
    PBDB_UPD_SCHEDULE_ACTIVITY,         // UPDATE playerbot_schedules SET is_active = ?, last_activity = NOW() WHERE bot_guid = ?
    PBDB_UPD_SCHEDULE_SESSION_START,    // UPDATE playerbot_schedules SET current_session_start = NOW(), total_sessions = total_sessions + 1 WHERE bot_guid = ?
    PBDB_UPD_SCHEDULE_SESSION_END,      // UPDATE playerbot_schedules SET current_session_start = NULL, total_playtime = total_playtime + ? WHERE bot_guid = ?
    PBDB_UPD_SCHEDULE_FAILURE,          // UPDATE playerbot_schedules SET consecutive_failures = ?, last_failure_reason = ?, next_retry = ? WHERE bot_guid = ?
    PBDB_DEL_BOT_SCHEDULE,              // DELETE FROM playerbot_schedules WHERE bot_guid = ?

    // Spawn Log (PBDB_LOG_*)
    PBDB_INS_SPAWN_LOG,                 // INSERT INTO playerbot_spawn_log (bot_guid, account_id, event_type, ...) VALUES (?, ?, ?, ...)
    PBDB_SEL_SPAWN_LOG_BY_GUID,         // SELECT * FROM playerbot_spawn_log WHERE bot_guid = ? ORDER BY event_timestamp DESC LIMIT ?
    PBDB_SEL_SPAWN_LOG_BY_TYPE,         // SELECT * FROM playerbot_spawn_log WHERE event_type = ? AND event_timestamp >= ? ORDER BY event_timestamp DESC
    PBDB_SEL_SPAWN_LOG_RECENT,          // SELECT * FROM playerbot_spawn_log WHERE event_timestamp >= ? ORDER BY event_timestamp DESC LIMIT ?
    PBDB_SEL_SPAWN_STATS_BY_ZONE,       // SELECT zone_id, COUNT(*) as spawn_count FROM playerbot_spawn_log WHERE event_type = 'SPAWN_SUCCESS' AND event_timestamp >= ? GROUP BY zone_id

    // Zone Populations (PBDB_ZONE_*)
    PBDB_SEL_ZONE_POPULATION,           // SELECT * FROM playerbot_zone_populations WHERE zone_id = ? AND map_id = ?
    PBDB_SEL_ALL_ZONE_POPULATIONS,      // SELECT * FROM playerbot_zone_populations WHERE is_enabled = 1 ORDER BY spawn_weight DESC
    PBDB_SEL_SPAWNABLE_ZONES,           // SELECT * FROM playerbot_zone_populations WHERE is_enabled = 1 AND current_bots < max_population
    PBDB_SEL_ZONES_BY_LEVEL,            // SELECT * FROM playerbot_zone_populations WHERE min_level <= ? AND max_level >= ? AND is_enabled = 1
    PBDB_SEL_STARTER_ZONES,             // SELECT * FROM playerbot_zone_populations WHERE is_starter_zone = 1 AND is_enabled = 1
    PBDB_SEL_ENDGAME_ZONES,             // SELECT * FROM playerbot_zone_populations WHERE is_endgame_zone = 1 AND is_enabled = 1
    PBDB_INS_ZONE_POPULATION,           // INSERT INTO playerbot_zone_populations (zone_id, map_id, target_population, ...) VALUES (?, ?, ?, ...)
    PBDB_UPD_ZONE_CURRENT_BOTS,         // UPDATE playerbot_zone_populations SET current_bots = ?, last_updated = NOW() WHERE zone_id = ? AND map_id = ?
    PBDB_UPD_ZONE_LAST_SPAWN,           // UPDATE playerbot_zone_populations SET last_spawn = NOW(), total_spawns_today = total_spawns_today + 1 WHERE zone_id = ? AND map_id = ?
    PBDB_UPD_ZONE_TARGET_POPULATION,    // UPDATE playerbot_zone_populations SET target_population = ? WHERE zone_id = ? AND map_id = ?
    PBDB_UPD_ZONE_SETTINGS,             // UPDATE playerbot_zone_populations SET spawn_weight = ?, population_multiplier = ? WHERE zone_id = ? AND map_id = ?

    // Lifecycle Events (PBDB_EVENT_*)
    PBDB_INS_LIFECYCLE_EVENT,           // INSERT INTO bot_lifecycle_events (event_category, event_type, severity, bot_guid, account_id, zone_id, message, metadata) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    PBDB_SEL_RECENT_EVENTS,             // SELECT * FROM bot_lifecycle_events WHERE timestamp >= ? ORDER BY timestamp DESC LIMIT ?
    PBDB_SEL_EVENTS_BY_CATEGORY,        // SELECT * FROM bot_lifecycle_events WHERE event_category = ? ORDER BY timestamp DESC LIMIT ?
    PBDB_SEL_EVENTS_BY_SEVERITY,        // SELECT * FROM bot_lifecycle_events WHERE severity IN (?, ?) ORDER BY timestamp DESC LIMIT ?
    PBDB_SEL_EVENTS_BY_BOT,             // SELECT * FROM bot_lifecycle_events WHERE bot_guid = ? ORDER BY timestamp DESC LIMIT ?
    PBDB_SEL_EVENTS_BY_CORRELATION,     // SELECT * FROM bot_lifecycle_events WHERE correlation_id = ? ORDER BY timestamp
    PBDB_DEL_OLD_EVENTS,                // DELETE FROM bot_lifecycle_events WHERE timestamp < ? AND severity NOT IN ('ERROR', 'CRITICAL')

    // Bot Account Metadata (PBDB_ACCOUNT_*)
    PBDB_SEL_ACCOUNT_METADATA,          // SELECT * FROM bot_account_metadata WHERE account_id = ?
    PBDB_INS_ACCOUNT_METADATA,          // INSERT INTO bot_account_metadata (account_id, bnet_account_id, email, character_count, expansion, locale, last_ip, join_date, total_time_played, notes) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    PBDB_UPD_ACCOUNT_METADATA,          // UPDATE bot_account_metadata SET email = ?, character_count = ?, expansion = ?, locale = ?, last_ip = ?, last_login = NOW(), total_time_played = ?, notes = ? WHERE account_id = ?
    PBDB_DEL_ACCOUNT_METADATA,          // DELETE FROM bot_account_metadata WHERE account_id = ?

    // Zone Population Management (PBDB_ZONE_POP_*)
    PBDB_SEL_ZONE_POPULATION_CURRENT,   // SELECT * FROM bot_zone_population WHERE zone_id = ?
    PBDB_UPD_ZONE_POPULATION_STATS,     // UPDATE bot_zone_population SET bot_count = ?, player_count = ?, total_count = ?, density_score = ? WHERE zone_id = ?
    PBDB_INS_ZONE_POPULATION_ENTRY,     // INSERT INTO bot_zone_population (zone_id, bot_count, player_count, total_count, max_capacity, density_score) VALUES (?, ?, ?, ?, ?, ?) ON DUPLICATE KEY UPDATE bot_count = VALUES(bot_count), player_count = VALUES(player_count), total_count = VALUES(total_count), density_score = VALUES(density_score)

    // Statistics and Monitoring (PBDB_STATS_*)
    PBDB_SEL_ACTIVE_BOT_COUNT,          // SELECT COUNT(*) FROM playerbot_schedules WHERE is_active = 1
    PBDB_SEL_SCHEDULED_BOT_COUNT,       // SELECT COUNT(*) FROM playerbot_schedules WHERE is_scheduled = 1
    PBDB_SEL_TOTAL_POPULATION,          // SELECT SUM(current_bots) FROM playerbot_zone_populations
    PBDB_SEL_POPULATION_BY_PATTERN,     // SELECT pattern_name, COUNT(*) FROM playerbot_schedules WHERE is_active = 1 GROUP BY pattern_name
    PBDB_SEL_AVERAGE_SESSION_TIME,      // SELECT AVG(total_playtime / GREATEST(total_sessions, 1)) FROM playerbot_schedules WHERE total_sessions > 0
    PBDB_SEL_SPAWN_SUCCESS_RATE,        // Complex query for spawn success rate

    // Maintenance (PBDB_MAINT_*)
    PBDB_CLEANUP_OLD_SPAWN_LOGS,        // DELETE FROM playerbot_spawn_log WHERE event_timestamp < ?
    PBDB_CLEANUP_OLD_EVENTS,            // DELETE FROM playerbot_lifecycle_events WHERE event_timestamp < ? AND severity NOT IN ('ERROR', 'CRITICAL')
    PBDB_RESET_DAILY_COUNTERS,          // UPDATE playerbot_zone_populations SET total_spawns_today = 0
    PBDB_UPDATE_ZONE_STATISTICS,        // Complex update for zone statistics

    // Views (PBDB_VIEW_*)
    PBDB_SEL_ACTIVE_SCHEDULES_VIEW,     // SELECT * FROM v_active_bot_schedules
    PBDB_SEL_ZONE_SUMMARY_VIEW,         // SELECT * FROM v_zone_population_summary
    PBDB_SEL_RECENT_EVENTS_VIEW,        // SELECT * FROM v_recent_lifecycle_events

    // Bot State Persistence (PBDB_STATE_*)
    PBDB_SEL_BOT_STATE,                 // SELECT * FROM playerbot_state WHERE bot_guid = ?
    PBDB_INS_BOT_STATE,                 // INSERT INTO playerbot_state (bot_guid, position_x, position_y, position_z, map_id, zone_id, gold_copper, ...) VALUES (?, ?, ?, ?, ?, ?, ?, ...)
    PBDB_UPD_BOT_POSITION,              // UPDATE playerbot_state SET position_x = ?, position_y = ?, position_z = ?, map_id = ?, zone_id = ?, orientation = ?, last_updated = NOW() WHERE bot_guid = ?
    PBDB_UPD_BOT_GOLD,                  // UPDATE playerbot_state SET gold_copper = ?, last_updated = NOW() WHERE bot_guid = ?
    PBDB_UPD_BOT_FULL_STATE,            // UPDATE playerbot_state SET position_x = ?, position_y = ?, position_z = ?, map_id = ?, zone_id = ?, orientation = ?, gold_copper = ?, health = ?, mana = ?, last_updated = NOW() WHERE bot_guid = ?
    PBDB_DEL_BOT_STATE,                 // DELETE FROM playerbot_state WHERE bot_guid = ?

    // Bot Inventory Persistence (PBDB_INV_*)
    PBDB_SEL_BOT_INVENTORY,             // SELECT * FROM playerbot_inventory WHERE bot_guid = ?
    PBDB_SEL_BOT_INVENTORY_SLOT,        // SELECT * FROM playerbot_inventory WHERE bot_guid = ? AND bag = ? AND slot = ?
    PBDB_INS_INVENTORY_ITEM,            // INSERT INTO playerbot_inventory (bot_guid, bag, slot, item_id, item_guid, stack_count, ...) VALUES (?, ?, ?, ?, ?, ?, ...)
    PBDB_UPD_INVENTORY_ITEM,            // UPDATE playerbot_inventory SET item_id = ?, stack_count = ?, enchantments = ?, durability = ?, last_updated = NOW() WHERE bot_guid = ? AND bag = ? AND slot = ?
    PBDB_DEL_INVENTORY_ITEM,            // DELETE FROM playerbot_inventory WHERE bot_guid = ? AND bag = ? AND slot = ?
    PBDB_DEL_BOT_INVENTORY,             // DELETE FROM playerbot_inventory WHERE bot_guid = ?
    PBDB_SEL_INVENTORY_SUMMARY,         // SELECT COUNT(*) as item_count, SUM(stack_count) as total_items FROM playerbot_inventory WHERE bot_guid = ?

    // Bot Equipment Persistence (PBDB_EQUIP_*)
    PBDB_SEL_BOT_EQUIPMENT,             // SELECT * FROM playerbot_equipment WHERE bot_guid = ?
    PBDB_SEL_EQUIPMENT_SLOT,            // SELECT * FROM playerbot_equipment WHERE bot_guid = ? AND slot = ?
    PBDB_INS_EQUIPMENT_ITEM,            // INSERT INTO playerbot_equipment (bot_guid, slot, item_id, item_guid, enchantments, ...) VALUES (?, ?, ?, ?, ?, ...)
    PBDB_UPD_EQUIPMENT_ITEM,            // UPDATE playerbot_equipment SET item_id = ?, enchantments = ?, gems = ?, durability = ?, last_updated = NOW() WHERE bot_guid = ? AND slot = ?
    PBDB_DEL_EQUIPMENT_ITEM,            // DELETE FROM playerbot_equipment WHERE bot_guid = ? AND slot = ?
    PBDB_DEL_BOT_EQUIPMENT,             // DELETE FROM playerbot_equipment WHERE bot_guid = ?
    PBDB_SEL_EQUIPMENT_SUMMARY,         // SELECT slot, item_id, durability FROM playerbot_equipment WHERE bot_guid = ? ORDER BY slot

    MAX_PLAYERBOTDATABASE_STATEMENTS
};

namespace PlayerbotDB
{
    void LoadStatements();
    extern char const* const statements[MAX_PLAYERBOTDATABASE_STATEMENTS];
}