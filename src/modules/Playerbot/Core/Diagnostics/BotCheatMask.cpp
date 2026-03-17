/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * BotCheatMask: Per-bot cheat system for testing and debugging.
 */

#include "BotCheatMask.h"
#include "Player.h"
#include "Log.h"
#include <sstream>
#include <algorithm>

namespace Playerbot
{

static std::vector<CheatInfo> const s_cheatList = {
    { "speed",          "2x movement speed",                    BotCheatFlag::SPEED },
    { "fly",            "Enable flying",                        BotCheatFlag::FLY },
    { "nofall",         "No fall damage",                       BotCheatFlag::NO_FALL_DAMAGE },
    { "teleport",       "Instant teleport to target",           BotCheatFlag::TELEPORT },
    { "damage",         "10x damage output",                    BotCheatFlag::DAMAGE },
    { "health",         "Infinite health",                      BotCheatFlag::HEALTH },
    { "mana",           "Infinite mana/resources",              BotCheatFlag::MANA },
    { "cooldowns",      "No spell cooldowns",                   BotCheatFlag::COOLDOWNS },
    { "god",            "Immune to all damage",                 BotCheatFlag::GOD_MODE },
    { "oneshot",        "Kill targets in one hit",              BotCheatFlag::ONE_SHOT },
    { "instant",        "Instant cast all spells",              BotCheatFlag::INSTANT_CAST },
    { "noaggro",        "NPCs won't aggro",                    BotCheatFlag::NO_AGGRO },
    { "lootall",        "Auto-loot everything",                 BotCheatFlag::LOOT_ALL },
    { "unlimitedbag",   "Never run out of bag space",           BotCheatFlag::UNLIMITED_BAG },
    { "xpboost",        "10x XP gain",                          BotCheatFlag::XP_BOOST },
    { "combat",         "All combat cheats",                    BotCheatFlag::ALL_COMBAT },
    { "movement",       "All movement cheats",                  BotCheatFlag::ALL_MOVEMENT },
    { "all",            "All cheats enabled",                   BotCheatFlag::ALL },
};

void BotCheatMask::Initialize()
{
    if (_initialized)
        return;

    _initialized = true;
    TC_LOG_INFO("module.playerbot", "BotCheatMask: Initialized ({} cheat types available)",
        s_cheatList.size());
}

// ============================================================================
// Per-Bot Cheat Management
// ============================================================================

void BotCheatMask::EnableCheat(ObjectGuid botGuid, BotCheatFlag cheat)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _botCheats[botGuid].flags |= cheat;
}

void BotCheatMask::DisableCheat(ObjectGuid botGuid, BotCheatFlag cheat)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _botCheats.find(botGuid);
    if (it != _botCheats.end())
    {
        it->second.flags &= ~cheat;
        if (it->second.flags == BotCheatFlag::NONE)
            _botCheats.erase(it);
    }
}

void BotCheatMask::ToggleCheat(ObjectGuid botGuid, BotCheatFlag cheat)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto& state = _botCheats[botGuid];
    if (Playerbot::HasCheat(state.flags, cheat))
        state.flags &= ~cheat;
    else
        state.flags |= cheat;

    if (state.flags == BotCheatFlag::NONE)
        _botCheats.erase(botGuid);
}

void BotCheatMask::SetCheats(ObjectGuid botGuid, BotCheatFlag cheats)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (cheats == BotCheatFlag::NONE)
        _botCheats.erase(botGuid);
    else
        _botCheats[botGuid].flags = cheats;
}

void BotCheatMask::ClearAllCheats(ObjectGuid botGuid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _botCheats.erase(botGuid);
}

void BotCheatMask::ClearAllBotCheats()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _botCheats.clear();
    TC_LOG_INFO("module.playerbot", "BotCheatMask: Cleared all cheats on all bots");
}

// ============================================================================
// Queries
// ============================================================================

bool BotCheatMask::HasCheat(ObjectGuid botGuid, BotCheatFlag cheat) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _botCheats.find(botGuid);
    if (it == _botCheats.end())
        return false;
    return Playerbot::HasCheat(it->second.flags, cheat);
}

BotCheatFlag BotCheatMask::GetCheats(ObjectGuid botGuid) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _botCheats.find(botGuid);
    return it != _botCheats.end() ? it->second.flags : BotCheatFlag::NONE;
}

BotCheatState BotCheatMask::GetCheatState(ObjectGuid botGuid) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _botCheats.find(botGuid);
    return it != _botCheats.end() ? it->second : BotCheatState{};
}

bool BotCheatMask::HasAnyCheats(ObjectGuid botGuid) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _botCheats.find(botGuid);
    return it != _botCheats.end() && it->second.flags != BotCheatFlag::NONE;
}

uint32 BotCheatMask::GetCheatBotCount() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return static_cast<uint32>(_botCheats.size());
}

// ============================================================================
// Multiplier Configuration
// ============================================================================

