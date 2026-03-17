/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file BotTemplateRepository.h
 * @brief Pre-defined bot templates for fast JIT cloning
 *
 * The BotTemplateRepository maintains a collection of pre-serialized bot
 * templates for each class/spec/role combination. These templates enable
 * rapid bot creation by:
 *
 * 1. Pre-computing gear sets for different item levels
 * 2. Pre-serializing talent builds
 * 3. Pre-configuring action bars
 * 4. Caching racial options per faction
 *
 * Template Creation Flow:
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │  BotTemplateRepository                                                   │
 * ├─────────────────────────────────────────────────────────────────────────┤
 * │                                                                         │
 * │  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐              │
 * │  │ Class:Warrior│    │ Class:Paladin│    │ Class:Hunter │  ...         │
 * │  └──────┬───────┘    └──────┬───────┘    └──────┬───────┘              │
 * │         │                   │                   │                       │
 * │  ┌──────┴──────┐     ┌──────┴──────┐     ┌──────┴──────┐               │
 * │  │Spec:Arms    │     │Spec:Holy    │     │Spec:BM      │               │
 * │  │Spec:Fury    │     │Spec:Prot    │     │Spec:MM      │               │
 * │  │Spec:Prot    │     │Spec:Retri   │     │Spec:Surv    │               │
 * │  └──────┬──────┘     └─────────────┘     └─────────────┘               │
 * │         │                                                               │
 * │  ┌──────┴──────────────────────────────┐                               │
 * │  │ Template Data:                      │                               │
 * │  │ - Serialized gear (per iLvl tier)   │                               │
 * │  │ - Serialized talents                │                               │
 * │  │ - Action bar configuration          │                               │
 * │  │ - Valid races for faction           │                               │
 * │  │ - Pre-calculated stats              │                               │
 * │  └─────────────────────────────────────┘                               │
 * │                                                                         │
 * └─────────────────────────────────────────────────────────────────────────┘
 *
 * Thread Safety:
 * - All public methods are thread-safe
 * - Templates are immutable after initialization
 */

#pragma once

