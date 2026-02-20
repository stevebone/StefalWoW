#include <random>

#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "SpellMgr.h"
#include "ThreatManager.h"
#include "Unit.h"

#include "Followship_bots_druid.h"
#include "Followship_bots_mage.h"
#include "Followship_bots_paladin.h"
#include "Followship_bots_priest.h"
#include "Followship_bots_warlock.h"
#include "Followship_bots_warrior.h"
#include "Followship_bots_utils.h"
#include "Followship_bots_mgr.h"

#include "Followship_bots_powers_handler.h"
#include "Followship_bots_spells_handler.h"

std::unordered_map<uint32 /*spellId*/, uint32 /*readyTimeMs*/> spellCooldowns;



std::unordered_map<FSB_Class, DispelAbility> DispelTable =
{
    { FSB_Class::Priest,  { SPELL_PRIEST_PURIFY,   { DISPEL_MAGIC, DISPEL_DISEASE }, 8000 } },
    { FSB_Class::Paladin, { SPELL_PALADIN_CLEANSE,       { DISPEL_MAGIC, DISPEL_DISEASE, DISPEL_POISON }, 8000  }},
    { FSB_Class::Druid,   { SPELL_DRUID_REMOVE_CORRUPTION, { DISPEL_CURSE, DISPEL_POISON }, 8000  }},
    { FSB_Class::Druid,   { SPELL_DRUID_NATURE_CURE,       { DISPEL_MAGIC, DISPEL_CURSE, DISPEL_POISON }, 8000 }},
    //{ FSB_Class::Shaman,  { SPELL_SHAMAN_CLEANSE_SPIRIT, { DISPEL_CURSE } } },
    { FSB_Class::Mage,    { SPELL_MAGE_REMOVE_CURSE,     { DISPEL_CURSE }, 8000  }}
};

std::unordered_map<FSB_Class, OffensiveDispelAbility> OffensiveDispelTable =
{
    { FSB_Class::Priest,  { SPELL_PRIEST_DISPEL_MAGIC, OFFDISPEL_MAGIC } },
    //{ FSB_Class::Shaman,  { SPELL_SHAMAN_PURGE,        OFFDISPEL_MAGIC } },
    // Hunter
    { FSB_Class::Mage,    { SPELL_MAGE_SPELL_STEAL,     OFFDISPEL_STEAL } }
};

namespace FSBSpellsUtils
{
    bool IsSpellReady(uint32 spellId)
    {
        uint32 now = getMSTime();

        auto it = spellCooldowns.find(spellId);
        if (it == spellCooldowns.end())
            return true; // no cooldown stored ? ready

        return now >= it->second;
    }

    void PutSpellOnCooldown(uint32 spellId, uint32 cooldownMs)
    {
        if (!spellId || !cooldownMs)
            return;

        uint32 now = getMSTime();

        spellCooldowns[spellId] = now + cooldownMs;
    }

    bool CanCastNow(Unit* me, uint32 now, uint32 globalCooldownUntil)
    {
        return me
            && now >= globalCooldownUntil
            && !me->HasUnitState(UNIT_STATE_CASTING);
    }

