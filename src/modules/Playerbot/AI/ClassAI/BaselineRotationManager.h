/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Baseline Rotation Manager - Handles Combat for Unspecialized Bots (Levels 1-9)
 *
 * This system provides fallback combat rotations for bots that haven't yet chosen
 * a specialization at level 10. Each class has baseline abilities available from
 * levels 1-9 that provide basic combat functionality.
 *
 * DESIGN PRINCIPLES:
 * - Module-only implementation (no core modifications)
 * - Automatic specialization selection at level 10
 * - Simple but effective rotations using baseline abilities
 * - Graceful fallback for edge cases
 * - Performance: <0.05% CPU per bot
 */

#pragma once

#include "Player.h"
#include "Unit.h"
#include "ClassAI.h"
#include <unordered_map>
#include <vector>
#include <functional>

namespace Playerbot
{

/**
 * Represents a baseline ability available before specialization
 */
struct BaselineAbility
{
    uint32 spellId;
    uint32 minLevel;          // Minimum level to use
    uint32 resourceCost;      // Resource cost (rage, mana, etc.)
    uint32 cooldown;          // Cooldown in milliseconds
    float priority;           // Priority in rotation (higher = more important)
    bool requiresMelee;       // Requires melee range
    bool isDefensive;         // Defensive ability
    bool isUtility;           // Utility ability

    BaselineAbility(uint32 spell, uint32 lvl, uint32 cost, uint32 cd, float pri,
                    bool melee = true, bool defensive = false, bool utility = false)
        : spellId(spell), minLevel(lvl), resourceCost(cost), cooldown(cd)
        , priority(pri), requiresMelee(melee), isDefensive(defensive), isUtility(utility)
    {}
};

/**
 * Manages baseline combat rotations for unspecialized bots
 *
 * Key Features:
 * - Level-appropriate ability usage
 * - Simple priority-based rotations
 * - Resource management
 * - Auto-specialization at level 10
 */
class TC_GAME_API BaselineRotationManager
{
public:
    BaselineRotationManager();
    ~BaselineRotationManager() = default;

    /**
     * Check if bot needs baseline rotation (level < 10 or no spec)
     * @param bot Player bot
     * @return true if baseline rotation should be used
     */
    static bool ShouldUseBaselineRotation(Player* bot);

    /**
     * Execute baseline rotation for bot
     * @param bot Player bot
     * @param target Combat target
     * @return true if any ability was cast
     */
    bool ExecuteBaselineRotation(Player* bot, ::Unit* target);

    /**
     * Apply baseline buffs
     * @param bot Player bot
     */
    void ApplyBaselineBuffs(Player* bot);

    /**
     * Handle auto-specialization at level 10
     * @param bot Player bot
     * @return true if specialization was selected
     */
    bool HandleAutoSpecialization(Player* bot);

    /**
     * Get optimal range for baseline rotation
     * @param bot Player bot
     * @return Optimal combat range
     */
    static float GetBaselineOptimalRange(Player* bot);

    /**
     * THREAD-SAFE spell casting via packet queue
     *
     * CRITICAL: This method is safe to call from worker threads.
     * It uses SpellPacketBuilder to queue a CMSG_CAST_SPELL packet
     * that will be processed on the main thread.
     *
     * DO NOT use bot->CastSpell() directly from worker threads!
     *
     * @param bot Player bot casting the spell
     * @param spellId Spell ID to cast
     * @param target Target unit (can be bot itself for self-cast)
     * @return true if packet was successfully queued
     */
    static bool QueueSpellCast(Player* bot, uint32 spellId, ::Unit* target);

private:
    // Initialize baseline abilities for each class
    void InitializeWarriorBaseline();
    void InitializePaladinBaseline();
    void InitializeHunterBaseline();
    void InitializeRogueBaseline();
    void InitializePriestBaseline();
    void InitializeDeathKnightBaseline();
    void InitializeShamanBaseline();
    void InitializeMageBaseline();
    void InitializeWarlockBaseline();
    void InitializeMonkBaseline();
    void InitializeDruidBaseline();
    void InitializeDemonHunterBaseline();
    void InitializeEvokerBaseline();

    /**
     * Get baseline abilities for a specific class
     * @param classId Class ID
     * @return Vector of baseline abilities
     */
    ::std::vector<BaselineAbility> const* GetBaselineAbilities(uint8 classId) const;

    /**
     * Cast ability if conditions are met
     * @param bot Player bot
     * @param target Combat target
     * @param ability Ability to cast
     * @return true if ability was cast
     */
    bool TryCastAbility(Player* bot, ::Unit* target, BaselineAbility const& ability);

    /**
     * Check if bot can use ability
     * @param bot Player bot
     * @param target Combat target
     * @param ability Ability to check
     * @return true if ability can be used
     */
    bool CanUseAbility(Player* bot, ::Unit* target, BaselineAbility const& ability) const;

    /**
     * Auto-select best specialization based on class and role preference
     * @param bot Player bot
     * @return Specialization ID (spec-specific enum value)
     */
    uint32 SelectOptimalSpecialization(Player* bot) const;

