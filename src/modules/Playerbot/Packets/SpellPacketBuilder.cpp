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

#include "SpellPacketBuilder.h"

// TrinityCore includes
#include "CombatLogPacketsCommon.h"
#include "GameObject.h"
#include "Log.h"
#include "Map.h"
#include "MovementPackets.h"
#include "ObjectAccessor.h"
#include "Opcodes.h"
#include "PacketOperators.h"
#include "Player.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellPackets.h"
#include "Unit.h"
#include "WorldPacket.h"
#include "WorldSession.h"

using namespace WorldPackets::Spells;

// ============================================================================
// Public API - BuildCastSpellPacket
// ============================================================================

SpellPacketBuilder::BuildResult SpellPacketBuilder::BuildCastSpellPacket(
    Player* caster,
    uint32 spellId,
    Unit* target,
    BuildOptions const& options)
{
    BuildResult result;
    result.result = ValidationResult::SUCCESS;

    // Pre-flight validation (unless completely skipped)
    if (!options.skipValidation)
    {
        // Step 1: Validate player object
    if (!options.skipSpellCheck)
        {
            result.result = ValidatePlayer(caster);
            if (result.result != ValidationResult::SUCCESS)
            {
                result.failureReason = "Player validation failed";
                if (options.logFailures)
                    LogValidationFailure(caster, spellId, result.result, result.failureReason);
                return result;
            }
        }

        // Step 2: Validate spell ID
    if (!options.skipSpellCheck)
        {
            result.result = ValidateSpellId(spellId, caster);
            if (result.result != ValidationResult::SUCCESS)
            {
                result.failureReason = fmt::format("Invalid spell ID: {}", spellId);
                if (options.logFailures)
                    LogValidationFailure(caster, spellId, result.result, result.failureReason);
                return result;
            }
        }

        // Get spell info (needed for all subsequent checks)
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, caster->GetMap()->GetDifficultyID());
        if (!spellInfo)
        {
            result.result = ValidationResult::SPELL_NOT_FOUND;
            result.failureReason = fmt::format("Spell info not found for spell {}", spellId);
            if (options.logFailures)
                LogValidationFailure(caster, spellId, result.result, result.failureReason);
            return result;
        }

        // Step 3: Validate spell is learned
    if (!options.skipSpellCheck)
        {
            result.result = ValidateSpellLearned(spellInfo, caster);
            if (result.result != ValidationResult::SUCCESS)
            {
                result.failureReason = fmt::format("Spell {} not learned by {}", spellId, caster->GetName());
                if (options.logFailures)
                    LogValidationFailure(caster, spellId, result.result, result.failureReason);
                return result;
            }
        }

        // Step 4: Validate cooldown
    if (!options.skipSpellCheck)
        {
            result.result = ValidateCooldown(spellInfo, caster);
            if (result.result != ValidationResult::SUCCESS)
            {
                result.failureReason = fmt::format("Spell {} on cooldown", spellId);
                if (options.logFailures)
                    LogValidationFailure(caster, spellId, result.result, result.failureReason);
                return result;
            }
        }

        // Step 5: Validate resources (mana, rage, energy, runes, etc.)
    if (!options.skipResourceCheck)
        {
            result.result = ValidateResources(spellInfo, caster);
            if (result.result != ValidationResult::SUCCESS)
            {
                result.failureReason = fmt::format("Insufficient resources for spell {}", spellId);
                if (options.logFailures)
                    LogValidationFailure(caster, spellId, result.result, result.failureReason);
                return result;
            }
        }

        // Step 6: Validate caster state (alive, not stunned, not silenced, etc.)
    if (!options.skipStateCheck)
        {
            result.result = ValidateCasterState(spellInfo, caster, options);
            if (result.result != ValidationResult::SUCCESS)
            {
                result.failureReason = fmt::format("Caster state invalid for spell {}", spellId);
                if (options.logFailures)
                    LogValidationFailure(caster, spellId, result.result, result.failureReason);
                return result;
            }
        }

        // Step 7: Validate GCD/casting state
    if (!options.skipGcdCheck)
        {
            result.result = ValidateGlobalCooldown(caster, spellInfo, options);
            if (result.result != ValidationResult::SUCCESS)
            {
                result.failureReason = "GCD active or spell in progress";
                if (options.logFailures)
                    LogValidationFailure(caster, spellId, result.result, result.failureReason);
                return result;
            }
        }

        // Step 8: Validate target (if target provided)
    if (!options.skipTargetCheck && target)
        {
            result.result = ValidateTarget(spellInfo, caster, target, options);
            if (result.result != ValidationResult::SUCCESS)
            {
                result.failureReason = fmt::format("Target validation failed for spell {}", spellId);
                if (options.logFailures)
                    LogValidationFailure(caster, spellId, result.result, result.failureReason);
                return result;
            }
        }

        // All validation passed - proceed to packet building
        result.packet = BuildCastSpellPacketInternal(caster, spellInfo, target, nullptr);
    }
    else
    {
        // Skip validation - build packet directly (DANGEROUS!)
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, caster->GetMap()->GetDifficultyID());
        result.packet = BuildCastSpellPacketInternal(caster, spellInfo, target, nullptr);
    }

    if (!result.packet)
    {
        result.result = ValidationResult::PACKET_BUILD_FAILED;
        result.failureReason = "Failed to build packet (internal error)";
        if (options.logFailures)
            LogValidationFailure(caster, spellId, result.result, result.failureReason);
    }

    return result;
}

// ============================================================================
// Public API - BuildCastSpellPacket (GameObject target overload)
// ============================================================================