    bool CheckSpellContextRequirements(Creature* bot, uint32 spellId, Unit* target)
    {
        switch (spellId)
        {

            // Can only be cast on more than 2/3 attackers
            // OR when 2/3 attackers are around
            // This way we limit spells to CC instead of general/spam use
        case SPELL_WARLOCK_HOWL_TERROR:
        case SPELL_DRUID_HIBERNATE:
        case SPELL_DRUID_STARFIRE:
        case SPELL_PALADIN_DIVINE_STORM:
        case SPELL_WARRIOR_COLOSSUS_SMASH:
        case SPELL_WARRIOR_CLEAVE:
        case SPELL_MAGE_FLAMESTRIKE:
        case SPELL_MAGE_ARCANE_EXPLOSION:
        case SPELL_MAGE_BLIZZARD:
        case SPELL_MAGE_FROST_NOVA:
        case SPELL_MAGE_POLYMORPH:
        case SPELL_PRIEST_PSYCHIC_SCREAM:
        {
            return CheckCrowdControlRequirements(bot);
        }

        case SPELL_PRIEST_DESPERATE_PRAYER:
        {
            return target == bot;
        }

        case SPELL_MAGE_ICE_BLOCK:
        {
            return target == bot && bot->GetHealthPct() < 10;
        }

        case SPELL_MAGE_EVOCATION:
            return bot->GetPowerPct(POWER_MANA) < 50;

        case SPELL_MAGE_ARCANE_SURGE:
            return bot->GetPowerPct(POWER_MANA) < 10;

        case SPELL_DRUID_BEAR_SURVIVAL_INSTINCTS:
        case SPELL_DRUID_IRONBARK:
        case SPELL_DRUID_CELESTIAL_ALIGNMENT:
            return target == bot && bot->GetHealthPct() < 50;

        case SPELL_MAGE_ICE_BARRIER:
            return target == bot && bot->GetHealthPct() < 75;

        case SPELL_PRIEST_SILENCE:
        case SPELL_WARLOCK_CURSE_TONGUES:
        case SPELL_PALADIN_REBUKE:
        case SPELL_WARRIOR_PUMMEL:
        case SPELL_MAGE_COUNTERSPELL:
            return target && target->HasUnitState(UNIT_STATE_CASTING);

        case SPELL_MAGE_BLINK:
            return !bot->GetMap()->IsDungeon();

        case SPELL_WARRIOR_EXECUTE:
            return target && target->GetHealthPct() <= 20;

        case SPELL_WARRIOR_LAST_STAND:
            return target == bot && bot->GetHealthPct() < 30;

        case SPELL_PALADIN_LAY_ON_HANDS:
        case SPELL_PALADIN_DIVINE_SHIELD:
            return bot->GetPowerPct(POWER_MANA) < 20;

        case SPELL_WARRIOR_GIFT_NAARU:
            return FSBMgr::Get()->GetBotRaceForEntry(bot->GetEntry()) == FSB_Race::Draenei;

        case SPELL_DWARF_STONEFORM:
            return CheckDwarfStoneformRequirements(bot);

        case SPELL_HUMAN_WILL_TO_SURVIVE:
            return FSBMgr::Get()->GetBotRaceForEntry(bot->GetEntry()) == FSB_Race::Human && HasAnyMechanic(bot, { MECHANIC_STUN });

        default:
            return true;
        }
    }

    bool CheckDwarfStoneformRequirements(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (FSBMgr::Get()->GetBotRaceForEntry(bot->GetEntry()) != FSB_Race::Dwarf)
            return false;

        if (HasAnyNegativeDispel(bot, { DISPEL_CURSE, DISPEL_POISON, DISPEL_MAGIC, DISPEL_DISEASE }))
            return true;

        if (HasAnyMechanic(bot, { MECHANIC_BLEED }))
            return true;

        return false;
    }

    bool CheckCrowdControlRequirements(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (FSBCombatUtils::HasHostileInRange(bot, 8.f, 2))
            return true;

        return false;
    }

    DispelType ConvertAuraToDispelType(Aura* aura)
    {
        switch (aura->GetSpellInfo()->Dispel)
        {
        case DISPEL_MAGIC:   return DISPEL_MAGIC;
        case DISPEL_CURSE:   return DISPEL_CURSE;
        case DISPEL_DISEASE: return DISPEL_DISEASE;
        case DISPEL_POISON:  return DISPEL_POISON;
        default:             return (DispelType)-1;
        }
    }

    // Gets the Mana Potion spell id on a provided level
    uint32 GetManaPotionSpellForLevel(uint8 level)
    {
        for (auto const& entry : ManaPotionTable)
        {
            if (level >= entry.minLevel && level <= entry.maxLevel)
                return entry.spellId;
        }
        return 0;
    }

    uint32 GetHealthPotionSpellForLevel(uint8 level)
    {
        for (auto const& entry : HealthPotionTable)
        {
            if (level >= entry.minLevel && level <= entry.maxLevel)
                return entry.spellId;
        }
        return 0;
    }

