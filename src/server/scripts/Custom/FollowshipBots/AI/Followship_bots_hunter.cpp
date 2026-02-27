#include "CharmInfo.h"
#include "CreatureData.h"
#include "Pet.h"
#include "PhasingHandler.h"

#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"

#include "Followship_bots_hunter.h"

#include "Followship_bots_pet_handler.h"
#include "Followship_bots_spells_handler.h"

std::vector<FSBSpellDefinition> HunterSpellsTable =
{
    // Spell ID                             Spell Type              ManaCost %  HP % for heal   Chance           Dist/Range     SelfCast    Cooldown Ms     RoleMask

    // ANY
    { SPELL_HUMAN_WILL_TO_SURVIVE,          FSBSpellType::Heal,     0.f,        80.f,           100.f,           0.f,           true,       180000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_DWARF_STONEFORM,                FSBSpellType::Heal,     0.f,        80.f,           100.f,           0.f,           true,       120000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_DRAENEI_GIFT_NAARU,             FSBSpellType::Heal,     0.f,        50.f,           100.f,           30.f,          false,      120000,         FSB_RoleMask::FSB_ROLEMASK_ANY },

    { SPELL_HUNTER_EXHILARATION,            FSBSpellType::Heal,     0.f,        60.f,           100.f,           0.f,           true,       120000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_HUNTER_MEND_PET,                FSBSpellType::Heal,     0.f,        100.f,          100.f,           0.f,           true,       10000,          FSB_RoleMask::FSB_ROLEMASK_ANY },

    { SPELL_HUNTER_MARK,                    FSBSpellType::Damage,   0.f,        0.f,            100.f,           60.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_HUNTER_WING_CLIP,               FSBSpellType::Damage,   0.f,        0.f,            100.f,           40.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_HUNTER_SCARE_BEAST,             FSBSpellType::Damage,   0.f,        0.f,            100.f,           30.f,          false,      20000,          FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_HUNTER_DISENGAGE,               FSBSpellType::Damage,   0.f,        0.f,            100.f,           40.f,          true,       20000,          FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_HUNTER_COUNTER_SHOT,            FSBSpellType::Damage,   0.f,        0.f,            100.f,           40.f,          false,      24000,          FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_HUNTER_BINDING_SHOT,            FSBSpellType::Damage,   0.f,        0.f,            100.f,           30.f,          false,      45000,          FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_HUNTER_ASPECT_TURTLE,           FSBSpellType::Damage,   0.f,        0.f,            100.f,           0.f,           true,       180000,         FSB_RoleMask::FSB_ROLEMASK_ANY },

    { SPELL_HUNTER_SHOOT,                   FSBSpellType::Damage,   0.f,        0.f,            50.f,            40.f,          false,      1500,           FSB_RoleMask::FSB_ROLEMASK_ANY },

    { SPELL_HUNTER_ARCANE_SHOT,             FSBSpellType::Damage,   0.f,        0.f,            80.f,            40.f,          false,      1500,           FSB_RoleMask::FSB_ROLEMASK_ANY },

    { SPELL_HUNTER_STEADY_SHOT,             FSBSpellType::Damage,   0.f,        0.f,            50.f,            40.f,          false,      1500,           FSB_RoleMask::FSB_ROLEMASK_ANY },

    //BEAST
    { SPELL_HUNTER_BESTIAL_WRATH,           FSBSpellType::Damage,   0.f,        0.f,            100.f,           0.f,           true,       90000,          FSB_RoleMask::FSB_ROLEMASK_ASSIST },
    { SPELL_HUNTER_KILL_COMMAND,            FSBSpellType::Damage,   0.3f,       0.f,            100.f,           0.f,           true,       8000,           FSB_RoleMask::FSB_ROLEMASK_ASSIST },
    { SPELL_HUNTER_FLAMEFANG_PITCH,         FSBSpellType::Damage,   0.f,        0.f,            80.f,            40.f,          false,      60000,          FSB_RoleMask::FSB_ROLEMASK_ASSIST },
    { SPELL_HUNTER_WILDFIRE_BOMB,           FSBSpellType::Damage,   0.f,        0.f,            80.f,            40.f,          false,      18000,          FSB_RoleMask::FSB_ROLEMASK_ASSIST },
    { SPELL_HUNTER_COBRA_SHOT,              FSBSpellType::Damage,   0.f,        0.f,            80.f,            40.f,          false,      1500,           FSB_RoleMask::FSB_ROLEMASK_ASSIST },

    { SPELL_HUNTER_RAPTOR_STRIKE,           FSBSpellType::Damage,   0.f,        0.f,            70.f,            40.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ASSIST },
    { SPELL_HUNTER_MONGOOSE_BITE,           FSBSpellType::Damage,   0.f,        0.f,            70.f,            40.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ASSIST },

    { SPELL_HUNTER_BARBED_SHOT,             FSBSpellType::Damage,   0.f,        0.f,            50.f,            40.f,          false,      18000,          FSB_RoleMask::FSB_ROLEMASK_ASSIST },

    //MARK
    { SPELL_HUNTER_SERPENT_STING,           FSBSpellType::Damage,   0.f,        0.f,            100.f,           40.f,          false,      1500,           FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_HUNTER_CONCUSSIVE_SHOT,         FSBSpellType::Damage,   0.f,        0.f,            100.f,           40.f,          false,      5000,           FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_HUNTER_TRANQUILIZING_SHOT,      FSBSpellType::Damage,   0.f,        0.f,            100.f,           40.f,          false,      10000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_HUNTER_SCATTER_SHOT,            FSBSpellType::Damage,   0.f,        0.f,            100.f,           20.f,          false,      30000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_HUNTER_WAILLING_ARROW,          FSBSpellType::Damage,   0.f,        0.f,            80.f,            40.f,          false,      60000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_HUNTER_AIMED_SHOT,              FSBSpellType::Damage,   0.f,        0.f,            80.f,            40.f,          false,      15000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_HUNTER_MULTI_SHOT,              FSBSpellType::Damage,   0.f,        0.f,            80.f,            40.f,          false,      1500,           FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },

    { SPELL_HUNTER_RAPID_FIRE,              FSBSpellType::Damage,   0.f,        0.f,            50.f,            40.f,          false,      16000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    
};

namespace FSBHunter
{
    bool BotSummonPet(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->IsInCombat())
            return false;

        if (FSBPet::BotHasPet(bot))
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        auto botClass = baseAI->botClass;
        if (botClass != FSB_Class::Hunter)
            return false;

        uint32 petSource = FSBMgr::Get()->GetBotPetSourceForEntry(bot->GetEntry());
        Creature* originalPet = bot->FindNearestCreature(petSource, 10.f);

        FSBSpells::BotCastSpell(bot, SPELL_HUNTER_SUMMON_HYENA, bot);

        Creature* pet = ObjectAccessor::GetCreatureOrPetOrVehicle(*bot, bot->GetPetGUID());

        // place pet before player
        pet->Relocate(bot->GetPositionX() + 2, bot->GetPositionY(), bot->GetPositionZ(), float(M_PI) - bot->GetOrientation());

        FSBPet::SetBasePetInformation(bot, pet);
        
        if (originalPet)
            originalPet->DespawnOrUnsummon();
        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BotSummonPet() triggered for bot: {} and pet guid: {} and pet name: {}", bot->GetName(), bot->GetPetGUID(), pet->GetName());

        

        

        /*
        // set pet to defensive mode by default (some classes can't control controlled pets in fact).
        pet->SetReactState(REACT_DEFENSIVE);

        
        
        // caster have pet now
        bot->SetMinion(pet->ToPet(), true);


        pet->SetCreatorGUID(bot->GetGUID());
        
        //pet->SetUnitFlag(UNIT_FLAG_PLAYER_CONTROLLED);

        pet->ToPet()->InitStatsForLevel(level);
        pet->SetName("Delia");
        

        pet->GetCharmInfo()->SetPetNumber(sObjectMgr->GeneratePetNumber(), true);
        // this enables pet details window (Shift+P)
        pet->ToPet()->InitPetCreateSpells();
        //pet->InitLevelupSpellsForLevel();
        
        pet->SetObjectScale(0.8f);
        */
        return true;
    }
}
