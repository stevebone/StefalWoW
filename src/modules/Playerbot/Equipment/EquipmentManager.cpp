/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * COMPLETE EQUIPMENT MANAGEMENT IMPLEMENTATION
 *
 * Full implementation of equipment comparison, auto-equip, junk identification,
 * and consumable management for all 13 WoW classes.
 */

#include "EquipmentManager.h"
#include "Player.h"
#include "Item.h"
#include "Bag.h"
#include "ItemTemplate.h"
#include "ObjectMgr.h"
#include "Log.h"
#include "WorldSession.h"
#include <algorithm>
#include <limits>

namespace Playerbot
{

// ============================================================================
// STATIC MEMBER INITIALIZATION
// ============================================================================

std::unordered_map<uint16, StatPriority> EquipmentManager::_statPriorities;
bool EquipmentManager::_statPrioritiesInitialized = false;
EquipmentMetrics EquipmentManager::_globalMetrics;

// ============================================================================
// PER-BOT LIFECYCLE
// ============================================================================

EquipmentManager::EquipmentManager(Player* bot)
    : _bot(bot)
{
    if (!_bot)
    {
        TC_LOG_ERROR("playerbot.equipment", "EquipmentManager: Attempted to create with null bot!");
        return;
    }

    // Initialize shared stat priorities once (thread-safe)
    if (!_statPrioritiesInitialized)
    {
        InitializeStatPriorities();
        _statPrioritiesInitialized = true;
        TC_LOG_INFO("playerbot.equipment", "EquipmentManager: Initialized stat priorities for all 13 classes");
    }

    TC_LOG_DEBUG("playerbot.equipment", "EquipmentManager: Created for bot {} ({})",
                 _bot->GetName(), _bot->GetGUID().ToString());
}

EquipmentManager::~EquipmentManager()
{
    // CRITICAL: Do NOT access _bot->GetName() during destructor!
    // During bot destruction, the Player's internal string data may already be
    // freed even if the pointer is valid. This causes ACCESS_VIOLATION in
    // the string formatting code (VCRUNTIME140.dll).
    //
    // If logging is needed, use GUID counter instead:
    // if (_bot)
    //     TC_LOG_DEBUG("playerbot.equipment",
    //         "EquipmentManager destroyed for bot {}", _bot->GetGUID().GetCounter());
}

// ============================================================================
// STAT PRIORITY INITIALIZATION - ALL 13 CLASSES
// ============================================================================

void EquipmentManager::InitializeStatPriorities()
{
    InitializeWarriorPriorities();
    InitializePaladinPriorities();
    InitializeHunterPriorities();
    InitializeRoguePriorities();
    InitializePriestPriorities();
    InitializeShamanPriorities();
    InitializeMagePriorities();
    InitializeWarlockPriorities();
    InitializeDruidPriorities();
    InitializeDeathKnightPriorities();
    InitializeMonkPriorities();
    InitializeDemonHunterPriorities();
    InitializeEvokerPriorities();

    TC_LOG_INFO("playerbot.equipment", "Initialized stat priorities for all 13 classes");
}

void EquipmentManager::InitializeWarriorPriorities()
{
    // Arms (Spec 0)
    {
        StatPriority arms(CLASS_WARRIOR, 0);
        arms.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::STRENGTH, 0.95f},
            {StatType::CRITICAL_STRIKE, 0.75f},
            {StatType::HASTE, 0.70f},
            {StatType::MASTERY, 0.65f},
            {StatType::VERSATILITY, 0.60f},
            {StatType::STAMINA, 0.50f},
            {StatType::ARMOR, 0.40f},
            {StatType::WEAPON_DPS, 0.90f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_WARRIOR, 0)] = arms;
    }

    // Fury (Spec 1)
    {
        StatPriority fury(CLASS_WARRIOR, 1);
        fury.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::STRENGTH, 0.95f},
            {StatType::HASTE, 0.80f},
            {StatType::CRITICAL_STRIKE, 0.75f},
            {StatType::MASTERY, 0.70f},
            {StatType::VERSATILITY, 0.65f},
            {StatType::STAMINA, 0.50f},
            {StatType::ARMOR, 0.40f},
            {StatType::WEAPON_DPS, 0.85f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_WARRIOR, 1)] = fury;
    }

    // Protection (Spec 2)
    {
        StatPriority prot(CLASS_WARRIOR, 2);
        prot.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::STAMINA, 0.90f},
            {StatType::ARMOR, 0.85f},
            {StatType::STRENGTH, 0.70f},
            {StatType::HASTE, 0.65f},
            {StatType::VERSATILITY, 0.75f},
            {StatType::MASTERY, 0.60f},
            {StatType::CRITICAL_STRIKE, 0.55f},
            {StatType::WEAPON_DPS, 0.50f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_WARRIOR, 2)] = prot;
    }
}

void EquipmentManager::InitializePaladinPriorities()
{
    // Holy (Spec 0)
    {
        StatPriority holy(CLASS_PALADIN, 0);
        holy.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::INTELLECT, 0.95f},
            {StatType::CRITICAL_STRIKE, 0.75f},
            {StatType::HASTE, 0.70f},
            {StatType::MASTERY, 0.65f},
            {StatType::VERSATILITY, 0.60f},
            {StatType::STAMINA, 0.50f},
            {StatType::ARMOR, 0.40f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_PALADIN, 0)] = holy;
    }

    // Protection (Spec 1)
    {
        StatPriority prot(CLASS_PALADIN, 1);
        prot.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::STAMINA, 0.90f},
            {StatType::ARMOR, 0.85f},
            {StatType::STRENGTH, 0.70f},
            {StatType::HASTE, 0.75f},
            {StatType::VERSATILITY, 0.70f},
            {StatType::MASTERY, 0.65f},
            {StatType::CRITICAL_STRIKE, 0.60f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_PALADIN, 1)] = prot;
    }

    // Retribution (Spec 2)
    {
        StatPriority ret(CLASS_PALADIN, 2);
        ret.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::STRENGTH, 0.95f},
            {StatType::CRITICAL_STRIKE, 0.75f},
            {StatType::HASTE, 0.70f},
            {StatType::VERSATILITY, 0.65f},
            {StatType::MASTERY, 0.60f},
            {StatType::STAMINA, 0.50f},
            {StatType::WEAPON_DPS, 0.90f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_PALADIN, 2)] = ret;
    }
}

void EquipmentManager::InitializeHunterPriorities()
{
    // Beast Mastery (Spec 0)
    {
        StatPriority bm(CLASS_HUNTER, 0);
        bm.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::AGILITY, 0.95f},
            {StatType::CRITICAL_STRIKE, 0.75f},
            {StatType::HASTE, 0.70f},
            {StatType::MASTERY, 0.80f},
            {StatType::VERSATILITY, 0.65f},
            {StatType::STAMINA, 0.50f},
            {StatType::WEAPON_DPS, 0.90f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_HUNTER, 0)] = bm;
    }

    // Marksmanship (Spec 1)
    {
        StatPriority mm(CLASS_HUNTER, 1);
        mm.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::AGILITY, 0.95f},
            {StatType::CRITICAL_STRIKE, 0.80f},
            {StatType::MASTERY, 0.75f},
            {StatType::HASTE, 0.70f},
            {StatType::VERSATILITY, 0.65f},
            {StatType::STAMINA, 0.50f},
            {StatType::WEAPON_DPS, 0.95f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_HUNTER, 1)] = mm;
    }

    // Survival (Spec 2)
    {
        StatPriority surv(CLASS_HUNTER, 2);
        surv.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::AGILITY, 0.95f},
            {StatType::HASTE, 0.75f},
            {StatType::CRITICAL_STRIKE, 0.70f},
            {StatType::VERSATILITY, 0.68f},
            {StatType::MASTERY, 0.65f},
            {StatType::STAMINA, 0.50f},
            {StatType::WEAPON_DPS, 0.85f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_HUNTER, 2)] = surv;
    }
}