    std::string GetSpellName(uint32 spellId)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo || !spellInfo->SpellName)
            return "unknown spell";

        std::string name = spellInfo->SpellName->Str[LOCALE_enUS];
        return name.empty() ? "unknown spell" : name;
    }

    const MountSpellList* GetMountSpellsForBot(FSB_Race race, uint8 level)
    {
        auto raceIt = MountSpells.find(race);
        if (raceIt == MountSpells.end())
            return nullptr;

        auto& levelMap = raceIt->second;

        // Find the highest level <= bot level
        MountSpellList const* bestList = nullptr;

        for (auto const& [reqLevel, spells] : levelMap)
        {
            if (level >= reqLevel)
                bestList = &spells;
        }

        return bestList; // may be nullptr if no mounts available
    }

    uint32 GetRandomMountSpellForBot(Creature* bot)
    {
        if (!bot)
            return 0;

        FSB_Race race = FSBMgr::Get()->GetBotRaceForEntry(bot->GetEntry());

        MountSpellList const* spells = GetMountSpellsForBot(race, bot->GetLevel());
        if (!spells || spells->empty())
            return 0;

        return (*spells)[urand(0, spells->size() - 1)];
    }

    bool CastRandomMountLevelSpell(Creature* bot)
    {
        if (!bot)
            return false;

        uint32 spellId = GetRandomMountSpellForBot(bot);
        if (!spellId)
            return false;

        SpellCastResult result = bot->CastSpell(bot, spellId, true);

        if (result == SPELL_CAST_OK)
            return true;

        return false;
    }

    Aura* FindEnemyBuffToDispel(Unit* enemy)
    {
        for (auto const& auraPair : enemy->GetAppliedAuras())
        {
            Aura* aura = auraPair.second->GetBase();
            if (!aura)
                continue;

            SpellInfo const* info = aura->GetSpellInfo();

            // Only beneficial auras
            if (!info->IsPositive())
                continue;

            // Only magic buffs
            if (info->Dispel == DISPEL_MAGIC)
                return aura;
        }

        return nullptr;
    }

    bool BotHasHealSpells(Creature* bot)
    {
        if (!bot)
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        auto botClass = baseAI->botClass;
        auto botRole = baseAI->botRole;
        uint32 botRoleMask = RoleToMask(botRole);

        auto it = sBotSpellTables.find(botClass);
        if (it == sBotSpellTables.end() || !it->second)
            return false;

        const auto& table = *(it->second);

        for (auto const& def : table)
        {
            if (def.type == FSBSpellType::Heal &&
                (def.allowedRoles == FSB_ROLEMASK_ANY || (def.allowedRoles & botRoleMask) != 0))
                return true;
        }

        return false;
    }

    bool BotHasHealSpellsForSelf(Creature* bot)
    {
        if (!bot)
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        auto botClass = baseAI->botClass;
        auto botRole = baseAI->botRole;
        uint32 botRoleMask = RoleToMask(botRole);

        auto it = sBotSpellTables.find(botClass);
        if (it == sBotSpellTables.end() || !it->second)
            return false;

        const auto& table = *(it->second);

        for (auto const& def : table)
        {
            if ((def.type == FSBSpellType::Heal && def.isSelfCast) &&
                (def.allowedRoles == FSB_ROLEMASK_ANY || (def.allowedRoles & botRoleMask) != 0))
                return true;
        }

        return false;
    }

    bool HasAnyMechanic(Unit* u, std::initializer_list<Mechanics> mechanics)
    {
        if (!u || !u->IsAlive())
            return false;

        for (auto const& auraPair : u->GetAppliedAuras())
        {
            Aura* aura = auraPair.second->GetBase();
            if (!aura)
                continue;

            SpellInfo const* info = aura->GetSpellInfo();
            if (!info)
                continue;

            for (Mechanics m : mechanics)
            {
                if (info->Mechanic == m)
                    return true;
            }
        }

        return false;
    }

    bool HasAnyAuraType(Unit* u, std::initializer_list<AuraType> auraTypes)
    {
        if (!u || !u->IsAlive())
            return false;

        for (auto const& auraPair : u->GetAppliedAuras())
        {
            Aura* aura = auraPair.second->GetBase();
            if (!aura)
                continue;

            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            {
                AuraEffect const* eff = aura->GetEffect(i);
                if (!eff)
                    continue;

                for (AuraType t : auraTypes)
                {
                    if (eff->GetAuraType() == t)
                        return true;
                }
            }
        }

        return false;
    }

    bool HasAnyNegativeDispel(Unit* u, std::initializer_list<DispelType> dispels)
    {
        if (!u || !u->IsAlive())
            return false;

        for (auto const& auraPair : u->GetAppliedAuras())
        {
            Aura* aura = auraPair.second->GetBase();
            if (!aura)
                continue;

            SpellInfo const* info = aura->GetSpellInfo();
            if (!info)
                continue;

            // Only negative auras
            if (info->IsPositive())
                continue;

            DispelType auraDispel = DispelType(info->Dispel);

            for (DispelType d : dispels)
            {
                if (auraDispel == d)
                    return true;
            }
        }

        return false;
    }
}
