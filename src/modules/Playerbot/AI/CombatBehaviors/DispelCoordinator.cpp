/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "DispelCoordinator.h"
#include "GameTime.h"
#include "BotAI.h"
#include "Player.h"
#include "Group.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "World.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "SpellHistory.h"
#include "Timer.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "../../Spatial/SpatialGridManager.h"  // Lock-free spatial grid for deadlock fix
#include "../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5B: Thread-safe helpers
#include "../../Packets/SpellPacketBuilder.h"  // PHASE 0 WEEK 3: Packet-based spell casting
#include "../../Group/GroupRoleEnums.h" // Centralized role detection utilities
#include <mutex>
#include <algorithm>
#include <cmath>

namespace Playerbot
{

// ============================================================================
// Role Detection Helpers - Using centralized utilities from GroupRoleEnums.h
// ============================================================================

namespace {
    enum BotRole : uint8 {
        BOT_ROLE_TANK = 0,
        BOT_ROLE_HEALER = 1,
        BOT_ROLE_DPS = 2
    };

    BotRole GetPlayerRole(Player const* player)
    {
        if (!player) return BOT_ROLE_DPS;

        // Use centralized role detection from GroupRoleEnums.h
        // This queries ChrSpecializationEntry for accurate role detection
        GroupRole role = GetPlayerSpecRole(player);

        switch (role)
        {
            case GroupRole::TANK:
                return BOT_ROLE_TANK;
            case GroupRole::HEALER:
                return BOT_ROLE_HEALER;
            default:
                return BOT_ROLE_DPS;
        }
    }
    bool IsTank(Player const* p) { return IsPlayerTank(p); }
    bool IsHealer(Player const* p) { return IsPlayerHealer(p); }

    // Spec-aware melee detection for spread debuff analysis
    bool IsMeleeSpec(uint32 specId)
    {
        switch (specId)
        {
            // Warrior - All specs are melee
            case 71:  // Arms
            case 72:  // Fury
            case 73:  // Protection
            // Paladin - Ret and Prot are melee
            case 66:  // Protection
            case 70:  // Retribution
            // Hunter - Survival is melee
            case 255: // Survival
            // Rogue - All specs are melee
            case 259: // Assassination
            case 260: // Outlaw
            case 261: // Subtlety
            // Death Knight - All specs are melee
            case 250: // Blood
            case 251: // Frost
            case 252: // Unholy
            // Monk - WW and BM are melee
            case 268: // Brewmaster
            case 269: // Windwalker
            // Shaman - Enhancement is melee
            case 263: // Enhancement
            // Druid - Feral and Guardian are melee
            case 103: // Feral
            case 104: // Guardian
            // Demon Hunter - Both specs are melee
            case 577: // Havoc
            case 581: // Vengeance
                return true;
            default:
                return false;
        }
    }
}

// Static member definitions moved to header with inline to fix DLL linkage (C2491)

// ============================================================================
// Class-specific Dispel Spell IDs
// ============================================================================

namespace DispelSpells
{
    // Magic Dispels
    constexpr uint32 PRIEST_DISPEL_MAGIC = 528;
    constexpr uint32 PRIEST_MASS_DISPEL = 32375;
    constexpr uint32 PALADIN_CLEANSE = 4987;
    constexpr uint32 SHAMAN_PURGE = 370;
    constexpr uint32 MAGE_REMOVE_CURSE = 475;
    constexpr uint32 WARLOCK_DEVOUR_MAGIC = 19505;  // Felhunter pet
    constexpr uint32 EVOKER_CAUTERIZING_FLAME = 374251;
    constexpr uint32 DEMON_HUNTER_CONSUME_MAGIC = 278326;

    // Disease Dispels
    constexpr uint32 PRIEST_ABOLISH_DISEASE = 552;
    constexpr uint32 PRIEST_CURE_DISEASE = 528;  // Same as Dispel Magic in some versions
    constexpr uint32 PALADIN_PURIFY = 1152;
    constexpr uint32 MONK_DETOX = 115450;

    // Poison Dispels
    constexpr uint32 DRUID_ABOLISH_POISON = 2893;
    constexpr uint32 DRUID_CURE_POISON = 8946;
    constexpr uint32 PALADIN_PURIFY_POISON = 1152;  // Same as disease
    constexpr uint32 SHAMAN_CURE_TOXINS = 526;
    constexpr uint32 MONK_DETOX_POISON = 115450;  // Same spell

    // Curse Dispels
    constexpr uint32 DRUID_REMOVE_CURSE = 2782;
    constexpr uint32 MAGE_REMOVE_LESSER_CURSE = 475;
    constexpr uint32 SHAMAN_CLEANSE_SPIRIT = 51886;

