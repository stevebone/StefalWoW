#include "BaseEntity.h"
#include "CharmInfo.h"
#include "Creature.h"
#include "Pet.h"
#include "PhasingHandler.h"

#include "Followship_bots_mgr.h"

#include "Followship_bots_pet_handler.h"

namespace FSBPet
{
    bool SetBasePetInformation(Creature* owner, Creature* pet)
    {
        if (!owner || !owner->IsAlive())
            return false;

        if (!pet || !pet->IsAlive())
            return false;

        if (owner->IsInCombat() || pet->IsInCombat())
            return false;

        pet->SetName(FSBMgr::Get()->GetBotPetNameForEntry(owner->GetEntry()));

        //entry needs to belong to the creature we want the model
        CreatureTemplate const* creatureInfo = sObjectMgr->GetCreatureTemplate(43292); //pet->GetEntry()
        CreatureModel model = *ObjectMgr::ChooseDisplayId(creatureInfo);
        pet->SetDisplayId(model.CreatureDisplayID, true);

        pet->SetModCastingSpeed(1.0f);
        pet->SetModSpellHaste(1.0f);
        pet->SetModHaste(1.0f);
        pet->SetModRangedHaste(1.0f);
        pet->SetModHasteRegen(1.0f);
        pet->SetModTimeRate(1.0f);
        pet->SetSpellEmpowerStage(-1);

        pet->SetSpeedRate(MOVE_WALK, creatureInfo->speed_walk);
        pet->SetSpeedRate(MOVE_RUN, creatureInfo->speed_run);
        pet->SetSpeedRate(MOVE_SWIM, 1.0f); // using 1.0 rate
        pet->SetSpeedRate(MOVE_FLIGHT, 1.0f); // using 1.0 rate

        // Will set UNIT_FIELD_BOUNDINGRADIUS, UNIT_FIELD_COMBATREACH and UNIT_FIELD_DISPLAYSCALE
        pet->SetObjectScale(pet->GetNativeObjectScale());
        
        pet->SetSheath(SHEATH_STATE_MELEE);
        pet->ReplaceAllNpcFlags(UNIT_NPC_FLAG_NONE);
        pet->ReplaceAllNpcFlags2(UNIT_NPC_FLAG_2_NONE);

        // Force regen flag for player pets, just like we do for players themselves
        pet->SetUnitFlag2(UNIT_FLAG2_REGENERATE_POWER);

        // calculate proper level
        uint8 level = owner->GetLevel();

        // prepare visual effect for levelup
        pet->SetLevel(level);        

        pet->SetClass(CLASS_WARRIOR);
        pet->SetGender(GENDER_NONE);
        pet->SetPowerType(POWER_FOCUS);
        pet->SetMaxPower(POWER_FOCUS, 100);
        pet->SetSheath(SHEATH_STATE_MELEE);
        pet->SetMaxHealth(owner->GetMaxHealth() * 0.75);
        pet->SetFullHealth();
        pet->AIM_Initialize();

        PhasingHandler::InheritPhaseShift(pet, owner);
        pet->SetFaction(owner->GetFaction());

        return true;
    }

}
