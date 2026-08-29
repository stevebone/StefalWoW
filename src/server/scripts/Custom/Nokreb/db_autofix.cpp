// ============================================================================
// db_autofix.cpp
//
// Runtime DB cleanup module for StefalWoW (TrinityCore fork, 12.0.7).
//
// WHAT IT DOES
// ============
// On server startup, after the world is fully initialized, this module
// scans multiple DB tables for the same validation errors that ObjectMgr
// logs as warnings, and generates ready-to-run SQL patches to fix them.
//
// FIX GROUPS
// ==========
//   1. creature.spawnDifficulties      — unsupported difficulty IDs per map
//   2. gameobject.spawnDifficulties    — same check for gameobject spawns
//   3. creature_template               — faction, unit_class, dmgschool,
//        speed_walk/run, type, family, VehicleId, MovementType,
//        RequiredExpansion, flags_extra, unit_flags/2/3, VignetteID, KillCredit
//   4. creature (spawns)               — MovementType, wander_distance,
//        phaseUseFlags, phaseId, phaseGroup, terrainSwapMap,
//        unit_flags/2/3, curHealthPct
//   5. gameobject (spawns)             — phaseUseFlags, phaseId, phaseGroup,
//        terrainSwapMap, state
//   6. creature_template_difficulty    — HealthScalingExpansion, GoldMin>GoldMax
//   7. creature_addon                  — orphaned records (GUID not in creature)
//   8. gameobject_addon                — orphaned records (GUID not in gameobject)
//   9. linked_respawn                  — orphaned links (guid or linkedGuid missing)
//  10. creature_template_difficulty    — orphaned (Entry not in creature_template)
//  11. creature_model_info             — orphaned (DisplayID not in CreatureDisplayInfo.db2)
//  12. creature_template_model         — non-existent CreatureDisplayID
//  13. creature_equip_template         — unknown item, bad appearance, not equipable
//  14. npc_text                        — probability without BroadcastTextID, bad BroadcastTextID
//  15. gameobject_template             — data field validations (PageText, Spell, consumable, HealthRec)
//  16. creature_template               — npcflag GOSSIP but no gossip menu assigned
//  17. creature_template_spell          — non-existing spell references
//
// OUTPUT
// ======
// Two files per day in the output directory:
//   DD_MM_YYYY.sql   — cumulative SQL patch (appended on each run)
//   DD_MM_YYYY_summary.txt — human-readable error summary grouped by category
//
// USAGE
// =====
// In worldserver.conf:
//
//     DBAutoFix.Enable = 1
//     DBAutoFix.Apply = 0          # 1 = also apply to DB immediately
//     DBAutoFix.OutputDir = "autofix"
// ============================================================================

#include "AccountMgr.h"
#include "Config.h"
#include "Creature.h"
#include "CreatureData.h"       // CREATURE_FLAG_EXTRA_DB_ALLOWED, CreatureTemplate
#include "DatabaseEnv.h"
#include "DB2Stores.h"          // sMapDifficultyStore, sMapStore, sFactionTemplateStore, etc.
#include "DB2Structure.h"       // MapDifficultyEntry, MapEntry, etc.
#include "DBCEnums.h"           // PHASE_USE_FLAGS_ALL, PHASE_USE_FLAGS_INVERSE
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"      // CLASSMASK_ALL_CREATURES, MAX_SPELL_SCHOOL, etc.
#include "SpellMgr.h"           // sSpellMgr
#include "TransmogMgr.h"        // TransmogMgr::GetItemModifiedAppearance
#include "ItemTemplate.h"       // InventoryTypesEquipable
#include "UnitDefines.h"        // UNIT_FLAG_ALLOWED, UNIT_FLAG2_ALLOWED, etc.
#include "World.h"
#include "MovementDefines.h"    // MAX_DB_MOTION_TYPE

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#if defined(__GNUC__)
#include <sys/stat.h>
#include <sys/types.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#endif


// ----------------------------------------------------------------------------
// Fix data structure
// ----------------------------------------------------------------------------

struct Fix
{
    std::string group;     // category, e.g. "creature_template", "creature"
    std::string sql;       // full SQL statement (UPDATE or DELETE)
    std::string comment;   // human-readable description
};


// ----------------------------------------------------------------------------
// Helper function declarations
// ----------------------------------------------------------------------------

static bool EnsureDirectory(std::string const& path);
static std::string FormatTimestamp();
static std::vector<uint32> ParseDifficulties(std::string const& str);
static std::string JoinDifficulties(std::vector<uint32> const& vals);


// ----------------------------------------------------------------------------
// Checker functions — one per fix group
// ----------------------------------------------------------------------------

// Group 1+2: spawnDifficulties for creature and gameobject
static void CheckSpawnDifficulties(std::vector<Fix>& fixes,
    std::unordered_map<uint32, std::set<uint32>> const& validMapDifficulties);

// Group 3: creature_template field validations
static void CheckCreatureTemplates(std::vector<Fix>& fixes);

// Group 4: creature (spawn) field validations
static void CheckCreatureSpawns(std::vector<Fix>& fixes);

// Group 5: gameobject (spawn) field validations
static void CheckGameobjectSpawns(std::vector<Fix>& fixes);

// Group 6: creature_template_difficulty validations
static void CheckCreatureTemplateDifficulty(std::vector<Fix>& fixes);

// Group 7: orphaned creature_addon records
static void CheckOrphanedCreatureAddon(std::vector<Fix>& fixes);

// Group 8: orphaned gameobject_addon records
static void CheckOrphanedGameobjectAddon(std::vector<Fix>& fixes);

// Group 9: orphaned linked_respawn records
static void CheckOrphanedLinkedRespawn(std::vector<Fix>& fixes);

// Group 10: creature_template_difficulty orphaned (Entry not in creature_template)
static void CheckOrphanedCreatureTemplateDifficulty(std::vector<Fix>& fixes);

// Group 11: creature_model_info orphaned (DisplayID not in CreatureDisplayInfo.db2)
static void CheckOrphanedCreatureModelInfo(std::vector<Fix>& fixes);

// Group 12: creature_template_model — non-existent CreatureDisplayID
static void CheckCreatureTemplateModelBadDisplay(std::vector<Fix>& fixes);

// Group 13: creature_equip_template — unknown item, bad appearance, not equipable
static void CheckCreatureEquipTemplate(std::vector<Fix>& fixes);

// Group 14: npc_text — probability without BroadcastTextID, or bad BroadcastTextID
static void CheckNpcText(std::vector<Fix>& fixes);

// Group 15: gameobject_template — data field validations (PageText, Spell, consumable, etc.)
static void CheckGameobjectTemplateData(std::vector<Fix>& fixes);

// Group 16: creature_template — npcflag GOSSIP but no gossip menu assigned
static void CheckCreatureTemplateGossipFlag(std::vector<Fix>& fixes);

// Group 17: creature_template_spell — non-existing spell references
static void CheckCreatureTemplateSpell(std::vector<Fix>& fixes);


// ----------------------------------------------------------------------------
// Main module script
// ----------------------------------------------------------------------------

class db_autofix_worldscript : public WorldScript
{
public:
    db_autofix_worldscript() : WorldScript("db_autofix_worldscript") {}

    void OnStartup() override;
};