    // Combined/Special
    constexpr uint32 DRUID_NATURES_CURE = 88423;  // Magic + Curse + Poison
    constexpr uint32 SHAMAN_CLEANSE_SPIRIT_IMPROVED = 77130;
    constexpr uint32 PRIEST_PURIFY = 527;
}

// ============================================================================
// Constructor & Destructor
// ============================================================================

DispelCoordinator::DispelCoordinator(BotAI* ai)
    : m_ai(ai)
    , m_bot(ai ? ai->GetBot() : nullptr)
    , m_group(nullptr)
{

    m_group = m_bot->GetGroup();

    // Initialize database if needed
    if (!s_databaseInitialized)
    {
        ::std::lock_guard lock(s_databaseMutex);
        if (!s_databaseInitialized)
        {
            InitializeGlobalDatabase();
        }
    }

    // Initialize dispeller capabilities
    UpdateDispellerCapabilities();
}

DispelCoordinator::~DispelCoordinator() = default;

// ============================================================================
// Static Database Initialization
// ============================================================================

void DispelCoordinator::InitializeGlobalDatabase()
{
    if (s_databaseInitialized)
        return;

    // ========================================================================
    // Debuff Database - Common debuffs that should be dispelled
    // ========================================================================

    // Incapacitate Effects (Highest Priority)
    s_debuffDatabase[118] = {118, DISPEL_MAGIC, PRIORITY_INCAPACITATE, 0, 0, true, true, false, 0};  // Polymorph
    s_debuffDatabase[12824] = {12824, DISPEL_MAGIC, PRIORITY_INCAPACITATE, 0, 0, true, true, false, 0};  // Polymorph (Rank 2)
    s_debuffDatabase[12825] = {12825, DISPEL_MAGIC, PRIORITY_INCAPACITATE, 0, 0, true, true, false, 0};  // Polymorph (Rank 3)
    s_debuffDatabase[12826] = {12826, DISPEL_MAGIC, PRIORITY_INCAPACITATE, 0, 0, true, true, false, 0};  // Polymorph (Rank 4)
    s_debuffDatabase[5782] = {5782, DISPEL_MAGIC, PRIORITY_INCAPACITATE, 0, 0, true, true, false, 0};  // Fear
    s_debuffDatabase[6213] = {6213, DISPEL_MAGIC, PRIORITY_INCAPACITATE, 0, 0, true, true, false, 0};  // Fear (Rank 2)
    s_debuffDatabase[6215] = {6215, DISPEL_MAGIC, PRIORITY_INCAPACITATE, 0, 0, true, true, false, 0};  // Fear (Rank 3)
    s_debuffDatabase[51514] = {51514, DISPEL_MAGIC, PRIORITY_INCAPACITATE, 0, 0, true, true, false, 0};  // Hex
    s_debuffDatabase[710] = {710, DISPEL_MAGIC, PRIORITY_INCAPACITATE, 0, 0, true, false, false, 0};  // Banish
    s_debuffDatabase[6770] = {6770, DISPEL_NONE, PRIORITY_INCAPACITATE, 0, 0, true, true, false, 0};  // Sap (not dispellable)
    s_debuffDatabase[2094] = {2094, DISPEL_NONE, PRIORITY_INCAPACITATE, 0, 0, true, true, false, 0};  // Blind (physical)
    s_debuffDatabase[8122] = {8122, DISPEL_MAGIC, PRIORITY_INCAPACITATE, 0, 0, true, false, false, 0};  // Psychic Scream
    s_debuffDatabase[605] = {605, DISPEL_MAGIC, PRIORITY_DEATH, 0, 0, true, true, false, 0};  // Mind Control

    // Dangerous DOTs (High Priority)
    s_debuffDatabase[348] = {348, DISPEL_MAGIC, PRIORITY_DANGEROUS, 1500, 0, false, false, false, 0};  // Immolate
    s_debuffDatabase[707] = {707, DISPEL_MAGIC, PRIORITY_DANGEROUS, 1000, 0, false, false, false, 0};  // Immolate (Rank 2)
    s_debuffDatabase[172] = {172, DISPEL_MAGIC, PRIORITY_DANGEROUS, 1800, 0, false, false, false, 0};  // Corruption
    s_debuffDatabase[6222] = {6222, DISPEL_MAGIC, PRIORITY_DANGEROUS, 1900, 0, false, false, false, 0};  // Corruption (Rank 2)
    s_debuffDatabase[589] = {589, DISPEL_MAGIC, PRIORITY_DANGEROUS, 1200, 0, false, false, false, 0};  // Shadow Word: Pain
    s_debuffDatabase[594] = {594, DISPEL_MAGIC, PRIORITY_DANGEROUS, 1300, 0, false, false, false, 0};  // Shadow Word: Pain (Rank 2)
    s_debuffDatabase[30108] = {30108, DISPEL_MAGIC, PRIORITY_DEATH, 3000, 0, false, false, false, 0};  // Unstable Affliction
    s_debuffDatabase[2120] = {2120, DISPEL_MAGIC, PRIORITY_DANGEROUS, 800, 0, false, false, false, 0};  // Flamestrike
    s_debuffDatabase[34914] = {34914, DISPEL_MAGIC, PRIORITY_DANGEROUS, 2000, 0, false, false, false, 0};  // Vampiric Touch
    s_debuffDatabase[15487] = {15487, DISPEL_MAGIC, PRIORITY_INCAPACITATE, 0, 0, false, true, false, 0};  // Silence

    // Slows and Roots (Moderate Priority)
    s_debuffDatabase[122] = {122, DISPEL_MAGIC, PRIORITY_MODERATE, 0, 1.0f, true, false, false, 0};  // Frost Nova
    s_debuffDatabase[865] = {865, DISPEL_MAGIC, PRIORITY_MODERATE, 0, 1.0f, true, false, false, 0};  // Frost Nova (Rank 2)
    s_debuffDatabase[116] = {116, DISPEL_MAGIC, PRIORITY_MODERATE, 200, 0.5f, false, false, false, 0};  // Frostbolt slow
    s_debuffDatabase[12674] = {12674, DISPEL_MAGIC, PRIORITY_MODERATE, 200, 0.5f, false, false, false, 0};  // Frostbolt (Rank 2)
    s_debuffDatabase[45524] = {45524, DISPEL_MAGIC, PRIORITY_DANGEROUS, 0, 1.0f, true, false, false, 0};  // Chains of Ice
    s_debuffDatabase[339] = {339, DISPEL_MAGIC, PRIORITY_MODERATE, 0, 1.0f, true, false, false, 0};  // Entangling Roots
    s_debuffDatabase[1062] = {1062, DISPEL_MAGIC, PRIORITY_MODERATE, 0, 1.0f, true, false, false, 0};  // Entangling Roots (Rank 2)
    s_debuffDatabase[15407] = {15407, DISPEL_MAGIC, PRIORITY_MODERATE, 1000, 0.5f, false, false, false, 0};  // Mind Flay
    s_debuffDatabase[6358] = {6358, DISPEL_MAGIC, PRIORITY_MODERATE, 0, 0.5f, false, false, false, 0};  // Seduction (Succubus)
    s_debuffDatabase[1513] = {1513, DISPEL_NONE, PRIORITY_MODERATE, 0, 0, true, false, false, 0};  // Scare Beast
    s_debuffDatabase[5246] = {5246, DISPEL_NONE, PRIORITY_DANGEROUS, 0, 0, true, false, false, 0};  // Intimidating Shout
    s_debuffDatabase[31661] = {31661, DISPEL_MAGIC, PRIORITY_DANGEROUS, 0, 1.0f, true, false, false, 0};  // Dragon's Breath

    // Curses (Moderate Priority)
    s_debuffDatabase[980] = {980, DISPEL_CURSE, PRIORITY_MODERATE, 1000, 0, false, false, false, 0};  // Curse of Agony
    s_debuffDatabase[1014] = {1014, DISPEL_CURSE, PRIORITY_MODERATE, 1100, 0, false, false, false, 0};  // Curse of Agony (Rank 2)
    s_debuffDatabase[18223] = {18223, DISPEL_CURSE, PRIORITY_DANGEROUS, 0, 0.5f, false, false, false, 0};  // Curse of Exhaustion
    s_debuffDatabase[1490] = {1490, DISPEL_CURSE, PRIORITY_MODERATE, 0, 0, false, false, false, 0};  // Curse of the Elements
    s_debuffDatabase[702] = {702, DISPEL_CURSE, PRIORITY_MODERATE, 0, 0, false, false, false, 0};  // Curse of Weakness
    s_debuffDatabase[1714] = {1714, DISPEL_CURSE, PRIORITY_DANGEROUS, 0, 0, false, true, false, 0};  // Curse of Tongues
    s_debuffDatabase[16231] = {16231, DISPEL_CURSE, PRIORITY_MODERATE, 0, 0, false, false, false, 0};  // Curse of Recklessness

    // Poisons (Dangerous Priority)
    s_debuffDatabase[2818] = {2818, DISPEL_POISON, PRIORITY_DANGEROUS, 1200, 0, false, false, false, 0};  // Deadly Poison
    s_debuffDatabase[2819] = {2819, DISPEL_POISON, PRIORITY_DANGEROUS, 1300, 0, false, false, false, 0};  // Deadly Poison II
    s_debuffDatabase[3409] = {3409, DISPEL_POISON, PRIORITY_MODERATE, 0, 0.7f, false, false, false, 0};  // Crippling Poison
    s_debuffDatabase[8680] = {8680, DISPEL_POISON, PRIORITY_DANGEROUS, 1500, 0, false, false, false, 0};  // Instant Poison
    s_debuffDatabase[5760] = {5760, DISPEL_POISON, PRIORITY_MODERATE, 0, 0.5f, false, true, false, 0};  // Mind-numbing Poison
    s_debuffDatabase[13218] = {13218, DISPEL_POISON, PRIORITY_DANGEROUS, 0, 0, false, false, false, 0};  // Wound Poison
    s_debuffDatabase[27189] = {27189, DISPEL_POISON, PRIORITY_DANGEROUS, 0, 0, false, false, false, 0};  // Wound Poison V
    s_debuffDatabase[25810] = {25810, DISPEL_POISON, PRIORITY_DANGEROUS, 2000, 0, false, false, false, 0};  // Viper Sting
    s_debuffDatabase[14280] = {14280, DISPEL_POISON, PRIORITY_DANGEROUS, 1800, 0, false, false, false, 0};  // Viper Sting (Rank 2)

    // Diseases (High Priority)
    s_debuffDatabase[55095] = {55095, DISPEL_DISEASE, PRIORITY_DANGEROUS, 1000, 0, false, false, true, 10};  // Frost Fever
    s_debuffDatabase[55078] = {55078, DISPEL_DISEASE, PRIORITY_DANGEROUS, 1000, 0, false, false, true, 10};  // Blood Plague
    s_debuffDatabase[3674] = {3674, DISPEL_DISEASE, PRIORITY_MODERATE, 600, 0, false, false, false, 0};  // Black Arrow
    s_debuffDatabase[19434] = {19434, DISPEL_DISEASE, PRIORITY_MODERATE, 800, 0, false, false, false, 0};  // Aimed Shot
    s_debuffDatabase[30981] = {30981, DISPEL_DISEASE, PRIORITY_MODERATE, 700, 0.3f, false, false, false, 0};  // Crippling Poison (Disease version)

    // Special/Misc Debuffs
    s_debuffDatabase[12654] = {12654, DISPEL_MAGIC, PRIORITY_MODERATE, 500, 0, false, false, false, 0};  // Ignite
    s_debuffDatabase[44572] = {44572, DISPEL_MAGIC, PRIORITY_DANGEROUS, 2000, 0, false, false, false, 0};  // Deep Freeze
    s_debuffDatabase[31117] = {31117, DISPEL_MAGIC, PRIORITY_INCAPACITATE, 0, 0, true, false, false, 0};  // Unstable Affliction (Silence)
    s_debuffDatabase[19503] = {19503, DISPEL_MAGIC, PRIORITY_MODERATE, 0, 0.4f, false, false, false, 0};  // Scatter Shot
    s_debuffDatabase[19185] = {19185, DISPEL_MAGIC, PRIORITY_MODERATE, 0, 1.0f, true, false, false, 0};  // Entrapment
    s_debuffDatabase[5116] = {5116, DISPEL_MAGIC, PRIORITY_MODERATE, 0, 0.5f, false, false, false, 0};  // Concussive Shot
    s_debuffDatabase[1330] = {1330, DISPEL_NONE, PRIORITY_INCAPACITATE, 0, 0, false, true, false, 0};  // Garrote - Silence
    s_debuffDatabase[408] = {408, DISPEL_NONE, PRIORITY_INCAPACITATE, 0, 0, true, true, false, 0};  // Kidney Shot
    s_debuffDatabase[1833] = {1833, DISPEL_NONE, PRIORITY_INCAPACITATE, 0, 0, true, true, false, 0};  // Cheap Shot
    s_debuffDatabase[51722] = {51722, DISPEL_MAGIC, PRIORITY_MODERATE, 0, 0.7f, false, false, false, 0};  // Dismantle
    s_debuffDatabase[676] = {676, DISPEL_NONE, PRIORITY_MODERATE, 0, 0.5f, false, false, false, 0};  // Disarm
    s_debuffDatabase[64044] = {64044, DISPEL_MAGIC, PRIORITY_INCAPACITATE, 0, 0, true, false, false, 0};  // Psychic Horror
    s_debuffDatabase[87204] = {87204, DISPEL_MAGIC, PRIORITY_INCAPACITATE, 0, 0, false, true, false, 0};  // Sin and Punishment

    // ========================================================================
    // Purge Database - Enemy buffs that should be purged
    // ========================================================================

    // Immunity Effects (Highest Priority)
    s_purgeDatabase[642] = {642, PURGE_IMMUNITY, false, true, false, false, 0, 0, 0};  // Divine Shield
    s_purgeDatabase[45438] = {45438, PURGE_IMMUNITY, false, true, false, false, 0, 0, 0};  // Ice Block
    s_purgeDatabase[1022] = {1022, PURGE_IMMUNITY, false, true, false, false, 0, 0, 0};  // Blessing of Protection
    s_purgeDatabase[33786] = {33786, PURGE_IMMUNITY, false, true, false, false, 0, 0, 0};  // Cyclone
    s_purgeDatabase[19574] = {19574, PURGE_IMMUNITY, false, true, false, false, 0, 0, 0};  // Bestial Wrath
    s_purgeDatabase[46924] = {46924, PURGE_IMMUNITY, false, true, false, false, 0, 0, 0};  // Bladestorm

    // Major Buffs (High Priority)
    s_purgeDatabase[2825] = {2825, PURGE_MAJOR_BUFF, false, false, true, false, 0.3f, 0, 0.3f};  // Bloodlust
    s_purgeDatabase[32182] = {32182, PURGE_MAJOR_BUFF, false, false, true, false, 0.3f, 0, 0.3f};  // Heroism
    s_purgeDatabase[80353] = {80353, PURGE_MAJOR_BUFF, false, false, true, false, 0.3f, 0, 0.3f};  // Time Warp
    s_purgeDatabase[90355] = {90355, PURGE_MAJOR_BUFF, false, false, true, false, 0.3f, 0, 0.3f};  // Ancient Hysteria
    s_purgeDatabase[10060] = {10060, PURGE_MAJOR_BUFF, false, false, true, true, 0.4f, 0.4f, 0.4f};  // Power Infusion
    s_purgeDatabase[31884] = {31884, PURGE_MAJOR_BUFF, false, false, true, false, 0.35f, 0, 0};  // Avenging Wrath
    s_purgeDatabase[1719] = {1719, PURGE_MAJOR_BUFF, true, false, true, false, 0.2f, 0, 0};  // Recklessness
    s_purgeDatabase[12472] = {12472, PURGE_MAJOR_BUFF, false, false, true, false, 0.2f, 0, 0.2f};  // Icy Veins
    s_purgeDatabase[12042] = {12042, PURGE_MAJOR_BUFF, false, false, true, false, 0.3f, 0, 0.3f};  // Arcane Power
    s_purgeDatabase[12043] = {12043, PURGE_MAJOR_BUFF, false, false, false, false, 0, 0, 0};  // Presence of Mind

    // Enrage Effects (High Priority for Tanks)
    s_purgeDatabase[18499] = {18499, PURGE_ENRAGE, true, false, true, false, 0.25f, 0, 0};  // Berserker Rage
    s_purgeDatabase[12880] = {12880, PURGE_ENRAGE, true, false, true, false, 0.2f, 0, 0};  // Enrage (Warrior)
    s_purgeDatabase[14202] = {14202, PURGE_ENRAGE, true, false, true, false, 0.25f, 0, 0};  // Enrage (Druid)
    s_purgeDatabase[15061] = {15061, PURGE_ENRAGE, true, false, true, false, 0.3f, 0, 0};  // Enrage (Hunter Pet)
    s_purgeDatabase[52610] = {52610, PURGE_ENRAGE, true, false, true, false, 0.1f, 0, 0};  // Savage Roar
    s_purgeDatabase[49016] = {49016, PURGE_ENRAGE, true, false, true, false, 0.15f, 0, 0};  // Unholy Frenzy

    // Moderate Buffs
    s_purgeDatabase[1126] = {1126, PURGE_MODERATE_BUFF, false, false, false, false, 0, 0, 0};  // Mark of the Wild
    s_purgeDatabase[21562] = {21562, PURGE_MODERATE_BUFF, false, false, false, false, 0, 0, 0};  // Power Word: Fortitude
    s_purgeDatabase[19740] = {19740, PURGE_MODERATE_BUFF, false, false, false, false, 0, 0, 0};  // Blessing of Might
    s_purgeDatabase[20217] = {20217, PURGE_MODERATE_BUFF, false, false, false, false, 0, 0, 0};  // Blessing of Kings
    s_purgeDatabase[27683] = {27683, PURGE_MODERATE_BUFF, false, false, false, false, 0, 0, 0};  // Prayer of Shadow Protection
    s_purgeDatabase[10938] = {10938, PURGE_MODERATE_BUFF, false, false, false, false, 0, 0, 0};  // Power Word: Fortitude (Group)
    s_purgeDatabase[24932] = {24932, PURGE_MODERATE_BUFF, false, false, false, false, 0, 0, 0};  // Leader of the Pack
    s_purgeDatabase[17] = {17, PURGE_MODERATE_BUFF, false, false, false, false, 0, 0, 0};  // Power Word: Shield
    s_purgeDatabase[592] = {592, PURGE_MODERATE_BUFF, false, false, false, false, 0, 0, 0};  // Power Word: Shield (Rank 2)
    s_purgeDatabase[139] = {139, PURGE_MODERATE_BUFF, false, false, false, true, 0, 0.2f, 0};  // Renew
    s_purgeDatabase[774] = {774, PURGE_MODERATE_BUFF, false, false, false, true, 0, 0.25f, 0};  // Rejuvenation
    s_purgeDatabase[8936] = {8936, PURGE_MODERATE_BUFF, false, false, false, true, 0, 0.3f, 0};  // Regrowth
    s_purgeDatabase[33763] = {33763, PURGE_MODERATE_BUFF, false, false, false, true, 0, 0.2f, 0};  // Lifebloom

    // Minor Buffs
    s_purgeDatabase[1243] = {1243, PURGE_MINOR_BUFF, false, false, false, false, 0, 0, 0};  // Power Word: Fortitude (Single)
    s_purgeDatabase[1244] = {1244, PURGE_MINOR_BUFF, false, false, false, false, 0, 0, 0};  // Power Word: Fortitude (Rank 2)
    s_purgeDatabase[1245] = {1245, PURGE_MINOR_BUFF, false, false, false, false, 0, 0, 0};  // Power Word: Fortitude (Rank 3)
    s_purgeDatabase[2791] = {2791, PURGE_MINOR_BUFF, false, false, false, false, 0, 0, 0};  // Power Word: Fortitude (Rank 4)
    s_purgeDatabase[10937] = {10937, PURGE_MINOR_BUFF, false, false, false, false, 0, 0, 0};  // Power Word: Fortitude (Rank 5)
    s_purgeDatabase[10938] = {10938, PURGE_MINOR_BUFF, false, false, false, false, 0, 0, 0};  // Power Word: Fortitude (Rank 6)
    s_purgeDatabase[1459] = {1459, PURGE_MINOR_BUFF, false, false, false, false, 0, 0, 0};  // Arcane Intellect
    s_purgeDatabase[8096] = {8096, PURGE_MINOR_BUFF, false, false, false, false, 0, 0, 0};  // Intellect (Scroll)
    s_purgeDatabase[8112] = {8112, PURGE_MINOR_BUFF, false, false, false, false, 0, 0, 0};  // Spirit (Scroll)

    s_databaseInitialized = true;
    TC_LOG_INFO("playerbot", "DispelCoordinator: Initialized global database with {} debuffs and {} purgeable buffs",
                s_debuffDatabase.size(), s_purgeDatabase.size());
}

// ============================================================================
// Database Accessors
// ============================================================================

const DispelCoordinator::DebuffData* DispelCoordinator::GetDebuffData(uint32 auraId)
{
    auto it = s_debuffDatabase.find(auraId);
    return it != s_debuffDatabase.end() ? &it->second : nullptr;
}

const DispelCoordinator::PurgeableBuff* DispelCoordinator::GetPurgeableBuffData(uint32 auraId)
{
    auto it = s_purgeDatabase.find(auraId);
    return it != s_purgeDatabase.end() ? &it->second : nullptr;
}

// ============================================================================
// Core Update Functions
// ============================================================================

void DispelCoordinator::Update(uint32 /*diff*/)
{
    if (!m_bot || !m_group)
        return;

    uint32 now = GameTime::GetGameTimeMS();

    // Update dispeller capabilities periodically
    if (now - m_lastCapabilityUpdate > m_config.capabilityUpdateInterval)
    {
        UpdateDispellerCapabilities();
        m_lastCapabilityUpdate = now;
    }

    // Update dispel assignments
    if (now - m_lastDebuffScan > m_config.debuffScanInterval)
    {
        UpdateDispelAssignments();
        m_lastDebuffScan = now;
    }

    // Check for purge targets
    if (now - m_lastPurgeScan > m_config.purgeScanInterval)
    {
        // Purge scan handled in ExecutePurge
        m_lastPurgeScan = now;
    }

    // Clean up expired assignments
    CleanupAssignments();

    // Update GCD tracker
    if (m_globalCooldownUntil > 0 && now > m_globalCooldownUntil)
    {
        m_globalCooldownUntil = 0;
    }
}

void DispelCoordinator::UpdateDispelAssignments()
{
    if (!m_group)
        return;

    // Gather all debuffs on group members
    ::std::vector<DebuffTarget> debuffs = GatherGroupDebuffs();

    // Sort by adjusted priority (highest first)
    ::std::sort(debuffs.begin(), debuffs.end(),
        [](const DebuffTarget& a, const DebuffTarget& b)
        {
            return a.adjustedPriority > b.adjustedPriority;
        });

    uint32 now = GameTime::GetGameTimeMS();

    // Process high priority debuffs
    for (const auto& debuff : debuffs)
    {
        // Skip if priority too low
    if (debuff.adjustedPriority < m_config.priorityThreshold)
            break;

        // Skip if already being handled
    if (IsBeingDispelled(debuff.targetGuid, debuff.auraId))
            continue;

        // Find best dispeller
        ObjectGuid bestDispeller = FindBestDispeller(debuff);
        if (bestDispeller.IsEmpty())
            continue;

        // Create assignment
        DispelAssignment assignment;
        assignment.dispeller = bestDispeller;
        assignment.target = debuff.targetGuid;
        assignment.auraId = debuff.auraId;
        assignment.priority = debuff.priority;
        assignment.assignedTime = now;
        assignment.fulfilled = false;
        assignment.dispelType = debuff.dispelType;

        m_assignments.push_back(assignment);
        ++m_statistics.assignmentsCreated;

        // If this is our assignment, save it
    if (bestDispeller == m_bot->GetGUID())
        {
            m_currentAssignment = assignment;
        }

        // Mark dispeller as busy
        MarkDispellerBusy(bestDispeller, m_config.dispelGCD);
    }
}

// ============================================================================
// Debuff Priority Adjustment
// ============================================================================

float DispelCoordinator::DebuffData::GetAdjustedPriority(Unit* target) const
{
    if (!target)
        return static_cast<float>(basePriority);

    float priority = static_cast<float>(basePriority);
    Player* player = target->ToPlayer();
    // Role-based adjustments
    if (player)
    {
        // Tank priority adjustments
    if (GetPlayerRole(player) == BOT_ROLE_TANK)
        {
            if (slowPercent > 0 || preventsActions)
                priority += 2.0f;  // Tank mobility is critical
    if (damagePerTick > 0 && target->GetHealthPct() < 50.0f)
                priority += 1.0f;  // Tank taking DOT damage at low health
        }

        // Healer priority adjustments
    if (GetPlayerRole(player) == BOT_ROLE_HEALER)
        {
            if (preventsCasting)
                priority += 2.5f;  // Healer silenced is emergency
    if (preventsActions)
                priority += 2.0f;  // Healer CC'd is critical
        }
    }

    // Health-based adjustments
    float healthPct = target->GetHealthPct();
    if (healthPct < 30.0f && damagePerTick > 0)
    {
        priority += 1.5f;  // Low HP with DOT
    if (damagePerTick > target->GetMaxHealth() * 0.05f)
            priority += 1.0f;  // Heavy DOT at low health
    }

    // Full spreading debuff detection with proximity analysis
    // Analyzes group clustering and potential spread targets
    if (spreads)
    {
        Player* targetPlayer = target->ToPlayer();
        if (targetPlayer && targetPlayer->GetGroup())
        {
            Group* group = targetPlayer->GetGroup();
            float spreadRadius = 8.0f; // Most spreading debuffs have 8-10 yard radius
            uint32 nearbyAllies = 0;
            uint32 nearbyMelee = 0;
            uint32 alreadyInfected = 0;

            // Get the aura's current stack count for fresh debuff detection
            uint8 stackCount = 1;
            if (Aura* aura = target->GetAura(auraId))
                stackCount = aura->GetStackAmount();

            // Count nearby allies and analyze clustering
            for (GroupReference& itr : group->GetMembers())
            {
                Player* member = itr.GetSource();
                if (!member || member == targetPlayer || member->isDead())
                    continue;

                float distSq = target->GetExactDistSq(member);
                if (distSq <= spreadRadius * spreadRadius)
                {
                    ++nearbyAllies;

                    // Check if they're melee (likely stacked)
                    uint32 specId = member->GetPrimarySpecializationEntry() ?
                                    member->GetPrimarySpecializationEntry()->ID : 0;
                    bool isMelee = IsMeleeSpec(specId);
                    if (isMelee)
                        ++nearbyMelee;

                    // Check if already infected with this debuff
                    if (member->HasAura(auraId))
                        ++alreadyInfected;
                }
            }

            // Calculate spread risk score
            float spreadRisk = 0.0f;

            // Base risk from number of nearby allies
            if (nearbyAllies >= 4)
                spreadRisk += 2.0f;  // High stacking = critical
            else if (nearbyAllies >= 2)
                spreadRisk += 1.5f;  // Medium stacking
            else if (nearbyAllies >= 1)
                spreadRisk += 1.0f;  // Some risk

            // Additional risk if melee are stacked (common in boss fights)
            if (nearbyMelee >= 2)
                spreadRisk += 0.5f;

            // Reduce priority if spread already occurred (damage already done)
            if (alreadyInfected >= 2)
                spreadRisk -= 0.5f;

            // Environmental spread factor - check if in combat (more movement = less spread)
            if (targetPlayer->IsInCombat())
            {
                // Active combat means movement, slightly less spread risk
                spreadRisk *= 0.9f;
            }

            // Apply spread risk to priority
            priority += ::std::max(0.5f, spreadRisk);

            // Extra priority if this is a chain-spreading debuff targeting a clump
            if (nearbyAllies >= 3 && stackCount <= 1)
            {
                // Fresh debuff on grouped targets = emergency
                priority += 1.0f;
            }
        }
        else
        {
            // Fallback if not in group - still add base spread priority
            priority += 1.0f;
        }
    }

    return priority;
}

// ============================================================================
// Dispeller Selection & Scoring
// ============================================================================

ObjectGuid DispelCoordinator::FindBestDispeller(const DebuffTarget& target) const
{
    ::std::vector<::std::pair<ObjectGuid, float>> candidates;

    for (const auto& dispeller : m_dispellers)
    {
        if (!CanDispel(dispeller, target))
            continue;

        float score = CalculateDispellerScore(dispeller, target);
        if (score > 0)
        {
            candidates.push_back({dispeller.botGuid, score});
        }
    }

    if (candidates.empty())
        return ObjectGuid::Empty;

    // Sort by score (highest first)
    ::std::sort(candidates.begin(), candidates.end(),
        [](const auto& a, const auto& b)
        {
            return a.second > b.second;
        });

    return candidates[0].first;
}

float DispelCoordinator::CalculateDispellerScore(const DispellerCapability& dispeller,
                                                  const DebuffTarget& target) const
{
    float score = 100.0f;

    // Check cooldown availability
    uint32 now = GameTime::GetGameTimeMS();
    if (dispeller.dispelCooldown > 0 &&
        now - dispeller.lastDispelTime < dispeller.dispelCooldown)
    {
        score -= 50.0f;  // On cooldown
    }

    // Check GCD
    if (dispeller.globalCooldown > 0)
    {
        score -= 30.0f;  // In GCD
    }

    // Mana efficiency
    score += dispeller.manaPercent * 0.3f;
    if (dispeller.manaPercent < m_config.minManaPctForDispel)
    {
        score -= 40.0f;  // Low mana penalty
    }

    // Range check
    if (!dispeller.inRange)
    {
        score -= 80.0f;  // Out of range
    }

    // Prefer healers for dispelling if configured
    if (m_config.preferHealersForDispel)
    {
        if (dispeller.botClass == CLASS_PRIEST ||
            dispeller.botClass == CLASS_DRUID ||
            dispeller.botClass == CLASS_PALADIN ||
            dispeller.botClass == CLASS_SHAMAN ||
            dispeller.botClass == CLASS_MONK)
        {
            score += 20.0f;

            // Extra bonus if target is low HP and dispeller is healer
    if (target.targetHealthPct < 50.0f)
            {
                score += 20.0f;
            }
        }
    }

    // Priority bonus for critical debuffs
    if (target.priority >= PRIORITY_INCAPACITATE)
    {
        score += 30.0f;
    }

    return score;
}

// ============================================================================
// Dispeller Capability Management
// ============================================================================
void DispelCoordinator::UpdateDispellerCapabilities()
{
    if (!m_group)
        return;

    m_dispellers.clear();

    for (GroupReference& itr : m_group->GetMembers())
    {
        Player* member = itr.GetSource();
        if (!member || member->isDead())
            continue;

        DispellerCapability cap;
        cap.botGuid = member->GetGUID();
        cap.botClass = static_cast<Classes>(member->GetClass());
        cap.canDispel = GetClassDispelTypes(cap.botClass);

        if (cap.canDispel.empty())
            continue;  // Can't dispel anything

        cap.manaPercent = member->GetPowerPct(POWER_MANA);
        cap.dispelCooldown = 0;  // Would need spell-specific tracking
        cap.lastDispelTime = 0;
        cap.globalCooldown = 0;
        cap.inRange = true;  // Will be updated per-target

        m_dispellers.push_back(cap);
    }
}

::std::vector<DispelType> DispelCoordinator::GetClassDispelTypes(Classes botClass) const
{
    ::std::vector<DispelType> types;

    switch (botClass)
    {
        case CLASS_PRIEST:
            types.push_back(DISPEL_MAGIC);
            types.push_back(DISPEL_DISEASE);
            break;

        case CLASS_PALADIN:
            types.push_back(DISPEL_MAGIC);
            types.push_back(DISPEL_DISEASE);
            types.push_back(DISPEL_POISON);
            break;

        case CLASS_DRUID:
            types.push_back(DISPEL_CURSE);
            types.push_back(DISPEL_POISON);
            types.push_back(DISPEL_MAGIC);  // With talent
            break;

        case CLASS_SHAMAN:
            types.push_back(DISPEL_CURSE);
            types.push_back(DISPEL_MAGIC);  // Purge for enemies
            types.push_back(DISPEL_POISON);  // With Cleanse Spirit
            types.push_back(DISPEL_DISEASE); // With improved Cleanse Spirit
            break;

        case CLASS_MAGE:
            types.push_back(DISPEL_CURSE);
            break;

        case CLASS_MONK:
            types.push_back(DISPEL_DISEASE);
            types.push_back(DISPEL_POISON);
            types.push_back(DISPEL_MAGIC);  // With talent
            break;

        case CLASS_WARLOCK:
            // Pet abilities - Felhunter can dispel magic
            types.push_back(DISPEL_MAGIC);
            break;

        case CLASS_EVOKER:
            types.push_back(DISPEL_MAGIC);
            types.push_back(DISPEL_POISON);
            types.push_back(DISPEL_DISEASE);
            types.push_back(DISPEL_CURSE);
            break;

        case CLASS_DEMON_HUNTER:
            types.push_back(DISPEL_MAGIC);  // Consume Magic (defensive)
            break;

        default:
            break;
    }

    return types;
}

// ============================================================================
// Dispel & Purge Spell Selection
// ============================================================================

uint32 DispelCoordinator::GetDispelSpell(DispelType dispelType) const
{
    if (!m_bot)
        return 0;

    Classes botClass = static_cast<Classes>(m_bot->GetClass());

    switch (botClass)
    {
        case CLASS_PRIEST:
            if (dispelType == DISPEL_MAGIC)
                return DispelSpells::PRIEST_DISPEL_MAGIC;
            if (dispelType == DISPEL_DISEASE)
                return DispelSpells::PRIEST_ABOLISH_DISEASE;
            break;

        case CLASS_PALADIN:
            return DispelSpells::PALADIN_CLEANSE;  // Handles magic, disease, poison

        case CLASS_DRUID:
            if (dispelType == DISPEL_CURSE)
                return DispelSpells::DRUID_REMOVE_CURSE;
            if (dispelType == DISPEL_POISON)
                return DispelSpells::DRUID_ABOLISH_POISON;
            if (dispelType == DISPEL_MAGIC)
                return DispelSpells::DRUID_NATURES_CURE;
            break;

        case CLASS_SHAMAN:
            if (dispelType == DISPEL_CURSE)
                return DispelSpells::SHAMAN_CLEANSE_SPIRIT;
            if (dispelType == DISPEL_MAGIC)
                return DispelSpells::SHAMAN_PURGE;  // For enemies
            break;

        case CLASS_MAGE:
            if (dispelType == DISPEL_CURSE)
                return DispelSpells::MAGE_REMOVE_CURSE;
            break;

        case CLASS_MONK:
            return DispelSpells::MONK_DETOX;  // Handles disease and poison

        case CLASS_EVOKER:
            return DispelSpells::EVOKER_CAUTERIZING_FLAME;

        default:
            break;
    }

    return 0;
}

uint32 DispelCoordinator::GetPurgeSpell() const
{
    if (!m_bot)
        return 0;

    switch (m_bot->GetClass())
    {
        case CLASS_SHAMAN:
            return DispelSpells::SHAMAN_PURGE;
        case CLASS_PRIEST:
            return DispelSpells::PRIEST_DISPEL_MAGIC;  // Can be used offensively
        case CLASS_DEMON_HUNTER:
            return DispelSpells::DEMON_HUNTER_CONSUME_MAGIC;
        case CLASS_WARLOCK:
            return DispelSpells::WARLOCK_DEVOUR_MAGIC;  // Pet ability
        case CLASS_MAGE:
            return 30449;  // Spellsteal (different mechanic but similar purpose)
        default:
            break;
    }

    return 0;
}

// ============================================================================
// Execution Functions
// ============================================================================

bool DispelCoordinator::ExecuteDispel()
{
    if (!m_bot || !m_currentAssignment.dispeller)
        return false;

    // Check if assignment is still valid
    if (m_currentAssignment.fulfilled)
        return false;

    uint32 now = GameTime::GetGameTimeMS();

    // Check GCD
    if (now < m_globalCooldownUntil)
        return false;

    // PHASE 5B: Thread-safe spatial grid validation (replaces ObjectAccessor::GetUnit)
    auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(m_bot, m_currentAssignment.target);
    Unit* target = nullptr;
    if (snapshot && snapshot->IsAlive())
    {
        // CRITICAL FIX: Actually assign the target pointer!
        // BUG: Previous code retrieved snapshot but never assigned target variable
        target = ObjectAccessor::GetUnit(*m_bot, m_currentAssignment.target);
    }

    if (!target || target->isDead())
    {
        m_currentAssignment.fulfilled = true;
        return false;
    }

    // Check if target still has the debuff
    if (!target->HasAura(m_currentAssignment.auraId))
    {
        m_currentAssignment.fulfilled = true;
        ++m_statistics.successfulDispels;
        return false;
    }

    // Get dispel spell
    uint32 dispelSpell = GetDispelSpell(m_currentAssignment.dispelType);
    if (!dispelSpell)
    {
        ++m_statistics.failedDispels;
        return false;
    }

    // Check range
    float maxRangeSq = m_config.maxDispelRange * m_config.maxDispelRange;
    if (!m_bot->IsWithinLOSInMap(target) ||
        m_bot->GetExactDistSq(target) > maxRangeSq)
    {
        return false;
    }

    // Check mana
    if (m_bot->GetPowerPct(POWER_MANA) < m_config.minManaPctForDispel)
        return false;

    // Cast dispel (MIGRATION COMPLETE 2025-10-30: Packet-based dispel)
    SpellPacketBuilder::BuildOptions options;
    options.skipGcdCheck = false;
    options.skipResourceCheck = false;
    options.skipTargetCheck = false;
    options.skipStateCheck = false;
    options.skipRangeCheck = false;
    options.logFailures = true;

    auto result = SpellPacketBuilder::BuildCastSpellPacket(m_bot, dispelSpell, target, options);
    if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
    {
        m_lastDispelAttempt = now;
        m_globalCooldownUntil = now + m_config.dispelGCD;
        m_currentAssignment.fulfilled = true;

        ++m_statistics.successfulDispels;
        ++m_statistics.dispelsByType[m_currentAssignment.dispelType];

        MarkDispelComplete(m_currentAssignment);

        TC_LOG_DEBUG("playerbot.dispel.packets",
                     "Bot {} queued CMSG_CAST_SPELL for dispel {} (target: {}, type: {})",
                     m_bot->GetName(), dispelSpell, target->GetName(),
                     static_cast<uint8>(m_currentAssignment.dispelType));
        return true;
    }
    else
    {
        TC_LOG_TRACE("playerbot.dispel.packets",
                     "Bot {} dispel {} validation failed: {} ({})",
                     m_bot->GetName(), dispelSpell,
                     static_cast<uint8>(result.result), result.failureReason);
    }

    ++m_statistics.failedDispels;
    return false;
}

bool DispelCoordinator::ExecutePurge()
{
    if (!m_bot)
        return false;

    uint32 now = GameTime::GetGameTimeMS();

    // Check GCD
    if (now < m_globalCooldownUntil)
        return false;

    // Rate limiting
    if (now - m_lastPurgeAttempt < 1000)
        return false;

    // Get purge targets
    ::std::vector<PurgeTarget> targets = GatherPurgeTargets();
    if (targets.empty())
        return false;

    // Get best target
    const PurgeTarget& bestTarget = targets[0];  // Already sorted by priority

    // PHASE 5B: Thread-safe spatial grid validation (replaces ObjectAccessor::GetUnit)
    auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(m_bot, bestTarget.enemyGuid);
    Unit* enemy = nullptr;
    if (snapshot && snapshot->IsAlive())
    {
        // CRITICAL FIX: Actually assign the enemy pointer!
        // BUG: Previous code retrieved snapshot but never assigned enemy variable
        enemy = ObjectAccessor::GetUnit(*m_bot, bestTarget.enemyGuid);
    }

    if (!enemy || enemy->isDead())
        return false;

    // Get purge spell
    uint32 purgeSpell = GetPurgeSpell();
    if (!purgeSpell)
        return false;

    // Check range
    float maxPurgeRangeSq = m_config.maxPurgeRange * m_config.maxPurgeRange;
    if (!m_bot->IsWithinLOSInMap(enemy) ||
        m_bot->GetExactDistSq(enemy) > maxPurgeRangeSq)
    {
        return false;
    }

    // Cast purge (MIGRATION COMPLETE 2025-10-30: Packet-based purge)
    SpellPacketBuilder::BuildOptions options;
    options.skipGcdCheck = false;
    options.skipResourceCheck = false;
    options.skipTargetCheck = false;
    options.skipStateCheck = false;
    options.skipRangeCheck = false;
    options.logFailures = true;

    auto result = SpellPacketBuilder::BuildCastSpellPacket(m_bot, purgeSpell, enemy, options);
    if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
    {
        m_lastPurgeAttempt = now;
        m_globalCooldownUntil = now + m_config.dispelGCD;
        ++m_statistics.successfulPurges;

        TC_LOG_DEBUG("playerbot.dispel.packets",
                     "Bot {} queued CMSG_CAST_SPELL for purge {} (target: {})",
                     m_bot->GetName(), purgeSpell, enemy->GetName());
        return true;
    }
    else
    {
        TC_LOG_TRACE("playerbot.dispel.packets",
                     "Bot {} purge {} validation failed: {} ({})",
                     m_bot->GetName(), purgeSpell,
                     static_cast<uint8>(result.result), result.failureReason);
    }

    ++m_statistics.failedPurges;
    return false;
}

// ============================================================================
// Gathering Functions
// ============================================================================

::std::vector<DispelCoordinator::DebuffTarget> DispelCoordinator::GatherGroupDebuffs() const
{
    ::std::vector<DebuffTarget> debuffs;

    if (!m_group)
        return debuffs;

    for (GroupReference& itr : m_group->GetMembers())
    {
        Player* member = itr.GetSource();
        if (!member || member->isDead())
            continue;

        // Check all auras
        Unit::AuraApplicationMap const& auras = member->GetAppliedAuras();
        for (auto const& [auraId, aurApp] : auras)
        {
            Aura* aura = aurApp->GetBase();
            if (!aura || aura->GetSpellInfo()->IsPositive())
                continue;

            // Check if this is a known debuff
            const DebuffData* debuffData = GetDebuffData(auraId);
            if (!debuffData)
                continue;

            DebuffTarget target;
            target.targetGuid = member->GetGUID();
            target.auraId = auraId;
            target.dispelType = debuffData->dispelType;
            target.priority = debuffData->basePriority;
            target.adjustedPriority = debuffData->GetAdjustedPriority(member);
            target.targetHealthPct = member->GetHealthPct();
            target.isTank = (GetPlayerRole(member) == BOT_ROLE_TANK);
            target.isHealer = (GetPlayerRole(member) == BOT_ROLE_HEALER);
            target.remainingDuration = aura->GetDuration();
            target.stackCount = aura->GetStackAmount();

            debuffs.push_back(target);

            // Update statistics
            ++const_cast<DispelCoordinator*>(this)->m_statistics.totalDebuffsDetected;
            ++const_cast<DispelCoordinator*>(this)->m_statistics.commonDebuffs[auraId];
        }
    }

    // Sort by adjusted priority
    ::std::sort(debuffs.begin(), debuffs.end(),
        [](const DebuffTarget& a, const DebuffTarget& b)
        {
            return a.adjustedPriority > b.adjustedPriority;
        });

    return debuffs;
}

::std::vector<DispelCoordinator::PurgeTarget> DispelCoordinator::GatherPurgeTargets() const
{
    ::std::vector<PurgeTarget> targets;

    if (!m_bot)
        return targets;

    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = m_bot->GetMap();
    if (!map)
        return targets;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        // Grid not yet created for this map - create it on demand
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return targets;
    }

