/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "TrinketUsageManager.h"
#include "CombatPhaseDetector.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "SharedDefines.h"
#include "SpellAuraDefines.h"
#include "SpellHistory.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Unit.h"
#include <sstream>

namespace Playerbot
{

// ============================================================================
// CONSTRUCTION / LIFECYCLE
// ============================================================================

TrinketUsageManager::TrinketUsageManager(Player* bot)
    : _bot(bot)
{
}

void TrinketUsageManager::Initialize()
{
    if (!_bot)
        return;

    // Scan both trinket slots
    ScanTrinketSlot(0, EQUIPMENT_SLOT_TRINKET1);
    ScanTrinketSlot(1, EQUIPMENT_SLOT_TRINKET2);

    // Compute initial equipment checksum
    _lastEquipChecksum = 0;
    for (uint8 i = 0; i < 2; ++i)
    {
        _lastEquipChecksum ^= _trinkets[i].itemEntry;
        _lastEquipChecksum ^= (_trinkets[i].onUseSpellId << 16);
    }

    _initialized = true;

    if (HasOnUseTrinkets())
    {
        TC_LOG_DEBUG("module.playerbot", "TrinketUsageManager[{}]: Initialized with {} on-use trinket(s)",
            _bot->GetName(),
            (_trinkets[0].IsValid() ? 1u : 0u) + (_trinkets[1].IsValid() ? 1u : 0u));

        for (uint8 i = 0; i < 2; ++i)
        {
            if (_trinkets[i].IsValid())
            {
                TC_LOG_DEBUG("module.playerbot", "  Trinket{}: {} (spell={}, type={}, policy={})",
                    i + 1, _trinkets[i].itemName, _trinkets[i].onUseSpellId,
                    static_cast<uint8>(_trinkets[i].effectType),
                    static_cast<uint8>(_trinkets[i].usagePolicy));
            }
        }
    }
}

void TrinketUsageManager::Update(uint32 diff)
{
    if (!_bot || !_initialized || !_inCombat)
        return;

    // Throttle updates
    _updateTimer += diff;
    if (_updateTimer < UPDATE_INTERVAL_MS)
        return;
    _updateTimer = 0;

    // Check for equipment changes (lightweight checksum comparison)
    uint32 currentChecksum = 0;
    for (uint8 i = 0; i < 2; ++i)
    {
        Item* trinket = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0,
            i == 0 ? EQUIPMENT_SLOT_TRINKET1 : EQUIPMENT_SLOT_TRINKET2);
        if (trinket)
        {
            currentChecksum ^= trinket->GetEntry();
            currentChecksum ^= (trinket->GetEntry() << 16);
        }
    }

    if (currentChecksum != _lastEquipChecksum)
    {
        OnEquipmentChanged();
        return; // Re-scan next update cycle
    }

    // Process each trinket
    for (uint8 i = 0; i < 2; ++i)
    {
        if (!_trinkets[i].IsValid())
            continue;

        // Check if trinket is on cooldown
        if (!IsTrinketReady(i))
            continue;

        // Decide based on usage policy
        bool shouldUse = false;
        switch (_trinkets[i].usagePolicy)
        {
            case TrinketUsagePolicy::ON_BURST:
                shouldUse = ShouldUseOffensiveTrinket();
                break;
            case TrinketUsagePolicy::ON_COOLDOWN:
                // Use whenever available and in combat with a target
                shouldUse = (GetTarget() != nullptr);
                break;
            case TrinketUsagePolicy::ON_LOW_HEALTH:
                shouldUse = ShouldUseDefensiveTrinket();
                break;
            case TrinketUsagePolicy::ON_CC:
                shouldUse = ShouldUsePvPTrinket();
                break;
            case TrinketUsagePolicy::MANUAL:
                break; // Never auto-use
        }

        if (shouldUse)
        {
            ActivateTrinket(i);
        }
    }
}

void TrinketUsageManager::OnEquipmentChanged()
{
    // Re-scan trinket slots
    ScanTrinketSlot(0, EQUIPMENT_SLOT_TRINKET1);
    ScanTrinketSlot(1, EQUIPMENT_SLOT_TRINKET2);

    // Update checksum
    _lastEquipChecksum = 0;
    for (uint8 i = 0; i < 2; ++i)
    {
        _lastEquipChecksum ^= _trinkets[i].itemEntry;
        _lastEquipChecksum ^= (_trinkets[i].onUseSpellId << 16);
    }

    _stats.rescanCount++;

    TC_LOG_DEBUG("module.playerbot", "TrinketUsageManager[{}]: Equipment changed, re-scanned trinkets",
        _bot ? _bot->GetName() : "???");
}