void db_autofix_worldscript::OnStartup()
{
    bool enabled = sConfigMgr->GetBoolDefault("DBAutoFix.Enable", false);
    if (!enabled)
    {
        TC_LOG_INFO("sql.sql", "[DBAutoFix] Module disabled (DBAutoFix.Enable = 0), skipping.");
        return;
    }

    bool applyToDb = sConfigMgr->GetBoolDefault("DBAutoFix.Apply", false);
    std::string outputDir = sConfigMgr->GetStringDefault("DBAutoFix.OutputDir", "autofix");

    TC_LOG_INFO("sql.sql", "[DBAutoFix] Module enabled. Mode: {}",
        applyToDb ? "APPLY (file + DB)" : "DRY RUN (file only)");
    TC_LOG_INFO("sql.sql", "[DBAutoFix] Output dir: {}", outputDir);

    // Build authoritative map -> valid difficulty set (used by multiple checkers)
    std::unordered_map<uint32, std::set<uint32>> validMapDifficulties;
    for (MapDifficultyEntry const* entry : sMapDifficultyStore)
        validMapDifficulties[entry->MapID].insert(entry->DifficultyID);

    TC_LOG_INFO("sql.sql", "[DBAutoFix] Loaded {} maps from sMapDifficultyStore.",
        uint32(validMapDifficulties.size()));

    // Run all checkers
    std::vector<Fix> fixes;

    CheckSpawnDifficulties(fixes, validMapDifficulties);
    CheckCreatureTemplates(fixes);
    CheckCreatureSpawns(fixes);
    CheckGameobjectSpawns(fixes);
    CheckCreatureTemplateDifficulty(fixes);
    CheckOrphanedCreatureAddon(fixes);
    CheckOrphanedGameobjectAddon(fixes);
    CheckOrphanedLinkedRespawn(fixes);
    CheckOrphanedCreatureTemplateDifficulty(fixes);
    CheckOrphanedCreatureModelInfo(fixes);
    CheckCreatureTemplateModelBadDisplay(fixes);
    CheckCreatureEquipTemplate(fixes);
    CheckNpcText(fixes);
    CheckGameobjectTemplateData(fixes);
    CheckCreatureTemplateGossipFlag(fixes);
    CheckCreatureTemplateSpell(fixes);

    // Count per group for reporting
    std::map<std::string, uint32> counts;
    for (Fix const& f : fixes)
        counts[f.group]++;

    TC_LOG_INFO("sql.sql", "[DBAutoFix] Total fixes found: {}", uint32(fixes.size()));
    for (auto const& [group, count] : counts)
        TC_LOG_INFO("sql.sql", "[DBAutoFix]   {}: {}", group, count);

    if (fixes.empty())
    {
        TC_LOG_INFO("sql.sql", "[DBAutoFix] Nothing to fix. Database is clean.");
        return;
    }

    // ----- Write per-group SQL and summary files -----
    if (!EnsureDirectory(outputDir))
    {
        TC_LOG_ERROR("sql.sql", "[DBAutoFix] Failed to create output directory '{}'. "
            "No files written.", outputDir);
        return;
    }

    // Sanitize group name into a safe filename: replace non-alphanumeric with '_'
    auto SanitizeName = [](std::string const& s) -> std::string
    {
        std::string out;
        out.reserve(s.size());
        for (char c : s)
        {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
                out.push_back(c);
            else
                out.push_back('_');
        }
        // Collapse consecutive underscores
        std::string collapsed;
        collapsed.reserve(out.size());
        bool prevUnderscore = false;
        for (char c : out)
        {
            if (c == '_')
            {
                if (!prevUnderscore)
                    collapsed.push_back(c);
                prevUnderscore = true;
            }
            else
            {
                collapsed.push_back(c);
                prevUnderscore = false;
            }
        }
        // Trim trailing underscore
        if (!collapsed.empty() && collapsed.back() == '_')
            collapsed.pop_back();
        return collapsed;
    };

    std::string timestamp = FormatTimestamp();
    uint32 filesWritten = 0;

    // Iterate over each group and write its own pair of files
    for (auto const& [group, count] : counts)
    {
        std::string safeName = SanitizeName(group);
        if (safeName.empty())
            safeName = "unknown";

        // --- SQL file for this group ---
        std::string sqlFilename = outputDir + "/" + safeName + ".sql";
        std::ofstream sqlOut(sqlFilename, std::ios::app);
        if (!sqlOut.is_open())
        {
            TC_LOG_ERROR("sql.sql", "[DBAutoFix] Failed to open SQL file '{}'.", sqlFilename);
            continue;
        }

        sqlOut << "-- ============================================================\n";
        sqlOut << "-- DB AutoFix — Group: " << group << "\n";
        sqlOut << "-- " << timestamp << "\n";
        sqlOut << "-- Fixes in this group: " << count << "\n";
        sqlOut << "-- Mode: " << (applyToDb ? "APPLY (file + DB)" : "DRY RUN (file only)") << "\n";
        sqlOut << "-- Generated by db_autofix module\n";
        sqlOut << "-- ============================================================\n\n";

        for (Fix const& f : fixes)
        {
            if (f.group != group)
                continue;
            sqlOut << "-- " << f.comment << "\n";
            sqlOut << f.sql << "\n";
        }
        sqlOut.close();
        filesWritten++;

        // --- Summary file for this group ---
        std::string summaryFilename = outputDir + "/" + safeName + "_summary.txt";
        std::ofstream sumOut(summaryFilename, std::ios::app);
        if (sumOut.is_open())
        {
            sumOut << "============================================================\n";
            sumOut << "DB AutoFix — Group: " << group << "\n";
            sumOut << timestamp << "\n";
            sumOut << "Mode: " << (applyToDb ? "APPLY (file + DB)" : "DRY RUN (file only)") << "\n";
            sumOut << "============================================================\n\n";
            sumOut << "Fixes in this group: " << count << "\n\n";
            sumOut << "Details:\n\n";

            for (Fix const& f : fixes)
            {
                if (f.group != group)
                    continue;
                sumOut << "  " << f.comment << "\n";
            }
            sumOut.close();
            filesWritten++;
        }

        TC_LOG_INFO("sql.sql", "[DBAutoFix] Group '{}': {} fixes -> {} (+ {})",
            group, count, sqlFilename, summaryFilename);
    }

    TC_LOG_INFO("sql.sql", "[DBAutoFix] Wrote {} files to: {}", filesWritten, outputDir);

    // ----- Optionally apply to DB -----
    if (applyToDb)
    {
        uint32 applied = 0;
        for (Fix const& f : fixes)
        {
            WorldDatabase.DirectExecute(f.sql.c_str());
            applied++;
        }
        TC_LOG_INFO("sql.sql", "[DBAutoFix] Applied {}/{} fixes to DB.", applied, uint32(fixes.size()));
    }
    else
    {
        TC_LOG_INFO("sql.sql", "[DBAutoFix] DRY RUN — DB not modified. "
            "To apply: review the SQL file and run it manually, OR set DBAutoFix.Apply=1 in config.");
    }
}


// ============================================================================
// Checker implementations
// ============================================================================

// ---- Group 1+2: spawnDifficulties for creature and gameobject ----

