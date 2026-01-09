#include "followship_bots_utils.h"
#include "ThreatManager.h"

namespace FSBUtils
{
    bool IsCombatActive(Unit* me)
    {
        if (!me)
            return false;

        Unit* owner = me->GetOwner();
        Player* player = owner ? owner->ToPlayer() : nullptr;

        return me->IsInCombat() || (player && player->IsInCombat());
    }

    uint8 CountActiveAttackers(Unit* me)
    {
        if (!me)
            return 0;

        uint8 count = 0;

        for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
        {
            Unit* u = ref->GetVictim();
            if (!u || !u->IsAlive())
                continue;

            if (!me->IsInMap(u))
                continue;

            ++count;
        }

        return count;
    }

    uint8 CountAttackersOn(Unit* who)
    {
        if (!who)
            return 0;

        uint8 count = 0;

        for (ThreatReference const* ref : who->GetThreatManager().GetUnsortedThreatList())
        {
            Unit* u = ref->GetVictim();
            if (u && u->IsAlive() && u->GetVictim() == who)
                ++count;
        }

        return count;
    }

    void StopFollow(Unit* me)
    {
        if (!me)
            return;

        me->StopMoving();
        me->GetMotionMaster()->Clear();
    }

    MovementGeneratorType GetMovementType(Unit* me)
    {
        if (!me)
            return IDLE_MOTION_TYPE;

        MotionMaster* mm = me->GetMotionMaster();
        if (!mm)
            return IDLE_MOTION_TYPE;

        return mm->GetCurrentMovementGeneratorType();
    }
}
