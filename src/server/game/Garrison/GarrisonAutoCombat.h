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

enum AutoCombatEffectType : uint8
{
    AUTO_COMBAT_EFFECT_DAMAGE           = 0,
    AUTO_COMBAT_EFFECT_HEAL             = 1,
    AUTO_COMBAT_EFFECT_BUFF_ATTACK      = 2,
    AUTO_COMBAT_EFFECT_DEBUFF_ATTACK    = 3,
    AUTO_COMBAT_EFFECT_SHIELD           = 4,
    AUTO_COMBAT_EFFECT_AOE_DAMAGE       = 5,
    AUTO_COMBAT_EFFECT_HOT              = 6,
    AUTO_COMBAT_EFFECT_DOT              = 7,
    AUTO_COMBAT_EFFECT_MAX
};

enum AutoCombatTargetType : uint8
{
    AUTO_COMBAT_TARGET_SELF                 = 0,
    AUTO_COMBAT_TARGET_SINGLE_ENEMY         = 1,
    AUTO_COMBAT_TARGET_ALL_ENEMIES          = 2,
    AUTO_COMBAT_TARGET_SINGLE_ALLY          = 3,
    AUTO_COMBAT_TARGET_ALL_ALLIES           = 4,
    AUTO_COMBAT_TARGET_LOWEST_HP_ALLY       = 5,
    AUTO_COMBAT_TARGET_HIGHEST_HP_ENEMY     = 6,
    AUTO_COMBAT_TARGET_RANDOM_ENEMY         = 7,
    AUTO_COMBAT_TARGET_MAX
};

enum AutoCombatRole : int32
{
    AUTO_COMBAT_ROLE_DPS        = 0,
    AUTO_COMBAT_ROLE_TANK       = 1,
    AUTO_COMBAT_ROLE_HEALER     = 2,
    AUTO_COMBAT_ROLE_SUPPORT    = 3
};

struct AutoCombatPeriodicEffect
{
    uint32 SpellID = 0;
    int32 Amount = 0;
    int32 RemainingTicks = 0;
    bool IsDamage = true;
    int8 SourceBoardIndex = -1;
};

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
    int32 Role = AUTO_COMBAT_ROLE_DPS;
    int32 AutoAttackSpellID = 0;
    int32 PrimarySpellID = 0;
    int32 Flags = 0;
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

    static AutoCombatCombatant BuildFollowerCombatant(
        uint32 followerLevel, uint32 quality, uint32 itemLevelWeapon,
        uint32 itemLevelArmor, int8 boardIndex, uint64 followerDbID);

    static AutoCombatCombatant BuildEnemyCombatant(
        GarrAutoCombatantEntry const* entry);

private:
    static void ProcessTurn(
        AutoCombatCombatant& combatant,
        std::vector<AutoCombatCombatant>& allies,
        std::vector<AutoCombatCombatant>& enemies,
        AutoCombatRound& round);

    static void ResolveSpell(
        AutoCombatCombatant& caster, int32 spellID,
        std::vector<AutoCombatCombatant>& allies,
        std::vector<AutoCombatCombatant>& enemies,
        AutoCombatRound& round);

    static void ResolveEffect(
        AutoCombatCombatant& caster, GarrAutoSpellEffectEntry const* effect,
        AutoCombatCombatant& target, uint32 spellID,
        AutoCombatRound& round);

    static std::vector<AutoCombatCombatant*> SelectTargets(
        AutoCombatCombatant& caster, uint8 targetType,
        std::vector<AutoCombatCombatant>& allies,
        std::vector<AutoCombatCombatant>& enemies);

    static void ApplyDamage(
        AutoCombatCombatant& target, int32 amount,
        int8 casterBoardIndex, uint32 spellID, uint8 effectType,
        AutoCombatRound& round);

    static void ApplyHealing(
        AutoCombatCombatant& target, int32 amount,
        int8 casterBoardIndex, uint32 spellID,
        AutoCombatRound& round);

    static bool IsTeamAlive(std::vector<AutoCombatCombatant> const& team);
    static AutoCombatCombatant* FindLowestHPAlive(std::vector<AutoCombatCombatant>& team);
    static AutoCombatCombatant* FindHighestHPAlive(std::vector<AutoCombatCombatant>& team);
};

#endif // GarrisonAutoCombat_h__
