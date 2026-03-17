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

#define CATCH_CONFIG_ENABLE_CHRONO_STRINGMAKER
#include "tc_catch2.h"

#include "BotMovementManager.h"
#include "BotMovementController.h"
#include "PositionValidator.h"
#include "GroundValidator.h"
#include "ValidationResult.h"
#include "BotMovementDefines.h"
#include "Position.h"

TEST_CASE("Phase 1 Integration - BotMovementManager Singleton", "[BotMovement][Integration][Phase1]")
{
    SECTION("Manager instance is accessible")
    {
        BotMovementManager* manager = sBotMovementManager;
        REQUIRE(manager != nullptr);
    }

    SECTION("Multiple calls return same instance")
    {
        BotMovementManager* manager1 = sBotMovementManager;
        BotMovementManager* manager2 = sBotMovementManager;
        REQUIRE(manager1 == manager2);
    }

    SECTION("Manager has valid config")
    {
        BotMovementManager* manager = sBotMovementManager;
        BotMovementConfig const& config = manager->GetConfig();
        REQUIRE(config.GetValidationLevel() != ValidationLevel::None);
    }

    SECTION("Manager has path cache")
    {
        BotMovementManager* manager = sBotMovementManager;
        PathCache* cache = manager->GetPathCache();
        REQUIRE(cache != nullptr);
    }
}

TEST_CASE("Phase 1 Integration - PositionValidator Bounds", "[BotMovement][Integration][Phase1][Validation]")
{
    SECTION("Valid position within normal bounds")
    {
        Position pos(0.0f, 0.0f, 0.0f);
        ValidationResult result = PositionValidator::ValidateBounds(pos);
        REQUIRE(result.isValid == true);
        REQUIRE(result.failureReason == ValidationFailureReason::None);
    }

    SECTION("Valid position with normal coordinates")
    {
        Position pos(1000.0f, 1000.0f, 100.0f);
        ValidationResult result = PositionValidator::ValidateBounds(pos);
        REQUIRE(result.isValid == true);
        REQUIRE(result.failureReason == ValidationFailureReason::None);
    }

    SECTION("Invalid position - NaN X coordinate")
    {
        float nanValue = std::numeric_limits<float>::quiet_NaN();
        ValidationResult result = PositionValidator::ValidateBounds(nanValue, 0.0f, 0.0f);
        REQUIRE(result.isValid == false);
        REQUIRE(result.failureReason == ValidationFailureReason::InvalidPosition);
        REQUIRE_FALSE(result.errorMessage.empty());
    }

    SECTION("Invalid position - NaN Y coordinate")
    {
        float nanValue = std::numeric_limits<float>::quiet_NaN();
        ValidationResult result = PositionValidator::ValidateBounds(0.0f, nanValue, 0.0f);
        REQUIRE(result.isValid == false);
        REQUIRE(result.failureReason == ValidationFailureReason::InvalidPosition);
    }

    SECTION("Invalid position - NaN Z coordinate")
    {
        float nanValue = std::numeric_limits<float>::quiet_NaN();
        ValidationResult result = PositionValidator::ValidateBounds(0.0f, 0.0f, nanValue);
        REQUIRE(result.isValid == false);
        REQUIRE(result.failureReason == ValidationFailureReason::InvalidPosition);
    }

    SECTION("Invalid position - Infinity X coordinate")
    {
        float infValue = std::numeric_limits<float>::infinity();
        ValidationResult result = PositionValidator::ValidateBounds(infValue, 0.0f, 0.0f);
        REQUIRE(result.isValid == false);
        REQUIRE(result.failureReason == ValidationFailureReason::OutOfBounds);
    }

    SECTION("Invalid position - Negative infinity")
    {
        float negInfValue = -std::numeric_limits<float>::infinity();
        ValidationResult result = PositionValidator::ValidateBounds(negInfValue, 0.0f, 0.0f);
        REQUIRE(result.isValid == false);
        REQUIRE(result.failureReason == ValidationFailureReason::OutOfBounds);
    }

    SECTION("Invalid position - extremely large coordinates")
    {
        ValidationResult result = PositionValidator::ValidateBounds(1000000.0f, 1000000.0f, 100000.0f);
        REQUIRE(result.isValid == false);
        REQUIRE(result.failureReason == ValidationFailureReason::OutOfBounds);
    }

    SECTION("Invalid position - extremely small coordinates")
    {
        ValidationResult result = PositionValidator::ValidateBounds(-1000000.0f, -1000000.0f, -100000.0f);
        REQUIRE(result.isValid == false);
        REQUIRE(result.failureReason == ValidationFailureReason::OutOfBounds);
    }
}

