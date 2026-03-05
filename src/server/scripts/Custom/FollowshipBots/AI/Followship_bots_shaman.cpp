#include "Log.h"
#include "ScriptedCreature.h"
#include "SpellHistory.h"

#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"

#include "Followship_bots_spells_handler.h"

std::vector<FSBSpellDefinition> ShamanSpellsTable =
{
    // Spell ID                             Spell Type              ManaCost %  HP % for heal   Chance           Dist/Range     SelfCast    Cooldown Ms     RoleMask

    // ANY
    { SPELL_HUMAN_WILL_TO_SURVIVE,          FSBSpellType::Heal,     0.f,        80.f,           100.f,           0.f,           true,       180000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_DWARF_STONEFORM,                FSBSpellType::Heal,     0.f,        80.f,           100.f,           0.f,           true,       120000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_DRAENEI_GIFT_NAARU,             FSBSpellType::Heal,     0.f,        50.f,           100.f,           30.f,          false,      120000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_PANDAREN_QUAKING_PALM,          FSBSpellType::Damage,   0.f,        0.f,            100.f,           2.f,           false,      120000,         FSB_RoleMask::FSB_ROLEMASK_ANY },

    { SPELL_SHAMAN_HEALING_SURGE,           FSBSpellType::Heal,     0.1f,       50.f,           100.f,           40.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY },

    { SPELL_SHAMAN_LIGHTNING_BOLT,          FSBSpellType::Damage,   0.f,        0.f,            80.f,            40.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_SHAMAN_PRIMAL_STRIKE,           FSBSpellType::Damage,   0.f,        0.f,            80.f,            2.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY },
};

namespace FSBShaman
{
    bool BotOOCBuffSelf(Creature* bot, uint32& outSpellId)
    {
        if (!bot || !bot->IsAlive())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        auto& buffTimer = baseAI->botBuffsTimer;
        auto& cooldown = baseAI->botGlobalCooldown;

        uint32 now = getMSTime();

        if (!bot->HasAura(SPELL_SHAMAN_SKYFURY))
        {
            Unit* target = nullptr;
            if (Player* player = FSBMgr::Get()->GetBotOwner(bot))
                target = player;
            else target = bot;

            uint32 spellId = SPELL_SHAMAN_SKYFURY;
            if (FSBSpells::BotCastSpell(bot, spellId, target))
            {
                //buffTimer = now + 60000; // 1 minute we set it after the second buff is set
                cooldown = now + 1500; // 
                outSpellId = spellId;
                TC_LOG_DEBUG("scripts.fsb.buffs", "FSB: Shaman Spell Cast: Skyfury on target: {}", bot->GetName());
                return true;
            }
        }

        else if (!bot->HasAura(SPELL_SHAMAN_LIGHTNING_SHIELD))
        {
            uint32 spellId = SPELL_SHAMAN_LIGHTNING_SHIELD;
            if (FSBSpells::BotCastSpell(bot, spellId, bot))
            {
                buffTimer = now + 60000; // 1 minute
                cooldown = now + 1500; // 
                outSpellId = spellId;
                TC_LOG_DEBUG("scripts.fsb.buffs", "FSB: Shaman Spell Cast: Lightning Shield on target: {}", bot->GetName());
                return true;
            }
        }

        return false;
    }

