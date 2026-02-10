#include "Followship_bots_warlock.h"
#include "Followship_bots_utils.h"
#include "Followship_bots_mgr.h"

#include "Followship_bots_group_handler.h"

std::vector<FSBSpellDefinition> WarlockSpellsTable =
{
    // Spell                                Spell                   Mana        HP % for    Chance        Dist/Range     SelfCast    Cooldown Ms     RoleMask
    // ID                                   Type                    Cost %      Heal

    //ANY
    { SPELL_WARLOCK_GIFT_NAARU,             FSBSpellType::Heal,     0.f,        50.f,       100.f,        40.f,          false,      120000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_WARLOCK_UNENDING_RESOLVE,       FSBSpellType::Damage,   0.f,        0.f,        50.f,         2.f,           true,       180000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_WARLOCK_SHADOW_BOLT,            FSBSpellType::Damage,   0.f,        0.f,        90.f,         40.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY },

    { SPELL_WARLOCK_FEAR,                   FSBSpellType::Damage,   0.f,        0.f,        80.f,         20.f,          false,      8000,           FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_WARLOCK_DRAIN_LIFE,             FSBSpellType::Damage,   0.f,        0.f,        80.f,         20.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY },
    
    // AFFLICTION
    { SPELL_WARLOCK_CURSE_TONGUES,          FSBSpellType::Damage,   0.f,        0.f,        100.f,        40.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_RANGED_AFFLICTION },

    { SPELL_WARLOCK_HAUNT,                  FSBSpellType::Damage,   0.2f,       0.f,        90.f,         40.f,          false,      15000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_AFFLICTION },
    
    { SPELL_WARLOCK_CORRUPTION,             FSBSpellType::Damage,   0.f,        0.f,        80.f,         40.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_RANGED_AFFLICTION },
    { SPELL_WARLOCK_AGONY,                  FSBSpellType::Damage,   0.f,        0.f,        80.f,         40.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_RANGED_AFFLICTION },
    { SPELL_WARLOCK_CURSE_WEAKNESS,         FSBSpellType::Damage,   0.f,        0.f,        80.f,         40.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_RANGED_AFFLICTION },
    
    { SPELL_WARLOCK_SEED_CORRUPTION,        FSBSpellType::Damage,   0.f,        0.f,        70.f,         30.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_RANGED_AFFLICTION },
    { SPELL_WARLOCK_UNSTABLE_AFFLICTION,    FSBSpellType::Damage,   0.f,        0.f,        70.f,         30.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_RANGED_AFFLICTION },

    // DEMONOLOGY
    { SPELL_WARLOCK_WILD_IMPS,              FSBSpellType::Damage,   0.5f,       0.f,        100.f,        40.f,          false,      1800000,        FSB_RoleMask::FSB_ROLEMASK_RANGED_DEMONOLOGY },
    { SPELL_WARLOCK_DEMONIC_TYRANT,         FSBSpellType::Damage,   0.f,        0.f,        100.f,        40.f,          false,      60000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DEMONOLOGY },
    { SPELL_WARLOCK_CALL_DREADSTALKERS,     FSBSpellType::Damage,   0.05f,      0.f,        100.f,        40.f,          false,      20000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DEMONOLOGY },
    { SPELL_WARLOCK_DEMONBOLT,              FSBSpellType::Damage,   0.f,        0.f,        90.f,         40.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_RANGED_DEMONOLOGY },

    // DESTRO
    { SPELL_WARLOCK_SUMMON_INFERNAL,        FSBSpellType::Damage,   0.3f,       0.f,        100.f,        40.f,          false,      180000,         FSB_RoleMask::FSB_ROLEMASK_RANGED_DESTRUCTION },
    { SPELL_WARLOCK_CATACLYSM,              FSBSpellType::Damage,   0.3f,       0.f,        100.f,        40.f,          false,      30000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DESTRUCTION },
    { SPELL_WARLOCK_IMMOLATE,               FSBSpellType::Damage,   0.02f,      0.f,        100.f,        30.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_RANGED_DESTRUCTION },
    { SPELL_WARLOCK_INCINERATE,             FSBSpellType::Damage,   0.04f,      0.f,        100.f,        40.f,          false,      12000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DESTRUCTION },
    //{ SPELL_PALADIN_HAMMER_OF_WRATH,        FSBSpellType::Damage,   0.04f,      0.f,        90.f,         40.f,          false,      12000,          FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    //{ SPELL_PALADIN_AVENGING_WRATH,         FSBSpellType::Damage,   0.f,        0.f,        50.f,         2.f,           true,       120000,         FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },

};

