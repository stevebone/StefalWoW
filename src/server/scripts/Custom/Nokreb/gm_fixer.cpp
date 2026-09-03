/*
 * gm_fixer - a set of GM .fix commands for quick NPC fixes.
 *
 * Each application:
 *   1) Synchronously executes SQL in the world database
 *   2) Appends the executed SQL to a cumulative daily file
 *      <DD_MM_YYYY>.sql in the gm_fixes/ folder next to the worldserver
 *      (e.g. gm_fixes/08_07_2026.sql)
 *   3) Also appends to a zone-grouped file <zone_name>.sql, so every fix
 *      is accumulated per the zone where it was applied (e.g.
 *      Elwynn_Forest.sql). The zone is taken from the GM's current
 *      location. Date/time is kept in each block's header comment, since
 *      the file name itself carries the zone, not the date.
 *
 * Commands:
 *   .fix faction <factionId>  — set factionId for the selected NPC
 *   .fix friendly             — quick set friendly faction (35)
 *   .fix hostile              — quick set hostile faction (16)
 *   .fix walk                 — disable flight (StaticFlags1 = 0x10000000)
 *   .fix fly                  — enable flight (StaticFlags1 = 0x20000000)
 *   .fix delete               — delete the selected NPC's spawn (entry + guid)
 *   .fix godel                — delete the gameobject under the mouse (nearest GO)
 *   .fix godelid <guid>       — delete a gameobject spawn by its DB guid
 *   .fix gophase <phaseId>    — set phase of the gameobject under the mouse (nearest GO)
 *   .fix gophaseid <guid> <phaseId>
 *                            — set phase of a gameobject by its DB guid
 *   .fix queststart <questId> — add quest questId to the selected NPC
 *   .fix cmount <mountId>     — set mount display ID for the selected spawn (creature_addon)
 *   .fix gmount <mountId>     — set mount display ID for the template (creature_template_addon)
 *   .fix gsheath <sheath>     — set SheathState for the selected spawn (creature_addon)
 *   .fix csheath <sheath>     — set SheathState for the template (creature_template_addon)
 *   .fix pooladd              — gather ALL guids of the selected NPC's entry into a
 *                              new pool (pool_template + pool_members, like the
 *                              Badlands spawn sql), max_limit = 1
 *   .fix pooldel              — delete the pool of the selected NPC's entry
 *                              (pool_members + pool_template)
 *   .fix follow <leader> <follower> <range>
 *                            — makes follower follow leader: adds a smart_scripts
 *                              DISTANCE_CREATURE(75) + FOLLOW(29) line on the
 *                              follower's script; follower follows leader when
 *                              within <range> yards
 *   .fix spellwow [wowheadNpcId] [repeatSec] [tag]
 *                            — fetch NPC abilities from Wowhead and insert each
 *                              as a smart_scripts CAST row on the selected NPC
 *                              (HTTPS fetch via boost::beast; same CAST layout
 *                              as .fix spellA / .fix spellCB). No npcId = use
 *                              the selected creature's entry as the Wowhead id.
 *   .fix wowinfo [id]       — compare the Wowhead page of the given NPC (or the
 *                             selected creature) against this DB: spells, drops,
 *                             pickpocketing. Prints a diff ([+]/[-]/[?]). Read-only.
 *   .fix lootwow [id]       — replace creature_loot_template with Wowhead drops
 *                             (DELETE + INSERT) for the given NPC / selected creature.
 *   .fix pickpocketwow [id] — replace pickpocketing_loot_template with Wowhead
 *                             pickpocketing (DELETE + INSERT).
 *   .fix wowall [id]        — apply all three sections at once: spells + drops
 *                             + pickpocketing.
 *   .fix spellclear         — delete ALL smart_scripts CAST (action_type=11) rows
 *                             of the selected NPC (every spell it casts via SmartAI)
 *   .fix spelldel <N>       — delete the Nth CAST row of the selected NPC
 *                             (N = 1-based position in .fix spellinfo)
 *   .fix spellcopy <donorEntry> <recipientEntry>
 *                           — copy every CAST row from donor entry to recipient
 *                             entry (new ids; skips spells the recipient already casts)
 *
 * Installation:
 *   1. File at: src/server/scripts/Custom/gm_fixer/gm_fixer.cpp
 *   2. In custom_script_loader.cpp:
 *        void AddSC_gm_fixer();        // forward
 *        AddSC_gm_fixer();             // inside AddCustomScripts()
 *   3. Regenerate CMake, rebuild.
 */

#include "ScriptMgr.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "Creature.h"
#include "GameObject.h"
#include "PhasingHandler.h"
#include "DatabaseEnv.h"
#include "ObjectMgr.h"
#include "Log.h"
#include "Player.h"
#include "WorldSession.h"
#include "RBAC.h"
#include "QuestDef.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "MotionMaster.h"
#include "WaypointManager.h"
#include "DB2Structure.h"
#include "DB2Stores.h"

#include <fstream>
#include <cstdio>
#include <ctime>
#if defined(_WIN32) || defined(_WIN64)
#include <process.h> // _popen / _pclose on MSVC
#endif
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <cmath>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <cctype>

// HTTP(S) fetch for .fix spellwow (Wowhead NPC page).
// This fork does NOT link libcurl — like NpcLLM / PlayerBotLLM it uses
// boost::asio + boost::beast (+ boost::asio::ssl for HTTPS). SNI is required
// for CloudFront-backed hosts like www.wowhead.com.
//
// OpenSSL headers are needed directly for the TLS-fingerprint hardening
// (SSL_set_cipher_list / SSL_set_ciphersuites / SSL_set_alpn_protos) and for
// OPENSSL_VERSION_NUMBER version gating.
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <openssl/ssl.h>
#include <openssl/opensslv.h>

using namespace Trinity::ChatCommands;

namespace
{
    // Quick faction presets.
    // Names with FIX_ prefix - to avoid conflicts with FactionTemplates enum
    // from SharedDefines.h (which already has FACTION_FRIENDLY / FACTION_HOSTILE).
    constexpr uint32 FIX_FACTION_FRIENDLY = 35;
    constexpr uint32 FIX_FACTION_HOSTILE = 16;

    // StaticFlags1 for movement control
    // (in StefalWoW / TrinityCore master these bits correspond to
    //  movement flags in creature_template_difficulty.StaticFlags1)
    constexpr uint32 STATIC_FLAG_WALK = 268435456u;  // 0x10000000
    constexpr uint32 STATIC_FLAG_FLY = 536870912u;  // 0x20000000

    // Full dead-flag combination for .fix gdead / .fix cdead.
    constexpr uint32 DEAD_FLAGS1 = 537133568;
    constexpr uint32 DEAD_FLAGS2 = 2049;
    constexpr uint32 DEAD_FLAGS3 = 8192;

    // Alive (clear) flags for .fix galive / .fix calive — inverse of dead.
    constexpr uint32 ALIVE_FLAGS1 = 0;
    constexpr uint32 ALIVE_FLAGS2 = 0;
    constexpr uint32 ALIVE_FLAGS3 = 0;

    // Log channel name for TC_LOG_INFO, etc.
    static constexpr char const* LOG_CHAN = "scripts.gm_fixer";

    // VerifiedBuild for hotfix entries (current StefalWoW client build).
    constexpr uint32 HOTFIX_VERIFIED_BUILD = 67186;

    // Filename for separate spell fix log (GCD and CD).
    static constexpr char const* SPELL_CD_FILE = "spell_CD.sql";

    // Filename for separate creature size fix log.
    static constexpr char const* SCALE_SIZE_FILE = "creature_sizes.sql";

    // All gm_fixer SQL logs go into this subdirectory next to the worldserver.
    // Created automatically on first use if it doesn't exist.
    static constexpr char const* LOG_DIR = "gm_fixes";

    /*
     * Returns the path to the gm_fixes log directory (next to worldserver),
     * creating it if it doesn't exist. Returns the absolute path on success,
     * or an empty path on failure (callers should fall back to current_path()
     * so logging never breaks a fix).
     */
    static std::filesystem::path GetLogDir()
    {
        namespace fs = std::filesystem;
        try
        {
            fs::path dir = fs::current_path() / LOG_DIR;
            std::error_code ec;
            fs::create_directory(dir, ec); // no-op + no throw if it exists
            return dir;
        }
        catch (std::exception const& e)
        {
            TC_LOG_ERROR(LOG_CHAN, "gm_fixer: cannot create log dir '{}': {}", LOG_DIR, e.what());
            return {};
        }
    }

    /*
     * Appends a SQL query to a separate file gm_fixes/creature_sizes.sql.
     * This file only contains creature scale fixes.
     * The file is NOT date-dependent - it accumulates into a single common list.
     */
    static void AppendToScaleSizeFile(std::string const& sql, std::string const& fixType, std::string const& what = "")
    {
        namespace fs = std::filesystem;
        try
        {
            fs::path dir = GetLogDir();
            fs::path out = (dir.empty() ? fs::current_path() : dir) / SCALE_SIZE_FILE;
            std::ofstream os(out, std::ios::app);
            if (!os.is_open())
            {
                TC_LOG_ERROR(LOG_CHAN, "gm_fixer: cannot open '{}' for append", out.string());
                return;
            }

            std::time_t now = std::time(nullptr);
            std::tm tm{};
#if defined(_WIN32) || defined(_WIN64)
            localtime_s(&tm, &now);
#else
            localtime_r(&now, &tm);
#endif
            std::ostringstream stamp;
            stamp << std::setfill('0')
                << std::setw(2) << tm.tm_hour << ":"
                << std::setw(2) << tm.tm_min << ":"
                << std::setw(2) << tm.tm_sec;
            std::ostringstream date;
            date << std::setfill('0')
                << std::setw(2) << tm.tm_mday << "."
                << std::setw(2) << (tm.tm_mon + 1) << "."
                << (tm.tm_year + 1900);

            if (!what.empty())
                os << "-- [" << date.str() << " " << stamp.str() << "] " << fixType << ": " << what << "\n"
                << sql << "\n\n";
            else
                os << "-- [" << date.str() << " " << stamp.str() << "] " << fixType << "\n"
                << sql << "\n\n";
        }
        catch (std::exception const& e)
        {
            TC_LOG_ERROR(LOG_CHAN, "gm_fixer: exception writing creature_sizes.sql: {}", e.what());
        }
    }

    /*
     * Appends a SQL query to a separate file gm_fixes/spell_CD.sql.
     * This file only contains spell fixes — GCD and RecoveryTime.
     * The file is NOT date-dependent — it accumulates into a single common list of spell fixes.
     */
    static void AppendToSpellCdFile(std::string const& sql, std::string const& fixType)
    {
        namespace fs = std::filesystem;
        try
        {
            fs::path dir = GetLogDir();
            fs::path out = (dir.empty() ? fs::current_path() : dir) / SPELL_CD_FILE;
            std::ofstream os(out, std::ios::app);
            if (!os.is_open())
            {
                TC_LOG_ERROR(LOG_CHAN, "gm_fixer: cannot open '{}' for append", out.string());
                return;
            }

            std::time_t now = std::time(nullptr);
            std::tm tm{};
#if defined(_WIN32) || defined(_WIN64)
            localtime_s(&tm, &now);
#else
            localtime_r(&now, &tm);
#endif
            std::ostringstream stamp;
            stamp << std::setfill('0')
                << std::setw(2) << tm.tm_hour << ":"
                << std::setw(2) << tm.tm_min << ":"
                << std::setw(2) << tm.tm_sec;
            // Date for spell_CD.sql - in the file header, not on every line.
            std::ostringstream date;
            date << std::setfill('0')
                << std::setw(2) << tm.tm_mday << "."
                << std::setw(2) << (tm.tm_mon + 1) << "."
                << (tm.tm_year + 1900);

            os << "-- [" << date.str() << " " << stamp.str() << "] " << fixType << "\n"
                << sql << "\n\n";
        }
        catch (std::exception const& e)
        {
            TC_LOG_ERROR(LOG_CHAN, "gm_fixer: exception writing spell_CD.sql: {}", e.what());
        }
    }

    /*
     * Parses seconds (with support for fractional part and comma instead of dot).
     * Accepts: "1.5", "1,5", "0.5", "10".
     * Returns milliseconds: 1500, 1500, 500, 10000.
     *
     * If args is empty - returns defMs (default value).
     */
    static bool ParseSecondsToMs(char const* args, uint32 defMs, uint32& outMs)
    {
        if (!args || !*args)
        {
            outMs = defMs;
            return true;
        }
        // Copy to a local buffer and replace comma with dot.
        std::string s(args);
        for (char& c : s)
            if (c == ',') c = '.';
        // trim
        size_t start = s.find_first_not_of(" \t");
        if (start == std::string::npos)
        {
            outMs = defMs;
            return true;
        }
        s = s.substr(start);
        char* end = nullptr;
        double seconds = std::strtod(s.c_str(), &end);
        if (end == s.c_str())
            return false; // nothing was parsed
        if (seconds < 0.0)
            return false;
        double ms = seconds * 1000.0;
        if (ms > 4294967295.0) // UINT32_MAX
            return false;
        outMs = static_cast<uint32>(ms);
        return true;
    }


    /*
     * Generates the daily cumulative file name: DD_MM_YYYY.sql
     * Uses the server's local time.
     */
    static std::string MakeDailyFileName()
    {
        std::time_t now = std::time(nullptr);
        std::tm tm{};
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&tm, &now);
#else
        localtime_r(&now, &tm);
#endif
        std::ostringstream ss;
        ss << std::setfill('0')
            << std::setw(2) << tm.tm_mday << "_"
            << std::setw(2) << (tm.tm_mon + 1) << "_"
            << (tm.tm_year + 1900)
            << ".sql";
        return ss.str();
    }

    /*
     * Appends a SQL query to the cumulative daily file in the gm_fixes/ folder.
     * File: gm_fixes/DD_MM_YYYY.sql  (next to worldserver.exe; folder auto-created)
     * If the file doesn't exist — it's created. If it exists — the query is appended.
     * The fix type (fixType) is written as a block header, e.g.:
     *   -- [18:07:47] faction
     *   UPDATE creature_template SET faction = 16 WHERE entry = 1986;
     */
    static void AppendToDailySql(std::string const& sql, std::string const& fixType, std::string const& what = "")
    {
        namespace fs = std::filesystem;
        try
        {
            fs::path dir = GetLogDir();
            fs::path out = (dir.empty() ? fs::current_path() : dir) / MakeDailyFileName();
            std::ofstream os(out, std::ios::app);
            if (!os.is_open())
            {
                TC_LOG_ERROR(LOG_CHAN, "gm_fixer: cannot open '{}' for append", out.string());
                return;
            }

            std::time_t now = std::time(nullptr);
            std::tm tm{};
#if defined(_WIN32) || defined(_WIN64)
            localtime_s(&tm, &now);
#else
            localtime_r(&now, &tm);
#endif
            std::ostringstream stamp;
            stamp << std::setfill('0')
                << std::setw(2) << tm.tm_hour << ":"
                << std::setw(2) << tm.tm_min << ":"
                << std::setw(2) << tm.tm_sec;

            if (!what.empty())
                os << "-- [" << stamp.str() << "] " << fixType << ": " << what << "\n"
                << sql << "\n\n";
            else
                os << "-- [" << stamp.str() << "] " << fixType << "\n"
                << sql << "\n\n";
        }
        catch (std::exception const& e)
        {
            TC_LOG_ERROR(LOG_CHAN, "gm_fixer: exception writing daily sql: {}", e.what());
        }
    }

    /*
     * Builds a filesystem-safe zone name for use as a SQL log file name.
     *
     * Rules:
     *   - whitespace / apostrophe  -> '_'   (e.g. "Ahn'Qiraj" -> "Ahn_Qiraj",
     *                                            "Elwynn Forest" -> "Elwynn_Forest")
     *   - any other char that is invalid in a filename (or non-ASCII) -> '_'
     *   - consecutive '_' are collapsed into one
     *   - leading/trailing '_' are stripped
     *   - empty result -> "unknown"
     */
    static std::string SanitizeZoneName(std::string const& name)
    {
        std::string out;
        out.reserve(name.size());
        bool prevUnderscore = true; // suppress leading underscores
        for (char c : name)
        {
            char ch;
            if (c == ' ' || c == '\t' || c == '\'')
                ch = '_';
            else if ((c >= 'A' && c <= 'Z') ||
                     (c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '9') ||
                     c == '-' || c == '(' || c == ')')
                ch = c;
            else
                ch = '_'; // invalid filename char or non-ASCII -> separator

            if (ch == '_')
            {
                if (!prevUnderscore)
                {
                    out.push_back('_');
                    prevUnderscore = true;
                }
            }
            else
            {
                out.push_back(ch);
                prevUnderscore = false;
            }
        }
        // strip the single possible trailing '_'
        if (!out.empty() && out.back() == '_')
            out.pop_back();
        if (out.empty())
            return "unknown";
        return out;
    }

    /*
     * Appends a SQL query to the zone-grouped file <zone_name>.sql next to
     * the worldserver. The zone is derived from the GM's current location
     * (the player who runs the command), so every fix is accumulated in the
     * file for the zone where it was applied.
     *
     * Unlike the daily file, this one spans multiple days, so each block
     * header carries the full date+time:
     *   -- [30.07.2026 18:07:47] faction: ...
     *
     * The zone name is sanitized for use as a file name. If the zone can't
     * be resolved, falls back to "unknown_zone.sql".
     */
    static void AppendToZoneSql(ChatHandler* h, std::string const& sql,
                                std::string const& fixType, std::string const& what)
    {
        namespace fs = std::filesystem;

        // Resolve the zone name from the GM's current location.
        std::string zoneFile = "unknown_zone";
        if (h && h->GetSession())
        {
            if (Player* p = h->GetSession()->GetPlayer())
            {
                uint32 zoneId = p->GetZoneId();
                if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(zoneId))
                {
                    char const* raw = area->AreaName[DEFAULT_LOCALE];
                    if (raw && *raw)
                        zoneFile = SanitizeZoneName(raw);
                }
            }
        }

        try
        {
            fs::path dir = GetLogDir();
            fs::path out = (dir.empty() ? fs::current_path() : dir) / (zoneFile + ".sql");
            std::ofstream os(out, std::ios::app);
            if (!os.is_open())
            {
                TC_LOG_ERROR(LOG_CHAN, "gm_fixer: cannot open '{}' for append", out.string());
                return;
            }

            std::time_t now = std::time(nullptr);
            std::tm tm{};
#if defined(_WIN32) || defined(_WIN64)
            localtime_s(&tm, &now);
#else
            localtime_r(&now, &tm);
#endif
            std::ostringstream stamp;
            stamp << std::setfill('0')
                  << std::setw(2) << tm.tm_mday  << "."
                  << std::setw(2) << (tm.tm_mon + 1) << "."
                  << (tm.tm_year + 1900) << " "
                  << std::setw(2) << tm.tm_hour << ":"
                  << std::setw(2) << tm.tm_min  << ":"
                  << std::setw(2) << tm.tm_sec;

            if (!what.empty())
                os << "-- [" << stamp.str() << "] " << fixType << ": " << what << "\n"
                   << sql << "\n\n";
            else
                os << "-- [" << stamp.str() << "] " << fixType << "\n"
                   << sql << "\n\n";
        }
        catch (std::exception const& e)
        {
            TC_LOG_ERROR(LOG_CHAN, "gm_fixer: exception writing zone sql '{}': {}", zoneFile, e.what());
        }
    }

    /*
     * Writes a fix SQL block to BOTH log files:
     *   1) the cumulative daily file <DD_MM_YYYY>.sql
     *   2) the zone-grouped file <zone_name>.sql (zone = GM's location)
     *
     * Use this for every creature/world-DB fix so it shows up in both places.
     * (The type-specific logs creature_sizes.sql and spell_CD.sql stay separate
     * and are written by their own handlers.)
     */
    static void AppendFixLog(ChatHandler* h, std::string const& sql,
                             std::string const& fixType, std::string const& what = "")
    {
        AppendToDailySql(sql, fixType, what);
        AppendToZoneSql(h, sql, fixType, what);
    }

    /*
     * Applies SQL to the world database (synchronously) and appends it to the daily file
     * and the zone-grouped file.
     * fixType — a short tag for the SQL block header ("faction", "delete", ...).
     * what    — a human-readable description for the player and log.
     * Always returns true — to avoid kicking the player out of the command on error,
     * as is customary in NpcLLM-style.
     */
    static bool ApplyFix(ChatHandler* h, std::string const& sql,
        std::string const& fixType, char const* what)
    {
        // Apply to the world database. DirectExecute is synchronous, so the data
        // is guaranteed to be written before returning control to the GM.
        WorldDatabase.DirectExecute(sql.c_str());
        AppendFixLog(h, sql, fixType, what);
        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL: {}", sql));
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sql);
        return true;
    }

    /*
     * Get the selected NPC and its entry.
     * On error, sends a message to the player and returns false.
     */
    static bool GetSelectedCreature(ChatHandler* h, Creature*& outC, uint32& outEntry)
    {
        Player* p = h->GetSession()->GetPlayer();
        if (!p) return false;
        Creature* c = p->GetSelectedUnit() ? p->GetSelectedUnit()->ToCreature() : nullptr;
        if (!c)
        {
            h->SendSysMessage("Select a creature first.");
            return false;
        }
        outC = c;
        outEntry = c->GetEntry();
        return true;
    }

    // ========================================================================
    // .fix spellwow — Wowhead NPC fetcher + spell-dictionary parser.
    //
    // Fetches https://www.wowhead.com/npc=<id> over HTTPS and extracts every
    // WH.Gatherer.addData(6, ...) spell entry.
    //
    // Fetching Wowhead requires a Chrome-matching TLS fingerprint (JA3), which
    // OpenSSL cannot reproduce (extension order is hardcoded in its state
    // machine). We therefore shell out to curl-impersonate (a curl build on
    // BoringSSL that reproduces Chrome's JA3 byte-for-byte) via popen. If it
    // isn't installed we fall back to boost::beast + OpenSSL, which will 403 on
    // Wowhead's WAF but still works for non-WAF'd hosts and gives a clear
    // install hint to the GM.
    // ========================================================================

    // Fetch the Wowhead NPC page HTML over HTTPS.
    //
    // Wowhead is fronted by CloudFront / AWS WAF which blocks by TLS fingerprint
    // (JA3/JA4) — the hash covers the TLS version, cipher list AND the order of
    // TLS extensions. OpenSSL (used by boost::asio::ssl) hardcodes the extension
    // order in its state machine, so no API call (SSL_set_cipher_list /
    // SSL_set_ciphersuites / SSL_set_alpn_protos) can make OpenSSL's JA3 match
    // Chrome's. The boost::beast request will keep getting 403 however perfect
    // the headers and UA are.
    //
    // We therefore shell out to wh_fetch.py — a tiny wrapper around curl-cffi,
    // a Python binding to libcurl-impersonate (built on BoringSSL), which
    // reproduces Chrome's JA3 byte-for-byte. wh_fetch.py must live next to
    // worldserver.exe (the current working dir), and curl-cffi must be
    // installed via:  pip install curl-cffi
    //
    // If wh_fetch.py is unavailable we fall back to boost::beast + OpenSSL,
    // which will 403 on Wowhead's WAF but still works for non-WAF'd hosts and
    // gives the GM a clear install hint.
    //
    // Setup (once per server):
    //   1. pip install curl-cffi
    //   2. Copy wh_fetch.py next to worldserver.exe
    //   3. Make sure `python` (or python3/py) is on PATH
    static bool FetchWowheadNpcHtml(uint32 npcId, std::string& outHtml, std::string& outError)
    {
        outHtml.clear();
        outError.clear();

        std::string const url = "https://www.wowhead.com/npc=" + std::to_string(npcId);

        // ---- 1. Try wh_fetch.py via Python (curl-cffi, correct Chrome JA3) ----
        // Wowhead's CloudFront WAF blocks by TLS fingerprint (JA3). OpenSSL
        // (boost::asio::ssl) can't reproduce Chrome's JA3 because the TLS
        // extension order is hardcoded in its state machine. We therefore shell
        // out to wh_fetch.py, a tiny wrapper around curl-cffi (a Python binding
        // to libcurl-impersonate, which is built on BoringSSL and reproduces
        // Chrome's JA3 byte-for-byte).
        //
        // wh_fetch.py lives next to worldserver.exe (the current working dir).
        // curl-cffi must be installed:  pip install curl-cffi
        //
        // We try several Python interpreter names, since "python" vs "python3"
        // vs "py" varies by install.
        char const* pyBins[] = {
            "python",
            "python3",
            "py",
            "C:\\Python\\python.exe"
        };

        auto runPyFetcher = [&](char const* pyBin) -> bool {
            // Build:  <pyBin> wh_fetch.py "<url>"
            // wh_fetch.py prints the HTML body to stdout and a short error to
            // stderr. We DO NOT merge stderr into stdout (2>&1) — instead we
            // read stdout as the body and let stderr surface only on failure
            // (via the exit code + a generic message). This avoids polluting
            // the body with Python warnings.
            std::string cmd = std::string(pyBin) + " wh_fetch.py \"" + url + "\"";

            TC_LOG_INFO(LOG_CHAN, "[gm_fixer] spellwow: exec: {}", cmd);

            FILE* pipe =
#if defined(_WIN32) || defined(_WIN64)
                _popen(cmd.c_str(), "r");
#else
                popen(cmd.c_str(), "r");
#endif
            if (!pipe)
                return false; // try next interpreter

            std::string body;
            char rbuf[8192];
            size_t total = 0;
            while (true)
            {
                size_t n = std::fread(rbuf, 1, sizeof(rbuf), pipe);
                if (n == 0) break;
                total += n;
                if (total > 16 * 1024 * 1024) // 16 MB safety cap
                {
                    body = "<too-large>";
                    break;
                }
                body.append(rbuf, n);
            }
#if defined(_WIN32) || defined(_WIN64)
            int rc = _pclose(pipe);
#else
            int rc = pclose(pipe);
#endif

            // On Windows, "python" not found -> rc != 0 and body empty. Treat
            // that as "interpreter missing, try next" so we can fall through to
            // the next pyBin.
            if (rc != 0 && body.empty())
                return false;

            if (rc == 3)
            {
                // wh_fetch.py: curl-cffi not installed.
                outError = "wh_fetch.py: curl-cffi not installed. Run: pip install curl-cffi";
                return true;
            }
            if (rc == 4)
            {
                outError = "HTTP 403 (CloudFront WAF block even via curl-cffi)";
                return true;
            }
            if (rc == 5)
            {
                outError = "HTTP error from Wowhead (wh_fetch.py exit 5)";
                return true;
            }
            if (rc == 6 || body.empty())
            {
                outError = "empty response body";
                return true;
            }
            if (body == "<too-large>")
            {
                outError = "response too large (>16MB)";
                return true;
            }
            if (rc != 0)
            {
                outError = "wh_fetch.py exit " + std::to_string(rc);
                return true;
            }
            outHtml = body;
            return true;
        };

        for (char const* pyBin : pyBins)
        {
            outError.clear();
            if (runPyFetcher(pyBin))
            {
                if (!outHtml.empty())
                    return true;
                if (!outError.empty())
                    return false;
            }
        }

        // ---- 2. Fallback: boost::beast (will 403 on Wowhead, but keep for
        //         hosts without JA3-based WAF or for clear error reporting) ----
        {
            std::string const host = "www.wowhead.com";
            std::string const target = "/npc=" + std::to_string(npcId);
            try
            {
                boost::asio::io_context io;
                boost::asio::ssl::context sslCtx(boost::asio::ssl::context::tlsv12_client);
                sslCtx.set_verify_mode(boost::asio::ssl::verify_none);

                boost::asio::ip::tcp::resolver resolver(io);
                auto const results = resolver.resolve(host, "443");
                boost::asio::ssl::stream<boost::asio::ip::tcp::socket> sslStream(io, sslCtx);

                if (!SSL_set_tlsext_host_name(sslStream.native_handle(), host.c_str()))
                {
                    outError = "SNI setup failed";
                    return false;
                }

                boost::asio::connect(sslStream.next_layer(), results);
                sslStream.handshake(boost::asio::ssl::stream_base::client);

                boost::beast::http::request<boost::beast::http::string_body> req;
                req.method(boost::beast::http::verb::get);
                req.target(target);
                req.version(11);
                req.set(boost::beast::http::field::host, host);
                req.set(boost::beast::http::field::user_agent,
                    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
                    "(KHTML, like Gecko) Chrome/130.0.0.0 Safari/537.36");
                req.set(boost::beast::http::field::accept,
                    "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
                req.set("accept-language", "en-US,en;q=0.9");
                req.set(boost::beast::http::field::accept_encoding, "identity");
                req.set(boost::beast::http::field::connection, "close");
                req.prepare_payload();

                boost::beast::http::write(sslStream, req);
                boost::beast::flat_buffer buf;
                boost::beast::http::response<boost::beast::http::string_body> res;
                boost::system::error_code readEc;
                boost::beast::http::read(sslStream, buf, res, readEc);
                (void)readEc;
                boost::beast::error_code shutdownEc;
                sslStream.shutdown(shutdownEc);

                if (res.result_int() == 403 || res.result_int() == 429)
                {
                    outError = "HTTP 403 — Wowhead WAF blocked the request. "
                               "Make sure wh_fetch.py runs: put it next to "
                               "worldserver.exe and install curl-cffi:\n"
                               "  pip install curl-cffi";
                    return false;
                }
                if (res.result_int() != 200)
                {
                    outError = "HTTP " + std::to_string(res.result_int());
                    return false;
                }
                if (res.body().empty())
                {
                    outError = "empty response body";
                    return false;
                }
                outHtml = res.body();
                return true;
            }
            catch (std::exception const& e)
            {
                outError = std::string("exception: ") + e.what();
                return false;
            }
        }
    }

    // Balanced-brace matcher: returns the index of '}' matching the '{' at `start`.
    // Respects single/double quoted strings and backslash escapes.
    static size_t GmFixer_MatchBrace(std::string const& s, size_t start)
    {
        if (start >= s.size() || s[start] != '{')
            return std::string::npos;
        int depth = 0;
        bool inStr = false; char sc = 0; bool esc = false;
        for (size_t i = start; i < s.size(); ++i)
        {
            char c = s[i];
            if (inStr)
            {
                if (esc)               esc = false;
                else if (c == '\\')    esc = true;
                else if (c == sc)      inStr = false;
            }
            else
            {
                if (c == '"' || c == '\'') { inStr = true; sc = c; }
                else if (c == '{') depth++;
                else if (c == '}') { depth--; if (depth == 0) return i; }
            }
        }
        return std::string::npos;
    }

    // Extract a JSON string value for `key` from a small JSON object blob.
    // e.g. blob = {"name_enus":"Enrage","icon":"..."}, key="name_enus" -> "Enrage".
    // Handles \" \/ \n \t escapes.
    static std::string GmFixer_ExtractJsonString(std::string const& blob, std::string const& key)
    {
        std::string needle = "\"" + key + "\":\"";
        size_t k = blob.find(needle);
        if (k == std::string::npos)
            return "";
        size_t v = k + needle.size();
        std::string out;
        out.reserve(64);
        for (size_t i = v; i < blob.size(); ++i)
        {
            char c = blob[i];
            if (c == '\\' && i + 1 < blob.size())
            {
                char n = blob[i + 1];
                switch (n)
                {
                    case '"':  out += '"';  ++i; continue;
                    case '\\': out += '\\'; ++i; continue;
                    case '/':  out += '/';  ++i; continue;
                    case 'n':  out += '\n'; ++i; continue;
                    case 't':  out += '\t'; ++i; continue;
                    case 'r':  out += '\r'; ++i; continue;
                    default:   out += c;        continue;
                }
            }
            if (c == '"')
                break;
            out += c;
        }
        return out;
    }

    // Strip Wowhead tooltip HTML comments <!--...--> and any embedded HTML tags,
    // then collapse whitespace. Turns:
    //   "Increases the caster's attack speed by <!--pts1:0:0:38166-->50<!---->% for 30 sec."
    // into:
    //   "Increases the caster's attack speed by 50% for 30 sec."
    static std::string GmFixer_CleanTooltip(std::string const& s)
    {
        if (s.empty()) return "";
        std::string stripped;
        stripped.reserve(s.size());
        size_t i = 0;
        while (i < s.size())
        {
            // skip <!-- ... -->
            if (i + 3 < s.size() && s[i] == '<' && s[i+1] == '!' && s[i+2] == '-' && s[i+3] == '-')
            {
                size_t end = s.find("-->", i + 4);
                if (end == std::string::npos) break;
                i = end + 3;
                continue;
            }
            // skip <...>
            if (s[i] == '<')
            {
                size_t end = s.find('>', i);
                if (end == std::string::npos) break;
                i = end + 1;
                continue;
            }
            stripped += s[i++];
        }
        // collapse whitespace
        std::string out;
        out.reserve(stripped.size());
        bool lastSpace = false;
        for (char c : stripped)
        {
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
            {
                if (!lastSpace) { out += ' '; lastSpace = true; }
            }
            else
            {
                out += c;
                lastSpace = false;
            }
        }
        // trim
        size_t b = out.find_first_not_of(' ');
        if (b == std::string::npos) return "";
        size_t e = out.find_last_not_of(' ');
        return out.substr(b, e - b + 1);
    }

    // One parsed ability from Wowhead.
    struct GmFixer_WowheadAbility
    {
        uint32      spellId;
        std::string name;
        std::string description;
        std::string icon;
    };

    // Parse every WH.Gatherer.addData(6, <idType>, { ... }) block in the HTML.
    // Type 6 is Wowhead's "spell" Gatherer type — the blob is a clean JSON object
    // keyed by spell ID: { "38166": {"name_enus":"Enrage","icon":"...","description_enus":"..."}, ... }
    // This block is present even in the "lite" (non-JS-rendered) HTML we receive,
    // so it's our primary spell source.
    static void GmFixer_ParseGathererSpells(std::string const& html,
                                            std::vector<GmFixer_WowheadAbility>& out)
    {
        std::string const sig = "WH.Gatherer.addData(6,";
        size_t from = 0;
        while (true)
        {
            size_t p = html.find(sig, from);
            if (p == std::string::npos) break;

            size_t lb = html.find('{', p + sig.size());
            if (lb == std::string::npos) break;
            size_t rb = GmFixer_MatchBrace(html, lb);
            if (rb == std::string::npos) break;

            std::string blob = html.substr(lb, rb - lb + 1);

            // Walk the blob and pick up every "<all-digits>": { ... } pair.
            // We jump past each inner object, so inner quoted strings (names,
            // icons, descriptions) are never scanned by this loop.
            size_t i = 0;
            while (i < blob.size())
            {
                size_t q = blob.find('"', i);
                if (q == std::string::npos) break;
                size_t q2 = blob.find('"', q + 1);
                if (q2 == std::string::npos) break;

                std::string key = blob.substr(q + 1, q2 - q - 1);
                bool isNum = !key.empty();
                for (char c : key)
                    if (c < '0' || c > '9') { isNum = false; break; }

                if (!isNum)
                {
                    i = q2 + 1;
                    continue;
                }

                // Find the { that starts this spell's inner object.
                size_t ob = blob.find('{', q2);
                if (ob == std::string::npos) break;
                size_t oe = GmFixer_MatchBrace(blob, ob);
                if (oe == std::string::npos) break;

                std::string inner = blob.substr(ob, oe - ob + 1);

                GmFixer_WowheadAbility a;
                try { a.spellId = std::stoul(key); }
                catch (...) { i = oe + 1; continue; }
                a.name        = GmFixer_ExtractJsonString(inner, "name_enus");
                a.description = GmFixer_CleanTooltip(GmFixer_ExtractJsonString(inner, "description_enus"));
                a.icon        = GmFixer_ExtractJsonString(inner, "icon");
                out.push_back(std::move(a));

                i = oe + 1;
            }
            from = rb + 1;
        }
    }

    // Guess the smart_scripts target_type from the spell name + description.
    //   1 = SMART_TARGET_SELF    (self-buffs: Enrage, Frenzy, "the caster's"...)
    //   2 = SMART_TARGET_VICTIM  (default: debuffs, damage, attack spells)
    static uint8 GmFixer_GuessTargetType(std::string const& name, std::string const& desc)
    {
        std::string n = name, d = desc;
        std::transform(n.begin(), n.end(), n.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        std::transform(d.begin(), d.end(), d.begin(),
                       [](unsigned char c){ return std::tolower(c); });

        if (n.find("enrage")   != std::string::npos ||
            n.find("frenzy")   != std::string::npos ||
            n.find("berserk")  != std::string::npos ||
            n.find("bloodlust") != std::string::npos ||
            d.find("the caster's")    != std::string::npos ||
            d.find("increases the caster") != std::string::npos)
            return 1; // SELF

        return 2; // VICTIM
    }

    // Guess the smart_scripts event_type.
    //   0 = SMART_EVENT_UPDATE_IC   (in combat — the default for attack spells)
    //   2 = SMART_EVENT_UPDATE_OOC  (out of combat — for passive self-buffs)
    static uint8 GmFixer_GuessEventType(std::string const& name, std::string const& desc)
    {
        // Default: in-combat periodic. Could be extended for OOC buffs.
        (void)name; (void)desc;
        return 0; // UPDATE_IC
    }

    // ---- loot / pickpocketing parsing (.fix lootwow / pickpocketwow / wowinfo) ----

    // One parsed item from a Wowhead NPC drops / pickpocketing Listview.
    //   count  — how many times this item dropped (sampled on Wowhead)
    //   outof  — total kills sampled on Wowhead
    //   chance = count/outof*100
    struct GmFixer_WowheadItem
    {
        uint32      itemId;
        std::string name;
        uint32      count;
        uint32      outof;
        uint32      stackMin;
        uint32      stackMax;
        int32       classs; // item class; 12 == quest item -> QuestRequired
    };

    // Extract a uint32 value for `key` from a small JSON object blob.
    // e.g. blob has "count":23 -> returns 23. Missing key -> def.
    static uint32 GmFixer_ExtractJsonUInt(std::string const& blob, std::string const& key, uint32 def)
    {
        std::string needle = "\"" + key + "\":";
        size_t k = blob.find(needle);
        if (k == std::string::npos)
            return def;
        size_t v = k + needle.size();
        // skip optional sign
        bool neg = false;
        if (v < blob.size() && (blob[v] == '-' || blob[v] == '+'))
        {
            neg = (blob[v] == '-');
            ++v;
        }
        char* end = nullptr;
        unsigned long val = std::strtoul(blob.c_str() + v, &end, 10);
        if (end == blob.c_str() + v)
            return def;
        return neg ? 0u : uint32(val);
    }

    // Extract the two-element numeric array `key`: [min,max] from a JSON blob.
    static void GmFixer_ExtractJsonUIntPair(std::string const& blob, std::string const& key,
                                            uint32& outA, uint32& outB)
    {
        std::string needle = "\"" + key + "\":[";
        size_t k = blob.find(needle);
        if (k == std::string::npos) return;
        size_t v = k + needle.size();
        char* end = nullptr;
        unsigned long a = std::strtoul(blob.c_str() + v, &end, 10);
        if (end == blob.c_str() + v) return;
        outA = uint32(a);
        if (*end != ',') { outB = outA; return; }
        char* end2 = nullptr;
        unsigned long b = std::strtoul(end + 1, &end2, 10);
        if (end2 == end + 1) { outB = outA; return; }
        outB = uint32(b);
    }

    // Parse a Wowhead "new Listview({template:'item', id:'<listId>', ... data:[...]})"
    // block into a list of items. Used for both 'drops' (creature_loot_template)
    // and 'pickpocketing' (pickpocketing_loot_template).
    static void GmFixer_ParseListviewItems(std::string const& html, std::string const& listId,
                                           std::vector<GmFixer_WowheadItem>& out)
    {
        // Find the Listview call whose id matches listId.
        size_t from = 0;
        while (true)
        {
            size_t lv = html.find("new Listview(", from);
            if (lv == std::string::npos) break;
            from = lv + 1;

            // Bound this Listview call by matching its outer '(' ... ')'.
            // (Cheap scan: track paren depth, respecting strings.)
            size_t open = html.find('(', lv);
            if (open == std::string::npos) break;
            int depth = 0;
            bool inStr = false; char sc = 0; bool esc = false;
            size_t end = std::string::npos;
            for (size_t i = open; i < html.size(); ++i)
            {
                char c = html[i];
                if (inStr)
                {
                    if (esc) esc = false;
                    else if (c == '\\') esc = true;
                    else if (c == sc) inStr = false;
                }
                else
                {
                    if (c == '"' || c == '\'') { inStr = true; sc = c; }
                    else if (c == '(') depth++;
                    else if (c == ')') { depth--; if (depth == 0) { end = i; break; } }
                }
            }
            if (end == std::string::npos) break;
            std::string callBody = html.substr(open, end - open + 1);

            // Does this call match the wanted listId? ("id:'<listId>'")
            std::string idNeedle = "id:'" + listId + "'";
            if (callBody.find(idNeedle) == std::string::npos)
                continue;

            // Find the data:[ ... ] array inside this call.
            size_t dataPos = callBody.find("data:[");
            if (dataPos == std::string::npos) break;
            size_t arrStart = callBody.find('[', dataPos);
            if (arrStart == std::string::npos) break;

            // Match the balanced [...] array.
            int d = 0; bool ins = false; char qc = 0; bool es = false;
            size_t arrEnd = std::string::npos;
            for (size_t i = arrStart; i < callBody.size(); ++i)
            {
                char c = callBody[i];
                if (ins)
                {
                    if (es) es = false;
                    else if (c == '\\') es = true;
                    else if (c == qc) ins = false;
                }
                else
                {
                    if (c == '"' || c == '\'') { ins = true; qc = c; }
                    else if (c == '[') d++;
                    else if (c == ']') { d--; if (d == 0) { arrEnd = i; break; } }
                }
            }
            if (arrEnd == std::string::npos) break;

            // Walk the array body and pick up every {...} object.
            std::string arr = callBody.substr(arrStart + 1, arrEnd - arrStart - 1);
            size_t i = 0;
            while (i < arr.size())
            {
                size_t ob = arr.find('{', i);
                if (ob == std::string::npos) break;
                size_t oe = GmFixer_MatchBrace(arr, ob);
                if (oe == std::string::npos) break;
                std::string inner = arr.substr(ob, oe - ob + 1);

                GmFixer_WowheadItem it{};
                it.itemId = GmFixer_ExtractJsonUInt(inner, "id", 0);
                if (it.itemId != 0)
                {
                    it.name    = GmFixer_ExtractJsonString(inner, "name_enus");
                    if (it.name.empty())
                        it.name = GmFixer_ExtractJsonString(inner, "name");
                    it.count   = GmFixer_ExtractJsonUInt(inner, "count", 0);
                    it.outof   = GmFixer_ExtractJsonUInt(inner, "outof", 0);
                    it.classs  = int32(GmFixer_ExtractJsonUInt(inner, "classs", 0));
                    // stack:[min,max]
                    it.stackMin = 1; it.stackMax = 1;
                    GmFixer_ExtractJsonUIntPair(inner, "stack", it.stackMin, it.stackMax);
                    if (it.stackMin == 0) it.stackMin = 1;
                    if (it.stackMax < it.stackMin) it.stackMax = it.stackMin;
                    out.push_back(std::move(it));
                }
                i = oe + 1;
            }
            break; // only one matching Listview per page
        }
    }

    // Resolve the target creature entry for the wow*/loot*/pickpocket* commands:
    // explicit npcId if given, else the selected creature's entry.
    static bool GmFixer_ResolveNpcEntry(ChatHandler* h, Optional<uint32> const& npcIdOpt,
                                        uint32& outEntry, Creature*& outC)
    {
        if (npcIdOpt)
        {
            outEntry = *npcIdOpt;
            outC = nullptr;
            if (outEntry == 0)
            {
                h->SendSysMessage("[gm_fixer] npcId must be > 0.");
                return false;
            }
            return true;
        }
        // Fall back to the selected creature (also return the pointer so the
        // spellwow path can keep using SmartAI checks on the live object).
        return GetSelectedCreature(h, outC, outEntry);
    }

    // Fetch a Wowhead NPC page once, resolving the target entry first.
    // Returns true and fills outHtml on success; sends an error to the GM on
    // failure. Shared by .fix wowinfo / lootwow / pickpocketwow / wowall.
    static bool GmFixer_FetchWowheadAll(ChatHandler* h, Optional<uint32> const& npcIdOpt,
                                        uint32& outEntry, std::string& outHtml)
    {
        Creature* c = nullptr;
        if (!GmFixer_ResolveNpcEntry(h, npcIdOpt, outEntry, c))
            return false;

        if (npcIdOpt)
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] fetching Wowhead NPC {} ...", outEntry));
        else
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] no npcId given, using selected creature entry {} ...", outEntry));

        std::string err;
        if (!FetchWowheadNpcHtml(outEntry, outHtml, err))
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] Wowhead fetch failed: {}", err));
            h->SendSysMessage(Trinity::StringFormat(
                "  Open https://www.wowhead.com/npc={} in a browser to check manually.", outEntry));
            TC_LOG_INFO(LOG_CHAN, "[gm_fixer] fetch failed for NPC {}: {}", outEntry, err);
            return false;
        }
        h->SendSysMessage(Trinity::StringFormat(
            "[gm_fixer] fetched {} bytes from Wowhead.", outHtml.size()));
        return true;
    }

    // SQL-escape single quotes for a comment string.
    static std::string GmFixer_EscapeSqlComment(std::string const& s)
    {
        std::string out;
        out.reserve(s.size() + 4);
        for (char c : s)
        {
            if (c == '\'') out += "''";
            else out += c;
        }
        return out;
    }

    // Apply a full Wowhead drop list to a loot template table (DELETE + INSERT).
    //   entry     — loot Entry (= creature entry in this codebase's convention)
    //   tableName — "creature_loot_template" or "pickpocketing_loot_template"
    //   items     — parsed from Wowhead
    //   fixTag    — short tag for the SQL-log header + messages
    // Returns the number of rows inserted.
    static uint32 GmFixer_ApplyLoot(ChatHandler* h, uint32 entry, char const* tableName,
                                    std::vector<GmFixer_WowheadItem> const& items,
                                    char const* fixTag)
    {
        if (items.empty())
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] {}: no items parsed from Wowhead; nothing changed.", fixTag));
            return 0;
        }

        // Full replace: delete all rows for this entry, then insert the Wowhead set.
        std::string sqlDel = Trinity::StringFormat(
            "DELETE FROM `{}` WHERE `Entry` = {};", tableName, entry);

        // Build one multi-row INSERT.
        std::string sqlIns = Trinity::StringFormat(
            "INSERT INTO `{}` "
            "(`Entry`, `ItemType`, `Item`, `Chance`, `QuestRequired`, `LootMode`, "
            " `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES ", tableName);

        uint32 n = 0;
        for (size_t i = 0; i < items.size(); ++i)
        {
            GmFixer_WowheadItem const& it = items[i];
            // chance = count / outof * 100, rounded to 2 decimals.
            // outof==0 -> treat as guaranteed (100%). Quest items -> 100%.
            double chance = 100.0;
            if (it.classs != 12 && it.outof > 0)
                chance = (double)it.count / (double)it.outof * 100.0;
            // clamp tiny non-zero chances up to a floor so the row actually drops
            if (chance > 0.0 && chance < 0.01) chance = 0.01;
            int questReq = (it.classs == 12) ? 1 : 0;

            char chbuf[32];
            std::snprintf(chbuf, sizeof(chbuf), "%.2f", chance);

            sqlIns += Trinity::StringFormat(
                "({}, 0, {}, {}, {}, 1, 0, {}, {}, '{}')",
                entry, it.itemId, chbuf, questReq,
                it.stackMin, it.stackMax,
                GmFixer_EscapeSqlComment(it.name));
            sqlIns += (i + 1 < items.size()) ? ", " : ";";
            ++n;

            // Send a per-item line for visibility.
            h->SendSysMessage(Trinity::StringFormat(
                "  [{}] item {} {} {}% x{}-{}{}",
                fixTag, it.itemId, it.name, chbuf, it.stackMin, it.stackMax,
                questReq ? " (quest)" : ""));
        }

        // Execute (DirectExecute does not support multi-statement in one call).
        WorldDatabase.DirectExecute(sqlDel.c_str());
        WorldDatabase.DirectExecute(sqlIns.c_str());

        std::string fullSql = sqlDel + "\n" + sqlIns;
        std::string what = Trinity::StringFormat(
            "{}: {} item(s) -> {} Entry {} (DELETE+INSERT)",
            fixTag, n, tableName, entry);
        AppendFixLog(h, fullSql, fixTag, what);

        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage("  Apply in-game: .reload <table> (or restart)");
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, fullSql);
        return n;
    }
}