static void CheckSpawnDifficulties(std::vector<Fix>& fixes,
    std::unordered_map<uint32, std::set<uint32>> const& validMapDifficulties)
{
    // --- creature table ---
    QueryResult result = WorldDatabase.Query(
        "SELECT `guid`, `map`, `spawnDifficulties` FROM `creature`");
    if (result)
    {
        do
        {
            Field* f = result->Fetch();
            uint64 guid  = f[0].GetUInt64();
            uint32 mapId = f[1].GetUInt32();
            std::string diffStr = f[2].GetString();
            if (diffStr.empty())
                continue;

            auto it = validMapDifficulties.find(mapId);
            bool knownMap = (it != validMapDifficulties.end());

            std::vector<uint32> tokens = ParseDifficulties(diffStr);
            std::vector<uint32> kept;
            bool hasBad = false;
            for (uint32 d : tokens)
            {
                if (knownMap && it->second.count(d) > 0)
                    kept.push_back(d);
                else
                    hasBad = true;
            }
            if (!hasBad)
                continue;

            std::string newVal = JoinDifficulties(kept);

            Fix fix;
            fix.group = "creature.spawnDifficulties";
            std::ostringstream cmt;
            cmt << "creature guid=" << guid << " map=" << mapId
                << ": spawnDifficulties '" << diffStr << "' -> '";
            if (newVal.empty())
                cmt << "' (all bad)";
            else
                cmt << newVal << "'";
            if (!knownMap)
                cmt << " [map not in MapDifficulty.db2]";
            fix.comment = cmt.str();

            std::ostringstream sql;
            sql << "UPDATE `creature` SET `spawnDifficulties`='" << newVal
                << "' WHERE `guid`=" << guid << ";";
            fix.sql = sql.str();
            fixes.push_back(fix);
        } while (result->NextRow());
    }

    // --- gameobject table ---
    result = WorldDatabase.Query(
        "SELECT `guid`, `map`, `spawnDifficulties` FROM `gameobject`");
    if (result)
    {
        do
        {
            Field* f = result->Fetch();
            uint64 guid  = f[0].GetUInt64();
            uint32 mapId = f[1].GetUInt32();
            std::string diffStr = f[2].GetString();
            if (diffStr.empty())
                continue;

            auto it = validMapDifficulties.find(mapId);
            bool knownMap = (it != validMapDifficulties.end());

            std::vector<uint32> tokens = ParseDifficulties(diffStr);
            std::vector<uint32> kept;
            bool hasBad = false;
            for (uint32 d : tokens)
            {
                if (knownMap && it->second.count(d) > 0)
                    kept.push_back(d);
                else
                    hasBad = true;
            }
            if (!hasBad)
                continue;

            std::string newVal = JoinDifficulties(kept);

            Fix fix;
            fix.group = "gameobject.spawnDifficulties";
            std::ostringstream cmt;
            cmt << "gameobject guid=" << guid << " map=" << mapId
                << ": spawnDifficulties '" << diffStr << "' -> '";
            if (newVal.empty())
                cmt << "' (all bad)";
            else
                cmt << newVal << "'";
            if (!knownMap)
                cmt << " [map not in MapDifficulty.db2]";
            fix.comment = cmt.str();

            std::ostringstream sql;
            sql << "UPDATE `gameobject` SET `spawnDifficulties`='" << newVal
                << "' WHERE `guid`=" << guid << ";";
            fix.sql = sql.str();
            fixes.push_back(fix);
        } while (result->NextRow());
    }
}


// ---- Group 3: creature_template field validations ----

static void CheckCreatureTemplates(std::vector<Fix>& fixes)
{
    // Query all relevant columns from creature_template.
    // NOTE: creature_template does NOT have spells1..spells8 columns.
    //       Spells are loaded from a separate creature_template_spell table.
    // Column indices must match the SELECT order below.
    // 0:entry 1:faction 2:unit_class 3:dmgschool 4:speed_walk 5:speed_run
    // 6:type 7:family 8:VehicleId 9:MovementType
    // 10:RequiredExpansion 11:flags_extra 12:unit_flags 13:unit_flags2
    // 14:unit_flags3 15:VignetteID 16:KillCredit1 17:KillCredit2
    QueryResult result = WorldDatabase.Query(
        "SELECT `entry`, `faction`, `unit_class`, `dmgschool`, `speed_walk`, `speed_run`, "
        "`type`, `family`, `VehicleId`, `MovementType`, `RequiredExpansion`, "
        "`flags_extra`, `unit_flags`, `unit_flags2`, `unit_flags3`, `VignetteID`, "
        "`KillCredit1`, `KillCredit2` FROM `creature_template`");
    if (!result)
        return;

    do
    {
        Field* f = result->Fetch();
        uint32 entry = f[0].GetUInt32();

        // Build a per-row set of SET clauses so we emit a single UPDATE per row
        std::vector<std::string> setClauses;
        std::vector<std::string> comments;

        // --- faction ---
        uint32 faction = f[1].GetUInt16();
        if (!sFactionTemplateStore.LookupEntry(faction))
        {
            setClauses.push_back("`faction`=35");
            comments.push_back("faction " + std::to_string(faction) + "->35 (non-existing)");
        }

        // --- unit_class ---
        uint32 unit_class = f[2].GetUInt8();
        if (!unit_class || ((1 << (unit_class - 1)) & CLASSMASK_ALL_CREATURES) == 0)
        {
            setClauses.push_back("`unit_class`=1");
            comments.push_back("unit_class " + std::to_string(unit_class) + "->1 (invalid)");
        }

        // --- dmgschool ---
        int32 dmgschool = f[3].GetInt8();
        if (uint32(dmgschool) >= MAX_SPELL_SCHOOL)
        {
            setClauses.push_back("`dmgschool`=0");
            comments.push_back("dmgschool " + std::to_string(dmgschool) + "->0 (invalid)");
        }

        // --- speed_walk ---
        float speed_walk = f[4].GetFloat();
        if (speed_walk == 0.0f)
        {
            setClauses.push_back("`speed_walk`=1");
            comments.push_back("speed_walk 0->1");
        }

        // --- speed_run ---
        float speed_run = f[5].GetFloat();
        if (speed_run == 0.0f)
        {
            setClauses.push_back("`speed_run`=1.14286");
            comments.push_back("speed_run 0->1.14286");
        }

        // --- type ---
        uint32 type = f[6].GetUInt8();
        if (type && !sCreatureTypeStore.LookupEntry(type))
        {
            setClauses.push_back("`type`=7");
            comments.push_back("type " + std::to_string(type) + "->7 (invalid)");
        }

        // --- family ---
        int32 family = f[7].GetInt32();
        if (family && !sCreatureFamilyStore.LookupEntry(family))
        {
            setClauses.push_back("`family`=0");
            comments.push_back("family " + std::to_string(family) + "->0 (invalid)");
        }

        // --- VehicleId ---
        uint32 vehicleId = f[8].GetUInt32();
        if (vehicleId && !sVehicleStore.LookupEntry(vehicleId))
        {
            setClauses.push_back("`VehicleId`=0");
            comments.push_back("VehicleId " + std::to_string(vehicleId) + "->0 (non-existing)");
        }

        // --- MovementType ---
        uint32 movementType = f[9].GetUInt8();
        if (movementType >= MAX_DB_MOTION_TYPE)
        {
            setClauses.push_back("`MovementType`=0");
            comments.push_back("MovementType " + std::to_string(movementType) + "->0 (invalid)");
        }

        // --- RequiredExpansion ---
        uint32 reqExp = f[10].GetUInt32();
        if (reqExp >= MAX_EXPANSIONS)
        {
            setClauses.push_back("`RequiredExpansion`=0");
            comments.push_back("RequiredExpansion " + std::to_string(reqExp) + "->0 (invalid)");
        }

        // --- flags_extra ---
        uint32 flags_extra = f[11].GetUInt32();
        if (uint32 badFlags = (flags_extra & ~CREATURE_FLAG_EXTRA_DB_ALLOWED))
        {
            uint32 fixed_flags = flags_extra & CREATURE_FLAG_EXTRA_DB_ALLOWED;
            std::ostringstream s;
            s << "`flags_extra`=" << fixed_flags;
            setClauses.push_back(s.str());
            comments.push_back("flags_extra removed disallowed bits 0x" +
                std::to_string(badFlags));
        }

        // --- unit_flags ---
        uint32 unit_flags = f[12].GetUInt32();
        if (uint32 disallowed = (unit_flags & ~UNIT_FLAG_ALLOWED))
        {
            uint32 fixed_flags = unit_flags & UNIT_FLAG_ALLOWED;
            std::ostringstream s;
            s << "`unit_flags`=" << fixed_flags;
            setClauses.push_back(s.str());
            comments.push_back("unit_flags removed disallowed bits 0x" +
                std::to_string(disallowed));
        }

        // --- unit_flags2 ---
        uint32 unit_flags2 = f[13].GetUInt32();
        if (uint32 disallowed = (unit_flags2 & ~UNIT_FLAG2_ALLOWED))
        {
            uint32 fixed_flags = unit_flags2 & UNIT_FLAG2_ALLOWED;
            std::ostringstream s;
            s << "`unit_flags2`=" << fixed_flags;
            setClauses.push_back(s.str());
            comments.push_back("unit_flags2 removed disallowed bits 0x" +
                std::to_string(disallowed));
        }

        // --- unit_flags3 ---
        uint32 unit_flags3 = f[14].GetUInt32();
        if (uint32 disallowed = (unit_flags3 & ~UNIT_FLAG3_ALLOWED))
        {
            uint32 fixed_flags = unit_flags3 & UNIT_FLAG3_ALLOWED;
            std::ostringstream s;
            s << "`unit_flags3`=" << fixed_flags;
            setClauses.push_back(s.str());
            comments.push_back("unit_flags3 removed disallowed bits 0x" +
                std::to_string(disallowed));
        }

        // --- VignetteID ---
        uint32 vignette = f[15].GetUInt32();
        if (vignette && !sVignetteStore.HasRecord(vignette))
        {
            setClauses.push_back("`VignetteID`=0");
            comments.push_back("VignetteID " + std::to_string(vignette) + "->0 (non-existing)");
        }

        // --- KillCredit1/2 ---
        for (uint8 k = 0; k < MAX_KILL_CREDIT; ++k)
        {
            uint32 killCredit = f[16 + k].GetUInt32();
            if (killCredit && !sObjectMgr->GetCreatureTemplate(killCredit))
            {
                std::string col = "`KillCredit" + std::to_string(k + 1) + "`=0";
                setClauses.push_back(col);
                comments.push_back("KillCredit" + std::to_string(k + 1) + " " +
                    std::to_string(killCredit) + "->0 (non-existing)");
            }
        }

        if (setClauses.empty())
            continue;

        // Build single UPDATE with all fixes for this row
        Fix fix;
        fix.group = "creature_template";

        std::ostringstream cmt;
        cmt << "entry=" << entry << ": ";
        for (size_t i = 0; i < comments.size(); ++i)
        {
            if (i > 0) cmt << "; ";
            cmt << comments[i];
        }
        fix.comment = cmt.str();

        std::ostringstream sql;
        sql << "UPDATE `creature_template` SET ";
        for (size_t i = 0; i < setClauses.size(); ++i)
        {
            if (i > 0) sql << ", ";
            sql << setClauses[i];
        }
        sql << " WHERE `entry`=" << entry << ";";
        fix.sql = sql.str();

        fixes.push_back(fix);
    } while (result->NextRow());
}


