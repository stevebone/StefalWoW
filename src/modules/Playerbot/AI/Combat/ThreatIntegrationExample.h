/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "ThreatCoordinator.h"
#include "BotThreatManager.h"
#include "InterruptCoordinator.h"
#include "PositionManager.h"
#include "BotAI.h"

namespace Playerbot
{

/**
 * Example integration of the enhanced threat management system
 *
 * This demonstrates how to use the threat management components
 * in a production bot group scenario.
 */
class ThreatIntegrationExample
{
public:
    // === Example 1: Basic 5-bot dungeon group setup ===
    static void SetupDungeonGroup(Group* group)
    {
        // Create coordinators
        auto threatCoordinator = ::std::make_unique<ThreatCoordinator>(group);
        auto interruptCoordinator = ::std::make_unique<InterruptCoordinator>(group);

        // Configure threat thresholds for dungeon content
        threatCoordinator->SetTankThreatThreshold(130.0f);    // Tanks maintain 130% threat
        threatCoordinator->SetDpsThreatThreshold(90.0f);      // DPS stay below 90%
        threatCoordinator->SetHealerThreatThreshold(70.0f);   // Healers stay below 70%

        // Enable automatic features
        threatCoordinator->SetAutoTauntEnabled(true);
        threatCoordinator->SetThreatThrottlingEnabled(true);
        threatCoordinator->SetEmergencyResponseEnabled(true);

        // Set update intervals for responsive gameplay
        threatCoordinator->SetUpdateInterval(100);         // 100ms standard updates
        threatCoordinator->SetEmergencyCheckInterval(50);  // 50ms emergency checks

        // Link systems
        threatCoordinator->SetInterruptCoordinator(interruptCoordinator.get());

        // Register group members
        group->DoForAllMembers([&](Player* member) {
            if (member && member->IsBot())
            {
                BotAI* ai = GetBotAI(member);
                if (ai)
                {
                    threatCoordinator->RegisterBot(member, ai);
                    interruptCoordinator->RegisterBot(member, ai);
                }
            }
        });

        TC_LOG_INFO("playerbots", "Dungeon group threat management initialized");
    }

    // === Example 2: Mythic+ configuration ===
    static void ConfigureForMythicPlus(ThreatCoordinator* coordinator, uint32 mythicLevel)
    {
        // Adjust thresholds based on M+ level
        float scalar = MythicPlusThreat::GetMythicThreatScalar(mythicLevel);

        // Tanks need higher threat margin in M+
        coordinator->SetTankThreatThreshold(130.0f + (mythicLevel * 2.0f));

        // DPS need tighter control
        coordinator->SetDpsThreatThreshold(85.0f - (mythicLevel * 0.5f));

        // Healers generate more threat in M+
        coordinator->SetHealerThreatThreshold(65.0f - (mythicLevel * 0.5f));

        // Faster response times for higher keys
        if (mythicLevel >= 10)
        {
            coordinator->SetUpdateInterval(50);           // 50ms updates
            coordinator->SetEmergencyCheckInterval(25);   // 25ms emergency
        }

        TC_LOG_INFO("playerbots", "Configured for Mythic+ level {}", mythicLevel);
    }

    // === Example 3: Tank swap mechanics ===
    static void HandleTankSwapMechanic(ThreatCoordinator* coordinator, uint32 stackCount)
    {
        // Example: Swap tanks at 3 stacks of a debuff
        if (stackCount >= 3)
        {
            GroupThreatStatus status = coordinator->GetGroupThreatStatus();

            // Initiate swap from primary to off tank
            if (!status.primaryTank.IsEmpty() && !status.offTank.IsEmpty())
            {
                coordinator->InitiateTankSwap(status.primaryTank, status.offTank);

                // Swap the assignments for next rotation
                coordinator->AssignPrimaryTank(status.offTank);
                coordinator->AssignOffTank(status.primaryTank);
            }
        }
    }

