/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Thread-Safe Aura State Cache Implementation
 */

#include "AuraStateCache.h"
#include "Player.h"
#include "Unit.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "Log.h"
#include <mutex>

namespace Playerbot
{

AuraStateCache::AuraStateCache()
{
    RegisterDefaultTrackedSpells();
}

AuraStateCache* AuraStateCache::instance()
{
    static AuraStateCache instance;
    return &instance;
}

void AuraStateCache::RegisterDefaultTrackedSpells()
{
    // Warrior
    _trackedSpells.insert(6673);   // Battle Shout
    _trackedSpells.insert(32216);  // Victorious (Victory Rush proc) - CRITICAL for Victory Rush check
    _trackedSpells.insert(1715);   // Hamstring
    _trackedSpells.insert(12880);  // Enrage

    // Paladin
    _trackedSpells.insert(25780);  // Righteous Fury
    _trackedSpells.insert(19740);  // Blessing of Might
    _trackedSpells.insert(20217);  // Blessing of Kings
    _trackedSpells.insert(465);    // Devotion Aura

    // Hunter
    _trackedSpells.insert(5384);   // Feign Death
    _trackedSpells.insert(136);    // Mend Pet

    // Rogue
    _trackedSpells.insert(1784);   // Stealth
    _trackedSpells.insert(5171);   // Slice and Dice

    // Priest
    _trackedSpells.insert(17);     // Power Word: Shield
    _trackedSpells.insert(139);    // Renew
    _trackedSpells.insert(21562);  // Power Word: Fortitude
    _trackedSpells.insert(586);    // Fade

    // Death Knight
    _trackedSpells.insert(48263);  // Blood Presence
    _trackedSpells.insert(48265);  // Unholy Presence
    _trackedSpells.insert(48266);  // Frost Presence
    _trackedSpells.insert(49222);  // Bone Shield

    // Shaman
    _trackedSpells.insert(192106); // Lightning Shield
    _trackedSpells.insert(546);    // Water Walking
    _trackedSpells.insert(974);    // Earth Shield

    // Mage
    _trackedSpells.insert(1459);   // Arcane Intellect
    _trackedSpells.insert(130);    // Slow Fall
    _trackedSpells.insert(543);    // Fire Ward
    _trackedSpells.insert(6143);   // Frost Ward

    // Warlock
    _trackedSpells.insert(687);    // Demon Skin/Armor
    _trackedSpells.insert(706);    // Demon Armor
    _trackedSpells.insert(172);    // Corruption (DoT tracking)
    _trackedSpells.insert(348);    // Immolate (DoT tracking)

    // Druid
    _trackedSpells.insert(1126);   // Mark of the Wild
    _trackedSpells.insert(774);    // Rejuvenation
    _trackedSpells.insert(8936);   // Regrowth
    _trackedSpells.insert(33763);  // Lifebloom
    _trackedSpells.insert(8921);   // Moonfire (DoT tracking)

    // Monk
    _trackedSpells.insert(116670); // Vivify
    _trackedSpells.insert(115175); // Soothing Mist

    // Demon Hunter
    _trackedSpells.insert(162264); // Metamorphosis

    // Evoker
    _trackedSpells.insert(355913); // Emerald Blossom

    // Common debuffs to track on targets
    _trackedSpells.insert(589);    // Shadow Word: Pain
    _trackedSpells.insert(8042);   // Earth Shock
    _trackedSpells.insert(122);    // Frost Nova
    _trackedSpells.insert(339);    // Entangling Roots
    _trackedSpells.insert(5782);   // Fear

    TC_LOG_INFO("playerbot.cache", "AuraStateCache: Registered {} default tracked spells",
        _trackedSpells.size());
}

void AuraStateCache::UpdateBotAuras(Player* bot)
{
    if (!bot)
        return;

    ObjectGuid botGuid = bot->GetGUID();
    auto now = std::chrono::steady_clock::now();
    auto expiresAt = now + _cacheTTL;

    std::unique_lock lock(_mutex);

    // Update last update time for this bot
    _unitLastUpdate[botGuid] = now;

    // Cache all tracked auras on the bot
    for (uint32 spellId : _trackedSpells)
    {
        AuraCacheKey key{botGuid, spellId, ObjectGuid::Empty};

        // Check if bot has this aura (from any caster)
        if (Aura* aura = bot->GetAura(spellId))
        {
            CachedAuraEntry entry;
            entry.spellId = spellId;
            entry.casterGuid = aura->GetCasterGUID();
            entry.stacks = aura->GetStackAmount();
            entry.duration = aura->GetDuration();
            entry.cachedAt = now;
            entry.expiresAt = expiresAt;

            _cache[key] = entry;

            // Also cache with specific caster for targeted queries
            if (entry.casterGuid != ObjectGuid::Empty)
            {
                AuraCacheKey specificKey{botGuid, spellId, entry.casterGuid};
                _cache[specificKey] = entry;
            }
        }
        else
        {
            // Explicitly cache that aura is NOT present
            _cache.erase(key);
        }
    }

    ++_updateCount;
}

void AuraStateCache::UpdateTargetAuras(Player* bot, Unit* target)
{
    if (!bot || !target)
        return;

    ObjectGuid botGuid = bot->GetGUID();
    ObjectGuid targetGuid = target->GetGUID();
    auto now = std::chrono::steady_clock::now();
    auto expiresAt = now + _cacheTTL;

    std::unique_lock lock(_mutex);

    // Update last update time for target
    _unitLastUpdate[targetGuid] = now;

    // Cache auras applied BY bot TO target
    for (uint32 spellId : _trackedSpells)
    {
        AuraCacheKey key{targetGuid, spellId, botGuid};

        // Check if target has this aura from the bot
        if (target->HasAura(spellId, botGuid))
        {
            Aura* aura = target->GetAura(spellId, botGuid);
            if (aura)
            {
                CachedAuraEntry entry;
                entry.spellId = spellId;
                entry.casterGuid = botGuid;
                entry.stacks = aura->GetStackAmount();
                entry.duration = aura->GetDuration();
                entry.cachedAt = now;
                entry.expiresAt = expiresAt;

                _cache[key] = entry;
            }
        }
        else
        {
            // Aura not present - remove from cache
            _cache.erase(key);
        }
    }

    ++_updateCount;
}

void AuraStateCache::SetAuraState(ObjectGuid unitGuid, uint32 spellId, ObjectGuid casterGuid,
                                   bool hasAura, uint32 stacks, uint32 duration)
{
    auto now = std::chrono::steady_clock::now();
    AuraCacheKey key{unitGuid, spellId, casterGuid};

    std::unique_lock lock(_mutex);

    if (hasAura)
    {
        CachedAuraEntry entry;
        entry.spellId = spellId;
        entry.casterGuid = casterGuid;
        entry.stacks = stacks;
        entry.duration = duration;
        entry.cachedAt = now;
        entry.expiresAt = now + _cacheTTL;

        _cache[key] = entry;
        _unitLastUpdate[unitGuid] = now;
    }
    else
    {
        _cache.erase(key);
    }
}

void AuraStateCache::InvalidateUnit(ObjectGuid unitGuid)
{
    std::unique_lock lock(_mutex);

    // Remove all entries for this unit
    for (auto it = _cache.begin(); it != _cache.end(); )
    {
        if (it->first.unitGuid == unitGuid)
            it = _cache.erase(it);
        else
            ++it;
    }

    _unitLastUpdate.erase(unitGuid);
}

void AuraStateCache::Clear()
{
    std::unique_lock lock(_mutex);
    _cache.clear();
    _unitLastUpdate.clear();
}

void AuraStateCache::CleanupExpired()
{
    auto now = std::chrono::steady_clock::now();

    std::unique_lock lock(_mutex);

    for (auto it = _cache.begin(); it != _cache.end(); )
    {
        if (it->second.expiresAt < now)
            it = _cache.erase(it);
        else
            ++it;
    }
}

bool AuraStateCache::HasCachedAura(ObjectGuid unitGuid, uint32 spellId, ObjectGuid casterGuid) const
{
    AuraCacheKey key{unitGuid, spellId, casterGuid};

    std::shared_lock lock(_mutex);

    auto it = _cache.find(key);
    if (it == _cache.end())
    {
        // If no specific caster requested, try with any caster
        if (!casterGuid.IsEmpty())
        {
            AuraCacheKey anyKey{unitGuid, spellId, ObjectGuid::Empty};
            it = _cache.find(anyKey);
        }

        if (it == _cache.end())
        {
            ++_cacheMisses;
            return false;
        }
    }

    // Check if expired
    if (it->second.IsExpired())
    {
        ++_cacheMisses;
        return false;
    }

    ++_cacheHits;
    return true;
}

bool AuraStateCache::GetCachedAura(ObjectGuid unitGuid, uint32 spellId, ObjectGuid casterGuid,
                                    CachedAuraEntry& outEntry) const
{
    AuraCacheKey key{unitGuid, spellId, casterGuid};

    std::shared_lock lock(_mutex);

    auto it = _cache.find(key);
    if (it == _cache.end())
    {
        // If no specific caster requested, try with any caster
        if (!casterGuid.IsEmpty())
        {
            AuraCacheKey anyKey{unitGuid, spellId, ObjectGuid::Empty};
            it = _cache.find(anyKey);
        }

        if (it == _cache.end())
        {
            ++_cacheMisses;
            return false;
        }
    }

    if (it->second.IsExpired())
    {
        ++_cacheMisses;
        return false;
    }

    ++_cacheHits;
    outEntry = it->second;
    return true;
}

bool AuraStateCache::HasCachedData(ObjectGuid unitGuid) const
{
    std::shared_lock lock(_mutex);
    return _unitLastUpdate.find(unitGuid) != _unitLastUpdate.end();
}

uint32 AuraStateCache::GetCacheAge(ObjectGuid unitGuid) const
{
    std::shared_lock lock(_mutex);

    auto it = _unitLastUpdate.find(unitGuid);
    if (it == _unitLastUpdate.end())
        return UINT32_MAX;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);
    return static_cast<uint32>(elapsed.count());
}

