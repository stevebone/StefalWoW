/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "CombatPhaseDetector.h"
#include "Player.h"
#include "Unit.h"
#include "SpellInfo.h"
#include "GameTime.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot
{

// ============================================================================
// CONSTRUCTOR
// ============================================================================

CombatPhaseDetector::CombatPhaseDetector(Player* bot)
    : _bot(bot)
{
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void CombatPhaseDetector::Initialize()
{
    if (_initialized)
        return;

    LoadSpecConfig();
    _recommendation.Reset();
    _initialized = true;
}

void CombatPhaseDetector::OnCombatStart()
{
    if (!_initialized)
        Initialize();

    _inCombat = true;
    _combatStartTimeMs = GameTime::GetGameTimeMS();
    _recommendation.Reset();
    _recommendation.phase = DetectedCombatPhase::OPENER;
    _recommendation.guidance = PhaseGuidance::OPENER_SEQUENCE;
    _recommendation.inOpenerWindow = true;
    _recommendation.openerTimeRemainingSec = _specConfig.openerDurationSec;

    if (!_specConfig.openerSpellIds.empty())
        _recommendation.prioritySpells = &_specConfig.openerSpellIds;
    else if (!_specConfig.openerBurstCDs.empty())
        _recommendation.prioritySpells = &_specConfig.openerBurstCDs;

    TC_LOG_DEBUG("module.playerbot", "CombatPhaseDetector: {} entered combat, opener window {:.1f}s, "
                 "execute threshold {:.0f}%",
                 _bot->GetName(), _specConfig.openerDurationSec, _specConfig.executeThresholdPct);
}

void CombatPhaseDetector::OnCombatEnd()
{
    _inCombat = false;
    _combatStartTimeMs = 0;
    _recommendation.Reset();
}

// ============================================================================
// UPDATE
// ============================================================================

void CombatPhaseDetector::Update(uint32 diff)
{
    if (!_initialized || !_inCombat)
        return;

    _updateTimer += diff;
    if (_updateTimer < UPDATE_INTERVAL_MS)
        return;
    _updateTimer = 0;

    DetectPhase();
}

// ============================================================================
// PHASE DETECTION
// ============================================================================

void CombatPhaseDetector::DetectPhase()
{
    uint32 now = GameTime::GetGameTimeMS();
    float timeSinceCombatSec = static_cast<float>(now - _combatStartTimeMs) / 1000.0f;
    _recommendation.timeSinceCombatStartSec = timeSinceCombatSec;

    // Get target health
    Unit* target = GetTarget();
    float targetHp = 100.0f;
    if (target && target->IsAlive())
        targetHp = target->GetHealthPct();
    _recommendation.targetHealthPct = targetHp;

    // Determine phase based on combat state

    // Phase 1: Check for finishing (target about to die)
    if (targetHp <= _specConfig.finishingThresholdPct && target && target->IsAlive())
    {
        _recommendation.phase = DetectedCombatPhase::FINISHING;
        _recommendation.guidance = PhaseGuidance::ALL_OUT_BURN;
        _recommendation.inExecutePhase = true;
        _recommendation.isFinishing = true;
        _recommendation.inOpenerWindow = false;
        _recommendation.openerTimeRemainingSec = 0.0f;
        _recommendation.executeThreshold = _specConfig.executeThresholdPct;
        _recommendation.prioritySpells = _specConfig.executeSpellIds.empty()
            ? nullptr : &_specConfig.executeSpellIds;
        return;
    }

    // Phase 2: Check for execute phase
    if (targetHp <= _specConfig.executeThresholdPct && target && target->IsAlive())
    {
        _recommendation.phase = DetectedCombatPhase::EXECUTE;
        _recommendation.guidance = PhaseGuidance::EXECUTE_SPAM;
        _recommendation.inExecutePhase = true;
        _recommendation.isFinishing = false;
        _recommendation.inOpenerWindow = false;
        _recommendation.openerTimeRemainingSec = 0.0f;
        _recommendation.executeThreshold = _specConfig.executeThresholdPct;
        _recommendation.prioritySpells = _specConfig.executeSpellIds.empty()
            ? nullptr : &_specConfig.executeSpellIds;
        return;
    }

    // Phase 3: Check for opener window
    if (timeSinceCombatSec < _specConfig.openerDurationSec)
    {
        _recommendation.phase = DetectedCombatPhase::OPENER;
        _recommendation.guidance = PhaseGuidance::USE_BURST_CDS;
        _recommendation.inExecutePhase = false;
        _recommendation.isFinishing = false;
        _recommendation.inOpenerWindow = true;
        _recommendation.openerTimeRemainingSec = _specConfig.openerDurationSec - timeSinceCombatSec;

        if (!_specConfig.openerSpellIds.empty())
            _recommendation.prioritySpells = &_specConfig.openerSpellIds;
        else if (!_specConfig.openerBurstCDs.empty())
            _recommendation.prioritySpells = &_specConfig.openerBurstCDs;
        else
            _recommendation.prioritySpells = nullptr;
        return;
    }

    // Phase 4: Sustained combat
    _recommendation.phase = DetectedCombatPhase::SUSTAINED;
    _recommendation.guidance = PhaseGuidance::NORMAL;
    _recommendation.inExecutePhase = false;
    _recommendation.isFinishing = false;
    _recommendation.inOpenerWindow = false;
    _recommendation.openerTimeRemainingSec = 0.0f;
    _recommendation.prioritySpells = nullptr;
}

// ============================================================================
// QUERIES
// ============================================================================

bool CombatPhaseDetector::ShouldPrioritizeSpell(uint32 spellId) const
{
    if (!_recommendation.prioritySpells)
        return false;

    return ::std::find(_recommendation.prioritySpells->begin(),
                       _recommendation.prioritySpells->end(),
                       spellId) != _recommendation.prioritySpells->end();
}

bool CombatPhaseDetector::IsExecuteAbility(uint32 spellId) const
{
    return ::std::find(_specConfig.executeSpellIds.begin(),
                       _specConfig.executeSpellIds.end(),
                       spellId) != _specConfig.executeSpellIds.end();
}

float CombatPhaseDetector::GetExecuteThreshold() const
{
    return _specConfig.executeThresholdPct;
}

Unit* CombatPhaseDetector::GetTarget() const
{
    if (!_bot)
        return nullptr;

    ObjectGuid targetGuid = _bot->GetTarget();
    if (targetGuid.IsEmpty())
        return nullptr;

    return ObjectAccessor::GetUnit(*_bot, targetGuid);
}

// ============================================================================
// SPEC CONFIGURATION LOADER
// ============================================================================

void CombatPhaseDetector::LoadSpecConfig()
{
    if (!_bot)
        return;

    ChrSpecialization spec = _bot->GetPrimarySpecialization();
    _specConfig = SpecPhaseConfig{};
    _specConfig.spec = spec;

    switch (spec)
    {
        // ====================================================================
        // WARRIOR
        // ====================================================================
        case ChrSpecialization::WarriorArms:
            _specConfig.specName = "Arms Warrior";
            _specConfig.openerDurationSec = 6.0f;
            _specConfig.executeThresholdPct = 35.0f;  // Arms Execute works below 35% with Massacre talent
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = true;
            _specConfig.executeSpellIds = {
                163201,     // Execute (Arms)
                227847,     // Bladestorm
                260708      // Sweeping Strikes (more value in execute)
            };
            _specConfig.openerBurstCDs = {
                107574,     // Avatar
                227847,     // Bladestorm
                260708      // Sweeping Strikes
            };
            break;

        case ChrSpecialization::WarriorFury:
            _specConfig.specName = "Fury Warrior";
            _specConfig.openerDurationSec = 6.0f;
            _specConfig.executeThresholdPct = 20.0f;  // Fury Execute at 20%
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = true;
            _specConfig.executeSpellIds = {
                5308,       // Execute (Fury)
                280735      // Execute (off-hand)
            };
            _specConfig.openerBurstCDs = {
                1719,       // Recklessness
                107574      // Avatar
            };
            break;

        case ChrSpecialization::WarriorProtection:
            _specConfig.specName = "Protection Warrior";
            _specConfig.openerDurationSec = 4.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = true;
            _specConfig.executeSpellIds = { 163201 };  // Execute
            _specConfig.openerBurstCDs = { 107574 };   // Avatar
            break;

        // ====================================================================
        // PALADIN
        // ====================================================================
        case ChrSpecialization::PaladinHoly:
            _specConfig.specName = "Holy Paladin";
            _specConfig.openerDurationSec = 3.0f;
            _specConfig.executeThresholdPct = 20.0f;  // No special execute
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            _specConfig.openerBurstCDs = { 31884 };   // Avenging Wrath
            break;

        case ChrSpecialization::PaladinProtection:
            _specConfig.specName = "Protection Paladin";
            _specConfig.openerDurationSec = 4.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = true;
            _specConfig.executeSpellIds = { 24275 };   // Hammer of Wrath (below 20%)
            _specConfig.openerBurstCDs = { 31884 };    // Avenging Wrath
            break;

        case ChrSpecialization::PaladinRetribution:
            _specConfig.specName = "Retribution Paladin";
            _specConfig.openerDurationSec = 6.0f;
            _specConfig.executeThresholdPct = 20.0f;   // Hammer of Wrath below 20%
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = true;
            _specConfig.executeSpellIds = {
                24275,      // Hammer of Wrath
                255937      // Wake of Ashes
            };
            _specConfig.openerBurstCDs = {
                31884,      // Avenging Wrath
                255937      // Wake of Ashes
            };
            break;

        // ====================================================================
        // HUNTER
        // ====================================================================
        case ChrSpecialization::HunterBeastMastery:
            _specConfig.specName = "Beast Mastery Hunter";
            _specConfig.openerDurationSec = 6.0f;
            _specConfig.executeThresholdPct = 20.0f;   // Kill Shot below 20%
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = true;
            _specConfig.executeSpellIds = { 53351 };   // Kill Shot
            _specConfig.openerBurstCDs = {
                19574,      // Bestial Wrath
                359844      // Call of the Wild
            };
            break;

        case ChrSpecialization::HunterMarksmanship:
            _specConfig.specName = "Marksmanship Hunter";
            _specConfig.openerDurationSec = 8.0f;      // Longer opener with Trueshot
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = true;
            _specConfig.executeSpellIds = {
                53351,      // Kill Shot
                257044      // Rapid Fire (more procs in execute)
            };
            _specConfig.openerBurstCDs = {
                288613,     // Trueshot
                257044      // Rapid Fire
            };
            break;

        case ChrSpecialization::HunterSurvival:
            _specConfig.specName = "Survival Hunter";
            _specConfig.openerDurationSec = 6.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = true;
            _specConfig.executeSpellIds = { 53351 };   // Kill Shot
            _specConfig.openerBurstCDs = { 360952 };   // Coordinated Assault
            break;

        // ====================================================================
        // ROGUE
        // ====================================================================
        case ChrSpecialization::RogueAssassination:
            _specConfig.specName = "Assassination Rogue";
            _specConfig.openerDurationSec = 5.0f;
            _specConfig.executeThresholdPct = 30.0f;   // Blindside procs, Kingsbane burst
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasStealthOpener = true;
            _specConfig.hasExecuteAbility = true;
            _specConfig.executeSpellIds = {
                328547      // Blindside (empowered Ambush below 30%)
            };
            _specConfig.openerSpellIds = {
                703,        // Garrote (from stealth, silence)
                1943,       // Rupture (setup DoTs)
                79140       // Vendetta (burst CD)
            };
            _specConfig.openerBurstCDs = { 79140 };    // Vendetta
            _specConfig.poolBeforeExecute = true;
            _specConfig.executeResourceTarget = 0.9f;
            break;

        case ChrSpecialization::RogueOutlaw:
            _specConfig.specName = "Outlaw Rogue";
            _specConfig.openerDurationSec = 5.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasStealthOpener = true;
            _specConfig.hasExecuteAbility = false;
            _specConfig.openerSpellIds = {
                8676,       // Ambush (from stealth)
                315508      // Roll the Bones
            };
            _specConfig.openerBurstCDs = {
                13750,      // Adrenaline Rush
                343142      // Dreadblades
            };
            break;

        case ChrSpecialization::RogueSubtely:  // TC misspelling: RogueSubtely
            _specConfig.specName = "Subtlety Rogue";
            _specConfig.openerDurationSec = 6.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasStealthOpener = true;
            _specConfig.hasExecuteAbility = false;
            _specConfig.openerSpellIds = {
                185438,     // Shadowstrike (from stealth/Shadow Dance)
                277925,     // Symbols of Death
                121471      // Shadow Blades
            };
            _specConfig.openerBurstCDs = {
                121471,     // Shadow Blades
                277925      // Symbols of Death
            };
            break;

        // ====================================================================
        // PRIEST
        // ====================================================================
        case ChrSpecialization::PriestDiscipline:
            _specConfig.specName = "Discipline Priest";
            _specConfig.openerDurationSec = 3.0f;
            _specConfig.executeThresholdPct = 20.0f;   // Shadow Word: Death
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = true;
            _specConfig.executeSpellIds = { 32379 };   // Shadow Word: Death
            break;

        case ChrSpecialization::PriestHoly:
            _specConfig.specName = "Holy Priest";
            _specConfig.openerDurationSec = 3.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = true;
            _specConfig.executeSpellIds = { 32379 };   // Shadow Word: Death
            break;

        case ChrSpecialization::PriestShadow:
            _specConfig.specName = "Shadow Priest";
            _specConfig.openerDurationSec = 8.0f;      // Long opener with VF/Dark Ascension
            _specConfig.executeThresholdPct = 20.0f;    // SW:Death refund below 20%
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = true;
            _specConfig.executeSpellIds = {
                32379,      // Shadow Word: Death (below 20% for insanity refund)
                263165      // Void Torrent (more value to finish)
            };
            _specConfig.openerBurstCDs = {
                228260,     // Void Eruption
                391109      // Dark Ascension
            };
            _specConfig.openerSpellIds = {
                589,        // Shadow Word: Pain (setup DoTs)
                34914,      // Vampiric Touch
                228260      // Void Eruption
            };
            break;

        // ====================================================================
        // DEATH KNIGHT
        // ====================================================================
        case ChrSpecialization::DeathKnightBlood:
            _specConfig.specName = "Blood Death Knight";
            _specConfig.openerDurationSec = 4.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            _specConfig.openerBurstCDs = { 49028 };    // Dancing Rune Weapon
            break;

        case ChrSpecialization::DeathKnightFrost:
            _specConfig.specName = "Frost Death Knight";
            _specConfig.openerDurationSec = 7.0f;      // Pillar + Breath opener
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            _specConfig.openerBurstCDs = {
                51271,      // Pillar of Frost
                152279      // Breath of Sindragosa
            };
            _specConfig.poolBeforeExecute = false;
            break;

        case ChrSpecialization::DeathKnightUnholy:
            _specConfig.specName = "Unholy Death Knight";
            _specConfig.openerDurationSec = 6.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = true;
            _specConfig.executeSpellIds = { 343294 };  // Soul Reaper (below 35% bonus damage)
            _specConfig.openerBurstCDs = {
                63560,      // Dark Transformation
                275699      // Apocalypse
            };
            _specConfig.openerSpellIds = {
                77575,      // Outbreak (diseases)
                63560,      // Dark Transformation
                275699      // Apocalypse
            };
            // Soul Reaper actually triggers at 35%
            _specConfig.executeThresholdPct = 35.0f;
            break;

        // ====================================================================
        // SHAMAN
        // ====================================================================
        case ChrSpecialization::ShamanElemental:
            _specConfig.specName = "Elemental Shaman";
            _specConfig.openerDurationSec = 6.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            _specConfig.hasPrepull = true;             // Pre-cast Stormkeeper
            _specConfig.openerBurstCDs = {
                114050,     // Ascendance
                191634      // Stormkeeper
            };
            _specConfig.openerSpellIds = {
                191634,     // Stormkeeper (prepull)
                188196,     // Lightning Bolt (empowered)
                114050      // Ascendance
            };
            break;

        case ChrSpecialization::ShamanEnhancement:
            _specConfig.specName = "Enhancement Shaman";
            _specConfig.openerDurationSec = 5.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            _specConfig.openerBurstCDs = {
                51533,      // Feral Spirit
                114051      // Ascendance (Enhancement)
            };
            break;

        case ChrSpecialization::ShamanRestoration:
            _specConfig.specName = "Restoration Shaman";
            _specConfig.openerDurationSec = 3.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            break;

        // ====================================================================
        // MAGE
        // ====================================================================
        case ChrSpecialization::MageArcane:
            _specConfig.specName = "Arcane Mage";
            _specConfig.openerDurationSec = 8.0f;      // Arcane Surge window
            _specConfig.executeThresholdPct = 35.0f;    // Arcane Barrage gains damage below 35%
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            _specConfig.openerBurstCDs = {
                365350,     // Arcane Surge
                12042       // Arcane Power
            };
            _specConfig.openerSpellIds = {
                365350,     // Arcane Surge
                44425       // Arcane Barrage (at max charges)
            };
            _specConfig.poolBeforeExecute = true;
            _specConfig.executeResourceTarget = 0.95f;
            break;

        case ChrSpecialization::MageFire:
            _specConfig.specName = "Fire Mage";
            _specConfig.openerDurationSec = 8.0f;      // Combustion window
            _specConfig.executeThresholdPct = 30.0f;    // Scorch execute (below 30%)
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = true;
            _specConfig.executeSpellIds = {
                2948        // Scorch (castable while moving below 30%, replaces Fireball)
            };
            _specConfig.openerBurstCDs = {
                190319      // Combustion
            };
            _specConfig.openerSpellIds = {
                190319,     // Combustion
                11366,      // Pyroblast (hardcast pre-pull)
                108853      // Fire Blast (instant)
            };
            _specConfig.hasPrepull = true;             // Prepull Pyroblast
            break;

        case ChrSpecialization::MageFrost:
            _specConfig.specName = "Frost Mage";
            _specConfig.openerDurationSec = 6.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            _specConfig.openerBurstCDs = { 12472 };    // Icy Veins
            break;

        // ====================================================================
        // WARLOCK
        // ====================================================================
        case ChrSpecialization::WarlockAffliction:
            _specConfig.specName = "Affliction Warlock";
            _specConfig.openerDurationSec = 8.0f;      // DoT setup phase
            _specConfig.executeThresholdPct = 20.0f;    // Drain Soul execute
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = true;
            _specConfig.executeSpellIds = {
                198590      // Drain Soul (deals 4x damage below 20%)
            };
            _specConfig.openerBurstCDs = { 205180 };   // Darkglare
            _specConfig.openerSpellIds = {
                980,        // Agony
                316099,     // Unstable Affliction
                172,        // Corruption
                205180      // Darkglare (after DoTs)
            };
            break;

        case ChrSpecialization::WarlockDemonology:
            _specConfig.specName = "Demonology Warlock";
            _specConfig.openerDurationSec = 7.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            _specConfig.openerBurstCDs = { 265187 };   // Demonic Tyrant
            _specConfig.openerSpellIds = {
                104316,     // Call Dreadstalkers
                105174,     // Hand of Gul'dan
                265187      // Demonic Tyrant (after demons)
            };
            _specConfig.poolBeforeExecute = false;
            break;

        case ChrSpecialization::WarlockDestruction:
            _specConfig.specName = "Destruction Warlock";
            _specConfig.openerDurationSec = 6.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            _specConfig.openerBurstCDs = { 1122 };     // Summon Infernal
            _specConfig.openerSpellIds = {
                348,        // Immolate
                116858,     // Chaos Bolt
                1122        // Summon Infernal
            };
            break;

        // ====================================================================
        // MONK
        // ====================================================================
        case ChrSpecialization::MonkBrewmaster:
            _specConfig.specName = "Brewmaster Monk";
            _specConfig.openerDurationSec = 4.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            break;

        case ChrSpecialization::MonkMistweaver:
            _specConfig.specName = "Mistweaver Monk";
            _specConfig.openerDurationSec = 3.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            break;

        case ChrSpecialization::MonkWindwalker:
            _specConfig.specName = "Windwalker Monk";
            _specConfig.openerDurationSec = 6.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            _specConfig.openerBurstCDs = {
                137639,     // Storm, Earth, and Fire
                152173      // Serenity
            };
            _specConfig.openerSpellIds = {
                137639,     // SEF
                113656,     // Fists of Fury
                107428      // Rising Sun Kick
            };
            break;

        // ====================================================================
        // DRUID
        // ====================================================================
        case ChrSpecialization::DruidBalance:
            _specConfig.specName = "Balance Druid";
            _specConfig.openerDurationSec = 8.0f;      // CA/Inc opener
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            _specConfig.hasPrepull = true;             // Prepull Wrath
            _specConfig.openerBurstCDs = {
                194223,     // Celestial Alignment
                102560      // Incarnation: Chosen of Elune
            };
            _specConfig.openerSpellIds = {
                190984,     // Wrath (prepull)
                93402,      // Sunfire
                8921,       // Moonfire
                194223      // Celestial Alignment
            };
            break;

        case ChrSpecialization::DruidFeral:
            _specConfig.specName = "Feral Druid";
            _specConfig.openerDurationSec = 6.0f;
            _specConfig.executeThresholdPct = 25.0f;   // Ferocious Bite refunds energy below 25%
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasStealthOpener = true;       // Prowl opener
            _specConfig.hasExecuteAbility = true;
            _specConfig.executeSpellIds = {
                22568       // Ferocious Bite (refunds energy below 25%)
            };
            _specConfig.openerSpellIds = {
                1822,       // Rake (from stealth, stuns)
                5217,       // Tiger's Fury
                106951      // Berserk
            };
            _specConfig.openerBurstCDs = {
                106951,     // Berserk
                102543      // Incarnation: Avatar of Ashamane
            };
            break;

        case ChrSpecialization::DruidGuardian:
            _specConfig.specName = "Guardian Druid";
            _specConfig.openerDurationSec = 4.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            break;

        case ChrSpecialization::DruidRestoration:
            _specConfig.specName = "Restoration Druid";
            _specConfig.openerDurationSec = 3.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            break;

        // ====================================================================
        // DEMON HUNTER
        // ====================================================================
        case ChrSpecialization::DemonHunterHavoc:
            _specConfig.specName = "Havoc Demon Hunter";
            _specConfig.openerDurationSec = 6.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            _specConfig.openerBurstCDs = {
                191427,     // Metamorphosis
                258920      // Immolation Aura
            };
            _specConfig.openerSpellIds = {
                191427,     // Metamorphosis
                258920,     // Immolation Aura
                162794      // Chaos Strike
            };
            break;

        case ChrSpecialization::DemonHunterVengeance:
            _specConfig.specName = "Vengeance Demon Hunter";
            _specConfig.openerDurationSec = 4.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            break;

        // ====================================================================
        // EVOKER
        // ====================================================================
        case ChrSpecialization::EvokerDevastation:
            _specConfig.specName = "Devastation Evoker";
            _specConfig.openerDurationSec = 7.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            _specConfig.openerBurstCDs = { 375087 };   // Dragonrage
            _specConfig.openerSpellIds = {
                375087,     // Dragonrage
                357208,     // Fire Breath
                382266      // Eternity Surge
            };
            break;

        case ChrSpecialization::EvokerPreservation:
            _specConfig.specName = "Preservation Evoker";
            _specConfig.openerDurationSec = 3.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            break;

        case ChrSpecialization::EvokerAugmentation:
            _specConfig.specName = "Augmentation Evoker";
            _specConfig.openerDurationSec = 6.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            _specConfig.openerBurstCDs = { 395152 };   // Ebon Might
            _specConfig.openerSpellIds = {
                395152,     // Ebon Might
                396286,     // Upheaval
                360995      // Eruption
            };
            break;

        // ====================================================================
        // DEFAULT
        // ====================================================================
        default:
            _specConfig.specName = "Unknown Spec";
            _specConfig.openerDurationSec = 5.0f;
            _specConfig.executeThresholdPct = 20.0f;
            _specConfig.finishingThresholdPct = 5.0f;
            _specConfig.hasExecuteAbility = false;
            break;
    }

    TC_LOG_DEBUG("module.playerbot", "CombatPhaseDetector: Loaded config for {} - opener {:.1f}s, "
                 "execute {:.0f}%, {} execute spells, {} opener spells, stealth={}",
                 _specConfig.specName, _specConfig.openerDurationSec,
                 _specConfig.executeThresholdPct,
                 _specConfig.executeSpellIds.size(),
                 _specConfig.openerSpellIds.size(),
                 _specConfig.hasStealthOpener);
}

} // namespace Playerbot
