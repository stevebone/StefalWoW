/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotDatabaseStatements.h"

char const* const PlayerbotDB::statements[MAX_PLAYERBOTDATABASE_STATEMENTS] =
{
    // Activity Patterns (PBDB_SEL_PATTERN_*)
    /* PBDB_SEL_PATTERN_BY_NAME */
    "SELECT pattern_name, display_name, description, active_hours, active_days, "
    "login_probability, min_session_duration, max_session_duration, average_sessions_per_day, "
    "prefer_peak_hours, weekend_multiplier, peak_hour_bonus, jitter_minutes, is_system_pattern "
    "FROM playerbot_activity_patterns WHERE pattern_name = ?",

    /* PBDB_SEL_ALL_PATTERNS */
    "SELECT pattern_name, display_name, description, active_hours, active_days, "
    "login_probability, min_session_duration, max_session_duration, average_sessions_per_day, "
    "prefer_peak_hours, weekend_multiplier, peak_hour_bonus, jitter_minutes, is_system_pattern "
    "FROM playerbot_activity_patterns ORDER BY pattern_name",

    /* PBDB_SEL_SYSTEM_PATTERNS */
    "SELECT pattern_name, display_name, description FROM playerbot_activity_patterns "
    "WHERE is_system_pattern = 1 ORDER BY pattern_name",

    /* PBDB_INS_ACTIVITY_PATTERN */
    "INSERT INTO playerbot_activity_patterns "
    "(pattern_name, display_name, description, active_hours, active_days, login_probability, "
    "min_session_duration, max_session_duration, average_sessions_per_day, prefer_peak_hours, "
    "weekend_multiplier, peak_hour_bonus, jitter_minutes, is_system_pattern) "
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",

    /* PBDB_UPD_ACTIVITY_PATTERN */
    "UPDATE playerbot_activity_patterns SET "
    "display_name = ?, description = ?, active_hours = ?, active_days = ?, login_probability = ?, "
    "min_session_duration = ?, max_session_duration = ?, average_sessions_per_day = ?, "
    "prefer_peak_hours = ?, weekend_multiplier = ?, peak_hour_bonus = ?, jitter_minutes = ? "
    "WHERE pattern_name = ?",

    /* PBDB_DEL_ACTIVITY_PATTERN */
    "DELETE FROM playerbot_activity_patterns WHERE pattern_name = ? AND is_system_pattern = 0",

    // Bot Schedules (PBDB_SCHEDULE_*)
    /* PBDB_SEL_SCHEDULE_BY_GUID */
    "SELECT bot_guid, pattern_name, next_login, next_logout, last_activity, last_calculation, "
    "total_sessions, total_playtime, current_session_start, is_scheduled, is_active, "
    "schedule_priority, consecutive_failures, last_failure_reason, next_retry "
    "FROM playerbot_schedules WHERE bot_guid = ?",

    /* PBDB_SEL_ACTIVE_SCHEDULES */
    "SELECT bot_guid, pattern_name, next_login, next_logout, last_activity, is_active, "
    "total_sessions, total_playtime, schedule_priority "
    "FROM playerbot_schedules WHERE is_scheduled = 1 ORDER BY schedule_priority DESC, next_login",

    /* PBDB_SEL_SCHEDULES_READY_LOGIN */
    "SELECT bot_guid, pattern_name, next_login, schedule_priority "
    "FROM playerbot_schedules "
    "WHERE next_login <= NOW() AND is_active = 0 AND is_scheduled = 1 "
    "AND (next_retry IS NULL OR next_retry <= NOW()) "
    "ORDER BY schedule_priority DESC, next_login LIMIT ?",

    /* PBDB_SEL_SCHEDULES_READY_LOGOUT */
    "SELECT bot_guid, pattern_name, next_logout, current_session_start "
    "FROM playerbot_schedules "
    "WHERE next_logout <= NOW() AND is_active = 1 AND is_scheduled = 1 "
    "ORDER BY next_logout LIMIT ?",

    /* PBDB_SEL_SCHEDULES_BY_PATTERN */
    "SELECT bot_guid, next_login, next_logout, is_active, total_sessions, total_playtime "
    "FROM playerbot_schedules WHERE pattern_name = ? AND is_scheduled = 1",

    /* PBDB_INS_BOT_SCHEDULE */
    "INSERT INTO playerbot_schedules "
    "(bot_guid, pattern_name, next_login, next_logout, is_scheduled, schedule_priority) "
    "VALUES (?, ?, ?, ?, ?, ?)",

    /* PBDB_UPD_BOT_SCHEDULE */
    "UPDATE playerbot_schedules SET "
    "pattern_name = ?, next_login = ?, next_logout = ?, is_scheduled = ?, "
    "schedule_priority = ?, last_calculation = NOW() "
    "WHERE bot_guid = ?",

    /* PBDB_UPD_SCHEDULE_LOGIN_TIME */
    "UPDATE playerbot_schedules SET next_login = ?, last_calculation = NOW() WHERE bot_guid = ?",

    /* PBDB_UPD_SCHEDULE_LOGOUT_TIME */
    "UPDATE playerbot_schedules SET next_logout = ?, last_calculation = NOW() WHERE bot_guid = ?",

    /* PBDB_UPD_SCHEDULE_ACTIVITY */
    "UPDATE playerbot_schedules SET is_active = ?, last_activity = NOW() WHERE bot_guid = ?",

    /* PBDB_UPD_SCHEDULE_SESSION_START */
    "UPDATE playerbot_schedules SET "
    "current_session_start = NOW(), total_sessions = total_sessions + 1, "
    "consecutive_failures = 0, last_failure_reason = NULL "
    "WHERE bot_guid = ?",

    /* PBDB_UPD_SCHEDULE_SESSION_END */
    "UPDATE playerbot_schedules SET "
    "current_session_start = NULL, total_playtime = total_playtime + ?, is_active = 0 "
    "WHERE bot_guid = ?",

    /* PBDB_UPD_SCHEDULE_FAILURE */
    "UPDATE playerbot_schedules SET "
    "consecutive_failures = ?, last_failure_reason = ?, next_retry = ? "
    "WHERE bot_guid = ?",

    /* PBDB_DEL_BOT_SCHEDULE */
    "DELETE FROM playerbot_schedules WHERE bot_guid = ?",

    // Spawn Log (PBDB_LOG_*)
    /* PBDB_INS_SPAWN_LOG */
    "INSERT INTO playerbot_spawn_log "
    "(bot_guid, account_id, event_type, map_id, zone_id, area_id, position_x, position_y, position_z, "
    "reason, initiator, pattern_name, processing_time_ms, queue_wait_time_ms, extra_data) "
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",

    /* PBDB_SEL_SPAWN_LOG_BY_GUID */
    "SELECT log_id, bot_guid, account_id, event_type, event_timestamp, map_id, zone_id, "
    "reason, initiator, pattern_name, processing_time_ms "
    "FROM playerbot_spawn_log WHERE bot_guid = ? "
    "ORDER BY event_timestamp DESC LIMIT ?",

    /* PBDB_SEL_SPAWN_LOG_BY_TYPE */
    "SELECT log_id, bot_guid, account_id, event_type, event_timestamp, zone_id, "
    "reason, processing_time_ms "
    "FROM playerbot_spawn_log "
    "WHERE event_type = ? AND event_timestamp >= ? "
    "ORDER BY event_timestamp DESC LIMIT 1000",

    /* PBDB_SEL_SPAWN_LOG_RECENT */
    "SELECT log_id, bot_guid, event_type, event_timestamp, zone_id, reason, processing_time_ms "
    "FROM playerbot_spawn_log "
    "WHERE event_timestamp >= ? "
    "ORDER BY event_timestamp DESC LIMIT ?",

    /* PBDB_SEL_SPAWN_STATS_BY_ZONE */
    "SELECT zone_id, COUNT(*) as spawn_count, AVG(processing_time_ms) as avg_processing_time "
    "FROM playerbot_spawn_log "
    "WHERE event_type = 'SPAWN_SUCCESS' AND event_timestamp >= ? "
    "GROUP BY zone_id ORDER BY spawn_count DESC",

    // Zone Populations (PBDB_ZONE_*)
    /* PBDB_SEL_ZONE_POPULATION */
    "SELECT zone_id, map_id, current_bots, target_population, max_population, "
    "min_level, max_level, spawn_weight, is_enabled, population_multiplier, "
    "last_spawn, spawn_cooldown_minutes, total_spawns_today "
    "FROM playerbot_zone_populations WHERE zone_id = ? AND map_id = ?",

    /* PBDB_SEL_ALL_ZONE_POPULATIONS */
    "SELECT zone_id, map_id, current_bots, target_population, max_population, "
    "min_level, max_level, spawn_weight, is_enabled, population_multiplier "
    "FROM playerbot_zone_populations WHERE is_enabled = 1 ORDER BY spawn_weight DESC",

    /* PBDB_SEL_SPAWNABLE_ZONES */
    "SELECT zone_id, map_id, current_bots, max_population, spawn_weight, "
    "min_level, max_level, last_spawn, spawn_cooldown_minutes "
    "FROM playerbot_zone_populations "
    "WHERE is_enabled = 1 AND current_bots < max_population "
    "AND (last_spawn IS NULL OR last_spawn <= DATE_SUB(NOW(), INTERVAL spawn_cooldown_minutes MINUTE)) "
    "ORDER BY spawn_weight DESC",

    /* PBDB_SEL_ZONES_BY_LEVEL */
    "SELECT zone_id, map_id, current_bots, max_population, spawn_weight "
    "FROM playerbot_zone_populations "
    "WHERE min_level <= ? AND max_level >= ? AND is_enabled = 1 "
    "ORDER BY spawn_weight DESC",

    /* PBDB_SEL_STARTER_ZONES */
    "SELECT zone_id, map_id, current_bots, target_population, max_population, spawn_weight "
    "FROM playerbot_zone_populations "
    "WHERE is_starter_zone = 1 AND is_enabled = 1 ORDER BY spawn_weight DESC",

    /* PBDB_SEL_ENDGAME_ZONES */
    "SELECT zone_id, map_id, current_bots, target_population, max_population, spawn_weight "
    "FROM playerbot_zone_populations "
    "WHERE is_endgame_zone = 1 AND is_enabled = 1 ORDER BY spawn_weight DESC",

    /* PBDB_INS_ZONE_POPULATION */
    "INSERT INTO playerbot_zone_populations "
    "(zone_id, map_id, target_population, max_population, min_level, max_level, "
    "spawn_weight, is_enabled, is_starter_zone, is_endgame_zone, spawn_cooldown_minutes) "
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",

    /* PBDB_UPD_ZONE_CURRENT_BOTS */
    "UPDATE playerbot_zone_populations SET current_bots = ?, last_updated = NOW() "
    "WHERE zone_id = ? AND map_id = ?",

    /* PBDB_UPD_ZONE_LAST_SPAWN */
    "UPDATE playerbot_zone_populations SET "
    "last_spawn = NOW(), total_spawns_today = total_spawns_today + 1 "
    "WHERE zone_id = ? AND map_id = ?",

    /* PBDB_UPD_ZONE_TARGET_POPULATION */
    "UPDATE playerbot_zone_populations SET target_population = ? "
    "WHERE zone_id = ? AND map_id = ?",

    /* PBDB_UPD_ZONE_SETTINGS */
    "UPDATE playerbot_zone_populations SET spawn_weight = ?, population_multiplier = ? "
    "WHERE zone_id = ? AND map_id = ?",

    // Lifecycle Events (PBDB_EVENT_*)
    /* PBDB_INS_LIFECYCLE_EVENT */
    "INSERT INTO bot_lifecycle_events "
    "(event_category, event_type, severity, bot_guid, account_id, zone_id, message, metadata) "
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?)",

    /* PBDB_SEL_RECENT_EVENTS */
    "SELECT event_id, timestamp, event_category, event_type, severity, "
    "bot_guid, account_id, zone_id, message, metadata "
    "FROM bot_lifecycle_events "
    "WHERE timestamp >= ? ORDER BY timestamp DESC LIMIT ?",

    /* PBDB_SEL_EVENTS_BY_CATEGORY */
    "SELECT event_id, timestamp, event_type, severity, bot_guid, message "
    "FROM bot_lifecycle_events "
    "WHERE event_category = ? ORDER BY timestamp DESC LIMIT ?",

    /* PBDB_SEL_EVENTS_BY_SEVERITY */
    "SELECT event_id, timestamp, event_category, event_type, "
    "bot_guid, account_id, zone_id, message, metadata "
    "FROM bot_lifecycle_events "
    "WHERE severity IN (?, ?) ORDER BY timestamp DESC LIMIT ?",

    /* PBDB_SEL_EVENTS_BY_BOT */
    "SELECT event_id, timestamp, event_category, event_type, severity, "
    "zone_id, message, metadata "
    "FROM bot_lifecycle_events "
    "WHERE bot_guid = ? ORDER BY timestamp DESC LIMIT ?",

    /* PBDB_SEL_EVENTS_BY_CORRELATION */
    "SELECT event_id, timestamp, event_category, event_type, severity, "
    "bot_guid, account_id, message "
    "FROM bot_lifecycle_events "
    "WHERE correlation_id = ? ORDER BY timestamp",

    /* PBDB_DEL_OLD_EVENTS */
    "DELETE FROM bot_lifecycle_events "
    "WHERE timestamp < ? AND severity NOT IN ('ERROR', 'CRITICAL')",

    // Bot Account Metadata (PBDB_ACCOUNT_*)
    /* PBDB_SEL_ACCOUNT_METADATA */
    "SELECT account_id, bnet_account_id, email, character_count, expansion, locale, "
    "last_ip, join_date, last_login, total_time_played, notes, created_at, updated_at "
    "FROM bot_account_metadata WHERE account_id = ?",

    /* PBDB_INS_ACCOUNT_METADATA */
    "INSERT INTO bot_account_metadata "
    "(account_id, bnet_account_id, email, character_count, expansion, locale, "
    "last_ip, join_date, total_time_played, notes) "
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",

    /* PBDB_UPD_ACCOUNT_METADATA */
    "UPDATE bot_account_metadata SET "
    "email = ?, character_count = ?, expansion = ?, locale = ?, last_ip = ?, "
    "last_login = NOW(), total_time_played = ?, notes = ? "
    "WHERE account_id = ?",

    /* PBDB_DEL_ACCOUNT_METADATA */
    "DELETE FROM bot_account_metadata WHERE account_id = ?",

    // Zone Population Management (PBDB_ZONE_POP_*)
    /* PBDB_SEL_ZONE_POPULATION_CURRENT */
    "SELECT zone_id, bot_count, player_count, total_count, max_capacity, "
    "density_score, last_updated "
    "FROM bot_zone_population WHERE zone_id = ?",

    /* PBDB_UPD_ZONE_POPULATION_STATS */
    "UPDATE bot_zone_population SET "
    "bot_count = ?, player_count = ?, total_count = ?, density_score = ?, "
    "last_updated = CURRENT_TIMESTAMP "
    "WHERE zone_id = ?",

    /* PBDB_INS_ZONE_POPULATION_ENTRY */
    "INSERT INTO bot_zone_population "
    "(zone_id, bot_count, player_count, total_count, max_capacity, density_score) "
    "VALUES (?, ?, ?, ?, ?, ?) "
    "ON DUPLICATE KEY UPDATE "
    "bot_count = VALUES(bot_count), player_count = VALUES(player_count), "
    "total_count = VALUES(total_count), density_score = VALUES(density_score), "
    "last_updated = CURRENT_TIMESTAMP",

    // Statistics and Monitoring (PBDB_STATS_*)
    /* PBDB_SEL_ACTIVE_BOT_COUNT */
    "SELECT COUNT(*) FROM playerbot_schedules WHERE is_active = 1",

    /* PBDB_SEL_SCHEDULED_BOT_COUNT */
    "SELECT COUNT(*) FROM playerbot_schedules WHERE is_scheduled = 1",

    /* PBDB_SEL_TOTAL_POPULATION */
    "SELECT SUM(current_bots) as total_bots, SUM(target_population) as target_total "
    "FROM playerbot_zone_populations WHERE is_enabled = 1",

    /* PBDB_SEL_POPULATION_BY_PATTERN */
    "SELECT pattern_name, COUNT(*) as active_count "
    "FROM playerbot_schedules WHERE is_active = 1 GROUP BY pattern_name",

    /* PBDB_SEL_AVERAGE_SESSION_TIME */
    "SELECT AVG(total_playtime / GREATEST(total_sessions, 1)) as avg_session_seconds "
    "FROM playerbot_schedules WHERE total_sessions > 0",

    /* PBDB_SEL_SPAWN_SUCCESS_RATE */
    "SELECT "
    "    (SELECT COUNT(*) FROM playerbot_spawn_log WHERE event_type = 'SPAWN_SUCCESS' AND event_timestamp >= ?) as successful_spawns, "
    "    (SELECT COUNT(*) FROM playerbot_spawn_log WHERE event_type = 'SPAWN_FAILURE' AND event_timestamp >= ?) as failed_spawns, "
    "    (SELECT COUNT(*) FROM playerbot_spawn_log WHERE event_type = 'SPAWN_REQUEST' AND event_timestamp >= ?) as total_requests",

    // Maintenance (PBDB_MAINT_*)
    /* PBDB_CLEANUP_OLD_SPAWN_LOGS */
    "DELETE FROM playerbot_spawn_log WHERE event_timestamp < ?",

    /* PBDB_CLEANUP_OLD_EVENTS */
    "DELETE FROM playerbot_lifecycle_events "
    "WHERE event_timestamp < ? AND severity NOT IN ('ERROR', 'CRITICAL')",

    /* PBDB_RESET_DAILY_COUNTERS */
    "UPDATE playerbot_zone_populations SET total_spawns_today = 0",

    /* PBDB_UPDATE_ZONE_STATISTICS */
    "UPDATE playerbot_zone_populations zp "
    "SET peak_population = GREATEST(peak_population, current_bots), "
    "    peak_timestamp = CASE WHEN current_bots > peak_population THEN NOW() ELSE peak_timestamp END "
    "WHERE current_bots > 0",

    // Views (PBDB_VIEW_*)
    /* PBDB_SEL_ACTIVE_SCHEDULES_VIEW */
    "SELECT bot_guid, pattern_name, next_login, next_logout, is_active, "
    "total_sessions, total_playtime, pattern_display_name, average_sessions_per_day "
    "FROM v_active_bot_schedules ORDER BY next_login",

    /* PBDB_SEL_ZONE_SUMMARY_VIEW */
    "SELECT zone_id, map_id, current_bots, target_population, max_population, "
    "spawn_weight, is_enabled, population_percentage, last_spawn, total_spawns_today "
    "FROM v_zone_population_summary ORDER BY population_percentage DESC",

    /* PBDB_SEL_RECENT_EVENTS_VIEW */
    "SELECT event_id, event_timestamp, event_category, event_type, severity, "
    "component, bot_guid, message, execution_time_ms "
    "FROM v_recent_lifecycle_events LIMIT 100"
};

void PlayerbotDB::LoadStatements()
{
    // This function would be called during database initialization
    // to prepare all the statements for optimal execution
}