/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GarrisonAutoCombat_h__
#define GarrisonAutoCombat_h__

#include "Define.h"
#include <map>
#include <vector>

struct GarrAutoCombatantEntry;
struct GarrAutoSpellEntry;
struct GarrAutoSpellEffectEntry;
struct GarrFollowerEntry;

// Slots on the Shadowlands Adventures board. These are the client's own GarrAutoBoardIndex values,
// recovered from the 12.0.7.68275 binary's enum-reflection registrars
// (c:/dumps/GARRISON_ENUMS_68275.md, "GarrAutoBoardIndex"): five ally slots 0..4, eight enemy slots
// 5..12, -1 = None. GarrMissionXEncounter.BoardIndex publishes the enemy slots directly (mission 2174's
// two encounters sit at 5 and 6); the ally slots come from the player's placement in the mission UI.
enum GarrAutoBoardIndex : int8
{
    GARR_AUTO_BOARD_NONE                        = -1,
    GARR_AUTO_BOARD_ALLY_LEFT_BACK              = 0,
    GARR_AUTO_BOARD_ALLY_RIGHT_BACK             = 1,
    GARR_AUTO_BOARD_ALLY_LEFT_FRONT             = 2,
    GARR_AUTO_BOARD_ALLY_CENTER_FRONT           = 3,
    GARR_AUTO_BOARD_ALLY_RIGHT_FRONT            = 4,
    GARR_AUTO_BOARD_ENEMY_LEFT_FRONT            = 5,
    GARR_AUTO_BOARD_ENEMY_CENTER_LEFT_FRONT     = 6,
    GARR_AUTO_BOARD_ENEMY_CENTER_RIGHT_FRONT    = 7,
    GARR_AUTO_BOARD_ENEMY_RIGHT_FRONT           = 8,
    GARR_AUTO_BOARD_ENEMY_LEFT_BACK             = 9,
    GARR_AUTO_BOARD_ENEMY_CENTER_LEFT_BACK      = 10,
    GARR_AUTO_BOARD_ENEMY_CENTER_RIGHT_BACK     = 11,
    GARR_AUTO_BOARD_ENEMY_RIGHT_BACK            = 12
};

// True for the five slots a companion may occupy.
inline constexpr bool IsAllyBoardIndex(int32 boardIndex)
{
    return boardIndex >= GARR_AUTO_BOARD_ALLY_LEFT_BACK && boardIndex <= GARR_AUTO_BOARD_ALLY_RIGHT_FRONT;
}

// What the replay calls each event. Client enum GarrAutoMissionEventType, from the 12.0.7.68275
// enum-reflection registrars (c:/dumps/GARRISON_ENUMS_68275.md) and cross-checked against
// GarrisonConstantsDocumentation.lua:176-192. The wire carries these values; AutoCombatEffectType
// below is our internal simulator vocabulary and is NOT interchangeable with them.
enum GarrAutoMissionEventType : uint32
{
    GARR_AUTO_MISSION_EVENT_MELEE_DAMAGE        = 0,
    GARR_AUTO_MISSION_EVENT_RANGE_DAMAGE        = 1,
    GARR_AUTO_MISSION_EVENT_SPELL_MELEE_DAMAGE  = 2,
    GARR_AUTO_MISSION_EVENT_SPELL_RANGE_DAMAGE  = 3,
    GARR_AUTO_MISSION_EVENT_HEAL                = 4,
    GARR_AUTO_MISSION_EVENT_PERIODIC_DAMAGE     = 5,
    GARR_AUTO_MISSION_EVENT_PERIODIC_HEAL       = 6,
    GARR_AUTO_MISSION_EVENT_APPLY_AURA          = 7,
    GARR_AUTO_MISSION_EVENT_REMOVE_AURA         = 8,
    GARR_AUTO_MISSION_EVENT_DIED                = 9
};

// Which aura bucket the board socket files an ApplyAura/RemoveAura event under. Client enum
// GarrAutoPreviewTargetType (GARRISON_ENUMS_68275.md); it is a mask, and the socket switches on it in
// AdventuresSocketMixin:GetCollectionByAuraType (Blizzard_AdventuresBoard.lua:603-612).
enum GarrAutoPreviewTargetType : uint32
{
    GARR_AUTO_PREVIEW_TARGET_NONE   = 0,
    GARR_AUTO_PREVIEW_TARGET_DAMAGE = 1,
    GARR_AUTO_PREVIEW_TARGET_HEAL   = 2,
    GARR_AUTO_PREVIEW_TARGET_BUFF   = 4,
    GARR_AUTO_PREVIEW_TARGET_DEBUFF = 8
};