SpellPacketBuilder::BuildResult SpellPacketBuilder::BuildCastSpellPacket(
    Player* caster,
    uint32 spellId,
    GameObject* goTarget,
    BuildOptions const& options)
{
    BuildResult result;
    result.result = ValidationResult::SUCCESS;

    // Pre-flight validation (unless completely skipped)
    if (!options.skipValidation)
    {
        // Step 1: Validate player object
    if (!options.skipSpellCheck)
        {
            result.result = ValidatePlayer(caster);
            if (result.result != ValidationResult::SUCCESS)
            {
                result.failureReason = "Player validation failed";
                if (options.logFailures)
                    LogValidationFailure(caster, spellId, result.result, result.failureReason);
                return result;
            }
        }

        // Step 2: Validate spell ID
    if (!options.skipSpellCheck)
        {
            result.result = ValidateSpellId(spellId, caster);
            if (result.result != ValidationResult::SUCCESS)
            {
                result.failureReason = fmt::format("Invalid spell ID: {}", spellId);
                if (options.logFailures)
                    LogValidationFailure(caster, spellId, result.result, result.failureReason);
                return result;
            }
        }

        // Get spell info (needed for all subsequent checks)
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, caster->GetMap()->GetDifficultyID());
        if (!spellInfo)
        {
            result.result = ValidationResult::SPELL_NOT_FOUND;
            result.failureReason = fmt::format("Spell info not found for spell {}", spellId);
            if (options.logFailures)
                LogValidationFailure(caster, spellId, result.result, result.failureReason);
            return result;
        }

        // Step 3: Validate spell is learned
    if (!options.skipSpellCheck)
        {
            result.result = ValidateSpellLearned(spellInfo, caster);
            if (result.result != ValidationResult::SUCCESS)
            {
                result.failureReason = fmt::format("Spell {} not learned by {}", spellId, caster->GetName());
                if (options.logFailures)
                    LogValidationFailure(caster, spellId, result.result, result.failureReason);
                return result;
            }
        }

        // Step 4: Validate cooldown
    if (!options.skipSpellCheck)
        {
            result.result = ValidateCooldown(spellInfo, caster);
            if (result.result != ValidationResult::SUCCESS)
            {
                result.failureReason = fmt::format("Spell {} on cooldown", spellId);
                if (options.logFailures)
                    LogValidationFailure(caster, spellId, result.result, result.failureReason);
                return result;
            }
        }

        // Step 5: Validate resources (mana, rage, energy, runes, etc.)
    if (!options.skipResourceCheck)
        {
            result.result = ValidateResources(spellInfo, caster);
            if (result.result != ValidationResult::SUCCESS)
            {
                result.failureReason = fmt::format("Insufficient resources for spell {}", spellId);
                if (options.logFailures)
                    LogValidationFailure(caster, spellId, result.result, result.failureReason);
                return result;
            }
        }

        // Step 6: Validate caster state (alive, not stunned, not silenced, etc.)
    if (!options.skipStateCheck)
        {
            result.result = ValidateCasterState(spellInfo, caster, options);
            if (result.result != ValidationResult::SUCCESS)
            {
                result.failureReason = fmt::format("Caster state invalid for spell {}", spellId);
                if (options.logFailures)
                    LogValidationFailure(caster, spellId, result.result, result.failureReason);
                return result;
            }
        }

        // Step 7: Validate GCD/casting state
    if (!options.skipGcdCheck)
        {
            result.result = ValidateGlobalCooldown(caster, spellInfo, options);
            if (result.result != ValidationResult::SUCCESS)
            {
                result.failureReason = "GCD active or spell in progress";
                if (options.logFailures)
                    LogValidationFailure(caster, spellId, result.result, result.failureReason);
                return result;
            }
        }

        // Step 8: Validate GameObject target (if provided)
    if (!options.skipTargetCheck && goTarget)
        {
            // Basic GameObject validation
    if (!goTarget->IsInWorld())
            {
                result.result = ValidationResult::INVALID_TARGET;
                result.failureReason = fmt::format("GameObject {} (entry {}) not in world",
                    goTarget->GetGUID().ToString(), goTarget->GetEntry());
                if (options.logFailures)
                    LogValidationFailure(caster, spellId, result.result, result.failureReason);
                return result;
            }

            // Validate range (if range check enabled)
    if (!options.skipRangeCheck)
            {
                float distance = caster->GetDistance(goTarget);
                float maxRange = spellInfo->GetMaxRange();
                if (maxRange > 0.0f && distance > maxRange)
                {
                    result.result = ValidationResult::TARGET_OUT_OF_RANGE;
                    result.failureReason = fmt::format("GameObject {} out of range ({:.1f}yd > {:.1f}yd max)",
                        goTarget->GetEntry(), distance, maxRange);
                    if (options.logFailures)
                        LogValidationFailure(caster, spellId, result.result, result.failureReason);
                    return result;
                }
            }
        }

        // All validation passed - build packet with GameObject target
        result.packet = BuildCastSpellPacketInternalGameObject(caster, spellInfo, goTarget);
    }
    else
    {
        // Skip validation - build packet directly (DANGEROUS!)
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, caster->GetMap()->GetDifficultyID());
        result.packet = BuildCastSpellPacketInternalGameObject(caster, spellInfo, goTarget);
    }

    if (!result.packet)
    {
        result.result = ValidationResult::PACKET_BUILD_FAILED;
        result.failureReason = "Failed to build packet (internal error)";
        if (options.logFailures)
            LogValidationFailure(caster, spellId, result.result, result.failureReason);
    }

    return result;
}

// ============================================================================
// Public API - BuildCastSpellPacketAtPosition
// ============================================================================

SpellPacketBuilder::BuildResult SpellPacketBuilder::BuildCastSpellPacketAtPosition(
    Player* caster,
    uint32 spellId,
    Position const& position,
    BuildOptions const& options)
{
    BuildResult result;
    result.result = ValidationResult::SUCCESS;

    // Validation (similar to unit-targeted spell)
    if (!options.skipValidation)
    {
        result.result = ValidatePlayer(caster);
        if (result.result != ValidationResult::SUCCESS)
        {
            result.failureReason = "Player validation failed";
            if (options.logFailures)
                LogValidationFailure(caster, spellId, result.result, result.failureReason);
            return result;
        }

        result.result = ValidateSpellId(spellId, caster);
        if (result.result != ValidationResult::SUCCESS)
        {
            result.failureReason = fmt::format("Invalid spell ID: {}", spellId);
            if (options.logFailures)
                LogValidationFailure(caster, spellId, result.result, result.failureReason);
            return result;
        }

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, caster->GetMap()->GetDifficultyID());
        if (!spellInfo)
        {
            result.result = ValidationResult::SPELL_NOT_FOUND;
            result.failureReason = fmt::format("Spell info not found for spell {}", spellId);
            if (options.logFailures)
                LogValidationFailure(caster, spellId, result.result, result.failureReason);
            return result;
        }

        // Validate position target is valid for this spell
        result.result = ValidatePositionTarget(spellInfo, caster, position);
        if (result.result != ValidationResult::SUCCESS)
        {
            result.failureReason = fmt::format("Position target invalid for spell {}", spellId);
            if (options.logFailures)
                LogValidationFailure(caster, spellId, result.result, result.failureReason);
            return result;
        }

        // Build packet with position target
        result.packet = BuildCastSpellPacketInternal(caster, spellInfo, nullptr, &position);
    }
    else
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, caster->GetMap()->GetDifficultyID());
        result.packet = BuildCastSpellPacketInternal(caster, spellInfo, nullptr, &position);
    }

    if (!result.packet)
    {
        result.result = ValidationResult::PACKET_BUILD_FAILED;
        result.failureReason = "Failed to build position-targeted packet";
        if (options.logFailures)
            LogValidationFailure(caster, spellId, result.result, result.failureReason);
    }

    return result;
}

