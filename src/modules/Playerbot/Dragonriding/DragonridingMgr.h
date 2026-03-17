/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "DragonridingDefines.h"
#include "Define.h"
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class Player;
class WorldSession;

namespace Playerbot
{
namespace Dragonriding
{

// Forward declarations
struct GlyphLocation;
struct TalentTemplate;

// ============================================================================
// GLYPH LOCATION DATA
// Loaded from playerbot_dragonriding_glyph_templates table
// ============================================================================

struct GlyphLocation
{
    uint32 glyphId;
    uint32 mapId;
    uint32 zoneId;
    std::string zoneName;
    float posX;
    float posY;
    float posZ;
    float collectionRadius;
    uint32 achievementId;
    std::string name;
};

// ============================================================================
// TALENT TEMPLATE DATA
// Loaded from playerbot_dragonriding_talent_templates table
// ============================================================================

struct TalentTemplate
{
    DragonridingTalentId talentId;
    std::string name;
    std::string description;
    std::string branch;
    uint8 tier;
    uint32 glyphCost;
    DragonridingTalentId prerequisiteTalentId;
    std::string effectType;
    int32 effectValue;
};

// ============================================================================
// ACCOUNT PROGRESSION DATA
// Cached per-account dragonriding progression
// ============================================================================

struct AccountDragonridingData
{
    uint32 accountId;
    std::set<uint32> collectedGlyphs;           // Glyph IDs collected
    std::set<DragonridingTalentId> learnedTalents;  // Talents learned
    uint32 glyphsCollected;                     // Total glyphs collected
    uint32 glyphsSpent;                         // Glyphs spent on talents
    bool isDirty;                               // Needs saving to database
    uint32 lastSaveTime;                        // Last database save timestamp

    AccountDragonridingData() : accountId(0), glyphsCollected(0), glyphsSpent(0),
                                 isDirty(false), lastSaveTime(0) {}
};

// ============================================================================
// DRAGONRIDING MANAGER
// Singleton manager for dragonriding progression system
// ============================================================================

class TC_GAME_API DragonridingMgr
{
public:
    static DragonridingMgr* Instance();

    // Initialization
    bool Initialize();
    void Shutdown();
    bool IsInitialized() const { return _initialized; }

    // Configuration
    bool IsEnabled() const { return _enabled; }
    bool IsProgressionEnabled() const { return _progressionEnabled; }
    uint32 GetStartingGlyphs() const { return _startingGlyphs; }
    bool IsBotAutoBoostEnabled() const { return _botAutoBoost; }
    float GetThrillSpeedThreshold() const { return _thrillSpeedThreshold; }
    float GetGroundSkimHeight() const { return _groundSkimHeight; }
    void LoadConfig();

    // ========================================================================
    // GLYPH MANAGEMENT (Account-Wide)
    // ========================================================================

    // Get total glyphs collected by account
    uint32 GetGlyphCount(uint32 accountId) const;

    // Get glyphs spent on talents
    uint32 GetSpentGlyphs(uint32 accountId) const;

    // Get available glyphs (collected - spent)
    uint32 GetAvailableGlyphs(uint32 accountId) const;

    // Check if account has collected a specific glyph
    bool HasGlyph(uint32 accountId, uint32 glyphId) const;

    // Collect a glyph (called when player flies through glyph location)
    void CollectGlyph(Player* player, uint32 glyphId);

    // Get all collected glyph IDs for account
    std::vector<uint32> GetCollectedGlyphs(uint32 accountId) const;

    // ========================================================================
    // TALENT MANAGEMENT (Account-Wide)
    // ========================================================================

    // Check if account has learned a specific talent
    bool HasTalent(uint32 accountId, DragonridingTalentId talent) const;

    // Check if account can learn a talent (has prerequisite and glyphs)
    bool CanLearnTalent(uint32 accountId, DragonridingTalentId talent) const;

    // Learn a talent (spends glyphs)
    bool LearnTalent(Player* player, DragonridingTalentId talent);

    // Reset all talents (refunds glyphs)
    void ResetTalents(uint32 accountId);

    // Get all learned talents for account
    std::vector<DragonridingTalentId> GetLearnedTalents(uint32 accountId) const;

    // ========================================================================
    // CALCULATED VALUES (Based on Learned Talents)
    // ========================================================================

    // Get maximum vigor capacity (3-6 based on talents)
    uint32 GetMaxVigor(uint32 accountId) const;

    // Get grounded vigor regeneration rate in milliseconds (30000-15000)
    uint32 GetGroundedRegenMs(uint32 accountId) const;

    // Get flying vigor regeneration rate in milliseconds (15000-5000)
    uint32 GetFlyingRegenMs(uint32 accountId) const;

    // Check if account has Ground Skimming ability
    bool HasGroundSkimming(uint32 accountId) const;

    // Check if account has Whirling Surge ability
    bool HasWhirlingSurge(uint32 accountId) const;

    // Check if account has Aerial Halt ability
    bool HasAerialHalt(uint32 accountId) const;

    // ========================================================================
    // DATA ACCESS
    // ========================================================================

    // Get glyph location data
    const GlyphLocation* GetGlyphLocation(uint32 glyphId) const;
    const std::vector<GlyphLocation>& GetAllGlyphLocations() const { return _glyphLocations; }

    // Get talent template data
    const TalentTemplate* GetTalentTemplate(DragonridingTalentId talentId) const;
    const std::vector<TalentTemplate>& GetAllTalentTemplates() const { return _talentTemplates; }

    // ========================================================================
    // PLAYER SESSION MANAGEMENT
    // ========================================================================

    // Load account data when player logs in
    void LoadAccountData(uint32 accountId);

    // Save account data when player logs out
    void SaveAccountData(uint32 accountId);

    // Save all dirty account data (called periodically)
    void SaveAllDirtyData();

    // Unload account data when no characters online
    void UnloadAccountData(uint32 accountId);

    // ========================================================================
    // ACCESS CONTROL
    // ========================================================================

    // Check if player can use Soar (Dracthyr Evoker only)
    bool CanUseSoar(Player* player) const;

    // Check if player can use dragonriding abilities
    bool CanUseDragonriding(Player* player) const;

private:
    DragonridingMgr();
    ~DragonridingMgr();
    DragonridingMgr(DragonridingMgr const&) = delete;
    DragonridingMgr& operator=(DragonridingMgr const&) = delete;

    // Initialization helpers
    void LoadGlyphLocations();
    void LoadTalentTemplates();

    // Account data management
    AccountDragonridingData* GetOrCreateAccountData(uint32 accountId);
    const AccountDragonridingData* GetAccountData(uint32 accountId) const;
    void MarkAccountDirty(uint32 accountId);

    // Configuration
    bool _enabled;
    bool _progressionEnabled;
    uint32 _startingGlyphs;
    bool _botAutoBoost;
    float _thrillSpeedThreshold;
    float _groundSkimHeight;
    bool _initialized;

    // Static data (loaded from database once)
    std::vector<GlyphLocation> _glyphLocations;
    std::unordered_map<uint32, size_t> _glyphLocationIndex;  // glyphId -> vector index
    std::vector<TalentTemplate> _talentTemplates;
    std::unordered_map<DragonridingTalentId, size_t> _talentTemplateIndex;  // talentId -> vector index

    // Per-account progression data (cached)
    std::unordered_map<uint32, AccountDragonridingData> _accountData;  // accountId -> data
    mutable std::mutex _dataMutex;
};

#define sDragonridingMgr DragonridingMgr::Instance()

} // namespace Dragonriding
} // namespace Playerbot