// ---- Group 4: creature (spawn) field validations ----

static void CheckCreatureSpawns(std::vector<Fix>& fixes)
{
    // 0:guid 1:id 2:map 3:MovementType 4:wander_distance
    // 5:phaseUseFlags 6:phaseid 7:phasegroup 8:terrainSwapMap
    // 9:unit_flags 10:unit_flags2 11:unit_flags3 12:curHealthPct
    QueryResult result = WorldDatabase.Query(
        "SELECT `guid`, `id`, `map`, `MovementType`, `wander_distance`, "
        "`phaseUseFlags`, `phaseid`, `phasegroup`, `terrainSwapMap`, "
        "`unit_flags`, `unit_flags2`, `unit_flags3`, `curHealthPct` FROM `creature`");
    if (!result)
        return;

    do
    {
        Field* f = result->Fetch();
        uint64 guid = f[0].GetUInt64();
        uint32 entry = f[1].GetUInt32();
        uint32 mapId = f[2].GetUInt16();

        std::vector<std::string> setClauses;
        std::vector<std::string> comments;

        // --- MovementType ---
        uint8 movementType = f[3].GetUInt8();
        if (movementType >= MAX_DB_MOTION_TYPE)
        {
            setClauses.push_back("`MovementType`=0");
            comments.push_back("MovementType " + std::to_string(movementType) + "->0 (invalid)");
        }

        // --- wander_distance ---
        float wander = f[4].GetFloat();
        if (wander < 0.0f)
        {
            setClauses.push_back("`wander_distance`=0");
            comments.push_back("wander_distance " + std::to_string(wander) + "->0 (negative)");
        }
        else if (wander > 0.0f && wander < 0.1f)
        {
            setClauses.push_back("`wander_distance`=0");
            comments.push_back("wander_distance " + std::to_string(wander) + "->0 (too small)");
        }
        else if (movementType == 0 /*IDLE*/ && wander != 0.0f)
        {
            setClauses.push_back("`wander_distance`=0");
            comments.push_back("wander_distance " + std::to_string(wander) + "->0 (idle but nonzero)");
        }
        else if (movementType == 1 /*RANDOM*/ && wander == 0.0f)
        {
            setClauses.push_back("`MovementType`=0");
            comments.push_back("MovementType 1->0 (random but wander=0)");
        }

        // --- phaseUseFlags ---
        uint8 phaseUseFlags = f[5].GetUInt8();
        if (phaseUseFlags & uint8(~PHASE_USE_FLAGS_ALL))
        {
            uint8 fixed_flags = phaseUseFlags & PHASE_USE_FLAGS_ALL;
            std::ostringstream s;
            s << "`phaseUseFlags`=" << uint32(fixed_flags);
            setClauses.push_back(s.str());
            comments.push_back("phaseUseFlags removed unknown bits");
        }
        if ((phaseUseFlags & PHASE_USE_FLAGS_ALWAYS_VISIBLE) &&
            (phaseUseFlags & PHASE_USE_FLAGS_INVERSE))
        {
            uint8 fixed_flags = phaseUseFlags & ~uint8(PHASE_USE_FLAGS_INVERSE);
            std::ostringstream s;
            s << "`phaseUseFlags`=" << uint32(fixed_flags);
            setClauses.push_back(s.str());
            comments.push_back("phaseUseFlags removed INVERSE (conflicts with ALWAYS_VISIBLE)");
        }

        // --- phaseId ---
        uint32 phaseId = f[6].GetUInt32();
        if (phaseId && !sPhaseStore.LookupEntry(phaseId))
        {
            setClauses.push_back("`phaseid`=0");
            comments.push_back("phaseid " + std::to_string(phaseId) + "->0 (non-existing)");
        }

        // --- phaseGroup ---
        uint32 phaseGroup = f[7].GetUInt32();
        if (phaseGroup && phaseId)
        {
            setClauses.push_back("`phasegroup`=0");
            comments.push_back("phasegroup " + std::to_string(phaseGroup) + "->0 (both phaseid and phasegroup set)");
        }
        else if (phaseGroup && !sDB2Manager.GetPhasesForGroup(phaseGroup))
        {
            setClauses.push_back("`phasegroup`=0");
            comments.push_back("phasegroup " + std::to_string(phaseGroup) + "->0 (non-existing)");
        }

        // --- terrainSwapMap ---
        int32 terrainSwapMap = f[8].GetInt32();
        if (terrainSwapMap != -1)
        {
            MapEntry const* mapEntry = sMapStore.LookupEntry(terrainSwapMap);
            if (!mapEntry)
            {
                setClauses.push_back("`terrainSwapMap`=-1");
                comments.push_back("terrainSwapMap " + std::to_string(terrainSwapMap) + "->-1 (non-existing)");
            }
            else if (mapEntry->ParentMapID != int16(mapId))
            {
                setClauses.push_back("`terrainSwapMap`=-1");
                comments.push_back("terrainSwapMap " + std::to_string(terrainSwapMap) + "->-1 (wrong parent map)");
            }
        }

        // --- unit_flags (spawn-level override, nullable) ---
        if (!f[9].IsNull())
        {
            uint32 unit_flags = f[9].GetUInt32();
            if (uint32 disallowed = (unit_flags & ~UNIT_FLAG_ALLOWED))
            {
                uint32 fixed_flags = unit_flags & UNIT_FLAG_ALLOWED;
                std::ostringstream s;
                s << "`unit_flags`=" << fixed_flags;
                setClauses.push_back(s.str());
                comments.push_back("unit_flags removed disallowed bits 0x" +
                    std::to_string(disallowed));
            }
        }

        // --- unit_flags2 ---
        if (!f[10].IsNull())
        {
            uint32 unit_flags2 = f[10].GetUInt32();
            if (uint32 disallowed = (unit_flags2 & ~UNIT_FLAG2_ALLOWED))
            {
                uint32 fixed_flags = unit_flags2 & UNIT_FLAG2_ALLOWED;
                std::ostringstream s;
                s << "`unit_flags2`=" << fixed_flags;
                setClauses.push_back(s.str());
                comments.push_back("unit_flags2 removed disallowed bits 0x" +
                    std::to_string(disallowed));
            }
        }

        // --- unit_flags3 ---
        if (!f[11].IsNull())
        {
            uint32 unit_flags3 = f[11].GetUInt32();
            if (uint32 disallowed = (unit_flags3 & ~UNIT_FLAG3_ALLOWED))
            {
                uint32 fixed_flags = unit_flags3 & UNIT_FLAG3_ALLOWED;
                std::ostringstream s;
                s << "`unit_flags3`=" << fixed_flags;
                setClauses.push_back(s.str());
                comments.push_back("unit_flags3 removed disallowed bits 0x" +
                    std::to_string(disallowed));
            }
        }

        // --- curHealthPct ---
        if (!f[12].IsNull())
        {
            uint32 curHealthPct = f[12].GetUInt32();
            uint32 clamped = std::clamp<uint32>(curHealthPct, 1, 100);
            if (curHealthPct != clamped)
            {
                std::ostringstream s;
                s << "`curHealthPct`=" << clamped;
                setClauses.push_back(s.str());
                comments.push_back("curHealthPct " + std::to_string(curHealthPct) +
                    "->" + std::to_string(clamped) + " (out of range)");
            }
        }

        if (setClauses.empty())
            continue;

        Fix fix;
        fix.group = "creature";

        std::ostringstream cmt;
        cmt << "guid=" << guid << " entry=" << entry << ": ";
        for (size_t i = 0; i < comments.size(); ++i)
        {
            if (i > 0) cmt << "; ";
            cmt << comments[i];
        }
        fix.comment = cmt.str();

        std::ostringstream sql;
        sql << "UPDATE `creature` SET ";
        for (size_t i = 0; i < setClauses.size(); ++i)
        {
            if (i > 0) sql << ", ";
            sql << setClauses[i];
        }
        sql << " WHERE `guid`=" << guid << ";";
        fix.sql = sql.str();

        fixes.push_back(fix);
    } while (result->NextRow());
}