TEST_CASE("Phase 1 Integration - PositionValidator Map ID", "[BotMovement][Integration][Phase1][Validation]")
{
    SECTION("Valid map ID 0 (Eastern Kingdoms)")
    {
        ValidationResult result = PositionValidator::ValidateMapId(0);
        REQUIRE(result.isValid == true);
        REQUIRE(result.failureReason == ValidationFailureReason::None);
    }

    SECTION("Valid map ID 1 (Kalimdor)")
    {
        ValidationResult result = PositionValidator::ValidateMapId(1);
        REQUIRE(result.isValid == true);
        REQUIRE(result.failureReason == ValidationFailureReason::None);
    }

    SECTION("Invalid map ID - out of range")
    {
        ValidationResult result = PositionValidator::ValidateMapId(999999);
        REQUIRE(result.isValid == false);
        REQUIRE(result.failureReason == ValidationFailureReason::InvalidMapId);
        REQUIRE_FALSE(result.errorMessage.empty());
    }
}

TEST_CASE("Phase 1 Integration - PositionValidator Combined Validation", "[BotMovement][Integration][Phase1][Validation]")
{
    SECTION("Valid position on valid map")
    {
        Position pos(0.0f, 0.0f, 0.0f);
        ValidationResult result = PositionValidator::ValidatePosition(0, pos);
        REQUIRE(result.isValid == true);
        REQUIRE(result.failureReason == ValidationFailureReason::None);
    }

    SECTION("Invalid position on valid map - NaN coordinates")
    {
        float nanValue = std::numeric_limits<float>::quiet_NaN();
        ValidationResult result = PositionValidator::ValidatePosition(0, nanValue, 0.0f, 0.0f);
        REQUIRE(result.isValid == false);
        REQUIRE(result.failureReason == ValidationFailureReason::InvalidPosition);
    }

    SECTION("Valid position on invalid map")
    {
        Position pos(0.0f, 0.0f, 0.0f);
        ValidationResult result = PositionValidator::ValidatePosition(999999, pos);
        REQUIRE(result.isValid == false);
        REQUIRE(result.failureReason == ValidationFailureReason::InvalidMapId);
    }

    SECTION("Invalid position on invalid map - both checks fail")
    {
        float nanValue = std::numeric_limits<float>::quiet_NaN();
        ValidationResult result = PositionValidator::ValidatePosition(999999, nanValue, 0.0f, 0.0f);
        REQUIRE(result.isValid == false);
        REQUIRE((result.failureReason == ValidationFailureReason::InvalidPosition || 
                 result.failureReason == ValidationFailureReason::InvalidMapId));
    }
}

TEST_CASE("Phase 1 Integration - ValidationResult Structure", "[BotMovement][Integration][Phase1][Validation]")
{
    SECTION("Success factory method")
    {
        ValidationResult result = ValidationResult::Success();
        REQUIRE(result.isValid == true);
        REQUIRE(result.failureReason == ValidationFailureReason::None);
        REQUIRE(result.errorMessage.empty());
    }

    SECTION("Failure factory method")
    {
        ValidationResult result = ValidationResult::Failure(
            ValidationFailureReason::CollisionDetected,
            "Wall collision detected"
        );
        REQUIRE(result.isValid == false);
        REQUIRE(result.failureReason == ValidationFailureReason::CollisionDetected);
        REQUIRE(result.errorMessage == "Wall collision detected");
    }

    SECTION("Default constructor")
    {
        ValidationResult result;
        REQUIRE(result.isValid == false);
        REQUIRE(result.failureReason == ValidationFailureReason::None);
    }
}