    // PHASE 5B: Thread-safe spatial grid query (replaces QueryNearbyCreatureGuids + ObjectAccessor)
    float searchRange = static_cast<float>(m_config.maxPurgeRange);
    auto hostileSnapshots = SpatialGridQueryHelpers::FindHostileCreaturesInRange(m_bot, searchRange, true);

    // Resolve snapshots to Unit pointers and check for enemies
    for (auto const& snapshot : hostileSnapshots)
    {
        ::Unit* enemy = ObjectAccessor::GetUnit(*m_bot, snapshot.guid);
        if (!enemy || enemy->isDead())
            continue;

        // Check if enemy (must be hostile)
    if (!m_bot->IsHostileTo(enemy))
            continue;
        // Check all auras
        Unit::AuraApplicationMap const& auras = enemy->GetAppliedAuras();
        for (auto const& [auraId, aurApp] : auras)
        {
            Aura* aura = aurApp->GetBase();
            if (!aura || !aura->GetSpellInfo()->IsPositive())
                continue;

            // Check if this is purgeable
            const PurgeableBuff* buffData = GetPurgeableBuffData(auraId);
            if (!buffData)
                continue;

            // Skip if not worth purging
    if (!m_config.smartPurging || !EvaluatePurgeBenefit(*buffData, enemy))
                continue;

            PurgeTarget target;
            target.enemyGuid = enemy->GetGUID();
            target.auraId = auraId;
            target.priority = buffData->priority;
            target.isEnrage = buffData->isEnrage;
            target.isImmunity = buffData->providesImmunity;
            target.threatLevel = m_bot->GetThreatManager().GetThreat(enemy);
            target.distance = ::std::sqrt(m_bot->GetExactDistSq(enemy)); // Calculate once from squared distance

            targets.push_back(target);
        }
    }