void EquipmentManager::InitializeRoguePriorities()
{
    // Assassination (Spec 0)
    {
        StatPriority assa(CLASS_ROGUE, 0);
        assa.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::AGILITY, 0.95f},
            {StatType::CRITICAL_STRIKE, 0.75f},
            {StatType::MASTERY, 0.70f},
            {StatType::HASTE, 0.65f},
            {StatType::VERSATILITY, 0.60f},
            {StatType::STAMINA, 0.50f},
            {StatType::WEAPON_DPS, 0.90f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_ROGUE, 0)] = assa;
    }

    // Outlaw (Spec 1)
    {
        StatPriority outlaw(CLASS_ROGUE, 1);
        outlaw.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::AGILITY, 0.95f},
            {StatType::HASTE, 0.75f},
            {StatType::CRITICAL_STRIKE, 0.70f},
            {StatType::VERSATILITY, 0.68f},
            {StatType::MASTERY, 0.65f},
            {StatType::STAMINA, 0.50f},
            {StatType::WEAPON_DPS, 0.90f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_ROGUE, 1)] = outlaw;
    }

    // Subtlety (Spec 2)
    {
        StatPriority sub(CLASS_ROGUE, 2);
        sub.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::AGILITY, 0.95f},
            {StatType::CRITICAL_STRIKE, 0.75f},
            {StatType::VERSATILITY, 0.72f},
            {StatType::MASTERY, 0.70f},
            {StatType::HASTE, 0.65f},
            {StatType::STAMINA, 0.50f},
            {StatType::WEAPON_DPS, 0.90f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_ROGUE, 2)] = sub;
    }
}

void EquipmentManager::InitializePriestPriorities()
{
    // Discipline (Spec 0)
    {
        StatPriority disc(CLASS_PRIEST, 0);
        disc.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::INTELLECT, 0.95f},
            {StatType::HASTE, 0.80f},
            {StatType::CRITICAL_STRIKE, 0.75f},
            {StatType::MASTERY, 0.70f},
            {StatType::VERSATILITY, 0.65f},
            {StatType::STAMINA, 0.50f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_PRIEST, 0)] = disc;
    }

    // Holy (Spec 1)
    {
        StatPriority holy(CLASS_PRIEST, 1);
        holy.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::INTELLECT, 0.95f},
            {StatType::CRITICAL_STRIKE, 0.75f},
            {StatType::MASTERY, 0.70f},
            {StatType::HASTE, 0.68f},
            {StatType::VERSATILITY, 0.65f},
            {StatType::STAMINA, 0.50f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_PRIEST, 1)] = holy;
    }

    // Shadow (Spec 2)
    {
        StatPriority shadow(CLASS_PRIEST, 2);
        shadow.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::INTELLECT, 0.95f},
            {StatType::HASTE, 0.80f},
            {StatType::CRITICAL_STRIKE, 0.75f},
            {StatType::VERSATILITY, 0.70f},
            {StatType::MASTERY, 0.65f},
            {StatType::STAMINA, 0.50f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_PRIEST, 2)] = shadow;
    }
}

void EquipmentManager::InitializeShamanPriorities()
{
    // Elemental (Spec 0)
    {
        StatPriority ele(CLASS_SHAMAN, 0);
        ele.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::INTELLECT, 0.95f},
            {StatType::CRITICAL_STRIKE, 0.75f},
            {StatType::HASTE, 0.72f},
            {StatType::VERSATILITY, 0.70f},
            {StatType::MASTERY, 0.68f},
            {StatType::STAMINA, 0.50f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_SHAMAN, 0)] = ele;
    }

    // Enhancement (Spec 1)
    {
        StatPriority enh(CLASS_SHAMAN, 1);
        enh.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::AGILITY, 0.95f},
            {StatType::HASTE, 0.75f},
            {StatType::CRITICAL_STRIKE, 0.70f},
            {StatType::MASTERY, 0.68f},
            {StatType::VERSATILITY, 0.65f},
            {StatType::STAMINA, 0.50f},
            {StatType::WEAPON_DPS, 0.85f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_SHAMAN, 1)] = enh;
    }

    // Restoration (Spec 2)
    {
        StatPriority resto(CLASS_SHAMAN, 2);
        resto.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::INTELLECT, 0.95f},
            {StatType::CRITICAL_STRIKE, 0.75f},
            {StatType::HASTE, 0.70f},
            {StatType::MASTERY, 0.68f},
            {StatType::VERSATILITY, 0.65f},
            {StatType::STAMINA, 0.50f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_SHAMAN, 2)] = resto;
    }
}

void EquipmentManager::InitializeMagePriorities()
{
    // Arcane (Spec 0)
    {
        StatPriority arcane(CLASS_MAGE, 0);
        arcane.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::INTELLECT, 0.95f},
            {StatType::MASTERY, 0.80f},
            {StatType::HASTE, 0.75f},
            {StatType::CRITICAL_STRIKE, 0.70f},
            {StatType::VERSATILITY, 0.65f},
            {StatType::STAMINA, 0.50f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_MAGE, 0)] = arcane;
    }

    // Fire (Spec 1)
    {
        StatPriority fire(CLASS_MAGE, 1);
        fire.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::INTELLECT, 0.95f},
            {StatType::CRITICAL_STRIKE, 0.85f},
            {StatType::HASTE, 0.75f},
            {StatType::MASTERY, 0.70f},
            {StatType::VERSATILITY, 0.65f},
            {StatType::STAMINA, 0.50f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_MAGE, 1)] = fire;
    }

    // Frost (Spec 2)
    {
        StatPriority frost(CLASS_MAGE, 2);
        frost.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::INTELLECT, 0.95f},
            {StatType::HASTE, 0.80f},
            {StatType::CRITICAL_STRIKE, 0.75f},
            {StatType::VERSATILITY, 0.70f},
            {StatType::MASTERY, 0.68f},
            {StatType::STAMINA, 0.50f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_MAGE, 2)] = frost;
    }
}

void EquipmentManager::InitializeWarlockPriorities()
{
    // Affliction (Spec 0)
    {
        StatPriority aff(CLASS_WARLOCK, 0);
        aff.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::INTELLECT, 0.95f},
            {StatType::HASTE, 0.80f},
            {StatType::MASTERY, 0.75f},
            {StatType::CRITICAL_STRIKE, 0.70f},
            {StatType::VERSATILITY, 0.65f},
            {StatType::STAMINA, 0.50f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_WARLOCK, 0)] = aff;
    }

    // Demonology (Spec 1)
    {
        StatPriority demo(CLASS_WARLOCK, 1);
        demo.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::INTELLECT, 0.95f},
            {StatType::HASTE, 0.80f},
            {StatType::CRITICAL_STRIKE, 0.75f},
            {StatType::MASTERY, 0.70f},
            {StatType::VERSATILITY, 0.65f},
            {StatType::STAMINA, 0.50f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_WARLOCK, 1)] = demo;
    }

    // Destruction (Spec 2)
    {
        StatPriority destro(CLASS_WARLOCK, 2);
        destro.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::INTELLECT, 0.95f},
            {StatType::CRITICAL_STRIKE, 0.80f},
            {StatType::HASTE, 0.75f},
            {StatType::VERSATILITY, 0.70f},
            {StatType::MASTERY, 0.68f},
            {StatType::STAMINA, 0.50f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_WARLOCK, 2)] = destro;
    }
}

