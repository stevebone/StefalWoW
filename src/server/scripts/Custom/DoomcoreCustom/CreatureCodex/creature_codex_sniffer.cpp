/*
#include "ScriptMgr.h"
#include "Creature.h"
#include "Player.h"
#include "SpellInfo.h"
#include "SpellAuraEffects.h"
#include "WorldSession.h"
#include "ChatPackets.h"
#include "SharedDefines.h"
#include "Object.h"

#include <mutex>
#include <unordered_set>
*/
/*
 * CreatureCodex Server-Side Sniffer
 *
 * Broadcasts creature spell casts and aura applications to nearby players
 * who have the CreatureCodex addon installed (registered the "CCDX" addon
 * prefix). This lets the addon capture ALL spell casts including
 * instant/hidden ones the client API blocks.
 *
 * Message protocol (pipe-delimited):
 *   SC|entry|spellID|school|name|hp%   -- Spell Cast complete
 *   SS|entry|spellID|school|name|hp%   -- Spell Start (cast begun)
 *   CF|entry|spellID|school|name|hp%   -- Channel Finished
 *   AA|entry|spellID|school|name|hp%   -- Aura Applied
 *
 * Requires 4 core hooks added to UnitScript:
 *   OnCreatureSpellCast, OnCreatureSpellStart, OnCreatureChannelFinished, OnAuraApply
 */
/*
static constexpr char const* CCDX_PREFIX = "CCDX";
static constexpr float CCDX_BROADCAST_RANGE = 100.0f;

// Spells to never broadcast (noise reduction)
static constexpr uint32 BLACKLISTED_SPELLS[] = {
    1604,   // Dazed
    6603,   // Auto Attack
    75,     // Auto Shot
    3018,   // Shoot
    1784,   // Stealth
    7744,   // Will of the Forsaken
    20549,  // War Stomp
    26297,  // Berserking
    20577,  // Cannibalize
};

// Creature entries to never broadcast
static constexpr uint32 BLACKLISTED_CREATURES[] = {
    0, 1,
    19871,  // WorldTrigger
    21252,  // World Trigger (Large AOI)
    22515,  // World Trigger (Large AOI, Not Immune)
};

// ============================================================
// Runtime blacklist (managed by .codex command)
// ============================================================

namespace CreatureCodex
{
    static std::mutex s_blacklistMutex;
    static std::unordered_set<uint32> s_runtimeBlacklist;

    bool IsRuntimeBlacklisted(uint32 spellId)
    {
        std::lock_guard<std::mutex> lock(s_blacklistMutex);
        return s_runtimeBlacklist.contains(spellId);
    }

    void AddToBlacklist(uint32 spellId)
    {
        std::lock_guard<std::mutex> lock(s_blacklistMutex);
        s_runtimeBlacklist.insert(spellId);
    }

    bool RemoveFromBlacklist(uint32 spellId)
    {
        std::lock_guard<std::mutex> lock(s_blacklistMutex);
        return s_runtimeBlacklist.erase(spellId) > 0;
    }

    std::unordered_set<uint32> GetBlacklistCopy()
    {
        std::lock_guard<std::mutex> lock(s_blacklistMutex);
        return s_runtimeBlacklist;
    }
}

static bool IsBlacklistedSpell(uint32 spellId)
{
    for (uint32 id : BLACKLISTED_SPELLS)
        if (id == spellId)
            return true;
    return CreatureCodex::IsRuntimeBlacklisted(spellId);
}

static bool IsBlacklistedCreature(uint32 entry)
{
    for (uint32 id : BLACKLISTED_CREATURES)
        if (id == entry)
            return true;
    return false;
}

static void BroadcastToNearbyPlayers(Creature* creature, SpellInfo const* spell, char const* msgType)
{
    if (!creature || !spell)
        return;

    uint32 entry = creature->GetEntry();
    if (IsBlacklistedCreature(entry))
        return;

    uint32 spellId = spell->Id;
    if (IsBlacklistedSpell(spellId))
        return;

    // Build message: TYPE|entry|spellID|schoolMask|creatureName|hp%
    uint32 hpPct = static_cast<uint32>(creature->GetHealthPct());
    std::string msg = std::string(msgType) + "|"
        + std::to_string(entry) + "|"
        + std::to_string(spellId) + "|"
        + std::to_string(uint32(spell->SchoolMask)) + "|"
        + creature->GetName() + "|"
        + std::to_string(hpPct);

    // Truncate to addon message size limit
    if (msg.size() > 255)
        msg.resize(255);

    // Get nearby players
    std::list<Player*> players;
    creature->GetPlayerListInGrid(players, CCDX_BROADCAST_RANGE, false);

    for (Player* player : players)
    {
        if (!player || !player->GetSession())
            continue;

        // Only send to players who have CreatureCodex installed
        if (!player->GetSession()->IsAddonRegistered(CCDX_PREFIX))
            continue;

        // Send as self-whisper addon message (standard pattern)
        WorldPackets::Chat::Chat chat;
        chat.Initialize(CHAT_MSG_WHISPER, LANG_ADDON, player, player, msg, 0, "", DEFAULT_LOCALE, CCDX_PREFIX);
        player->GetSession()->SendPacket(chat.Write());
    }
}

class CreatureCodexSniffer : public UnitScript
{
public:
    CreatureCodexSniffer() : UnitScript("creature_codex_sniffer") {}

    // Fires after Spell::SendSpellGo -- the spell has been cast
    void OnCreatureSpellCast(Creature* creature, SpellInfo const* spell) override
    {
        BroadcastToNearbyPlayers(creature, spell, "SC");
    }

    // Fires at the start of casting (beginning of Spell::prepare)
    void OnCreatureSpellStart(Creature* creature, SpellInfo const* spell) override
    {
        BroadcastToNearbyPlayers(creature, spell, "SS");
    }

    // Fires when a channeled spell finishes
    void OnCreatureChannelFinished(Creature* creature, SpellInfo const* spell) override
    {
        BroadcastToNearbyPlayers(creature, spell, "CF");
    }

    // Fires when any aura is applied to a unit
    void OnAuraApply(Unit* target, AuraApplication* aurApp) override
    {
        if (!aurApp)
            return;

        Aura* aura = aurApp->GetBase();
        if (!aura)
            return;

        // We only care about auras cast BY creatures
        Unit* caster = aura->GetCaster();
        if (!caster)
            return;

        Creature* creature = caster->ToCreature();
        if (!creature)
            return;

        SpellInfo const* spell = aura->GetSpellInfo();
        if (!spell)
            return;

        BroadcastToNearbyPlayers(creature, spell, "AA");
    }
};

void AddSC_creature_codex_sniffer()
{
    new CreatureCodexSniffer();
}
*/
