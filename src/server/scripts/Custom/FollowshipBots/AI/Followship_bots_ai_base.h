/*
 * This file is part of the Stefal WoW Project.
 * It is designed to work exclusively with the TrinityCore framework.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * This code is provided for personal and educational use within the
 * Stefal WoW Project. It is not intended for commercial distribution,
 * resale, or any form of monetization.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "CreatureAI.h"
#include "EventMap.h"
#include "ScriptedCreature.h"
#include "Unit.h"

#include "Followship_bots_defines.h"

#include "Followship_bots_regen_handler.h"
#include "Followship_bots_spells_handler.h"

struct FSB_ClassStats;

class FSB_BaseAI : public ScriptedAI
{
public:
    FSB_BaseAI(Creature* creature) : ScriptedAI(creature),
        botRole(FSB_Roles::FSB_ROLE_NONE),
        botClass(FSB_Class::None),
        botRace(FSB_Race::None),
        botGender(GENDER_NONE),
        botMoveState(FSB_MovementStates::FSB_MOVE_STATE_IDLE),
        botChatterType(FSB_ChatterType::None),
        botRegenMods(),
        botClassStats(),
        botStats(),

        botFollowDistance(0.f),
        botFollowAngle(0.f),

        botHired(false),
        botMounted(false),
        botHasSoulstone(false),
        botManaPotionUsed(false),
        botHealthPotionUsed(false),
        botHasDemon(false),
        botCastedCombatBuffs(false),
        botSayMemberDead(false),
        botMeleeMode(false),
        botHasCompanion(false),
        botSitsByFire(false),
        botDoingRandomEvent(false),
        botAnnouncedLowMana(false),
        botAnnouncedLowHealth(false),
        botAnnouncedVeryLowHealth(false),

        botGlobalCooldown(0),
        botBuffsTimer(0),
        botSelfBuffsTimer(0),
        botRandomEventTimer(0),
        botOutOfCombatTimer(0),
        botOwnerNotMovingTimer(getMSTime()),
        botPetSpellsTimer(0),
        botChatChannelsTimer(0),

        botCorpsePos(),

        botResurrectTargetGuid(ObjectGuid::Empty)
    {
    }

    FSB_Roles botRole; 
    FSB_Class botClass;
    FSB_Race botRace;
    Gender botGender;
    FSB_MovementStates botMoveState;
    FSB_ChatterType botChatterType;
    FSBRegenMods botRegenMods;
    FSB_ClassStats const* botClassStats = nullptr;
    FSBBotStats botStats;

    float botFollowDistance;
    float botFollowAngle;

    bool botHired;
    bool botMounted;
    bool botHasSoulstone;
    bool botManaPotionUsed;
    bool botHealthPotionUsed;
    bool botHasDemon;
    bool botCastedCombatBuffs;
    bool botSayMemberDead;
    bool botMeleeMode;
    bool botHasCompanion;
    bool botSitsByFire;
    bool botDoingRandomEvent;
    bool botAnnouncedLowMana;
    bool botAnnouncedLowHealth;
    bool botAnnouncedVeryLowHealth;

    uint32 botGlobalCooldown;
    uint32 botBuffsTimer;
    uint32 botSelfBuffsTimer;
    uint32 botRandomEventTimer;
    uint32 botOutOfCombatTimer;
    uint32 botOwnerNotMovingTimer;
    uint32 botPetSpellsTimer;
    int64 botChatChannelsTimer;

    Position botCorpsePos;

    ObjectGuid botResurrectTargetGuid;

    void ScheduleBotEvent(uint32 eventId, Milliseconds time);
    void ScheduleBotEvent(uint32 eventId, Milliseconds minTime, Milliseconds maxTime);
    void ScheduleBotEvent(uint32 eventId, Milliseconds minTime, Milliseconds maxTime, FSB_ReplyType replyType, const std::string& chatterString, Unit* target);

    Creature* GetBot() { return me; };

    std::vector<Unit*> botLogicalGroup;
    std::vector<FSBSpellRuntime> botRuntimeSpells; // runtime for spells cooldowns

    struct FSBEventPayload
    {
        FSB_ReplyType replyType;
        std::string chatterString;
        Unit* unit;

        FSBEventPayload()
            : replyType(FSB_ReplyType::None),
            chatterString(""),
            unit(nullptr)
        {}

        FSBEventPayload(FSB_ReplyType r, std::string c, Unit* u)
            : replyType(r),
            chatterString(std::move(c)),
            unit(u)
        {}
    };

    std::unordered_map<uint32, FSBEventPayload> eventPayloads;

protected:
    EventMap botEvents;
    void HandleBotEvent(FSB_BaseAI* ai, uint32 eventId);
    void HandleBotEvent(FSB_BaseAI* ai, uint32 eventId, FSB_ReplyType replyType, std::string chatterString, Unit* target);

};