namespace FSBWarlock
{
    void HandleOnSpellCast(Creature* bot, uint32 spellId)
    {
        Unit* target = bot->GetVictim();

        switch (spellId)
        {
        case SPELL_WARLOCK_CALL_DREADSTALKERS:
            //if(target)
            //    bot->CastSpell(target, SPELL_WARLOCK_CALL_DREADSTALKERS);
            break;

        default:
            break;
        }
    }

    bool BotOOCBuffSoulstone(Creature* bot, uint32& globalCooldown, const std::vector<Unit*> botGroup)
    {
        if (!bot || !bot->IsAlive())
            return false;

        Unit* target = FSBGroup::BotGetFirstGroupHealer(botGroup);
        uint32 spellId = SPELL_WARLOCK_SOULSTONE;
        uint32 visualId = SPELL_WARLOCK_SOULSTONE_VISUAL;
        bool check = false;

        if (target && target->IsAlive())
        {
            if (!target->HasAura(spellId))
            {
                bot->AddAura(spellId, target);
                bot->CastSpell(target, visualId, false);
                check = true;
            }
        }

        else if (!target)
        {
            Player* player = FSBMgr::GetBotOwner(bot);

            if (player && player->IsAlive())
            {
                bot->CastSpell(player, spellId, false);
                check = true;
            }
        }

        else
        {
            bot->AddAura(spellId, bot);
            bot->CastSpell(bot, visualId, false);
            check = true;
        }

        if (check)
        {
            uint32 now = getMSTime();
            globalCooldown = now + 1500;
            return true;
        } else
        return false;
    }

    bool BotOOCBuffSelf(Creature* bot, uint32& globalCooldown, uint32& selfBuffTimer, uint32& outSpellId)
    {
        if (!bot)
            return false;

        uint32 now = getMSTime();

        bool isDemonology = FSBUtils::GetRole(bot) == FSB_ROLE_RANGED_DEMONOLOGY;

        if (isDemonology && !bot->HasAura(SPELL_WARLOCK_DEMON_ARMOR))
        {

            bot->CastSpell(bot, SPELL_WARLOCK_DEMON_ARMOR, false);
            selfBuffTimer = now + 60000; // 1 minute
            globalCooldown = now + 1500; // use 10s cooldown to not interrup duration of channel spell
            outSpellId = SPELL_WARLOCK_DEMON_ARMOR;

            return true;
        }

        return false;
    }

    bool BotSummonRandomDemon(Creature* bot, uint32& globalCooldown, bool& demonDead)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (!demonDead)
            return false;

        FSB_Roles role = FSBUtils::GetRole(bot);

        uint32 spellId = SPELL_WARLOCK_SUMMON_IMP;

        if(role == FSB_ROLE_RANGED_DEMONOLOGY)
            spellId = RAND(
                SPELL_WARLOCK_SUMMON_IMP,
                SPELL_WARLOCK_SUMMON_SUCCUBUS,
                SPELL_WARLOCK_SUMMON_VOIDWALKER,
                SPELL_WARLOCK_SUMMON_FELHUNTER,
                SPELL_WARLOCK_SUMMON_FELGUARD,
                SPELL_WARLOCK_SUMMON_DOOMGUARD);

        //bot->SetPowerType(POWER_SOUL_SHARDS, true);
        //bot->SetMaxPower(POWER_SOUL_SHARDS, 50);
        //bot->SetPower(POWER_SOUL_SHARDS, 50, true);

        SpellCastResult result = bot->CastSpell(bot, spellId, false);

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB BotSummonRandomDemon result {} for bot {}", result, bot->GetName());

        

        if (result == SPELL_CAST_OK)
        {
            uint32 now = getMSTime();

            //demonDead = false;
            globalCooldown = now + 1500;
            

            return true;
        }

        return false;
    }

    bool BotHasUnendingResolve(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->HasAura(SPELL_WARLOCK_UNENDING_RESOLVE))
            return true;

        return false;

    }
}