TEST_CASE("Phase 1 Integration - BotMovementDefines Enums", "[BotMovement][Integration][Phase1]")
{
    SECTION("MovementStateType values are distinct")
    {
        REQUIRE(MovementStateType::Idle != MovementStateType::Ground);
        REQUIRE(MovementStateType::Ground != MovementStateType::Swimming);
        REQUIRE(MovementStateType::Swimming != MovementStateType::Flying);
        REQUIRE(MovementStateType::Flying != MovementStateType::Falling);
        REQUIRE(MovementStateType::Falling != MovementStateType::Stuck);
    }

    SECTION("ValidationFailureReason values are distinct")
    {
        REQUIRE(ValidationFailureReason::None != ValidationFailureReason::InvalidPosition);
        REQUIRE(ValidationFailureReason::InvalidPosition != ValidationFailureReason::OutOfBounds);
        REQUIRE(ValidationFailureReason::OutOfBounds != ValidationFailureReason::InvalidMapId);
        REQUIRE(ValidationFailureReason::CollisionDetected != ValidationFailureReason::PathBlocked);
    }

    SECTION("ValidationLevel values are ordered correctly")
    {
        REQUIRE(static_cast<uint8>(ValidationLevel::None) < static_cast<uint8>(ValidationLevel::Basic));
        REQUIRE(static_cast<uint8>(ValidationLevel::Basic) < static_cast<uint8>(ValidationLevel::Standard));
        REQUIRE(static_cast<uint8>(ValidationLevel::Standard) < static_cast<uint8>(ValidationLevel::Strict));
    }

    SECTION("RecoveryLevel values are ordered correctly")
    {
        REQUIRE(static_cast<uint8>(RecoveryLevel::Level1_RecalculatePath) < 
                static_cast<uint8>(RecoveryLevel::Level2_BackupAndRetry));
        REQUIRE(static_cast<uint8>(RecoveryLevel::Level2_BackupAndRetry) < 
                static_cast<uint8>(RecoveryLevel::Level3_RandomNearbyPosition));
        REQUIRE(static_cast<uint8>(RecoveryLevel::Level3_RandomNearbyPosition) < 
                static_cast<uint8>(RecoveryLevel::Level4_TeleportToSafePosition));
        REQUIRE(static_cast<uint8>(RecoveryLevel::Level4_TeleportToSafePosition) < 
                static_cast<uint8>(RecoveryLevel::Level5_EvadeAndReset));
    }
}

TEST_CASE("Phase 1 Integration - PositionSnapshot Structure", "[BotMovement][Integration][Phase1]")
{
    SECTION("Default constructor initializes correctly")
    {
        PositionSnapshot snapshot;
        REQUIRE(snapshot.timestamp == 0);
    }

    SECTION("Parameterized constructor")
    {
        Position pos(100.0f, 200.0f, 50.0f);
        uint32 time = 12345;
        PositionSnapshot snapshot(pos, time);
        
        REQUIRE(snapshot.pos.GetPositionX() == 100.0f);
        REQUIRE(snapshot.pos.GetPositionY() == 200.0f);
        REQUIRE(snapshot.pos.GetPositionZ() == 50.0f);
        REQUIRE(snapshot.timestamp == time);
    }

    SECTION("Copy position data correctly")
    {
        Position pos1(1.0f, 2.0f, 3.0f);
        PositionSnapshot snapshot1(pos1, 100);
        
        Position pos2(10.0f, 20.0f, 30.0f);
        PositionSnapshot snapshot2(pos2, 200);
        
        REQUIRE(snapshot1.pos.GetPositionX() != snapshot2.pos.GetPositionX());
        REQUIRE(snapshot1.timestamp != snapshot2.timestamp);
    }
}

TEST_CASE("Phase 1 Integration - Validation Pipeline Correctness", "[BotMovement][Integration][Phase1][Validation]")
{
    SECTION("Sequential validation - first check passes, all checks run")
    {
        Position validPos(100.0f, 100.0f, 10.0f);
        
        ValidationResult boundsResult = PositionValidator::ValidateBounds(validPos);
        REQUIRE(boundsResult.isValid == true);
        
        ValidationResult mapResult = PositionValidator::ValidateMapId(0);
        REQUIRE(mapResult.isValid == true);
        
        ValidationResult combinedResult = PositionValidator::ValidatePosition(0, validPos);
        REQUIRE(combinedResult.isValid == true);
    }

    SECTION("Sequential validation - first check fails, error captured")
    {
        float nanValue = std::numeric_limits<float>::quiet_NaN();
        
        ValidationResult boundsResult = PositionValidator::ValidateBounds(nanValue, 0.0f, 0.0f);
        REQUIRE(boundsResult.isValid == false);
        REQUIRE(boundsResult.failureReason == ValidationFailureReason::InvalidPosition);
        
        ValidationResult combinedResult = PositionValidator::ValidatePosition(0, nanValue, 0.0f, 0.0f);
        REQUIRE(combinedResult.isValid == false);
        REQUIRE(combinedResult.failureReason == ValidationFailureReason::InvalidPosition);
    }

    SECTION("Multiple validation failures - first failure is reported")
    {
        float nanValue = std::numeric_limits<float>::quiet_NaN();
        uint32 invalidMapId = 999999;
        
        ValidationResult result = PositionValidator::ValidatePosition(invalidMapId, nanValue, 0.0f, 0.0f);
        REQUIRE(result.isValid == false);
    }
}