// ---- Group 5: gameobject (spawn) field validations ----

static void CheckGameobjectSpawns(std::vector<Fix>& fixes)
{
    // 0:guid 1:id 2:map 3:state 4:phaseUseFlags 5:phaseid 6:phasegroup 7:terrainSwapMap
    QueryResult result = WorldDatabase.Query(
        "SELECT `guid`, `id`, `map`, `state`, "
        "`phaseUseFlags`, `phaseid`, `phasegroup`, `terrainSwapMap` "
        "FROM `gameobject`");
    if (!result)
        return;

    // MAX_GO_STATE is 4 in the core (GO_STATE_ACTIVE=0..DESTROYED=3).
    static constexpr uint32 MAX_GO_STATE_VAL = 4;

    do
    {
        Field* f = result->Fetch();
        uint64 guid = f[0].GetUInt64();
        uint32 entry = f[1].GetUInt32();
        uint32 mapId = f[2].GetUInt16();

        std::vector<std::string> setClauses;
        std::vector<std::string> comments;

        // --- state ---
        uint32 go_state = f[3].GetUInt8();
        if (go_state >= MAX_GO_STATE_VAL)
        {
            setClauses.push_back("`state`=0");
            comments.push_back("state " + std::to_string(go_state) + "->0 (invalid)");
        }

        // --- phaseUseFlags ---
        uint8 phaseUseFlags = f[4].GetUInt8();
        if (phaseUseFlags & uint8(~PHASE_USE_FLAGS_ALL))
        {
            uint8 fixed_flags = phaseUseFlags & PHASE_USE_FLAGS_ALL;
            std::ostringstream s;
            s << "`phaseUseFlags`=" << uint32(fixed_flags);
            setClauses.push_back(s.str());
            comments.push_back("phaseUseFlags removed unknown bits");
        }
        if ((phaseUseFlags & PHASE_USE_FLAGS_ALWAYS_VISIBLE) &&
            (phaseUseFlags & PHASE_USE_FLAGS_INVERSE))
        {
            uint8 fixed_flags = phaseUseFlags & ~uint8(PHASE_USE_FLAGS_INVERSE);
            std::ostringstream s;
            s << "`phaseUseFlags`=" << uint32(fixed_flags);
            setClauses.push_back(s.str());
            comments.push_back("phaseUseFlags removed INVERSE (conflicts with ALWAYS_VISIBLE)");
        }

        // --- phaseId ---
        uint32 phaseId = f[5].GetUInt32();
        if (phaseId && !sPhaseStore.LookupEntry(phaseId))
        {
            setClauses.push_back("`phaseid`=0");
            comments.push_back("phaseid " + std::to_string(phaseId) + "->0 (non-existing)");
        }

        // --- phaseGroup ---
        uint32 phaseGroup = f[6].GetUInt32();
        if (phaseGroup && phaseId)
        {
            setClauses.push_back("`phasegroup`=0");
            comments.push_back("phasegroup " + std::to_string(phaseGroup) + "->0 (both phaseid and phasegroup set)");
        }
        else if (phaseGroup && !sDB2Manager.GetPhasesForGroup(phaseGroup))
        {
            setClauses.push_back("`phasegroup`=0");
            comments.push_back("phasegroup " + std::to_string(phaseGroup) + "->0 (non-existing)");
        }

        // --- terrainSwapMap ---
        int32 terrainSwapMap = f[7].GetInt32();
        if (terrainSwapMap != -1)
        {
            MapEntry const* mapEntry = sMapStore.LookupEntry(terrainSwapMap);
            if (!mapEntry)
            {
                setClauses.push_back("`terrainSwapMap`=-1");
                comments.push_back("terrainSwapMap " + std::to_string(terrainSwapMap) + "->-1 (non-existing)");
            }
            else if (mapEntry->ParentMapID != int16(mapId))
            {
                setClauses.push_back("`terrainSwapMap`=-1");
                comments.push_back("terrainSwapMap " + std::to_string(terrainSwapMap) + "->-1 (wrong parent map)");
            }
        }

        if (setClauses.empty())
            continue;

        Fix fix;
        fix.group = "gameobject";

        std::ostringstream cmt;
        cmt << "guid=" << guid << " entry=" << entry << ": ";
        for (size_t i = 0; i < comments.size(); ++i)
        {
            if (i > 0) cmt << "; ";
            cmt << comments[i];
        }
        fix.comment = cmt.str();

        std::ostringstream sql;
        sql << "UPDATE `gameobject` SET ";
        for (size_t i = 0; i < setClauses.size(); ++i)
        {
            if (i > 0) sql << ", ";
            sql << setClauses[i];
        }
        sql << " WHERE `guid`=" << guid << ";";
        fix.sql = sql.str();

        fixes.push_back(fix);
    } while (result->NextRow());
}


// ---- Group 6: creature_template_difficulty validations ----