    // === Example 4: Combat update loop integration ===
    static void UpdateCombatSystems(ThreatCoordinator* threatCoord,
                                   InterruptCoordinator* interruptCoord,
                                   uint32 diff)
    {
        // Update threat management
        threatCoord->Update(diff);

        // Update interrupt coordination
        interruptCoord->Update(diff);

        // Check for emergency situations
        GroupThreatStatus status = threatCoord->GetGroupThreatStatus();

        if (status.state == ThreatState::CRITICAL)
        {
            TC_LOG_WARN("playerbots", "CRITICAL threat situation detected!");

            // Handle healers under attack
            for (const auto& targetGuid : status.activeTargets)
            {
                Unit* target = GetUnitByGuid(targetGuid);
                if (target && target->GetVictim())
                {
                    Player* victim = target->GetVictim()->ToPlayer();
                    if (victim && IsHealer(victim))
                    {
                        threatCoord->ProtectHealer(victim->GetGUID(), target);
                    }
                }
            }
        }

        // Performance monitoring
        static uint32 updateCount = 0;
        if (++updateCount % 100 == 0) // Every 100 updates
        {
            auto metrics = threatCoord->GetMetrics();
            TC_LOG_DEBUG("playerbots", "Threat Performance - Avg update: {} μs, Tank control: {:.1f}%",
                        metrics.averageUpdateTime.count(),
                        metrics.tankControlRate * 100);
        }
    }

    // === Example 5: Role-specific threat handling ===
    static void HandleBotThreatByRole(Player* bot, BotThreatManager* threatMgr)
    {
        if (!bot || !threatMgr)
            return;

        ThreatRole role = threatMgr->GetBotRole();

        switch (role)
        {
            case ThreatRole::TANK:
            {
                // Tank: Maintain high threat on all targets
                auto targets = threatMgr->GetAllThreatTargets();
                for (Unit* target : targets)
                {
                    float threatPercent = threatMgr->GetThreatPercent(target);
                    if (threatPercent < 110.0f) // Lost aggro
                    {
                        // Use taunt ability
                        uint32 tauntSpell = GetTauntSpell(bot->GetClass());
                        if (tauntSpell && bot->IsSpellReady(tauntSpell))
                        {
                            bot->CastSpell(tauntSpell, false, target);
                            threatMgr->OnTauntUsed(target);
                        }
                    }
                }
                break;
            }

            case ThreatRole::DPS:
            {
                // DPS: Monitor threat and reduce if necessary
                auto primaryTarget = threatMgr->GetPrimaryThreatTarget();
                if (primaryTarget && primaryTarget->info.threatPercent > 85.0f)
                {
                    // Use threat reduction
                    if (bot->GetClass() == CLASS_ROGUE && bot->IsSpellReady(ThreatSpells::FEINT))
                    {
                        bot->CastSpell(ThreatSpells::FEINT, false, bot);
                        threatMgr->ModifyThreat(primaryTarget->target, 0.5f);
                    }
                    else if (bot->GetClass() == CLASS_HUNTER && bot->IsSpellReady(ThreatSpells::FEIGN_DEATH))
                    {
                        bot->CastSpell(ThreatSpells::FEIGN_DEATH, false, bot);
                        threatMgr->ClearAllThreat();
                    }
                }
                break;
            }

            case ThreatRole::HEALER:
            {
                // Healer: Use Fade/threat reduction when targeted
                auto threats = threatMgr->GetThreatTargetsByPriority(ThreatPriority::HIGH_PRIORITY);
                for (Unit* threat : threats)
                {
                    if (threat->GetVictim() == bot)
                    {
                        // Emergency threat drop
                        if (bot->GetClass() == CLASS_PRIEST && bot->IsSpellReady(ThreatSpells::FADE))
                        {
                            bot->CastSpell(ThreatSpells::FADE, false, bot);
                            threatMgr->ModifyThreat(threat, 0.1f);
                        }
                    }
                }
                break;
            }

            default:
                break;
        }
    }

