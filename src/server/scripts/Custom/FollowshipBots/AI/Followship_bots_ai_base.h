// followship_bots_ai_base.h
#pragma once

#include "CreatureAI.h"
#include "EventMap.h"
#include "Unit.h"

#include "Followship_bots_defines.h"

#include "Followship_bots_regen_handler.h"
#include "Followship_bots_spells_handler.h"

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

        botGlobalCooldown(0),
        botBuffsTimer(0),
        botSelfBuffsTimer(0),

        botCorpsePos(),

        botResurrectTargetGuid(ObjectGuid::Empty)
    {
    }

    FSB_Roles botRole; 
    FSB_Class botClass;
    FSB_Race botRace;
    FSB_MovementStates botMoveState;
    FSBRegenMods botRegenMods;
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

    uint32 botGlobalCooldown;
    uint32 botBuffsTimer;
    uint32 botSelfBuffsTimer;

    Position botCorpsePos;

    ObjectGuid botResurrectTargetGuid;

    void ScheduleBotEvent(uint32 eventId, Milliseconds time);
    void ScheduleBotEvent(uint32 eventId, Milliseconds minTime, Milliseconds maxTime);

    Creature* GetBot() { return me; };

    std::vector<Unit*> botLogicalGroup;
    std::vector<FSBSpellRuntime> botRuntimeSpells; // runtime for spells cooldowns

protected:
    EventMap botEvents;
    void HandleBotEvent(FSB_BaseAI* ai, uint32 eventId);

};