void AuraStateCache::RegisterTrackedSpells(std::vector<uint32> const& spellIds)
{
    std::unique_lock lock(_mutex);
    for (uint32 spellId : spellIds)
        _trackedSpells.insert(spellId);

    TC_LOG_INFO("playerbot.cache", "AuraStateCache: Now tracking {} spells", _trackedSpells.size());
}

void AuraStateCache::AddTrackedSpell(uint32 spellId)
{
    std::unique_lock lock(_mutex);
    _trackedSpells.insert(spellId);
}

AuraStateCache::CacheStats AuraStateCache::GetStats() const
{
    std::shared_lock lock(_mutex);

    CacheStats stats;
    stats.totalEntries = static_cast<uint32>(_cache.size());
    stats.cacheHits = _cacheHits.load();
    stats.cacheMisses = _cacheMisses.load();
    stats.updateCount = _updateCount.load();

    // Count expired entries
    auto now = std::chrono::steady_clock::now();
    stats.expiredEntries = 0;
    for (auto const& pair : _cache)
    {
        if (pair.second.expiresAt < now)
            ++stats.expiredEntries;
    }

    return stats;
}

void AuraStateCache::ResetStats()
{
    _cacheHits = 0;
    _cacheMisses = 0;
    _updateCount = 0;
}

} // namespace Playerbot