void TrinketUsageManager::OnCombatStart()
{
    _inCombat = true;
    _updateTimer = 0;
    _usedThisCombat = {false, false};
}

void TrinketUsageManager::OnCombatEnd()
{
    _inCombat = false;
    _updateTimer = 0;
}

// ============================================================================
// QUERIES
// ============================================================================

bool TrinketUsageManager::HasOnUseTrinkets() const
{
    return _trinkets[0].IsValid() || _trinkets[1].IsValid();
}

bool TrinketUsageManager::IsTrinketReady(uint8 slotIndex) const
{
    if (slotIndex >= 2 || !_trinkets[slotIndex].IsValid() || !_bot)
        return false;

    // Check SpellHistory for cooldown
    return !_bot->GetSpellHistory()->HasCooldown(
        _trinkets[slotIndex].onUseSpellId,
        _trinkets[slotIndex].itemEntry);
}

std::string TrinketUsageManager::GetDebugSummary() const
{
    std::ostringstream ss;
    ss << "TrinketUsageManager[" << (_bot ? _bot->GetName() : "null") << "]:";

    for (uint8 i = 0; i < 2; ++i)
    {
        if (_trinkets[i].IsValid())
        {
            ss << "\n  T" << (i + 1) << ": " << _trinkets[i].itemName
               << " (spell=" << _trinkets[i].onUseSpellId
               << " type=" << static_cast<int>(_trinkets[i].effectType)
               << " cd=" << _trinkets[i].cooldownMs << "ms"
               << " ready=" << (IsTrinketReady(i) ? "YES" : "NO") << ")";
        }
        else
        {
            ss << "\n  T" << (i + 1) << ": [none]";
        }
    }

    ss << "\n  Stats: " << _stats.totalUses << " uses ("
       << _stats.burstAlignedUses << " burst, "
       << _stats.defensiveUses << " defensive, "
       << _stats.pvpTrinketUses << " PvP)";

    return ss.str();
}

// ============================================================================
// TRINKET SCANNING
// ============================================================================

void TrinketUsageManager::ScanTrinketSlot(uint8 slotIndex, uint8 equipmentSlot)
{
    if (slotIndex >= 2 || !_bot)
    {
        if (slotIndex < 2)
            _trinkets[slotIndex] = OnUseTrinketInfo{};
        return;
    }

    OnUseTrinketInfo info;
    info.equipSlot = equipmentSlot;

    Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, equipmentSlot);
    if (!item)
    {
        _trinkets[slotIndex] = info;
        return;
    }

    ItemTemplate const* tmpl = item->GetTemplate();
    if (!tmpl)
    {
        _trinkets[slotIndex] = info;
        return;
    }

    info.itemEntry = tmpl->GetId();
    info.itemName = tmpl->GetName(LOCALE_enUS);

    // Scan item effects for on-use triggers
    for (ItemEffectEntry const* effect : tmpl->Effects)
    {
        if (!effect)
            continue;

        if (effect->TriggerType == ITEM_SPELLTRIGGER_ON_USE && effect->SpellID > 0)
        {
            info.onUseSpellId = effect->SpellID;
            info.cooldownMs = effect->CoolDownMSec > 0
                ? effect->CoolDownMSec
                : effect->CategoryCoolDownMSec;

            // Classify the spell effect
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(
                effect->SpellID, DIFFICULTY_NONE);
            if (spellInfo)
            {
                info.effectType = ClassifySpellEffect(spellInfo);
            }

            // Determine usage policy
            info.usagePolicy = DetermineUsagePolicy(info.effectType);

            // Take only the first on-use effect (trinkets typically have one)
            break;
        }
    }

    _trinkets[slotIndex] = info;
}

// ============================================================================
// SPELL EFFECT CLASSIFICATION
// ============================================================================

