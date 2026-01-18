// followship_bots_ai_base.h
#pragma once

#include "CreatureAI.h"
#include "Unit.h"

#include "Followship_bots_defines.h"
#include "Followship_bots_utils_stats.h"

class FSB_BaseAI : public ScriptedAI
{
public:
    FSB_BaseAI(Creature* creature) : ScriptedAI(creature),
        roleState(FSB_Roles::FSB_ROLE_NONE),
        botClass(FSB_Class::None)
        //moveState(FSB_MOVE_STATE_IDLE)
    {
    }

    FSB_Roles roleState; // each bot sets this
    FSB_Class botClass;
    //uint16 moveState;
};