// GarrFollowerMissionCompleteState, per-companion outcome in SMSG_GARRISON_COMPLETE_MISSION_RESULT
// (GARRISON_ENUMS_68275.md).
enum GarrFollowerMissionCompleteState : uint32
{
    GARR_FOLLOWER_MISSION_COMPLETE_ALIVE                = 0,
    GARR_FOLLOWER_MISSION_COMPLETE_KILLED_BY_FAILURE    = 1,
    GARR_FOLLOWER_MISSION_COMPLETE_SAVED_BY_PREVENT     = 2,
    GARR_FOLLOWER_MISSION_COMPLETE_OUT_OF_DURABILITY    = 3
};

// GarrAutoSpellEffect.Effect - what one effect row of an auto-combat spell does. Labels from WoWDBDefs
// GarrAutoSpellEffect.dbd (layout ACEA7666 = 12.0.7.68275). Only the values whose meaning the client's
// own GarrAutoSpell.db2 description text corroborates are named here and simulated; every other Effect
// value is deliberately left unhandled (see TranslateSpellEffect in the .cpp), because emitting a
// replay event for a mechanic the simulation never applied would be a lie on the combat log:
//   0, 5, 6, 9, 11, 13, 15, 16, 17  undocumented or DNT/test-only rows
//   10  taunt / become untargetable      12  damage-dealt multiplier
//   14  damage-taken multiplier          18  increase max health
//   19, 20                               present in the 68275 data with no DBD entry at all
enum GarrAutoSpellEffectType : uint8
{
    // "Auto Attack - Deal attack damage to the closest enemy" (GarrAutoSpell 11), "Deal damage to an
    // enemy at range" (15). All 355 GarrAutoCombatant.AttackSpellID references in 68275 resolve to a
    // spell whose effect rows carry this value - this, not a missing spell id, is what marks a hit as
    // an auto-attack.
    GARR_AUTO_SPELL_EFFECT_DEAL_AUTO_DAMAGE = 1,
    // The DBD labels both 2 and 4 "Heal" and does not say what separates them; both are simulated as a
    // heal, which is what their spells say they do ("healing himself for $s2" on GarrAutoSpell 17 is a
    // 2, "Heal all allies for $s1% of their maximum health" on 9 is a 4).
    GARR_AUTO_SPELL_EFFECT_HEAL             = 2,
    GARR_AUTO_SPELL_EFFECT_DEAL_DAMAGE      = 3,
    GARR_AUTO_SPELL_EFFECT_HEAL_ALT         = 4,
    GARR_AUTO_SPELL_EFFECT_DOT              = 7,
    GARR_AUTO_SPELL_EFFECT_HOT              = 8
};

// GarrAutoSpellEffect.TargetType is a bit mask. The bit meanings below are the ones WoWDBDefs
// documents, and every documented combination agrees with the shipped spell descriptions:
//   1        the caster itself     (GarrAutoSpell 17 effect 1, "healing himself")
//   1|2 =  3 the closest enemy     (4 "strikes the closest enemy", also 7, 8, 11)
//   1|4 =  5 the farthest enemy    (16 "the farthest enemy", 15 "an enemy at range")
//   1|2|4 = 7 all enemies          (5 "all enemies", 17 "all enemies")
//   1|2|4|8 = 15 all enemies in melee range
//   1|16 = 17 all enemies at range (6 "all enemies at range")
//   2   =  2 the closest ally      (71 "Heals the closest ally")
//   2|4 =  6 all allies            (9, 12, 14 "all allies")
//   16  = 16 all ranged allies
// So bit 0 picks the enemy team (on its own it means the caster), bits 1/2 are near/far and having
// both means the whole team, and bits 3/4 restrict the pick to a board row. The simulation does not
// model board rows, so a row restriction resolves to "the whole team" - stated here rather than
// silently approximated.
enum GarrAutoSpellTargetMask : uint8
{
    GARR_AUTO_TARGET_ENEMY_TEAM     = 0x01,
    GARR_AUTO_TARGET_NEAR           = 0x02,
    GARR_AUTO_TARGET_FAR            = 0x04,
    GARR_AUTO_TARGET_MELEE_ROW      = 0x08,
    GARR_AUTO_TARGET_RANGED_ROW     = 0x10
};

