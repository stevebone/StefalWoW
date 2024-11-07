/*
 * Copyright (C) 2010 - 2024 Eluna Lua Engine <https://elunaluaengine.github.io/>
 * This program is free software licensed under GPL version 3
 * Please see the included DOCS/LICENSE.md for more information
 */

#ifndef _ELUNA_CREATURE_AI_H
#define _ELUNA_CREATURE_AI_H

#include "LuaEngine.h"

struct ScriptedAI;

struct ElunaCreatureAI : ScriptedAI
{
    // used to delay the spawn hook triggering on AI creation
    bool justSpawned;
    // used to delay movementinform hook (WP hook)
    std::vector< std::pair<uint32, uint32> > movepoints;

    ElunaCreatureAI(Creature* creature) : ScriptedAI(creature), justSpawned(true)
    {
    }
    ~ElunaCreatureAI() { }

    //Called at World update tick
    void UpdateAI(uint32 diff) override
    {
        //Spawns are handled by Creature.cpp - in function Creature::Update() 

        if (!movepoints.empty())
        {
            for (auto& point : movepoints)
            {
                if (!me->GetEluna()->MovementInform(me, point.first, point.second))
                    ScriptedAI::MovementInform(point.first, point.second);
            }
            movepoints.clear();
        }

        if (!me->GetEluna()->UpdateAI(me, diff))
        {
            if (!me->HasUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC))
                ScriptedAI::UpdateAI(diff);
        }
    }

    // Called for reaction when initially engaged - this will always happen _after_ JustEnteredCombat
    // Called at creature aggro either by MoveInLOS or Attack Start
    void JustEngagedWith(Unit* target) override
    {
        if (!me->GetEluna()->EnterCombat(me, target))
            ScriptedAI::JustEngagedWith(target);
    }

    // Called at any Damage from any attacker (before damage apply)
    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType damageType, SpellInfo const* spellInfo) override
    {
        if (!me->GetEluna()->DamageTaken(me, attacker, damage))
        {
            ScriptedAI::DamageTaken(attacker, damage, damageType, spellInfo);
        }
    }

    //Called at creature death
    void JustDied(Unit* killer) override
    {
        if (!me->GetEluna()->JustDied(me, killer))
            ScriptedAI::JustDied(killer);
    }

    //Called at creature killing another unit
    void KilledUnit(Unit* victim) override
    {
        if (!me->GetEluna()->KilledUnit(me, victim))
            ScriptedAI::KilledUnit(victim);
    }

    // Called when the creature summon successfully other creature
    void JustSummoned(Creature* summon) override
    {
        if (!me->GetEluna()->JustSummoned(me, summon))
            ScriptedAI::JustSummoned(summon);
    }

    // Called when a summoned creature is despawned
    void SummonedCreatureDespawn(Creature* summon) override
    {
        if (!me->GetEluna()->SummonedCreatureDespawn(me, summon))
            ScriptedAI::SummonedCreatureDespawn(summon);
    }

    //Called at waypoint reached or PointMovement end
    void MovementInform(uint32 type, uint32 id) override
    {
        // delayed since hook triggers before actually reaching the point
        // and starting new movement would bug
        movepoints.push_back(std::make_pair(type, id));
    }

    // Called before EnterCombat even before the creature is in combat.
    void AttackStart(Unit* target) override
    {
        if (!me->GetEluna()->AttackStart(me, target))
            ScriptedAI::AttackStart(target);
    }

    // Called for reaction at stopping attack at no attackers or targets
    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        if (!me->GetEluna()->EnterEvadeMode(me))
            ScriptedAI::EnterEvadeMode();
    }

    // Called when creature appears in the world (spawn, respawn, grid load etc...)
    void JustAppeared() override
    {
        if (!me->GetEluna()->JustRespawned(me))
            ScriptedAI::JustAppeared();
    }

    // Called at reaching home after evade
    void JustReachedHome() override
    {
        if (!me->GetEluna()->JustReachedHome(me))
            ScriptedAI::JustReachedHome();
    }

    // Called at text emote receive from player
    void ReceiveEmote(Player* player, uint32 emoteId) override
    {
        if (!me->GetEluna()->ReceiveEmote(me, player, emoteId))
            ScriptedAI::ReceiveEmote(player, emoteId);
    }

    // called when the corpse of this creature gets removed
    void CorpseRemoved(uint32& respawnDelay) override
    {
        if (!me->GetEluna()->CorpseRemoved(me, respawnDelay))
            ScriptedAI::CorpseRemoved(respawnDelay);
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (!me->GetEluna()->MoveInLineOfSight(me, who))
            ScriptedAI::MoveInLineOfSight(who);
    }

    // Called when hit by a spell
    void SpellHit(WorldObject* caster, SpellInfo const* spell) override
    {
        if (!me->GetEluna()->SpellHit(me, caster, spell))
            ScriptedAI::SpellHit(caster, spell);
    }

    // Called when spell hits a target
    void SpellHitTarget(WorldObject* target, SpellInfo const* spell) override
    {
        if (!me->GetEluna()->SpellHitTarget(me, target, spell))
            ScriptedAI::SpellHitTarget(target, spell);
    }

    // Called when the creature is summoned successfully by other creature
    void IsSummonedBy(WorldObject* summoner) override
    {
        if (!summoner->ToUnit() || !me->GetEluna()->OnSummoned(me, summoner->ToUnit()))
            ScriptedAI::IsSummonedBy(summoner);
    }

    void SummonedCreatureDies(Creature* summon, Unit* killer) override
    {
        if (!me->GetEluna()->SummonedCreatureDies(me, summon, killer))
            ScriptedAI::SummonedCreatureDies(summon, killer);
    }

    // Called when owner takes damage
    void OwnerAttackedBy(Unit* attacker) override
    {
        if (!me->GetEluna()->OwnerAttackedBy(me, attacker))
            ScriptedAI::OwnerAttackedBy(attacker);
    }

    // Called when owner attacks something
    void OwnerAttacked(Unit* target) override
    {
        if (!me->GetEluna()->OwnerAttacked(me, target))
            ScriptedAI::OwnerAttacked(target);
    }
};

#endif
