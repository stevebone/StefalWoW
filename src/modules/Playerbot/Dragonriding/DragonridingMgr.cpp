/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "DragonridingMgr.h"
#include "DragonridingDefines.h"
#include "Database/PlayerbotDatabase.h"
#include "Config/PlayerbotConfig.h"
#include "Player.h"
#include "WorldSession.h"
#include "Log.h"
#include "GameTime.h"
#include <sstream>

namespace Playerbot
{
namespace Dragonriding
{

// ============================================================================
// SINGLETON INSTANCE
// ============================================================================

DragonridingMgr* DragonridingMgr::Instance()
{
    static DragonridingMgr instance;
    return &instance;
}

DragonridingMgr::DragonridingMgr()
    : _enabled(true)
    , _progressionEnabled(true)
    , _startingGlyphs(0)
    , _botAutoBoost(true)
    , _thrillSpeedThreshold(THRILL_SPEED_THRESHOLD)
    , _groundSkimHeight(GROUND_SKIM_HEIGHT)
    , _initialized(false)
{
}

DragonridingMgr::~DragonridingMgr()
{
    Shutdown();
}

// ============================================================================
// INITIALIZATION
// ============================================================================

bool DragonridingMgr::Initialize()
{
    if (_initialized)
        return true;

    TC_LOG_INFO("playerbot.dragonriding", "DragonridingMgr: Initializing dragonriding system...");

    // Load configuration
    LoadConfig();

    if (!_enabled)
    {
        TC_LOG_INFO("playerbot.dragonriding", "DragonridingMgr: Dragonriding system is disabled in config");
        return true;
    }

    // Load static data from database
    LoadGlyphLocations();
    LoadTalentTemplates();

    _initialized = true;
    TC_LOG_INFO("playerbot.dragonriding", "DragonridingMgr: Initialized successfully - {} glyphs, {} talents loaded",
                _glyphLocations.size(), _talentTemplates.size());

    return true;
}

void DragonridingMgr::Shutdown()
{
    if (!_initialized)
        return;

    TC_LOG_INFO("playerbot.dragonriding", "DragonridingMgr: Shutting down...");

    // Save all dirty account data
    SaveAllDirtyData();

    // Clear data
    {
        std::lock_guard<std::mutex> lock(_dataMutex);
        _accountData.clear();
    }

    _glyphLocations.clear();
    _glyphLocationIndex.clear();
    _talentTemplates.clear();
    _talentTemplateIndex.clear();

    _initialized = false;
    TC_LOG_INFO("playerbot.dragonriding", "DragonridingMgr: Shutdown complete");
}

void DragonridingMgr::LoadConfig()
{
    if (!sPlayerbotConfig)
        return;

    _enabled = sPlayerbotConfig->GetBool(CONFIG_DRAGONRIDING_ENABLED, true);
    _progressionEnabled = sPlayerbotConfig->GetBool(CONFIG_PROGRESSION_ENABLED, true);
    _startingGlyphs = sPlayerbotConfig->GetInt(CONFIG_STARTING_GLYPHS, 0);
    _botAutoBoost = sPlayerbotConfig->GetBool(CONFIG_BOT_AUTO_BOOST, true);
    _thrillSpeedThreshold = sPlayerbotConfig->GetFloat(CONFIG_THRILL_SPEED_THRESHOLD, THRILL_SPEED_THRESHOLD * 100.0f) / 100.0f;
    _groundSkimHeight = sPlayerbotConfig->GetFloat(CONFIG_GROUND_SKIM_HEIGHT, GROUND_SKIM_HEIGHT);

    TC_LOG_DEBUG("playerbot.dragonriding", "DragonridingMgr: Config loaded - enabled={}, progression={}, startingGlyphs={}",
                 _enabled, _progressionEnabled, _startingGlyphs);
}

// ============================================================================
// DATA LOADING
// ============================================================================

void DragonridingMgr::LoadGlyphLocations()
{
    _glyphLocations.clear();
    _glyphLocationIndex.clear();

    if (!sPlayerbotDatabase || !sPlayerbotDatabase->IsConnected())
    {
        TC_LOG_WARN("playerbot.dragonriding", "DragonridingMgr: Database not connected, using hardcoded glyph data");
        return;
    }

    QueryResult result = sPlayerbotDatabase->Query(
        "SELECT glyph_id, map_id, zone_id, zone_name, pos_x, pos_y, pos_z, collection_radius, achievement_id, name "
        "FROM playerbot_dragonriding_glyph_templates ORDER BY glyph_id");

    if (!result)
    {
        TC_LOG_WARN("playerbot.dragonriding", "DragonridingMgr: No glyph templates found in database");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        GlyphLocation glyph;
        glyph.glyphId = fields[0].GetUInt32();
        glyph.mapId = fields[1].GetUInt32();
        glyph.zoneId = fields[2].GetUInt32();
        glyph.zoneName = fields[3].GetString();
        glyph.posX = fields[4].GetFloat();
        glyph.posY = fields[5].GetFloat();
        glyph.posZ = fields[6].GetFloat();
        glyph.collectionRadius = fields[7].GetFloat();
        glyph.achievementId = fields[8].GetUInt32();
        glyph.name = fields[9].GetString();

        _glyphLocationIndex[glyph.glyphId] = _glyphLocations.size();
        _glyphLocations.push_back(std::move(glyph));
    }
    while (result->NextRow());

    TC_LOG_INFO("playerbot.dragonriding", "DragonridingMgr: Loaded {} glyph locations", _glyphLocations.size());
}

void DragonridingMgr::LoadTalentTemplates()
{
    _talentTemplates.clear();
    _talentTemplateIndex.clear();

    if (!sPlayerbotDatabase || !sPlayerbotDatabase->IsConnected())
    {
        TC_LOG_WARN("playerbot.dragonriding", "DragonridingMgr: Database not connected, using hardcoded talent data");
        return;
    }

    QueryResult result = sPlayerbotDatabase->Query(
        "SELECT talent_id, name, description, branch, tier, glyph_cost, prerequisite_talent_id, effect_type, effect_value "
        "FROM playerbot_dragonriding_talent_templates ORDER BY talent_id");

    if (!result)
    {
        TC_LOG_WARN("playerbot.dragonriding", "DragonridingMgr: No talent templates found in database");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        TalentTemplate talent;
        talent.talentId = static_cast<DragonridingTalentId>(fields[0].GetUInt32());
        talent.name = fields[1].GetString();
        talent.description = fields[2].GetString();
        talent.branch = fields[3].GetString();
        talent.tier = fields[4].GetUInt8();
        talent.glyphCost = fields[5].GetUInt32();
        talent.prerequisiteTalentId = static_cast<DragonridingTalentId>(fields[6].GetUInt32());
        talent.effectType = fields[7].GetString();
        talent.effectValue = fields[8].GetInt32();

        _talentTemplateIndex[talent.talentId] = _talentTemplates.size();
        _talentTemplates.push_back(std::move(talent));
    }
    while (result->NextRow());

    TC_LOG_INFO("playerbot.dragonriding", "DragonridingMgr: Loaded {} talent templates", _talentTemplates.size());
}

// ============================================================================
// ACCOUNT DATA MANAGEMENT
// ============================================================================

AccountDragonridingData* DragonridingMgr::GetOrCreateAccountData(uint32 accountId)
{
    std::lock_guard<std::mutex> lock(_dataMutex);

    auto it = _accountData.find(accountId);
    if (it != _accountData.end())
        return &it->second;

    // Create new account data
    AccountDragonridingData& data = _accountData[accountId];
    data.accountId = accountId;

    // Grant starting glyphs if configured (for convenience/testing)
    if (_startingGlyphs > 0 && !_progressionEnabled)
    {
        data.glyphsCollected = _startingGlyphs;
    }

    return &data;
}

const AccountDragonridingData* DragonridingMgr::GetAccountData(uint32 accountId) const
{
    std::lock_guard<std::mutex> lock(_dataMutex);

    auto it = _accountData.find(accountId);
    if (it != _accountData.end())
        return &it->second;

    return nullptr;
}

void DragonridingMgr::MarkAccountDirty(uint32 accountId)
{
    std::lock_guard<std::mutex> lock(_dataMutex);

    auto it = _accountData.find(accountId);
    if (it != _accountData.end())
        it->second.isDirty = true;
}

void DragonridingMgr::LoadAccountData(uint32 accountId)
{
    if (!sPlayerbotDatabase || !sPlayerbotDatabase->IsConnected())
        return;

    AccountDragonridingData* data = GetOrCreateAccountData(accountId);
    if (!data)
        return;

    // Load collected glyphs
    std::ostringstream glyphQuery;
    glyphQuery << "SELECT glyph_id FROM playerbot_dragonriding_glyphs WHERE account_id = " << accountId;
    if (QueryResult result = sPlayerbotDatabase->Query(glyphQuery.str()))
    {
        do
        {
            Field* fields = result->Fetch();
            data->collectedGlyphs.insert(fields[0].GetUInt32());
        }
        while (result->NextRow());
    }
    data->glyphsCollected = static_cast<uint32>(data->collectedGlyphs.size());

    // Load learned talents
    std::ostringstream talentQuery;
    talentQuery << "SELECT talent_id, glyphs_spent FROM playerbot_dragonriding_talents WHERE account_id = " << accountId;
    if (QueryResult result = sPlayerbotDatabase->Query(talentQuery.str()))
    {
        do
        {
            Field* fields = result->Fetch();
            DragonridingTalentId talentId = static_cast<DragonridingTalentId>(fields[0].GetUInt32());
            uint32 glyphsSpent = fields[1].GetUInt32();

            data->learnedTalents.insert(talentId);
            data->glyphsSpent += glyphsSpent;
        }
        while (result->NextRow());
    }

    data->isDirty = false;
    data->lastSaveTime = GameTime::GetGameTimeMS();

    TC_LOG_DEBUG("playerbot.dragonriding", "DragonridingMgr: Loaded account {} data - {} glyphs, {} talents",
                 accountId, data->glyphsCollected, data->learnedTalents.size());
}

void DragonridingMgr::SaveAccountData(uint32 accountId)
{
    if (!sPlayerbotDatabase || !sPlayerbotDatabase->IsConnected())
        return;

    std::lock_guard<std::mutex> lock(_dataMutex);

    auto it = _accountData.find(accountId);
    if (it == _accountData.end() || !it->second.isDirty)
        return;

    AccountDragonridingData& data = it->second;

    // Save glyphs (using REPLACE to handle conflicts)
    for (uint32 glyphId : data.collectedGlyphs)
    {
        std::ostringstream sql;
        sql << "REPLACE INTO playerbot_dragonriding_glyphs (account_id, glyph_id) VALUES ("
            << accountId << ", " << glyphId << ")";
        sPlayerbotDatabase->Execute(sql.str());
    }

    // Save talents (using REPLACE to handle conflicts)
    for (DragonridingTalentId talentId : data.learnedTalents)
    {
        const TalentCost* cost = GetTalentCost(talentId);
        uint32 glyphsSpent = cost ? cost->glyphCost : 0;

        std::ostringstream sql;
        sql << "REPLACE INTO playerbot_dragonriding_talents (account_id, talent_id, glyphs_spent) VALUES ("
            << accountId << ", " << static_cast<uint32>(talentId) << ", " << glyphsSpent << ")";
        sPlayerbotDatabase->Execute(sql.str());
    }

    data.isDirty = false;
    data.lastSaveTime = GameTime::GetGameTimeMS();

    TC_LOG_DEBUG("playerbot.dragonriding", "DragonridingMgr: Saved account {} data", accountId);
}

void DragonridingMgr::SaveAllDirtyData()
{
    std::vector<uint32> dirtyAccounts;

    {
        std::lock_guard<std::mutex> lock(_dataMutex);
        for (auto& pair : _accountData)
        {
            if (pair.second.isDirty)
                dirtyAccounts.push_back(pair.first);
        }
    }

    for (uint32 accountId : dirtyAccounts)
        SaveAccountData(accountId);

    if (!dirtyAccounts.empty())
        TC_LOG_DEBUG("playerbot.dragonriding", "DragonridingMgr: Saved {} dirty accounts", dirtyAccounts.size());
}

void DragonridingMgr::UnloadAccountData(uint32 accountId)
{
    SaveAccountData(accountId);

    std::lock_guard<std::mutex> lock(_dataMutex);
    _accountData.erase(accountId);

    TC_LOG_DEBUG("playerbot.dragonriding", "DragonridingMgr: Unloaded account {} data", accountId);
}

// ============================================================================
// GLYPH MANAGEMENT
// ============================================================================

uint32 DragonridingMgr::GetGlyphCount(uint32 accountId) const
{
    // If progression is disabled, return max glyphs
    if (!_progressionEnabled)
        return TOTAL_GLYPHS;

    const AccountDragonridingData* data = GetAccountData(accountId);
    return data ? data->glyphsCollected : 0;
}

uint32 DragonridingMgr::GetSpentGlyphs(uint32 accountId) const
{
    if (!_progressionEnabled)
        return 0;

    const AccountDragonridingData* data = GetAccountData(accountId);
    return data ? data->glyphsSpent : 0;
}

uint32 DragonridingMgr::GetAvailableGlyphs(uint32 accountId) const
{
    if (!_progressionEnabled)
        return TOTAL_GLYPHS;

    const AccountDragonridingData* data = GetAccountData(accountId);
    if (!data)
        return 0;

    return data->glyphsCollected > data->glyphsSpent ? data->glyphsCollected - data->glyphsSpent : 0;
}

bool DragonridingMgr::HasGlyph(uint32 accountId, uint32 glyphId) const
{
    if (!_progressionEnabled)
        return true;

    const AccountDragonridingData* data = GetAccountData(accountId);
    if (!data)
        return false;

    std::lock_guard<std::mutex> lock(_dataMutex);
    return data->collectedGlyphs.find(glyphId) != data->collectedGlyphs.end();
}

void DragonridingMgr::CollectGlyph(Player* player, uint32 glyphId)
{
    if (!player)
        return;

    uint32 accountId = player->GetSession()->GetAccountId();

    // Check if already collected
    if (HasGlyph(accountId, glyphId))
        return;

    AccountDragonridingData* data = GetOrCreateAccountData(accountId);
    if (!data)
        return;

    {
        std::lock_guard<std::mutex> lock(_dataMutex);
        data->collectedGlyphs.insert(glyphId);
        data->glyphsCollected = static_cast<uint32>(data->collectedGlyphs.size());
        data->isDirty = true;
    }

    // Get glyph info for logging
    const GlyphLocation* glyph = GetGlyphLocation(glyphId);
    std::string glyphName = glyph ? glyph->name : "Unknown";

    TC_LOG_INFO("playerbot.dragonriding", "DragonridingMgr: Player {} (account {}) collected glyph {} '{}'",
                player->GetName(), accountId, glyphId, glyphName);

    // Achievement, sound/effect, and UI notification would require client addon or custom packet.
    // Collection is tracked server-side and visible via .dr status command.
}

std::vector<uint32> DragonridingMgr::GetCollectedGlyphs(uint32 accountId) const
{
    std::vector<uint32> result;

    const AccountDragonridingData* data = GetAccountData(accountId);
    if (!data)
        return result;

    std::lock_guard<std::mutex> lock(_dataMutex);
    result.reserve(data->collectedGlyphs.size());
    for (uint32 glyphId : data->collectedGlyphs)
        result.push_back(glyphId);

    return result;
}

// ============================================================================
// TALENT MANAGEMENT
// ============================================================================

bool DragonridingMgr::HasTalent(uint32 accountId, DragonridingTalentId talent) const
{
    // If progression is disabled, all talents are available
    if (!_progressionEnabled)
        return true;

    const AccountDragonridingData* data = GetAccountData(accountId);
    if (!data)
        return false;

    std::lock_guard<std::mutex> lock(_dataMutex);
    return data->learnedTalents.find(talent) != data->learnedTalents.end();
}

bool DragonridingMgr::CanLearnTalent(uint32 accountId, DragonridingTalentId talent) const
{
    if (!_progressionEnabled)
        return true;

    // Already learned?
    if (HasTalent(accountId, talent))
        return false;

    // Get talent cost
    const TalentCost* cost = GetTalentCost(talent);
    if (!cost)
        return false;

    // Check prerequisite
    if (cost->prerequisite != DragonridingTalentId::NONE && !HasTalent(accountId, cost->prerequisite))
        return false;

    // Check available glyphs
    if (GetAvailableGlyphs(accountId) < cost->glyphCost)
        return false;

    return true;
}

bool DragonridingMgr::LearnTalent(Player* player, DragonridingTalentId talent)
{
    if (!player)
        return false;

    uint32 accountId = player->GetSession()->GetAccountId();

    if (!CanLearnTalent(accountId, talent))
        return false;

    const TalentCost* cost = GetTalentCost(talent);
    if (!cost)
        return false;

    AccountDragonridingData* data = GetOrCreateAccountData(accountId);
    if (!data)
        return false;

    {
        std::lock_guard<std::mutex> lock(_dataMutex);
        data->learnedTalents.insert(talent);
        data->glyphsSpent += cost->glyphCost;
        data->isDirty = true;
    }

    // Get talent info for logging
    const TalentTemplate* tmpl = GetTalentTemplate(talent);
    std::string talentName = tmpl ? tmpl->name : "Unknown";

    TC_LOG_INFO("playerbot.dragonriding", "DragonridingMgr: Player {} (account {}) learned talent {} '{}' (cost: {} glyphs)",
                player->GetName(), accountId, static_cast<uint32>(talent), talentName, cost->glyphCost);

    return true;
}

void DragonridingMgr::ResetTalents(uint32 accountId)
{
    AccountDragonridingData* data = GetOrCreateAccountData(accountId);
    if (!data)
        return;

    {
        std::lock_guard<std::mutex> lock(_dataMutex);
        data->learnedTalents.clear();
        data->glyphsSpent = 0;
        data->isDirty = true;
    }

    // Delete from database
    if (sPlayerbotDatabase && sPlayerbotDatabase->IsConnected())
    {
        std::ostringstream sql;
        sql << "DELETE FROM playerbot_dragonriding_talents WHERE account_id = " << accountId;
        sPlayerbotDatabase->Execute(sql.str());
    }

    TC_LOG_INFO("playerbot.dragonriding", "DragonridingMgr: Reset talents for account {}", accountId);
}

std::vector<DragonridingTalentId> DragonridingMgr::GetLearnedTalents(uint32 accountId) const
{
    std::vector<DragonridingTalentId> result;

    const AccountDragonridingData* data = GetAccountData(accountId);
    if (!data)
        return result;

    std::lock_guard<std::mutex> lock(_dataMutex);
    result.reserve(data->learnedTalents.size());
    for (DragonridingTalentId talentId : data->learnedTalents)
        result.push_back(talentId);

    return result;
}

// ============================================================================
// CALCULATED VALUES
// ============================================================================

uint32 DragonridingMgr::GetMaxVigor(uint32 accountId) const
{
    // If progression disabled, return max
    if (!_progressionEnabled)
        return MAX_MAX_VIGOR;

    // Start with base
    uint32 maxVigor = BASE_MAX_VIGOR;

    // Check vigor talents (in order)
    if (HasTalent(accountId, DragonridingTalentId::BEYOND_INFINITY))
        maxVigor = 6;
    else if (HasTalent(accountId, DragonridingTalentId::DRAGONRIDING_LEARNER))
        maxVigor = 5;
    else if (HasTalent(accountId, DragonridingTalentId::TAKE_TO_THE_SKIES))
        maxVigor = 4;

    return maxVigor;
}

uint32 DragonridingMgr::GetGroundedRegenMs(uint32 accountId) const
{
    // If progression disabled, return best
    if (!_progressionEnabled)
        return UPGRADED_REGEN_GROUNDED_MS;

    // Start with base
    uint32 regenMs = BASE_REGEN_GROUNDED_MS;

    // Check grounded regen talents (in order)
    if (HasTalent(accountId, DragonridingTalentId::YEARNING_FOR_THE_SKY))
        regenMs = 15000;
    else if (HasTalent(accountId, DragonridingTalentId::RESTORATIVE_TRAVELS))
        regenMs = 20000;
    else if (HasTalent(accountId, DragonridingTalentId::DYNAMIC_STRETCHING))
        regenMs = 25000;

    return regenMs;
}

uint32 DragonridingMgr::GetFlyingRegenMs(uint32 accountId) const
{
    // If progression disabled, return best
    if (!_progressionEnabled)
        return UPGRADED_REGEN_FLYING_MS;

    // Start with base
    uint32 regenMs = BASE_REGEN_FLYING_MS;

    // Check flying regen talents (in order)
    if (HasTalent(accountId, DragonridingTalentId::THRILL_SEEKER))
        regenMs = 5000;
    else if (HasTalent(accountId, DragonridingTalentId::THRILL_CHASER))
        regenMs = 10000;

    return regenMs;
}

bool DragonridingMgr::HasGroundSkimming(uint32 accountId) const
{
    if (!_progressionEnabled)
        return true;

    return HasTalent(accountId, DragonridingTalentId::GROUND_SKIMMING);
}

bool DragonridingMgr::HasWhirlingSurge(uint32 accountId) const
{
    if (!_progressionEnabled)
        return true;

    return HasTalent(accountId, DragonridingTalentId::AIRBORNE_TUMBLING);
}

bool DragonridingMgr::HasAerialHalt(uint32 accountId) const
{
    if (!_progressionEnabled)
        return true;

    return HasTalent(accountId, DragonridingTalentId::AT_HOME_ALOFT);
}

// ============================================================================
// DATA ACCESS
// ============================================================================

const GlyphLocation* DragonridingMgr::GetGlyphLocation(uint32 glyphId) const
{
    auto it = _glyphLocationIndex.find(glyphId);
    if (it != _glyphLocationIndex.end() && it->second < _glyphLocations.size())
        return &_glyphLocations[it->second];

    return nullptr;
}

const TalentTemplate* DragonridingMgr::GetTalentTemplate(DragonridingTalentId talentId) const
{
    auto it = _talentTemplateIndex.find(talentId);
    if (it != _talentTemplateIndex.end() && it->second < _talentTemplates.size())
        return &_talentTemplates[it->second];

    return nullptr;
}

// ============================================================================
// ACCESS CONTROL
// ============================================================================

bool DragonridingMgr::CanUseSoar(Player* player) const
{
    if (!player)
        return false;

    return Playerbot::Dragonriding::CanUseSoar(player->GetRace(), player->GetClass());
}

bool DragonridingMgr::CanUseDragonriding(Player* player) const
{
    // For now, dragonriding abilities are only available through Soar (Evoker racial)
    return CanUseSoar(player);
}

} // namespace Dragonriding
} // namespace Playerbot
