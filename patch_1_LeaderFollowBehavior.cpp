// PATCH 1: LeaderFollowBehavior.cpp Movement Deduplication Fix
// Location: Line 1210-1219 in StartMovement() method
//
// REPLACE THIS CODE:
    // Log the movement command for debugging
    TC_LOG_ERROR("module.playerbot", "🎯 StartMovement: Bot {} moving to ({:.2f},{:.2f},{:.2f})",
                 bot->GetName(), destination.GetPositionX(), destination.GetPositionY(), destination.GetPositionZ());

    // Use motion master for movement
    // Point ID 0 is used for follow movements
    motionMaster->MovePoint(0, destination);

    TC_LOG_ERROR("module.playerbot", "✓ StartMovement: Movement command sent for Bot {}", bot->GetName());
    return true;

// WITH THIS CODE:
    // FIX: Check if bot is already moving to this exact destination
    // Prevents re-issuing movement every frame which cancels the previous movement
    if (motionMaster->GetMotionSlotType(MOTION_SLOT_ACTIVE) == POINT_MOTION_TYPE)
    {
        // Bot is already moving via MovePoint - check if it's the same destination
        float distToDestination = bot->GetExactDist2d(destination.GetPositionX(), destination.GetPositionY());
        if (distToDestination > 0.5f) // Different destination
        {
            TC_LOG_ERROR("module.playerbot", "🔄 StartMovement: Bot {} changing destination to ({:.2f},{:.2f},{:.2f})",
                         bot->GetName(), destination.GetPositionX(), destination.GetPositionY(), destination.GetPositionZ());
            motionMaster->MovePoint(0, destination);
        }
        else
        {
            // Already moving to same destination - don't re-issue command
            TC_LOG_DEBUG("module.playerbot", "⏭️ StartMovement: Bot {} already moving to destination, skipping", bot->GetName());
            return true;
        }
    }
    else
    {
        // Not currently in point movement - issue new command
        TC_LOG_ERROR("module.playerbot", "🎯 StartMovement: Bot {} moving to ({:.2f},{:.2f},{:.2f})",
                     bot->GetName(), destination.GetPositionX(), destination.GetPositionY(), destination.GetPositionZ());
        motionMaster->MovePoint(0, destination);
    }

    TC_LOG_ERROR("module.playerbot", "✓ StartMovement: Movement command sent for Bot {}", bot->GetName());
    return true;