// The simulator's own vocabulary for what an effect row did to a combatant. NOT a DB2 column, and not
// interchangeable with GarrAutoSpellEffectType above or with GarrAutoMissionEventType on the wire.
enum AutoCombatEffectType : uint8
{
    AUTO_COMBAT_EFFECT_DAMAGE           = 0,
    AUTO_COMBAT_EFFECT_HEAL             = 1,
    AUTO_COMBAT_EFFECT_HOT              = 6,
    AUTO_COMBAT_EFFECT_DOT              = 7
};

// GarrAutoCombatant.Role, values documented in the GarrAutoCombatant.dbd definition
// (WoWDBDefs, layout 0x6ADAF487 = build 12.0.7.68275).
enum AutoCombatRole : int32
{
    AUTO_COMBAT_ROLE_NONE               = 0,
    AUTO_COMBAT_ROLE_MELEE              = 1,
    AUTO_COMBAT_ROLE_RANGED_PHYSICAL    = 2,
    AUTO_COMBAT_ROLE_RANGED_MAGIC       = 3,
    AUTO_COMBAT_ROLE_HEAL_SUPPORT       = 4,
    AUTO_COMBAT_ROLE_TANK               = 5
};

struct AutoCombatPeriodicEffect
{
    uint32 SpellID = 0;
    int32 Amount = 0;
    int32 RemainingTicks = 0;
    bool IsDamage = true;
    int8 SourceBoardIndex = -1;
    // Carried so that every tick reports the same (spellID, effectIndex) pair the applying cast did;
    // the client keys its per-socket aura bookkeeping on that pair.
    uint8 EffectIndex = 0;
    int32 SourceCasterRole = 0;
};

// Nothing produces these today: the GarrAutoSpellEffect rows that would (Effect 12 damage-dealt
// multiplier, 14 damage-taken multiplier, 18 increase max health) are multiplicative and their Points
// scaling is not published, so they are not simulated. The machinery is kept because the shield and
// attack-modifier bookkeeping is already correct for the day those are decoded.
struct AutoCombatAttackModifier
{
    int32 Amount = 0;
    int32 RemainingRounds = 0;
};

struct TC_GAME_API AutoCombatCombatant
{
    uint32 AutoCombatantID = 0;
    int32 CurrentHealth = 0;
    int32 MaxHealth = 0;
    int32 BaseAttack = 0;
    int8 BoardIndex = -1;
    int32 Role = AUTO_COMBAT_ROLE_NONE;
    int32 AutoAttackSpellID = 0;
    int32 PrimarySpellID = 0;
    int32 SecondarySpellID = 0;
    int32 PassiveSpellID = 0;
    bool IsPlayerSide = false;
    uint64 FollowerDbID = 0;

    int32 ShieldAmount = 0;
    std::map<int32 /*spellID*/, int32 /*remainingCooldown*/> SpellCooldowns;
    std::vector<AutoCombatPeriodicEffect> PeriodicEffects;
    std::vector<AutoCombatAttackModifier> AttackModifiers;

    bool IsAlive() const { return CurrentHealth > 0; }
    int32 GetEffectiveAttack() const;
    void TickPeriodicEffects(struct AutoCombatRound& round);
    void TickCooldowns();
    void TickModifiers();
};

struct AutoCombatEvent
{
    int8 CasterBoardIndex = -1;
    int8 TargetBoardIndex = -1;
    uint32 SpellID = 0;
    int32 Amount = 0;
    uint8 EffectType = 0;
    // The target's health either side of this event. The replay UI draws the health bars from these
    // (FollowerMissionCompleteInfo / GarrisonAutoMissionTargetInfo carry oldHealth/newHealth/maxHealth,
    // GarrisonInfoDocumentation.lua), so they must be captured where the damage/heal is applied - they
    // cannot be reconstructed afterwards.
    int32 TargetOldHealth = 0;
    int32 TargetNewHealth = 0;
    int32 TargetMaxHealth = 0;
    // Context the replay needs to pick the right GarrAutoMissionEventType, captured here because it is
    // only knowable while the event is being produced: the caster's GarrAutoCombatant.Role separates
    // Melee from Range damage, IsAutoAttack separates plain attacks from ability casts, and
    // IsPeriodicTick separates a DoT/HoT *tick* from the cast that applied it (both use EffectType
    // AUTO_COMBAT_EFFECT_DOT/_HOT).
    int32 CasterRole = AUTO_COMBAT_ROLE_NONE;
    // GarrAutoSpellEffect.EffectIndex of the row that produced this event - the DB2 column, not the
    // row's position in our lookup vector. The client keys its per-socket aura bookkeeping on
    // (spellID, effectIndex) - AdventuresSocketMixin:AddAura/RemoveAura, Blizzard_AdventuresBoard.lua:
    // 570-590 - so it has to be the same number the client's own copy of the table carries.
    uint8 EffectIndex = 0;
    bool IsAutoAttack = false;
    bool IsPeriodicTick = false;
    bool TargetDied = false;
};

