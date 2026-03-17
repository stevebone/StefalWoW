/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 */

#include "GroupBuffCoordinator.h"
#include "Player.h"
#include "Group.h"
#include "SpellAuras.h"
#include "GameTime.h"
#include "Log.h"

namespace Playerbot
{

bool GroupBuffCoordinator::TryClaimBuff(Player const* bot, uint32 spellId)
{
    if (!bot)
        return false;

    RaidBuffCategory category = RaidBuffs::GetCategory(spellId);
    if (category == RaidBuffCategory::MAX)
        return true; // Not a tracked raid buff — allow freely

    // If not in a group, always allow (solo buffing)
    Group const* group = bot->GetGroup();
    if (!group)
        return true;

    // Check if any group member already has this buff
    if (IsBuffActiveInGroup(bot, spellId))
        return false; // Buff already active, no need to cast

    uint32 groupId = group->GetDbStoreId();
    ClaimKey key = MakeKey(groupId, category);
    uint32 currentTime = GameTime::GetGameTimeMS();

    std::unique_lock lock(_mutex);

    auto it = _claims.find(key);
    if (it != _claims.end())
    {
        BuffClaim& claim = it->second;

        // If existing claim is expired, replace it
        if (claim.IsExpired(currentTime))
        {
            claim.claimerGuid = bot->GetGUID();
            claim.claimTimeMs = currentTime;
            claim.spellId = spellId;
            return true;
        }

        // If this bot already has the claim, refresh it
        if (claim.claimerGuid == bot->GetGUID())
        {
            claim.claimTimeMs = currentTime;
            return true;
        }

        // Another bot has an active claim
        return false;
    }

    // No claim exists — create one
    _claims[key] = BuffClaim{bot->GetGUID(), currentTime, spellId};
    return true;
}

void GroupBuffCoordinator::OnBuffApplied(Player const* bot, uint32 spellId)
{
    if (!bot)
        return;

    RaidBuffCategory category = RaidBuffs::GetCategory(spellId);
    if (category == RaidBuffCategory::MAX)
        return;

    Group const* group = bot->GetGroup();
    if (!group)
        return;

    uint32 groupId = group->GetDbStoreId();
    ClaimKey key = MakeKey(groupId, category);

    std::unique_lock lock(_mutex);
    _claims.erase(key);
}

bool GroupBuffCoordinator::IsBuffActiveInGroup(Player const* bot, uint32 spellId) const
{
    if (!bot)
        return false;

    Group const* group = bot->GetGroup();
    if (!group)
        return bot->HasAura(spellId);

    // Check if any group member has the buff
    for (GroupReference const& itr : group->GetMembers())
    {
        Player* member = itr.GetSource();
        if (member && member->IsInWorld() && member->HasAura(spellId))
            return true;
    }

    return false;
}

std::vector<RaidBuffCategory> GroupBuffCoordinator::GetMissingBuffs(Player const* bot) const
{
    std::vector<RaidBuffCategory> missing;
    if (!bot)
        return missing;

    Group const* group = bot->GetGroup();

    for (size_t i = 0; i < RaidBuffs::BUFF_COUNT; ++i)
    {
        const RaidBuffInfo& info = RaidBuffs::ALL_BUFFS[i];
        bool found = false;

        if (group)
        {
            for (GroupReference const& itr : group->GetMembers())
            {
                Player* member = itr.GetSource();
                if (member && member->IsInWorld() && member->HasAura(info.spellId))
                {
                    found = true;
                    break;
                }
            }
        }
        else
        {
            found = bot->HasAura(info.spellId);
        }

        if (!found)
            missing.push_back(info.category);
    }

    return missing;
}

uint32 GroupBuffCoordinator::GetBuffToCast(Player const* bot) const
{
    if (!bot)
        return 0;

    uint8 botClass = bot->GetClass();
    auto missingBuffs = GetMissingBuffs(bot);

    for (RaidBuffCategory category : missingBuffs)
    {
        // Find the buff info for this category
        for (size_t i = 0; i < RaidBuffs::BUFF_COUNT; ++i)
        {
            const RaidBuffInfo& info = RaidBuffs::ALL_BUFFS[i];
            if (info.category == category && info.providerClassId == botClass)
            {
                // This bot's class provides this missing buff
                if (bot->HasSpell(info.spellId))
                    return info.spellId;
            }
        }
    }

    return 0; // No missing buff that this bot can provide
}

void GroupBuffCoordinator::ClearGroupClaims(uint32 groupId)
{
    std::unique_lock lock(_mutex);

    std::erase_if(_claims, [groupId](const auto& pair) {
        return (pair.first >> 8) == groupId;
    });
}

void GroupBuffCoordinator::CleanupExpiredClaims(uint32 currentTimeMs)
{
    if (currentTimeMs - _lastCleanupMs < CLEANUP_INTERVAL_MS)
        return;

    _lastCleanupMs = currentTimeMs;

    std::unique_lock lock(_mutex);

    size_t before = _claims.size();
    std::erase_if(_claims, [currentTimeMs](const auto& pair) {
        return pair.second.IsExpired(currentTimeMs);
    });

    size_t removed = before - _claims.size();
    if (removed > 0)
    {
        TC_LOG_DEBUG("module.playerbot", "GroupBuffCoordinator: Cleaned up {} expired claims ({} remaining)",
            removed, _claims.size());
    }
}

} // namespace Playerbot