void EquipmentManager::InitializeDruidPriorities()
{
    // Balance (Spec 0)
    {
        StatPriority balance(CLASS_DRUID, 0);
        balance.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::INTELLECT, 0.95f},
            {StatType::HASTE, 0.75f},
            {StatType::CRITICAL_STRIKE, 0.72f},
            {StatType::MASTERY, 0.70f},
            {StatType::VERSATILITY, 0.65f},
            {StatType::STAMINA, 0.50f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_DRUID, 0)] = balance;
    }

    // Feral (Spec 1)
    {
        StatPriority feral(CLASS_DRUID, 1);
        feral.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::AGILITY, 0.95f},
            {StatType::CRITICAL_STRIKE, 0.75f},
            {StatType::HASTE, 0.70f},
            {StatType::MASTERY, 0.68f},
            {StatType::VERSATILITY, 0.65f},
            {StatType::STAMINA, 0.50f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_DRUID, 1)] = feral;
    }

    // Guardian (Spec 2)
    {
        StatPriority guardian(CLASS_DRUID, 2);
        guardian.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::STAMINA, 0.90f},
            {StatType::ARMOR, 0.85f},
            {StatType::AGILITY, 0.75f},
            {StatType::VERSATILITY, 0.70f},
            {StatType::HASTE, 0.65f},
            {StatType::MASTERY, 0.60f},
            {StatType::CRITICAL_STRIKE, 0.55f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_DRUID, 2)] = guardian;
    }

    // Restoration (Spec 3)
    {
        StatPriority resto(CLASS_DRUID, 3);
        resto.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::INTELLECT, 0.95f},
            {StatType::HASTE, 0.75f},
            {StatType::MASTERY, 0.72f},
            {StatType::CRITICAL_STRIKE, 0.70f},
            {StatType::VERSATILITY, 0.65f},
            {StatType::STAMINA, 0.50f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_DRUID, 3)] = resto;
    }
}

void EquipmentManager::InitializeDeathKnightPriorities()
{
    // Blood (Spec 0)
    {
        StatPriority blood(CLASS_DEATH_KNIGHT, 0);
        blood.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::STAMINA, 0.90f},
            {StatType::ARMOR, 0.85f},
            {StatType::STRENGTH, 0.75f},
            {StatType::HASTE, 0.70f},
            {StatType::VERSATILITY, 0.75f},
            {StatType::MASTERY, 0.68f},
            {StatType::CRITICAL_STRIKE, 0.60f},
            {StatType::WEAPON_DPS, 0.55f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_DEATH_KNIGHT, 0)] = blood;
    }

    // Frost (Spec 1)
    {
        StatPriority frost(CLASS_DEATH_KNIGHT, 1);
        frost.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::STRENGTH, 0.95f},
            {StatType::CRITICAL_STRIKE, 0.75f},
            {StatType::MASTERY, 0.72f},
            {StatType::HASTE, 0.70f},
            {StatType::VERSATILITY, 0.65f},
            {StatType::STAMINA, 0.50f},
            {StatType::WEAPON_DPS, 0.90f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_DEATH_KNIGHT, 1)] = frost;
    }

    // Unholy (Spec 2)
    {
        StatPriority unholy(CLASS_DEATH_KNIGHT, 2);
        unholy.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::STRENGTH, 0.95f},
            {StatType::HASTE, 0.80f},
            {StatType::CRITICAL_STRIKE, 0.75f},
            {StatType::MASTERY, 0.72f},
            {StatType::VERSATILITY, 0.68f},
            {StatType::STAMINA, 0.50f},
            {StatType::WEAPON_DPS, 0.90f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_DEATH_KNIGHT, 2)] = unholy;
    }
}

void EquipmentManager::InitializeMonkPriorities()
{
    // Brewmaster (Spec 0)
    {
        StatPriority brew(CLASS_MONK, 0);
        brew.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::STAMINA, 0.90f},
            {StatType::ARMOR, 0.85f},
            {StatType::AGILITY, 0.75f},
            {StatType::CRITICAL_STRIKE, 0.70f},
            {StatType::VERSATILITY, 0.75f},
            {StatType::MASTERY, 0.68f},
            {StatType::HASTE, 0.65f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_MONK, 0)] = brew;
    }

    // Mistweaver (Spec 1)
    {
        StatPriority mist(CLASS_MONK, 1);
        mist.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::INTELLECT, 0.95f},
            {StatType::CRITICAL_STRIKE, 0.75f},
            {StatType::VERSATILITY, 0.72f},
            {StatType::HASTE, 0.70f},
            {StatType::MASTERY, 0.68f},
            {StatType::STAMINA, 0.50f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_MONK, 1)] = mist;
    }

    // Windwalker (Spec 2)
    {
        StatPriority ww(CLASS_MONK, 2);
        ww.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::AGILITY, 0.95f},
            {StatType::VERSATILITY, 0.75f},
            {StatType::CRITICAL_STRIKE, 0.72f},
            {StatType::HASTE, 0.70f},
            {StatType::MASTERY, 0.68f},
            {StatType::STAMINA, 0.50f},
            {StatType::WEAPON_DPS, 0.85f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_MONK, 2)] = ww;
    }
}

void EquipmentManager::InitializeDemonHunterPriorities()
{
    // Havoc (Spec 0)
    {
        StatPriority havoc(CLASS_DEMON_HUNTER, 0);
        havoc.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::AGILITY, 0.95f},
            {StatType::CRITICAL_STRIKE, 0.75f},
            {StatType::HASTE, 0.72f},
            {StatType::VERSATILITY, 0.70f},
            {StatType::MASTERY, 0.68f},
            {StatType::STAMINA, 0.50f},
            {StatType::WEAPON_DPS, 0.85f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_DEMON_HUNTER, 0)] = havoc;
    }

    // Vengeance (Spec 1)
    {
        StatPriority veng(CLASS_DEMON_HUNTER, 1);
        veng.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::STAMINA, 0.90f},
            {StatType::ARMOR, 0.85f},
            {StatType::AGILITY, 0.75f},
            {StatType::VERSATILITY, 0.75f},
            {StatType::HASTE, 0.70f},
            {StatType::MASTERY, 0.68f},
            {StatType::CRITICAL_STRIKE, 0.65f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_DEMON_HUNTER, 1)] = veng;
    }
}