void BotCheatMask::SetSpeedMultiplier(ObjectGuid botGuid, float mult)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _botCheats[botGuid].speedMultiplier = std::max(0.1f, std::min(mult, 20.0f));
}

void BotCheatMask::SetDamageMultiplier(ObjectGuid botGuid, float mult)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _botCheats[botGuid].damageMultiplier = std::max(0.1f, std::min(mult, 1000.0f));
}

void BotCheatMask::SetXPMultiplier(ObjectGuid botGuid, float mult)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _botCheats[botGuid].xpMultiplier = std::max(0.1f, std::min(mult, 100.0f));
}

float BotCheatMask::GetSpeedMultiplier(ObjectGuid botGuid) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _botCheats.find(botGuid);
    if (it == _botCheats.end() || !Playerbot::HasCheat(it->second.flags, BotCheatFlag::SPEED))
        return 1.0f;
    return it->second.speedMultiplier;
}

float BotCheatMask::GetDamageMultiplier(ObjectGuid botGuid) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _botCheats.find(botGuid);
    if (it == _botCheats.end() || !Playerbot::HasCheat(it->second.flags, BotCheatFlag::DAMAGE))
        return 1.0f;
    return it->second.damageMultiplier;
}

float BotCheatMask::GetXPMultiplier(ObjectGuid botGuid) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _botCheats.find(botGuid);
    if (it == _botCheats.end() || !Playerbot::HasCheat(it->second.flags, BotCheatFlag::XP_BOOST))
        return 1.0f;
    return it->second.xpMultiplier;
}

// ============================================================================
// Cheat Application
// ============================================================================

void BotCheatMask::ApplyCheatEffects(Player* bot)
{
    if (!bot)
        return;

    ObjectGuid guid = bot->GetGUID();
    BotCheatState state;

    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _botCheats.find(guid);
        if (it == _botCheats.end())
            return;
        state = it->second;
    }

    // Health cheat: keep at max
    if (Playerbot::HasCheat(state.flags, BotCheatFlag::HEALTH))
    {
        if (bot->GetHealth() < bot->GetMaxHealth())
            bot->SetFullHealth();
    }

    // Mana cheat: keep at max
    if (Playerbot::HasCheat(state.flags, BotCheatFlag::MANA))
    {
        if (bot->GetPower(bot->GetPowerType()) < bot->GetMaxPower(bot->GetPowerType()))
            bot->SetFullPower(bot->GetPowerType());
    }
}

uint32 BotCheatMask::ModifyDamage(ObjectGuid botGuid, uint32 baseDamage) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _botCheats.find(botGuid);
    if (it == _botCheats.end())
        return baseDamage;

    if (Playerbot::HasCheat(it->second.flags, BotCheatFlag::ONE_SHOT))
        return 999999999;

    if (Playerbot::HasCheat(it->second.flags, BotCheatFlag::DAMAGE))
        return static_cast<uint32>(baseDamage * it->second.damageMultiplier);

    return baseDamage;
}

bool BotCheatMask::ShouldTakeDamage(ObjectGuid botGuid) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _botCheats.find(botGuid);
    if (it == _botCheats.end())
        return true;
    return !Playerbot::HasCheat(it->second.flags, BotCheatFlag::GOD_MODE);
}

bool BotCheatMask::ShouldHaveCooldown(ObjectGuid botGuid) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _botCheats.find(botGuid);
    if (it == _botCheats.end())
        return true;
    return !Playerbot::HasCheat(it->second.flags, BotCheatFlag::COOLDOWNS);
}

// ============================================================================
// Command Parsing
// ============================================================================

std::vector<CheatInfo> const& BotCheatMask::GetCheatList()
{
    return s_cheatList;
}

BotCheatFlag BotCheatMask::ParseCheatName(std::string const& name)
{
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    for (auto const& info : s_cheatList)
    {
        if (lower == info.name)
            return info.flag;
    }
    return BotCheatFlag::NONE;
}

char const* BotCheatMask::GetCheatName(BotCheatFlag flag)
{
    for (auto const& info : s_cheatList)
    {
        if (info.flag == flag)
            return info.name;
    }
    return "unknown";
}

std::string BotCheatMask::FormatActiveCheats(ObjectGuid botGuid) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _botCheats.find(botGuid);
    if (it == _botCheats.end() || it->second.flags == BotCheatFlag::NONE)
        return "none";

    std::ostringstream ss;
    bool first = true;

    // Only list individual cheats (skip presets)
    for (auto const& info : s_cheatList)
    {
        // Skip preset flags
        if (info.flag == BotCheatFlag::ALL_COMBAT ||
            info.flag == BotCheatFlag::ALL_MOVEMENT ||
            info.flag == BotCheatFlag::ALL)
            continue;

        if (Playerbot::HasCheat(it->second.flags, info.flag))
        {
            if (!first)
                ss << ", ";
            ss << info.name;
            first = false;
        }
    }

    return first ? "none" : ss.str();
}

} // namespace Playerbot