// ============================================================================
// Public API - BuildCancelCastPacket
// ============================================================================

SpellPacketBuilder::BuildResult SpellPacketBuilder::BuildCancelCastPacket(Player* caster, uint32 spellId)
{
    BuildResult result;
    result.result = ValidationResult::SUCCESS;

    if (!caster)
    {
        result.result = ValidationResult::PLAYER_NULLPTR;
        result.failureReason = "Caster is nullptr";
        return result;
    }

    result.packet = BuildCancelCastPacketInternal(caster, spellId);
    if (!result.packet)
    {
        result.result = ValidationResult::PACKET_BUILD_FAILED;
        result.failureReason = "Failed to build cancel cast packet";
    }

    return result;
}

// ============================================================================
// Public API - BuildCancelAuraPacket
// ============================================================================

SpellPacketBuilder::BuildResult SpellPacketBuilder::BuildCancelAuraPacket(Player* caster, uint32 spellId)
{
    BuildResult result;
    result.result = ValidationResult::SUCCESS;

    if (!caster)
    {
        result.result = ValidationResult::PLAYER_NULLPTR;
        result.failureReason = "Caster is nullptr";
        return result;
    }

    if (spellId == 0)
    {
        result.result = ValidationResult::INVALID_SPELL_ID;
        result.failureReason = "Spell ID cannot be 0 for CancelAura";
        return result;
    }

    result.packet = BuildCancelAuraPacketInternal(caster, spellId);
    if (!result.packet)
    {
        result.result = ValidationResult::PACKET_BUILD_FAILED;
        result.failureReason = "Failed to build cancel aura packet";
    }

    return result;
}

// ============================================================================
// Public API - BuildCancelChannelPacket
// ============================================================================

SpellPacketBuilder::BuildResult SpellPacketBuilder::BuildCancelChannelPacket(Player* caster, uint32 spellId, int32 reason)
{
    BuildResult result;
    result.result = ValidationResult::SUCCESS;

    if (!caster)
    {
        result.result = ValidationResult::PLAYER_NULLPTR;
        result.failureReason = "Caster is nullptr";
        return result;
    }

    result.packet = BuildCancelChannelPacketInternal(caster, spellId, reason);
    if (!result.packet)
    {
        result.result = ValidationResult::PACKET_BUILD_FAILED;
        result.failureReason = "Failed to build cancel channel packet";
    }

    return result;
}

// ============================================================================
// Public API - BuildCancelAutoRepeatPacket
// ============================================================================

SpellPacketBuilder::BuildResult SpellPacketBuilder::BuildCancelAutoRepeatPacket(Player* caster)
{
    BuildResult result;
    result.result = ValidationResult::SUCCESS;

    if (!caster)
    {
        result.result = ValidationResult::PLAYER_NULLPTR;
        result.failureReason = "Caster is nullptr";
        return result;
    }

    result.packet = BuildCancelAutoRepeatPacketInternal(caster);
    if (!result.packet)
    {
        result.result = ValidationResult::PACKET_BUILD_FAILED;
        result.failureReason = "Failed to build cancel auto-repeat packet";
    }

    return result;
}

// ============================================================================
// Public API - ValidateSpellCast (Pre-flight check)
// ============================================================================

SpellPacketBuilder::BuildResult SpellPacketBuilder::ValidateSpellCast(
    Player* caster,
    uint32 spellId,
    Unit* target,
    BuildOptions const& options)
{
    // Reuse BuildCastSpellPacket validation logic but don't build packet
    BuildOptions validateOnly = options;
    validateOnly.skipValidation = false; // Force validation

    BuildResult result;
    result.result = ValidationResult::SUCCESS;

    // Run all validation steps
    result.result = ValidatePlayer(caster);
    if (result.result != ValidationResult::SUCCESS)
    {
        result.failureReason = "Player validation failed";
        return result;
    }

    result.result = ValidateSpellId(spellId, caster);
    if (result.result != ValidationResult::SUCCESS)
    {
        result.failureReason = fmt::format("Invalid spell ID: {}", spellId);
        return result;
    }

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, caster->GetMap()->GetDifficultyID());
    if (!spellInfo)
    {
        result.result = ValidationResult::SPELL_NOT_FOUND;
        result.failureReason = fmt::format("Spell info not found for spell {}", spellId);
        return result;
    }

    result.result = ValidateSpellLearned(spellInfo, caster);
    if (result.result != ValidationResult::SUCCESS)
    {
        result.failureReason = fmt::format("Spell {} not learned", spellId);
        return result;
    }

    result.result = ValidateCooldown(spellInfo, caster);
    if (result.result != ValidationResult::SUCCESS)
    {
        result.failureReason = fmt::format("Spell {} on cooldown", spellId);
        return result;
    }

    result.result = ValidateResources(spellInfo, caster);
    if (result.result != ValidationResult::SUCCESS)
    {
        result.failureReason = "Insufficient resources";
        return result;
    }

    result.result = ValidateCasterState(spellInfo, caster, validateOnly);
    if (result.result != ValidationResult::SUCCESS)
    {
        result.failureReason = "Caster state invalid";
        return result;
    }

    result.result = ValidateGlobalCooldown(caster, spellInfo, validateOnly);
    if (result.result != ValidationResult::SUCCESS)
    {
        result.failureReason = "GCD active or spell in progress";
        return result;
    }

    if (target)
    {
        result.result = ValidateTarget(spellInfo, caster, target, validateOnly);
        if (result.result != ValidationResult::SUCCESS)
        {
            result.failureReason = "Target validation failed";
            return result;
        }
    }

    // All validation passed
    result.result = ValidationResult::SUCCESS;
    result.failureReason = "Validation successful";
    return result;
}

// ============================================================================
// Validation Methods (Enterprise-grade, comprehensive)
// ============================================================================

SpellPacketBuilder::ValidationResult SpellPacketBuilder::ValidatePlayer(Player const* caster)
{
    if (!caster)
        return ValidationResult::PLAYER_NULLPTR;

    if (!caster->GetSession())
        return ValidationResult::SESSION_NULLPTR;

    if (!caster->GetMap())
        return ValidationResult::MAP_NULLPTR;

    if (!caster->IsInWorld())
        return ValidationResult::CASTER_DEAD; // Not in world = effectively dead/non-functional

    return ValidationResult::SUCCESS;
}

