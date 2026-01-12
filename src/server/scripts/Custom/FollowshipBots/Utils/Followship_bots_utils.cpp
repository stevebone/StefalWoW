#include <algorithm>

#include "CreatureAI.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "ThreatManager.h"

#include "followship_bots.h"
#include "followship_bots_utils.h"

namespace FSBUtils
{
    float GetRandomLeftAngle()
    {
        // Base left is +90 degrees (M_PI/2)
        // Add a random small offset for natural variation
        return (M_PI / 2.0f) + frand(-SIDE_OFFSET_MAX, SIDE_OFFSET_MAX);
    }

    float GetRandomRightAngle()
    {
        // Base right is -90 degrees (-M_PI/2)
        // Add a random small offset for natural variation
        return (-M_PI / 2.0f) + frand(-SIDE_OFFSET_MAX, SIDE_OFFSET_MAX);
    }

    FSB_Roles GetRole(Creature* unit)
    {
        if (!unit || !unit->IsBot())
            return FSB_Roles::FSB_ROLE_NONE;

        
            if (FSB_BaseAI* ai = dynamic_cast<FSB_BaseAI*>(unit->AI()))
                return ai->roleState;
        

        return FSB_Roles::FSB_ROLE_NONE;
    }

    void SetRole(Creature* unit, FSB_Roles role)
    {
        if (!unit || !unit->IsBot())
            return;


            if (FSB_BaseAI* ai = dynamic_cast<FSB_BaseAI*>(unit->AI()))
                ai->roleState = role;
        
    }
  
}

namespace FSBUtilsMovement
{
    bool FSBUtilsMovement::EnsureInRange(Creature* me, Unit* target, float requiredRange)
    {
        if (!me || !target || !target->IsAlive())
            return false;

        float dist = me->GetDistance(target);

        // Already in range
        if (dist <= requiredRange)
            return false;

        MotionMaster* mm = me->GetMotionMaster();
        if (!mm)
            return false;

        // Avoid restarting the same chase every tick
        if (mm->GetCurrentMovementGeneratorType() == CHASE_MOTION_TYPE)
            return true;

        mm->Clear();
        mm->MoveChase(target, requiredRange);

        TC_LOG_DEBUG("scripts.ai.fsb",
            "FSB EnsureInRange: chasing {} to {:.1f} yards",
            target->GetName(), requiredRange);

        return true;
    }

    bool FSBUtilsMovement::EnsureLOS(Unit* me, Unit* target)
    {
        if (!me || !target)
            return false;

        if (me->IsWithinLOSInMap(target))
            return false; // LOS OK ? no movement needed

        MotionMaster* mm = me->GetMotionMaster();
        if (!mm)
            return false;

        // Move directly toward target to resolve LOS
        mm->MoveChase(target);

        return true; // movement started
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