    // Baseline abilities by class
    ::std::unordered_map<uint8, ::std::vector<BaselineAbility>> _baselineAbilities;

    // Cooldown tracking for baseline abilities
    ::std::unordered_map<uint32 /*bot GUID*/, ::std::unordered_map<uint32 /*spell ID*/, uint32 /*expiry time*/>> _cooldowns;
};

/**
 * Warrior Baseline Rotation Helper
 *
 * Warrior levels 1-9 baseline abilities:
 * - Level 1: Charge (100), Slam (1464)
 * - Level 3: Victory Rush (34428)
 * - Level 7: Hamstring (1715)
 * - Level 9: Execute (5308)
 *
 * Simple rotation:
 * 1. Charge to engage if not in melee
 * 2. Execute if target < 20% health
 * 3. Victory Rush if available (after killing blow or low health)
 * 4. Slam as rage dump
 * 5. Hamstring to prevent fleeing
 */
class TC_GAME_API WarriorBaselineRotation
{
public:
    static bool ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager);
    static void ApplyBuffs(Player* bot);

private:
    enum WarriorBaseline
    {
        CHARGE = 100,
        SLAM = 1464,
        VICTORY_RUSH = 34428,
        HAMSTRING = 1715,
        EXECUTE = 5308,
        BATTLE_SHOUT = 6673
    };
};

/**
 * Paladin Baseline Rotation Helper
 *
 * Paladin levels 1-9:
 * - Level 1: Crusader Strike (35395)
 * - Level 3: Judgment (20271)
 * - Level 5: Word of Glory (85673)
 * - Level 7: Blessing of Protection (1022)
 * - Level 9: Hammer of Justice (853)
 */
class TC_GAME_API PaladinBaselineRotation
{
public:
    static bool ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager);
    static void ApplyBuffs(Player* bot);

private:
    enum PaladinBaseline
    {
        CRUSADER_STRIKE = 35395,
        JUDGMENT = 20271,
        WORD_OF_GLORY = 85673,
        HAMMER_OF_JUSTICE = 853,
        BLESSING_OF_PROTECTION = 1022
    };
};

/**
 * Hunter Baseline Rotation Helper
 *
 * Hunter levels 1-9:
 * - Level 1: Aimed Shot (19434), Auto Shot
 * - Level 3: Arcane Shot (185358)
 * - Level 5: Kill Command (34026)
 * - Level 7: Concussive Shot (5116)
 * - Level 9: Steady Shot (56641)
 */
class TC_GAME_API HunterBaselineRotation
{
public:
    static bool ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager);
    static void ApplyBuffs(Player* bot);

private:
    enum HunterBaseline
    {
        AIMED_SHOT = 19434,
        ARCANE_SHOT = 185358,
        KILL_COMMAND = 34026,
        STEADY_SHOT = 56641,
        CONCUSSIVE_SHOT = 5116
    };
};

/**
 * Rogue Baseline Rotation Helper
 *
 * Rogue levels 1-9:
 * - Level 1: Sinister Strike (1752), Stealth (1784)
 * - Level 3: Eviscerate (196819)
 * - Level 5: Kidney Shot (408)
 * - Level 7: Slice and Dice (5171)
 * - Level 9: Gouge (1776)
 */
class TC_GAME_API RogueBaselineRotation
{
public:
    static bool ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager);
    static void ApplyBuffs(Player* bot);

private:
    enum RogueBaseline
    {
        SINISTER_STRIKE = 1752,
        EVISCERATE = 196819,
        KIDNEY_SHOT = 408,
        SLICE_AND_DICE = 5171,
        STEALTH = 1784,
        GOUGE = 1776
    };
};

/**
 * Priest Baseline Rotation Helper
 *
 * Priest levels 1-9:
 * - Level 1: Smite (585), Shadow Word: Pain (589)
 * - Level 3: Power Word: Shield (17)
 * - Level 5: Flash Heal (2061)
 * - Level 7: Renew (139)
 * - Level 9: Fade (586)
 */
class TC_GAME_API PriestBaselineRotation
{
public:
    static bool ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager);
    static void ApplyBuffs(Player* bot);

private:
    enum PriestBaseline
    {
        SMITE = 585,
        SHADOW_WORD_PAIN = 589,
        POWER_WORD_SHIELD = 17,
        FLASH_HEAL = 2061,
        RENEW = 139,
        FADE = 586
    };
};

/**
 * Mage Baseline Rotation Helper
 *
 * Mage levels 1-9:
 * - Level 1: Frostbolt (116), Fireball (133)
 * - Level 3: Fire Blast (2136)
 * - Level 5: Arcane Explosion (1449)
 * - Level 7: Frost Nova (122)
 * - Level 9: Blink (1953)
 */
class TC_GAME_API MageBaselineRotation
{
public:
    static bool ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager);
    static void ApplyBuffs(Player* bot);

private:
    enum MageBaseline
    {
        FROSTBOLT = 116,
        FIREBALL = 133,
        FIRE_BLAST = 2136,
        ARCANE_EXPLOSION = 1449,
        FROST_NOVA = 122,
        BLINK = 1953
    };
};