    bool BotInitialCombatSpells(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (!bot->IsInCombat())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        if (baseAI->botCastedCombatBuffs)
            return false;

        uint32 totem1Spell = SPELL_SHAMAN_EARTH_TOTEM; // all roles
        uint32 totem2Spell = 0;
        uint32 totem3Spell = 0;
        uint32 totem4Spell = 0;

        Position pos = Position{ bot->GetPositionX() + frand(-2.f, 2.f), bot->GetPositionY() + frand(-2.f, 2.f), bot->GetPositionZ() };

        //Unit* target = nullptr;
        //Unit* tank = FSBGroup::BotGetFirstGroupTank(botGroup);

        switch (baseAI->botRole)
        {
        case FSB_ROLE_HEALER:
            totem2Spell = SPELL_SHAMAN_HEALING_STREAM_TOTEM;
            totem3Spell = SPELL_SHAMAN_MANA_TIDE_TOTEM;
            totem4Spell = SPELL_SHAMAN_HEALING_TIDE_TOTEM;
            break;
        case FSB_ROLE_TANK:
            totem2Spell = SPELL_SHAMAN_EARTHGRAB_TOTEM;
            totem3Spell = SPELL_SHAMAN_FIRE_NOVA_TOTEM;
            break;
        case FSB_ROLE_MELEE_DAMAGE:
            totem2Spell = SPELL_SHAMAN_WIND_RUSH_TOTEM;
            totem3Spell = SPELL_SHAMAN_LIQUID_MAGMA_TOTEM;
            break;
        default:
            break;
        }

        uint32 now = getMSTime();

        if (!bot->GetSpellHistory()->HasCooldown(totem1Spell))
        {
            if (FSBSpells::BotCastSpellatLocation(bot, totem1Spell, pos))
            {
                baseAI->botGlobalCooldown = now + 1500;
                TC_LOG_DEBUG("scripts.fsb.buffs", "FSB: Shaman Initial Totem Spell Cast: {} at location: {}", FSBSpellsUtils::GetSpellName(totem1Spell), pos.ToString());
                return true;
            }
        }

        if (totem2Spell && !bot->GetSpellHistory()->HasCooldown(totem2Spell))
        {
            if (totem2Spell == SPELL_SHAMAN_HEALING_STREAM_TOTEM)
            {
                Unit* healingStreamtotem = bot->FindNearestCreature(3527, 40.f);
                if (!healingStreamtotem && FSBSpells::BotCastSpell(bot, totem2Spell, bot))
                {
                    baseAI->botGlobalCooldown = now + 1500;
                    TC_LOG_DEBUG("scripts.fsb.buffs", "FSB: Shaman Initial Totem Spell Cast: {}", FSBSpellsUtils::GetSpellName(totem2Spell));
                    return true;
                }
            }
            else
            {
                if (FSBSpells::BotCastSpellatLocation(bot, totem2Spell, pos))
                {
                    baseAI->botGlobalCooldown = now + 1500;
                    TC_LOG_DEBUG("scripts.fsb.buffs", "FSB: Shaman Initial Totem Spell Cast: {} at location: {}", FSBSpellsUtils::GetSpellName(totem2Spell), pos.ToString());
                    return true;
                }
            }
        }

        if (totem3Spell && !bot->GetSpellHistory()->HasCooldown(totem3Spell))
        {
            if (totem3Spell == SPELL_SHAMAN_LIQUID_MAGMA_TOTEM)
            {
                if (FSBSpells::BotCastSpellatLocation(bot, totem3Spell, pos))
                {
                    baseAI->botGlobalCooldown = now + 1500;
                    TC_LOG_DEBUG("scripts.fsb.buffs", "FSB: Shaman Initial Totem Spell Cast: {} at location: {}", FSBSpellsUtils::GetSpellName(totem3Spell), pos.ToString());
                    return true;
                }
            }
            else if (totem3Spell == SPELL_SHAMAN_MANA_TIDE_TOTEM || totem3Spell == SPELL_SHAMAN_FIRE_NOVA_TOTEM)
            {
                if (FSBSpells::BotCastSpell(bot, totem3Spell, bot))
                {
                    baseAI->botGlobalCooldown = now + 1500;
                    TC_LOG_DEBUG("scripts.fsb.buffs", "FSB: Shaman Initial Totem Spell Cast: {}", FSBSpellsUtils::GetSpellName(totem3Spell));
                    return true;
                }
            }
        }

        if (totem4Spell && !bot->GetSpellHistory()->HasCooldown(totem4Spell))
        {
            if (totem4Spell == SPELL_SHAMAN_HEALING_TIDE_TOTEM)
            {
                if (FSBSpells::BotCastSpell(bot, totem4Spell, bot))
                {
                    baseAI->botGlobalCooldown = now + 1500;
                    TC_LOG_DEBUG("scripts.fsb.buffs", "FSB: Shaman Initial Totem Spell Cast: {}", FSBSpellsUtils::GetSpellName(totem4Spell));
                    return true;
                }
            }
        }
        return false;

    }
}
