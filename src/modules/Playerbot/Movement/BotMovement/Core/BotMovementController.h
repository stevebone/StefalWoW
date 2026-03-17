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

#ifndef TRINITY_BOTMOVEMENTCONTROLLER_H
#define TRINITY_BOTMOVEMENTCONTROLLER_H

#include "Define.h"
#include "BotMovementDefines.h"
#include "Position.h"
#include <deque>
#include <memory>

class Unit;
class MovementStateMachine;
class StuckDetector;
class ValidatedPathGenerator;
struct Position;

class TC_GAME_API BotMovementController
{
public:
    explicit BotMovementController(Unit* owner);
    ~BotMovementController();

    BotMovementController(BotMovementController const&) = delete;
    BotMovementController& operator=(BotMovementController const&) = delete;

    // Owner access
    Unit* GetOwner() const { return _owner; }

    // Main update loop
    void Update(uint32 diff);

    // Validated movement API
    bool MoveToPosition(Position const& dest, bool forceDest = false);
    bool MoveFollow(Unit* target, float distance, float angle = 0.0f);

    // State queries
    MovementStateType GetCurrentState() const;
    bool IsStuck() const;
    bool IsMoving() const;
    bool IsInWater() const;
    bool IsFalling() const;

    // Position history
    Position const* GetLastPosition() const;
    void RecordPosition();
    std::deque<PositionSnapshot> const& GetPositionHistory() const { return _positionHistory; }

    // State machine access
    MovementStateMachine* GetStateMachine() const { return _stateMachine.get(); }

    // Stuck detector access
    StuckDetector* GetStuckDetector() const { return _stuckDetector.get(); }

    // Recovery
    void TriggerStuckRecovery();
    void ClearStuckState();

    // Force state synchronization with environment
    void SyncWithEnvironment();

private:
    Unit* _owner;
    std::unique_ptr<MovementStateMachine> _stateMachine;
    std::unique_ptr<StuckDetector> _stuckDetector;
    std::unique_ptr<ValidatedPathGenerator> _pathGenerator;

    std::deque<PositionSnapshot> _positionHistory;
    uint32 _totalTimePassed;
    uint32 _positionRecordTimer;
    uint32 _stateSyncTimer;

    static constexpr size_t MAX_POSITION_HISTORY = 100;
    static constexpr uint32 POSITION_RECORD_INTERVAL = 500;  // Record every 500ms
    static constexpr uint32 STATE_SYNC_INTERVAL = 200;       // Sync state every 200ms

    // Internal update methods
    void UpdateStateMachine(uint32 diff);
    void UpdateStuckDetection(uint32 diff);
    void UpdatePositionHistory(uint32 diff);
    void SyncMovementFlags();

    // State machine helpers
    void UpdateStateTransitions();
    MovementStateType DetermineAppropriateState() const;

    // Handle stuck state
    void HandleStuckState();
};

#endif // TRINITY_BOTMOVEMENTCONTROLLER_H
