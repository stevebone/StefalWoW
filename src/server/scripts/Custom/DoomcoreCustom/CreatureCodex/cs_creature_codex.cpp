/*
#include "Chat.h"
#include "ChatCommand.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "SpellMgr.h"
#include "World.h"
#include "SpellInfo.h"
#include "WorldSession.h"
#include "SpellInfo.h"

#include <unordered_set>

using namespace Trinity::ChatCommands;

// Forward declarations from creature_codex_sniffer.cpp
namespace CreatureCodex
{
    bool IsRuntimeBlacklisted(uint32 spellId);
    void AddToBlacklist(uint32 spellId);
    bool RemoveFromBlacklist(uint32 spellId);
    std::unordered_set<uint32> GetBlacklistCopy();
}

class codex_commandscript : public CommandScript
{
public:
    codex_commandscript() : CommandScript("codex_commandscript") {}

    std::span<ChatCommandBuilder const> GetCommands() const override
    {
        static ChatCommandTable blacklistTable =
        {
            { "add",    HandleBlacklistAdd,    rbac::RBAC_PERM_COMMAND_CREATURE_CODEX, Console::No },
            { "remove", HandleBlacklistRemove, rbac::RBAC_PERM_COMMAND_CREATURE_CODEX, Console::No },
            { "list",   HandleBlacklistList,   rbac::RBAC_PERM_COMMAND_CREATURE_CODEX, Console::No },
        };

        static ChatCommandTable codexTable =
        {
            { "query",     HandleCodexQuery, rbac::RBAC_PERM_COMMAND_CREATURE_CODEX, Console::No },
            { "stats",     HandleCodexStats, rbac::RBAC_PERM_COMMAND_CREATURE_CODEX, Console::No },
            { "blacklist", blacklistTable },
        };

        static ChatCommandTable commandTable =
        {
            { "codex", codexTable },
        };
        return commandTable;
    }

    static bool HandleCodexQuery(ChatHandler* handler, uint32 entry)
    {
        CreatureTemplate const* cInfo = sObjectMgr->GetCreatureTemplate(entry);
        if (!cInfo)
        {
            handler->PSendSysMessage("[CreatureCodex] Creature entry %u not found.", entry);
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("[CreatureCodex] Spells for |cFF00FF00%s|r (entry %u):",
            cInfo->Name.c_str(), entry);

        bool found = false;
        for (uint8 i = 0; i < MAX_CREATURE_SPELLS; ++i)
        {
            uint32 spellId = cInfo->spells[i];
            if (!spellId)
                continue;

            found = true;
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
            if (spellInfo && spellInfo->SpellName)
            {
                handler->PSendSysMessage("  [%u] Spell %u - %s (school 0x%02X)",
                    i, spellId, (*spellInfo->SpellName)[LOCALE_enUS],
                    uint32(spellInfo->SchoolMask));
            }
            else
            {
                handler->PSendSysMessage("  [%u] Spell %u - (unknown)", i, spellId);
            }
        }

        if (!found)
            handler->SendSysMessage("  (no spells assigned)");

        return true;
    }

    static bool HandleCodexStats(ChatHandler* handler)
    {
        uint32 totalSessions = 0;
        uint32 listeningPlayers = 0;

        SessionMap const& sessions = sWorld->GetAllSessions();
        for (auto const& [accountId, session] : sessions)
        {
            if (!session || !session->GetPlayer() || !session->GetPlayer()->IsInWorld())
                continue;

            ++totalSessions;
            if (session->IsAddonRegistered("CCDX"))
                ++listeningPlayers;
        }

        auto blacklist = CreatureCodex::GetBlacklistCopy();

        handler->SendSysMessage("[CreatureCodex] Sniffer Statistics:");
        handler->PSendSysMessage("  Players online: %u", totalSessions);
        handler->PSendSysMessage("  Players with CCDX addon: %u", listeningPlayers);
        handler->PSendSysMessage("  Runtime blacklisted spells: %u", uint32(blacklist.size()));

        return true;
    }

    static bool HandleBlacklistAdd(ChatHandler* handler, uint32 spellId)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        CreatureCodex::AddToBlacklist(spellId);

        if (spellInfo && spellInfo->SpellName)
            handler->PSendSysMessage("[CreatureCodex] Blacklisted spell %u (%s).",
                spellId, (*spellInfo->SpellName)[LOCALE_enUS]);
        else
            handler->PSendSysMessage("[CreatureCodex] Blacklisted spell %u (unknown).", spellId);

        return true;
    }

    static bool HandleBlacklistRemove(ChatHandler* handler, uint32 spellId)
    {
        if (!CreatureCodex::RemoveFromBlacklist(spellId))
        {
            handler->PSendSysMessage("[CreatureCodex] Spell %u not in runtime blacklist.", spellId);
            return true;
        }

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (spellInfo && spellInfo->SpellName)
            handler->PSendSysMessage("[CreatureCodex] Removed spell %u (%s) from blacklist.",
                spellId, (*spellInfo->SpellName)[LOCALE_enUS]);
        else
            handler->PSendSysMessage("[CreatureCodex] Removed spell %u from blacklist.", spellId);

        return true;
    }

    static bool HandleBlacklistList(ChatHandler* handler)
    {
        auto blacklist = CreatureCodex::GetBlacklistCopy();

        if (blacklist.empty())
        {
            handler->SendSysMessage("[CreatureCodex] Runtime blacklist is empty.");
            return true;
        }

        handler->PSendSysMessage("[CreatureCodex] Runtime blacklist (%u entries):", uint32(blacklist.size()));

        for (uint32 spellId : blacklist)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
            if (spellInfo && spellInfo->SpellName)
                handler->PSendSysMessage("  Spell %u - %s", spellId, (*spellInfo->SpellName)[LOCALE_enUS]);
            else
                handler->PSendSysMessage("  Spell %u - (unknown)", spellId);
        }

        return true;
    }
};

void AddSC_creature_codex_commands()
{
    new codex_commandscript();
}
*/