    // Sort by priority, then by threat
    ::std::sort(targets.begin(), targets.end(),
        [](const PurgeTarget& a, const PurgeTarget& b)
        {
            if (a.priority != b.priority)
                return a.priority > b.priority;
            return a.threatLevel > b.threatLevel;
        });

    return targets;
}

// ============================================================================
// Helper Functions
// ============================================================================

bool DispelCoordinator::IsTank(Unit* unit) const
{
    Player* player = unit->ToPlayer();
    if (!player)
        return false;

    return GetPlayerRole(player) == BOT_ROLE_TANK;
}

bool DispelCoordinator::IsHealer(Unit* unit) const
{
    Player* player = unit->ToPlayer();
    if (!player)
        return false;

    return GetPlayerRole(player) == BOT_ROLE_HEALER;
}

uint32 DispelCoordinator::GetNearbyAlliesCount(Unit* center, float radius) const
{
    if (!center || !m_group)
        return 0;

    uint32 count = 0;
    for (GroupReference& itr : m_group->GetMembers())
    {
        Player* member = itr.GetSource();
        if (!member || member == center || member->isDead())
            continue;

        float radiusSq = radius * radius;
        if (center->GetExactDistSq(member) <= radiusSq)
            ++count;
    }

    return count;
}

bool DispelCoordinator::IsTankTakingDamage() const
{
    if (!m_group)
        return false;

    for (GroupReference& itr : m_group->GetMembers())
    {
        Player* member = itr.GetSource();
        if (!member || !IsTank(member))
            continue;

        // Simple check - tank below 70% health
    if (member->GetHealthPct() < 70.0f)
            return true;
    }

    return false;
}

