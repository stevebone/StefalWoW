// followship_bots_ai_base.h
#pragma once

#include "CreatureAI.h"
#include "Unit.h"

#include "Followship_bots_defines.h"

#include "Followship_bots_regen_handler.h"

class FSB_BaseAI : public ScriptedAI
{
public:
    FSB_BaseAI(Creature* creature) : ScriptedAI(creature),
        botRole(FSB_Roles::FSB_ROLE_NONE),
        botClass(FSB_Class::None),
        botRace(FSB_Race::None),
        botMoveState(FSB_MovementStates::FSB_MOVE_STATE_IDLE),
        botRegenMods(),
        botStats(),

        botHired(false),
        botMounted(false),
        botHasSoulstone(false),
        botManaPotionUsed(false),
        botHealthPotionUsed(false),
        botHasDemon(false),

        botCorpsePos()
    {
    }

    FSB_Roles botRole; 
    FSB_Class botClass;
    FSB_Race botRace;
    FSB_MovementStates botMoveState;
    FSBRegenMods botRegenMods;
    FSBBotStats botStats;

    bool botHired;
    bool botMounted;
    bool botHasSoulstone;
    bool botManaPotionUsed;
    bool botHealthPotionUsed;
    bool botHasDemon;

    Position botCorpsePos;

};
