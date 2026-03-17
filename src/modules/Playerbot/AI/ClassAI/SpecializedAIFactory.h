/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Enterprise-Grade Specialized AI Factory Header
 *
 * Factory for creating specialization-specific AI instances with full
 * Phase 5 decision system integration (ActionPriorityQueue + BehaviorTree).
 */

#pragma once

#include "BotAI.h"
#include <memory>

class Player;

namespace Playerbot
{

/**
 * SpecializedAIFactory
 *
 * Enterprise-grade factory for creating bot AI instances based on
 * class and specialization. Routes to appropriate Refactored class
 * with full Phase 5 decision systems initialized.
 *
 * Supports all 40 specializations across 13 classes:
 * - Warrior (3): Arms, Fury, Protection
 * - Paladin (3): Holy, Protection, Retribution
 * - Hunter (3): Beast Mastery, Marksmanship, Survival
 * - Rogue (3): Assassination, Outlaw, Subtlety
 * - Priest (3): Discipline, Holy, Shadow
 * - Death Knight (3): Blood, Frost, Unholy
 * - Shaman (3): Elemental, Enhancement, Restoration
 * - Mage (3): Arcane, Fire, Frost
 * - Warlock (3): Affliction, Demonology, Destruction
 * - Monk (3): Brewmaster, Mistweaver, Windwalker
 * - Druid (4): Balance, Feral, Guardian, Restoration
 * - Demon Hunter (2): Havoc, Vengeance
 * - Evoker (3): Devastation, Preservation, Augmentation
 */
class TC_GAME_API SpecializedAIFactory
{
public:
    /**
     * Create specialized AI based on bot's class and active specialization
     *
     * @param bot Player bot to create AI for
     * @return Unique pointer to specialized AI instance, nullptr on failure
     */
    static ::std::unique_ptr<BotAI> CreateSpecializedAI(Player* bot);

private:
    // Class-specific factory methods
    static ::std::unique_ptr<BotAI> CreateWarriorAI(Player* bot, uint8 specId);
    static ::std::unique_ptr<BotAI> CreatePaladinAI(Player* bot, uint8 specId);
    static ::std::unique_ptr<BotAI> CreateHunterAI(Player* bot, uint8 specId);
    static ::std::unique_ptr<BotAI> CreateRogueAI(Player* bot, uint8 specId);
    static ::std::unique_ptr<BotAI> CreatePriestAI(Player* bot, uint8 specId);
    static ::std::unique_ptr<BotAI> CreateDeathKnightAI(Player* bot, uint8 specId);
    static ::std::unique_ptr<BotAI> CreateShamanAI(Player* bot, uint8 specId);
    static ::std::unique_ptr<BotAI> CreateMageAI(Player* bot, uint8 specId);
    static ::std::unique_ptr<BotAI> CreateWarlockAI(Player* bot, uint8 specId);
    static ::std::unique_ptr<BotAI> CreateMonkAI(Player* bot, uint8 specId);
    static ::std::unique_ptr<BotAI> CreateDruidAI(Player* bot, uint8 specId);
    static ::std::unique_ptr<BotAI> CreateDemonHunterAI(Player* bot, uint8 specId);
    static ::std::unique_ptr<BotAI> CreateEvokerAI(Player* bot, uint8 specId);
};

} // namespace Playerbot