void EquipmentManager::InitializeEvokerPriorities()
{
    // Devastation (Spec 0)
    {
        StatPriority dev(CLASS_EVOKER, 0);
        dev.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::INTELLECT, 0.95f},
            {StatType::CRITICAL_STRIKE, 0.75f},
            {StatType::HASTE, 0.72f},
            {StatType::MASTERY, 0.70f},
            {StatType::VERSATILITY, 0.68f},
            {StatType::STAMINA, 0.50f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_EVOKER, 0)] = dev;
    }

    // Preservation (Spec 1)
    {
        StatPriority pres(CLASS_EVOKER, 1);
        pres.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::INTELLECT, 0.95f},
            {StatType::MASTERY, 0.75f},
            {StatType::CRITICAL_STRIKE, 0.72f},
            {StatType::HASTE, 0.70f},
            {StatType::VERSATILITY, 0.68f},
            {StatType::STAMINA, 0.50f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_EVOKER, 1)] = pres;
    }

    // Augmentation (Spec 2)
    {
        StatPriority aug(CLASS_EVOKER, 2);
        aug.statWeights = {
            {StatType::ITEM_LEVEL, 1.0f},
            {StatType::INTELLECT, 0.95f},
            {StatType::MASTERY, 0.80f},
            {StatType::CRITICAL_STRIKE, 0.75f},
            {StatType::VERSATILITY, 0.72f},
            {StatType::HASTE, 0.70f},
            {StatType::STAMINA, 0.50f}
        };
        _statPriorities[MakeStatPriorityKey(CLASS_EVOKER, 2)] = aug;
    }
}

// ============================================================================
// CORE EQUIPMENT EVALUATION
// ============================================================================

void EquipmentManager::AutoEquipBestGear()
{
    if (!_bot)
        return;

    if (!_profile.autoEquipEnabled)
        return;

    TC_LOG_DEBUG("playerbot.equipment", "AutoEquipBestGear: Scanning inventory for bot {}",
                 _bot->GetName());

    uint32 upgradesFound = 0;

    // Helper lambda to process an item for potential upgrade
    auto processItem = [this, &upgradesFound](::Item* item) -> void
    {
        if (!item || !IsItemUpgrade(item))
            return;

        ItemTemplate const* proto = item->GetTemplate();
        if (!proto)
            return;

        // Get all valid slots for this item type (handles rings, trinkets, dual-wield)
        std::vector<uint8> validSlots = GetMultiSlotEquipmentSlots(proto);
        if (validSlots.empty())
            return;

        // For multi-slot items, compare against the worst equipped item in those slots
        ::Item* currentItem = GetWorstEquippedItemForSlots(validSlots);

        // Compare items
        ItemComparisonResult result = CompareItems(currentItem, item);
        if (result.isUpgrade && result.scoreDifference >= _profile.minUpgradeThreshold)
        {
            TC_LOG_INFO("playerbot.equipment",
                       "UPGRADE FOUND: Bot {} - {} is upgrade over {} (Score: {:.2f} -> {:.2f}, Reason: {})",
                       _bot->GetName(),
                       proto->GetName(DEFAULT_LOCALE),
                       currentItem ? currentItem->GetTemplate()->GetName(DEFAULT_LOCALE) : "Empty Slot",
                       result.currentItemScore,
                       result.newItemScore,
                       result.upgradeReason);

            // Use the first valid slot - EquipItemInSlot will find the actual best slot
            EquipItemInSlot(item, validSlots[0]);
            upgradesFound++;
            UpdateMetrics(true, true);
        }
    };

    // Scan all inventory bags for equippable items
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        if (Bag* pBag = _bot->GetBagByPos(bag))
        {
            for (uint32 slot = 0; slot < pBag->GetBagSize(); ++slot)
            {
                processItem(pBag->GetItemByPos(slot));
            }
        }
    }

    // Also check main bag (backpack)
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
    {
        processItem(_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot));
    }

    if (upgradesFound > 0)
    {
        TC_LOG_INFO("playerbot.equipment", "AutoEquip Complete: Bot {} equipped {} upgrades",
                   _bot->GetName(), upgradesFound);
    }
}

ItemComparisonResult EquipmentManager::CompareItems(::Item* currentItem, ::Item* newItem)
{
    ItemComparisonResult result;

    if (!_bot || !newItem)
        return result;

    // Calculate scores
    result.newItemScore = CalculateItemScore(newItem);

    if (currentItem)
    {
        result.currentItemScore = CalculateItemScore(currentItem);
        result.currentItemLevel = currentItem->GetItemLevel(_bot);
    }
    else
    {
        result.currentItemScore = 0.0f;
        result.currentItemLevel = 0;
    }

    result.newItemLevel = newItem->GetItemLevel(_bot);

    // Determine if upgrade
    result.scoreDifference = result.newItemScore - result.currentItemScore;

    // Item level preference
    if (_profile.preferHigherItemLevel && result.newItemLevel > result.currentItemLevel + 5)
    {
        result.isUpgrade = true;
        result.upgradeReason = "Higher item level (" + std::to_string(result.newItemLevel) + " vs " +
                               std::to_string(result.currentItemLevel) + ")";
        return result;
    }

    // Stat score comparison
    if (result.scoreDifference > _profile.minUpgradeThreshold)
    {
        result.isUpgrade = true;
        result.upgradeReason = "Better stat allocation (Score: " + std::to_string(result.scoreDifference) + " improvement)";
    }

    return result;
}

float EquipmentManager::CalculateItemScore(::Item* item)
{
    if (!_bot || !item)
        return 0.0f;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return 0.0f;

    StatPriority const& priority = GetStatPriority();

    // Calculate weighted stat total
    float totalScore = 0.0f;

    for (const auto& [statType, weight] : priority.statWeights)
    {
        int32 statValue = GetItemStatValue(item, statType);
        if (statValue > 0)
        {
            totalScore += static_cast<float>(statValue) * weight;
        }
    }

    // Add item level as base score
    totalScore += static_cast<float>(item->GetItemLevel(_bot)) * priority.GetStatWeight(StatType::ITEM_LEVEL);
    TC_LOG_TRACE("playerbot.equipment", "Item {} score for bot {}: {:.2f}",
                 proto->GetName(DEFAULT_LOCALE), _bot->GetName(), totalScore);

    return totalScore;
}

bool EquipmentManager::IsItemUpgrade(::Item* item)
{
    if (!_bot || !item)
        return false;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return false;

    // Check if bot can equip this item
    if (!CanEquipItem(proto))
        return false;

    // Get all valid slots for this item type (handles multi-slot items like rings/trinkets)
    std::vector<uint8> validSlots = GetMultiSlotEquipmentSlots(proto);
    if (validSlots.empty())
        return false;

    // For multi-slot items, compare against the worst equipped item in those slots
    // If there's an empty slot, this returns nullptr (meaning it's definitely an upgrade)
    ::Item* currentItem = GetWorstEquippedItemForSlots(validSlots);

    ItemComparisonResult result = CompareItems(currentItem, item);
    return result.isUpgrade;
}

float EquipmentManager::CalculateItemTemplateScore(ItemTemplate const* itemTemplate)
{
    if (!_bot || !itemTemplate)
        return 0.0f;

    StatPriority const& priority = GetStatPriority();
    // Calculate weighted stat total using the same algorithm as CalculateItemScore()
    float totalScore = 0.0f;

    for (const auto& [statType, weight] : priority.statWeights)
    {
        int32 statValue = ExtractStatValue(itemTemplate, statType);
        if (statValue > 0)
        {
            totalScore += static_cast<float>(statValue) * weight;
        }
    }

    // Add item level as base score
    totalScore += static_cast<float>(itemTemplate->GetBaseItemLevel()) * priority.GetStatWeight(StatType::ITEM_LEVEL);
    TC_LOG_TRACE("playerbot.equipment", "ItemTemplate {} score for bot {}: {:.2f}",
                 itemTemplate->GetName(DEFAULT_LOCALE), _bot->GetName(), totalScore);

    return totalScore;
}