#include "Define.h"
#include "PoolSlotState.h"
#include <array>
#include <atomic>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Playerbot
{

// ============================================================================
// GEAR TEMPLATE
// ============================================================================

/**
 * @brief Template for a single gear slot
 */
struct GearSlotTemplate
{
    uint8 slotId = 0;               ///< Equipment slot (HEAD, CHEST, etc.)
    uint32 itemId = 0;              ///< Item entry ID
    uint32 itemLevel = 0;           ///< Base item level
    uint32 enchantId = 0;           ///< Enchant ID (if any)
    std::array<uint32, 3> gemIds = {};  ///< Gem IDs for sockets
};

/**
 * @brief Complete gear set for a specific item level tier
 */
struct GearSetTemplate
{
    uint32 targetItemLevel = 0;     ///< Target item level for this set
    uint32 actualGearScore = 0;     ///< Calculated gear score
    std::array<GearSlotTemplate, 19> slots;  ///< All equipment slots

    /**
     * @brief Check if gear set is valid (all required slots filled)
     */
    bool IsValid() const
    {
        // Check critical slots are filled
        return slots[0].itemId != 0 &&   // Head
               slots[4].itemId != 0 &&   // Chest
               slots[6].itemId != 0 &&   // Legs
               slots[15].itemId != 0;    // Main hand
    }
};

// ============================================================================
// TALENT TEMPLATE
// ============================================================================

/**
 * @brief Pre-serialized talent configuration
 */
struct TalentTemplate
{
    uint32 specId = 0;              ///< Specialization ID
    std::string specName;           ///< Human-readable spec name
    std::vector<uint32> talentIds;  ///< Selected talent IDs
    std::vector<uint32> pvpTalentIds;  ///< PvP talent selections
    uint32 heroTalentTreeId = 0;    ///< Hero talent tree (if applicable)

    /**
     * @brief Serialize talents to binary blob for fast application
     */
    std::vector<uint8> Serialize() const;

    /**
     * @brief Deserialize talents from binary blob
     */
    static TalentTemplate Deserialize(std::vector<uint8> const& data);
};

// ============================================================================
// ACTION BAR TEMPLATE
// ============================================================================

/**
 * @brief Single action bar button
 */
struct ActionBarButton
{
    uint8 actionBar = 0;            ///< Action bar number (0-5)
    uint8 slot = 0;                 ///< Button slot (0-11)
    uint32 actionType = 0;          ///< SPELL, ITEM, MACRO, etc.
    uint32 actionId = 0;            ///< Spell ID, Item ID, etc.
};

/**
 * @brief Complete action bar configuration
 */
struct ActionBarTemplate
{
    std::vector<ActionBarButton> buttons;

    /**
     * @brief Serialize action bars to binary blob
     */
    std::vector<uint8> Serialize() const;

    /**
     * @brief Deserialize from binary blob
     */
    static ActionBarTemplate Deserialize(std::vector<uint8> const& data);
};

// ============================================================================
// BOT TEMPLATE
// ============================================================================

/**
 * @brief Complete bot template for fast cloning
 *
 * Contains all pre-serialized data needed to create a fully-configured
 * bot character in minimal time.
 */
struct TC_GAME_API BotTemplate
{
    // ========================================================================
    // IDENTITY
    // ========================================================================

    uint32 templateId = 0;          ///< Unique template ID
    std::string templateName;       ///< Human-readable name (e.g., "Warrior_Arms_Tank")

    // ========================================================================
    // CHARACTER DEFINITION
    // ========================================================================

    uint8 playerClass = 0;          ///< WoW class ID
    uint32 specId = 0;              ///< Specialization ID
    BotRole role = BotRole::DPS;    ///< Combat role
    std::string className;          ///< Class name for logging
    std::string specName;           ///< Spec name for logging

    // ========================================================================
    // FACTION/RACE OPTIONS
    // ========================================================================

    /// Valid races for Alliance
    std::vector<uint8> allianceRaces;
    /// Valid races for Horde
    std::vector<uint8> hordeRaces;

    // ========================================================================
    // SERIALIZED DATA
    // ========================================================================

    TalentTemplate talents;
    ActionBarTemplate actionBars;

    /// Gear sets indexed by target item level
    std::unordered_map<uint32, GearSetTemplate> gearSets;

    // ========================================================================
    // CACHED DATA
    // ========================================================================

    /// Pre-serialized talent blob for fast application
    std::vector<uint8> talentBlob;

    /// Pre-serialized action bar blob
    std::vector<uint8> actionBarBlob;

    // ========================================================================
    // STATISTICS
    // ========================================================================

    std::chrono::milliseconds avgCreationTime{0};  ///< Average time to clone
    uint32 usageCount = 0;                         ///< Times this template used

    // ========================================================================
    // METHODS
    // ========================================================================

    /**
     * @brief Get a random race for faction
     * @param faction Target faction
     * @return Race ID
     */
    uint8 GetRandomRace(Faction faction) const;

    /**
     * @brief Get gear set closest to target item level
     * @param targetItemLevel Desired item level
     * @return Gear set (or empty if none available)
     */
    GearSetTemplate const* GetGearSetForItemLevel(uint32 targetItemLevel) const;

    /**
     * @brief Check if template is valid and ready for use
     */
    bool IsValid() const;

    /**
     * @brief Pre-serialize all data for fast application
     */
    void PreSerialize();

    /**
     * @brief Get string representation for logging
     */
    std::string ToString() const;
};

// ============================================================================
// TEMPLATE REPOSITORY
// ============================================================================

/**
 * @brief Repository of bot templates for fast JIT cloning
 *
 * Singleton class managing all bot templates. Templates are created
 * during initialization and remain immutable during runtime.
 */
class TC_GAME_API BotTemplateRepository
{
public:
    /**
     * @brief Get singleton instance
     */
    static BotTemplateRepository* Instance();

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize repository and create default templates
     * @return true if successful
     */
    bool Initialize();

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    /**
     * @brief Load templates from database
     */
    void LoadFromDatabase();

    /**
     * @brief Save templates to database
     */
    void SaveToDatabase();

    // ========================================================================
    // TEMPLATE ACCESS
    // ========================================================================

    /**
     * @brief Get template for class/role combination
     * @param playerClass WoW class ID
     * @param role Combat role
     * @return Template or nullptr if not found
     */
    BotTemplate const* GetTemplate(uint8 playerClass, BotRole role) const;

    /**
     * @brief Get template for class/spec combination
     * @param playerClass WoW class ID
     * @param specId Specialization ID
     * @return Template or nullptr if not found
     */
    BotTemplate const* GetTemplate(uint8 playerClass, uint32 specId) const;

    /**
     * @brief Get template by ID
     * @param templateId Template ID
     * @return Template or nullptr if not found
     */
    BotTemplate const* GetTemplateById(uint32 templateId) const;

    /**
     * @brief Get all templates for a role
     * @param role Combat role
     * @return Vector of matching templates
     */
    std::vector<BotTemplate const*> GetTemplatesForRole(BotRole role) const;

    /**
     * @brief Get all templates for a faction
     * @param faction Alliance or Horde
     * @return Vector of templates valid for faction
     */
    std::vector<BotTemplate const*> GetTemplatesForFaction(Faction faction) const;

    /**
     * @brief Get all templates
     * @return Vector of all templates
     */
    std::vector<BotTemplate const*> GetAllTemplates() const;

    /**
     * @brief Get total template count
     */
    uint32 GetTemplateCount() const;

    // ========================================================================
    // TEMPLATE SELECTION
    // ========================================================================

    /**
     * @brief Select best template for requirements
     * @param role Required role
     * @param faction Required faction
     * @param preferredClass Preferred class (0 for any)
     * @return Best matching template or nullptr
     */
    BotTemplate const* SelectBestTemplate(
        BotRole role,
        Faction faction,
        uint8 preferredClass = 0) const;

    /**
     * @brief Select random template matching criteria
     * @param role Required role
     * @param faction Required faction
     * @return Random matching template or nullptr
     */
    BotTemplate const* SelectRandomTemplate(
        BotRole role,
        Faction faction) const;

    // ========================================================================
    // STATISTICS
    // ========================================================================

    /**
     * @brief Record template usage
     * @param templateId Template ID used
     * @param creationTime Time taken to create bot
     */
    void RecordTemplateUsage(uint32 templateId, std::chrono::milliseconds creationTime);

    /**
     * @brief Print template statistics to log
     */
    void PrintStatistics() const;

private:
    BotTemplateRepository() = default;
    ~BotTemplateRepository() = default;
    BotTemplateRepository(BotTemplateRepository const&) = delete;
    BotTemplateRepository& operator=(BotTemplateRepository const&) = delete;

    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * @brief Create default templates for all class/spec combinations
     */
    void CreateDefaultTemplates();

    /**
     * @brief Create template for specific class/spec
     */
    void CreateClassTemplate(uint8 playerClass, uint32 specId, BotRole role);

    /**
     * @brief Generate gear set for template
     */
    void GenerateGearSet(BotTemplate& tmpl, uint32 itemLevel);

    /**
     * @brief Generate talent configuration
     */
    void GenerateTalents(BotTemplate& tmpl);

    /**
     * @brief Generate action bar configuration
     */
    void GenerateActionBars(BotTemplate& tmpl);

    /**
     * @brief Get valid races for class/faction
     */
    std::vector<uint8> GetValidRaces(uint8 playerClass, Faction faction) const;

    /**
     * @brief Calculate template key for indexing
     */
    static uint64 MakeTemplateKey(uint8 playerClass, uint32 specId);
    static uint64 MakeRoleKey(uint8 playerClass, BotRole role);

    // ========================================================================
    // DATA MEMBERS
    // ========================================================================

    /// Templates indexed by template ID
    std::unordered_map<uint32, std::unique_ptr<BotTemplate>> _templates;

    /// Index by class+spec
    std::unordered_map<uint64, uint32> _classSpecIndex;

    /// Index by class+role
    std::unordered_map<uint64, uint32> _classRoleIndex;

    /// Index by role
    std::unordered_map<BotRole, std::vector<uint32>> _roleIndex;

    /// Next template ID
    std::atomic<uint32> _nextTemplateId{1};

    /// Thread safety
    mutable std::shared_mutex _mutex;

    /// Initialization state
    std::atomic<bool> _initialized{false};

    // ========================================================================
    // CONFIGURATION (loaded from playerbots.conf)
    // ========================================================================

    /// Log template usage statistics
    bool _logUsage = false;

    /// Log template creation details
    bool _logCreation = true;

    /// Gear levels to pre-generate for templates
    std::vector<uint32> _gearLevels = {400, 450, 480, 510, 545, 580};

    /// Default gear item level for new bots
    uint32 _defaultGearLevel = 450;

    /// Scale bot gear to match content difficulty
    bool _scaleGearToContent = true;
};

} // namespace Playerbot

#define sBotTemplateRepository Playerbot::BotTemplateRepository::Instance()
