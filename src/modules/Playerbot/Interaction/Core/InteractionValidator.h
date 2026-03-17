/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_BOT_INTERACTION_VALIDATOR_H
#define TRINITYCORE_BOT_INTERACTION_VALIDATOR_H

#include "InteractionTypes.h"
#include "Threading/LockHierarchy.h"
#include "Define.h"
#include "SharedDefines.h"
#include <unordered_map>
#include <shared_mutex>

// Forward declarations (global namespace)
class Player;
class Creature;
class GameObject;
class WorldObject;
class Item;
class SpellInfo;

namespace Playerbot
{
    /**
     * @class InteractionValidator
     * @brief Validates bot-NPC interaction requirements and conditions
     *
     * Checks:
     * - Range and positioning requirements
     * - Faction and reputation requirements
     * - Level and skill requirements
     * - Money and inventory space
     * - Combat and status restrictions
     * - Quest prerequisites
     * - Item requirements
     */
    class InteractionValidator
    {
    public:
        InteractionValidator();
        ~InteractionValidator();

        /**
         * @brief Check if bot can interact with target
         * @param bot The bot player
         * @param target The target NPC or object
         * @param type The interaction type
         * @return True if all requirements are met
         */
        bool CanInteract(::Player* bot, ::WorldObject* target, InteractionType type) const;

        /**
         * @brief Check if bot is in valid range for interaction
         * @param bot The bot player
         * @param target The target object
         * @param maxRange Optional custom max range
         * @return True if in valid range
         */
        bool CheckRange(::Player* bot, ::WorldObject* target, float maxRange = 0.0f) const;

        /**
         * @brief Check if bot has correct faction for NPC
         * @param bot The bot player
         * @param creature The NPC
         * @return True if faction allows interaction
         */
        bool CheckFaction(::Player* bot, ::Creature* creature) const;

        /**
         * @brief Check if bot meets level requirements
         * @param bot The bot player
         * @param minLevel Minimum required level
         * @param maxLevel Maximum allowed level (0 = no max)
         * @return True if level is appropriate
         */
        bool CheckLevel(::Player* bot, uint32 minLevel, uint32 maxLevel = 0) const;

        /**
         * @brief Check if bot has required reputation
         * @param bot The bot player
         * @param factionId The faction to check
         * @param minRank Minimum reputation rank required
         * @return True if reputation is sufficient
         */
        bool CheckReputation(::Player* bot, uint32 factionId, ReputationRank minRank) const;

        /**
         * @brief Check if bot has enough money
         * @param bot The bot player
         * @param amount Required amount in copper
         * @return True if bot can afford
         */
        bool CheckMoney(::Player* bot, uint32 amount) const;

        /**
         * @brief Check if bot has inventory space
         * @param bot The bot player
         * @param slotsNeeded Number of free slots required
         * @return True if enough space
         */
        bool CheckInventorySpace(::Player* bot, uint32 slotsNeeded) const;

        /**
         * @brief Check if bot's combat state allows interaction
         * @param bot The bot player
         * @param allowInCombat Whether interaction is allowed in combat
         * @return True if combat state is valid
         */
        bool CheckCombatState(::Player* bot, bool allowInCombat = false) const;

        /**
         * @brief Check if bot is alive (or dead for spirit healer)
         * @param bot The bot player
         * @param requireAlive True if bot must be alive
         * @return True if alive state matches requirement
         */
        bool CheckAliveState(::Player* bot, bool requireAlive = true) const;

        /**
         * @brief Check if bot has completed required quest
         * @param bot The bot player
         * @param questId The quest to check
         * @return True if quest is completed
         */
        bool CheckQuestStatus(::Player* bot, uint32 questId) const;

        /**
         * @brief Check if bot has required item
         * @param bot The bot player
         * @param itemId The item to check for
         * @param count Required count
         * @return True if bot has enough of the item
         */
        bool CheckItemRequirement(::Player* bot, uint32 itemId, uint32 count = 1) const;

        /**
         * @brief Check if bot knows required spell
         * @param bot The bot player
         * @param spellId The spell to check
         * @return True if bot knows the spell
         */
        bool CheckSpellKnown(::Player* bot, uint32 spellId) const;

        /**
         * @brief Check if bot has required skill level
         * @param bot The bot player
         * @param skillId The skill to check
         * @param minValue Minimum skill value required
         * @return True if skill level is sufficient
         */
        bool CheckSkillLevel(::Player* bot, uint32 skillId, uint32 minValue) const;

        // Vendor-specific validations

        /**
         * @brief Check if vendor has items bot needs
         * @param bot The bot player
         * @param vendor The vendor NPC
         * @return True if vendor has useful items
         */
        bool ValidateVendor(::Player* bot, ::Creature* vendor) const;

        /**
         * @brief Check if item is worth buying for bot
         * @param bot The bot player
         * @param itemId The item to evaluate
         * @return True if item is useful
         */
        bool ShouldBuyItem(::Player* bot, uint32 itemId) const;

        /**
         * @brief Check if bot should sell item
         * @param bot The bot player
         * @param item The item to evaluate
         * @return True if item should be sold
         */
        bool ShouldSellItem(::Player* bot, ::Item* item) const;

        /**
         * @brief Check if bot needs repairs
         * @param bot The bot player
         * @param threshold Durability percentage threshold
         * @return True if repairs needed
         */
        bool NeedsRepair(::Player* bot, float threshold = 30.0f) const;

        // Trainer-specific validations

        /**
         * @brief Check if trainer can teach bot
         * @param bot The bot player
         * @param trainer The trainer NPC
         * @return True if trainer has spells for bot
         */
        bool ValidateTrainer(::Player* bot, ::Creature* trainer) const;