static void CheckCreatureTemplateDifficulty(std::vector<Fix>& fixes)
{
    // HealthScalingExpansion: valid range is [EXPANSION_LEVEL_CURRENT, MAX_EXPANSIONS)
    // EXPANSION_LEVEL_CURRENT = -1, so anything < -1 or >= MAX_EXPANSIONS is bad.
    // GoldMin > GoldMax: set GoldMax = GoldMin.
    QueryResult result = WorldDatabase.Query(
        "SELECT `Entry`, `DifficultyID`, `HealthScalingExpansion`, `GoldMin`, `GoldMax` "
        "FROM `creature_template_difficulty`");
    if (!result)
        return;

    do
    {
        Field* f = result->Fetch();
        uint32 entry = f[0].GetUInt32();
        int32 difficulty = f[1].GetInt32();
        int32 healthScalingExp = f[2].GetInt32();
        uint32 goldMin = f[3].GetUInt32();
        uint32 goldMax = f[4].GetUInt32();

        std::vector<std::string> setClauses;
        std::vector<std::string> comments;

        // HealthScalingExpansion
        if (healthScalingExp < EXPANSION_LEVEL_CURRENT || healthScalingExp >= MAX_EXPANSIONS)
        {
            setClauses.push_back("`HealthScalingExpansion`=0");
            comments.push_back("HealthScalingExpansion " + std::to_string(healthScalingExp) + "->0 (invalid)");
        }

        // GoldMin > GoldMax
        if (goldMin > goldMax)
        {
            std::ostringstream s;
            s << "`GoldMax`=" << goldMin;
            setClauses.push_back(s.str());
            comments.push_back("GoldMax " + std::to_string(goldMax) + "->" +
                std::to_string(goldMin) + " (was < GoldMin)");
        }

        if (setClauses.empty())
            continue;

        Fix fix;
        fix.group = "creature_template_difficulty";

        std::ostringstream cmt;
        cmt << "entry=" << entry << " difficulty=" << difficulty << ": ";
        for (size_t i = 0; i < comments.size(); ++i)
        {
            if (i > 0) cmt << "; ";
            cmt << comments[i];
        }
        fix.comment = cmt.str();

        std::ostringstream sql;
        sql << "UPDATE `creature_template_difficulty` SET ";
        for (size_t i = 0; i < setClauses.size(); ++i)
        {
            if (i > 0) sql << ", ";
            sql << setClauses[i];
        }
        sql << " WHERE `Entry`=" << entry << " AND `DifficultyID`=" << difficulty << ";";
        fix.sql = sql.str();

        fixes.push_back(fix);
    } while (result->NextRow());
}


// ---- Group 7: orphaned creature_addon records ----

static void CheckOrphanedCreatureAddon(std::vector<Fix>& fixes)
{
    // Records in creature_addon whose guid doesn't exist in creature table
    QueryResult result = WorldDatabase.Query(
        "SELECT ca.`guid` FROM `creature_addon` ca "
        "LEFT JOIN `creature` c ON ca.`guid` = c.`guid` "
        "WHERE c.`guid` IS NULL");
    if (!result)
        return;

    do
    {
        Field* f = result->Fetch();
        uint64 guid = f[0].GetUInt64();

        Fix fix;
        fix.group = "creature_addon (orphaned)";
        fix.comment = "guid=" + std::to_string(guid) + " not in creature table";

        std::ostringstream sql;
        sql << "DELETE FROM `creature_addon` WHERE `guid`=" << guid << ";";
        fix.sql = sql.str();

        fixes.push_back(fix);
    } while (result->NextRow());
}


// ---- Group 8: orphaned gameobject_addon records ----

static void CheckOrphanedGameobjectAddon(std::vector<Fix>& fixes)
{
    QueryResult result = WorldDatabase.Query(
        "SELECT ga.`guid` FROM `gameobject_addon` ga "
        "LEFT JOIN `gameobject` g ON ga.`guid` = g.`guid` "
        "WHERE g.`guid` IS NULL");
    if (!result)
        return;

    do
    {
        Field* f = result->Fetch();
        uint64 guid = f[0].GetUInt64();

        Fix fix;
        fix.group = "gameobject_addon (orphaned)";
        fix.comment = "guid=" + std::to_string(guid) + " not in gameobject table";

        std::ostringstream sql;
        sql << "DELETE FROM `gameobject_addon` WHERE `guid`=" << guid << ";";
        fix.sql = sql.str();

        fixes.push_back(fix);
    } while (result->NextRow());
}


// ---- Group 9: orphaned linked_respawn records ----

static void CheckOrphanedLinkedRespawn(std::vector<Fix>& fixes)
{
    // linked_respawn: guid (master), linkedGuid (slave), linkType
    // linkType: 0=creature->creature, 1=creature->go, 2=go->go, 3=go->creature
    // We check if master and slave exist in their respective tables.
    // Use a single query with subqueries for efficiency.
    QueryResult result = WorldDatabase.Query(
        "SELECT lr.`guid`, lr.`linkedGuid`, lr.`linkType` FROM `linked_respawn` lr "
        "WHERE "
        // Master is creature (linkType 0,1): check creature table
        "((lr.linkType IN (0,1)) AND lr.`guid` NOT IN (SELECT `guid` FROM `creature`)) OR "
        // Master is gameobject (linkType 2,3): check gameobject table
        "((lr.linkType IN (2,3)) AND lr.`guid` NOT IN (SELECT `guid` FROM `gameobject`)) OR "
        // Slave is creature (linkType 0,3): check creature table
        "((lr.linkType IN (0,3)) AND lr.`linkedGuid` NOT IN (SELECT `guid` FROM `creature`)) OR "
        // Slave is gameobject (linkType 1,2): check gameobject table
        "((lr.linkType IN (1,2)) AND lr.`linkedGuid` NOT IN (SELECT `guid` FROM `gameobject`))");
    if (!result)
        return;

    do
    {
        Field* f = result->Fetch();
        uint64 guid = f[0].GetUInt64();
        uint64 linkedGuid = f[1].GetUInt64();
        uint8 linkType = f[2].GetUInt8();

        Fix fix;
        fix.group = "linked_respawn (orphaned)";
        fix.comment = "guid=" + std::to_string(guid) +
            " linkedGuid=" + std::to_string(linkedGuid) +
            " linkType=" + std::to_string(linkType) + " (master or slave not found)";

        std::ostringstream sql;
        sql << "DELETE FROM `linked_respawn` WHERE `guid`=" << guid
            << " AND `linkedGuid`=" << linkedGuid << ";";
        fix.sql = sql.str();

        fixes.push_back(fix);
    } while (result->NextRow());
}


// ---- Group 10: orphaned creature_template_difficulty records ----

static void CheckOrphanedCreatureTemplateDifficulty(std::vector<Fix>& fixes)
{
    // Records in creature_template_difficulty whose Entry doesn't exist in creature_template
    QueryResult result = WorldDatabase.Query(
        "SELECT ctd.`Entry`, ctd.`DifficultyID` FROM `creature_template_difficulty` ctd "
        "LEFT JOIN `creature_template` ct ON ctd.`Entry` = ct.`entry` "
        "WHERE ct.`entry` IS NULL");
    if (!result)
        return;

    do
    {
        Field* f = result->Fetch();
        uint32 entry = f[0].GetUInt32();
        int32 difficulty = f[1].GetInt32();

        Fix fix;
        fix.group = "creature_template_difficulty (orphaned)";
        fix.comment = "Entry=" + std::to_string(entry) +
            " DifficultyID=" + std::to_string(difficulty) + " not in creature_template";

        std::ostringstream sql;
        sql << "DELETE FROM `creature_template_difficulty` WHERE `Entry`=" << entry
            << " AND `DifficultyID`=" << difficulty << ";";
        fix.sql = sql.str();

        fixes.push_back(fix);
    } while (result->NextRow());
}


// ---- Group 11: orphaned creature_model_info records ----

static void CheckOrphanedCreatureModelInfo(std::vector<Fix>& fixes)
{
    // Records in creature_model_info whose DisplayID doesn't exist in CreatureDisplayInfo.db2
    QueryResult result = WorldDatabase.Query(
        "SELECT `DisplayID` FROM `creature_model_info`");
    if (!result)
        return;

    do
    {
        Field* f = result->Fetch();
        uint32 displayId = f[0].GetUInt32();

        if (sCreatureDisplayInfoStore.LookupEntry(displayId))
            continue;

        Fix fix;
        fix.group = "creature_model_info (orphaned)";
        fix.comment = "DisplayID=" + std::to_string(displayId) +
            " not in CreatureDisplayInfo.db2";

        std::ostringstream sql;
        sql << "DELETE FROM `creature_model_info` WHERE `DisplayID`=" << displayId << ";";
        fix.sql = sql.str();

        fixes.push_back(fix);
    } while (result->NextRow());
}


// ---- Group 12: creature_template_model — non-existent CreatureDisplayID ----