SpellPacketBuilder::ValidationResult SpellPacketBuilder::ValidateSpellId(uint32 spellId, Player const* caster)
{
    if (spellId == 0)
        return ValidationResult::INVALID_SPELL_ID;

    // Check if spell exists in spell manager
    if (!sSpellMgr->GetSpellInfo(spellId, caster->GetMap()->GetDifficultyID()))
        return ValidationResult::SPELL_NOT_FOUND;

    return ValidationResult::SUCCESS;
}

SpellPacketBuilder::ValidationResult SpellPacketBuilder::ValidateSpellInfo(SpellInfo const* spellInfo, Player const* caster)
{
    if (!spellInfo)
        return ValidationResult::SPELL_NOT_FOUND;

    // Additional spell info validation could go here
    // (e.g., check if spell is disabled, deprecated, etc.)

    return ValidationResult::SUCCESS;
}

SpellPacketBuilder::ValidationResult SpellPacketBuilder::ValidateSpellLearned(SpellInfo const* spellInfo, Player const* caster)
{
    if (!spellInfo)
        return ValidationResult::SPELL_NOT_FOUND;

    // Check if player has learned the spell
    if (!caster->HasSpell(spellInfo->Id))
        return ValidationResult::SPELL_NOT_LEARNED;

    return ValidationResult::SUCCESS;
}

SpellPacketBuilder::ValidationResult SpellPacketBuilder::ValidateCooldown(SpellInfo const* spellInfo, Player const* caster)
{
    if (!spellInfo)
        return ValidationResult::SPELL_NOT_FOUND;

    // Check if spell is on cooldown
    if (caster->GetSpellHistory()->HasCooldown(spellInfo->Id))
        return ValidationResult::SPELL_ON_COOLDOWN;

    return ValidationResult::SUCCESS;
}

SpellPacketBuilder::ValidationResult SpellPacketBuilder::ValidateResources(SpellInfo const* spellInfo, Player const* caster)
{
    if (!spellInfo)
        return ValidationResult::SPELL_NOT_FOUND;

    // Check power cost (mana, rage, energy, runes, etc.)
    for (SpellPowerCost const& cost : spellInfo->CalcPowerCost(caster, spellInfo->GetSchoolMask()))
    {
        if (caster->GetPower(cost.Power) < cost.Amount)
        {
            switch (cost.Power)
            {
                case POWER_MANA:
                    return ValidationResult::INSUFFICIENT_MANA;
                case POWER_RAGE:
                    return ValidationResult::INSUFFICIENT_RAGE;
                case POWER_ENERGY:
                    return ValidationResult::INSUFFICIENT_ENERGY;
                case POWER_RUNES:
                    return ValidationResult::INSUFFICIENT_RUNES;
                default:
                    return ValidationResult::INSUFFICIENT_POWER;
            }
        }
    }

    return ValidationResult::SUCCESS;
}

SpellPacketBuilder::ValidationResult SpellPacketBuilder::ValidateCasterState(
    SpellInfo const* spellInfo,
    Player const* caster,
    BuildOptions const& options)
{
    if (!spellInfo)
        return ValidationResult::SPELL_NOT_FOUND;

    // Check if caster is dead
    if (!options.allowDeadCaster && !caster->IsAlive())
        return ValidationResult::CASTER_DEAD;

    // Check if caster is stunned
    if (caster->HasUnitState(UNIT_STATE_STUNNED))
        return ValidationResult::CASTER_STUNNED;

    // Check if caster is silenced (for spells that can't be cast while silenced)
    // Note: Silence is handled via HasUnitState(UNIT_STATE_CASTING) or spell interrupt system

    // Check if caster is pacified (can't cast offensive spells)
    // Note: Pacify is handled via spell interrupt and aura system

    // Check if caster is moving (for spells that can't be cast while moving)
    if (!options.allowWhileMoving && !spellInfo->IsPassive())
    {
        if (caster->isMoving() || caster->IsFalling())
            return ValidationResult::CASTER_MOVING;
    }

    return ValidationResult::SUCCESS;
}

SpellPacketBuilder::ValidationResult SpellPacketBuilder::ValidateGlobalCooldown(
    Player const* caster,
    SpellInfo const* spellInfo,
    BuildOptions const& options)
{
    // Check if GCD is active
    if (!options.allowWhileCasting && caster->GetSpellHistory()->HasGlobalCooldown(spellInfo))
        return ValidationResult::GCD_ACTIVE;

    // Check if player is already casting a spell
    if (!options.allowWhileCasting && caster->IsNonMeleeSpellCast(false))
        return ValidationResult::SPELL_IN_PROGRESS;

    return ValidationResult::SUCCESS;
}

SpellPacketBuilder::ValidationResult SpellPacketBuilder::ValidateTarget(
    SpellInfo const* spellInfo,
    Player const* caster,
    Unit const* target,
    BuildOptions const& options)
{
    if (!spellInfo)
        return ValidationResult::SPELL_NOT_FOUND;

    if (!target)
    {
        // Check if spell requires a target
    if (spellInfo->NeedsExplicitUnitTarget())
            return ValidationResult::INVALID_TARGET;
        return ValidationResult::SUCCESS;
    }

    // Check if target is dead (for spells that can't target dead units)
    if (!spellInfo->IsAllowingDeadTarget() && !target->IsAlive())
        return ValidationResult::TARGET_DEAD;

    // Check if target is friendly/hostile (based on spell requirements)
    if (spellInfo->IsPositive())
    {
        if (caster->IsHostileTo(target))
            return ValidationResult::TARGET_HOSTILE;
    }
    else
    {
        if (caster->IsFriendlyTo(target))
            return ValidationResult::TARGET_FRIENDLY;
    }

    // Check range
    if (!options.skipRangeCheck)
    {
        ValidationResult rangeCheck = ValidateTargetRange(spellInfo, caster, target);
        if (rangeCheck != ValidationResult::SUCCESS)
            return rangeCheck;
    }

    // Check LOS
    if (!options.skipRangeCheck)
    {
        ValidationResult losCheck = ValidateTargetLOS(caster, target);
        if (losCheck != ValidationResult::SUCCESS)
            return losCheck;
    }

    return ValidationResult::SUCCESS;
}