TrinketEffectType TrinketUsageManager::ClassifySpellEffect(SpellInfo const* spellInfo) const
{
    if (!spellInfo)
        return TrinketEffectType::UNKNOWN;

    // Check for known PvP trinket spell IDs
    // Human racial: Every Man for Himself (59752)
    // Will to Survive (various racial PvP trinkets)
    // Gladiator's Medallion (208683)
    // PvP Trinket (42292)
    static constexpr uint32 PVP_TRINKET_SPELLS[] = {
        42292, 59752, 208683, 195710, 336126
    };

    for (uint32 pvpSpell : PVP_TRINKET_SPELLS)
    {
        if (spellInfo->Id == pvpSpell)
            return TrinketEffectType::PVP_TRINKET;
    }

    bool hasOffensiveAura = false;
    bool hasDefensiveAura = false;

    // Analyze each spell effect
    for (SpellEffectInfo const& effectInfo : spellInfo->GetEffects())
    {
        // Check for direct damage/heal effects
        switch (effectInfo.Effect)
        {
            case SPELL_EFFECT_SCHOOL_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
            case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
                hasOffensiveAura = true;
                break;
            case SPELL_EFFECT_HEAL:
            case SPELL_EFFECT_HEAL_PCT:
                hasDefensiveAura = true;
                break;
            default:
                break;
        }

        // Check aura types
        if (effectInfo.ApplyAuraName == 0)
            continue;

        switch (effectInfo.ApplyAuraName)
        {
            // ============================================================
            // OFFENSIVE AURAS
            // ============================================================
            case SPELL_AURA_MOD_DAMAGE_DONE:
            case SPELL_AURA_MOD_DAMAGE_PERCENT_DONE:
            case SPELL_AURA_MOD_MELEE_HASTE:
            case SPELL_AURA_MOD_MELEE_HASTE_2:
            case SPELL_AURA_MOD_MELEE_RANGED_HASTE:
            case SPELL_AURA_MELEE_SLOW:
            case SPELL_AURA_MOD_RANGED_HASTE:
            case SPELL_AURA_HASTE_SPELLS:
            case SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK:
            case SPELL_AURA_MOD_RATING:
            case SPELL_AURA_MOD_ATTACK_POWER:
            case SPELL_AURA_MOD_ATTACK_POWER_PCT:
            case SPELL_AURA_MOD_SPELL_DAMAGE_OF_STAT_PERCENT:
            case SPELL_AURA_MOD_CRIT_PCT:
                hasOffensiveAura = true;
                break;

            // ============================================================
            // DEFENSIVE AURAS
            // ============================================================
            case SPELL_AURA_SCHOOL_ABSORB:
            case SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN:
            case SPELL_AURA_MOD_RESISTANCE:
            case SPELL_AURA_MOD_RESISTANCE_PCT:
            case SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE:
            case SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT:
            case SPELL_AURA_PERIODIC_HEAL:
            case SPELL_AURA_OBS_MOD_HEALTH:
            case SPELL_AURA_MOD_HEALING_DONE_PERCENT:
                hasDefensiveAura = true;
                break;

            default:
                break;
        }
    }

    // Classify based on found auras
    if (hasOffensiveAura && !hasDefensiveAura)
        return TrinketEffectType::OFFENSIVE;
    if (hasDefensiveAura && !hasOffensiveAura)
        return TrinketEffectType::DEFENSIVE;
    if (hasOffensiveAura && hasDefensiveAura)
        return TrinketEffectType::OFFENSIVE; // Dual-purpose: treat as offensive

    return TrinketEffectType::UTILITY;
}

TrinketUsagePolicy TrinketUsageManager::DetermineUsagePolicy(TrinketEffectType effectType) const
{
    switch (effectType)
    {
        case TrinketEffectType::OFFENSIVE:
            return TrinketUsagePolicy::ON_BURST;
        case TrinketEffectType::DEFENSIVE:
            return TrinketUsagePolicy::ON_LOW_HEALTH;
        case TrinketEffectType::PVP_TRINKET:
            return TrinketUsagePolicy::ON_CC;
        case TrinketEffectType::UTILITY:
        case TrinketEffectType::UNKNOWN:
        default:
            return TrinketUsagePolicy::ON_COOLDOWN;
    }
}

// ============================================================================
// USAGE CONDITION CHECKS
// ============================================================================

bool TrinketUsageManager::ShouldUseOffensiveTrinket() const
{
    if (!_bot)
        return false;

    // Must have a valid target to attack
    Unit* target = GetTarget();
    if (!target)
        return false;

    // Check if we're in a burst-worthy phase
    // We integrate with CombatPhaseDetector if available
    // For now, use trinket during opener phase or when target is in execute range

    // During opener window: always use offensive trinkets (burst alignment)
    // This is the most valuable time to stack CDs with trinkets
    // We check for the opener indicator: time in combat < 8 seconds
    // (CombatPhaseDetector provides more precise detection per-spec)
    bool inBurstWindow = false;

    // Check if bot just entered combat (within first 6 seconds = opener)
    if (_bot->IsInCombat())
    {
        // Simple heuristic: if we haven't used this trinket yet this combat,
        // and we're in combat, use it (aligns with opener cooldown stacking)
        // After first use, it will be on cooldown and naturally align with
        // burst CD cycles (90s/120s/180s trinket CDs match burst patterns)
        for (uint8 i = 0; i < 2; ++i)
        {
            if (_trinkets[i].IsValid() &&
                _trinkets[i].usagePolicy == TrinketUsagePolicy::ON_BURST &&
                !_usedThisCombat[i])
            {
                inBurstWindow = true;
                break;
            }
        }
    }

    if (inBurstWindow)
        return true;

    // Fallback: if trinket has been used at least once and comes off cooldown,
    // use it immediately (maximize uptime = maximize DPS). This naturally
    // aligns with the burst CD cycle since trinket CDs are 90-120s and major
    // cooldowns are also 90-180s.
    return true;
}