/**
 * Warlock Baseline Rotation Helper
 *
 * Warlock levels 1-9:
 * - Level 1: Shadow Bolt (686), Corruption (172)
 * - Level 3: Immolate (348)
 * - Level 5: Fear (5782)
 * - Level 7: Life Tap (1454)
 * - Level 9: Drain Life (234153)
 */
class TC_GAME_API WarlockBaselineRotation
{
public:
    static bool ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager);
    static void ApplyBuffs(Player* bot);

private:
    enum WarlockBaseline
    {
        SHADOW_BOLT = 686,
        CORRUPTION = 172,
        IMMOLATE = 348,
        FEAR = 5782,
        LIFE_TAP = 1454,
        DRAIN_LIFE = 234153
    };
};

/**
 * Shaman Baseline Rotation Helper
 *
 * Shaman levels 1-9:
 * - Level 1: Lightning Bolt (403), Primal Strike (73899)
 * - Level 3: Earth Shock (8042)
 * - Level 5: Healing Surge (8004)
 * - Level 7: Lightning Shield (192106)
 * - Level 9: Frost Shock (196840)
 */
class TC_GAME_API ShamanBaselineRotation
{
public:
    static bool ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager);
    static void ApplyBuffs(Player* bot);

private:
    enum ShamanBaseline
    {
        LIGHTNING_BOLT = 403,
        PRIMAL_STRIKE = 73899,
        EARTH_SHOCK = 8042,
        HEALING_SURGE = 8004,
        LIGHTNING_SHIELD = 192106,
        FROST_SHOCK = 196840
    };
};

/**
 * Druid Baseline Rotation Helper
 *
 * Druid levels 1-9:
 * - Level 1: Wrath (5176), Moonfire (8921)
 * - Level 3: Healing Touch (5185)
 * - Level 5: Rejuvenation (774)
 * - Level 7: Regrowth (8936)
 * - Level 9: Entangling Roots (339)
 */
class TC_GAME_API DruidBaselineRotation
{
public:
    static bool ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager);
    static void ApplyBuffs(Player* bot);

private:
    enum DruidBaseline
    {
        WRATH = 5176,
        MOONFIRE = 8921,
        HEALING_TOUCH = 5185,
        REJUVENATION = 774,
        REGROWTH = 8936,
        ENTANGLING_ROOTS = 339
    };
};

/**
 * Death Knight Baseline Rotation Helper
 *
 * Death Knight (starts at level 8):
 * - Level 8: Death Strike (49998), Icy Touch (45477)
 * - Level 9: Plague Strike (45462)
 */
class TC_GAME_API DeathKnightBaselineRotation
{
public:
    static bool ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager);
    static void ApplyBuffs(Player* bot);

private:
    enum DeathKnightBaseline
    {
        DEATH_STRIKE = 49998,
        ICY_TOUCH = 45477,
        PLAGUE_STRIKE = 45462
    };
};

/**
 * Monk Baseline Rotation Helper
 *
 * Monk levels 1-9:
 * - Level 1: Tiger Palm (100780), Blackout Kick (100784)
 * - Level 3: Rising Sun Kick (107428)
 * - Level 5: Roll (109132)
 * - Level 7: Vivify (116670)
 * - Level 9: Expel Harm (322101)
 */
class TC_GAME_API MonkBaselineRotation
{
public:
    static bool ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager);
    static void ApplyBuffs(Player* bot);

private:
    enum MonkBaseline
    {
        TIGER_PALM = 100780,
        BLACKOUT_KICK = 100784,
        RISING_SUN_KICK = 107428,
        ROLL = 109132,
        VIVIFY = 116670,
        EXPEL_HARM = 322101
    };
};

/**
 * Demon Hunter Baseline Rotation Helper
 *
 * Demon Hunter (starts at level 8):
 * - Level 8: Demon's Bite (162243), Chaos Strike (162794)
 * - Level 9: Fel Rush (195072)
 */
class TC_GAME_API DemonHunterBaselineRotation
{
public:
    static bool ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager);
    static void ApplyBuffs(Player* bot);

private:
    enum DemonHunterBaseline
    {
        DEMONS_BITE = 162243,
        CHAOS_STRIKE = 162794,
        FEL_RUSH = 195072
    };
};

/**
 * Evoker Baseline Rotation Helper
 *
 * Evoker levels 1-9:
 * - Level 1: Azure Strike (361469), Living Flame (361500)
 * - Level 3: Disintegrate (356995)
 * - Level 5: Fire Breath (357208)
 * - Level 7: Emerald Blossom (355913)
 * - Level 9: Verdant Embrace (360995)
 */
class TC_GAME_API EvokerBaselineRotation
{
public:
    static bool ExecuteRotation(Player* bot, ::Unit* target, BaselineRotationManager& manager);
    static void ApplyBuffs(Player* bot);

private:
    enum EvokerBaseline
    {
        AZURE_STRIKE = 361469,
        LIVING_FLAME = 361500,
        DISINTEGRATE = 356995,
        FIRE_BREATH = 357208,
        EMERALD_BLOSSOM = 355913,
        VERDANT_EMBRACE = 360995
    };
};

} // namespace Playerbot
