#include "Followship_bots_defines.h"

namespace FSBMovement
{
    FSB_MovementStates GetBotMoveState(Creature* bot);

    void StopFollow(Creature* bot);
    void ResumeFollow(Creature* bot, float followDistance, float followAngle);

    bool EnsureInRange(Creature* bot, Unit* target);
    bool EnsureInRange(Creature* bot, Unit* target, float range);

    MovementGeneratorType GetMovementType(Unit* me);

    void BotSetMountedState(Creature* bot, bool& botMounted);

    void BotHandleReturnMovement(Creature* bot);
}