struct AutoCombatRound
{
    int32 RoundNumber = 0;
    std::vector<AutoCombatEvent> Events;
};

struct TC_GAME_API AutoCombatResult
{
    bool PlayerWon = false;
    int32 TotalRounds = 0;
    std::vector<AutoCombatRound> CombatLog;
};

class TC_GAME_API GarrisonAutoCombat
{
public:
    static constexpr int32 MAX_ROUNDS = 20;

    static AutoCombatResult SimulateCombat(
        std::vector<AutoCombatCombatant>& playerUnits,
        std::vector<AutoCombatCombatant>& enemyUnits);

    // Health/attack of a GarrAutoCombatant statline at the given level. HealthBase/AttackBase are
    // the level-1 values, the GainPerLevel columns the per-level increment.
    static int32 ScaleHealth(GarrAutoCombatantEntry const* entry, uint32 level);
    static int32 ScaleAttack(GarrAutoCombatantEntry const* entry, uint32 level);

    // followerEntry may be null (callers that only have the runtime follower record). When it
    // carries an AutoCombatantID the combatant is built entirely from GarrAutoCombatant; otherwise
    // the legacy WoD/Legion approximation below is used.
    static AutoCombatCombatant BuildFollowerCombatant(
        GarrFollowerEntry const* followerEntry,
        uint32 followerLevel, uint32 quality, uint32 itemLevelWeapon,
        uint32 itemLevelArmor, int8 boardIndex, uint64 followerDbID);

    static AutoCombatCombatant BuildEnemyCombatant(
        GarrAutoCombatantEntry const* entry, uint32 level, int8 boardIndex);

private:
    static void ProcessTurn(
        AutoCombatCombatant& combatant,
        std::vector<AutoCombatCombatant>& allies,
        std::vector<AutoCombatCombatant>& enemies,
        AutoCombatRound& round);

    // Returns true when the spell actually produced at least one replay event. A companion whose
    // ability is one of the effect kinds the simulation cannot model must not lose its turn to it, and
    // must not put it on cooldown either, so the caller falls through to the next option.
    static bool ResolveSpell(
        AutoCombatCombatant& caster, int32 spellID,
        std::vector<AutoCombatCombatant>& allies,
        std::vector<AutoCombatCombatant>& enemies,
        AutoCombatRound& round);

    // Maps a GarrAutoSpellEffect.Effect value onto the simulator's vocabulary. False means the value is
    // one this build does not know how to simulate; the row is then skipped entirely.
    static bool TranslateSpellEffect(uint8 dbEffect, uint8& simulatedEffect);

    static void ResolveEffect(
        AutoCombatCombatant& caster, GarrAutoSpellEffectEntry const* effect,
        AutoCombatCombatant& target, uint32 spellID, uint8 simulatedEffect,
        AutoCombatRound& round);

    static std::vector<AutoCombatCombatant*> SelectTargets(
        AutoCombatCombatant& caster, uint8 targetMask,
        std::vector<AutoCombatCombatant>& allies,
        std::vector<AutoCombatCombatant>& enemies);

    // These take the caster itself rather than only its board index: the replay event has to record the
    // caster's role and whether the hit was an auto-attack, and neither is recoverable later. spellID
    // is the GarrAutoSpell the hit came from and must be non-zero - the client resolves it through
    // C_Garrison.GetCombatLogSpellInfo and indexes the result unconditionally, so an event without one
    // faults its combat log.
    static void ApplyDamage(
        AutoCombatCombatant& target, int32 amount,
        AutoCombatCombatant const& caster, uint32 spellID, uint8 effectType,
        uint8 effectIndex, bool isAutoAttack, AutoCombatRound& round);

    static void ApplyHealing(
        AutoCombatCombatant& target, int32 amount,
        AutoCombatCombatant const& caster, uint32 spellID, uint8 effectIndex,
        AutoCombatRound& round);

    static bool IsTeamAlive(std::vector<AutoCombatCombatant> const& team);
    static AutoCombatCombatant* FindLowestHPAlive(std::vector<AutoCombatCombatant>& team);
    static AutoCombatCombatant* FindHighestHPAlive(std::vector<AutoCombatCombatant>& team);
};

#endif // GarrisonAutoCombat_h__