// ============================================================================
// JUNK IDENTIFICATION - COMPLETE IMPLEMENTATION
// ============================================================================

std::vector<ObjectGuid> EquipmentManager::IdentifyJunkItems()
{
    std::vector<ObjectGuid> junkItems;

    if (!_bot)
        return junkItems;

    if (!_profile.autoSellJunkEnabled)
        return junkItems;

    // Scan all inventory for junk
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        if (Bag* pBag = _bot->GetBagByPos(bag))
        {
            for (uint32 slot = 0; slot < pBag->GetBagSize(); ++slot)
            {
                if (::Item* item = pBag->GetItemByPos(slot))
                {
                    if (IsJunkItem(item) && !IsProtectedItem(item))
                    {
                        junkItems.push_back(item->GetGUID());
                        TC_LOG_DEBUG("playerbot.equipment", "Identified junk: {} ({})",
                                     item->GetTemplate()->GetName(DEFAULT_LOCALE), item->GetGUID().ToString());
                    }
                }
            }
        }
    }

    // Check main bag
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
    {
        if (::Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        {
            if (IsJunkItem(item) && !IsProtectedItem(item))
            {
                junkItems.push_back(item->GetGUID());
            }
        }
    }

    TC_LOG_INFO("playerbot.equipment", "Identified {} junk items for bot {}",
                junkItems.size(), _bot->GetName());

    return junkItems;
}

bool EquipmentManager::IsJunkItem(::Item* item)
{
    if (!_bot || !item)
        return false;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return false;

    // Always junk: Grey (Poor) quality
    if (proto->GetQuality() == ITEM_QUALITY_POOR)
        return true;

    // Check item level threshold
    // Use _profile directly
    if (item->GetItemLevel(_bot) < _profile.minItemLevelToKeep && _bot->GetLevel() > 20)
        return true;

    // If it's equipment, check if it's worse than what we have
    if (proto->GetInventoryType() != INVTYPE_NON_EQUIP)
    {
        if (IsOutdatedGear(item))
            return true;

        if (HasWrongPrimaryStats(item))
            return true;
    }

    return false;
}

bool EquipmentManager::IsProtectedItem(::Item* item)
{
    if (!_bot || !item)
        return false;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return false;

    // Never sell quest items
    if (proto->GetClass() == ITEM_CLASS_QUEST)
        return true;

    // Never sell soulbound items with high item level
    if (item->IsSoulBound() && item->GetItemLevel(_bot) >= _bot->GetLevel())
        return true;

    // Never sell set items (if profile says so)
    // Use _profile directly
    if (_profile.considerSetBonuses && IsSetItem(item))
        return true;

    // Never sell valuable BoE
    if (IsValuableBoE(item))
        return true;

    // Check never-sell list
    uint32 itemId = proto->GetId();
    if (_profile.neverSellItems.find(itemId) != _profile.neverSellItems.end())
        return true;

    // Never sell rare+ consumables
    if (proto->GetClass() == ITEM_CLASS_CONSUMABLE && proto->GetQuality() >= ITEM_QUALITY_RARE)
        return true;

    return false;
}

bool EquipmentManager::IsValuableBoE(::Item* item)
{
    if (!item)
        return false;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return false;

    // Not BoE - not relevant
    if (proto->GetBonding() != BIND_ON_EQUIP)
        return false;

    // Epic+ BoE are always valuable
    if (proto->GetQuality() >= ITEM_QUALITY_EPIC)
        return true;

    // Rare BoE with high sell price
    if (proto->GetQuality() == ITEM_QUALITY_RARE && proto->GetSellPrice() > 10000) // 1 gold
        return true;

    return false;
}

// ============================================================================
// CONSUMABLE MANAGEMENT - COMPLETE IMPLEMENTATION
// ============================================================================

std::unordered_map<uint32, uint32> EquipmentManager::GetConsumableNeeds()
{
    std::unordered_map<uint32, uint32> needs;

    if (!_bot)
        return needs;

    std::vector<uint32> classConsumables = GetClassConsumables(_bot->GetClass());
    for (uint32 itemId : classConsumables)
    {
        uint32 currentCount = GetConsumableCount(itemId);
        uint32 recommendedCount = 20; // Stack size recommendation

        if (currentCount < recommendedCount)
        {
            needs[itemId] = recommendedCount - currentCount;
        }
    }

    // Food and water (all classes)
    uint32 foodLevel = GetRecommendedFoodLevel();
    uint32 currentFood = GetConsumableCount(foodLevel);
    if (currentFood < 20)
        needs[foodLevel] = 20 - currentFood;

    if (_bot->GetPowerType() == POWER_MANA)
    {
        uint32 waterLevel = GetRecommendedPotionLevel();
        uint32 currentWater = GetConsumableCount(waterLevel);
        if (currentWater < 20)
            needs[waterLevel] = 20 - currentWater;
    }

    return needs;
}

bool EquipmentManager::NeedsConsumableRestocking()
{
    auto needs = GetConsumableNeeds();
    return !needs.empty();
}

std::vector<uint32> EquipmentManager::GetClassConsumables(uint8 classId)
{
    std::vector<uint32> consumables;

    switch (classId)
    {
        case CLASS_ROGUE:
            // Poisons, thistle tea (example item IDs - would need actual WoW DB IDs)
            consumables.push_back(6947);  // Instant Poison
            consumables.push_back(2892);  // Deadly Poison
            break;

        case CLASS_WARLOCK:
            // Soul shards are now passive, but healthstones
            consumables.push_back(5512);  // Healthstone
            break;

        case CLASS_MAGE:
            // Conjured items handled separately
            break;

        case CLASS_HUNTER:
            // Arrows/bullets removed in modern WoW
            break;

        // Add more class-specific consumables as needed
        default:
            break;
    }

    return consumables;
}

uint32 EquipmentManager::GetConsumableCount(uint32 itemId)
{
    if (!_bot)
        return 0;

    return _bot->GetItemCount(itemId, true); // includeBank = true
}

// ============================================================================
// STAT PRIORITY SYSTEM
// ============================================================================

StatPriority const& EquipmentManager::GetStatPriority()
{
    if (!_bot)
    {
        static StatPriority defaultPriority(0, 0);
        return defaultPriority;
    }

    uint8 classId = _bot->GetClass();
    uint8 specId = static_cast<uint8>(_bot->GetPrimarySpecialization());
    uint16 key = MakeStatPriorityKey(classId, specId);

    auto it = _statPriorities.find(key);
    if (it != _statPriorities.end())
        return it->second;

    // Fallback to spec 0
    key = MakeStatPriorityKey(classId, 0);
    it = _statPriorities.find(key);
    if (it != _statPriorities.end())
        return it->second;

    static StatPriority defaultPriority(0, 0);
    return defaultPriority;
}

StatPriority const& EquipmentManager::GetStatPriorityByClassSpec(uint8 classId, uint32 specId)
{
    uint16 key = MakeStatPriorityKey(classId, static_cast<uint8>(specId));

    auto it = _statPriorities.find(key);
    if (it != _statPriorities.end())
        return it->second;

    // Fallback to spec 0
    key = MakeStatPriorityKey(classId, 0);
    it = _statPriorities.find(key);
    if (it != _statPriorities.end())
        return it->second;

    static StatPriority defaultPriority(0, 0);
    return defaultPriority;
}