/*
 * =========================================================================
 * CommandScript
 * =========================================================================
 */
class gm_fixer_cmd : public CommandScript
{
public:
    gm_fixer_cmd() : CommandScript("gm_fixer_cmd") {}

    std::span<ChatCommandBuilder const> GetCommands() const override
    {
        static ChatCommandTable fixTable = {
            {"faction",    HandleFixFaction,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"friendly",   HandleFixFriendly,          rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"hostile",    HandleFixHostile,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"walk",       HandleFixWalk,              rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"fly",        HandleFixFly,               rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"delete",     HandleFixDelete,            rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"godel",      HandleFixGoDel,             rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"godelid",    HandleFixGoDelId,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"gophase",    HandleFixGoPhase,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"gophaseid",  HandleFixGoPhaseId,         rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"queststart", HandleFixQuestStart,        rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"ct",         HandleFixContentTuning,     rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"hp",         HandleFixHealth,            rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"hp1",        HandleFixHealth1,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"hp2",        HandleFixHealth2,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"hp3",        HandleFixHealth3,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"hp4",        HandleFixHealth4,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"hp5",        HandleFixHealth5,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"diff",       HandleFixDifficulty,        rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"diff1",      HandleFixDifficulty1,       rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"diff2",      HandleFixDifficulty2,       rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"diff3",      HandleFixDifficulty3,       rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"diff4",      HandleFixDifficulty4,       rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"diff5",      HandleFixDifficulty5,       rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"eflag",      HandleFixFlagsExtra,        rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"npcinfo",    HandleFixNpcInfo,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"npcfind",    HandleFixNpcFind,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"questinfo",  HandleFixQuestInfo,         rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"spellgcd",   HandleFixSpellGCD,          rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"spellcd",    HandleFixSpellCD,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"gstate",     HandleFixGState,            rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"cstate",     HandleFixCState,            rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"gdead",      HandleFixGDead,             rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"cdead",      HandleFixCDead,             rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"galive",     HandleFixGAlive,            rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"calive",     HandleFixCAlive,            rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"gnpcflag",   HandleFixGNpcFlag,          rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"cnpcflag",   HandleFixCNpcFlag,          rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"guflags",    HandleFixGUnitFlags,        rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"cuflags",    HandleFixCUnitFlags,        rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"guflags2",   HandleFixGUnitFlags2,       rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"cuflags2",   HandleFixCUnitFlags2,       rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"guflags3",   HandleFixGUnitFlags3,       rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"cuflags3",   HandleFixCUnitFlags3,       rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"combat",     HandleFixCombat,            rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"questprev",  HandleFixQuestPrev,         rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"addnpc",     HandleFixAddNpc,            rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"gmove",      HandleFixGMove,             rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"cmove",      HandleFixCMove,             rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"gmoverange", HandleFixGMoveRange,        rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"scalesize",  HandleFixScaleSize,         rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"gmoveall",   HandleFixGMoveAll,          rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"gpos",       HandleFixGPos,              rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"gemote",     HandleFixGEmote,            rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"emote",      HandleFixEmote,             rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"gauras",     HandleFixGAuras,            rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"smart",      HandleFixSmart,             rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"unsmart",    HandleFixUnsmart,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"wayinfo",    HandleFixWayInfo,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"wayadd",     HandleFixWayAdd,            rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"waynode",    HandleFixWayNode,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
            // Equipment (creature_equip_template):
            //   item1/2/3 [itemid]   — set full equip slot (ItemID, AppearanceModID, ItemVisual) at once
            //   itemid1/2/3 <ID>     — set ItemID for slot N only
            //   itemapp1/2/3 <ID>    — set AppearanceModID for slot N only
            //   itemvis1/2/3 <ID>    — set ItemVisual for slot N only
            //   iteminfo             — show current creature_equip_template rows for the entry
            {"item1",      HandleFixItemFull1,         rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"item2",      HandleFixItemFull2,         rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"item3",      HandleFixItemFull3,         rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"itemid1",    HandleFixItemId1,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"itemid2",    HandleFixItemId2,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"itemid3",    HandleFixItemId3,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"itemapp1",   HandleFixItemApp1,          rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"itemapp2",   HandleFixItemApp2,          rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"itemapp3",   HandleFixItemApp3,          rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"itemvis1",   HandleFixItemVis1,          rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"itemvis2",   HandleFixItemVis2,          rbac::RBAC_PERM_COMMAND_GM, Console::No},
            {"itemvis3",   HandleFixItemVis3,          rbac::RBAC_PERM_COMMAND_GM, Console::No},
             {"iteminfo",   HandleFixItemInfo,          rbac::RBAC_PERM_COMMAND_GM, Console::No},
             //   gequip <ID>           — set creature.equipment_id = ID for ALL spawns of the entry
             {"gequip",     HandleFixGEquip,            rbac::RBAC_PERM_COMMAND_GM, Console::No},
             //   cmount <ID>           — set mount display ID for the selected spawn (creature_addon)
             {"cmount",     HandleFixCmount,            rbac::RBAC_PERM_COMMAND_GM, Console::No},
             //   gmount <ID>           — set mount display ID for the template (creature_template_addon)
             {"gmount",     HandleFixGmount,            rbac::RBAC_PERM_COMMAND_GM, Console::No},
             //   gsheath <ID>          — set SheathState for the selected spawn (creature_addon)
             {"gsheath",    HandleFixGSheath,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
             //   csheath <ID>          — set SheathState for the template (creature_template_addon)
             {"csheath",    HandleFixCSheath,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
             //   pooladd               — gather ALL guids of the selected entry into a new pool (max_limit=1)
             {"pooladd",    HandleFixPoolAdd,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
             //   pooldel               — delete the pool of the selected entry (members + template)
             {"pooldel",    HandleFixPoolDel,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
             //   follow <leader> <follower> <range>  — follower follows leader (DISTANCE_CREATURE + FOLLOW smart line)
             {"follow",     HandleFixFollow,            rbac::RBAC_PERM_COMMAND_GM, Console::No},
             //   spellA <ID> [sec]     — periodic cast on victim (attack spell, in combat). sec default 6.
             {"spellA",     HandleFixSpellAttack,       rbac::RBAC_PERM_COMMAND_GM, Console::No},
             //   spellCB <ID> [sec]    — periodic self-cast buff (in combat). sec default 6.
             {"spellCB",    HandleFixSpellCombatBuff,   rbac::RBAC_PERM_COMMAND_GM, Console::No},
             //   spellB <ID> [sec]     — periodic self-cast buff (out of combat). sec default 6.
             {"spellB",     HandleFixSpellBuff,         rbac::RBAC_PERM_COMMAND_GM, Console::No},
             //   spellinfo             — list all SmartAI CAST lines of the selected NPC
             {"spellinfo",  HandleFixSpellInfo,         rbac::RBAC_PERM_COMMAND_GM, Console::No},
             //   spellparam <ID> <p1> <p2> <p3> <p4> — set event params of the selected
             //   NPC's CAST line(s) for spell ID (action_type=11, action_param1=ID)
             {"spellparam", HandleFixSpellParam,        rbac::RBAC_PERM_COMMAND_GM, Console::No},
             //   spellwow <wowheadNpcId> [repeatSec] [tag]
             //   — fetch NPC abilities from Wowhead, insert as smart_scripts CAST rows
             {"spellwow",  HandleFixSpellWow,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
             //   spellclear           — delete ALL CAST (action_type=11) smart_scripts
             //                         lines of the selected NPC
             {"spellclear", HandleFixSpellClear,        rbac::RBAC_PERM_COMMAND_GM, Console::No},
             //   spelldel <N>         — delete the Nth CAST line of the selected NPC
             //                         (N = 1-based position in .fix spellinfo)
             {"spelldel",  HandleFixSpellDel,           rbac::RBAC_PERM_COMMAND_GM, Console::No},
             //   spellcopy <donor> <recipient>
             //                        — copy every CAST line from donor entry to
             //                         recipient entry (new ids, skips duplicate spells)
             {"spellcopy", HandleFixSpellCopy,          rbac::RBAC_PERM_COMMAND_GM, Console::No},
             //   wowinfo [id]         — compare Wowhead spells/drops/pickpocket of the
             //                         given NPC (or selected creature) vs the DB (diff, read-only)
             {"wowinfo",       HandleFixWowInfo,        rbac::RBAC_PERM_COMMAND_GM, Console::No},
             //   lootwow [id]         — replace creature_loot_template with Wowhead drops
             //                         (DELETE + INSERT) for the given NPC / selected creature
             {"lootwow",       HandleFixLootWow,        rbac::RBAC_PERM_COMMAND_GM, Console::No},
             //   pickpocketwow [id]   — replace pickpocketing_loot_template with Wowhead
             //                         pickpocketing (DELETE + INSERT)
             {"pickpocketwow", HandleFixPickpocketWow,  rbac::RBAC_PERM_COMMAND_GM, Console::No},
             //   wowall [id]          — apply all three: spells + drops + pickpocketing
             {"wowall",        HandleFixWowAll,         rbac::RBAC_PERM_COMMAND_GM, Console::No},
        };
        static ChatCommandTable commandTable = {
            {"fix", fixTable},
        };
        return commandTable;
    }

private:
    // ----- .fix faction <factionId> -----
    static bool HandleFixFaction(ChatHandler* h, uint32 factionId)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint32 oldVal = 0;
        if (QueryResult r = WorldDatabase.PQuery("SELECT faction FROM creature_template WHERE entry = {}", entry))
            oldVal = r->Fetch()[0].GetUInt32();

        std::string sql = Trinity::StringFormat(
            "UPDATE creature_template SET faction = {} WHERE entry = {};", factionId, entry);
        return ApplyFix(h, sql, "faction",
            Trinity::StringFormat("faction: {} -> {} (entry {})", oldVal, factionId, entry).c_str());
    }

    // ----- .fix friendly -----
    static bool HandleFixFriendly(ChatHandler* h)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint32 oldVal = 0;
        if (QueryResult r = WorldDatabase.PQuery("SELECT faction FROM creature_template WHERE entry = {}", entry))
            oldVal = r->Fetch()[0].GetUInt32();

        std::string sql = Trinity::StringFormat(
            "UPDATE creature_template SET faction = {} WHERE entry = {};", FIX_FACTION_FRIENDLY, entry);
        return ApplyFix(h, sql, "friendly",
            Trinity::StringFormat("faction: {} -> {} (entry {}, friendly)", oldVal, FIX_FACTION_FRIENDLY, entry).c_str());
    }

    // ----- .fix hostile -----
    static bool HandleFixHostile(ChatHandler* h)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint32 oldVal = 0;
        if (QueryResult r = WorldDatabase.PQuery("SELECT faction FROM creature_template WHERE entry = {}", entry))
            oldVal = r->Fetch()[0].GetUInt32();

        std::string sql = Trinity::StringFormat(
            "UPDATE creature_template SET faction = {} WHERE entry = {};", FIX_FACTION_HOSTILE, entry);
        return ApplyFix(h, sql, "hostile",
            Trinity::StringFormat("faction: {} -> {} (entry {}, hostile)", oldVal, FIX_FACTION_HOSTILE, entry).c_str());
    }

    // ----- .fix walk -----
    static bool HandleFixWalk(ChatHandler* h)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        std::string sql = Trinity::StringFormat(
            "UPDATE creature_template_difficulty SET StaticFlags1 = {} "
            "WHERE Entry = {};", STATIC_FLAG_WALK, entry);
        return ApplyFix(h, sql, "walk",
            Trinity::StringFormat("Walk (StaticFlags1={}) entry {}", STATIC_FLAG_WALK, entry).c_str());
    }

    // ----- .fix fly -----
    static bool HandleFixFly(ChatHandler* h)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        std::string sql = Trinity::StringFormat(
            "UPDATE creature_template_difficulty SET StaticFlags1 = {} "
            "WHERE Entry = {};", STATIC_FLAG_FLY, entry);
        return ApplyFix(h, sql, "fly",
            Trinity::StringFormat("Fly (StaticFlags1={}) entry {}", STATIC_FLAG_FLY, entry).c_str());
    }

    // ----- .fix delete -----
    // Deletes the specific spawn of the selected NPC (entry + spawnId).
    // Despawn first, then SQL - otherwise GetSpawnId() may become invalid.
    static bool HandleFixDelete(ChatHandler* h)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint64 spawnId = c->GetSpawnId();
        if (spawnId == 0)
        {
            h->SendSysMessage("[gm_fixer] Selected creature has no SpawnId (probably a temporary summon).");
            return true;
        }

        c->DespawnOrUnsummon(0ms);

        std::string sql = Trinity::StringFormat(
            "DELETE FROM creature WHERE id = {} AND guid = {};", entry, spawnId);
        return ApplyFix(h, sql, "delete",
            Trinity::StringFormat("Delete spawn (entry {}, guid {})", entry, spawnId).c_str());
    }

    // ----- .fix godel -----
    // Deletes the gameobject the GM is currently "targeting" with the mouse.
    //
    // IMPORTANT: this fork has NO real gameobject target like for creatures —
    // players can only target Units via the standard target frame. The core's
    // convention for "the selected gameobject" (used by getSelectedObject when
    // no Unit is targeted, and by .gobject target) is the NEAREST gameobject on
    // the grid, via ChatHandler::GetNearbyGameObject(). We use exactly that —
    // stand next to / point the camera at the GO and run .fix godel.
    //
    // Uses GameObject::DeleteFromDB, the canonical TrinityCore path: it despawns
    // every live instance of the spawn across ALL maps (via DoForAllMapsWithMapId),
    // clears respawn timers, deletes the in-memory GameObjectData, and removes
    // the DB row. Returns true only if the spawn exists.
    //
    // Since DeleteFromDB already executes the DB DELETE, we only write a portable
    // SQL block to the daily/zone audit files (not re-execute it) — same
    // runtime/log separation as .fix pooladd / .fix pooldel.
    static bool HandleFixGoDel(ChatHandler* h)
    {
        // Find the nearest gameobject to the GM (the core's "selected GO").
        GameObject* go = h->GetNearbyGameObject();
        if (!go)
        {
            h->SendSysMessage("[gm_fixer] godel: no gameobject found nearby. Stand close to it and try again.");
            return true;
        }

        ObjectGuid::LowType spawnId = go->GetSpawnId();
        uint32 goEntry = go->GetEntry();

        // Canonical delete: despawns across all maps + removes the DB row.
        if (!GameObject::DeleteFromDB(spawnId))
        {
            // GetNearbyGameObject just returned it, so this is unexpected.
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] godel: DeleteFromDB failed for gameobject guid {}.", spawnId));
            return true;
        }

        // Log SQL for the audit files. DeleteFromDB already executed the DELETE,
        // so this block is written to the daily/zone files for replay only.
        std::string sql = Trinity::StringFormat(
            "DELETE FROM `gameobject` WHERE `guid` = {};", spawnId);