SpellPacketBuilder::ValidationResult SpellPacketBuilder::ValidateTargetRange(
    SpellInfo const* spellInfo,
    Player const* caster,
    Unit const* target)
{
    if (!spellInfo || !target)
        return ValidationResult::INVALID_TARGET;

    float maxRange = spellInfo->GetMaxRange(spellInfo->IsPositive(), const_cast<Player*>(caster));
    float distance = caster->GetDistance(target);

    if (distance > maxRange)
        return ValidationResult::TARGET_OUT_OF_RANGE;

    return ValidationResult::SUCCESS;
}

SpellPacketBuilder::ValidationResult SpellPacketBuilder::ValidateTargetLOS(
    Player const* caster,
    Unit const* target)
{
    if (!target)
        return ValidationResult::INVALID_TARGET;

    // Check line of sight
    if (!caster->IsWithinLOSInMap(target))
        return ValidationResult::TARGET_NOT_IN_LOS;

    return ValidationResult::SUCCESS;
}

SpellPacketBuilder::ValidationResult SpellPacketBuilder::ValidatePositionTarget(
    SpellInfo const* spellInfo,
    Player const* caster,
    Position const& position)
{
    if (!spellInfo)
        return ValidationResult::SPELL_NOT_FOUND;

    // Validate position is within map bounds
    if (!caster->GetMap()->IsGridLoaded(position.GetPositionX(), position.GetPositionY()))
        return ValidationResult::POSITION_INVALID;

    // Check range to position
    float maxRange = spellInfo->GetMaxRange(spellInfo->IsPositive(), const_cast<Player*>(caster));
    float distance = caster->GetExactDist(&position);
    if (distance > maxRange)
        return ValidationResult::TARGET_OUT_OF_RANGE;

    return ValidationResult::SUCCESS;
}

// ============================================================================
// Internal Packet Builders (Use TrinityCore official structures)
// ============================================================================

// ============================================================================
// Helper: Write MovementInfo to packet (mirrors operator<< in MovementPackets.cpp:72-160)
// ============================================================================
// This is duplicated here because the operator<< for MovementInfo is defined in
// MovementPackets.cpp but NOT declared in MovementPackets.h (only TransportInfo is).
// Rather than modifying core headers, we duplicate the serialization logic here.
// ============================================================================
static void WriteMovementInfo(ByteBuffer& data, MovementInfo const& movementInfo)
{
    bool hasTransportData = !movementInfo.transport.guid.IsEmpty();
    bool hasFallDirection = movementInfo.HasMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR);
    bool hasFallData = hasFallDirection || movementInfo.jump.fallTime != 0;
    bool hasSpline = false; // Not used for client->server packets
    bool hasInertia = movementInfo.inertia.has_value();
    bool hasAdvFlying = movementInfo.advFlying.has_value();
    bool hasDriveStatus = movementInfo.driveStatus.has_value();
    bool hasStandingOnGameObjectGUID = movementInfo.standingOnGameObjectGUID.has_value();

    data << movementInfo.guid;
    data << uint32(movementInfo.flags);
    data << uint32(movementInfo.flags2);
    data << uint32(movementInfo.flags3);
    data << uint32(movementInfo.time);
    data << movementInfo.pos.PositionXYZOStream();
    data << float(movementInfo.pitch);
    data << float(movementInfo.stepUpStartElevation);

    uint32 removeMovementForcesCount = 0;
    data << removeMovementForcesCount;

    uint32 moveIndex = 0;
    data << moveIndex;

    data.WriteBit(hasStandingOnGameObjectGUID);
    data.WriteBit(hasTransportData);
    data.WriteBit(hasFallData);
    data.WriteBit(hasSpline);

    data.WriteBit(false); // HeightChangeFailed
    data.WriteBit(false); // RemoteTimeValid
    data.WriteBit(hasInertia);
    data.WriteBit(hasAdvFlying);
    data.WriteBit(hasDriveStatus);

    data.FlushBits();

    if (hasTransportData)
        data << movementInfo.transport;

    if (hasStandingOnGameObjectGUID)
        data << *movementInfo.standingOnGameObjectGUID;

    if (hasInertia)
    {
        data << uint32(movementInfo.inertia->id);
        data << movementInfo.inertia->force.PositionXYZStream();
        data << uint32(movementInfo.inertia->lifetime);
    }

    if (hasAdvFlying)
    {
        data << float(movementInfo.advFlying->forwardVelocity);
        data << float(movementInfo.advFlying->upVelocity);
    }

    if (hasFallData)
    {
        data << uint32(movementInfo.jump.fallTime);
        data << float(movementInfo.jump.zspeed);

        data.WriteBit(hasFallDirection);
        data.FlushBits();
        if (hasFallDirection)
        {
            data << float(movementInfo.jump.sinAngle);
            data << float(movementInfo.jump.cosAngle);
            data << float(movementInfo.jump.xyspeed);
        }
    }

    if (hasDriveStatus)
    {
        data << float(movementInfo.driveStatus->speed);
        data << float(movementInfo.driveStatus->movementAngle);
        data.WriteBit(movementInfo.driveStatus->accelerating);
        data.WriteBit(movementInfo.driveStatus->drifting);
        data.FlushBits();
    }
}

// ============================================================================
// Helper: Write SpellTargetData to packet (mirrors operator>> in SpellPackets.cpp:167-194)
// ============================================================================
static void WriteSpellTargetData(ByteBuffer& buffer, WorldPackets::Spells::SpellTargetData const& targetData)
{
    size_t startSize = buffer.size();

    // Fixed fields (must match exact read order in SpellPackets.cpp:169-172)
    // WoW 12.0: Unknown1127_1 renamed to HousingGUID
    buffer << uint32(targetData.Flags);
    buffer << targetData.Unit;
    buffer << targetData.Item;
    buffer << targetData.HousingGUID;  // WoW 12.0: Renamed from Unknown1127_1

    TC_LOG_DEBUG("playerbot.spells.packets", "WriteSpellTargetData: After Flags/Unit/Item/HousingGUID, Unit={}, size={}",
        targetData.Unit.ToString(), buffer.size());

    // Bit fields section (must match exact read order)
    // WoW 12.0: Unknown1127_2 renamed to HousingIsResident
    buffer << WorldPackets::Bits<1>(targetData.HousingIsResident);  // WoW 12.0: Renamed from Unknown1127_2
    buffer << WorldPackets::OptionalInit(targetData.SrcLocation);
    buffer << WorldPackets::OptionalInit(targetData.DstLocation);
    buffer << WorldPackets::OptionalInit(targetData.Orientation);
    buffer << WorldPackets::OptionalInit(targetData.MapID);
    buffer << WorldPackets::SizedString::BitsSize<7>(targetData.Name);
    buffer.FlushBits();

    TC_LOG_DEBUG("playerbot.spells.packets", "WriteSpellTargetData: After bits, SrcLoc={}, DstLoc={}, Name.len={}, size={}",
        targetData.SrcLocation.has_value(), targetData.DstLocation.has_value(), targetData.Name.length(), buffer.size());

    // Write optional data
    if (targetData.SrcLocation)
    {
        buffer << targetData.SrcLocation->Transport;
        buffer << targetData.SrcLocation->Location;
    }

    if (targetData.DstLocation)
    {
        buffer << targetData.DstLocation->Transport;
        buffer << targetData.DstLocation->Location;
    }

    if (targetData.Orientation)
        buffer << float(*targetData.Orientation);

    if (targetData.MapID)
        buffer << int32(*targetData.MapID);

    buffer << WorldPackets::SizedString::Data(targetData.Name);
}