        /**
         * @brief Check if bot should learn spell
         * @param bot The bot player
         * @param spellInfo The spell to evaluate
         * @return True if spell is worth learning
         */
        bool ShouldLearnSpell(::Player* bot, ::SpellInfo const* spellInfo) const;

        /**
         * @brief Check if bot can learn spell
         * @param bot The bot player
         * @param spellId The spell to check
         * @return True if bot meets all requirements
         */
        bool CanLearnSpell(::Player* bot, uint32 spellId) const;

        // Flight Master validations

        /**
         * @brief Check if bot can use flight path
         * @param bot The bot player
         * @param nodeId The flight node
         * @return True if node is accessible
         */
        bool CanUseFlight(::Player* bot, uint32 nodeId) const;

        /**
         * @brief Check if flight path is discovered
         * @param bot The bot player
         * @param nodeId The flight node
         * @return True if node is known
         */
        bool IsFlightNodeDiscovered(::Player* bot, uint32 nodeId) const;

        // Bank validations

        /**
         * @brief Check if bot can access bank
         * @param bot The bot player
         * @return True if bank access is allowed
         */
        bool CanAccessBank(::Player* bot) const;

        /**
         * @brief Check if bot should deposit item
         * @param bot The bot player
         * @param item The item to evaluate
         * @return True if item should be banked
         */
        bool ShouldBankItem(::Player* bot, ::Item* item) const;

        // Mailbox validations

        /**
         * @brief Check if bot can use mailbox
         * @param bot The bot player
         * @return True if mailbox access is allowed
         */
        bool CanUseMail(::Player* bot) const;

        /**
         * @brief Check if mail should be taken
         * @param bot The bot player
         * @param mailId The mail to evaluate
         * @return True if mail should be collected
         */
        bool ShouldTakeMail(::Player* bot, uint32 mailId) const;

        // Complex validation combinations

        /**
         * @brief Validate complete interaction requirements
         * @param bot The bot player
         * @param requirements The requirements to check
         * @return True if all requirements are met
         */
        bool ValidateRequirements(::Player* bot, const InteractionRequirement& requirements) const;

        /**
         * @brief Get missing requirements for interaction
         * @param bot The bot player
         * @param target The target object
         * @param type The interaction type
         * @return List of unmet requirements
         */
        ::std::vector<::std::string> GetMissingRequirements(::Player* bot, ::WorldObject* target,
                                                        InteractionType type) const;

        /**
         * @brief Calculate priority of interaction
         * @param bot The bot player
         * @param type The interaction type
         * @return Priority score (higher = more important)
         */
        int32 GetInteractionPriority(::Player* bot, InteractionType type) const;

        // Configuration

        /**
         * @brief Set validation strictness level
         * @param strict True for strict validation, false for lenient
         */
        void SetStrictMode(bool strict) { m_strictMode = strict; }

        /**
         * @brief Get validation metrics
         * @return Validation statistics
         */
        struct ValidationMetrics
        {
            uint32 totalValidations = 0;
            uint32 passedValidations = 0;
            uint32 failedValidations = 0;
            ::std::unordered_map<InteractionType, uint32> failuresByType;
        };
        ValidationMetrics GetMetrics() const;

        /**
         * @brief Reset validation metrics
         */
        void ResetMetrics();

    private:
        /**
         * @brief Check vendor-specific requirements
         * @param bot The bot player
         * @param vendor The vendor NPC
         * @return True if requirements are met
         */
        bool CheckVendorRequirements(::Player* bot, ::Creature* vendor) const;

        /**
         * @brief Check trainer-specific requirements
         * @param bot The bot player
         * @param trainer The trainer NPC
         * @return True if requirements are met
         */
        bool CheckTrainerRequirements(::Player* bot, ::Creature* trainer) const;

        /**
         * @brief Check if interaction is on cooldown
         * @param bot The bot player
         * @param type The interaction type
         * @return True if still on cooldown
         */
        bool IsOnCooldown(::Player* bot, InteractionType type) const;

        /**
         * @brief Record validation result for metrics
         * @param type The interaction type
         * @param passed Whether validation passed
         */
        void RecordValidation(InteractionType type, bool passed) const;

    private:
        // Thread safety
        mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::ACTION_PRIORITY> m_mutex;

        // Configuration
        bool m_strictMode = false;

        // Cooldown tracking
        mutable ::std::unordered_map<ObjectGuid, ::std::unordered_map<InteractionType,
                                   ::std::chrono::steady_clock::time_point>> m_cooldowns;

        // Interaction cooldown durations (ms)
        ::std::unordered_map<InteractionType, uint32> m_cooldownDurations = {
            {InteractionType::Vendor, 1000},
            {InteractionType::Trainer, 2000},
            {InteractionType::Bank, 1000},
            {InteractionType::Mailbox, 1000},
            {InteractionType::FlightMaster, 3000},
            {InteractionType::Innkeeper, 2000}
        };

        // Validation metrics
        mutable ValidationMetrics m_metrics;

        // Cache for expensive checks
        mutable ::std::unordered_map<ObjectGuid, ::std::chrono::steady_clock::time_point> m_lastValidation;
        mutable ::std::unordered_map<ObjectGuid, bool> m_validationCache;
        const uint32 CACHE_DURATION_MS = 5000;

        // Item evaluation cache
        mutable ::std::unordered_map<uint32, bool> m_usefulItemCache;
        mutable ::std::unordered_map<uint32, bool> m_junkItemCache;

        bool m_initialized = false;
    };
}

#endif // TRINITYCORE_BOT_INTERACTION_VALIDATOR_H