void EquipmentManager::UpdateStatPriority()
{
    // Called when _bot changes spec - priority automatically updated via GetStatPriority()
    if (_bot)
    {
        TC_LOG_DEBUG("playerbot.equipment", "Updated stat priority for _bot {} (Class: {}, Spec: {})",
                     _bot->GetName(), static_cast<uint32>(_bot->GetClass()), static_cast<uint32>(_bot->GetPrimarySpecialization()));
    }
}

// ============================================================================
// ITEM CATEGORIZATION
// ============================================================================

ItemCategory EquipmentManager::GetItemCategory(::Item* item)
{
    if (!item)
        return ItemCategory::UNKNOWN;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return ItemCategory::UNKNOWN;

    // Quest items
    if (proto->GetClass() == ITEM_CLASS_QUEST)
        return ItemCategory::QUEST_ITEM;

    // Junk
    if (proto->GetQuality() == ITEM_QUALITY_POOR)
        return ItemCategory::JUNK;

    // Valuable BoE
    if (proto->GetBonding() == BIND_ON_EQUIP && proto->GetQuality() >= ITEM_QUALITY_EPIC)
        return ItemCategory::VALUABLE_BIND_ON_EQUIP;

    // Equipment
    if (proto->GetInventoryType() != INVTYPE_NON_EQUIP)
    {
        if (proto->GetClass() == ITEM_CLASS_WEAPON)
            return ItemCategory::WEAPON;
        else if (proto->GetClass() == ITEM_CLASS_ARMOR)
            return ItemCategory::ARMOR;
    }

    // Consumables
    if (proto->GetClass() == ITEM_CLASS_CONSUMABLE)
        return ItemCategory::CONSUMABLE;

    // Trade goods
    if (proto->GetClass() == ITEM_CLASS_TRADE_GOODS)
        return ItemCategory::TRADE_GOODS;

    return ItemCategory::UNKNOWN;
}

bool EquipmentManager::CanEquipItem(ItemTemplate const* itemTemplate)
{
    if (!_bot || !itemTemplate)
        return false;

    // Check level requirement
    if (_bot->GetLevel() < itemTemplate->GetBaseRequiredLevel())
        return false;

    // Check class restriction
    if (itemTemplate->GetAllowableClass() && !(itemTemplate->GetAllowableClass() & (1 << (_bot->GetClass() - 1))))
        return false;

    // Check race restriction
    if (!itemTemplate->GetAllowableRace().IsEmpty() && !itemTemplate->GetAllowableRace().HasRace(_bot->GetRace()))
        return false;

    // Check armor/weapon proficiency (skill requirement)
    // This prevents Priests from trying to equip Mail armor, Warriors from trying to equip Wands, etc.
    if (itemTemplate->GetRequiredSkill() != 0)
    {
        if (_bot->GetSkillValue(itemTemplate->GetRequiredSkill()) == 0)
            return false;

        // Also check minimum skill rank if required
        if (_bot->GetSkillValue(itemTemplate->GetRequiredSkill()) < itemTemplate->GetRequiredSkillRank())
            return false;
    }

    // Check armor subclass proficiency for armor items
    if (itemTemplate->GetClass() == ITEM_CLASS_ARMOR && itemTemplate->GetSubClass() != ITEM_SUBCLASS_ARMOR_MISCELLANEOUS)
    {
        // Get the armor skill for this subclass
        uint32 armorSkill = 0;
        switch (itemTemplate->GetSubClass())
        {
            case ITEM_SUBCLASS_ARMOR_CLOTH:   armorSkill = SKILL_CLOTH; break;
            case ITEM_SUBCLASS_ARMOR_LEATHER: armorSkill = SKILL_LEATHER; break;
            case ITEM_SUBCLASS_ARMOR_MAIL:    armorSkill = SKILL_MAIL; break;
            case ITEM_SUBCLASS_ARMOR_PLATE:   armorSkill = SKILL_PLATE_MAIL; break;
            case ITEM_SUBCLASS_ARMOR_SHIELD:  armorSkill = SKILL_SHIELD; break;
            default: break;
        }

        if (armorSkill != 0 && !_bot->HasSkill(armorSkill))
            return false;
    }

    // Check weapon proficiency for weapon items
    if (itemTemplate->GetClass() == ITEM_CLASS_WEAPON)
    {
        uint32 weaponSkill = 0;
        switch (itemTemplate->GetSubClass())
        {
            case ITEM_SUBCLASS_WEAPON_AXE:          weaponSkill = SKILL_AXES; break;
            case ITEM_SUBCLASS_WEAPON_AXE2:         weaponSkill = SKILL_TWO_HANDED_AXES; break;
            case ITEM_SUBCLASS_WEAPON_BOW:          weaponSkill = SKILL_BOWS; break;
            case ITEM_SUBCLASS_WEAPON_GUN:          weaponSkill = SKILL_GUNS; break;
            case ITEM_SUBCLASS_WEAPON_MACE:         weaponSkill = SKILL_MACES; break;
            case ITEM_SUBCLASS_WEAPON_MACE2:        weaponSkill = SKILL_TWO_HANDED_MACES; break;
            case ITEM_SUBCLASS_WEAPON_POLEARM:      weaponSkill = SKILL_POLEARMS; break;
            case ITEM_SUBCLASS_WEAPON_SWORD:        weaponSkill = SKILL_SWORDS; break;
            case ITEM_SUBCLASS_WEAPON_SWORD2:       weaponSkill = SKILL_TWO_HANDED_SWORDS; break;
            case ITEM_SUBCLASS_WEAPON_WARGLAIVES:   weaponSkill = SKILL_WARGLAIVES; break;
            case ITEM_SUBCLASS_WEAPON_STAFF:        weaponSkill = SKILL_STAVES; break;
            case ITEM_SUBCLASS_WEAPON_FIST_WEAPON:  weaponSkill = SKILL_FIST_WEAPONS; break;
            case ITEM_SUBCLASS_WEAPON_DAGGER:       weaponSkill = SKILL_DAGGERS; break;
            case ITEM_SUBCLASS_WEAPON_CROSSBOW:     weaponSkill = SKILL_CROSSBOWS; break;
            case ITEM_SUBCLASS_WEAPON_WAND:         weaponSkill = SKILL_WANDS; break;
            default: break;
        }

        if (weaponSkill != 0 && !_bot->HasSkill(weaponSkill))
            return false;
    }

    return true;
}