// ============================================================================
// Helper: Write SpellCastRequest to packet (mirrors operator>> in SpellPackets.cpp:226-272)
// WoW 12.0: Updated packet structure with renamed fields
// ============================================================================
static void WriteSpellCastRequest(ByteBuffer& buffer, WorldPackets::Spells::SpellCastRequest const& request)
{
    size_t startSize = buffer.size();

    // Fixed fields (must match read order exactly - see SpellPackets.cpp:228-240)
    buffer << request.CastID;
    TC_LOG_DEBUG("playerbot.spells.packets", "WriteSpellCastRequest: After CastID, size={}", buffer.size());

    buffer << uint8(request.SendCastFlags);
    buffer << int32(request.Misc[0]);
    buffer << int32(request.Misc[1]);
    buffer << int32(request.Misc[2]);  // WoW 12.0: Third element added
    buffer << int32(request.SpellID);
    TC_LOG_DEBUG("playerbot.spells.packets", "WriteSpellCastRequest: After SpellID={}, size={}", request.SpellID, buffer.size());

    buffer << request.Visual;

    // MissileTrajectory (pitch and speed)
    buffer << float(request.MissileTrajectory.Pitch);
    buffer << float(request.MissileTrajectory.Speed);

    buffer << request.CraftingNPC;

    // Array sizes - WoW 12.0: Field names changed
    buffer << WorldPackets::Size<uint32>(request.ExtraCurrencyCosts);  // Was OptionalCurrencies
    buffer << WorldPackets::Size<uint32>(request.CraftingReagents);    // Was OptionalReagents
    buffer << WorldPackets::Size<uint32>(request.RemovedReagents);     // Was RemovedModifications

    buffer << uint8(request.CraftingCastFlags);  // Was CraftingFlags

    // Write ExtraCurrencyCosts (was OptionalCurrencies)
    for (auto const& currency : request.ExtraCurrencyCosts)
    {
        buffer << int32(currency.CurrencyID);
        buffer << int32(currency.Count);
    }

    // SpellTargetData
    TC_LOG_DEBUG("playerbot.spells.packets", "WriteSpellCastRequest: Before Target, size={}", buffer.size());
    WriteSpellTargetData(buffer, request.Target);
    TC_LOG_DEBUG("playerbot.spells.packets", "WriteSpellCastRequest: After Target (Flags={}), size={}", uint32(request.Target.Flags), buffer.size());

    // Bit fields section (must reset bit position before writing)
    buffer.ResetBitPos();
    buffer << WorldPackets::OptionalInit(request.MoveUpdate);
    buffer << WorldPackets::BitsSize<2>(request.Weight);
    buffer << WorldPackets::OptionalInit(request.CraftingOrderID);
    buffer.FlushBits();
    TC_LOG_DEBUG("playerbot.spells.packets", "WriteSpellCastRequest: After bits, MoveUpdate={}, WeightSize={}, size={}",
        request.MoveUpdate.has_value(), request.Weight.size(), buffer.size());

    // Write CraftingReagents (was OptionalReagents) - WoW 12.0: Field structure changed
    for (auto const& reagent : request.CraftingReagents)
    {
        buffer << int32(reagent.Slot);
        buffer << int32(reagent.Quantity);
        buffer << reagent.Reagent;  // CraftingReagentBase
        buffer << WorldPackets::OptionalInit(reagent.Source);
        if (reagent.Source)
            buffer << uint8(*reagent.Source);
    }

    // CraftingOrderID
    if (request.CraftingOrderID)
        buffer << uint64(*request.CraftingOrderID);

    // Write RemovedReagents (was RemovedModifications) - WoW 12.0: Same structure as CraftingReagents
    for (auto const& reagent : request.RemovedReagents)
    {
        buffer << int32(reagent.Slot);
        buffer << int32(reagent.Quantity);
        buffer << reagent.Reagent;  // CraftingReagentBase
        buffer << WorldPackets::OptionalInit(reagent.Source);
        if (reagent.Source)
            buffer << uint8(*reagent.Source);
    }

    // MoveUpdate - Used when casting while moving (e.g., kiting classes like Frost Mage, Hunter)
    // TrinityCore will validate if the spell can be cast while moving and return appropriate errors
    if (request.MoveUpdate)
        WriteMovementInfo(buffer, *request.MoveUpdate);

    // Weight array
    for (auto const& weight : request.Weight)
    {
        buffer.ResetBitPos();
        buffer << WorldPackets::Bits<2>(weight.Type);
        buffer << int32(weight.ID);
        buffer << uint32(weight.Quantity);
    }

    TC_LOG_DEBUG("playerbot.spells.packets", "WriteSpellCastRequest: COMPLETE, totalSize={}", buffer.size() - startSize);
}