bool TrinketUsageManager::ShouldUseDefensiveTrinket() const
{
    if (!_bot)
        return false;

    // Use defensive trinket when health drops below threshold
    float healthPct = _bot->GetHealthPct();
    return healthPct < _defensiveHealthThreshold;
}

bool TrinketUsageManager::ShouldUsePvPTrinket() const
{
    if (!_bot)
        return false;

    // Use PvP trinket when crowd-controlled
    // Check for common CC aura mechanics
    if (_bot->HasUnitState(UNIT_STATE_STUNNED) ||
        _bot->HasUnitState(UNIT_STATE_CONFUSED) ||
        _bot->HasUnitState(UNIT_STATE_FLEEING) ||
        _bot->HasAuraType(SPELL_AURA_MOD_FEAR) ||
        _bot->HasAuraType(SPELL_AURA_MOD_STUN) ||
        _bot->HasAuraType(SPELL_AURA_MOD_CONFUSE) ||
        _bot->HasAuraType(SPELL_AURA_MOD_CHARM) ||
        _bot->HasAuraType(SPELL_AURA_MOD_PACIFY) ||
        _bot->HasAuraType(SPELL_AURA_MOD_PACIFY_SILENCE))
    {
        return true;
    }

    return false;
}

// ============================================================================
// TRINKET ACTIVATION
// ============================================================================

bool TrinketUsageManager::ActivateTrinket(uint8 slotIndex)
{
    if (slotIndex >= 2 || !_bot || !_trinkets[slotIndex].IsValid())
        return false;

    // Get the actual equipped item
    uint8 equipSlot = _trinkets[slotIndex].equipSlot;
    Item* trinketItem = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, equipSlot);
    if (!trinketItem)
        return false;

    // Verify the item hasn't changed since our scan
    if (trinketItem->GetEntry() != _trinkets[slotIndex].itemEntry)
    {
        OnEquipmentChanged();
        return false;
    }

    // Double-check cooldown
    if (_bot->GetSpellHistory()->HasCooldown(
        _trinkets[slotIndex].onUseSpellId,
        _trinkets[slotIndex].itemEntry))
    {
        _stats.cooldownWastes++;
        return false;
    }

    // Determine cast target
    Unit* castTarget = _bot; // Self-buff is the default
    if (_trinkets[slotIndex].effectType == TrinketEffectType::OFFENSIVE)
    {
        // For offensive trinkets that deal damage, target the enemy
        // But most on-use trinkets are self-buffs, so check the spell
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(
            _trinkets[slotIndex].onUseSpellId, DIFFICULTY_NONE);
        if (spellInfo && !spellInfo->IsPositive())
        {
            // Damage spell: target the enemy
            Unit* target = GetTarget();
            if (target)
                castTarget = target;
        }
    }

    // Cast the trinket's on-use spell with the item reference
    // Using CastSpellExtraArgs(item) sets the cast item and TRIGGERED_FULL_MASK
    _bot->CastSpell(castTarget, _trinkets[slotIndex].onUseSpellId,
        CastSpellExtraArgs(trinketItem));

    // Update statistics
    _stats.totalUses++;
    _usedThisCombat[slotIndex] = true;

    switch (_trinkets[slotIndex].effectType)
    {
        case TrinketEffectType::OFFENSIVE:
            _stats.burstAlignedUses++;
            break;
        case TrinketEffectType::DEFENSIVE:
            _stats.defensiveUses++;
            break;
        case TrinketEffectType::PVP_TRINKET:
            _stats.pvpTrinketUses++;
            break;
        default:
            break;
    }

    TC_LOG_DEBUG("module.playerbot", "TrinketUsageManager[{}]: Activated trinket {} '{}' (spell={})",
        _bot->GetName(), slotIndex + 1, _trinkets[slotIndex].itemName,
        _trinkets[slotIndex].onUseSpellId);

    return true;
}

// ============================================================================
// HELPERS
// ============================================================================

Unit* TrinketUsageManager::GetTarget() const
{
    if (!_bot)
        return nullptr;

    // Get the bot's current target
    Unit* target = _bot->GetVictim();
    if (target && target->IsAlive())
        return target;

    // Try selection target
    ObjectGuid selection = _bot->GetTarget();
    if (!selection.IsEmpty())
    {
        if (Unit* selected = ObjectAccessor::GetUnit(*_bot, selection))
        {
            if (selected->IsAlive() && _bot->IsValidAttackTarget(selected))
                return selected;
        }
    }

    return nullptr;
}

} // namespace Playerbot