        std::string what = Trinity::StringFormat(
            "Delete gameobject (entry {}, guid {})", goEntry, spawnId);
        AppendFixLog(h, sql, "godel", what);

        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL: {}", sql));
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sql);
        return true;
    }

    // ----- .fix godelid <guid> -----
    // Deletes a gameobject spawn by its DB guid (the `guid` column of the
    // `gameobject` table). No selected object is required — the guid is passed
    // explicitly, so the command works even when the object is on a different
    // map than the GM, or is too far to resolve via the grid search.
    static bool HandleFixGoDelId(ChatHandler* h, uint32 guid)
    {
        ObjectGuid::LowType spawnId = guid;

        // Look up the spawn data for an informative message (entry + map).
        GameObjectData const* data = sObjectMgr->GetGameObjectData(spawnId);
        if (!data)
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] godelid: gameobject guid {} not found.", spawnId));
            return true;
        }
        uint32 goEntry = data->id;
        uint32 mapId   = data->mapId;

        // Canonical delete: despawns across all maps + removes the DB row.
        if (!GameObject::DeleteFromDB(spawnId))
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] godelid: DeleteFromDB failed for gameobject guid {}.", spawnId));
            return true;
        }

        std::string sql = Trinity::StringFormat(
            "DELETE FROM `gameobject` WHERE `guid` = {};", spawnId);

        std::string what = Trinity::StringFormat(
            "Delete gameobject (entry {}, guid {}, map {})", goEntry, spawnId, mapId);
        AppendFixLog(h, sql, "godelid", what);

        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL: {}", sql));
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sql);
        return true;
    }

    // Shared phase-set core for .fix gophase / .fix gophaseid.
    //
    //   go       — live object on the GM's map (may be nullptr for gophaseid when
    //              the spawn isn't loaded on this map; then only the DB is updated).
    //   phaseId  — single PhaseId to apply (phaseUseFlags=0, PhaseGroup=0).
    //   fixTag   — "gophase" or "gophaseid", for the SQL-log header + messages.
    //   spawnId  — when go == nullptr, the caller passes the explicit spawn guid.
    //   data     — when go == nullptr, the caller passes the looked-up spawn data
    //              (entry/map for the message). Ignored when go != nullptr.
    //
    // Modern phasing (this fork): no phaseMask. We set a single PhaseId.
    //
    // Runtime rephase (only when go != nullptr): reset the live PhaseShift to the
    // single PhaseId via PhasingHandler so the GO visually moves phase for the GM
    // immediately, and SetDBPhase(phaseId) so SaveToDB persists the right column.
    // We still also write a portable UPDATE to the DB and the audit files — that
    // covers spawns not loaded on the GM's map and gives a reviewable SQL log.
    static bool GmFixer_ApplyGoPhase(ChatHandler* h, GameObject* go, uint32 phaseId,
                                     char const* fixTag,
                                     ObjectGuid::LowType spawnId = 0,
                                     GameObjectData const* data = nullptr)
    {
        // Resolve entry / spawnId / map for the message regardless of the path.
        uint32 goEntry;
        uint32 mapId;
        if (go)
        {
            goEntry = go->GetEntry();
            spawnId = go->GetSpawnId();
            mapId   = go->GetMapId();
        }
        else
        {
            // gophaseid path with a non-loaded spawn — data must have been
            // resolved by the caller.
            goEntry = data->id;
            mapId   = data->mapId;
        }

        if (phaseId == 0)
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] {}: phaseId must be > 0 (use phaseId 169 for the default world phase).", fixTag));
            return true;
        }

        // --- Runtime rephase (only when the GO is live on this map) ---
        if (go)
        {
            // Read old phase for the message.
            uint32 oldPhaseId = 0;
            if (GameObjectData const* gd = sObjectMgr->GetGameObjectData(spawnId))
                oldPhaseId = gd->phaseId;

            // Rebuild the live phase shift to the single PhaseId.
            PhasingHandler::ResetPhaseShift(go);
            PhasingHandler::AddPhase(go, phaseId, true);
            // SetDBPhase(>0) makes SaveToDB write the new PhaseId column
            // (SaveToDB reads GetDBPhase(); see GameObject.cpp).
            go->SetDBPhase(int32(phaseId));
            go->SaveToDB();

            std::string sql = Trinity::StringFormat(
                "UPDATE `gameobject` SET `phaseUseFlags` = 0, `PhaseId` = {}, `PhaseGroup` = 0 WHERE `guid` = {};",
                phaseId, spawnId);
            std::string what = Trinity::StringFormat(
                "PhaseId: {} -> {} (gameobject entry {}, guid {}, map {})",
                oldPhaseId, phaseId, goEntry, spawnId, mapId);
            AppendFixLog(h, sql, fixTag, what);

            h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
            h->SendSysMessage(Trinity::StringFormat("  SQL: {}", sql));
            h->SendSysMessage("  Runtime phase applied; .respawn the GO if it does not update visually.");
            TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sql);
            return true;
        }

        // --- DB-only path (gophaseid, spawn not loaded on this map) ---
        // Read old values for the message.
        uint32 oldPhaseId = data->phaseId;

        std::string sql = Trinity::StringFormat(
            "UPDATE `gameobject` SET `phaseUseFlags` = 0, `PhaseId` = {}, `PhaseGroup` = 0 WHERE `guid` = {};",
            phaseId, spawnId);
        WorldDatabase.DirectExecute(sql.c_str());

        std::string what = Trinity::StringFormat(
            "PhaseId: {} -> {} (gameobject entry {}, guid {}, map {}, DB-only)",
            oldPhaseId, phaseId, goEntry, spawnId, mapId);
        AppendFixLog(h, sql, fixTag, what);

        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL: {}", sql));
        h->SendSysMessage("  GO not on this map — phase applies on next spawn / restart.");
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sql);
        return true;
    }

    // ----- .fix gophase <phaseId> -----
    // Sets the phase of the gameobject the GM is currently "targeting" with the
    // mouse (nearest gameobject — same convention as .fix godel; see its comment).
    //
    // Phasing in this fork is modern: phaseUseFlags + PhaseId + PhaseGroup
    // (there is NO phaseMask column on `gameobject`). This command sets a SINGLE
    // PhaseId and clears any group: phaseUseFlags=0, PhaseGroup=0.
    //
    // Runtime + DB: the live object's phase shift is rebuilt from scratch to
    // the single PhaseId (PhasingHandler::ResetPhaseShift + AddPhase) and
    // SetDBPhase(phaseId) is called so SaveToDB persists the new PhaseId column
    // reliably (SaveToDB reads GetDBPhase()). Then a portable UPDATE is written
    // to the audit files.
    static bool HandleFixGoPhase(ChatHandler* h, uint32 phaseId)
    {
        // Find the nearest gameobject to the GM.
        GameObject* go = h->GetNearbyGameObject();
        if (!go)
        {
            h->SendSysMessage("[gm_fixer] gophase: no gameobject found nearby. Stand close to it and try again.");
            return true;
        }
        return GmFixer_ApplyGoPhase(h, go, phaseId, /*fixTag*/ "gophase");
    }

    // ----- .fix gophaseid <go_guid> <phaseId> -----
    // Sets the phase of a gameobject by its DB guid (the `guid` column of the
    // `gameobject` table). No selected object required — works even when the
    // object is on a different map than the GM, or not currently spawned on the
    // GM's map. If the object is live on the GM's map, its phase shift is also
    // updated at runtime; otherwise only the DB row changes (effective on next
    // spawn / restart).
    static bool HandleFixGoPhaseId(ChatHandler* h, uint32 guid, uint32 phaseId)
    {
        ObjectGuid::LowType spawnId = guid;

        // Confirm the spawn exists in memory (and read entry/map for the message).
        GameObjectData const* data = sObjectMgr->GetGameObjectData(spawnId);
        if (!data)
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] gophaseid: gameobject guid {} not found.", spawnId));
            return true;
        }

        // Try to grab the live object on the GM's current map for runtime rephase.
        GameObject* go = h->GetObjectFromPlayerMapByDbGuid(spawnId);
        return GmFixer_ApplyGoPhase(h, go, phaseId, /*fixTag*/ "gophaseid", spawnId, data);
    }

    // ----- .fix queststart <questId> -----
    static bool HandleFixQuestStart(ChatHandler* h, uint32 questId)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // Check that the quest exists - otherwise we'd get garbage in the DB.
        if (!sObjectMgr->GetQuestTemplate(questId))
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] Quest {} not found in quest_template.", questId));
            return true;
        }

        // INSERT IGNORE - in case of a duplicate (id, quest) PK.
        std::string sql = Trinity::StringFormat(
            "INSERT IGNORE INTO creature_queststarter (id, quest, VerifiedBuild) "
            "VALUES ({}, {}, 0);", entry, questId);
        return ApplyFix(h, sql, "queststart",
            Trinity::StringFormat("QuestStart: quest {} -> entry {}", questId, entry).c_str());
    }

    // ----- .fix ct <ContentTuningId> [rowIndex] -----
    // Updates ContentTuningID for a row in creature_template_difficulty.
    //
    // Command forms:
    //   .fix ct <ID>       - updates the FIRST row in order
    //   .fix ct <ID> <N>   - updates the Nth row in order (1-indexed)
    //                        If the Nth row doesn't exist - INSERT with that DifficultyID.
    //
    // If no rows exist at all - INSERT with standard StefalWoW defaults.
    static bool HandleFixContentTuning(ChatHandler* h, uint32 ctId, Optional<uint32> rowIndexOpt)
    {
        if (ctId == 0)
        {
            h->SendSysMessage("Usage: .fix ct <ContentTuningId> [rowIndex]");
            return true;
        }

        uint32 rowIndex = 0;  // 0 = first row (default)
        if (rowIndexOpt)
        {
            if (*rowIndexOpt == 0)
            {
                h->SendSysMessage("[gm_fixer] rowIndex must be >= 1");
                return true;
            }
            rowIndex = *rowIndexOpt - 1;  // convert to 0-indexed
        }

        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // Read all rows ordered by ascending DifficultyID.
        QueryResult fullRes = WorldDatabase.PQuery(
            "SELECT DifficultyID FROM creature_template_difficulty "
            "WHERE Entry = {} ORDER BY DifficultyID", entry);

        std::vector<uint32> existingDiffs;
        if (fullRes)
        {
            do
            {
                existingDiffs.push_back(fullRes->Fetch()[0].GetUInt32());
            } while (fullRes->NextRow());
        }

        std::string sql;
        std::string what;

        if (existingDiffs.empty())
        {
            // No rows at all - INSERT a new row with DifficultyID = 0
            // (or with DifficultyID = rowIndex if the Nth is needed)
            uint32 newDiffId = (rowIndex == 0) ? 0 : rowIndex;
            sql = Trinity::StringFormat(
                "INSERT INTO creature_template_difficulty "
                "(Entry, DifficultyID, LevelScalingDeltaMin, LevelScalingDeltaMax, "
                " ContentTuningID, HealthScalingExpansion, HealthModifier, ManaModifier, "
                " ArmorModifier, DamageModifier, CreatureDifficultyID, TypeFlags, TypeFlags2, "
                " TypeFlags3, LootID, PickPocketLootID, SkinLootID, GoldMin, GoldMax, "
                " StaticFlags1, StaticFlags2, StaticFlags3, StaticFlags4, StaticFlags5, "
                " StaticFlags6, StaticFlags7, StaticFlags8, VerifiedBuild) "
                "VALUES ({}, {}, 0, 0, {}, 0, 1, 1, 1, 1, 0, 0, 0, 0, {}, {}, 0, 167, 167, "
                " 268435456, 0, 0, 0, 0, 0, 0, 0, 61609);",
                entry, newDiffId, ctId, entry, entry);
            what = Trinity::StringFormat(
                "ContentTuning -> {} (entry {}, INSERT new row DifficultyID={})",
                ctId, entry, newDiffId);
        }
        else if (rowIndex < existingDiffs.size())
        {
            // Nth row exists - UPDATE by exact PK (Entry, oldDifficultyID).
            uint32 oldDiffId = existingDiffs[rowIndex];
            sql = Trinity::StringFormat(
                "UPDATE creature_template_difficulty SET ContentTuningID = {} "
                "WHERE Entry = {} AND DifficultyID = {};",
                ctId, entry, oldDiffId);
            what = Trinity::StringFormat(
                "ContentTuning -> {} (entry {}, UPDATE row {}/DifficultyID={})",
                ctId, entry, rowIndex + 1, oldDiffId);
        }
        else
        {
            // Nth row doesn't exist - INSERT with a new DifficultyID.
            // Use max existing +1 as the new DifficultyID
            // (guarantees PK uniqueness).
            uint32 maxDiff = 0;
            for (uint32 d : existingDiffs)
                if (d > maxDiff) maxDiff = d;
            uint32 newDiffId = maxDiff + 1;
            sql = Trinity::StringFormat(
                "INSERT INTO creature_template_difficulty "
                "(Entry, DifficultyID, LevelScalingDeltaMin, LevelScalingDeltaMax, "
                " ContentTuningID, HealthScalingExpansion, HealthModifier, ManaModifier, "
                " ArmorModifier, DamageModifier, CreatureDifficultyID, TypeFlags, TypeFlags2, "
                " TypeFlags3, LootID, PickPocketLootID, SkinLootID, GoldMin, GoldMax, "
                " StaticFlags1, StaticFlags2, StaticFlags3, StaticFlags4, StaticFlags5, "
                " StaticFlags6, StaticFlags7, StaticFlags8, VerifiedBuild) "
                "VALUES ({}, {}, 0, 0, {}, 0, 1, 1, 1, 1, 0, 0, 0, 0, {}, {}, 0, 167, 167, "
                " 268435456, 0, 0, 0, 0, 0, 0, 0, 61609);",
                entry, newDiffId, ctId, entry, entry);
            what = Trinity::StringFormat(
                "ContentTuning -> {} (entry {}, INSERT new row DifficultyID={})",
                ctId, entry, newDiffId);
        }
        return ApplyFix(h, sql, "ct", what.c_str());
    }

    // ----- .fix hp <HealthModifier> -----
    // Changes HealthModifier on ALL rows in creature_template_difficulty for the entry.
    // For a single difficulty row use .fix hp<N> (same ordering as .fix diff<N>).
    static bool HandleFixHealth(ChatHandler* h, double hpMod)
    {
        if (hpMod <= 0.0)
        {
            h->SendSysMessage("[gm_fixer] HealthModifier must be > 0.");
            return true;
        }

        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        QueryResult res = WorldDatabase.PQuery(
            "SELECT COUNT(*) FROM creature_template_difficulty WHERE Entry = {}", entry);

        uint32 rowCount = 0;
        if (res)
            rowCount = res->Fetch()[0].GetUInt32();

        std::string sql;
        std::string what;
        if (rowCount > 0)
        {
            sql = Trinity::StringFormat(
                "UPDATE creature_template_difficulty SET HealthModifier = {} "
                "WHERE Entry = {};", hpMod, entry);
            what = Trinity::StringFormat(
                "HealthModifier -> {} (entry {}, UPDATE all {} row(s))",
                hpMod, entry, rowCount);
        }
        else
        {
            sql = Trinity::StringFormat(
                "INSERT INTO creature_template_difficulty "
                "(Entry, DifficultyID, LevelScalingDeltaMin, LevelScalingDeltaMax, "
                " ContentTuningID, HealthScalingExpansion, HealthModifier, ManaModifier, "
                " ArmorModifier, DamageModifier, CreatureDifficultyID, TypeFlags, TypeFlags2, "
                " TypeFlags3, LootID, PickPocketLootID, SkinLootID, GoldMin, GoldMax, "
                " StaticFlags1, StaticFlags2, StaticFlags3, StaticFlags4, StaticFlags5, "
                " StaticFlags6, StaticFlags7, StaticFlags8, VerifiedBuild) "
                "VALUES ({}, 0, 0, 0, 0, 0, {}, 1, 1, 1, 0, 0, 0, 0, {}, {}, 0, 167, 167, "
                " 268435456, 0, 0, 0, 0, 0, 0, 0, 61609);",
                entry, hpMod, entry, entry);
            what = Trinity::StringFormat(
                "HealthModifier -> {} (entry {}, INSERT)", hpMod, entry);
        }
        return ApplyFix(h, sql, "hp", what.c_str());
    }

    // ----- .fix hp<N> <HealthModifier>  (N = 1..5) -----
    // Updates HealthModifier on the Nth row in order in creature_template_difficulty.
    // If the Nth row doesn't exist - INSERT a new one (DifficultyID chosen like .fix ct).
    static bool ApplyFixHealthAt(ChatHandler* h, double hpMod, uint32 rowIndex, char const* fixTag)
    {
        if (hpMod <= 0.0)
        {
            h->SendSysMessage("[gm_fixer] HealthModifier must be > 0.");
            return true;
        }

        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        QueryResult fullRes = WorldDatabase.PQuery(
            "SELECT DifficultyID FROM creature_template_difficulty "
            "WHERE Entry = {} ORDER BY DifficultyID", entry);

        std::vector<uint32> existingDiffs;
        if (fullRes)
        {
            do
            {
                existingDiffs.push_back(fullRes->Fetch()[0].GetUInt32());
            } while (fullRes->NextRow());
        }

        std::string sql;
        std::string what;

        if (rowIndex < existingDiffs.size())
        {
            uint32 oldDiffId = existingDiffs[rowIndex];
            sql = Trinity::StringFormat(
                "UPDATE creature_template_difficulty SET HealthModifier = {} "
                "WHERE Entry = {} AND DifficultyID = {};",
                hpMod, entry, oldDiffId);
            what = Trinity::StringFormat(
                "HealthModifier row {} -> {} (entry {}, UPDATE DifficultyID={})",
                rowIndex + 1, hpMod, entry, oldDiffId);
        }
        else
        {
            uint32 newDiffId;
            if (existingDiffs.empty())
                newDiffId = (rowIndex == 0) ? 0 : rowIndex;
            else
            {
                uint32 maxDiff = 0;
                for (uint32 d : existingDiffs)
                    if (d > maxDiff) maxDiff = d;
                newDiffId = maxDiff + 1;
            }

            sql = Trinity::StringFormat(
                "INSERT INTO creature_template_difficulty "
                "(Entry, DifficultyID, LevelScalingDeltaMin, LevelScalingDeltaMax, "
                " ContentTuningID, HealthScalingExpansion, HealthModifier, ManaModifier, "
                " ArmorModifier, DamageModifier, CreatureDifficultyID, TypeFlags, TypeFlags2, "
                " TypeFlags3, LootID, PickPocketLootID, SkinLootID, GoldMin, GoldMax, "
                " StaticFlags1, StaticFlags2, StaticFlags3, StaticFlags4, StaticFlags5, "
                " StaticFlags6, StaticFlags7, StaticFlags8, VerifiedBuild) "
                "VALUES ({}, {}, 0, 0, 0, 0, {}, 1, 1, 1, 1, 0, 0, 0, 0, {}, {}, 0, 167, 167, "
                " 268435456, 0, 0, 0, 0, 0, 0, 0, 61609);",
                entry, newDiffId, hpMod, entry, entry);
            what = Trinity::StringFormat(
                "HealthModifier row {} -> {} (entry {}, INSERT DifficultyID={})",
                rowIndex + 1, hpMod, entry, newDiffId);
        }
        return ApplyFix(h, sql, fixTag, what.c_str());
    }

    static bool HandleFixHealth1(ChatHandler* h, double hpMod) { return ApplyFixHealthAt(h, hpMod, 0, "hp1"); }
    static bool HandleFixHealth2(ChatHandler* h, double hpMod) { return ApplyFixHealthAt(h, hpMod, 1, "hp2"); }
    static bool HandleFixHealth3(ChatHandler* h, double hpMod) { return ApplyFixHealthAt(h, hpMod, 2, "hp3"); }
    static bool HandleFixHealth4(ChatHandler* h, double hpMod) { return ApplyFixHealthAt(h, hpMod, 3, "hp4"); }
    static bool HandleFixHealth5(ChatHandler* h, double hpMod) { return ApplyFixHealthAt(h, hpMod, 4, "hp5"); }

    // ----- .fix eflag <flags_extra> -----
    // Changes the creature's flags_extra in creature_template.
    // This is a bitmask of extra flags (corpse despawning, ignore aggro, etc.).
    static bool HandleFixFlagsExtra(ChatHandler* h, uint32 flags)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // Read old value
        uint32 oldVal = 0;
        if (QueryResult r = WorldDatabase.PQuery("SELECT flags_extra FROM creature_template WHERE entry = {}", entry))
            oldVal = r->Fetch()[0].GetUInt32();

        std::string sql = Trinity::StringFormat(
            "UPDATE creature_template SET flags_extra = {} WHERE entry = {};",
            flags, entry);
        return ApplyFix(h, sql, "eflag",
            Trinity::StringFormat("flags_extra: {} -> {} (entry {})", oldVal, flags, entry).c_str());
    }

    // ----- .fix diff <DifficultyID> [<DifficultyID>...] -----
    // Changes the creature's DifficultyID in creature_template_difficulty.
    //
    // (Entry, DifficultyID) is a composite PK, so a simple UPDATE of all rows
    // won't work - it would cause a PK conflict.
    //
    // Two modes:
    //   .fix diff X           - updates only the FIRST row in order
    //                            (by ascending current DifficultyID).
    //                            Simple UPDATE by exact PK.
    //   .fix diff X Y Z       - updates the first N rows in order:
    //                            1st to X, 2nd to Y, 3rd to Z.
    //                            Safe via DELETE + INSERT in a transaction.
    //
    // If the number of arguments > number of rows - error.
    // If the number of arguments < number of rows - remaining rows
    // keep their current DifficultyID (they are rewritten as-is).
    static bool HandleFixDifficulty(ChatHandler* h, std::vector<uint32> newIds)
    {
        if (newIds.empty())
        {
            h->SendSysMessage("Usage: .fix diff <DifficultyID> [<DifficultyID>...]");
            return true;
        }

        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // Read all current rows ordered by ascending DifficultyID - all columns,
        // so we can correctly restore the data later (for multi-mode).
        QueryResult fullRes = WorldDatabase.PQuery(
            "SELECT DifficultyID, LevelScalingDeltaMin, LevelScalingDeltaMax, "
            "       ContentTuningID, HealthScalingExpansion, HealthModifier, ManaModifier, "
            "       ArmorModifier, DamageModifier, CreatureDifficultyID, TypeFlags, TypeFlags2, "
            "       TypeFlags3, LootID, PickPocketLootID, SkinLootID, GoldMin, GoldMax, "
            "       StaticFlags1, StaticFlags2, StaticFlags3, StaticFlags4, StaticFlags5, "
            "       StaticFlags6, StaticFlags7, StaticFlags8, VerifiedBuild "
            "FROM creature_template_difficulty WHERE Entry = {} ORDER BY DifficultyID",
            entry);

        if (!fullRes)
        {
            h->SendSysMessage("[gm_fixer] No row in creature_template_difficulty. Use .fix ct <id> to create.");
            return true;
        }

        // Collect all rows into a vector.
        struct DiffRow
        {
            uint32 diffId;
            int32  lvlMin, lvlMax;
            uint32 ctId;
            int32  hpScaleExp;
            float  hpMod, manaMod, armorMod, dmgMod;
            uint32 cdid, tflg1, tflg2, tflg3;
            uint32 lootId, ppLootId, skinLootId;
            uint32 goldMin, goldMax;
            uint32 sf1, sf2, sf3, sf4, sf5, sf6, sf7, sf8;
            int32  verified;
        };
        std::vector<DiffRow> rows;
        do
        {
            Field* f = fullRes->Fetch();
            DiffRow r;
            r.diffId = f[0].GetUInt32();
            r.lvlMin = f[1].GetInt32();
            r.lvlMax = f[2].GetInt32();
            r.ctId = f[3].GetUInt32();
            r.hpScaleExp = f[4].GetInt32();
            r.hpMod = f[5].GetFloat();
            r.manaMod = f[6].GetFloat();
            r.armorMod = f[7].GetFloat();
            r.dmgMod = f[8].GetFloat();
            r.cdid = f[9].GetUInt32();
            r.tflg1 = f[10].GetUInt32();
            r.tflg2 = f[11].GetUInt32();
            r.tflg3 = f[12].GetUInt32();
            r.lootId = f[13].GetUInt32();
            r.ppLootId = f[14].GetUInt32();
            r.skinLootId = f[15].GetUInt32();
            r.goldMin = f[16].GetUInt32();
            r.goldMax = f[17].GetUInt32();
            r.sf1 = f[18].GetUInt32();
            r.sf2 = f[19].GetUInt32();
            r.sf3 = f[20].GetUInt32();
            r.sf4 = f[21].GetUInt32();
            r.sf5 = f[22].GetUInt32();
            r.sf6 = f[23].GetUInt32();
            r.sf7 = f[24].GetUInt32();
            r.sf8 = f[25].GetUInt32();
            r.verified = f[26].GetInt32();
            rows.push_back(r);
        } while (fullRes->NextRow());

        // Validation: arguments must not exceed the number of rows.
        if (newIds.size() > rows.size())
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] {} rows exist, but {} IDs given. Use .fix npcinfo to inspect.",
                rows.size(), newIds.size()));
            return true;
        }

        // ---- Single mode: one argument, UPDATE first row ----
        if (newIds.size() == 1 && rows.size() == 1)
        {
            std::string sql = Trinity::StringFormat(
                "UPDATE creature_template_difficulty SET DifficultyID = {} "
                "WHERE Entry = {};", newIds[0], entry);
            return ApplyFix(h, sql, "diff",
                Trinity::StringFormat(
                    "DifficultyID -> {} (entry {}, single UPDATE)", newIds[0], entry).c_str());
        }
        if (newIds.size() == 1)
        {
            // Multiple rows, but only updating the first one.
            std::string sql = Trinity::StringFormat(
                "UPDATE creature_template_difficulty SET DifficultyID = {} "
                "WHERE Entry = {} AND DifficultyID = {};",
                newIds[0], entry, rows[0].diffId);
            return ApplyFix(h, sql, "diff",
                Trinity::StringFormat(
                    "DifficultyID -> {} (entry {}, first row {} updated)",
                    newIds[0], entry, rows[0].diffId).c_str());
        }

        // ---- Multi-mode: multiple arguments. Safe DELETE + INSERT ----
        // Apply new DifficultyIDs to the first newIds.size() rows,
        // the remaining rows are rewritten as-is.
        //
        // Use a transaction so data isn't lost if INSERT fails.
        std::string sqlDelete = Trinity::StringFormat(
            "DELETE FROM creature_template_difficulty WHERE Entry = {};", entry);

        std::string sqlInsert =
            "INSERT INTO creature_template_difficulty "
            "(Entry, DifficultyID, LevelScalingDeltaMin, LevelScalingDeltaMax, "
            " ContentTuningID, HealthScalingExpansion, HealthModifier, ManaModifier, "
            " ArmorModifier, DamageModifier, CreatureDifficultyID, TypeFlags, TypeFlags2, "
            " TypeFlags3, LootID, PickPocketLootID, SkinLootID, GoldMin, GoldMax, "
            " StaticFlags1, StaticFlags2, StaticFlags3, StaticFlags4, StaticFlags5, "
            " StaticFlags6, StaticFlags7, StaticFlags8, VerifiedBuild) VALUES ";

        for (size_t i = 0; i < rows.size(); ++i)
        {
            // For the first newIds.size() rows take the new DifficultyID,
            // for the rest - keep the old one.
            uint32 newDiff = (i < newIds.size()) ? newIds[i] : rows[i].diffId;
            DiffRow const& r = rows[i];
            sqlInsert += Trinity::StringFormat(
                "({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {})",
                entry, newDiff,
                r.lvlMin, r.lvlMax, r.ctId, r.hpScaleExp,
                r.hpMod, r.manaMod, r.armorMod, r.dmgMod,
                r.cdid, r.tflg1, r.tflg2, r.tflg3,
                r.lootId, r.ppLootId, r.skinLootId,
                r.goldMin, r.goldMax,
                r.sf1, r.sf2, r.sf3, r.sf4, r.sf5, r.sf6, r.sf7, r.sf8,
                r.verified);
            if (i + 1 < rows.size())
                sqlInsert += ", ";
        }
        sqlInsert += ";";

        // Full SQL block for writing to the daily file and log.
        std::string fullSql = sqlDelete + "\n" + sqlInsert;

        // Execute atomically via transaction.
        auto trans = WorldDatabase.BeginTransaction();
        trans->Append(sqlDelete.c_str());
        trans->Append(sqlInsert.c_str());
        WorldDatabase.CommitTransaction(trans);

        // Append to the daily file (as a single block).
        AppendFixLog(h, fullSql, "diff",
            Trinity::StringFormat("entry {}, {} rows", entry, rows.size()));

        std::string what = Trinity::StringFormat(
            "DifficultyID multi-update (entry {}, {} rows)", entry, rows.size());
        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL:\n{}", fullSql));
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, fullSql);
        return true;
    }

    // ----- .fix diff<N> <DifficultyID>  (N = 1..5) -----
    // Updates the Nth row in order in creature_template_difficulty.
    // If the Nth row doesn't exist - INSERT with the new DifficultyID.
    //
    // Example: the table has rows with DifficultyID=2 and DifficultyID=4.
    //   .fix diff2 10  =>  the second row (DifficultyID=4) changes to 10.
    //   .fix diff3 10  =>  no third row, INSERT a new one with DifficultyID=10.
    //
    // Implementation: a common ApplyFixDifficultyAt function + 5 thin wrappers,
    // because the old ChatCommand API requires separate functions for different
    // command names (can't pass a parameter).
    static bool ApplyFixDifficultyAt(ChatHandler* h, uint32 newDiffId, uint32 rowIndex)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // Read all rows ordered by ascending DifficultyID.
        QueryResult fullRes = WorldDatabase.PQuery(
            "SELECT DifficultyID FROM creature_template_difficulty "
            "WHERE Entry = {} ORDER BY DifficultyID", entry);

        std::vector<uint32> existingDiffs;
        if (fullRes)
        {
            do
            {
                existingDiffs.push_back(fullRes->Fetch()[0].GetUInt32());
            } while (fullRes->NextRow());
        }

        std::string sql;
        std::string what;

        if (rowIndex < existingDiffs.size())
        {
            // Nth row exists - UPDATE by exact PK.
            // Change DifficultyID to newDiffId.
            uint32 oldDiffId = existingDiffs[rowIndex];
            sql = Trinity::StringFormat(
                "UPDATE creature_template_difficulty SET DifficultyID = {} "
                "WHERE Entry = {} AND DifficultyID = {};",
                newDiffId, entry, oldDiffId);
            what = Trinity::StringFormat(
                "DifficultyID row {} -> {} (entry {}, UPDATE old={})",
                rowIndex + 1, newDiffId, entry, oldDiffId);
        }
        else
        {
            // Nth row doesn't exist - INSERT with newDiffId as DifficultyID.
            sql = Trinity::StringFormat(
                "INSERT INTO creature_template_difficulty "
                "(Entry, DifficultyID, LevelScalingDeltaMin, LevelScalingDeltaMax, "
                " ContentTuningID, HealthScalingExpansion, HealthModifier, ManaModifier, "
                " ArmorModifier, DamageModifier, CreatureDifficultyID, TypeFlags, TypeFlags2, "
                " TypeFlags3, LootID, PickPocketLootID, SkinLootID, GoldMin, GoldMax, "
                " StaticFlags1, StaticFlags2, StaticFlags3, StaticFlags4, StaticFlags5, "
                " StaticFlags6, StaticFlags7, StaticFlags8, VerifiedBuild) "
                "VALUES ({}, {}, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, {}, {}, 0, 167, 167, "
                " 268435456, 0, 0, 0, 0, 0, 0, 0, 61609);",
                entry, newDiffId, entry, entry);
            what = Trinity::StringFormat(
                "DifficultyID row {} -> {} (entry {}, INSERT)",
                rowIndex + 1, newDiffId, entry);
        }
        return ApplyFix(h, sql, "diff", what.c_str());
    }

    static bool HandleFixDifficulty1(ChatHandler* h, uint32 newDiffId) { return ApplyFixDifficultyAt(h, newDiffId, 0); }
    static bool HandleFixDifficulty2(ChatHandler* h, uint32 newDiffId) { return ApplyFixDifficultyAt(h, newDiffId, 1); }
    static bool HandleFixDifficulty3(ChatHandler* h, uint32 newDiffId) { return ApplyFixDifficultyAt(h, newDiffId, 2); }
    static bool HandleFixDifficulty4(ChatHandler* h, uint32 newDiffId) { return ApplyFixDifficultyAt(h, newDiffId, 3); }
    static bool HandleFixDifficulty5(ChatHandler* h, uint32 newDiffId) { return ApplyFixDifficultyAt(h, newDiffId, 4); }

    // ----- .fix gstate <StandState> -----
    // Changes StandState in creature_addon for a specific spawn (by SpawnId).
    // creature_addon stores per-spawn data, guid = spawn GUID.
    //
    // If no row exists - INSERT a new one with guid + StandState (remaining columns
    // take DEFAULT from the table schema).
    //
    // SQL:
    //   UPDATE: UPDATE creature_addon SET StandState = X WHERE guid = <spawnId>;
    //   INSERT: INSERT INTO creature_addon (guid, StandState) VALUES (<spawnId>, X);
    //
    // Common StandState values:
    //   0 = STAND (standing)
    //   1 = SIT (sitting on the ground)
    //   2 = SIT_CHAIR (in a chair)
    //   3 = SLEEP (sleeping)
    //   4 = SIT_LOW_CHAIR (low chair)
    //   5 = DEAD (dead)
    //   6 = KNEEL (kneeling)
    //   7 = SUBMERGED (underwater)
    static bool HandleFixGState(ChatHandler* h, uint32 state)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint64 spawnId = c->GetSpawnId();
        if (spawnId == 0)
        {
            h->SendSysMessage("[gm_fixer] Selected creature has no SpawnId (probably a temporary summon).");
            return true;
        }

        // Check if a row exists in creature_addon.
        QueryResult res = WorldDatabase.PQuery(
            "SELECT 1 FROM creature_addon WHERE guid = {} LIMIT 1", spawnId);

        std::string sql;
        std::string what;
        if (res)
        {
            // Row exists - UPDATE.
            sql = Trinity::StringFormat(
                "UPDATE creature_addon SET StandState = {} WHERE guid = {};",
                state, spawnId);
            what = Trinity::StringFormat(
                "StandState (creature_addon) -> {} (spawn guid {}, UPDATE)",
                state, spawnId);
        }
        else
        {
            // No row - INSERT with guid + StandState.
            // Remaining columns take DEFAULT from the schema (mount=0, emote=0, etc.).
            sql = Trinity::StringFormat(
                "INSERT INTO creature_addon (guid, StandState) VALUES ({}, {});",
                spawnId, state);
            what = Trinity::StringFormat(
                "StandState (creature_addon) -> {} (spawn guid {}, INSERT)",
                state, spawnId);
        }
        return ApplyFix(h, sql, "gstate", what.c_str());
    }

    // ----- .fix cstate <StandState> -----
    // Changes StandState in creature_template_addon for the template (by entry).
    // creature_template_addon stores per-template data, Entry = creature_template.entry.
    //
    // If no row exists - INSERT a new one with Entry + StandState (remaining columns
    // take DEFAULT from the table schema).
    //
    // SQL:
    //   UPDATE: UPDATE creature_template_addon SET StandState = X WHERE Entry = <entry>;
    //   INSERT: INSERT INTO creature_template_addon (Entry, StandState) VALUES (<entry>, X);
    //
    // Affects ALL spawns of this entry (not just one specific spawn).
    static bool HandleFixCState(ChatHandler* h, uint32 state)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // Check if a row exists in creature_template_addon.
        QueryResult res = WorldDatabase.PQuery(
            "SELECT 1 FROM creature_template_addon WHERE Entry = {} LIMIT 1", entry);

        std::string sql;
        std::string what;
        if (res)
        {
            // Row exists - UPDATE.
            sql = Trinity::StringFormat(
                "UPDATE creature_template_addon SET StandState = {} WHERE Entry = {};",
                state, entry);
            what = Trinity::StringFormat(
                "StandState (creature_template_addon) -> {} (entry {}, UPDATE)",
                state, entry);
        }
        else
        {
            // No row - INSERT with Entry + StandState.
            sql = Trinity::StringFormat(
                "INSERT INTO creature_template_addon (Entry, StandState) VALUES ({}, {});",
                entry, state);
            what = Trinity::StringFormat(
                "StandState (creature_template_addon) -> {} (entry {}, INSERT)",
                state, entry);
        }
        return ApplyFix(h, sql, "cstate", what.c_str());
    }

    // ----- .fix gdead -----
    // Sets DEAD status for a specific spawn by stamping the full dead-flag
    // combination on the per-spawn columns of the creature table:
    //   unit_flags  = 537133568   (force the dead visual state)
    //   unit_flags2 = 2049
    //   unit_flags3 = 8192
    //   curHealthPct= 0           (otherwise the NPC lies down but with full HP)
    //
    // SQL (per-spawn, via creature):
    //   UPDATE creature SET unit_flags = 537133568, unit_flags2 = 2049,
    //                       unit_flags3 = 8192, curHealthPct = 0 WHERE guid = <spawnId>;
    static bool HandleFixGDead(ChatHandler* h)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint64 spawnId = c->GetSpawnId();
        if (spawnId == 0)
        {
            h->SendSysMessage("[gm_fixer] Selected creature has no SpawnId (probably a temporary summon).");
            return true;
        }

        // Full dead-flag combination for per-spawn creature columns.
        // Read old values for the message.
        uint32 oldF1 = 0, oldF2 = 0, oldF3 = 0;
        if (QueryResult r = WorldDatabase.PQuery(
            "SELECT unit_flags, unit_flags2, unit_flags3 FROM creature WHERE guid = {}", spawnId))
        {
            Field* f = r->Fetch();
            oldF1 = f[0].GetUInt32();
            oldF2 = f[1].GetUInt32();
            oldF3 = f[2].GetUInt32();
        }

        std::string sql = Trinity::StringFormat(
            "UPDATE creature SET unit_flags = {}, unit_flags2 = {}, unit_flags3 = {}, curHealthPct = 0 WHERE guid = {};",
            DEAD_FLAGS1, DEAD_FLAGS2, DEAD_FLAGS3, spawnId);
        return ApplyFix(h, sql, "gdead",
            Trinity::StringFormat("DEAD: unit_flags {}->{} / unit_flags2 {}->{} / unit_flags3 {}->{}, curHealthPct -> 0 (spawn guid {})",
                oldF1, DEAD_FLAGS1, oldF2, DEAD_FLAGS2, oldF3, DEAD_FLAGS3, spawnId).c_str());
    }

    // ----- .fix cdead -----
    // Sets DEAD status for all spawns of an entry by stamping the same full
    // dead-flag combination as .fix gdead on creature_template:
    //   unit_flags  = 537133568
    //   unit_flags2 = 2049
    //   unit_flags3 = 8192
    //
    // SQL (per-template):
    //   UPDATE creature_template SET unit_flags = 537133568, unit_flags2 = 2049,
    //                                unit_flags3 = 8192 WHERE entry = <entry>;
    static bool HandleFixCDead(ChatHandler* h)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // Read old values for the message.
        uint32 oldF1 = 0, oldF2 = 0, oldF3 = 0;
        if (QueryResult r = WorldDatabase.PQuery(
            "SELECT unit_flags, unit_flags2, unit_flags3 FROM creature_template WHERE entry = {}", entry))
        {
            Field* f = r->Fetch();
            oldF1 = f[0].GetUInt32();
            oldF2 = f[1].GetUInt32();
            oldF3 = f[2].GetUInt32();
        }

        std::string sql = Trinity::StringFormat(
            "UPDATE creature_template SET unit_flags = {}, unit_flags2 = {}, unit_flags3 = {} WHERE entry = {};",
            DEAD_FLAGS1, DEAD_FLAGS2, DEAD_FLAGS3, entry);
        return ApplyFix(h, sql, "cdead",
            Trinity::StringFormat("DEAD: unit_flags {}->{} / unit_flags2 {}->{} / unit_flags3 {}->{} (entry {})",
                oldF1, DEAD_FLAGS1, oldF2, DEAD_FLAGS2, oldF3, DEAD_FLAGS3, entry).c_str());
    }

    // ----- .fix galive -----
    // Clears DEAD status for a specific spawn (inverse of .fix gdead):
    //   unit_flags / unit_flags2 / unit_flags3 = 0
    //   curHealthPct = NULL  (full HP; 0 is treated as invalid by the core)
    //
    // SQL (per-spawn, via creature):
    //   UPDATE creature SET unit_flags = 0, unit_flags2 = 0, unit_flags3 = 0,
    //                       curHealthPct = NULL WHERE guid = <spawnId>;
    static bool HandleFixGAlive(ChatHandler* h)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint64 spawnId = c->GetSpawnId();
        if (spawnId == 0)
        {
            h->SendSysMessage("[gm_fixer] Selected creature has no SpawnId (probably a temporary summon).");
            return true;
        }

        uint32 oldF1 = 0, oldF2 = 0, oldF3 = 0;
        if (QueryResult r = WorldDatabase.PQuery(
            "SELECT unit_flags, unit_flags2, unit_flags3 FROM creature WHERE guid = {}", spawnId))
        {
            Field* f = r->Fetch();
            oldF1 = f[0].GetUInt32();
            oldF2 = f[1].GetUInt32();
            oldF3 = f[2].GetUInt32();
        }

        std::string sql = Trinity::StringFormat(
            "UPDATE creature SET unit_flags = {}, unit_flags2 = {}, unit_flags3 = {}, curHealthPct = NULL WHERE guid = {};",
            ALIVE_FLAGS1, ALIVE_FLAGS2, ALIVE_FLAGS3, spawnId);
        return ApplyFix(h, sql, "galive",
            Trinity::StringFormat("ALIVE: unit_flags {}->{} / unit_flags2 {}->{} / unit_flags3 {}->{}, curHealthPct -> NULL (spawn guid {})",
                oldF1, ALIVE_FLAGS1, oldF2, ALIVE_FLAGS2, oldF3, ALIVE_FLAGS3, spawnId).c_str());
    }

    // ----- .fix calive -----
    // Clears DEAD status for all spawns of an entry (inverse of .fix cdead):
    //   unit_flags / unit_flags2 / unit_flags3 = 0
    //
    // SQL (per-template):
    //   UPDATE creature_template SET unit_flags = 0, unit_flags2 = 0,
    //                                unit_flags3 = 0 WHERE entry = <entry>;
    static bool HandleFixCAlive(ChatHandler* h)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint32 oldF1 = 0, oldF2 = 0, oldF3 = 0;
        if (QueryResult r = WorldDatabase.PQuery(
            "SELECT unit_flags, unit_flags2, unit_flags3 FROM creature_template WHERE entry = {}", entry))
        {
            Field* f = r->Fetch();
            oldF1 = f[0].GetUInt32();
            oldF2 = f[1].GetUInt32();
            oldF3 = f[2].GetUInt32();
        }

        std::string sql = Trinity::StringFormat(
            "UPDATE creature_template SET unit_flags = {}, unit_flags2 = {}, unit_flags3 = {} WHERE entry = {};",
            ALIVE_FLAGS1, ALIVE_FLAGS2, ALIVE_FLAGS3, entry);
        return ApplyFix(h, sql, "calive",
            Trinity::StringFormat("ALIVE: unit_flags {}->{} / unit_flags2 {}->{} / unit_flags3 {}->{} (entry {})",
                oldF1, ALIVE_FLAGS1, oldF2, ALIVE_FLAGS2, oldF3, ALIVE_FLAGS3, entry).c_str());
    }

    // ----- .fix gnpcflag <npcflag> -----
    // Changes npcflag for a specific spawn (per-spawn, via creature).
    // In modern TrinityCore per-spawn npcflag is stored in the creature table.
    //
    // SQL: UPDATE creature SET npcflag = X WHERE guid = <spawnId>;
    //
    // Common npcflag values:
    //   0         = none
    //   1         = GOSSIP (can talk to)
    //   128       = VENDOR (vendor)
    //   4096      = TRAINER (trainer)
    //   65536     = BANKER (banker)
    //   131072    = PETITIONER (guild registrar)
    //   524288    = TABARDDESIGNER
    //   1048576   = BATTLEMASTER (battlemaster)
    //   2097152   = AUCTIONEER (auctioneer)
    //   0x1000000 = SPIRITHEALER (spirit healer)
    //   0x2000000 = SPIRITGUIDE (spirit guide)
    // Can be combined by addition.
    static bool HandleFixGNpcFlag(ChatHandler* h, uint64 flags)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint64 spawnId = c->GetSpawnId();
        if (spawnId == 0)
        {
            h->SendSysMessage("[gm_fixer] Selected creature has no SpawnId (probably a temporary summon).");
            return true;
        }

        // Read old value
        uint64 oldVal = 0;
        if (QueryResult r = WorldDatabase.PQuery("SELECT npcflag FROM creature WHERE guid = {}", spawnId))
            oldVal = r->Fetch()[0].GetUInt64();

        std::string sql = Trinity::StringFormat(
            "UPDATE creature SET npcflag = {} WHERE guid = {};",
            flags, spawnId);
        return ApplyFix(h, sql, "gnpcflag",
            Trinity::StringFormat("npcflag: {} -> {} (spawn guid {})", oldVal, flags, spawnId).c_str());
    }

    // ----- .fix cnpcflag <npcflag> -----
    // Changes npcflag for the template (per-template, via creature_template).
    // Affects ALL spawns of this entry.
    //
    // SQL: UPDATE creature_template SET npcflag = X WHERE entry = <entry>;
    static bool HandleFixCNpcFlag(ChatHandler* h, uint64 flags)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // Read old value
        uint64 oldVal = 0;
        if (QueryResult r = WorldDatabase.PQuery("SELECT npcflag FROM creature_template WHERE entry = {}", entry))
            oldVal = r->Fetch()[0].GetUInt64();

        std::string sql = Trinity::StringFormat(
            "UPDATE creature_template SET npcflag = {} WHERE entry = {};",
            flags, entry);
        return ApplyFix(h, sql, "cnpcflag",
            Trinity::StringFormat("npcflag: {} -> {} (entry {})", oldVal, flags, entry).c_str());
    }

    // ----- .fix guflags <unit_flags> -----
    // Changes unit_flags for a specific spawn (per-spawn, via creature).
    //
    // SQL: UPDATE creature SET unit_flags = X WHERE guid = <spawnId>;
    //
    // Common unit_flags values:
    //   0           = none
    //   64          = UNIT_FLAG_NOT_ATTACKABLE_1 (cannot be attacked)
    //   256         = UNIT_FLAG_IMMUNE_TO_NPC (immune to NPC spells)
    //   512         = UNIT_FLAG_IMMUNE_TO_PC (immune to players)
    //   768         = 256 + 512 (fully immune NPC)
    //   32768       = UNIT_FLAG_PACIFIED (doesn't attack on its own)
    //   33554432    = UNIT_FLAG_NOT_SELECTABLE (cannot be selected)
    //   67108864    = UNIT_FLAG_SKINNABLE (can be skinned)
    // Can be combined by addition.
    static bool HandleFixGUnitFlags(ChatHandler* h, uint32 flags)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint64 spawnId = c->GetSpawnId();
        if (spawnId == 0)
        {
            h->SendSysMessage("[gm_fixer] Selected creature has no SpawnId (probably a temporary summon).");
            return true;
        }

        // Read old value
        uint32 oldVal = 0;
        if (QueryResult r = WorldDatabase.PQuery("SELECT unit_flags FROM creature WHERE guid = {}", spawnId))
            oldVal = r->Fetch()[0].GetUInt32();

        std::string sql = Trinity::StringFormat(
            "UPDATE creature SET unit_flags = {} WHERE guid = {};",
            flags, spawnId);
        return ApplyFix(h, sql, "guflags",
            Trinity::StringFormat("unit_flags: {} -> {} (spawn guid {})", oldVal, flags, spawnId).c_str());
    }

    // ----- .fix cuflags <unit_flags> -----
    // Changes unit_flags for the template (per-template, via creature_template).
    // Affects ALL spawns of this entry.
    //
    // SQL: UPDATE creature_template SET unit_flags = X WHERE entry = <entry>;
    static bool HandleFixCUnitFlags(ChatHandler* h, uint32 flags)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // Read old value
        uint32 oldVal = 0;
        if (QueryResult r = WorldDatabase.PQuery("SELECT unit_flags FROM creature_template WHERE entry = {}", entry))
            oldVal = r->Fetch()[0].GetUInt32();

        std::string sql = Trinity::StringFormat(
            "UPDATE creature_template SET unit_flags = {} WHERE entry = {};",
            flags, entry);
        return ApplyFix(h, sql, "cuflags",
            Trinity::StringFormat("unit_flags: {} -> {} (entry {})", oldVal, flags, entry).c_str());
    }

    // ----- .fix guflags2 <unit_flags2> -----
    static bool HandleFixGUnitFlags2(ChatHandler* h, uint32 flags)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint64 spawnId = c->GetSpawnId();
        if (spawnId == 0)
        {
            h->SendSysMessage("[gm_fixer] Selected creature has no SpawnId (probably a temporary summon).");
            return true;
        }

        uint32 oldVal = 0;
        if (QueryResult r = WorldDatabase.PQuery("SELECT unit_flags2 FROM creature WHERE guid = {}", spawnId))
            oldVal = r->Fetch()[0].GetUInt32();

        std::string sql = Trinity::StringFormat(
            "UPDATE creature SET unit_flags2 = {} WHERE guid = {};",
            flags, spawnId);
        return ApplyFix(h, sql, "guflags2",
            Trinity::StringFormat("unit_flags2: {} -> {} (spawn guid {})", oldVal, flags, spawnId).c_str());
    }

    // ----- .fix cuflags2 <unit_flags2> -----
    static bool HandleFixCUnitFlags2(ChatHandler* h, uint32 flags)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint32 oldVal = 0;
        if (QueryResult r = WorldDatabase.PQuery("SELECT unit_flags2 FROM creature_template WHERE entry = {}", entry))
            oldVal = r->Fetch()[0].GetUInt32();

        std::string sql = Trinity::StringFormat(
            "UPDATE creature_template SET unit_flags2 = {} WHERE entry = {};",
            flags, entry);
        return ApplyFix(h, sql, "cuflags2",
            Trinity::StringFormat("unit_flags2: {} -> {} (entry {})", oldVal, flags, entry).c_str());
    }

    // ----- .fix guflags3 <unit_flags3> -----
    static bool HandleFixGUnitFlags3(ChatHandler* h, uint32 flags)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint64 spawnId = c->GetSpawnId();
        if (spawnId == 0)
        {
            h->SendSysMessage("[gm_fixer] Selected creature has no SpawnId (probably a temporary summon).");
            return true;
        }

        uint32 oldVal = 0;
        if (QueryResult r = WorldDatabase.PQuery("SELECT unit_flags3 FROM creature WHERE guid = {}", spawnId))
            oldVal = r->Fetch()[0].GetUInt32();

        std::string sql = Trinity::StringFormat(
            "UPDATE creature SET unit_flags3 = {} WHERE guid = {};",
            flags, spawnId);
        return ApplyFix(h, sql, "guflags3",
            Trinity::StringFormat("unit_flags3: {} -> {} (spawn guid {})", oldVal, flags, spawnId).c_str());
    }

    // ----- .fix cuflags3 <unit_flags3> -----
    static bool HandleFixCUnitFlags3(ChatHandler* h, uint32 flags)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint32 oldVal = 0;
        if (QueryResult r = WorldDatabase.PQuery("SELECT unit_flags3 FROM creature_template WHERE entry = {}", entry))
            oldVal = r->Fetch()[0].GetUInt32();

        std::string sql = Trinity::StringFormat(
            "UPDATE creature_template SET unit_flags3 = {} WHERE entry = {};",
            flags, entry);
        return ApplyFix(h, sql, "cuflags3",
            Trinity::StringFormat("unit_flags3: {} -> {} (entry {})", oldVal, flags, entry).c_str());
    }

    // ----- .fix combat -----
    // Enables SmartAI and adds a smart_script to attack nearby creatures
    // within a 10 yard radius.
    //
    // Logic:
    //   1. Checks AIName in creature_template.
    //      If AIName != 'SmartAI' - UPDATE it to 'SmartAI'.
    //   2. Checks for a smart_scripts entry with attack parameters for this entry
    //      (action_type=49, target_type=25, target_param1=10).
    //      If no such script exists - INSERT with id = MAX(id)+1 (to avoid conflicts
    //      with existing scripts for this entry).
    //
    // SQL:
    //   UPDATE creature_template SET AIName = 'SmartAI' WHERE entry = <entry>;
    //   INSERT INTO smart_scripts (entryorguid, source_type, id, event_type,
    //     event_chance, action_type, target_type, target_param1, target_param2,
    //     target_param3, target_param4)
    //   VALUES (entry, 0, <nextId>, 60, 100, 49, 25, 10, 0, 0, 0);
    //
    // smart_script parameters:
    //   source_type  = 0   (SMART_SCRIPT_SOURCE_CREATURE)
    //   id           = auto (MAX+1 for this entry)
    //   event_type   = 60  (SMART_EVENT_UPDATE - periodic)
    //   event_chance = 100 (100% trigger chance)
    //   action_type  = 49  (SMART_ACTION_ATTACK - attack)
    //   target_type  = 25  (SMART_TARGET_CREATURE_RANGE - creatures in range)
    //   target_param1= 10  (radius 10 yards)
    static bool HandleFixCombat(ChatHandler* h)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // 1. Check AIName in creature_template.
        QueryResult aiRes = WorldDatabase.PQuery(
            "SELECT AIName FROM creature_template WHERE entry = {} LIMIT 1", entry);
        bool needUpdateAI = true;
        if (aiRes)
        {
            std::string aiName = aiRes->Fetch()[0].GetString();
            if (aiName == "SmartAI")
                needUpdateAI = false;
        }

        // 2. Check for smart_scripts with attack parameters for this entry.
        // Look for action_type=49 (ATTACK) + target_type=25 (CREATURE_RANGE) +
        // target_param1=10 (10yd radius) - these are the unique parameters of our fix.
        QueryResult ssRes = WorldDatabase.PQuery(
            "SELECT 1 FROM smart_scripts WHERE entryorguid = {} AND source_type = 0 "
            "AND action_type = 49 AND target_type = 25 AND target_param1 = 10 LIMIT 1",
            entry);
        bool needInsertScript = !ssRes;

        // If already SmartAI and script exists - nothing to do.
        if (!needUpdateAI && !needInsertScript)
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] combat: entry {} already has SmartAI + combat script, nothing to do.",
                entry));
            return true;
        }

        // Calculate the next free id for this entry (MAX+1, minimum 0).
        uint32 nextId = 0;
        if (needInsertScript)
        {
            QueryResult maxRes = WorldDatabase.PQuery(
                "SELECT IFNULL(MAX(id), -1) + 1 FROM smart_scripts "
                "WHERE entryorguid = {} AND source_type = 0", entry);
            if (maxRes)
                nextId = maxRes->Fetch()[0].GetUInt32();
        }

        // Build the SQL block (one or two statements).
        std::string sqlLog;  // for the daily file and log
        std::vector<std::string> stmts;  // for separate execution
        if (needUpdateAI)
        {
            std::string s = Trinity::StringFormat(
                "UPDATE creature_template SET AIName = 'SmartAI' WHERE entry = {};",
                entry);
            stmts.push_back(s);
            sqlLog += s + "\n";
        }
        if (needInsertScript)
        {
            // event_param1=0, event_param2=0  - no delay on first trigger
            // event_param3=1000, event_param4=1000 - repeat every 1 second (1 sec = 1000 ms)
            // Without event_param3/4 SMART_EVENT_UPDATE triggers once and never repeats!
            std::string s = Trinity::StringFormat(
                "INSERT INTO smart_scripts "
                "(entryorguid, source_type, id, link, event_type, event_phase_mask, "
                " event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, "
                " action_type, action_param1, action_param2, action_param3, "
                " action_param4, action_param5, action_param6, "
                " target_type, target_param1, target_param2, target_param3, target_param4, "
                " target_x, target_y, target_z, target_o, comment) "
                "VALUES ({}, 0, {}, 0, 60, 0, 100, 0, "
                " 0, 0, 1000, 1000, "
                " 49, 0, 0, 0, 0, 0, 0, "
                " 25, 10, 0, 0, 0, "
                " 0, 0, 0, 0, 'gm_fixer: combat attack 10yd');",
                entry, nextId);
            stmts.push_back(s);
            sqlLog += s;
        }

        // Execute each statement separately - DirectExecute does NOT support
        // multiple statements in a single call (MySQL C API limitation).
        for (auto const& s : stmts)
            WorldDatabase.DirectExecute(s.c_str());

        std::string what;
        if (needUpdateAI && needInsertScript)
            what = Trinity::StringFormat(
                "combat: entry {} (SmartAI + script id={})", entry, nextId);
        else if (needUpdateAI)
            what = Trinity::StringFormat(
                "combat: entry {} (SmartAI only, combat script already exists)", entry);
        else
            what = Trinity::StringFormat(
                "combat: entry {} (script id={}, SmartAI already set)", entry, nextId);

        AppendFixLog(h, sqlLog, "combat", what);

        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL:\n{}", sqlLog));
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sqlLog);
        return true;
    }

    // ----- .fix questprev <questId> <prevQuestId> -----
    // Sets PrevQuestID in quest_template_addon for quest questId.
    // prevQuestId is the ID of the quest that must be completed BEFORE questId.
    //
    // If no row exists in quest_template_addon - INSERT a new one.
    //
    // SQL:
    //   UPDATE: UPDATE quest_template_addon SET PrevQuestID = X WHERE ID = Y;
    //   INSERT: INSERT INTO quest_template_addon (ID, PrevQuestID) VALUES (Y, X);
    //
    // Notes:
    //   - If prevQuestId > 0 - the quest must be completed (normal prerequisite)
    //   - If prevQuestId < 0 - the quest must NOT be completed (exclusive)
    static bool HandleFixQuestPrev(ChatHandler* h, uint32 questId, int32 prevQuestId)
    {
        // Check that the quest exists.
        if (!sObjectMgr->GetQuestTemplate(questId))
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] Quest {} not found in quest_template.", questId));
            return true;
        }
        // If prevQuestId != 0 - check that the prerequisite quest also exists.
        if (prevQuestId != 0 && !sObjectMgr->GetQuestTemplate(uint32(std::abs(prevQuestId))))
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] PrevQuest {} not found in quest_template.",
                std::abs(prevQuestId)));
            return true;
        }

        // Check if a row exists in quest_template_addon.
        QueryResult res = WorldDatabase.PQuery(
            "SELECT 1 FROM quest_template_addon WHERE ID = {} LIMIT 1", questId);

        std::string sql;
        std::string what;
        if (res)
        {
            // Row exists - UPDATE.
            sql = Trinity::StringFormat(
                "UPDATE quest_template_addon SET PrevQuestID = {} WHERE ID = {};",
                prevQuestId, questId);
            what = Trinity::StringFormat(
                "PrevQuestID -> {} (quest {}, UPDATE)", prevQuestId, questId);
        }
        else
        {
            // No row - INSERT with ID + PrevQuestID.
            sql = Trinity::StringFormat(
                "INSERT INTO quest_template_addon (ID, PrevQuestID) VALUES ({}, {});",
                questId, prevQuestId);
            what = Trinity::StringFormat(
                "PrevQuestID -> {} (quest {}, INSERT)", prevQuestId, questId);
        }
        return ApplyFix(h, sql, "questprev", what.c_str());
    }

    // ----- .fix gmove <MovementType> [wander_distance] -----
    // Changes MovementType AND wander_distance for a specific spawn (per-spawn).
    // If wander_distance is not specified - default = 8.
    //
    // SQL: UPDATE creature SET MovementType = X, wander_distance = Y WHERE guid = <spawnId>;
    //
    // Common MovementType values:
    //   0 = IDLE (stays in place)
    //   1 = RANDOM (wanders randomly within wander_distance radius)
    //   2 = WAYPOINT (follows a waypoint route)
    static bool HandleFixGMove(ChatHandler* h, uint32 moveType, Optional<double> wanderDistOpt)
    {
        if (moveType > 2)
        {
            h->SendSysMessage("[gm_fixer] MovementType must be 0 (IDLE), 1 (RANDOM) or 2 (WAYPOINT).");
            return true;
        }

        double wanderDist = 8.0;
        if (wanderDistOpt)
        {
            if (*wanderDistOpt < 0.0)
            {
                h->SendSysMessage("[gm_fixer] wander_distance must be >= 0.");
                return true;
            }
            wanderDist = *wanderDistOpt;
        }

        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint64 spawnId = c->GetSpawnId();
        if (spawnId == 0)
        {
            h->SendSysMessage("[gm_fixer] Selected creature has no SpawnId (probably a temporary summon).");
            return true;
        }

        // Check AIName - if empty, add UPDATE to SmartAI.
        QueryResult aiRes = WorldDatabase.PQuery(
            "SELECT AIName FROM creature_template WHERE entry = {} LIMIT 1", entry);
        bool needSmartAI = true;
        if (aiRes)
        {
            std::string aiName = aiRes->Fetch()[0].GetString();
            if (!aiName.empty())
                needSmartAI = false;
        }

        // Build SQL (one or two operations).
        std::string sqlLog;
        std::vector<std::string> stmts;
        if (needSmartAI)
        {
            std::string s = Trinity::StringFormat(
                "UPDATE creature_template SET AIName = 'SmartAI' WHERE entry = {};", entry);
            stmts.push_back(s);
            sqlLog += s + "\n";
        }

        std::string moveSql = Trinity::StringFormat(
            "UPDATE creature SET MovementType = {}, wander_distance = {} WHERE guid = {};",
            moveType, wanderDist, spawnId);
        stmts.push_back(moveSql);
        sqlLog += moveSql;

        for (auto const& s : stmts)
            WorldDatabase.DirectExecute(s.c_str());

        std::string what = Trinity::StringFormat(
            "MovementType -> {}, wander_distance -> {} (spawn guid {}, entry {}){}",
            moveType, wanderDist, spawnId, entry,
            needSmartAI ? " + SmartAI" : "");
        AppendFixLog(h, sqlLog, "gmove", what);

        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL:\n{}", sqlLog));
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sqlLog);
        return true;
    }

    // ----- .fix cmove <MovementType> -----
    // Changes MovementType for the template (per-template, via creature_template).
    // Affects ALL spawns of this entry.
    //
    // SQL: UPDATE creature_template SET MovementType = X WHERE entry = <entry>;
    static bool HandleFixCMove(ChatHandler* h, uint32 moveType)
    {
        if (moveType > 2)
        {
            h->SendSysMessage("[gm_fixer] MovementType must be 0 (IDLE), 1 (RANDOM) or 2 (WAYPOINT).");
            return true;
        }
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // Read old value
        uint32 oldVal = 0;
        if (QueryResult r = WorldDatabase.PQuery("SELECT MovementType FROM creature_template WHERE entry = {}", entry))
            oldVal = r->Fetch()[0].GetUInt32();

        std::string sql = Trinity::StringFormat(
            "UPDATE creature_template SET MovementType = {} WHERE entry = {};",
            moveType, entry);
        return ApplyFix(h, sql, "cmove",
            Trinity::StringFormat("MovementType: {} -> {} (entry {})", oldVal, moveType, entry).c_str());
    }

    // ----- .fix gmoverange <wander_distance> -----
    // Changes wander_distance for a specific spawn (per-spawn, via creature).
    // wander_distance is the random wandering radius (in yards).
    // Only meaningful with MovementType = 1 (RANDOM).
    //
    // SQL: UPDATE creature SET wander_distance = X WHERE guid = <spawnId>;
    static bool HandleFixGMoveRange(ChatHandler* h, double dist)
    {
        if (dist < 0.0)
        {
            h->SendSysMessage("[gm_fixer] wander_distance must be >= 0.");
            return true;
        }
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint64 spawnId = c->GetSpawnId();
        if (spawnId == 0)
        {
            h->SendSysMessage("[gm_fixer] Selected creature has no SpawnId (probably a temporary summon).");
            return true;
        }

        // Read old value
        float oldVal = 0.0f;
        if (QueryResult r = WorldDatabase.PQuery("SELECT wander_distance FROM creature WHERE guid = {}", spawnId))
            oldVal = r->Fetch()[0].GetFloat();

        std::string sql = Trinity::StringFormat(
            "UPDATE creature SET wander_distance = {} WHERE guid = {};",
            dist, spawnId);
        return ApplyFix(h, sql, "gmoverange",
            Trinity::StringFormat("wander_distance: {} -> {} (spawn guid {})", oldVal, dist, spawnId).c_str());
    }

    // ----- .fix smart -----
    // Checks AIName in creature_template. If empty - sets it to 'SmartAI'.
    // If already set - does nothing.
    //
    // SQL: UPDATE creature_template SET AIName = 'SmartAI' WHERE entry = <entry>;
    static bool HandleFixSmart(ChatHandler* h)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // Check AIName in creature_template.
        QueryResult aiRes = WorldDatabase.PQuery(
            "SELECT AIName FROM creature_template WHERE entry = {} LIMIT 1", entry);
        if (aiRes)
        {
            std::string aiName = aiRes->Fetch()[0].GetString();
            if (!aiName.empty())
            {
                h->SendSysMessage(Trinity::StringFormat(
                    "[gm_fixer] AIName already set to '{}' (entry {}), nothing to do.",
                    aiName, entry));
                return true;
            }
        }

        std::string sql = Trinity::StringFormat(
            "UPDATE creature_template SET AIName = 'SmartAI' WHERE entry = {};", entry);
        return ApplyFix(h, sql, "smart",
            Trinity::StringFormat("AIName: '<empty>' -> 'SmartAI' (entry {})", entry).c_str());
    }

    // ----- .fix unsmart -----
    // Removes AIName (sets empty string) in creature_template.
    // If AIName is already empty - does nothing.
    static bool HandleFixUnsmart(ChatHandler* h)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        QueryResult aiRes = WorldDatabase.PQuery(
            "SELECT AIName FROM creature_template WHERE entry = {} LIMIT 1", entry);
        if (aiRes)
        {
            std::string aiName = aiRes->Fetch()[0].GetString();
            if (aiName.empty())
            {
                h->SendSysMessage(Trinity::StringFormat(
                    "[gm_fixer] AIName already empty (entry {}), nothing to do.", entry));
                return true;
            }

            std::string sql = Trinity::StringFormat(
                "UPDATE creature_template SET AIName = '' WHERE entry = {};", entry);
            return ApplyFix(h, sql, "unsmart",
                Trinity::StringFormat("AIName: '{}' -> '' (entry {})", aiName, entry).c_str());
        }

        h->SendSysMessage(Trinity::StringFormat(
            "[gm_fixer] creature_template entry {} not found.", entry));
        return true;
    }

    // ----- .fix wayinfo -----
    // Finds waypoint path IDs for all waypoint nodes within 30 yards of the player.
    static bool HandleFixWayInfo(ChatHandler* h)
    {
        Player* p = h->GetSession()->GetPlayer();
        if (!p) return true;

        float x = p->GetPositionX();
        float y = p->GetPositionY();
        float z = p->GetPositionZ();
        uint32 mapId = p->GetMapId();

        // Convert float to string via snprintf (StringFormat may produce a comma)
        auto fmtFloatW = [](float v) -> std::string {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.4f", v);
            return std::string(buf);
            };

        std::string xmin = fmtFloatW(x - 30.0f);
        std::string xmax = fmtFloatW(x + 30.0f);
        std::string ymin = fmtFloatW(y - 30.0f);
        std::string ymax = fmtFloatW(y + 30.0f);

        std::string query =
            "SELECT PathId, NodeId, PositionX, PositionY, PositionZ "
            "FROM waypoint_path_node "
            "WHERE PositionX BETWEEN " + xmin + " AND " + xmax +
            " AND PositionY BETWEEN " + ymin + " AND " + ymax +
            " LIMIT 50";
        QueryResult result = WorldDatabase.Query(query.c_str());

        if (!result)
        {
            h->SendSysMessage("[gm_fixer] No waypoint nodes found within 30 yards.");
            return true;
        }

        h->SendSysMessage(Trinity::StringFormat(
            "[gm_fixer] Waypoint nodes near ({:.1f}, {:.1f}, {:.1f}) map {}:", x, y, z, mapId));

        std::set<uint32> seenPaths;
        do
        {
            Field* f = result->Fetch();
            uint32 pathId = f[0].GetUInt32();
            uint32 nodeId = f[1].GetUInt32();
            float nx = f[2].GetFloat();
            float ny = f[3].GetFloat();
            float nz = f[4].GetFloat();

            float dx = nx - x;
            float dy = ny - y;
            float dist = std::sqrt(dx * dx + dy * dy);

            // Skip nodes farther than 30 yards (BETWEEN filters by a square, not a circle)
            if (dist > 30.0f)
                continue;

            h->SendSysMessage(Trinity::StringFormat(
                "  Path {} Node {} at ({:.1f}, {:.1f}, {:.1f}) dist {:.1f}yd",
                pathId, nodeId, nx, ny, nz, dist));
            seenPaths.insert(pathId);
        } while (result->NextRow());

        h->SendSysMessage(Trinity::StringFormat(
            "  Found {} unique path(s).",
            uint32(seenPaths.size())));
        return true;
    }

    // ----- .fix wayadd -----
    // Creates a new waypoint_path and assigns it to the selected NPC.
    // PathId = MAX(PathId)+1.
    // Sets MovementType=2 (WAYPOINT) and movementId=PathId in creature.
    // Also checks AIName - if empty, sets it to SmartAI.
    static bool HandleFixWayAdd(ChatHandler* h)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint64 spawnId = c->GetSpawnId();
        if (spawnId == 0)
        {
            h->SendSysMessage("[gm_fixer] Selected creature has no SpawnId.");
            return true;
        }

        // Calculate PathId = MAX+1 (via raw query, PQuery with aggregates is unstable)
        uint32 newPathId = 1;
        QueryResult maxRes = WorldDatabase.Query("SELECT IFNULL(MAX(PathId), 0) + 1 FROM waypoint_path");
        if (maxRes)
            newPathId = maxRes->Fetch()[0].GetUInt32();

        // Check AIName
        QueryResult aiRes = WorldDatabase.PQuery(
            "SELECT AIName FROM creature_template WHERE entry = {} LIMIT 1", entry);
        bool needSmartAI = true;
        if (aiRes)
        {
            std::string aiName = aiRes->Fetch()[0].GetString();
            if (!aiName.empty())
                needSmartAI = false;
        }

        // Build SQL
        std::string sqlLog;
        std::vector<std::string> stmts;

        if (needSmartAI)
        {
            std::string s = Trinity::StringFormat(
                "UPDATE creature_template SET AIName = 'SmartAI' WHERE entry = {};", entry);
            stmts.push_back(s);
            sqlLog += s + "\n";
        }

        // INSERT waypoint_path
        std::string sPath = Trinity::StringFormat(
            "INSERT INTO waypoint_path (PathId, MoveType, Flags, Comment) VALUES ({}, 0, 0, 'gm_fixer: entry {}');",
            newPathId, entry);
        stmts.push_back(sPath);
        sqlLog += sPath + "\n";

        // Link the path to the spawn via creature_addon.PathId (like .wp load)
        // If row exists - UPDATE, if not - INSERT
        QueryResult addonRes = WorldDatabase.PQuery(
            "SELECT 1 FROM creature_addon WHERE guid = {} LIMIT 1", spawnId);
        std::string sAddon;
        if (addonRes)
        {
            sAddon = Trinity::StringFormat(
                "UPDATE creature_addon SET PathId = {} WHERE guid = {};",
                newPathId, spawnId);
        }
        else
        {
            sAddon = Trinity::StringFormat(
                "INSERT INTO creature_addon (guid, PathId) VALUES ({}, {});",
                spawnId, newPathId);
        }
        stmts.push_back(sAddon);
        sqlLog += sAddon + "\n";

        // MovementType = WAYPOINT (2)
        std::string sCreature = Trinity::StringFormat(
            "UPDATE creature SET MovementType = 2 WHERE guid = {};",
            spawnId);
        stmts.push_back(sCreature);
        sqlLog += sCreature;

        for (auto const& s : stmts)
            WorldDatabase.DirectExecute(s.c_str());
        AppendFixLog(h, sqlLog, "wayadd",
            Trinity::StringFormat("PathId={} -> entry {} (spawn {}){}",
                newPathId, entry, spawnId,
                needSmartAI ? " + SmartAI" : ""));

        // Apply at runtime - reload path + load + init
        sWaypointMgr->ReloadPath(newPathId);
        c->LoadPath(newPathId);
        c->SetDefaultMovementType(WAYPOINT_MOTION_TYPE);
        c->GetMotionMaster()->Initialize();
        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] .wp reload {} applied automatically", newPathId));

        std::string what = Trinity::StringFormat(
            "waypoint_path {} created -> entry {} (spawn {}){}",
            newPathId, entry, spawnId,
            needSmartAI ? " + SmartAI" : "");
        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL:\n{}", sqlLog));
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sqlLog);
        return true;
    }

    // ----- .fix waynode [nodeId] [delay] -----
    // No arguments: adds a new node (NodeId = MAX+1) at the player's current position.
    // .fix waynode <nodeId>        - update coordinates of an existing node (player position)
    // .fix waynode <nodeId> <delay>- update coordinates + delay of an existing node
    static bool HandleFixWayNode(ChatHandler* h, Optional<uint32> nodeIdOpt, Optional<uint32> delayOpt)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint64 spawnId = c->GetSpawnId();
        if (spawnId == 0)
        {
            h->SendSysMessage("[gm_fixer] Selected creature has no SpawnId.");
            return true;
        }

        // Read PathId from creature_addon.PathId (NOT creature.currentwaypoint!)
        uint32 pathId = 0;
        QueryResult crRes = WorldDatabase.PQuery(
            "SELECT PathId FROM creature_addon WHERE guid = {}", spawnId);
        if (crRes)
            pathId = crRes->Fetch()[0].GetUInt32();

        if (pathId == 0)
        {
            h->SendSysMessage("[gm_fixer] This NPC has no waypoint path. Use .fix wayadd first.");
            return true;
        }

        // Parse arguments: [nodeId] [delay]
        uint32 nodeId = 0;   // 0 = new node
        uint32 delay = 0;
        bool updateMode = false;

        if (nodeIdOpt)
        {
            nodeId = *nodeIdOpt;
            updateMode = true;
        }
        if (delayOpt)
            delay = *delayOpt;

        // Player position
        Player* p = h->GetSession()->GetPlayer();
        if (!p) return true;

        // Convert float to string with dot via snprintf
        // (StringFormat/ostringstream may use comma in Russian locale)
        auto fmtFloat = [](float v) -> std::string {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.4f", v);
            return std::string(buf);
            };

        std::string sx = fmtFloat(p->GetPositionX());
        std::string sy = fmtFloat(p->GetPositionY());
        std::string sz = fmtFloat(p->GetPositionZ());
        std::string so = fmtFloat(p->GetOrientation());

        std::string sql;
        std::string what;

        if (updateMode)
        {
            // UPDATE existing node
            sql = "UPDATE waypoint_path_node SET PositionX = " + sx +
                ", PositionY = " + sy +
                ", PositionZ = " + sz +
                ", Orientation = " + so +
                ", Delay = " + std::to_string(delay) +
                " WHERE PathId = " + std::to_string(pathId) +
                " AND NodeId = " + std::to_string(nodeId) + ";";
            what = Trinity::StringFormat("Path {} Node {} -> ({},{},{}) delay {} (UPDATE)",
                pathId, nodeId, sx, sy, sz, delay);
        }
        else
        {
            // INSERT new node - calculate NodeId = MAX+1 via raw query
            uint32 newNodeId = 1;
            std::string maxQuery = "SELECT IFNULL(MAX(NodeId), 0) + 1 FROM waypoint_path_node WHERE PathId = " + std::to_string(pathId);
            QueryResult maxRes = WorldDatabase.Query(maxQuery.c_str());
            if (maxRes)
                newNodeId = maxRes->Fetch()[0].GetUInt32();
            nodeId = newNodeId;

            sql = "INSERT INTO waypoint_path_node (PathId, NodeId, PositionX, PositionY, PositionZ, Orientation, Delay) VALUES (" +
                std::to_string(pathId) + ", " + std::to_string(nodeId) + ", " +
                sx + ", " + sy + ", " + sz + ", " + so + ", " + std::to_string(delay) + ");";
            what = Trinity::StringFormat("Path {} Node {} -> ({},{},{}) delay {} (INSERT)",
                pathId, nodeId, sx, sy, sz, delay);
        }

        WorldDatabase.DirectExecute(sql.c_str());
        AppendFixLog(h, sql, "waynode", what);

        // Automatic .wp reload for this NPC's pathId
        sWaypointMgr->ReloadPath(pathId);
        c->LoadPath(pathId);
        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] .wp reload {} applied automatically", pathId));

        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL: {}", sql));
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sql);
        return true;
    }

    // ----- .fix gauras <spellId> [<spellId>...] -----
    // Sets auras in creature_addon for a specific spawn.
    // Multiple spellIds separated by spaces - all written to the auras column.
    // Also checks AIName and applies auras at runtime.
    //
    // SQL: UPDATE creature_addon SET auras = 'ID1 ID2 ID3' WHERE guid = <spawnId>;
    static bool HandleFixGAuras(ChatHandler* h, std::vector<uint32> spellIds)
    {
        if (spellIds.empty())
        {
            h->SendSysMessage("Usage: .fix gauras <spellId> [<spellId>...]");
            return true;
        }

        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint64 spawnId = c->GetSpawnId();
        if (spawnId == 0)
        {
            h->SendSysMessage("[gm_fixer] Selected creature has no SpawnId (probably a temporary summon).");
            return true;
        }

        // Build auras string: "ID1 ID2 ID3"
        std::string aurasStr;
        for (size_t i = 0; i < spellIds.size(); ++i)
        {
            if (i > 0)
                aurasStr += " ";
            aurasStr += std::to_string(spellIds[i]);
        }

        // Check AIName
        QueryResult aiRes = WorldDatabase.PQuery(
            "SELECT AIName FROM creature_template WHERE entry = {} LIMIT 1", entry);
        bool needSmartAI = true;
        if (aiRes)
        {
            std::string aiName = aiRes->Fetch()[0].GetString();
            if (!aiName.empty())
                needSmartAI = false;
        }

        // Read old auras
        std::string oldAuras = "<none>";
        QueryResult oldRes = WorldDatabase.PQuery(
            "SELECT auras FROM creature_addon WHERE guid = {}", spawnId);
        if (oldRes)
        {
            std::string s = oldRes->Fetch()[0].GetString();
            oldAuras = s.empty() ? "<empty>" : s;
        }

        // Check if a row exists in creature_addon
        QueryResult res = WorldDatabase.PQuery(
            "SELECT 1 FROM creature_addon WHERE guid = {} LIMIT 1", spawnId);

        std::string auraSql;
        std::string what;
        if (res)
        {
            auraSql = Trinity::StringFormat(
                "UPDATE creature_addon SET auras = '{}' WHERE guid = {};",
                aurasStr, spawnId);
            what = Trinity::StringFormat("auras: '{}' -> '{}' (spawn guid {}, UPDATE)",
                oldAuras, aurasStr, spawnId);
        }
        else
        {
            auraSql = Trinity::StringFormat(
                "INSERT INTO creature_addon (guid, auras) VALUES ({}, '{}');",
                spawnId, aurasStr);
            what = Trinity::StringFormat("auras: '<none>' -> '{}' (spawn guid {}, INSERT)",
                aurasStr, spawnId);
        }

        // Build SQL (one or two operations)
        std::string sqlLog;
        std::vector<std::string> stmts;
        if (needSmartAI)
        {
            std::string s = Trinity::StringFormat(
                "UPDATE creature_template SET AIName = 'SmartAI' WHERE entry = {};", entry);
            stmts.push_back(s);
            sqlLog += s + "\n";
        }
        stmts.push_back(auraSql);
        sqlLog += auraSql;

        for (auto const& s : stmts)
            WorldDatabase.DirectExecute(s.c_str());

        if (needSmartAI)
            what += Trinity::StringFormat(" + SmartAI (entry {})", entry);
        AppendFixLog(h, sqlLog, "gauras", what);

        // Apply auras at runtime
        for (uint32 sid : spellIds)
        {
            if (sSpellMgr->GetSpellInfo(sid, DIFFICULTY_NONE))
                c->AddAura(sid, c);
        }

        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL:\n{}", sqlLog));
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sqlLog);
        return true;
    }

    // ----- .fix emote <emoteId> -----
    // Applies emoteState to the selected NPC at runtime (without DB changes).
    // Equivalent to .npc playemote, but within gm_fixer.
    static bool HandleFixEmote(ChatHandler* h, uint32 emote)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        c->SetEmoteState(Emote(emote));
        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] emote {} applied to '{}' (runtime only)", emote, c->GetName()));
        return true;
    }

    // ----- .fix gemote <emoteId> -----
    // Changes Emote (animation) in creature_addon for a specific spawn (by SpawnId).
    // If no row exists - INSERT a new one.
    //
    // SQL:
    //   UPDATE: UPDATE creature_addon SET emote = X WHERE guid = <spawnId>;
    //   INSERT: INSERT INTO creature_addon (guid, emote) VALUES (<spawnId>, X);
    //
    // Common emote values:
    //   0  = none (reset)
    //   1  = TALK
    //   4  = ONE_SHOT_BOW
    //   10 = STATE_DANCE
    //   16 = ONE_SHOT_KNEEL
    //   23 = STATE_WORK
    //   26 = STATE_SLEEP
    //   27 = STATE_SIT
    //   43 = STATE_SLEEP
    static bool HandleFixGEmote(ChatHandler* h, uint32 emote)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint64 spawnId = c->GetSpawnId();
        if (spawnId == 0)
        {
            h->SendSysMessage("[gm_fixer] Selected creature has no SpawnId (probably a temporary summon).");
            return true;
        }

        // Check AIName - if empty, add UPDATE to SmartAI.
        QueryResult aiRes = WorldDatabase.PQuery(
            "SELECT AIName FROM creature_template WHERE entry = {} LIMIT 1", entry);
        bool needSmartAI = true;
        if (aiRes)
        {
            std::string aiName = aiRes->Fetch()[0].GetString();
            if (!aiName.empty())
                needSmartAI = false;
        }

        // Check if a row exists in creature_addon.
        QueryResult res = WorldDatabase.PQuery(
            "SELECT 1 FROM creature_addon WHERE guid = {} LIMIT 1", spawnId);

        std::string emoteSql;
        std::string what;
        if (res)
        {
            emoteSql = Trinity::StringFormat(
                "UPDATE creature_addon SET emote = {} WHERE guid = {};",
                emote, spawnId);
            what = Trinity::StringFormat(
                "emote -> {} (spawn guid {}, UPDATE)", emote, spawnId);
        }
        else
        {
            emoteSql = Trinity::StringFormat(
                "INSERT INTO creature_addon (guid, emote) VALUES ({}, {});",
                spawnId, emote);
            what = Trinity::StringFormat(
                "emote -> {} (spawn guid {}, INSERT)", emote, spawnId);
        }

        // Build SQL (one or two operations).
        std::string sqlLog;
        std::vector<std::string> stmts;
        if (needSmartAI)
        {
            std::string s = Trinity::StringFormat(
                "UPDATE creature_template SET AIName = 'SmartAI' WHERE entry = {};", entry);
            stmts.push_back(s);
            sqlLog += s + "\n";
        }
        stmts.push_back(emoteSql);
        sqlLog += emoteSql;

        for (auto const& s : stmts)
            WorldDatabase.DirectExecute(s.c_str());

        if (needSmartAI)
            what += Trinity::StringFormat(" + SmartAI (entry {})", entry);
        AppendFixLog(h, sqlLog, "gemote", what);

        // Apply emote at runtime
        c->SetEmoteState(Emote(emote));

        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL:\n{}", sqlLog));
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sqlLog);
        return true;
    }

    // ----- .fix gpos -----
    // Updates position_x, position_y, position_z, orientation for a specific
    // spawn (by SpawnID/guid) with the current coordinates of the selected NPC in the world.
    //
    // SQL: UPDATE creature SET position_x = X, position_y = Y, position_z = Z, orientation = O WHERE guid = <spawnId>;
    //
    // Useful when the GM has moved the NPC to a new location via .npc move or
    // manually, and wants to save the new coordinates to the DB.
    static bool HandleFixGPos(ChatHandler* h)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint64 spawnId = c->GetSpawnId();
        if (spawnId == 0)
        {
            h->SendSysMessage("[gm_fixer] Selected creature has no SpawnId (probably a temporary summon).");
            return true;
        }

        float x = c->GetPositionX();
        float y = c->GetPositionY();
        float z = c->GetPositionZ();
        float o = c->GetOrientation();

        // Read old values
        float oldX = 0, oldY = 0, oldZ = 0, oldO = 0;
        if (QueryResult r = WorldDatabase.PQuery("SELECT position_x, position_y, position_z, orientation FROM creature WHERE guid = {}", spawnId))
        {
            Field* f = r->Fetch();
            oldX = f[0].GetFloat(); oldY = f[1].GetFloat(); oldZ = f[2].GetFloat(); oldO = f[3].GetFloat();
        }

        std::string sql = Trinity::StringFormat(
            "UPDATE creature SET position_x = {}, position_y = {}, position_z = {}, orientation = {} WHERE guid = {};",
            x, y, z, o, spawnId);
        return ApplyFix(h, sql, "gpos",
            Trinity::StringFormat("position: ({:.2f},{:.2f},{:.2f},{:.2f}) -> ({:.2f},{:.2f},{:.2f},{:.2f}) (spawn guid {})",
                oldX, oldY, oldZ, oldO, x, y, z, o, spawnId).c_str());
    }

    // ----- .fix gmoveall [movementType] [wander_distance] -----
    // Updates MovementType AND wander_distance for ALL spawns of the selected NPC
    // (by entry, not by a specific guid).
    //
    // No arguments: MovementType = 1 (RANDOM), wander_distance = 8
    // With arguments: .fix gmoveall <movementType> <wander_distance>
    //
    // SQL: UPDATE creature SET MovementType = X, wander_distance = Y WHERE id = <entry>;
    static bool HandleFixGMoveAll(ChatHandler* h, Optional<uint32> moveTypeOpt, Optional<double> wanderDistOpt)
    {
        // Defaults: RANDOM movement, 8 yards wander
        uint32 moveType = 1;
        double wanderDist = 8.0;

        if (moveTypeOpt)
        {
            if (*moveTypeOpt > 2)
            {
                h->SendSysMessage("[gm_fixer] MovementType must be 0 (IDLE), 1 (RANDOM) or 2 (WAYPOINT).");
                return true;
            }
            moveType = *moveTypeOpt;
        }

        if (wanderDistOpt)
        {
            if (*wanderDistOpt < 0.0)
            {
                h->SendSysMessage("[gm_fixer] wander_distance must be >= 0.");
                return true;
            }
            wanderDist = *wanderDistOpt;
        }

        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // Read old values
        uint32 oldMt = 0; float oldWd = 0.0f;
        if (QueryResult r = WorldDatabase.PQuery("SELECT MovementType, wander_distance FROM creature WHERE id = {} LIMIT 1", entry))
        {
            oldMt = r->Fetch()[0].GetUInt32();
            oldWd = r->Fetch()[1].GetFloat();
        }

        std::string sql = Trinity::StringFormat(
            "UPDATE creature SET MovementType = {}, wander_distance = {} WHERE id = {};",
            moveType, wanderDist, entry);
        return ApplyFix(h, sql, "gmoveall",
            Trinity::StringFormat("MovementType: {} -> {}, wander_distance: {} -> {} (ALL spawns entry {})",
                oldMt, moveType, oldWd, wanderDist, entry).c_str());
    }

    // ----- .fix scalesize <scale> -----
    // Changes the creature's scale (size) in creature_template.
    // Value can be fractional: 0.85, 1.0, 1.5, 2.0
    //
    // SQL: UPDATE creature_template SET scale = X WHERE entry = <entry>;
    //
    // Saved to a separate file gm_fixes/creature_sizes.sql next to the worldserver.
    static bool HandleFixScaleSize(ChatHandler* h, double scale)
    {
        if (scale <= 0.0 || scale > 100.0)
        {
            h->SendSysMessage("[gm_fixer] scale must be between 0.01 and 100.0");
            return true;
        }
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // Read old value
        float oldVal = 0.0f;
        if (QueryResult r = WorldDatabase.PQuery("SELECT scale FROM creature_template WHERE entry = {}", entry))
            oldVal = r->Fetch()[0].GetFloat();

        std::string sql = Trinity::StringFormat(
            "UPDATE creature_template SET scale = {} WHERE entry = {};",
            scale, entry);

        // Execute SQL and write to a separate file (not the daily file).
        WorldDatabase.DirectExecute(sql.c_str());
        AppendToScaleSizeFile(sql, "scalesize",
            Trinity::StringFormat("scale: {} -> {} (entry {})", oldVal, scale, entry));

        std::string what = Trinity::StringFormat(
            "scale: {} -> {} (entry {})", oldVal, scale, entry);
        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL: {}", sql));
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sql);
        return true;
    }

    // ----- .fix addnpc -----
    // Reads the selected NPC's spawn data from the creature table (by SpawnID/guid)
    // and generates a full INSERT with a new guid = MAX(guid)+1 (to avoid
    // PK conflicts if the table doesn't use AUTO_INCREMENT).
    //
    // SQL is saved to the daily file, NOT executed.
    // The file can be run later to recreate the spawn on this or another server.
    //
    // Example output in the file:
    //   INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, ...) VALUES (12346, 1976, 0, 12, ...);
    static bool HandleFixAddNpc(ChatHandler* h)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint64 spawnId = c->GetSpawnId();
        if (spawnId == 0)
        {
            h->SendSysMessage("[gm_fixer] Selected creature has no SpawnId (probably a temporary summon).");
            return true;
        }

        // Read all spawn columns from creature.
        // guid will be replaced with MAX(guid)+1 when generating SQL.
        QueryResult result = WorldDatabase.PQuery(
            "SELECT `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, "
            "`phaseUseFlags`, `phaseId`, `phaseGroup`, `equipment_id`, "
            "`position_x`, `position_y`, `position_z`, `orientation`, "
            "`spawntimesecs`, `wander_distance`, `MovementType`, `ScriptName`, `StringId` "
            "FROM `creature` WHERE `guid` = {}",
            spawnId);

        if (!result)
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] addnpc: no creature row found for guid {}", spawnId));
            return true;
        }

        Field* fields = result->Fetch();

        // Column names in the same order as in the SELECT above.
        static constexpr char const* colNames[] = {
            "id", "map", "zoneId", "areaId", "spawnDifficulties",
            "phaseUseFlags", "phaseId", "phaseGroup", "equipment_id",
            "position_x", "position_y", "position_z", "orientation",
            "spawntimesecs", "wander_distance", "MovementType", "ScriptName", "StringId"
        };
        static constexpr uint32 numCols = 18;

        // Which columns are strings (need quotes in SQL).
        static constexpr bool colIsString[] = {
            false, false, false, false, true,   // id, map, zoneId, areaId, spawnDifficulties
            false, false, false, false,          // phaseUseFlags, phaseId, phaseGroup, equipment_id
            false, false, false, false,          // position_x, y, z, orientation
            false, false, false, true,  false    // spawntimesecs, wander_distance, MovementType, ScriptName, StringId
        };

        // Build columns and VALUES.
        // guid is calculated dynamically at SQL execution time via a subquery,
        // so the file works on any server with its own guids.
        //
        // MySQL Error 1093 (can't specify target table 'creature' for update
        // in FROM clause) is worked around via a derived table:
        //   FROM (SELECT `guid` FROM `creature`) AS tmp
        // MySQL materializes the subquery before INSERT and doesn't count it
        // as a reference to the target table.
        std::string colList = "(`guid`";
        std::string valList = "((SELECT IFNULL(MAX(`guid`), 0) + 1 FROM (SELECT `guid` FROM `creature`) AS tmp)";
        for (uint32 i = 0; i < numCols; ++i)
        {
            colList += ", `";
            colList += colNames[i];
            colList += "`";

            std::string val = fields[i].GetString();
            if (colIsString[i])
            {
                // Escape single quotes (SQL escaping).
                for (size_t pos = 0; (pos = val.find('\'', pos)) != std::string::npos; pos += 2)
                    val.insert(pos, 1, '\'');
                valList += ", '";
                valList += val;
                valList += "'";
            }
            else
            {
                // Numeric fields - as-is. If NULL/empty, use 0.
                if (val.empty())
                    val = "0";
                valList += ", ";
                valList += val;
            }
        }
        colList += ")";
        valList += ")";

        std::string sql = "INSERT INTO `creature` " + colList + " VALUES " + valList + ";";

        // Save to the daily file. NOT executed — the user will run the file themselves.
        AppendFixLog(h, sql, "addnpc",
            Trinity::StringFormat("entry {} (source guid {})", entry, spawnId));

        h->SendSysMessage(Trinity::StringFormat(
            "[gm_fixer] addnpc: saved INSERT for entry {} (source guid {}) to daily SQL file",
            entry, spawnId));
        h->SendSysMessage(Trinity::StringFormat("  SQL: {}", sql));
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] addnpc: entry {} spawn {} | SQL: {}", entry, spawnId, sql);
        return true;
    }

    // ----- .fix npcinfo -----
    // Shows information about the selected NPC:
    //   entry, name, faction (from creature_template)
    //   whether a row exists in creature_template_difficulty
    //   current ContentTuningID, StaticFlags1, HealthModifier
    // Does not write anything to the DB or daily file - read and display only.
    static bool HandleFixNpcInfo(ChatHandler* h)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // 1) creature_template - faction, npcflag, name
        QueryResult tplRes = WorldDatabase.PQuery(
            "SELECT name, faction, npcflag, unit_flags FROM creature_template WHERE entry = {}",
            entry);
        if (!tplRes)
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] creature_template entry {} not found.", entry));
            return true;
        }
        Field* tf = tplRes->Fetch();
        std::string name = tf[0].GetString();
        uint32     faction = tf[1].GetUInt32();
        uint32     npcflag = tf[2].GetUInt32();
        uint32     uflags = tf[3].GetUInt32();

        h->SendSysMessage(Trinity::StringFormat(
            "[gm_fixer] NPC entry={} name='{}'", entry, name));
        h->SendSysMessage(Trinity::StringFormat(
            "  creature_template: faction={} npcflag={} unit_flags={}",
            faction, npcflag, uflags));

        // 2) creature_template_difficulty - all rows (normal/heroic/mythic).
        QueryResult diffRes = WorldDatabase.PQuery(
            "SELECT DifficultyID, ContentTuningID, StaticFlags1, HealthModifier, "
            "       HealthScalingExpansion "
            "FROM creature_template_difficulty WHERE Entry = {} ORDER BY DifficultyID",
            entry);
        if (!diffRes)
        {
            h->SendSysMessage("  creature_template_difficulty: NO ROW (use .fix ct to create)");
        }
        else
        {
            h->SendSysMessage(Trinity::StringFormat(
                "  creature_template_difficulty ({} rows):",
                uint32(diffRes->GetRowCount())));
            do
            {
                Field* df = diffRes->Fetch();
                uint32 diffId = df[0].GetUInt32();
                uint32 ctId = df[1].GetUInt32();
                uint32 staticF1 = df[2].GetUInt32();
                float  hpMod = df[3].GetFloat();
                uint32 hpScale = df[4].GetUInt32();
                h->SendSysMessage(Trinity::StringFormat(
                    "    diff={}: ContentTuningID={} StaticFlags1={} HPx={} HpScalingExp={}",
                    diffId, ctId, staticF1, hpMod, hpScale));
            } while (diffRes->NextRow());
        }
        return true;
    }

    // ----- .fix npcfind <entity_id> -----
    // Shows all spawn GUIDs of the given creature entry (creature.id = entity_id).
    // Lists guid + map + position so the GM can tell the spawns apart.
    // Does not write to the DB or daily file — read only.
    // Does not require a selected NPC (unlike most .fix commands).
    static bool HandleFixNpcFind(ChatHandler* h, uint32 entry)
    {
        // Show the entry's name if it exists, for context.
        if (QueryResult nameRes = WorldDatabase.PQuery(
            "SELECT name FROM creature_template WHERE entry = {}", entry))
        {
            std::string name = nameRes->Fetch()[0].GetString();
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] spawns of entry {} ('{}'):", entry, name));
        }
        else
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] entry {} not found in creature_template, searching spawns anyway:", entry));
        }

        QueryResult res = WorldDatabase.PQuery(
            "SELECT guid, map, position_x, position_y, position_z "
            "FROM creature WHERE id = {} ORDER BY guid", entry);

        if (!res)
        {
            h->SendSysMessage("  no spawns found.");
            return true;
        }

        uint32 count = 0;
        do
        {
            Field* f = res->Fetch();
            uint64 guid = f[0].GetUInt64();
            uint32 map  = f[1].GetUInt32();
            float  x    = f[2].GetFloat();
            float  y    = f[3].GetFloat();
            float  z    = f[4].GetFloat();
            h->SendSysMessage(Trinity::StringFormat(
                "  guid {} | map {} | ({:.1f}, {:.1f}, {:.1f})", guid, map, x, y, z));
            ++count;
        } while (res->NextRow());

        h->SendSysMessage(Trinity::StringFormat(
            "  Found {} spawn(s).", count));
        return true;
    }

    // ----- .fix questinfo <questId> -----
    // Shows PrevQuestID from quest_template_addon - this is the quest that
    // must be completed before this quest becomes available.
    // Does not write to the DB - read only.
    static bool HandleFixQuestInfo(ChatHandler* h, uint32 questId)
    {

        // First check that the quest exists at all.
        Quest const* q = sObjectMgr->GetQuestTemplate(questId);
        if (!q)
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] Quest {} not found in quest_template.", questId));
            return true;
        }
        h->SendSysMessage(Trinity::StringFormat(
            "[gm_fixer] Quest {}: '{}'", questId, q->GetLogTitle()));

        // Questgivers (NPCs that START this quest) — creature_queststarter.
        // For each entry show the template name and every spawn guid + position,
        // so the GM can locate the giver in the world.
        QueryResult giverRes = WorldDatabase.PQuery(
            "SELECT cqs.id AS entry, ct.name, cr.guid, cr.map, "
            "       cr.position_x, cr.position_y, cr.position_z "
            "FROM creature_queststarter cqs "
            "LEFT JOIN creature_template ct ON ct.entry = cqs.id "
            "LEFT JOIN creature cr ON cr.id = cqs.id "
            "WHERE cqs.quest = {} "
            "ORDER BY cqs.id, cr.guid",
            questId);

        if (!giverRes)
        {
            h->SendSysMessage("  Questgiver: NONE (quest is not offered by any creature)");
        }
        else
        {
            // Collect unique giver entries (a creature entry can have many spawns).
            std::set<uint32> entries;
            uint32 spawnCount = 0;

            do
            {
                Field* g = giverRes->Fetch();
                uint32 entry = g[0].GetUInt32();
                std::string name = g[1].GetString();
                uint64 guid = g[2].GetUInt64();
                uint32 map = g[3].GetUInt32();
                float x = g[4].GetFloat();
                float y = g[5].GetFloat();
                float z = g[6].GetFloat();

                entries.insert(entry);
                if (guid)
                    ++spawnCount;

                // Per-spawn detail line.
                h->SendSysMessage(Trinity::StringFormat(
                    "    guid {} (entry {} '{}') map {} ({:.1f}, {:.1f}, {:.1f})",
                    guid, entry, name, map, x, y, z));
            } while (giverRes->NextRow());

            h->SendSysMessage(Trinity::StringFormat(
                "  Questgivers: {} creature entry/entry(s), {} spawn(s) in total.",
                uint32(entries.size()), spawnCount));
        }

        // Read quest_template_addon — PrevQuestID (and also NextQuestID,
        // BreadcrumbForQuestId, AllowableClasses — for context).
        QueryResult res = WorldDatabase.PQuery(
            "SELECT PrevQuestID, NextQuestID, ExclusiveGroup, BreadcrumbForQuestId, "
            "       AllowableClasses, RequiredSkillID, RequiredSkillPoints "
            "FROM quest_template_addon WHERE ID = {}",
            questId);
        if (!res)
        {
            h->SendSysMessage("  quest_template_addon: NO ROW (no prerequisite chain)");
            return true;
        }

        Field* f = res->Fetch();
        int32  prevQ = f[0].GetInt32();
        uint32 nextQ = f[1].GetUInt32();
        int32  exclGroup = f[2].GetInt32();
        uint32 bcFor = f[3].GetUInt32();
        uint32 allowCls = f[4].GetUInt32();
        uint32 reqSkill = f[5].GetUInt32();
        uint32 reqSkillP = f[6].GetUInt32();

        if (prevQ == 0)
        {
            h->SendSysMessage("  PrevQuestID: 0 (no prerequisite)");
        }
        else
        {
            // If prevQ > 0 - this quest must be completed.
            // If prevQ < 0 - this quest must NOT be completed (exclusive).
            if (prevQ > 0)
            {
                Quest const* prev = sObjectMgr->GetQuestTemplate(uint32(prevQ));
                h->SendSysMessage(Trinity::StringFormat(
                    "  PrevQuestID: {} (must be completed) - '{}'",
                    prevQ, prev ? prev->GetLogTitle() : "<unknown>"));
            }
            else
            {
                Quest const* prev = sObjectMgr->GetQuestTemplate(uint32(-prevQ));
                h->SendSysMessage(Trinity::StringFormat(
                    "  PrevQuestID: {} (must NOT be completed) - '{}'",
                    prevQ, prev ? prev->GetLogTitle() : "<unknown>"));
            }
        }

        if (nextQ)     h->SendSysMessage(Trinity::StringFormat("  NextQuestID: {}", nextQ));
        if (exclGroup) h->SendSysMessage(Trinity::StringFormat("  ExclusiveGroup: {}", exclGroup));
        if (bcFor)     h->SendSysMessage(Trinity::StringFormat("  BreadcrumbForQuestId: {}", bcFor));
        if (allowCls)  h->SendSysMessage(Trinity::StringFormat("  AllowableClasses: {}", allowCls));
        if (reqSkill)  h->SendSysMessage(Trinity::StringFormat("  RequiredSkill: {} ({} points)", reqSkill, reqSkillP));

        return true;
    }

    // ----- .fix spellgcd <SpellID> [seconds] -----
    // Sets StartRecoveryTime (GCD) for a spell in hotfixes.spell_cooldowns.
    // Seconds are specified as a fraction: "1.5" or "1,5" = 1500 ms.
    // If seconds are not specified - default 1500 ms (1.5s, standard GCD) is used.
    //
    // SQL: INSERT INTO hotfixes.spell_cooldowns
    //        (StartRecoveryTime, SpellID, VerifiedBuild)
    //      VALUES (<ms>, <spellId>, 67186);
    static bool HandleFixSpellGCD(ChatHandler* h, uint32 spellId, Optional<std::string_view> secondsOpt)
    {
        std::string secondsStr = secondsOpt ? std::string(*secondsOpt) : "";

        uint32 ms = 0;
        // Default GCD = 1500 ms (1.5 seconds).
        if (!ParseSecondsToMs(secondsStr.c_str(), 1500, ms))
        {
            h->SendSysMessage("Usage: .fix spellgcd <SpellID> [seconds]");
            h->SendSysMessage("  seconds can be '1.5', '1,5', '0.5', etc.");
            return true;
        }

        // Check that the spell exists.
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo)
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] Spell {} not found in SpellMgr.", spellId));
            return true;
        }

        // INSERT with ON DUPLICATE KEY UPDATE - atomic, without deleting the row.
        // IMPORTANT: PK of the spell_cooldowns table = (ID, VerifiedBuild), not SpellID!
        // ID is calculated as MAX(ID)+1 to avoid conflicts with other hotfix entries.
        // To prevent repeated runs of the same file from creating new rows,
        // wrap the entire INSERT in a condition: execute only if no record exists
        // for this SpellID with this VerifiedBuild yet.
        //
        // But MySQL can't do IF NOT EXISTS in a single atomic statement,
        // so we use INSERT ... SELECT with WHERE NOT EXISTS - this is
        // atomic and won't fail on a duplicate (SELECT simply returns 0 rows).
        std::string sql = Trinity::StringFormat(
            "INSERT INTO `spell_cooldowns` "
            "(`ID`, `DifficultyID`, `CategoryRecoveryTime`, `RecoveryTime`, "
            " `StartRecoveryTime`, `AuraSpellID`, `SpellID`, `VerifiedBuild`) "
            "SELECT (SELECT IFNULL(MAX(`ID`), 0) + 1 FROM `spell_cooldowns`), "
            "       0, 0, 0, {}, 0, {}, {} "
            "FROM DUAL "
            "WHERE NOT EXISTS ("
            "  SELECT 1 FROM `spell_cooldowns` "
            "  WHERE `SpellID` = {} AND `VerifiedBuild` = {}"
            ");",
            ms, spellId, HOTFIX_VERIFIED_BUILD,
            spellId, HOTFIX_VERIFIED_BUILD);

        // If the record already exists - UPDATE it via a separate query.
        // (INSERT ... ON DUPLICATE KEY UPDATE doesn't work since ID isn't known yet.)
        std::string sqlUpd = Trinity::StringFormat(
            "UPDATE `spell_cooldowns` SET `StartRecoveryTime` = {} "
            "WHERE `SpellID` = {} AND `VerifiedBuild` = {};",
            ms, spellId, HOTFIX_VERIFIED_BUILD);

        // Execute both queries sequentially.
        HotfixDatabase.DirectExecute(sql.c_str());
        HotfixDatabase.DirectExecute(sqlUpd.c_str());

        // Write both statements to the log file (for audit).
        std::string sqlLog = sql + "\n" + sqlUpd;

        // Write to file and log (SQL already executed above).
        AppendToSpellCdFile(sqlLog, "spellgcd");
        std::string what = Trinity::StringFormat(
            "SpellGCD spell={} -> {} ms ({} sec)",
            spellId, ms, (ms / 1000.0));
        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL:\n{}", sqlLog));
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sqlLog);
        return true;
    }

    // ----- .fix spellcd <SpellID> <seconds> -----
    // Sets RecoveryTime (cooldown) for a spell in hotfixes.spell_cooldowns.
    // Seconds are specified as a fraction: "1.5" or "1,5" = 1500 ms.
    // Unlike spellgcd - seconds are required (no default).
    //
    // SQL: INSERT INTO hotfixes.spell_cooldowns
    //        (RecoveryTime, SpellID, VerifiedBuild)
    //      VALUES (<ms>, <spellId>, 67186);
    static bool HandleFixSpellCD(ChatHandler* h, uint32 spellId, std::string_view secondsStr)
    {
        std::string seconds(secondsStr);
        uint32 ms = 0;
        // For spellcd there is no default - if not specified, error.
        if (!ParseSecondsToMs(seconds.c_str(), 0, ms) || ms == 0)
        {
            h->SendSysMessage("Usage: .fix spellcd <SpellID> <seconds>");
            h->SendSysMessage("  seconds can be '1.5', '1,5', '10', etc. (must be > 0)");
            return true;
        }

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo)
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] Spell {} not found in SpellMgr.", spellId));
            return true;
        }

        // PK = (ID, VerifiedBuild), ID = MAX(ID)+1 via subquery.
        std::string sql = Trinity::StringFormat(
            "INSERT INTO `spell_cooldowns` "
            "(`ID`, `DifficultyID`, `CategoryRecoveryTime`, `RecoveryTime`, "
            " `StartRecoveryTime`, `AuraSpellID`, `SpellID`, `VerifiedBuild`) "
            "SELECT (SELECT IFNULL(MAX(`ID`), 0) + 1 FROM `spell_cooldowns`), "
            "       0, 0, {}, 0, 0, {}, {} "
            "FROM DUAL "
            "WHERE NOT EXISTS ("
            "  SELECT 1 FROM `spell_cooldowns` "
            "  WHERE `SpellID` = {} AND `VerifiedBuild` = {}"
            ");",
            ms, spellId, HOTFIX_VERIFIED_BUILD,
            spellId, HOTFIX_VERIFIED_BUILD);

        std::string sqlUpd = Trinity::StringFormat(
            "UPDATE `spell_cooldowns` SET `RecoveryTime` = {} "
            "WHERE `SpellID` = {} AND `VerifiedBuild` = {};",
            ms, spellId, HOTFIX_VERIFIED_BUILD);

        HotfixDatabase.DirectExecute(sql.c_str());
        HotfixDatabase.DirectExecute(sqlUpd.c_str());

        std::string sqlLog = sql + "\n" + sqlUpd;

        AppendToSpellCdFile(sqlLog, "spellcd");
        std::string what = Trinity::StringFormat(
            "SpellCD spell={} -> {} ms ({} sec)",
            spellId, ms, (ms / 1000.0));
        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL:\n{}", sqlLog));
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sqlLog);
        return true;
    }

    // =====================================================================
    //  Equipment — creature_equip_template
    // =====================================================================
    // The table stores per-template equipment sets, PK = (CreatureID, ID).
    //   CreatureID — creature_template.entry (NOT a spawn guid)
    //   ID         — equipment set number referenced by creature.equipment_id
    //                (default 1; id 0 is rejected by ObjectMgr on load)
    // Each set has 3 slots, each slot = (ItemID, AppearanceModID, ItemVisual):
    //   slot 1: ItemID1, AppearanceModID1, ItemVisual1
    //   slot 2: ItemID2, AppearanceModID2, ItemVisual2
    //   slot 3: ItemID3, AppearanceModID3, ItemVisual3
    //
    // Affects ALL spawns of the entry. Applied at runtime by reloading the
    // ObjectMgr cache and calling Creature::LoadEquipment on the target.

    // The ID of the equipment row we operate on. Defaults to 1 (standard set).
    // A creature without a custom equipment_id uses set 1, so that is the row
    // a GM almost always wants to edit. We read creature.equipment_id of the
    // spawn to stay on the set the selected NPC actually wears.
    static constexpr uint8 EQUIP_DEFAULT_ID = 1;

    // Resolves which equipment set ID (creature_equip_template.ID) to edit
    // for the selected NPC: the spawn's own equipment_id, falling back to 1.
    static uint8 ResolveEquipRowId(Creature* c, uint32 entry)
    {
        uint8 setId = EQUIP_DEFAULT_ID;
        if (CreatureData const* data = sObjectMgr->GetCreatureData(c->GetSpawnId()))
        {
            if (data->equipmentId > 0)
                setId = static_cast<uint8>(data->equipmentId);
        }
        (void)entry;
        return setId;
    }

    // Sets a single column of a single slot in creature_equip_template.
    //   slot  — 1..3
    //   field — "ItemID" | "AppearanceModID" | "ItemVisual"
    //   value — new value (32-bit covers all column types)
    //
    // If the (CreatureID, ID) row exists — UPDATE only that column
    // (the other slots keep their values).
    // If the row does not exist — INSERT a fresh row with default 0s in every
    // slot, then the requested slot takes `value`.
    static bool EquipSetItemField(ChatHandler* h, uint8 slot, std::string_view field, uint32 value)
    {
        // Validate slot and map it to the numeric suffix used in column names.
        if (slot < 1 || slot > 3)
        {
            h->SendSysMessage("[gm_fixer] slot must be 1, 2 or 3");
            return true;
        }
        std::string fieldCol(field);
        if (fieldCol != "ItemID" && fieldCol != "AppearanceModID" && fieldCol != "ItemVisual")
        {
            h->SendSysMessage("[gm_fixer] internal: unknown equipment field");
            return true;
        }

        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint8 setId = ResolveEquipRowId(c, entry);

        // Does a row for this (CreatureID, ID) already exist?
        QueryResult res = WorldDatabase.PQuery(
            "SELECT 1 FROM creature_equip_template WHERE CreatureID = {} AND ID = {} LIMIT 1",
            entry, setId);

        std::string sql;
        std::string what;
        if (res)
        {
            // Row exists — UPDATE only the requested column of the requested slot.
            sql = Trinity::StringFormat(
                "UPDATE creature_equip_template SET {}{} = {} "
                "WHERE CreatureID = {} AND ID = {};",
                fieldCol, uint32(slot), value, entry, setId);
            what = Trinity::StringFormat(
                "equip {}{} = {} (entry {}, set {}, UPDATE)", fieldCol, uint32(slot), value, entry, setId);
        }
        else
        {
            // No row — INSERT a fresh set with default 0s, then the requested
            // slot/column takes `value`. Other slots stay 0 (empty hands).
            uint32 i1 = 0, a1 = 0, v1 = 0;
            uint32 i2 = 0, a2 = 0, v2 = 0;
            uint32 i3 = 0, a3 = 0, v3 = 0;
            switch (slot)
            {
                case 1:
                    if (fieldCol == "ItemID")           i1 = value;
                    else if (fieldCol == "AppearanceModID") a1 = value;
                    else                                 v1 = value;
                    break;
                case 2:
                    if (fieldCol == "ItemID")           i2 = value;
                    else if (fieldCol == "AppearanceModID") a2 = value;
                    else                                 v2 = value;
                    break;
                case 3:
                    if (fieldCol == "ItemID")           i3 = value;
                    else if (fieldCol == "AppearanceModID") a3 = value;
                    else                                 v3 = value;
                    break;
            }

            sql = Trinity::StringFormat(
                "INSERT INTO `creature_equip_template` "
                "(`CreatureID`, `ID`, `ItemID1`, `AppearanceModID1`, `ItemVisual1`, "
                " `ItemID2`, `AppearanceModID2`, `ItemVisual2`, "
                " `ItemID3`, `AppearanceModID3`, `ItemVisual3`, `VerifiedBuild`) "
                "VALUES ({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});",
                entry, setId, i1, a1, v1, i2, a2, v2, i3, a3, v3, HOTFIX_VERIFIED_BUILD);
            what = Trinity::StringFormat(
                "equip {}{} = {} (entry {}, set {}, INSERT)", fieldCol, slot, value, entry, setId);
        }

        // Execute + log, then refresh the runtime cache and the target NPC.
        WorldDatabase.DirectExecute(sql.c_str());
        AppendFixLog(h, sql, "equip", what);

        // Reload ObjectMgr equipment cache so the new values are visible.
        sObjectMgr->LoadEquipmentTemplates();
        // Re-apply visuals on the selected NPC for its current equipment set.
        c->LoadEquipment(static_cast<int8>(setId), true);

        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL: {}", sql));
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sql);
        return true;
    }

    // Sets a full slot (ItemID, AppearanceModID, ItemVisual) at once.
    // Used by .fix item1/2/3 [itemid]. With no itemid — the slot is cleared (0).
    static bool EquipSetItemFull(ChatHandler* h, uint8 slot, Optional<uint32> itemIdOpt)
    {
        if (slot < 1 || slot > 3)
        {
            h->SendSysMessage("[gm_fixer] slot must be 1, 2 or 3");
            return true;
        }

        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint8 setId = ResolveEquipRowId(c, entry);
        uint32 itemId = itemIdOpt.value_or(0);

        // Does a row for this (CreatureID, ID) already exist?
        bool exists = static_cast<bool>(WorldDatabase.PQuery(
            "SELECT 1 FROM creature_equip_template WHERE CreatureID = {} AND ID = {} LIMIT 1",
            entry, setId));

        std::string sql;
        std::string what;
        if (exists)
        {
            // Row exists — UPDATE only the three fields of the requested slot,
            // leaving the other slots untouched.
            sql = Trinity::StringFormat(
                "UPDATE creature_equip_template "
                "SET ItemID{} = {}, AppearanceModID{} = 0, ItemVisual{} = 0 "
                "WHERE CreatureID = {} AND ID = {};",
                uint32(slot), itemId, uint32(slot), uint32(slot), entry, setId);
            what = Trinity::StringFormat(
                "equip slot {} -> ItemID={} (entry {}, set {}, UPDATE)",
                uint32(slot), itemId, entry, setId);
        }
        else
        {
            // No row — INSERT a fresh set with only the requested slot set,
            // the rest stay 0 (empty).
            uint32 i1 = 0, a1 = 0, v1 = 0;
            uint32 i2 = 0, a2 = 0, v2 = 0;
            uint32 i3 = 0, a3 = 0, v3 = 0;
            switch (slot)
            {
                case 1: i1 = itemId; break;
                case 2: i2 = itemId; break;
                case 3: i3 = itemId; break;
            }
            sql = Trinity::StringFormat(
                "INSERT INTO `creature_equip_template` "
                "(`CreatureID`, `ID`, `ItemID1`, `AppearanceModID1`, `ItemVisual1`, "
                " `ItemID2`, `AppearanceModID2`, `ItemVisual2`, "
                " `ItemID3`, `AppearanceModID3`, `ItemVisual3`, `VerifiedBuild`) "
                "VALUES ({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});",
                entry, setId, i1, a1, v1, i2, a2, v2, i3, a3, v3, HOTFIX_VERIFIED_BUILD);
            what = Trinity::StringFormat(
                "equip slot {} -> ItemID={} (entry {}, set {}, INSERT)",
                uint32(slot), itemId, entry, setId);
        }

        WorldDatabase.DirectExecute(sql.c_str());
        AppendFixLog(h, sql, "equip", what);

        sObjectMgr->LoadEquipmentTemplates();
        c->LoadEquipment(static_cast<int8>(setId), true);

        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL: {}", sql));
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sql);
        return true;
    }

    // ----- .fix item1/2/3 [itemid] -----
    // Sets a full equipment slot (ItemID + clears Appearance/Visual) for the
    // selected NPC's equipment set. No argument clears the slot.
    static bool HandleFixItemFull1(ChatHandler* h, Optional<uint32> itemId) { return EquipSetItemFull(h, 1, itemId); }
    static bool HandleFixItemFull2(ChatHandler* h, Optional<uint32> itemId) { return EquipSetItemFull(h, 2, itemId); }
    static bool HandleFixItemFull3(ChatHandler* h, Optional<uint32> itemId) { return EquipSetItemFull(h, 3, itemId); }

    // ----- .fix itemid1/2/3 <ID> -----
    // Sets only ItemID for slot N, leaving Appearance/Visual untouched.
    static bool HandleFixItemId1(ChatHandler* h, uint32 value) { return EquipSetItemField(h, 1, "ItemID", value); }
    static bool HandleFixItemId2(ChatHandler* h, uint32 value) { return EquipSetItemField(h, 2, "ItemID", value); }
    static bool HandleFixItemId3(ChatHandler* h, uint32 value) { return EquipSetItemField(h, 3, "ItemID", value); }

    // ----- .fix itemapp1/2/3 <ID> -----
    // Sets only AppearanceModID for slot N, leaving ItemID/Visual untouched.
    static bool HandleFixItemApp1(ChatHandler* h, uint32 value) { return EquipSetItemField(h, 1, "AppearanceModID", value); }
    static bool HandleFixItemApp2(ChatHandler* h, uint32 value) { return EquipSetItemField(h, 2, "AppearanceModID", value); }
    static bool HandleFixItemApp3(ChatHandler* h, uint32 value) { return EquipSetItemField(h, 3, "AppearanceModID", value); }

    // ----- .fix itemvis1/2/3 <ID> -----
    // Sets only ItemVisual for slot N, leaving ItemID/Appearance untouched.
    static bool HandleFixItemVis1(ChatHandler* h, uint32 value) { return EquipSetItemField(h, 1, "ItemVisual", value); }
    static bool HandleFixItemVis2(ChatHandler* h, uint32 value) { return EquipSetItemField(h, 2, "ItemVisual", value); }
    static bool HandleFixItemVis3(ChatHandler* h, uint32 value) { return EquipSetItemField(h, 3, "ItemVisual", value); }

    // ----- .fix iteminfo -----
    // Shows all creature_equip_template rows for the selected NPC's entry,
    // and highlights the set the selected spawn currently wears.
    // Read-only — does not write to the DB or daily file.
    static bool HandleFixItemInfo(ChatHandler* h)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint8 curSet = ResolveEquipRowId(c, entry);

        QueryResult res = WorldDatabase.PQuery(
            "SELECT ID, ItemID1, AppearanceModID1, ItemVisual1, "
            "       ItemID2, AppearanceModID2, ItemVisual2, "
            "       ItemID3, AppearanceModID3, ItemVisual3, VerifiedBuild "
            "FROM creature_equip_template WHERE CreatureID = {} ORDER BY ID",
            entry);

        if (!res)
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] entry {}: NO rows in creature_equip_template", entry));
            h->SendSysMessage("  Use .fix itemid1/2/3 <id> to create one.");
            return true;
        }

        h->SendSysMessage(Trinity::StringFormat(
            "[gm_fixer] creature_equip_template for entry {} (spawn wears set {}):",
            entry, uint32(curSet)));
        do
        {
            Field* f = res->Fetch();
            uint32 id  = f[0].GetUInt32();
            uint32 i1  = f[1].GetUInt32(); uint32 a1 = f[2].GetUInt32(); uint32 v1 = f[3].GetUInt32();
            uint32 i2  = f[4].GetUInt32(); uint32 a2 = f[5].GetUInt32(); uint32 v2 = f[6].GetUInt32();
            uint32 i3  = f[7].GetUInt32(); uint32 a3 = f[8].GetUInt32(); uint32 v3 = f[9].GetUInt32();
            int32  vb  = f[10].GetInt32();
            char const* mark = (id == uint32(curSet)) ? " *" : "";
            h->SendSysMessage(Trinity::StringFormat(
                "  set {}{}: slot1={}/{}/{}, slot2={}/{}/{}, slot3={}/{}/{}, VerifiedBuild={}",
                id, mark,
                i1, a1, v1,
                i2, a2, v2,
                i3, a3, v3, vb));
        } while (res->NextRow());

        h->SendSysMessage("  (* = the set the selected spawn currently wears)");
        return true;
    }

    // ----- .fix gequip <ID> -----
    // Sets creature.equipment_id = ID for ALL spawns of the selected NPC's entry.
    // creature.equipment_id references creature_equip_template.ID (the equipment
    // set number). Affects every spawn of the entry (by creature.id = entry),
    // not just the selected one.
    //
    // SQL: UPDATE creature SET equipment_id = <ID> WHERE id = <entry>;
    //
    // The DB update applies to all spawns; for immediate feedback the selected
    // NPC is re-equipped at runtime. Other spawned instances pick up the change
    // on next respawn (.respawn).
    //
    // ID = 0 means "no equipment" (creature wears nothing).
    static bool HandleFixGEquip(ChatHandler* h, uint8 id)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // Read old value from one of the entry's spawn rows (for the message).
        int8 oldVal = 0;
        if (QueryResult r = WorldDatabase.PQuery(
            "SELECT equipment_id FROM creature WHERE id = {} LIMIT 1", entry))
            oldVal = r->Fetch()[0].GetInt8();

        std::string sql = Trinity::StringFormat(
            "UPDATE creature SET equipment_id = {} WHERE id = {};", id, entry);
        WorldDatabase.DirectExecute(sql.c_str());
        AppendFixLog(h, sql, "gequip",
            Trinity::StringFormat("equipment_id: {} -> {} (ALL spawns entry {})", int32(oldVal), int32(id), entry));

        // Re-equip the selected NPC at runtime for immediate feedback.
        // id == 0 clears equipment (empty hands).
        c->LoadEquipment(static_cast<int8>(id), true);

        std::string what = Trinity::StringFormat(
            "equipment_id: {} -> {} (ALL spawns entry {})", int32(oldVal), int32(id), entry);
        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL: {}", sql));
        if (id > 0)
            h->SendSysMessage("  Selected NPC re-equipped now; other spawns update on .respawn");
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sql);
        return true;
    }

    // ----- .fix cmount <mountId> -----
    // Sets the mount display ID in creature_addon for a specific spawn (by guid).
    // If no row exists — INSERT a new one with guid + mount (remaining columns
    // take DEFAULT from the table schema).
    //
    // SQL:
    //   UPDATE: UPDATE creature_addon SET mount = X WHERE guid = <spawnId>;
    //   INSERT: INSERT INTO creature_addon (guid, mount) VALUES (<spawnId>, X);
    static bool HandleFixCmount(ChatHandler* h, uint32 mountId)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint64 spawnId = c->GetSpawnId();
        if (spawnId == 0)
        {
            h->SendSysMessage("[gm_fixer] Selected creature has no SpawnId (probably a temporary summon).");
            return true;
        }

        // Check if a row exists in creature_addon.
        QueryResult res = WorldDatabase.PQuery(
            "SELECT 1 FROM creature_addon WHERE guid = {} LIMIT 1", spawnId);

        std::string sql;
        std::string what;
        if (res)
        {
            // Row exists — UPDATE.
            sql = Trinity::StringFormat(
                "UPDATE creature_addon SET mount = {} WHERE guid = {};",
                mountId, spawnId);
            what = Trinity::StringFormat(
                "mount -> {} (spawn guid {}, UPDATE)",
                mountId, spawnId);
        }
        else
        {
            // No row — INSERT with guid + mount.
            // Remaining columns take DEFAULT from the schema (StandState=0, emote=0, etc.).
            sql = Trinity::StringFormat(
                "INSERT INTO creature_addon (guid, mount) VALUES ({}, {});",
                spawnId, mountId);
            what = Trinity::StringFormat(
                "mount -> {} (spawn guid {}, INSERT)",
                mountId, spawnId);
        }
        return ApplyFix(h, sql, "cmount", what.c_str());
    }

    // ----- .fix gmount <mountId> -----
    // Sets the mount display ID in creature_template_addon for the template (by entry).
    // Affects ALL spawns of this entry.
    // If no row exists — INSERT a new one with Entry + mount (remaining columns
    // take DEFAULT from the table schema).
    //
    // SQL:
    //   UPDATE: UPDATE creature_template_addon SET mount = X WHERE Entry = <entry>;
    //   INSERT: INSERT INTO creature_template_addon (Entry, mount) VALUES (<entry>, X);
    static bool HandleFixGmount(ChatHandler* h, uint32 mountId)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // Check if a row exists in creature_template_addon.
        QueryResult res = WorldDatabase.PQuery(
            "SELECT 1 FROM creature_template_addon WHERE Entry = {} LIMIT 1", entry);

        std::string sql;
        std::string what;
        if (res)
        {
            // Row exists — UPDATE.
            sql = Trinity::StringFormat(
                "UPDATE creature_template_addon SET mount = {} WHERE Entry = {};",
                mountId, entry);
            what = Trinity::StringFormat(
                "mount -> {} (entry {}, UPDATE)",
                mountId, entry);
        }
        else
        {
            // No row — INSERT with Entry + mount.
            // Remaining columns take DEFAULT from the schema.
            sql = Trinity::StringFormat(
                "INSERT INTO creature_template_addon (Entry, mount) VALUES ({}, {});",
                entry, mountId);
            what = Trinity::StringFormat(
                "mount -> {} (entry {}, INSERT)",
                mountId, entry);
        }
        return ApplyFix(h, sql, "gmount", what.c_str());
    }

    // ----- .fix gsheath <sheathState> -----
    // Sets SheathState in creature_addon for a specific spawn (by SpawnId).
    // If no row exists — INSERT a new one.
    //
    // SQL:
    //   UPDATE: UPDATE creature_addon SET SheathState = X WHERE guid = <spawnId>;
    //   INSERT: INSERT INTO creature_addon (guid, SheathState) VALUES (<spawnId>, X);
    //
    // Common SheathState values (SharedDefines.h SheathState):
    //   0 = SHEATH_STATE_UNARMED (weapon hidden)
    //   1 = SHEATH_STATE_MELEE   (melee weapon drawn)
    //   2 = SHEATH_STATE_RANGED  (ranged weapon drawn)
    static bool HandleFixGSheath(ChatHandler* h, uint8 sheath)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint64 spawnId = c->GetSpawnId();
        if (spawnId == 0)
        {
            h->SendSysMessage("[gm_fixer] Selected creature has no SpawnId (probably a temporary summon).");
            return true;
        }

        // Check if a row exists in creature_addon.
        QueryResult res = WorldDatabase.PQuery(
            "SELECT 1 FROM creature_addon WHERE guid = {} LIMIT 1", spawnId);

        std::string sql;
        std::string what;
        if (res)
        {
            sql = Trinity::StringFormat(
                "UPDATE creature_addon SET SheathState = {} WHERE guid = {};",
                uint32(sheath), spawnId);
            what = Trinity::StringFormat(
                "SheathState -> {} (spawn guid {}, UPDATE)",
                uint32(sheath), spawnId);
        }
        else
        {
            sql = Trinity::StringFormat(
                "INSERT INTO creature_addon (guid, SheathState) VALUES ({}, {});",
                spawnId, uint32(sheath));
            what = Trinity::StringFormat(
                "SheathState -> {} (spawn guid {}, INSERT)",
                uint32(sheath), spawnId);
        }

        // Apply at runtime for immediate feedback.
        c->SetSheath(SheathState(sheath));

        WorldDatabase.DirectExecute(sql.c_str());
        AppendFixLog(h, sql, "gsheath", what);
        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL: {}", sql));
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sql);
        return true;
    }

    // ----- .fix csheath <sheathState> -----
    // Sets SheathState in creature_template_addon for the template (by entry).
    // Affects ALL spawns of this entry.
    // If no row exists — INSERT a new one.
    //
    // SQL:
    //   UPDATE: UPDATE creature_template_addon SET SheathState = X WHERE Entry = <entry>;
    //   INSERT: INSERT INTO creature_template_addon (Entry, SheathState) VALUES (<entry>, X);
    static bool HandleFixCSheath(ChatHandler* h, uint8 sheath)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // Check if a row exists in creature_template_addon.
        QueryResult res = WorldDatabase.PQuery(
            "SELECT 1 FROM creature_template_addon WHERE Entry = {} LIMIT 1", entry);

        std::string sql;
        std::string what;
        if (res)
        {
            sql = Trinity::StringFormat(
                "UPDATE creature_template_addon SET SheathState = {} WHERE Entry = {};",
                uint32(sheath), entry);
            what = Trinity::StringFormat(
                "SheathState -> {} (entry {}, UPDATE)",
                uint32(sheath), entry);
        }
        else
        {
            sql = Trinity::StringFormat(
                "INSERT INTO creature_template_addon (Entry, SheathState) VALUES ({}, {});",
                entry, uint32(sheath));
            what = Trinity::StringFormat(
                "SheathState -> {} (entry {}, INSERT)",
                uint32(sheath), entry);
        }
        return ApplyFix(h, sql, "csheath", what.c_str());
    }

    // ----- .fix pooladd -----
    // Gathers ALL spawn guids of the selected NPC's entry into a brand-new
    // spawn pool, exactly the way the Badlands spawn SQL does it:
    //
    //   - a pool_template row  (entry = MAX+1, max_limit = 1, description)
    //   - one pool_members row per spawn guid
    //       (type=0 (creature), spawnId=guid, poolSpawnId=<pool>, chance=0)
    //
    // If the entry's spawns are already part of a pool — reports it and
    // does nothing (use .fix pooldel first).
    //
    // max_limit is fixed to 1 (one spawn alive at a time), matching the
    // rare-spawn / vignette pattern from the attached SQL.
    //
    // Two distinct SQL streams:
    //   1) RUNTIME — concrete pool id (computed on THIS db) is executed now so
    //      the pool works immediately.
    //   2) LOG     — the daily/zone file gets a PORTABLE template using
    //      SET @POOLID := (SELECT IFNULL(MAX(entry),0)+1 FROM pool_template);
    //      so the file can be imported on ANY database (different servers have
    //      different pool id ranges) without a PK conflict — same approach as
    //      .fix addnpc and the attached Badlands sql.
    static bool HandleFixPoolAdd(ChatHandler* h)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // Collect ALL guids of this entry. Ordered by guid so the generated
        // SQL is stable / reviewable.
        QueryResult spawnRes = WorldDatabase.PQuery(
            "SELECT guid FROM creature WHERE id = {} ORDER BY guid", entry);
        if (!spawnRes)
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] pooladd: entry {} has no spawns in `creature`.", entry));
            return true;
        }

        std::vector<uint64> guids;
        do
        {
            guids.push_back(spawnRes->Fetch()[0].GetUInt64());
        } while (spawnRes->NextRow());

        // Single spawn — a pool of one makes no sense.
        if (guids.size() < 2)
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] pooladd: entry {} has only {} spawn — nothing to pool. "
                "Need at least 2.", entry, uint32(guids.size())));
            return true;
        }

        // If any of the guids is already in a pool — refuse, otherwise we'd
        // silently move it from its current pool into the new one.
        // pool_members.spawnId is BIGINT UNSIGNED, so we can't bind a vector
        // of arbitrary size; check per-guid (the count here is small, typically
        // 2–6 spawns).
        for (uint64 g : guids)
        {
            QueryResult inPool = WorldDatabase.PQuery(
                "SELECT poolSpawnId FROM pool_members WHERE type = 0 AND spawnId = {} LIMIT 1", g);
            if (inPool)
            {
                uint32 existingPool = inPool->Fetch()[0].GetUInt32();
                h->SendSysMessage(Trinity::StringFormat(
                    "[gm_fixer] pooladd: spawn guid {} (entry {}) is already in pool {} "
                    "— use .fix pooldel first.", g, entry, existingPool));
                return true;
            }
        }

        // Next free pool_template.entry = MAX+1.
        uint32 newPoolId = 1;
        QueryResult maxRes = WorldDatabase.Query(
            "SELECT IFNULL(MAX(entry), 0) + 1 FROM pool_template");
        if (maxRes)
            newPoolId = maxRes->Fetch()[0].GetUInt32();

        // Look up the NPC name for a human-readable description.
        std::string npcName = "entry " + std::to_string(entry);
        if (QueryResult nameRes = WorldDatabase.PQuery(
            "SELECT name FROM creature_template WHERE entry = {} LIMIT 1", entry))
        {
            std::string n = nameRes->Fetch()[0].GetString();
            if (!n.empty())
                npcName = n;
        }

        // --- Runtime SQL: concrete newPoolId (computed above for THIS db). ---
        // These statements are executed now so the pool works immediately.
        std::string safeName = npcName;
        for (size_t pos = 0; (pos = safeName.find('\'', pos)) != std::string::npos; pos += 2)
            safeName.insert(pos, 1, '\'');

        std::string rtTpl = Trinity::StringFormat(
            "INSERT INTO pool_template (entry, max_limit, description) "
            "VALUES ({}, 1, 'gm_fixer: {} (entry {})');",
            newPoolId, safeName, entry);

        std::string rtMembers =
            "INSERT INTO pool_members (`type`, `spawnId`, `poolSpawnId`, `chance`, `description`) VALUES ";
        for (size_t i = 0; i < guids.size(); ++i)
        {
            rtMembers += Trinity::StringFormat(
                "(0, {}, {}, 0, 'gm_fixer: {} (entry {})')",
                guids[i], newPoolId, safeName, entry);
            if (i + 1 < guids.size())
                rtMembers += ", ";
        }
        rtMembers += ";";

        WorldDatabase.DirectExecute(rtTpl.c_str());
        WorldDatabase.DirectExecute(rtMembers.c_str());

        // --- Log SQL: portable template using @POOLID, so the file imports on ANY
        // database without a PK conflict (same idea as .fix addnpc and the
        // Badlands spawn sql which use SET @POOLID := ...). The concrete newPoolId
        // is NOT baked into the file.
        std::string logSql;
        logSql += "-- pool_template + pool_members for entry " + std::to_string(entry) +
                  " (" + safeName + ")\n";
        logSql += "SET @POOLID := (SELECT IFNULL(MAX(entry), 0) + 1 FROM pool_template);\n";
        logSql += Trinity::StringFormat(
            "INSERT INTO pool_template (entry, max_limit, description) "
            "VALUES (@POOLID, 1, 'gm_fixer: {} (entry {})');\n",
            safeName, entry);
        logSql += "INSERT INTO pool_members (`type`, `spawnId`, `poolSpawnId`, `chance`, `description`) VALUES ";
        for (size_t i = 0; i < guids.size(); ++i)
        {
            logSql += Trinity::StringFormat(
                "(0, {}, @POOLID, 0, 'gm_fixer: {} (entry {})')",
                guids[i], safeName, entry);
            if (i + 1 < guids.size())
                logSql += ", ";
        }
        logSql += ";";

        std::string what = Trinity::StringFormat(
            "pooladd: pool {} <- entry {} ({} spawns, max_limit=1)",
            newPoolId, entry, uint32(guids.size()));
        AppendFixLog(h, logSql, "pooladd", what);

        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage("  Log SQL (portable, @POOLID):");
        h->SendSysMessage(Trinity::StringFormat("  {}", logSql));
        h->SendSysMessage("  Apply in-game: .reload pool_template / .reload pool_members (or restart)");
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | Log SQL: {}", what, logSql);
        return true;
    }

    // ----- .fix pooldel -----
    // Deletes the spawn pool that the selected NPC's entry belongs to.
    //
    // Finds the pool the selected spawn's guid is a member of, then deletes
    //   - every pool_members row of that pool (all creatures in the pool)
    //   - the pool_template row itself
    //
    // Mirrors .fix pooladd in reverse. If the spawn isn't in any pool —
    // reports it and does nothing.
    //
    // Two distinct SQL streams (same idea as .fix pooladd / .fix addnpc):
    //   1) RUNTIME — the concrete pool id (found on THIS db) is deleted now.
    //   2) LOG     — the daily/zone file gets a PORTABLE template that looks the
    //      pool up dynamically by the NPC's entry at import time
    //      (any guid of the entry -> its poolSpawnId), so the file applies
    //      correctly on any server whose pool ids differ from this one.
    static bool HandleFixPoolDel(ChatHandler* h)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        uint64 spawnId = c->GetSpawnId();
        if (spawnId == 0)
        {
            h->SendSysMessage("[gm_fixer] Selected creature has no SpawnId (probably a temporary summon).");
            return true;
        }

        // Which pool does this spawn belong to?
        QueryResult poolRes = WorldDatabase.PQuery(
            "SELECT poolSpawnId FROM pool_members WHERE type = 0 AND spawnId = {} LIMIT 1", spawnId);
        if (!poolRes)
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] pooldel: spawn guid {} (entry {}) is not part of any pool.",
                spawnId, entry));
            return true;
        }

        uint32 poolId = poolRes->Fetch()[0].GetUInt32();

        // Count how many members the pool has (for the message).
        uint32 memberCount = 0;
        if (QueryResult cnt = WorldDatabase.PQuery(
            "SELECT COUNT(*) FROM pool_members WHERE poolSpawnId = {}", poolId))
            memberCount = cnt->Fetch()[0].GetUInt32();

        // --- Runtime SQL: delete the concrete pool (poolId found above on THIS db). ---
        std::string rtMembers = Trinity::StringFormat(
            "DELETE FROM pool_members WHERE poolSpawnId = {};", poolId);
        std::string rtTpl = Trinity::StringFormat(
            "DELETE FROM pool_template WHERE entry = {};", poolId);
        WorldDatabase.DirectExecute(rtMembers.c_str());
        WorldDatabase.DirectExecute(rtTpl.c_str());

        // --- Log SQL: portable. The concrete poolId is NOT baked into the file.
        // The pool is looked up dynamically by the NPC's entry at import time:
        // any creature guid of the entry -> its poolSpawnId. Works on any server
        // whose pool ids differ from the one where this fix was recorded.
        std::string logSql;
        logSql += "-- delete pool of entry " + std::to_string(entry) + "\n";
        logSql += "DELETE FROM pool_members WHERE poolSpawnId IN (\n";
        logSql += "    SELECT pm.poolSpawnId FROM (\n";
        logSql += "        SELECT poolSpawnId FROM pool_members WHERE type = 0 AND spawnId IN\n";
        logSql += Trinity::StringFormat(
            "            (SELECT guid FROM creature WHERE id = {})\n", entry);
        logSql += "    ) AS pm);\n";
        logSql += "DELETE FROM pool_template WHERE entry IN (\n";
        logSql += "    SELECT pm.poolSpawnId FROM (\n";
        logSql += "        SELECT poolSpawnId FROM pool_members WHERE type = 0 AND spawnId IN\n";
        logSql += Trinity::StringFormat(
            "            (SELECT guid FROM creature WHERE id = {})\n", entry);
        logSql += "    ) AS pm);";

        std::string what = Trinity::StringFormat(
            "pooldel: pool {} deleted (entry {}, {} member(s))",
            poolId, entry, memberCount);
        AppendFixLog(h, logSql, "pooldel", what);

        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage("  Log SQL (portable, lookup by entry):");
        h->SendSysMessage(Trinity::StringFormat("  {}", logSql));
        h->SendSysMessage("  Apply in-game: .reload pool_template / .reload pool_members (or restart)");
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | Log SQL: {}", what, logSql);
        return true;
    }

    // ----- .fix follow <leaderEntry> <followerEntry> <range> -----
    // Makes the <followerEntry> NPC follow the <leaderEntry> NPC whenever the
    // leader is within <range> yards. Adds exactly one smart_scripts line on the
    // follower's template script:
    //
    //   SMART_EVENT_DISTANCE_CREATURE (75):
    //     event_param1 = 0            (guid; we track by entry instead)
    //     event_param2 = leaderEntry  (creature_template.entry to find nearby)
    //     event_param3 = range        (trigger distance, yards)
    //     event_param4 = 1000         (repeat interval, ms — re-check every 1s)
    //
    //   SMART_ACTION_FOLLOW (29):
    //     action_param1 = range       (follow distance, yards)
    //     action_param2 = 0           (angle, 0 = default)
    //     action_param3 = 0           (EndCreatureEntry)
    //     action_param4 = 0           (credit)
    //     action_param5 = 0           (creditType)
    //
    //   target_type = 1 (SELF). ProcessTimedAction for DISTANCE_CREATURE passes
    //   the found leader Creature as the target regardless of target_type, so the
    //   follower ends up following the leader. (target_type=1 matches the working
    //   reference template.)
    //
    // The follower must be a SmartAI creature. If its AIName is empty, it's set
    // to 'SmartAI' (same auto-enable as .fix gemote / .fix spell*).
    //
    // Both entries are validated against creature_template. The leader entry is
    // required by the SmartAIMgr validator (event.distance.entry != 0 must point
    // at an existing template, otherwise the line is skipped at load time).
    //
    // id = MAX(id)+1 for the follower (avoids PK conflict with existing scripts).
    //
    // This command does NOT need a selected creature — both entries are passed
    // explicitly, so it can be run from anywhere.
    static bool HandleFixFollow(ChatHandler* h, uint32 leaderEntry, uint32 followerEntry, uint32 range)
    {
        if (leaderEntry == 0 || followerEntry == 0 || range == 0)
        {
            h->SendSysMessage("Usage: .fix follow <leaderEntry> <followerEntry> <range>");
            return true;
        }
        if (leaderEntry == followerEntry)
        {
            h->SendSysMessage("[gm_fixer] leader and follower must be different entries.");
            return true;
        }

        // Both entries must exist as creature templates.
        if (!sObjectMgr->GetCreatureTemplate(leaderEntry))
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] leader entry {} not found in creature_template.", leaderEntry));
            return true;
        }
        if (!sObjectMgr->GetCreatureTemplate(followerEntry))
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] follower entry {} not found in creature_template.", followerEntry));
            return true;
        }

        // Follower must be SmartAI. Set it if empty (same pattern as .fix gemote).
        QueryResult aiRes = WorldDatabase.PQuery(
            "SELECT AIName FROM creature_template WHERE entry = {} LIMIT 1", followerEntry);
        bool needSmartAI = true;
        if (aiRes)
        {
            std::string aiName = aiRes->Fetch()[0].GetString();
            if (!aiName.empty())
                needSmartAI = false;
        }

        // Next free id for the follower (MAX+1, minimum 0).
        uint32 nextId = 0;
        QueryResult maxRes = WorldDatabase.PQuery(
            "SELECT IFNULL(MAX(id), -1) + 1 FROM smart_scripts "
            "WHERE entryorguid = {} AND source_type = 0", followerEntry);
        if (maxRes)
            nextId = maxRes->Fetch()[0].GetUInt32();

        // Build the SQL block (one or two statements).
        // The INSERT uses the SAME 29-column subset as .fix spellA / spellwow (and
        // NOT the full loader set) so it works on older smart_scripts schemas too —
        // deployed DBs may lack Difficulties / event_param5 / *_param_string /
        // action_param7 / target_string, and referencing a missing column yields MySQL
        // ER_BAD_FIELD_ERROR (1054), which the core treats as fatal and ABORTs on.
        std::string sqlLog;
        std::vector<std::string> stmts;

        if (needSmartAI)
        {
            std::string s = Trinity::StringFormat(
                "UPDATE creature_template SET AIName = 'SmartAI' WHERE entry = {};",
                followerEntry);
            stmts.push_back(s);
            sqlLog += s + "\n";
        }

        // DISTANCE_CREATURE(75) + FOLLOW(29). Column list matches .fix spellA
        // (29 columns) — see the note above on why the full loader set isn't used.
        std::string sScript = Trinity::StringFormat(
            "INSERT INTO smart_scripts "
            "(entryorguid, source_type, id, link, event_type, event_phase_mask, "
            " event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, "
            " action_type, action_param1, action_param2, action_param3, "
            " action_param4, action_param5, action_param6, "
            " target_type, target_param1, target_param2, target_param3, target_param4, "
            " target_x, target_y, target_z, target_o, comment) "
            "VALUES ({}, 0, {}, 0, 75, 0, 100, 0, 0, {}, {}, 1000, 29, {}, 0, 0, 0, 0, 0, "
            " 1, 0, 0, 0, 0, 0, 0, 0, 0, 'gm_fixer: {} follows {} (range {})');",
            followerEntry, nextId,
            leaderEntry, range,
            range,
            followerEntry, leaderEntry, range);
        stmts.push_back(sScript);
        sqlLog += sScript;

        // DirectExecute does NOT support multiple statements in one call — run each.
        for (auto const& s : stmts)
            WorldDatabase.DirectExecute(s.c_str());

        std::string what = Trinity::StringFormat(
            "follow: entry {} -> follows entry {} (range {}, script id={}){}",
            followerEntry, leaderEntry, range, nextId,
            needSmartAI ? " + SmartAI" : "");
        AppendFixLog(h, sqlLog, "follow", what);

        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL:\n{}", sqlLog));
        h->SendSysMessage("  Apply in-game: .reload smart_scripts (or .respawn the follower)");
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sqlLog);
        return true;
    }

    // =====================================================================
    //  Smart spell casting — smart_scripts CAST action
    // =====================================================================
    // Adds a periodic CAST spell line to smart_scripts for the selected NPC.
    // Like .fix gemote / .fix combat, requires AIName = 'SmartAI' (set it if empty).
    //
    // smart_scripts columns written:
    //   entryorguid  = creature_template.entry
    //   source_type  = 0   (SMART_SCRIPT_SOURCE_CREATURE)
    //   id           = MAX(id)+1 for this entry (avoids PK conflict)
    //   link         = 0
    //   event_type   = caller-supplied (IC / OOC)
    //   event_phase_mask = 0
    //   event_chance = 100
    //   event_flags  = 0
    //   event_param1 = 0     (min initial delay; first cast happens almost immediately)
    //   event_param2 = 0     (max initial delay)
    //   event_param3 = caller minRepeatMs — REPEAT interval min (default 6000)
    //   event_param4 = caller maxRepeatMs — REPEAT interval max
    //   action_type  = 11 (SMART_ACTION_CAST)
    //   action_param1= spellId
    //   action_param2= 0  (SMART_CAST_FLAGS_NONE — normal cast with cast time, costs mana)
    //   action_param3..6 = 0
    //   target_type  = caller-supplied (1=self, 2=victim)
    //   target_param1..4 = 0
    //   target_x..o  = 0 / NULL
    //   comment       = "gm_fixer: <tag> cast <spellId>"
    //
    // NOTE on param mapping: SMART_EVENT_UPDATE_IC / _OOC read the event through
    // the `minMaxRepeat` struct (SmartScriptMgr.h):
    //   param1/param2 = initial min/max delay, param3/param4 = repeat min/max.
    // ProcessEvent passes param3/param4 to RecalcTimer, so the repeat interval
    // MUST live in param3/param4 — otherwise the NPC casts once and never repeats.
    //
    // event_type values:
    //   0 = SMART_EVENT_UPDATE_IC     — periodic while in combat
    //   1 = SMART_EVENT_UPDATE        — periodic always (in & out of combat)
    //   2 = SMART_EVENT_UPDATE_OOC    — periodic while out of combat
    //
    // target_type values:
    //   1 = SMART_TARGET_SELF
    //   2 = SMART_TARGET_VICTIM

    // Shared implementation for .fix spellA / spellCB / spellB.
    //   spellId       — spell to cast (must exist in SpellMgr)
    //   eventType     — 0 (IC), 1 (UPDATE), 2 (OOC)
    //   targetType    — 1 (self) or 2 (victim)
    //   minRepeatMs   — min interval between casts (ms)
    //   maxRepeatMs   — max interval between casts (ms)
    //   tag           — short label for the comment / log ("spellA", "spellCB", "spellB")
    static bool ApplyFixSpellCast(ChatHandler* h, uint32 spellId, uint8 eventType,
                                  uint8 targetType, uint32 minRepeatMs, uint32 maxRepeatMs,
                                  char const* tag)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // Validate the spell exists.
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo)
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] Spell {} not found in SpellMgr.", spellId));
            return true;
        }

        // Check AIName — if empty, add an UPDATE to SmartAI (same as .fix gemote).
        QueryResult aiRes = WorldDatabase.PQuery(
            "SELECT AIName FROM creature_template WHERE entry = {} LIMIT 1", entry);
        bool needSmartAI = true;
        if (aiRes)
        {
            std::string aiName = aiRes->Fetch()[0].GetString();
            if (!aiName.empty())
                needSmartAI = false;
        }

        // Next free id for this entry (MAX+1, minimum 0) — avoids PK conflict.
        uint32 nextId = 0;
        QueryResult maxRes = WorldDatabase.PQuery(
            "SELECT IFNULL(MAX(id), -1) + 1 FROM smart_scripts "
            "WHERE entryorguid = {} AND source_type = 0", entry);
        if (maxRes)
            nextId = maxRes->Fetch()[0].GetUInt32();

        // Build the SQL block (one or two statements).
        std::string sqlLog;
        std::vector<std::string> stmts;
        if (needSmartAI)
        {
            std::string s = Trinity::StringFormat(
                "UPDATE creature_template SET AIName = 'SmartAI' WHERE entry = {};", entry);
            stmts.push_back(s);
            sqlLog += s + "\n";
        }

        // INSERT the CAST line.
        // For SMART_EVENT_UPDATE_IC / _OOC the SmartScript event reads its
        // parameters through the `minMaxRepeat` struct (SmartScriptMgr.h):
        //   event_param1 = min  — min INITIAL delay (ms), 0 = no delay
        //   event_param2 = max  — max INITIAL delay (ms)
        //   event_param3 = repeatMin — min REPEAT interval (ms)  <-- this is what
        //   event_param4 = repeatMax — max REPEAT interval (ms)     makes it recast
        // ProcessEvent(SMART_EVENT_UPDATE_IC) passes param3/param4 to RecalcTimer,
        // so the repeat interval MUST go into param3/param4 (NOT param1/param2).
        // param1/param2 only set the delay before the FIRST cast.
        // We use a 0/0 first-cast delay (no initial delay) and put the caller's
        // repeat interval into param3/param4.
        std::string sScript = Trinity::StringFormat(
            "INSERT INTO smart_scripts "
            "(entryorguid, source_type, id, link, event_type, event_phase_mask, "
            " event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, "
            " action_type, action_param1, action_param2, action_param3, "
            " action_param4, action_param5, action_param6, "
            " target_type, target_param1, target_param2, target_param3, target_param4, "
            " target_x, target_y, target_z, target_o, comment) "
            "VALUES ({}, 0, {}, 0, {}, 0, 100, 0, "
            " 0, 0, {}, {}, "
            " 11, {}, 0, 0, 0, 0, 0, "
            " {}, 0, 0, 0, 0, "
            " 0, 0, 0, 0, 'gm_fixer: {} cast {} (repeat {}-{}ms)');",
            entry, nextId, uint32(eventType),
            minRepeatMs, maxRepeatMs,
            spellId,
            uint32(targetType),
            tag, spellId, minRepeatMs, maxRepeatMs);
        stmts.push_back(sScript);
        sqlLog += sScript;

        // DirectExecute does NOT support multiple statements in one call — run each.
        for (auto const& s : stmts)
            WorldDatabase.DirectExecute(s.c_str());

        std::string what = Trinity::StringFormat(
            "CAST spell {} id={} (entry {}){}",
            spellId, nextId, entry,
            needSmartAI ? " + SmartAI" : "");
        AppendFixLog(h, sqlLog, tag, what);

        // smart_scripts is cached in memory at server startup, so the new line
        // is only picked up after a smart_scripts reload / respawn. Like .fix combat,
        // we don't force a runtime swap here.
        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL:\n{}", sqlLog));
        h->SendSysMessage("  Apply in-game: .reload smart_scripts (or .respawn the NPC)");
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sqlLog);
        return true;
    }

    // ----- .fix spellA <spellId> [sec] -----
    // Periodic CAST on the VICTIM (attack spell), in combat only.
    //   event_type = 0 (SMART_EVENT_UPDATE_IC)
    //   target_type= 2 (SMART_TARGET_VICTIM)
    // sec = repeat interval in seconds (fractional ok: "1.5", "1,5"); default 6.
    static bool HandleFixSpellAttack(ChatHandler* h, uint32 spellId, Optional<std::string_view> secOpt)
    {
        std::string secStr = secOpt ? std::string(*secOpt) : "";
        uint32 ms = 0;
        if (!ParseSecondsToMs(secStr.c_str(), 6000, ms) || ms == 0)
        {
            h->SendSysMessage("Usage: .fix spellA <spellId> [sec]");
            h->SendSysMessage("  sec is the repeat interval in seconds (e.g. 6, 1.5). Default 6.");
            return true;
        }
        return ApplyFixSpellCast(h, spellId,
            /*eventType*/ 0, /*targetType*/ 2, ms, ms, "spellA");
    }

    // ----- .fix spellCB <spellId> [sec] -----
    // Periodic SELF-cast buff, in combat only.
    //   event_type = 0 (SMART_EVENT_UPDATE_IC)
    //   target_type= 1 (SMART_TARGET_SELF)
    static bool HandleFixSpellCombatBuff(ChatHandler* h, uint32 spellId, Optional<std::string_view> secOpt)
    {
        std::string secStr = secOpt ? std::string(*secOpt) : "";
        uint32 ms = 0;
        if (!ParseSecondsToMs(secStr.c_str(), 6000, ms) || ms == 0)
        {
            h->SendSysMessage("Usage: .fix spellCB <spellId> [sec]");
            h->SendSysMessage("  sec is the repeat interval in seconds (e.g. 6, 1.5). Default 6.");
            return true;
        }
        return ApplyFixSpellCast(h, spellId,
            /*eventType*/ 0, /*targetType*/ 1, ms, ms, "spellCB");
    }

    // ----- .fix spellB <spellId> [sec] -----
    // Periodic SELF-cast buff, out of combat.
    //   event_type = 2 (SMART_EVENT_UPDATE_OOC)
    //   target_type= 1 (SMART_TARGET_SELF)
    static bool HandleFixSpellBuff(ChatHandler* h, uint32 spellId, Optional<std::string_view> secOpt)
    {
        std::string secStr = secOpt ? std::string(*secOpt) : "";
        uint32 ms = 0;
        if (!ParseSecondsToMs(secStr.c_str(), 6000, ms) || ms == 0)
        {
            h->SendSysMessage("Usage: .fix spellB <spellId> [sec]");
            h->SendSysMessage("  sec is the repeat interval in seconds (e.g. 6, 1.5). Default 6.");
            return true;
        }
        return ApplyFixSpellCast(h, spellId,
            /*eventType*/ 2, /*targetType*/ 1, ms, ms, "spellB");
    }

    // ----- .fix spellinfo -----
    // Lists all SmartAI CAST (action_type = 11) lines for the selected NPC and
    // shows their key parameters in human-readable form:
    //   - which spell is cast (id + name)
    //   - how often (event_param1..param4, min/max/delay/repeat, in seconds)
    //   - on whom (target_type: self / victim / etc.)
    //   - when (event_type: IC / OOC / UPDATE / etc.)
    //   - smart_scripts id and the row's comment
    //
    // Read-only — does not touch the DB or any log file.
    static bool HandleFixSpellInfo(ChatHandler* h)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // Show the template's AIName for context (a CAST line is only meaningful
        // when the creature actually runs SmartAI).
        QueryResult aiRes = WorldDatabase.PQuery(
            "SELECT AIName FROM creature_template WHERE entry = {} LIMIT 1", entry);
        std::string aiName = aiRes ? aiRes->Fetch()[0].GetString() : "";
        h->SendSysMessage(Trinity::StringFormat(
            "[gm_fixer] smart CASTs for entry {} ('{}') AIName='{}'",
            entry, c->GetName(), aiName.empty() ? "<empty>" : aiName));

        // Read every CAST line for this entry (source_type 0 = creature).
        // Order by id so the output is stable.
        QueryResult res = WorldDatabase.PQuery(
            "SELECT id, event_type, event_param1, event_param2, event_param3, "
            "       event_param4, action_param1, target_type, comment "
            "FROM smart_scripts "
            "WHERE entryorguid = {} AND source_type = 0 AND action_type = 11 "
            "ORDER BY id",
            entry);

        if (!res)
        {
            h->SendSysMessage("  no CAST (action_type=11) lines in smart_scripts.");
            h->SendSysMessage("  Use .fix spellA / spellCB / spellB to add one.");
            return true;
        }

        // Helper: human-readable event timing for the periodic UPDATE events.
        // For SMART_EVENT_UPDATE_IC / _OOC the event params map to the
        // `minMaxRepeat` struct (SmartScriptMgr.h):
        //   param1/param2 = initial delay min/max (ms)
        //   param3/param4 = REPEAT interval min/max (ms)  <- the actual recast rate
        auto fmtTiming = [](uint32 p1, uint32 p2, uint32 p3, uint32 p4) -> std::string
        {
            std::ostringstream os;
            // REPEAT interval (this is what makes the NPC recast).
            double rMin = p3 / 1000.0;
            double rMax = p4 / 1000.0;
            if (p3 == 0 && p4 == 0)
                os << "repeat NEVER (param3/4=0 — NPC will cast once)";
            else if (p3 == p4)
                os << Trinity::StringFormat("every {:.1f}s", rMin);
            else
                os << Trinity::StringFormat("every {:.1f}-{:.1f}s", rMin, rMax);
            // Initial delay before the first cast.
            if (p1 > 0 || p2 > 0)
            {
                if (p1 == p2)
                    os << Trinity::StringFormat(", first cast after {:.1f}s", p1 / 1000.0);
                else
                    os << Trinity::StringFormat(", first cast after {:.1f}-{:.1f}s", p1 / 1000.0, p2 / 1000.0);
            }
            return os.str();
        };

        // Helper: event_type -> when the CAST happens.
        auto fmtEvent = [](uint32 et) -> char const*
        {
            switch (et)
            {
                case 0:  return "in combat";
                case 1:  return "always (IC+OOC)";
                case 2:  return "out of combat";
                case 60: return "UPDATE (periodic)";
                default: return "other";
            }
        };

        // Helper: target_type -> who is cast on.
        auto fmtTarget = [](uint32 tt) -> char const*
        {
            switch (tt)
            {
                case 0:  return "none";
                case 1:  return "self";
                case 2:  return "victim";
                default: return "other";
            }
        };

        uint32 count = 0;
        do
        {
            Field* f = res->Fetch();
            uint32 id       = f[0].GetUInt32();
            uint32 et       = f[1].GetUInt32();
            uint32 p1       = f[2].GetUInt32();
            uint32 p2       = f[3].GetUInt32();
            uint32 p3       = f[4].GetUInt32();
            uint32 p4       = f[5].GetUInt32();
            uint32 spellId  = f[6].GetUInt32();
            uint32 tt       = f[7].GetUInt32();
            std::string comment = f[8].GetString();

            // Resolve the spell name for readability.
            std::string spellName = "<unknown>";
            if (SpellInfo const* si = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE))
                if (si->SpellName)
                    spellName = si->SpellName->Str[DEFAULT_LOCALE];

            h->SendSysMessage(Trinity::StringFormat(
                "  [id {}] {} ({}) -> {} | {} | {}",
                id, spellId, spellName,
                std::string(fmtTarget(tt)), std::string(fmtEvent(et)),
                fmtTiming(p1, p2, p3, p4)));
            if (!comment.empty())
                h->SendSysMessage(Trinity::StringFormat("        comment: '{}'", comment));
            ++count;
        } while (res->NextRow());

        h->SendSysMessage(Trinity::StringFormat("  Found {} CAST line(s).", count));
        return true;
    }

    // ----- .fix spellparam <spellId> <p1> <p2> <p3> <p4> -----
    // Sets the event_param1..4 of the selected NPC's CAST line(s) for the given
    // spell (matched by action_type=11 AND action_param1=spellId).
    //
    // This lets you fine-tune the SmartAI event timing of an existing CAST line
    // without re-creating it. Remember the param meaning depends on event_type:
    //
    //   For UPDATE_IC / UPDATE_OOC (the minMaxRepeat struct):
    //     event_param1 = min initial delay (ms)  -- 0 = no first-cast delay
    //     event_param2 = max initial delay (ms)
    //     event_param3 = min REPEAT interval (ms) -- 0 = cast ONCE and never repeat!
    //     event_param4 = max REPEAT interval (ms) -- 0 = cast ONCE and never repeat!
    //   (param3/param4 are what makes the NPC recast the spell periodically.)
    //
    // If multiple CAST lines exist for the same spell (e.g. IC + OOC variants),
    // ALL of them are updated.
    //
    // Example: .fix spellparam 371313 0 0 6000 6000
    //   -> spell 371313: no initial delay, repeat every 6s.
    static bool HandleFixSpellParam(ChatHandler* h, uint32 spellId, uint32 p1, uint32 p2, uint32 p3, uint32 p4)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // How many CAST lines match this spell for this entry?
        QueryResult countRes = WorldDatabase.PQuery(
            "SELECT COUNT(*) FROM smart_scripts "
            "WHERE entryorguid = {} AND source_type = 0 AND action_type = 11 AND action_param1 = {}",
            entry, spellId);
        uint32 matchCount = countRes ? countRes->Fetch()[0].GetUInt32() : 0;
        if (matchCount == 0)
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] spellparam: entry {} has no CAST line for spell {}.", entry, spellId));
            h->SendSysMessage("  Use .fix spellA / spellCB / spellB to add one.");
            return true;
        }

        // UPDATE all matching CAST lines.
        std::string sql = Trinity::StringFormat(
            "UPDATE smart_scripts SET event_param1 = {}, event_param2 = {}, "
            "event_param3 = {}, event_param4 = {} "
            "WHERE entryorguid = {} AND source_type = 0 AND action_type = 11 AND action_param1 = {};",
            p1, p2, p3, p4, entry, spellId);
        WorldDatabase.DirectExecute(sql.c_str());
        AppendFixLog(h, sql, "spellparam",
            Trinity::StringFormat("spell {} params p1={} p2={} p3={} p4={} (entry {}, {} line(s))",
                spellId, p1, p2, p3, p4, entry, matchCount));

        // Warn if repeat interval is zero — that is the classic "casts once" bug.
        if (p3 == 0 && p4 == 0)
        {
            h->SendSysMessage("[gm_fixer] WARNING: param3=0 and param4=0 — for IC/OOC events this means");
            h->SendSysMessage("  the NPC casts the spell ONCE and never repeats. Set param3/param4 to the");
            h->SendSysMessage("  repeat interval in ms (e.g. 6000 6000 for every 6s).");
        }

        std::string what = Trinity::StringFormat(
            "spellparam: spell {} -> p1={} p2={} p3={} p4={} (entry {}, {} line(s))",
            spellId, p1, p2, p3, p4, entry, matchCount);
        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL: {}", sql));
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sql);
        return true;
    }

    // ----- .fix spellwow [wowheadNpcId] [repeatSec] [tag] -----
    //
    // Parses the given NPC's abilities from Wowhead (over HTTPS via boost::beast)
    // and inserts a smart_scripts CAST row for each spell on the SELECTED
    // creature's entry. The SQL format is identical to .fix spellA / .fix spellCB:
    //
    //   INSERT INTO smart_scripts
    //     (entryorguid, source_type, id, link, event_type, event_phase_mask,
    //      event_chance, event_flags, event_param1, event_param2, event_param3, event_param4,
    //      action_type, action_param1, action_param2, action_param3,
    //      action_param4, action_param5, action_param6,
    //      target_type, target_param1, target_param2, target_param3, target_param4,
    //      target_x, target_y, target_z, target_o, comment)
    //   VALUES (<entry>, 0, <id>, 0, <eventType>, 0, 100, 0,
    //           0, 0, <repeatMs>, <repeatMs>,
    //           11, <spellId>, 0, 0, 0, 0, 0,
    //           <targetType>, 0, 0, 0, 0,
    //           0, 0, 0, 0, 'gm_fixer: <tag> cast <spellId> (<name>; repeat X-Yms)');
    //
    // Usage:
    //   .fix spellwow 19799              — explicit npcId, default 6s repeat, tag "spellwow"
    //   .fix spellwow 19799 8            — explicit npcId, 8s repeat
    //   .fix spellwow 19799 8 mytag      — explicit npcId, 8s repeat, custom tag
    //   .fix spellwow                    — npcId from the SELECTED creature's entry
    //   .fix spellwow 8                  — npcId from selection, 8s repeat
    //   .fix spellwow 8 mytag            — npcId from selection, 8s repeat, custom tag
    //
    // Requires a creature to be selected (like .fix spellA). The spells attach to
    // the selected creature's entry, NOT to the Wowhead NPC ID — so you can copy
    // abilities from any Wowhead NPC onto any creature in your DB. When no npcId
    // is given, the selected creature's entry is used as the Wowhead NPC id
    // (handy for copying the "canonical" abilities of that exact creature).
    static bool HandleFixSpellWow(ChatHandler* h, Optional<uint32> npcIdOpt,
                                  Optional<std::string_view> repeatOpt,
                                  Optional<std::string_view> tagOpt)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // Resolve the Wowhead NPC id. If not passed explicitly, fall back to the
        // SELECTED creature's entry — so ".fix spellwow" (no args) copies the
        // abilities of the very NPC you're looking at.
        uint32 wowheadNpcId = 0;
        if (npcIdOpt)
            wowheadNpcId = *npcIdOpt;
        else
            wowheadNpcId = entry;

        if (wowheadNpcId == 0)
        {
            h->SendSysMessage("Usage: .fix spellwow [wowheadNpcId] [repeatSec] [tag]");
            h->SendSysMessage("  No npcId given and no creature selected — nothing to fetch.");
            return true;
        }

        // Parse repeat seconds (default 6 -> 6000 ms, same as .fix spellA).
        uint32 repeatMs = 6000;
        if (repeatOpt)
        {
            uint32 ms = 0;
            if (!ParseSecondsToMs(std::string(*repeatOpt).c_str(), 6000, ms) || ms == 0)
            {
                h->SendSysMessage("Usage: .fix spellwow [wowheadNpcId] [repeatSec] [tag]");
                h->SendSysMessage("  repeatSec = repeat interval in seconds (default 6).");
                h->SendSysMessage("  tag       = comment tag (default 'spellwow').");
                h->SendSysMessage("  Example: .fix spellwow 19799 8   (or just .fix spellwow 8");
                h->SendSysMessage("           with a creature selected to use its entry)");
                return true;
            }
            repeatMs = ms;
        }
        std::string tag = tagOpt ? std::string(*tagOpt) : "spellwow";

        if (npcIdOpt)
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] spellwow: fetching Wowhead NPC {} ...", wowheadNpcId));
        else
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] spellwow: no npcId given, using selected creature entry {} ...", wowheadNpcId));

        // ----- 1. Fetch the Wowhead NPC page -----
        std::string html, err;
        if (!FetchWowheadNpcHtml(wowheadNpcId, html, err))
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] Wowhead fetch failed: {}", err));
            h->SendSysMessage(Trinity::StringFormat(
                "  Open https://www.wowhead.com/npc={} in a browser to check manually.", wowheadNpcId));
            TC_LOG_INFO(LOG_CHAN, "[gm_fixer] spellwow fetch failed for NPC {}: {}",
                wowheadNpcId, err);
            return true;
        }
        h->SendSysMessage(Trinity::StringFormat(
            "[gm_fixer] fetched {} bytes from Wowhead.", html.size()));

        // ----- 2. Parse the WH.Gatherer.addData(6, ...) spell dictionary -----
        std::vector<GmFixer_WowheadAbility> abilities;
        GmFixer_ParseGathererSpells(html, abilities);

        if (abilities.empty())
        {
            h->SendSysMessage("[gm_fixer] no abilities found on this NPC's Wowhead page.");
            h->SendSysMessage("  (some NPCs have no listed abilities, or Wowhead changed its markup)");
            return true;
        }

        h->SendSysMessage(Trinity::StringFormat(
            "[gm_fixer] parsed {} ability(ies) from Wowhead.", abilities.size()));

        // ----- 3. Ensure AIName = 'SmartAI' (same as .fix spellA) -----
        QueryResult aiRes = WorldDatabase.PQuery(
            "SELECT AIName FROM creature_template WHERE entry = {} LIMIT 1", entry);
        bool needSmartAI = true;
        if (aiRes)
        {
            std::string aiName = aiRes->Fetch()[0].GetString();
            if (!aiName.empty())
                needSmartAI = false;
        }

        std::vector<std::string> stmts;
        std::string sqlLog;
        if (needSmartAI)
        {
            std::string s = Trinity::StringFormat(
                "UPDATE creature_template SET AIName = 'SmartAI' WHERE entry = {};", entry);
            stmts.push_back(s);
            sqlLog += s + "\n";
        }

        // ----- 4. For each spell: validate + insert a CAST row -----
        uint32 inserted = 0;
        uint32 skipped  = 0;

        for (auto const& a : abilities)
        {
            // Validate the spell exists in SpellMgr (same check as .fix spellA).
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(a.spellId, DIFFICULTY_NONE);
            if (!spellInfo)
            {
                h->SendSysMessage(Trinity::StringFormat(
                    "  [skip] spell {} ({}) not found in SpellMgr", a.spellId, a.name));
                ++skipped;
                continue;
            }

            // Next free smart_scripts.id for this entry (MAX+1, min 0).
            uint32 nextId = 0;
            QueryResult maxRes = WorldDatabase.PQuery(
                "SELECT IFNULL(MAX(id), -1) + 1 FROM smart_scripts "
                "WHERE entryorguid = {} AND source_type = 0", entry);
            if (maxRes)
                nextId = maxRes->Fetch()[0].GetUInt32();

            uint8 eventType  = GmFixer_GuessEventType(a.name, a.description);
            uint8 targetType = GmFixer_GuessTargetType(a.name, a.description);

            // Build the INSERT. Format mirrors ApplyFixSpellCast() exactly:
            //   event_param1/2 = 0 (no initial delay)
            //   event_param3/4 = repeat min/max (ms)   <-- makes it recast
            //   action_type    = 11 (SMART_ACTION_CAST)
            //   action_param1  = spellId
            //   target_type    = 1 (SELF) or 2 (VICTIM)
            std::string sScript = Trinity::StringFormat(
                "INSERT INTO smart_scripts "
                "(entryorguid, source_type, id, link, event_type, event_phase_mask, "
                " event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, "
                " action_type, action_param1, action_param2, action_param3, "
                " action_param4, action_param5, action_param6, "
                " target_type, target_param1, target_param2, target_param3, target_param4, "
                " target_x, target_y, target_z, target_o, comment) "
                "VALUES ({}, 0, {}, 0, {}, 0, 100, 0, "
                " 0, 0, {}, {}, "
                " 11, {}, 0, 0, 0, 0, 0, "
                " {}, 0, 0, 0, 0, "
                " 0, 0, 0, 0, 'gm_fixer: {} cast {} ({}; repeat {}-{}ms)');",
                entry, nextId, uint32(eventType),
                repeatMs, repeatMs,
                a.spellId,
                uint32(targetType),
                tag, a.spellId, a.name, repeatMs, repeatMs);
            stmts.push_back(sScript);
            sqlLog += sScript + "\n";
            ++inserted;

            // Escape the name for display (replace ' for the in-game chat).
            std::string dispName = a.name;
            for (char& ch : dispName)
                if (ch == '|') ch = '/';

            char const* tgtStr = (targetType == 1) ? "SELF" :
                                 (targetType == 2) ? "VICTIM" : "OTHER";
            h->SendSysMessage(Trinity::StringFormat(
                "  [+#{} {}] id={} evt={} tgt={} ({}ms)",
                a.spellId, dispName, nextId, uint32(eventType), tgtStr, repeatMs));
        }

        if (inserted == 0)
        {
            h->SendSysMessage("[gm_fixer] nothing inserted (all spells skipped — not in SpellMgr).");
            return true;
        }

        // ----- 5. Execute all statements -----
        for (auto const& s : stmts)
            WorldDatabase.DirectExecute(s.c_str());

        std::string what = Trinity::StringFormat(
            "spellwow NPC {} -> {} CAST row(s) on entry {} ({} skipped){}",
            wowheadNpcId, inserted, entry, skipped,
            needSmartAI ? " + SmartAI" : "");
        AppendFixLog(h, sqlLog, "spellwow", what);

        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage("  Apply in-game: .reload smart_scripts (or .respawn the NPC)");
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sqlLog);
        return true;
    }

    // =====================================================================
    //  Smart spell CAST line management — delete / copy
    // =====================================================================
    // The spell family above (spellA / spellCB / spellB / spellwow / spellinfo /
    // spellparam) all operate on the SmartAI CAST rows of the selected NPC — i.e.
    // smart_scripts rows with source_type = 0 and action_type = 11
    // (SMART_ACTION_CAST, action_param1 = spellId). The three commands below add
    // the matching bulk delete / copy operations on that same row set.

    // ----- .fix spellclear -----
    // Deletes EVERY SmartAI CAST (action_type = 11) line from smart_scripts for the
    // SELECTED NPC's entry. This wipes every spell the NPC casts via SmartAI (the
    // same lines that .fix spellA / spellCB / spellB / spellwow add and that
    // .fix spellinfo lists). Other smart_scripts rows (non-CAST actions like
    // FOLLOW, ATTACK, EMOTE ...) are left untouched.
    //
    // Does NOT clear AIName — use .fix unsmart for that.
    static bool HandleFixSpellClear(ChatHandler* h)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // How many CAST lines will be removed?
        QueryResult countRes = WorldDatabase.PQuery(
            "SELECT COUNT(*) FROM smart_scripts "
            "WHERE entryorguid = {} AND source_type = 0 AND action_type = 11",
            entry);
        uint32 matchCount = countRes ? countRes->Fetch()[0].GetUInt32() : 0;
        if (matchCount == 0)
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] spellclear: entry {} has no CAST (action_type=11) lines.", entry));
            h->SendSysMessage("  Nothing to delete. (use .fix spellinfo to list them)");
            return true;
        }

        std::string sql = Trinity::StringFormat(
            "DELETE FROM smart_scripts "
            "WHERE entryorguid = {} AND source_type = 0 AND action_type = 11;",
            entry);
        WorldDatabase.DirectExecute(sql.c_str());
        AppendFixLog(h, sql, "spellclear",
            Trinity::StringFormat("clear {} CAST line(s) (entry {})", matchCount, entry));

        std::string what = Trinity::StringFormat(
            "spellclear: deleted {} CAST line(s) (entry {})", matchCount, entry);
        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL: {}", sql));
        h->SendSysMessage("  Apply in-game: .reload smart_scripts (or .respawn the NPC)");
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sql);
        return true;
    }

    // ----- .fix spelldel <N> -----
    // Deletes a single SmartAI CAST (action_type = 11) line from the SELECTED NPC.
    // <N> is the 1-based ordinal position of the CAST line in the listing produced
    // by .fix spellinfo (lines are ordered by smart_scripts.id). So if .fix
    // spellinfo shows three CAST lines, ".fix spelldel 2" removes the second one.
    //
    // The ordinal is resolved to the row's real smart_scripts.id before the DELETE,
    // so it works correctly even when the ids are not contiguous (e.g. after earlier
    // deletions). Non-CAST smart_scripts rows are never counted. On a bad N the
    // current CAST listing is printed with its ordinals so the GM can pick.
    static bool HandleFixSpellDel(ChatHandler* h, uint32 ordinal)
    {
        Creature* c = nullptr; uint32 entry = 0;
        if (!GetSelectedCreature(h, c, entry)) return true;

        // Pull every CAST line ordered by id (same order as .fix spellinfo).
        QueryResult res = WorldDatabase.PQuery(
            "SELECT id, action_param1 FROM smart_scripts "
            "WHERE entryorguid = {} AND source_type = 0 AND action_type = 11 "
            "ORDER BY id",
            entry);
        if (!res)
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] spelldel: entry {} has no CAST (action_type=11) lines.", entry));
            h->SendSysMessage("  Nothing to delete. (use .fix spellinfo to list them)");
            return true;
        }

        // Walk the rows; collect (id, spellId) pairs.
        std::vector<std::pair<uint32, uint32>> rows;
        do
        {
            Field* f = res->Fetch();
            rows.emplace_back(f[0].GetUInt32(), f[1].GetUInt32());
        } while (res->NextRow());

        // Bad ordinal -> print usage + the current listing with ordinals.
        if (ordinal == 0 || ordinal > rows.size())
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] spelldel: N={} is out of range (entry {} has {} CAST line(s)).",
                ordinal, entry, rows.size()));
            h->SendSysMessage("  Usage: .fix spelldel <N>   (N = 1-based position in .fix spellinfo)");
            h->SendSysMessage("  Current CAST lines:");
            for (size_t i = 0; i < rows.size(); ++i)
            {
                uint32 sid = rows[i].second;
                std::string name = "<unknown>";
                if (SpellInfo const* si = sSpellMgr->GetSpellInfo(sid, DIFFICULTY_NONE))
                    if (si->SpellName)
                        name = si->SpellName->Str[DEFAULT_LOCALE];
                h->SendSysMessage(Trinity::StringFormat(
                    "    #{}) id={} {} ({})", i + 1, rows[i].first, sid, name));
            }
            return true;
        }

        uint32 targetId    = rows[ordinal - 1].first;
        uint32 targetSpell = rows[ordinal - 1].second;

        std::string spellName = "<unknown>";
        if (SpellInfo const* si = sSpellMgr->GetSpellInfo(targetSpell, DIFFICULTY_NONE))
            if (si->SpellName)
                spellName = si->SpellName->Str[DEFAULT_LOCALE];

        std::string sql = Trinity::StringFormat(
            "DELETE FROM smart_scripts "
            "WHERE entryorguid = {} AND source_type = 0 AND id = {};",
            entry, targetId);
        WorldDatabase.DirectExecute(sql.c_str());
        AppendFixLog(h, sql, "spelldel",
            Trinity::StringFormat("del CAST #{} id={} spell {} ({}) (entry {})",
                ordinal, targetId, targetSpell, spellName, entry));

        std::string what = Trinity::StringFormat(
            "spelldel: removed CAST #{} (id={}, spell {} '{}') from entry {}",
            ordinal, targetId, targetSpell, spellName, entry);
        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage(Trinity::StringFormat("  SQL: {}", sql));
        h->SendSysMessage("  Apply in-game: .reload smart_scripts (or .respawn the NPC)");
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sql);
        return true;
    }

    // ----- .fix spellcopy <donorEntry> <recipientEntry> -----
    // Copies every SmartAI CAST (action_type = 11) line from the DONOR creature's
    // entry to the RECIPIENT creature's entry. The recipient gets new smart_scripts
    // rows that mirror the donor's CAST lines exactly — same spell, same event
    // timing, same target — only entryorguid (-> recipient) and id (-> next free id
    // on the recipient) change. The donor's rows are not modified.
    //
    // This is the batch form of .fix spellA / spellCB: instead of re-adding each
    // spell by hand, you point it at a donor (e.g. a blizzlike NPC) and every spell
    // CAST is cloned onto the recipient.
    //
    // Behaviour:
    //   * Only CAST (action_type=11) lines are copied — other smart actions
    //     (FOLLOW, EMOTE, ATTACK ...) stay on the donor.
    //   * If the recipient already has a CAST line for the same spell (matched by
    //     action_param1), that donor line is SKIPPED so the spell isn't cast twice.
    //   * The recipient's AIName is auto-set to 'SmartAI' if empty (same as the
    //     other spell commands).
    //   * Existing CAST lines on the recipient are kept (additive copy). Run
    //     ".fix spellclear" on the recipient first for a clean overwrite.
    //
    // No creature needs to be selected — both entries are passed explicitly, so
    // this can be run from anywhere (like .fix follow).
    static bool HandleFixSpellCopy(ChatHandler* h, uint32 donorEntry, uint32 recipientEntry)
    {
        if (donorEntry == 0 || recipientEntry == 0)
        {
            h->SendSysMessage("Usage: .fix spellcopy <donorEntry> <recipientEntry>");
            h->SendSysMessage("  Copies every SmartAI CAST spell line from donor to recipient.");
            return true;
        }
        if (donorEntry == recipientEntry)
        {
            h->SendSysMessage("[gm_fixer] spellcopy: donor and recipient must be different entries.");
            return true;
        }

        // Both entries must exist as creature templates.
        if (!sObjectMgr->GetCreatureTemplate(donorEntry))
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] spellcopy: donor entry {} not found in creature_template.", donorEntry));
            return true;
        }
        if (!sObjectMgr->GetCreatureTemplate(recipientEntry))
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] spellcopy: recipient entry {} not found in creature_template.", recipientEntry));
            return true;
        }

        // Pull every CAST line from the donor (id + spellId), ordered by id so the
        // copy order is stable and matches what .fix spellinfo shows for the donor.
        QueryResult donorRes = WorldDatabase.PQuery(
            "SELECT id, action_param1 FROM smart_scripts "
            "WHERE entryorguid = {} AND source_type = 0 AND action_type = 11 "
            "ORDER BY id",
            donorEntry);
        if (!donorRes)
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] spellcopy: donor entry {} has no CAST (action_type=11) lines.", donorEntry));
            h->SendSysMessage("  Nothing to copy.");
            return true;
        }

        std::vector<std::pair<uint32, uint32>> donorRows; // (id, spellId)
        do
        {
            Field* f = donorRes->Fetch();
            donorRows.emplace_back(f[0].GetUInt32(), f[1].GetUInt32());
        } while (donorRes->NextRow());

        // Spells the recipient already casts (to skip duplicates).
        std::set<uint32> recipientSpells;
        if (QueryResult rRes = WorldDatabase.PQuery(
                "SELECT action_param1 FROM smart_scripts "
                "WHERE entryorguid = {} AND source_type = 0 AND action_type = 11",
                recipientEntry))
        {
            do
                recipientSpells.insert(rRes->Fetch()[0].GetUInt32());
            while (rRes->NextRow());
        }

        // Ensure the recipient runs SmartAI (auto-enable, same as .fix spellA).
        QueryResult aiRes = WorldDatabase.PQuery(
            "SELECT AIName FROM creature_template WHERE entry = {} LIMIT 1", recipientEntry);
        bool needSmartAI = true;
        if (aiRes)
        {
            std::string aiName = aiRes->Fetch()[0].GetString();
            if (!aiName.empty())
                needSmartAI = false;
        }

        // Next free id on the recipient. Tracked LOCALLY and incremented per copied
        // row — we do NOT re-query MAX(id) inside the loop, because the INSERTs are
        // deferred until after the loop, so a per-row MAX() would return the same
        // value every time and collide on the PK (entryorguid, source_type, id).
        uint32 nextId = 0;
        if (QueryResult maxRes = WorldDatabase.PQuery(
                "SELECT IFNULL(MAX(id), -1) + 1 FROM smart_scripts "
                "WHERE entryorguid = {} AND source_type = 0", recipientEntry))
            nextId = maxRes->Fetch()[0].GetUInt32();

        std::vector<std::string> stmts;
        std::string sqlLog;
        if (needSmartAI)
        {
            std::string s = Trinity::StringFormat(
                "UPDATE creature_template SET AIName = 'SmartAI' WHERE entry = {};", recipientEntry);
            stmts.push_back(s);
            sqlLog += s + "\n";
        }

        // Clone each donor CAST line via INSERT...SELECT, overriding entryorguid
        // (-> recipient) and id (-> nextId). The full column set matches the
        // smart_scripts loader (see .fix follow), so every column of the donor row
        // (Difficulties, event_param5, *_param_string, comment ...) is preserved.
        uint32 inserted = 0;
        uint32 skipped  = 0;
        for (auto const& dr : donorRows)
        {
            uint32 donorId = dr.first;
            uint32 spellId = dr.second;

            std::string name = "<unknown>";
            if (SpellInfo const* si = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE))
                if (si->SpellName)
                    name = si->SpellName->Str[DEFAULT_LOCALE];

            if (recipientSpells.count(spellId))
            {
                h->SendSysMessage(Trinity::StringFormat(
                    "  [skip] spell {} ({}) — recipient {} already casts it",
                    spellId, name, recipientEntry));
                ++skipped;
                continue;
            }

            // Clone each donor CAST line via INSERT...SELECT, overriding entryorguid
            // (-> recipient) and id (-> nextId). The column list is intentionally the
            // SAME 29-column subset that .fix spellA / spellwow write (event/timing/
            // action/target + comment) and NOT the full loader set — deployed DBs may
            // be on an older smart_scripts schema that lacks Difficulties / event_param5
            // / event_param_string / action_param7 / action_param_string / target_string.
            // Referencing a non-existent column yields MySQL ER_BAD_FIELD_ERROR (1054),
            // which TrinityCore's _HandleMySQLErrno treats as fatal ("outdated DB") and
            // ABORTs the worldserver on. Sticking to the spellA set works on BOTH the
            // old and the new schema (the extra columns just take their defaults on the
            // new one, which is exactly what spellA would have produced anyway).
            std::string sScript = Trinity::StringFormat(
                "INSERT INTO smart_scripts "
                "(entryorguid, source_type, id, link, event_type, event_phase_mask, "
                " event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, "
                " action_type, action_param1, action_param2, action_param3, "
                " action_param4, action_param5, action_param6, "
                " target_type, target_param1, target_param2, target_param3, target_param4, "
                " target_x, target_y, target_z, target_o, comment) "
                "SELECT {}, source_type, {}, link, event_type, event_phase_mask, "
                " event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, "
                " action_type, action_param1, action_param2, action_param3, "
                " action_param4, action_param5, action_param6, "
                " target_type, target_param1, target_param2, target_param3, target_param4, "
                " target_x, target_y, target_z, target_o, comment "
                "FROM smart_scripts "
                "WHERE entryorguid = {} AND source_type = 0 AND id = {};",
                recipientEntry, nextId, donorEntry, donorId);
            stmts.push_back(sScript);
            sqlLog += sScript + "\n";

            // Guard against the donor itself having two CAST lines for the same spell.
            recipientSpells.insert(spellId);
            ++inserted;

            h->SendSysMessage(Trinity::StringFormat(
                "  [copy] spell {} ({}) -> recipient id={}", spellId, name, nextId));
            ++nextId;
        }

        if (inserted == 0)
        {
            h->SendSysMessage(Trinity::StringFormat(
                "[gm_fixer] spellcopy: nothing copied (recipient {} already has all {} donor spell(s)).",
                recipientEntry, donorRows.size()));
            return true;
        }

        // DirectExecute does NOT support multiple statements in one call — run each.
        for (auto const& s : stmts)
            WorldDatabase.DirectExecute(s.c_str());

        std::string what = Trinity::StringFormat(
            "spellcopy: entry {} -> entry {} ({} CAST line(s) copied, {} skipped){}",
            donorEntry, recipientEntry, inserted, skipped,
            needSmartAI ? " + SmartAI" : "");
        AppendFixLog(h, sqlLog, "spellcopy", what);

        h->SendSysMessage(Trinity::StringFormat("[gm_fixer] {} -> OK", what));
        h->SendSysMessage("  Apply in-game: .reload smart_scripts (or .respawn the recipient)");
        TC_LOG_INFO(LOG_CHAN, "[gm_fixer] {} | SQL: {}", what, sqlLog);
        return true;
    }

    // ===== Wowhead loot / pickpocket / compare / all-in-one =====

    // ----- .fix wowinfo [id] -----
    // Compares the Wowhead page (spells, drops, pickpocketing) of the given NPC
    // (or the selected creature) against THIS server's database and prints a
    // diff. No DB changes. Diff legend:
    //   [+] in Wowhead AND in DB
    //   [-] in Wowhead only (missing from DB)
    //   [?] in DB only (custom / not on Wowhead)
    static bool HandleFixWowInfo(ChatHandler* h, Optional<uint32> npcIdOpt)
    {
        uint32 entry = 0;
        std::string html;
        if (!GmFixer_FetchWowheadAll(h, npcIdOpt, entry, html))
            return true;

        // Parse all three categories from the fetched HTML.
        std::vector<GmFixer_WowheadAbility> abilities;
        GmFixer_ParseGathererSpells(html, abilities);
        std::vector<GmFixer_WowheadItem> drops, pickpocket;
        GmFixer_ParseListviewItems(html, "drops", drops);
        GmFixer_ParseListviewItems(html, "pickpocketing", pickpocket);

        // --- SPELLS: Wowhead spell IDs vs smart_scripts CAST lines ---
        std::set<uint32> whSpells;
        for (auto const& a : abilities) whSpells.insert(a.spellId);
        std::set<uint32> dbSpells;
        if (QueryResult r = WorldDatabase.PQuery(
            "SELECT action_param1 FROM smart_scripts "
            "WHERE entryorguid = {} AND source_type = 0 AND action_type = 11", entry))
        {
            do dbSpells.insert(r->Fetch()[0].GetUInt32()); while (r->NextRow());
        }

        h->SendSysMessage(Trinity::StringFormat(
            "[gm_fixer] wowinfo: NPC {} | SPELLS (Wowhead {}, DB {})",
            entry, uint32(whSpells.size()), uint32(dbSpells.size())));
        for (auto const& a : abilities)
        {
            char const* mark = dbSpells.count(a.spellId) ? "[+]" : "[-]";
            h->SendSysMessage(Trinity::StringFormat(
                "  {} {} {}", mark, a.spellId, a.name.empty() ? std::string("(no name)") : a.name));
        }
        for (uint32 s : dbSpells)
            if (!whSpells.count(s))
            {
                std::string nm = "<unknown>";
                if (SpellInfo const* si = sSpellMgr->GetSpellInfo(s, DIFFICULTY_NONE))
                    if (si->SpellName) nm = si->SpellName->Str[DEFAULT_LOCALE];
                h->SendSysMessage(Trinity::StringFormat("  [?] {} {} (in DB only)", s, nm));
            }

        // --- DROPS: Wowhead drops vs creature_loot_template ---
        GmFixer_PrintItemDiff(h, "DROPS", drops, "creature_loot_template", entry);

        // --- PICKPOCKETING: vs pickpocketing_loot_template ---
        GmFixer_PrintItemDiff(h, "PICKPOCKETING", pickpocket, "pickpocketing_loot_template", entry);

        return true;
    }

    // Helper used by wowinfo: print a diff of Wowhead items vs a loot table.
    static void GmFixer_PrintItemDiff(ChatHandler* h, char const* title,
                                      std::vector<GmFixer_WowheadItem> const& whItems,
                                      char const* tableName, uint32 entry)
    {
        std::set<uint32> whIds;
        for (auto const& it : whItems) whIds.insert(it.itemId);
        std::set<uint32> dbIds;
        if (QueryResult r = WorldDatabase.PQuery(
            "SELECT `Item` FROM `{}` WHERE `Entry` = {} AND `ItemType` = 0", tableName, entry))
        {
            do dbIds.insert(r->Fetch()[0].GetUInt32()); while (r->NextRow());
        }

        h->SendSysMessage(Trinity::StringFormat(
            "[gm_fixer] wowinfo: NPC {} | {} (Wowhead {}, DB {})",
            entry, title, uint32(whIds.size()), uint32(dbIds.size())));
        for (auto const& it : whItems)
        {
            char const* mark = dbIds.count(it.itemId) ? "[+]" : "[-]";
            double chance = (it.classs == 12 || it.outof == 0) ? 100.0
                : (double)it.count / (double)it.outof * 100.0;
            h->SendSysMessage(Trinity::StringFormat(
                "  {} item {} {} {:.2f}%{}",
                mark, it.itemId, it.name.empty() ? std::string("(no name)") : it.name,
                chance, it.classs == 12 ? " (quest)" : ""));
        }
        for (uint32 id : dbIds)
            if (!whIds.count(id))
                h->SendSysMessage(Trinity::StringFormat(
                    "  [?] item {} (in DB only)", id));
    }

    // ----- .fix lootwow [id] -----
    // Replaces creature_loot_template for the given NPC (or the selected
    // creature) with the Wowhead drops list (DELETE + INSERT).
    static bool HandleFixLootWow(ChatHandler* h, Optional<uint32> npcIdOpt)
    {
        uint32 entry = 0;
        std::string html;
        if (!GmFixer_FetchWowheadAll(h, npcIdOpt, entry, html))
            return true;

        std::vector<GmFixer_WowheadItem> drops;
        GmFixer_ParseListviewItems(html, "drops", drops);
        GmFixer_ApplyLoot(h, entry, "creature_loot_template", drops, "lootwow");
        return true;
    }

    // ----- .fix pickpocketwow [id] -----
    // Replaces pickpocketing_loot_template for the given NPC (or the selected
    // creature) with the Wowhead pickpocketing list (DELETE + INSERT).
    static bool HandleFixPickpocketWow(ChatHandler* h, Optional<uint32> npcIdOpt)
    {
        uint32 entry = 0;
        std::string html;
        if (!GmFixer_FetchWowheadAll(h, npcIdOpt, entry, html))
            return true;

        std::vector<GmFixer_WowheadItem> pickpocket;
        GmFixer_ParseListviewItems(html, "pickpocketing", pickpocket);
        GmFixer_ApplyLoot(h, entry, "pickpocketing_loot_template", pickpocket, "pickpocketwow");
        return true;
    }

    // ----- .fix wowall [id] -----
    // Applies all three sections from Wowhead to the given NPC (or the selected
    // creature): smart_scripts CAST rows (via .fix spellwow logic), creature
    // drops, and pickpocketing loot. One fetch, three applies.
    static bool HandleFixWowAll(ChatHandler* h, Optional<uint32> npcIdOpt)
    {
        uint32 entry = 0;
        std::string html;
        if (!GmFixer_FetchWowheadAll(h, npcIdOpt, entry, html))
            return true;

        // 1) Spells — delegate to the spellwow handler so the SmartAI enable +
        //    CAST-row INSERT + logging stay identical to .fix spellwow.
        //    spellwow fetches again internally; that's one extra page load but
        //    keeps the code paths unified and tested.
        h->SendSysMessage("[gm_fixer] wowall: === SPELLS ===");
        HandleFixSpellWow(h, npcIdOpt, /*repeat*/ std::nullopt, /*tag*/ std::nullopt);

        // 2) Drops.
        h->SendSysMessage("[gm_fixer] wowall: === DROPS ===");
        std::vector<GmFixer_WowheadItem> drops;
        GmFixer_ParseListviewItems(html, "drops", drops);
        GmFixer_ApplyLoot(h, entry, "creature_loot_template", drops, "wowall/loot");

        // 3) Pickpocketing.
        h->SendSysMessage("[gm_fixer] wowall: === PICKPOCKETING ===");
        std::vector<GmFixer_WowheadItem> pickpocket;
        GmFixer_ParseListviewItems(html, "pickpocketing", pickpocket);
        GmFixer_ApplyLoot(h, entry, "pickpocketing_loot_template", pickpocket, "wowall/pickpocket");

        h->SendSysMessage(Trinity::StringFormat(
            "[gm_fixer] wowall: NPC {} done (spells+loot+pickpocket). Reload smart_scripts + loot tables.", entry));
        return true;
    }
};

void AddSC_gm_fixer()
{
    new gm_fixer_cmd();
}