bool DispelCoordinator::EvaluatePurgeBenefit(const PurgeableBuff& buff, Unit* enemy) const
{
    // Always purge immunities
    if (buff.providesImmunity)
        return true;

    // Always purge major buffs
    if (buff.priority >= PURGE_MAJOR_BUFF)
        return true;

    // Purge enrage if tank is taking damage
    if (buff.isEnrage && IsTankTakingDamage())
        return true;

    // Evaluate moderate buffs based on situation
    if (buff.priority == PURGE_MODERATE_BUFF)
    {
        // Purge damage increases if enemy is high threat
    if (buff.increasesDamage &&
            m_bot->GetThreatManager().GetThreat(enemy) > 1000)
            return true;

        // Purge healing increases if enemy can heal
    if (buff.increasesHealing)
            return true;
    }

    return false;
}

bool DispelCoordinator::IsBeingDispelled(ObjectGuid target, uint32 auraId) const
{
    for (const auto& assignment : m_assignments)
    {
        if (assignment.target == target &&
            assignment.auraId == auraId &&
            !assignment.fulfilled)
        {
            return true;
        }
    }
    return false;
}

void DispelCoordinator::MarkDispelComplete(const DispelAssignment& assignment)
{
    for (auto& assign : m_assignments)
    {
        if (assign.dispeller == assignment.dispeller &&
            assign.target == assignment.target &&
            assign.auraId == assignment.auraId)
        {
            assign.fulfilled = true;
            break;
        }
    }
}