static void CheckCreatureTemplateModelBadDisplay(std::vector<Fix>& fixes)
{
    // 0:CreatureID 1:CreatureDisplayID 2:DisplayScale 3:Probability
    QueryResult result = WorldDatabase.Query(
        "SELECT `CreatureID`, `CreatureDisplayID` FROM `creature_template_model`");
    if (!result)
        return;

    do
    {
        Field* f = result->Fetch();
        uint32 creatureId = f[0].GetUInt32();
        uint32 displayId = f[1].GetUInt32();

        if (sCreatureDisplayInfoStore.LookupEntry(displayId))
            continue;

        Fix fix;
        fix.group = "creature_template_model (bad display)";
        fix.comment = "CreatureID=" + std::to_string(creatureId) +
            " CreatureDisplayID=" + std::to_string(displayId) + " not in CreatureDisplayInfo.db2";

        std::ostringstream sql;
        sql << "DELETE FROM `creature_template_model` WHERE `CreatureID`=" << creatureId
            << " AND `CreatureDisplayID`=" << displayId << ";";
        fix.sql = sql.str();

        fixes.push_back(fix);
    } while (result->NextRow());
}


// ---- Group 13: creature_equip_template — unknown item, bad appearance, not equipable ----

static void CheckCreatureEquipTemplate(std::vector<Fix>& fixes)
{
    // 0:CreatureID 1:ID 2:ItemID1 3:AppearanceModID1 4:ItemVisual1
    // 5:ItemID2 6:AppearanceModID2 7:ItemVisual2
    // 8:ItemID3 9:AppearanceModID3 10:ItemVisual3
    QueryResult result = WorldDatabase.Query(
        "SELECT `CreatureID`, `ID`, "
        "`ItemID1`, `AppearanceModID1`, `ItemVisual1`, "
        "`ItemID2`, `AppearanceModID2`, `ItemVisual2`, "
        "`ItemID3`, `AppearanceModID3`, `ItemVisual3` "
        "FROM `creature_equip_template`");
    if (!result)
        return;

    do
    {
        Field* f = result->Fetch();
        uint32 creatureId = f[0].GetUInt32();
        uint8 equipId = f[1].GetUInt8();

        std::vector<std::string> setClauses;
        std::vector<std::string> comments;

        for (uint8 i = 0; i < MAX_EQUIPMENT_ITEMS; ++i)
        {
            uint32 itemId = f[2 + i * 3].GetUInt32();
            uint32 appearanceModId = f[3 + i * 3].GetUInt16();

            if (!itemId)
                continue;

            ItemEntry const* dbcItem = sItemStore.LookupEntry(itemId);
            if (!dbcItem)
            {
                std::string col = "`ItemID" + std::to_string(i + 1) + "`=0";
                setClauses.push_back(col);
                comments.push_back("ItemID" + std::to_string(i + 1) + " " +
                    std::to_string(itemId) + "->0 (unknown item)");
                continue;
            }

            // Check appearance
            if (appearanceModId && !TransmogMgr::GetItemModifiedAppearance(itemId, appearanceModId))
            {
                std::string col = "`AppearanceModID" + std::to_string(i + 1) + "`=0";
                setClauses.push_back(col);
                comments.push_back("AppearanceModID" + std::to_string(i + 1) + " " +
                    std::to_string(appearanceModId) + "->0 (unknown appearance)");
                // Don't continue — still check if item is equipable
            }

            // Check if equipable in a hand
            bool equipable = false;
            for (InventoryType invType : InventoryTypesEquipable)
            {
                if (dbcItem->InventoryType == invType)
                {
                    equipable = true;
                    break;
                }
            }
            if (!equipable)
            {
                std::string col = "`ItemID" + std::to_string(i + 1) + "`=0";
                setClauses.push_back(col);
                comments.push_back("ItemID" + std::to_string(i + 1) + " " +
                    std::to_string(itemId) + "->0 (not equipable in hand)");
            }
        }

        if (setClauses.empty())
            continue;

        Fix fix;
        fix.group = "creature_equip_template";

        std::ostringstream cmt;
        cmt << "CreatureID=" << creatureId << " ID=" << uint32(equipId) << ": ";
        for (size_t i = 0; i < comments.size(); ++i)
        {
            if (i > 0) cmt << "; ";
            cmt << comments[i];
        }
        fix.comment = cmt.str();

        std::ostringstream sql;
        sql << "UPDATE `creature_equip_template` SET ";
        for (size_t i = 0; i < setClauses.size(); ++i)
        {
            if (i > 0) sql << ", ";
            sql << setClauses[i];
        }
        sql << " WHERE `CreatureID`=" << creatureId << " AND `ID`=" << uint32(equipId) << ";";
        fix.sql = sql.str();

        fixes.push_back(fix);
    } while (result->NextRow());
}


// ---- Group 14: npc_text — probability without BroadcastTextID, or bad BroadcastTextID ----

static void CheckNpcText(std::vector<Fix>& fixes)
{
    // 0:ID 1-8:Probability0-7 9-16:BroadcastTextID0-7
    QueryResult result = WorldDatabase.Query(
        "SELECT `ID`, "
        "`Probability0`, `Probability1`, `Probability2`, `Probability3`, "
        "`Probability4`, `Probability5`, `Probability6`, `Probability7`, "
        "`BroadcastTextID0`, `BroadcastTextID1`, `BroadcastTextID2`, `BroadcastTextID3`, "
        "`BroadcastTextID4`, `BroadcastTextID5`, `BroadcastTextID6`, `BroadcastTextID7` "
        "FROM `npc_text`");
    if (!result)
        return;

    do
    {
        Field* f = result->Fetch();
        uint32 textId = f[0].GetUInt32();
        if (!textId)
            continue;

        std::vector<std::string> setClauses;
        std::vector<std::string> comments;

        for (uint8 i = 0; i < 8; ++i)
        {
            float probability = f[1 + i].GetFloat();
            uint32 broadcastTextId = f[9 + i].GetUInt32();

            // Bad BroadcastTextID — set both to 0
            if (broadcastTextId && !sBroadcastTextStore.LookupEntry(broadcastTextId))
            {
                std::ostringstream s;
                s << "`Probability" << uint32(i) << "`=0, `BroadcastTextID" << uint32(i) << "`=0";
                setClauses.push_back(s.str());
                comments.push_back("BroadcastTextID" + std::to_string(i) + " " +
                    std::to_string(broadcastTextId) + " non-existing, zeroed");
                continue;
            }

            // Probability > 0 but no BroadcastTextID
            if (probability > 0.0f && broadcastTextId == 0)
            {
                std::ostringstream s;
                s << "`Probability" << uint32(i) << "`=0";
                setClauses.push_back(s.str());
                comments.push_back("Probability" + std::to_string(i) +
                    " >0 but no BroadcastTextID, zeroed");
            }
        }

        if (setClauses.empty())
            continue;

        Fix fix;
        fix.group = "npc_text";

        std::ostringstream cmt;
        cmt << "ID=" << textId << ": ";
        for (size_t i = 0; i < comments.size(); ++i)
        {
            if (i > 0) cmt << "; ";
            cmt << comments[i];
        }
        fix.comment = cmt.str();

        std::ostringstream sql;
        sql << "UPDATE `npc_text` SET ";
        for (size_t i = 0; i < setClauses.size(); ++i)
        {
            if (i > 0) sql << ", ";
            sql << setClauses[i];
        }
        sql << " WHERE `ID`=" << textId << ";";
        fix.sql = sql.str();

        fixes.push_back(fix);
    } while (result->NextRow());
}


// ---- Group 15: gameobject_template — data field validations ----

