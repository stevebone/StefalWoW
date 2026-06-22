/*
 * This file is part of the Stefal WoW Project.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Creature.h"
#include "Followship_bots_defines.h"

class FSB_BaseAI;

namespace FSBLlamaPrompts
{
    enum class FSB_AcknowledgeContext
    {
        FollowDistanceClose,
        FollowDistanceNormal,
        FollowDistanceWide,
        FollowAngleFront,
        FollowAngleBehind,
        FollowAngleRight,
        FollowAngleLeft,
        StayCommand,
        FollowCommand
    };

    void DispatchBotInfoRequest(Creature* bot);
    void DispatchBotRoleAcknowledge(Creature* bot);
    void DispatchBotAcknowledge(Creature* bot, FSB_AcknowledgeContext context);
    void DispatchBotDismissed(Creature* bot);
    void DispatchBotHired(Creature* bot, uint32 durationHours);
    void DispatchBotRecovery(Creature* bot, uint32 spellId);
    void DispatchBotBuff(Creature* bot, ObjectGuid targetGuid, uint32 spellId);
    void DispatchBotPotion(Creature* bot, uint32 spellId, bool isManaPotion);
    void DispatchBotHeal(Creature* bot, ObjectGuid targetGuid, uint32 spellId);
    void DispatchBotCombatSpell(Creature* bot, ObjectGuid targetGuid, uint32 spellId);
    void DispatchBotTargetKilled(Creature* bot, ObjectGuid victimGuid);
    void DispatchBotDeath(Creature* bot, ObjectGuid killerGuid);
    void DispatchBotRevived(Creature* bot);
    void DispatchBotRevivedTarget(Creature* healer, ObjectGuid targetGuid);
    void DispatchBotRevivedSelf(Creature* bot);

    enum class FSB_DistressContext
    {
        OutOfMana,
        LowHealth,
        CriticalHealth
    };

    void DispatchBotDistress(Creature* bot, FSB_DistressContext context);
    void DispatchBotWhisperAFK(Creature* bot);
    void DispatchBotTalk(Creature* bot);
    void DispatchBotMemberDied(Creature* bot, ObjectGuid deadUnitGuid);
    void DispatchBotCooking(Creature* bot);
}