    // === Example 6: Affix handling ===
    static void HandleMythicPlusAffixes(ThreatCoordinator* coordinator, uint32 activeAffixes)
    {
        // Skittish - Tanks generate less threat
        if (activeAffixes & AFFIX_SKITTISH)
        {
            coordinator->HandleSkittishAffix();
            coordinator->SetTankThreatThreshold(150.0f); // Need higher margin
        }

        // Raging - Enemies enrage at low health
        if (activeAffixes & AFFIX_RAGING)
        {
            coordinator->HandleRagingAffix();
            // Prioritize low health enemies for threat control
        }

        // Bolstering - Enemies buff on death
        if (activeAffixes & AFFIX_BOLSTERING)
        {
            coordinator->HandleBolsteringAffix();
            // Ensure even threat distribution
        }
    }

    // === Example 7: Performance optimization ===
    static void OptimizeForLargeGroups(ThreatCoordinator* coordinator, uint32 botCount)
    {
        if (botCount > 10)
        {
            // Reduce update frequency for large groups
            coordinator->SetUpdateInterval(200);          // 200ms updates
            coordinator->SetEmergencyCheckInterval(100);  // 100ms emergency

            // Disable non-critical features
            coordinator->SetThreatThrottlingEnabled(false);
        }
        else if (botCount > 20)
        {
            // Further optimizations for raid scenarios
            coordinator->SetUpdateInterval(500);          // 500ms updates
            coordinator->SetEmergencyCheckInterval(250);  // 250ms emergency
        }
    }

private:
    // Helper methods
    static BotAI* GetBotAI(Player* bot)
    {
        // Implementation would retrieve the bot's AI instance
        return nullptr;
    }

    static Unit* GetUnitByGuid(ObjectGuid guid)
    {
        return ObjectAccessor::GetUnit(*ObjectAccessor::FindPlayer(guid), guid);
    }

    static bool IsHealer(Player* player)
    {
        if (!player)
            return false;

        // Simplified check - in production, check actual spec
        uint8 classId = player->GetClass();
        return classId == CLASS_PRIEST || classId == CLASS_SHAMAN ||
               classId == CLASS_DRUID || classId == CLASS_MONK ||
               classId == CLASS_PALADIN || classId == CLASS_EVOKER;
    }

    static uint32 GetTauntSpell(Classes playerClass)
    {
        switch (playerClass)
        {
            case CLASS_WARRIOR: return ThreatSpells::TAUNT;
            case CLASS_PALADIN: return ThreatSpells::HAND_OF_RECKONING;
            case CLASS_DEATH_KNIGHT: return ThreatSpells::DARK_COMMAND;
            case CLASS_DEMON_HUNTER: return ThreatSpells::TORMENT;
            case CLASS_MONK: return ThreatSpells::PROVOKE;
            case CLASS_DRUID: return ThreatSpells::GROWL;
            default: return 0;
        }
    }

    // M+ Affix IDs (examples)
    static constexpr uint32 AFFIX_SKITTISH = 2;
    static constexpr uint32 AFFIX_RAGING = 6;
    static constexpr uint32 AFFIX_BOLSTERING = 7;
};

/**
 * Production usage example in BotAI update
 */
class BotCombatAI : public BotAI
{
public:
    void UpdateCombat(uint32 diff) override
    {
        if (!_threatCoordinator || !_interruptCoordinator)
            return;

        // Update threat and interrupt systems
        ThreatIntegrationExample::UpdateCombatSystems(
            _threatCoordinator.get(),
            _interruptCoordinator.get(),
            diff
        );

        // Handle individual bot threat
        if (_threatManager)
        {
            _threatManager->UpdateThreat(diff);
            ThreatIntegrationExample::HandleBotThreatByRole(_bot, _threatManager.get());
        }

        // Log performance periodically
        if (++_perfCounter % 1000 == 0)
        {
            TC_LOG_DEBUG("playerbots", "{}", _threatCoordinator->GetThreatReport());
        }
    }

private:
    ::std::unique_ptr<ThreatCoordinator> _threatCoordinator;
    ::std::unique_ptr<InterruptCoordinator> _interruptCoordinator;
    ::std::unique_ptr<BotThreatManager> _threatManager;
    uint32 _perfCounter = 0;
};

} // namespace Playerbot