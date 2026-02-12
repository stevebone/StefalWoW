#include "Followship_bots_defines.h"

namespace FSBMovement
{
    FSB_MovementStates GetBotMoveState(Creature* bot);
    void SetBotMoveState(Creature* bot, FSB_MovementStates moveState);

    void StopFollow(Creature* bot);
    void ResumeFollow(Creature* bot, float followDistance, float followAngle);

    MovementGeneratorType GetMovementType(Unit* me);

    void BotSetMountedState(Creature* bot, bool& botMounted);
}