static void CheckGameobjectTemplateData(std::vector<Fix>& fixes)
{
    // We need entry, type, and all Data0..Data11 fields that are checked
    // 0:entry 1:type 2-13:Data0-Data11
    QueryResult result = WorldDatabase.Query(
        "SELECT `entry`, `type`, "
        "`Data0`, `Data1`, `Data2`, `Data3`, `Data4`, `Data5`, "
        "`Data6`, `Data7`, `Data8`, `Data9`, `Data10`, `Data11` "
        "FROM `gameobject_template`");
    if (!result)
        return;

    do
    {
        Field* f = result->Fetch();
        uint32 entry = f[0].GetUInt32();
        uint32 goType = f[1].GetUInt8();

        std::vector<std::string> setClauses;
        std::vector<std::string> comments;

        // GAMEOBJECT_TYPE_GOOBER = 10
        if (goType == 10)
        {
            // data3 = consumable (must be 0 or 1)
            uint32 consumable = f[5].GetUInt32(); // Data3
            if (consumable > 1)
            {
                setClauses.push_back("`Data3`=0");
                comments.push_back("Data3 (consumable) " + std::to_string(consumable) + "->0 (not boolean)");
            }

            // data7 = pageID — check PageText exists
            uint32 pageId = f[9].GetUInt32(); // Data7
            if (pageId && !sObjectMgr->GetPageText(pageId))
            {
                setClauses.push_back("`Data7`=0");
                comments.push_back("Data7 (pageID) " + std::to_string(pageId) + "->0 (PageText not found)");
            }
        }

        // GAMEOBJECT_TYPE_SPELLCASTER = 22
        if (goType == 22)
        {
            // data0 = spell — must exist
            uint32 spellId = f[2].GetUInt32(); // Data0
            if (!sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE))
            {
                setClauses.push_back("`Data0`=0");
                comments.push_back("Data0 (spell) " + std::to_string(spellId) + "->0 (spell not found)");
            }
        }

        // GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING = 33
        if (goType == 33)
        {
            // data2 = HealthRec — check DestructibleHitpoint exists
            uint32 healthRec = f[4].GetUInt32(); // Data2
            if (healthRec && !sObjectMgr->GetDestructibleHitpoint(healthRec))
            {
                setClauses.push_back("`Data2`=0");
                comments.push_back("Data2 (HealthRec) " + std::to_string(healthRec) + "->0 (DestructibleHitpoint not found)");
            }
        }

        if (setClauses.empty())
            continue;

        Fix fix;
        fix.group = "gameobject_template (data)";

        std::ostringstream cmt;
        cmt << "entry=" << entry << " type=" << goType << ": ";
        for (size_t i = 0; i < comments.size(); ++i)
        {
            if (i > 0) cmt << "; ";
            cmt << comments[i];
        }
        fix.comment = cmt.str();

        std::ostringstream sql;
        sql << "UPDATE `gameobject_template` SET ";
        for (size_t i = 0; i < setClauses.size(); ++i)
        {
            if (i > 0) sql << ", ";
            sql << setClauses[i];
        }
        sql << " WHERE `entry`=" << entry << ";";
        fix.sql = sql.str();

        fixes.push_back(fix);
    } while (result->NextRow());
}


// ---- Group 16: creature_template — npcflag GOSSIP but no gossip menu assigned ----

static void CheckCreatureTemplateGossipFlag(std::vector<Fix>& fixes)
{
    // Find creatures that have UNIT_NPC_FLAG_GOSSIP (0x1) in npcflag
    // but have no entries in creature_template_gossip
    // UNIT_NPC_FLAG_GOSSIP = 0x00000001
    QueryResult result = WorldDatabase.Query(
        "SELECT ct.`entry` FROM `creature_template` ct "
        "WHERE (ct.`npcflag` & 1) != 0 "
        "AND NOT EXISTS (SELECT 1 FROM `creature_template_gossip` ctg WHERE ctg.`CreatureID` = ct.`entry`)");
    if (!result)
        return;

    do
    {
        Field* f = result->Fetch();
        uint32 entry = f[0].GetUInt32();

        Fix fix;
        fix.group = "creature_template (gossip flag)";
        fix.comment = "entry=" + std::to_string(entry) +
            " has UNIT_NPC_FLAG_GOSSIP but no gossip menu assigned, removing flag";

        // Remove UNIT_NPC_FLAG_GOSSIP (0x1) from npcflag
        std::ostringstream sql;
        sql << "UPDATE `creature_template` SET `npcflag` = `npcflag` & ~1 WHERE `entry`=" << entry << ";";
        fix.sql = sql.str();

        fixes.push_back(fix);
    } while (result->NextRow());
}


// ---- Group 17: creature_template_spell — non-existing spell references ----

static void CheckCreatureTemplateSpell(std::vector<Fix>& fixes)
{
    // 0:CreatureID 1:Index 2:Spell
    QueryResult result = WorldDatabase.Query(
        "SELECT `CreatureID`, `Index`, `Spell` FROM `creature_template_spell`");
    if (!result)
        return;

    do
    {
        Field* f = result->Fetch();
        uint32 creatureId = f[0].GetUInt32();
        uint8 index = f[1].GetUInt8();
        uint32 spell = f[2].GetUInt32();

        if (!spell)
            continue;

        if (sSpellMgr->GetSpellInfo(spell, DIFFICULTY_NONE))
            continue;

        Fix fix;
        fix.group = "creature_template_spell";
        fix.comment = "CreatureID=" + std::to_string(creatureId) +
            " Index=" + std::to_string(index) +
            " Spell=" + std::to_string(spell) + " non-existing, set to 0";

        std::ostringstream sql;
        sql << "UPDATE `creature_template_spell` SET `Spell`=0"
            << " WHERE `CreatureID`=" << creatureId
            << " AND `Index`=" << uint32(index) << ";";
        fix.sql = sql.str();

        fixes.push_back(fix);
    } while (result->NextRow());
}


// ============================================================================
// Helper implementations
// ============================================================================

bool EnsureDirectory(std::string const& path)
{
    if (path.empty())
        return false;

#if defined(_WIN32)
    std::string p = path;
    for (char& c : p) if (c == '/') c = '\\';

    size_t pos = 0;
    while ((pos = p.find('\\', pos + 1)) != std::string::npos)
    {
        std::string sub = p.substr(0, pos);
        CreateDirectoryA(sub.c_str(), nullptr);
    }
    CreateDirectoryA(p.c_str(), nullptr);

    DWORD attrs = GetFileAttributesA(p.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
    std::string p = path;
    size_t pos = 0;
    while ((pos = p.find('/', pos + 1)) != std::string::npos)
    {
        std::string sub = p.substr(0, pos);
        mkdir(sub.c_str(), 0755);
    }
    mkdir(p.c_str(), 0755);

    struct stat st;
    return (stat(p.c_str(), &st) == 0) && S_ISDIR(st.st_mode);
#endif
}

std::string FormatTimestamp()
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << (tm.tm_year + 1900) << "-"
        << std::setw(2) << (tm.tm_mon + 1) << "-"
        << std::setw(2) << tm.tm_mday << " "
        << std::setw(2) << tm.tm_hour << ":"
        << std::setw(2) << tm.tm_min << ":"
        << std::setw(2) << tm.tm_sec;
    return oss.str();
}

std::vector<uint32> ParseDifficulties(std::string const& str)
{
    std::vector<uint32> result;
    std::istringstream iss(str);
    std::string token;
    while (std::getline(iss, token, ','))
    {
        size_t start = token.find_first_not_of(" \t");
        size_t end = token.find_last_not_of(" \t");
        if (start == std::string::npos)
            continue;
        token = token.substr(start, end - start + 1);
        if (token.empty())
            continue;
        try
        {
            result.push_back(static_cast<uint32>(std::stoul(token)));
        }
        catch (...)
        {
            // ignore non-numeric tokens
        }
    }
    return result;
}

std::string JoinDifficulties(std::vector<uint32> const& vals)
{
    std::ostringstream oss;
    for (size_t i = 0; i < vals.size(); ++i)
    {
        if (i > 0) oss << ",";
        oss << vals[i];
    }
    return oss.str();
}


// ----------------------------------------------------------------------------
// AddSC_db_autofix — script loader entry point
// ----------------------------------------------------------------------------
void AddSC_db_autofix()
{
    new db_autofix_worldscript();
}