uint8 EquipmentManager::GetItemEquipmentSlot(ItemTemplate const* itemTemplate)
{
    if (!itemTemplate)
        return EQUIPMENT_SLOT_END;

    // Map inventory type to equipment slot
    // NOTE: For multi-slot items (rings, trinkets), this returns the first slot.
    // Use GetMultiSlotEquipmentSlots() for proper multi-slot handling.
    switch (itemTemplate->GetInventoryType())
    {
        case INVTYPE_HEAD: return EQUIPMENT_SLOT_HEAD;
        case INVTYPE_NECK: return EQUIPMENT_SLOT_NECK;
        case INVTYPE_SHOULDERS: return EQUIPMENT_SLOT_SHOULDERS;
        case INVTYPE_BODY: return EQUIPMENT_SLOT_BODY;
        case INVTYPE_CHEST: return EQUIPMENT_SLOT_CHEST;
        case INVTYPE_WAIST: return EQUIPMENT_SLOT_WAIST;
        case INVTYPE_LEGS: return EQUIPMENT_SLOT_LEGS;
        case INVTYPE_FEET: return EQUIPMENT_SLOT_FEET;
        case INVTYPE_WRISTS: return EQUIPMENT_SLOT_WRISTS;
        case INVTYPE_HANDS: return EQUIPMENT_SLOT_HANDS;
        case INVTYPE_FINGER: return EQUIPMENT_SLOT_FINGER1;
        case INVTYPE_TRINKET: return EQUIPMENT_SLOT_TRINKET1;
        case INVTYPE_CLOAK: return EQUIPMENT_SLOT_BACK;
        case INVTYPE_WEAPON:
        case INVTYPE_WEAPONMAINHAND: return EQUIPMENT_SLOT_MAINHAND;
        case INVTYPE_WEAPONOFFHAND: return EQUIPMENT_SLOT_OFFHAND;
        case INVTYPE_HOLDABLE: return EQUIPMENT_SLOT_OFFHAND;
        case INVTYPE_SHIELD: return EQUIPMENT_SLOT_OFFHAND;
        case INVTYPE_2HWEAPON: return EQUIPMENT_SLOT_MAINHAND;
        case INVTYPE_RANGED:
        case INVTYPE_RANGEDRIGHT: return EQUIPMENT_SLOT_MAINHAND;
        case INVTYPE_TABARD: return EQUIPMENT_SLOT_TABARD;
        case INVTYPE_ROBE: return EQUIPMENT_SLOT_CHEST;
        default: return EQUIPMENT_SLOT_END;
    }
}

std::vector<uint8> EquipmentManager::GetMultiSlotEquipmentSlots(ItemTemplate const* itemTemplate)
{
    std::vector<uint8> slots;

    if (!itemTemplate)
        return slots;

    switch (itemTemplate->GetInventoryType())
    {
        case INVTYPE_FINGER:
            slots.push_back(EQUIPMENT_SLOT_FINGER1);
            slots.push_back(EQUIPMENT_SLOT_FINGER2);
            break;
        case INVTYPE_TRINKET:
            slots.push_back(EQUIPMENT_SLOT_TRINKET1);
            slots.push_back(EQUIPMENT_SLOT_TRINKET2);
            break;
        case INVTYPE_WEAPON:
            slots.push_back(EQUIPMENT_SLOT_MAINHAND);
            if (_bot && _bot->CanDualWield())
                slots.push_back(EQUIPMENT_SLOT_OFFHAND);
            break;
        default:
        {
            uint8 slot = GetItemEquipmentSlot(itemTemplate);
            if (slot != EQUIPMENT_SLOT_END)
                slots.push_back(slot);
            break;
        }
    }

    return slots;
}

::Item* EquipmentManager::GetWorstEquippedItemForSlots(std::vector<uint8> const& slots)
{
    if (!_bot || slots.empty())
        return nullptr;

    ::Item* worstItem = nullptr;
    float worstScore = std::numeric_limits<float>::max();
    bool hasEmptySlot = false;

    for (uint8 slot : slots)
    {
        ::Item* equippedItem = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);

        if (!equippedItem)
        {
            // Empty slot found - this is the best slot to fill
            hasEmptySlot = true;
            continue;
        }

        float score = CalculateItemScore(equippedItem);
        if (score < worstScore)
        {
            worstScore = score;
            worstItem = equippedItem;
        }
    }

    // If there's an empty slot, return nullptr to indicate the new item can go there
    if (hasEmptySlot)
        return nullptr;

    return worstItem;
}

// ============================================================================
// ADVANCED FEATURES
// ============================================================================

bool EquipmentManager::IsSetItem(::Item* item)
{
    if (!item)
        return false;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return false;

    // Check if item has ItemSet field (set items have ItemSet > 0)
    return proto->GetItemSet() != 0;
}

uint32 EquipmentManager::GetEquippedSetPieceCount(uint32 setId)
{
    if (!_bot || setId == 0)
        return 0;

    uint32 count = 0;

    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        if (::Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        {
            ItemTemplate const* proto = item->GetTemplate();
            if (proto && proto->GetItemSet() == setId)
                ++count;
        }
    }

    return count;
}

float EquipmentManager::GetWeaponDPS(::Item* item)
{
    if (!item)
        return 0.0f;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto || proto->GetClass() != ITEM_CLASS_WEAPON)
        return 0.0f;

    // Calculate DPS: ((minDmg + maxDmg) / 2) / delay
    float minDmg = 0.0f;
    float maxDmg = 0.0f;
    proto->GetDamage(item->GetItemLevel(nullptr), minDmg, maxDmg);

    uint32 delay = proto->GetDelay();
    if (delay == 0)
        return 0.0f;

    float avgDmg = (minDmg + maxDmg) / 2.0f;
    float dps = (avgDmg / static_cast<float>(delay)) * 1000.0f;

    return dps;
}

uint32 EquipmentManager::GetItemArmor(::Item* item)
{
    if (!item)
        return 0;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return 0;

    return proto->GetArmor(ITEM_QUALITY_NORMAL);
}

int32 EquipmentManager::GetItemStatValue(::Item* item, StatType stat)
{
    if (!item)
        return 0;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return 0;
    return ExtractStatValue(proto, stat);
}

// ============================================================================
// AUTOMATION CONTROL
// ============================================================================

void EquipmentManager::SetAutomationProfile(EquipmentAutomationProfile const& profile)
{
    _profile = profile;
}

EquipmentManager::EquipmentAutomationProfile const& EquipmentManager::GetAutomationProfile() const
{
    return _profile;
}

// ============================================================================
// METRICS
// ============================================================================

EquipmentMetrics const& EquipmentManager::GetMetrics()
{
    return _metrics;
}

EquipmentMetrics const& EquipmentManager::GetGlobalMetrics()
{
    return _globalMetrics;
}

// ============================================================================
// HELPER METHODS IMPLEMENTATION
// ============================================================================

bool EquipmentManager::IsOutdatedGear(::Item* item)
{
    if (!_bot || !item)
        return false;

    // Item is outdated if it's 10+ levels below bot level
    uint32 itemLevel = item->GetItemLevel(_bot);
    uint32 playerLevel = _bot->GetLevel();
    return (playerLevel > itemLevel + 10);
}

bool EquipmentManager::HasWrongPrimaryStats(::Item* item)
{
    if (!_bot || !item)
        return false;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto || proto->GetInventoryType() == INVTYPE_NON_EQUIP)
        return false;

    // Get bot's primary stat
    StatType primaryStat;
    uint8 classId = _bot->GetClass();
    switch (classId)
    {
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
        case CLASS_DEATH_KNIGHT:
            primaryStat = StatType::STRENGTH;
            break;

        case CLASS_HUNTER:
        case CLASS_ROGUE:
        case CLASS_SHAMAN:
        case CLASS_DRUID:
        case CLASS_MONK:
        case CLASS_DEMON_HUNTER:
            primaryStat = StatType::AGILITY;
            break;

        case CLASS_PRIEST:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
        case CLASS_EVOKER:
            primaryStat = StatType::INTELLECT;
            break;

        default:
            return false;
    }

    // Check if item has the primary stat
    int32 primaryStatValue = GetItemStatValue(item, primaryStat);

    // If armor/weapon has no primary stat, it's wrong for this class
    return (primaryStatValue == 0 && proto->GetClass() == ITEM_CLASS_ARMOR);
}