void DispelCoordinator::MarkDispellerBusy(ObjectGuid dispeller, uint32 busyTimeMs)
{
    for (auto& disp : m_dispellers)
    {
        if (disp.botGuid == dispeller)
        {
            disp.globalCooldown = busyTimeMs;
            disp.lastDispelTime = GameTime::GetGameTimeMS();
            break;
        }
    }
}

bool DispelCoordinator::CanDispel(const DispellerCapability& dispeller, const DebuffTarget& target) const
{
    // Check if dispeller can handle this dispel type
    if (!dispeller.CanDispelType(target.dispelType))
        return false;

    // Check if on cooldown
    uint32 now = GameTime::GetGameTimeMS();
    if (dispeller.dispelCooldown > 0 &&
        now - dispeller.lastDispelTime < dispeller.dispelCooldown)
        return false;

    // Check GCD
    if (dispeller.globalCooldown > 0)
        return false;

    // Check mana
    if (dispeller.manaPercent < m_config.minManaPctForDispel)
        return false;

    return true;
}

void DispelCoordinator::CleanupAssignments()
{
    uint32 now = GameTime::GetGameTimeMS();

    // Remove expired or fulfilled assignments
    m_assignments.erase(
        ::std::remove_if(m_assignments.begin(), m_assignments.end(),
            [this, now](const DispelAssignment& assign)
            {
                if (assign.fulfilled)
                    return true;

                if (now - assign.assignedTime > m_config.assignmentTimeout)
                {
                    ++m_statistics.assignmentsExpired;
                    return true;
                }

                return false;
            }),
        m_assignments.end());

    // Clear current assignment if fulfilled or expired
    if (m_currentAssignment.fulfilled ||
        (m_currentAssignment.assignedTime > 0 &&
         now - m_currentAssignment.assignedTime > m_config.assignmentTimeout))
    {
        m_currentAssignment = DispelAssignment();
    }
}