std::unique_ptr<WorldPacket> SpellPacketBuilder::BuildCastSpellPacketInternal(
    Player* caster,
    SpellInfo const* spellInfo,
    Unit* target,
    Position const* position)
{
    if (!caster || !spellInfo)
        return nullptr;

    // =========================================================================
    // ENTERPRISE-GRADE CMSG_CAST_SPELL PACKET CONSTRUCTION
    // =========================================================================
    // This implementation creates a complete, valid CMSG_CAST_SPELL packet that
    // exactly mirrors what a real WoW client would send. The packet structure
    // is defined in WorldPackets::Spells::SpellCastRequest and must be serialized
    // in the exact order that operator>> reads it (see SpellPackets.cpp:224-270).
    //
    // Flow: Bot AI → SpellPacketBuilder → QueuePacket → HandleCastSpellOpcode
    //       → CanRequestSpellCast → RequestSpellCast → Thread-safe execution
    // =========================================================================

    // Create CMSG_CAST_SPELL packet
    auto packet = std::make_unique<WorldPacket>(CMSG_CAST_SPELL);

    // Build SpellCastRequest structure with all required fields
    WorldPackets::Spells::SpellCastRequest castRequest;

    // Generate unique CastID (required for spell cast identification)
    castRequest.CastID = ObjectGuid::Create<HighGuid::Cast>(
        SPELL_CAST_SOURCE_NORMAL,
        caster->GetMapId(),
        spellInfo->Id,
        caster->GetMap()->GenerateLowGuid<HighGuid::Cast>()
    );

    // Core spell identification
    castRequest.SpellID = spellInfo->Id;

    // Visual information (optional - server will use defaults from SpellInfo)
    castRequest.Visual.SpellXSpellVisualID = 0;
    castRequest.Visual.ScriptVisualID = 0;

    // Cast flags (0 = normal cast)
    castRequest.SendCastFlags = 0;

    // Misc data (typically 0 for normal spell casts)
    castRequest.Misc[0] = 0;
    castRequest.Misc[1] = 0;

    // Missile trajectory (for ranged/projectile spells - use defaults)
    castRequest.MissileTrajectory.Pitch = 0.0f;
    castRequest.MissileTrajectory.Speed = 0.0f;

    // Crafting-related fields (not used for combat spells)
    castRequest.CraftingNPC = ObjectGuid::Empty;
    castRequest.CraftingCastFlags = 0;
    // OptionalCurrencies, OptionalReagents, RemovedModifications - leave empty
    // CraftingOrderID - leave unset

    // =========================================================================
    // TARGET CONFIGURATION
    // =========================================================================
    // SpellTargetData structure must match what HandleCastSpellOpcode expects
    // See SpellPackets.cpp:167-194 for the full structure

    if (target)
    {
        // Unit target (most common case for combat spells)
        castRequest.Target.Flags = TARGET_FLAG_UNIT;
        castRequest.Target.Unit = target->GetGUID();
        castRequest.Target.Item = ObjectGuid::Empty;
        // SrcLocation, DstLocation, Orientation, MapID - leave unset
        castRequest.Target.Name.clear();
    }
    else if (position)
    {
        // Ground-targeted spell (e.g., Blizzard, Rain of Fire)
        castRequest.Target.Flags = TARGET_FLAG_DEST_LOCATION;
        castRequest.Target.Unit = ObjectGuid::Empty;
        castRequest.Target.Item = ObjectGuid::Empty;

        // Set destination location
        castRequest.Target.DstLocation = WorldPackets::Spells::TargetLocation();
        castRequest.Target.DstLocation->Transport = ObjectGuid::Empty;
        castRequest.Target.DstLocation->Location.Pos.Relocate(
            position->GetPositionX(),
            position->GetPositionY(),
            position->GetPositionZ()
        );

        castRequest.Target.Name.clear();
    }
    else
    {
        // Self-cast or no explicit target
        // Many spells work with TARGET_FLAG_UNIT + self GUID
        castRequest.Target.Flags = TARGET_FLAG_UNIT;
        castRequest.Target.Unit = caster->GetGUID();
        castRequest.Target.Item = ObjectGuid::Empty;
        castRequest.Target.Name.clear();
    }

    // Weight array (spell weighting for queue system - leave empty for normal casts)
    // MoveUpdate - not needed for bots (they don't send movement updates during casts)

    // =========================================================================
    // SERIALIZE TO PACKET
    // =========================================================================
    // Write the complete SpellCastRequest structure to the packet buffer
    // This must match the exact order that operator>> reads (SpellPackets.cpp:224-270)

    WriteSpellCastRequest(*packet, castRequest);

    TC_LOG_DEBUG("playerbot.spells.packets",
        "Built CMSG_CAST_SPELL packet: caster={}, spell={}, target={}, packetSize={}",
        caster->GetName(),
        spellInfo->Id,
        target ? target->GetName() : (position ? "position" : "self"),
        packet->size());

    return packet;
}

std::unique_ptr<WorldPacket> SpellPacketBuilder::BuildCastSpellPacketInternalGameObject(
    Player* caster,
    SpellInfo const* spellInfo,
    GameObject* goTarget)
{
    if (!caster || !spellInfo)
        return nullptr;

    // =========================================================================
    // ENTERPRISE-GRADE CMSG_CAST_SPELL PACKET FOR GAMEOBJECT TARGETS
    // =========================================================================
    // Used for quest interactions, harvesting, opening chests, etc.
    // GameObjects use TARGET_FLAG_GAMEOBJECT and store GUID in Unit field
    // =========================================================================

    auto packet = std::make_unique<WorldPacket>(CMSG_CAST_SPELL);

    // Build SpellCastRequest structure
    WorldPackets::Spells::SpellCastRequest castRequest;

    // Generate unique CastID
    castRequest.CastID = ObjectGuid::Create<HighGuid::Cast>(
        SPELL_CAST_SOURCE_NORMAL,
        caster->GetMapId(),
        spellInfo->Id,
        caster->GetMap()->GenerateLowGuid<HighGuid::Cast>()
    );

    // Core spell identification
    castRequest.SpellID = spellInfo->Id;

    // Visual information
    castRequest.Visual.SpellXSpellVisualID = 0;
    castRequest.Visual.ScriptVisualID = 0;

    // Cast flags
    castRequest.SendCastFlags = 0;

    // Misc data
    castRequest.Misc[0] = 0;
    castRequest.Misc[1] = 0;

    // Missile trajectory
    castRequest.MissileTrajectory.Pitch = 0.0f;
    castRequest.MissileTrajectory.Speed = 0.0f;

    // Crafting fields
    castRequest.CraftingNPC = ObjectGuid::Empty;
    castRequest.CraftingCastFlags = 0;

    // Set GameObject target
    if (goTarget)
    {
        // GameObjects use TARGET_FLAG_GAMEOBJECT
        // The GUID is stored in the Unit field (TrinityCore convention)
        castRequest.Target.Flags = TARGET_FLAG_GAMEOBJECT;
        castRequest.Target.Unit = goTarget->GetGUID();
        castRequest.Target.Item = ObjectGuid::Empty;
        castRequest.Target.Name.clear();
    }
    else
    {
        // Fallback to self-cast
        castRequest.Target.Flags = TARGET_FLAG_UNIT;
        castRequest.Target.Unit = caster->GetGUID();
        castRequest.Target.Item = ObjectGuid::Empty;
        castRequest.Target.Name.clear();
    }

    // Serialize complete packet
    WriteSpellCastRequest(*packet, castRequest);

    TC_LOG_DEBUG("playerbot.spells.packets",
        "Built CMSG_CAST_SPELL packet (GameObject): caster={}, spell={}, goTarget={}, packetSize={}",
        caster->GetName(),
        spellInfo->Id,
        goTarget ? goTarget->GetGUID().ToString() : "none",
        packet->size());

    return packet;
}