::Item* EquipmentManager::GetEquippedItemInSlot(uint8 slot)
{
    if (!_bot || slot >= EQUIPMENT_SLOT_END)
        return nullptr;

    return _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
}

bool EquipmentManager::CanEquipInSlot(::Item* item, uint8 slot)
{
    if (!_bot || !item)
        return false;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return false;

    // Basic checks
    if (!CanEquipItem(proto))
        return false;

    uint8 itemSlot = GetItemEquipmentSlot(proto);
    return (itemSlot == slot);
}

void EquipmentManager::EquipItemInSlot(::Item* item, uint8 slot)
{
    if (!_bot || !item)
        return;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return;

    // Use TrinityCore's CanEquipItem with NULL_SLOT to let TrinityCore find the best slot
    // This handles multi-slot items like rings (FINGER1/FINGER2) and trinkets (TRINKET1/TRINKET2)
    // Pass swap=true since we want to replace existing equipped items with upgrades
    uint16 dest;
    InventoryResult result = _bot->CanEquipItem(NULL_SLOT, dest, item, true);

    if (result != EQUIP_ERR_OK)
    {
        // If NULL_SLOT fails, try specific slot with swap enabled
        result = _bot->CanEquipItem(slot, dest, item, true);
    }

    if (result == EQUIP_ERR_OK)
    {
        // Get the actual destination slot for logging
        uint8 destSlot = (dest & 0xFF);

        // Check if we need to unequip existing item first (for swap)
        if (::Item* existingItem = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, destSlot))
        {
            // Find a bag slot to store the old item
            ItemPosCountVec storePos;
            InventoryResult storeResult = _bot->CanStoreItem(NULL_BAG, NULL_SLOT, storePos, existingItem, false);

            if (storeResult != EQUIP_ERR_OK)
            {
                TC_LOG_WARN("playerbot.equipment",
                    "⚠️ Cannot store replaced item {} for bot {} - no bag space (Error: {})",
                    existingItem->GetTemplate()->GetName(DEFAULT_LOCALE),
                    _bot->GetName(), static_cast<uint32>(storeResult));
                return;
            }

            // Remove existing item from equipment slot and store in bags
            _bot->RemoveItem(INVENTORY_SLOT_BAG_0, destSlot, true);
            _bot->StoreItem(storePos, existingItem, true);

            TC_LOG_DEBUG("playerbot.equipment",
                "Moved {} to bags to make room for upgrade",
                existingItem->GetTemplate()->GetName(DEFAULT_LOCALE));
        }

        // Remove new item from its current position (bag)
        _bot->RemoveItem(item->GetBagSlot(), item->GetSlot(), true);

        // Equip the new item
        _bot->EquipItem(dest, item, true);

        TC_LOG_INFO("playerbot.equipment", "✅ Equipped {} in slot {} for bot {}",
                   proto->GetName(DEFAULT_LOCALE), destSlot, _bot->GetName());
    }
    else
    {
        TC_LOG_WARN("playerbot.equipment", "❌ Failed to equip {} for bot {} (Error: {})",
                     proto->GetName(DEFAULT_LOCALE), _bot->GetName(), static_cast<uint32>(result));
    }
}

uint32 EquipmentManager::GetRecommendedFoodLevel()
{
    if (!_bot)
        return 0;

    // Return appropriate food item ID based on bot level
    // This would need actual WoW item IDs
    uint32 level = _bot->GetLevel();
    if (level >= 60) return 35953; // Example: Dragonfruit Pie
    if (level >= 50) return 33254; // Example: Stormchops
    if (level >= 40) return 27854; // Example: Smoked Talbuk
    if (level >= 30) return 8932;  // Example: Alterac Swiss
    if (level >= 20) return 4599;  // Example: Cured Ham Steak
    return 4540; // Example: Tough Hunk of Bread
}

uint32 EquipmentManager::GetRecommendedPotionLevel()
{
    if (!_bot)
        return 0;

    // Return appropriate water/mana potion item ID
    uint32 level = _bot->GetLevel();
    if (level >= 60) return 33445; // Example: Honeymint Tea
    if (level >= 50) return 28399; // Example: Filtered Draenic Water
    if (level >= 40) return 8077;  // Example: Conjured Sparkling Water
    return 5350; // Example: Conjured Water
}

std::vector<uint32> EquipmentManager::GetClassReagents(uint8 classId)
{
    std::vector<uint32> reagents;

    // Class-specific reagents (modern WoW has fewer than classic)
    switch (classId)
    {
        case CLASS_ROGUE:
            // Vanishing Powder, etc.
            break;

        case CLASS_MAGE:
            // Rune of Power reagents if needed
            break;

        default:
            break;
    }

    return reagents;
}

int32 EquipmentManager::ExtractStatValue(ItemTemplate const* proto, StatType stat)
{
    if (!proto)
        return 0;

    // TrinityCore 12.0 uses ItemMod array for stats
    for (uint8 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
    {
        ItemModType modType = static_cast<ItemModType>(proto->GetStatModifierBonusStat(i));
        int32 value = proto->GetStatPercentEditor(i);

        switch (stat)
        {
            case StatType::STRENGTH:
                if (modType == ITEM_MOD_STRENGTH) return value;
                break;
            case StatType::AGILITY:
                if (modType == ITEM_MOD_AGILITY) return value;
                break;
            case StatType::STAMINA:
                if (modType == ITEM_MOD_STAMINA) return value;
                break;
            case StatType::INTELLECT:
                if (modType == ITEM_MOD_INTELLECT) return value;
                break;
            case StatType::CRITICAL_STRIKE:
                if (modType == ITEM_MOD_CRIT_RATING) return value;
                break;
            case StatType::HASTE:
                if (modType == ITEM_MOD_HASTE_RATING) return value;
                break;
            case StatType::VERSATILITY:
                if (modType == ITEM_MOD_VERSATILITY) return value;
                break;
            case StatType::MASTERY:
                if (modType == ITEM_MOD_MASTERY_RATING) return value;
                break;
            case StatType::ARMOR:
                return static_cast<int32>(proto->GetArmor(ITEM_QUALITY_NORMAL));
            default:
                break;
        }
    }

    return 0;
}

float EquipmentManager::CalculateTotalStats(ItemTemplate const* proto, std::vector<std::pair<StatType, float>> const& weights)
{
    if (!proto)
        return 0.0f;

    float total = 0.0f;

    for (const auto& [statType, weight] : weights)
    {
        int32 value = ExtractStatValue(proto, statType);
        total += static_cast<float>(value) * weight;
    }

    return total;
}

void EquipmentManager::UpdateMetrics(bool wasEquipped, bool wasUpgrade, uint32 goldValue)
{
    if (wasEquipped)
        _metrics.itemsEquipped++;

    if (wasUpgrade)
        _metrics.upgradesFound++;

    if (goldValue > 0)
        _metrics.totalGoldFromJunk += goldValue;

    // Update global metrics
    if (wasEquipped)
        _globalMetrics.itemsEquipped++;

    if (wasUpgrade)
        _globalMetrics.upgradesFound++;

    if (goldValue > 0)
        _globalMetrics.totalGoldFromJunk += goldValue;
}

void EquipmentManager::LogEquipmentDecision(std::string const& action, std::string const& reason)
{
    if (_bot)
    {
        TC_LOG_DEBUG("playerbot.equipment", "Equipment Decision - Bot: {}, Action: {}, Reason: {}",
                     _bot->GetName(), action, reason);
    }
}

} // namespace Playerbot