TEST_CASE("Phase 1 Integration - Config and Manager Integration", "[BotMovement][Integration][Phase1]")
{
    SECTION("Manager config can be reloaded")
    {
        BotMovementManager* manager = sBotMovementManager;
        REQUIRE(manager != nullptr);
        
        REQUIRE_NOTHROW(manager->ReloadConfig());
        
        BotMovementConfig const& config = manager->GetConfig();
        REQUIRE(config.GetValidationLevel() != ValidationLevel::None);
    }

    SECTION("Manager metrics are accessible")
    {
        BotMovementManager* manager = sBotMovementManager;
        REQUIRE(manager != nullptr);
        
        REQUIRE_NOTHROW(manager->GetGlobalMetrics());
    }

    SECTION("Manager metrics can be reset")
    {
        BotMovementManager* manager = sBotMovementManager;
        REQUIRE(manager != nullptr);
        
        REQUIRE_NOTHROW(manager->ResetMetrics());
    }
}

TEST_CASE("Phase 1 Integration - Edge Cases and Boundary Conditions", "[BotMovement][Integration][Phase1][Validation]")
{
    SECTION("Position at origin is valid")
    {
        Position origin(0.0f, 0.0f, 0.0f);
        ValidationResult result = PositionValidator::ValidateBounds(origin);
        REQUIRE(result.isValid == true);
    }

    SECTION("Position with very small positive values")
    {
        Position pos(0.001f, 0.001f, 0.001f);
        ValidationResult result = PositionValidator::ValidateBounds(pos);
        REQUIRE(result.isValid == true);
    }

    SECTION("Position with very small negative values")
    {
        Position pos(-0.001f, -0.001f, -0.001f);
        ValidationResult result = PositionValidator::ValidateBounds(pos);
        REQUIRE(result.isValid == true);
    }

    SECTION("Position with mixed positive and negative coordinates")
    {
        Position pos(-100.0f, 100.0f, -50.0f);
        ValidationResult result = PositionValidator::ValidateBounds(pos);
        REQUIRE(result.isValid == true);
    }

    SECTION("Large but valid coordinates")
    {
        Position pos(10000.0f, 10000.0f, 1000.0f);
        ValidationResult result = PositionValidator::ValidateBounds(pos);
        REQUIRE(result.isValid == true);
    }

    SECTION("Z coordinate at extremes")
    {
        Position highZ(0.0f, 0.0f, 5000.0f);
        ValidationResult result = PositionValidator::ValidateBounds(highZ);
        REQUIRE(result.isValid == true);
    }
}

TEST_CASE("Phase 1 Integration - Error Message Quality", "[BotMovement][Integration][Phase1][Validation]")
{
    SECTION("Invalid position error has descriptive message")
    {
        float nanValue = std::numeric_limits<float>::quiet_NaN();
        ValidationResult result = PositionValidator::ValidateBounds(nanValue, 0.0f, 0.0f);
        REQUIRE(result.isValid == false);
        REQUIRE_FALSE(result.errorMessage.empty());
        REQUIRE(result.errorMessage.length() > 10);
    }

    SECTION("Out of bounds error has descriptive message")
    {
        ValidationResult result = PositionValidator::ValidateBounds(1000000.0f, 0.0f, 0.0f);
        REQUIRE(result.isValid == false);
        REQUIRE_FALSE(result.errorMessage.empty());
        REQUIRE(result.errorMessage.length() > 10);
    }

    SECTION("Invalid map ID error has descriptive message")
    {
        ValidationResult result = PositionValidator::ValidateMapId(999999);
        REQUIRE(result.isValid == false);
        REQUIRE_FALSE(result.errorMessage.empty());
        REQUIRE(result.errorMessage.length() > 10);
    }
}