// ============================================================================
// Public Interface Functions
// ============================================================================

void DispelCoordinator::RegisterDebuff(ObjectGuid target, uint32 auraId)
{
    // Trigger immediate rescan if high priority
    const DebuffData* data = GetDebuffData(auraId);
    if (data && data->basePriority >= PRIORITY_INCAPACITATE)
    {
        UpdateDispelAssignments();
    }
}

bool DispelCoordinator::ShouldDispel(uint32 auraId) const
{
    const DebuffData* data = GetDebuffData(auraId);
    return data && data->basePriority >= PRIORITY_MODERATE;
}

DispelCoordinator::DispelAssignment DispelCoordinator::GetDispelAssignment() const
{
    return m_currentAssignment;
}

bool DispelCoordinator::ShouldPurge(Unit* enemy, uint32 auraId) const
{
    if (!enemy)
        return false;

    const PurgeableBuff* data = GetPurgeableBuffData(auraId);
    if (!data)
        return false;

    return EvaluatePurgeBenefit(*data, enemy);
}

DispelCoordinator::PurgeTarget DispelCoordinator::GetPurgeTarget() const
{
    ::std::vector<PurgeTarget> targets = GatherPurgeTargets();
    if (!targets.empty())
        return targets[0];

    return PurgeTarget();
}

} // namespace Playerbot