std::unique_ptr<WorldPacket> SpellPacketBuilder::BuildCancelCastPacketInternal(Player* caster, uint32 spellId)
{
    if (!caster)
        return nullptr;

    auto packet = std::make_unique<WorldPacket>(CMSG_CANCEL_CAST);
    *packet << uint32(0); // Cast count
    *packet << spellId;

    TC_LOG_TRACE("playerbot.spells.packets",
        "Built CMSG_CANCEL_CAST packet: caster={}, spell={}",
        caster->GetName(), spellId);

    return packet;
}

std::unique_ptr<WorldPacket> SpellPacketBuilder::BuildCancelAuraPacketInternal(Player* caster, uint32 spellId)
{
    if (!caster)
        return nullptr;

    auto packet = std::make_unique<WorldPacket>(CMSG_CANCEL_AURA);
    *packet << spellId;
    *packet << caster->GetGUID(); // Caster GUID
    TC_LOG_TRACE("playerbot.spells.packets",
        "Built CMSG_CANCEL_AURA packet: caster={}, spell={}",
        caster->GetName(), spellId);

    return packet;
}

std::unique_ptr<WorldPacket> SpellPacketBuilder::BuildCancelChannelPacketInternal(Player* caster, uint32 spellId, int32 reason)
{
    if (!caster)
        return nullptr;

    auto packet = std::make_unique<WorldPacket>(CMSG_CANCEL_CHANNELLING);
    *packet << spellId;
    *packet << reason;

    TC_LOG_TRACE("playerbot.spells.packets",
        "Built CMSG_CANCEL_CHANNELLING packet: caster={}, spell={}, reason={}",
        caster->GetName(), spellId, reason);

    return packet;
}

std::unique_ptr<WorldPacket> SpellPacketBuilder::BuildCancelAutoRepeatPacketInternal(Player* caster)
{
    if (!caster)
        return nullptr;

    auto packet = std::make_unique<WorldPacket>(CMSG_CANCEL_AUTO_REPEAT_SPELL);
    // No data for this packet

    TC_LOG_TRACE("playerbot.spells.packets",
        "Built CMSG_CANCEL_AUTO_REPEAT_SPELL packet: caster={}",
        caster->GetName());

    return packet;
}

// ============================================================================
// Helper Utilities
// ============================================================================

char const* SpellPacketBuilder::BuildResult::GetResultName() const
{
    return GetValidationResultString(result).c_str();
}

std::string SpellPacketBuilder::GetValidationResultString(ValidationResult result)
{
    switch (result)
    {
        case ValidationResult::SUCCESS: return "SUCCESS";
        case ValidationResult::INVALID_SPELL_ID: return "INVALID_SPELL_ID";
        case ValidationResult::SPELL_NOT_FOUND: return "SPELL_NOT_FOUND";
        case ValidationResult::SPELL_NOT_LEARNED: return "SPELL_NOT_LEARNED";
        case ValidationResult::SPELL_ON_COOLDOWN: return "SPELL_ON_COOLDOWN";
        case ValidationResult::SPELL_NOT_READY: return "SPELL_NOT_READY";
        case ValidationResult::INSUFFICIENT_MANA: return "INSUFFICIENT_MANA";
        case ValidationResult::INSUFFICIENT_RAGE: return "INSUFFICIENT_RAGE";
        case ValidationResult::INSUFFICIENT_ENERGY: return "INSUFFICIENT_ENERGY";
        case ValidationResult::INSUFFICIENT_RUNES: return "INSUFFICIENT_RUNES";
        case ValidationResult::INSUFFICIENT_POWER: return "INSUFFICIENT_POWER";
        case ValidationResult::INVALID_TARGET: return "INVALID_TARGET";
        case ValidationResult::TARGET_OUT_OF_RANGE: return "TARGET_OUT_OF_RANGE";
        case ValidationResult::TARGET_NOT_IN_LOS: return "TARGET_NOT_IN_LOS";
        case ValidationResult::TARGET_DEAD: return "TARGET_DEAD";
        case ValidationResult::TARGET_FRIENDLY: return "TARGET_FRIENDLY";
        case ValidationResult::TARGET_HOSTILE: return "TARGET_HOSTILE";
        case ValidationResult::NO_TARGET_REQUIRED: return "NO_TARGET_REQUIRED";
        case ValidationResult::CASTER_DEAD: return "CASTER_DEAD";
        case ValidationResult::CASTER_MOVING: return "CASTER_MOVING";
        case ValidationResult::CASTER_CASTING: return "CASTER_CASTING";
        case ValidationResult::CASTER_STUNNED: return "CASTER_STUNNED";
        case ValidationResult::CASTER_SILENCED: return "CASTER_SILENCED";
        case ValidationResult::CASTER_PACIFIED: return "CASTER_PACIFIED";
        case ValidationResult::CASTER_INTERRUPTED: return "CASTER_INTERRUPTED";
        case ValidationResult::GCD_ACTIVE: return "GCD_ACTIVE";
        case ValidationResult::SPELL_IN_PROGRESS: return "SPELL_IN_PROGRESS";
        case ValidationResult::NOT_IN_COMBAT: return "NOT_IN_COMBAT";
        case ValidationResult::IN_COMBAT: return "IN_COMBAT";
        case ValidationResult::NOT_MOUNTED: return "NOT_MOUNTED";
        case ValidationResult::MOUNTED: return "MOUNTED";
        case ValidationResult::POSITION_INVALID: return "POSITION_INVALID";
        case ValidationResult::PLAYER_NULLPTR: return "PLAYER_NULLPTR";
        case ValidationResult::SESSION_NULLPTR: return "SESSION_NULLPTR";
        case ValidationResult::MAP_NULLPTR: return "MAP_NULLPTR";
        case ValidationResult::PACKET_BUILD_FAILED: return "PACKET_BUILD_FAILED";
        default: return "UNKNOWN";
    }
}

void SpellPacketBuilder::LogValidationFailure(
    Player const* caster,
    uint32 spellId,
    ValidationResult result,
    std::string const& reason)
{
    TC_LOG_WARN("playerbot.spells.validation",
        "Spell cast validation failed: caster={}, spell={}, result={}, reason={}",
        caster ? caster->GetName() : "nullptr",
        spellId,
        GetValidationResultString(result),
        reason);
}
