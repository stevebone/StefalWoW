/*
 * Copyright (C) 2025+ TrinityCore Playerbot Integration
 */

#ifndef PLAYERBOT_AI_COORDINATION_BG_RESOURCERACE_RESOURCERACESCRIPTBASE_H
#define PLAYERBOT_AI_COORDINATION_BG_RESOURCERACE_RESOURCERACESCRIPTBASE_H

#include "../BGScriptBase.h"

namespace Playerbot::Coordination::Battleground
{

/**
 * @class ResourceRaceScriptBase
 * @brief Base class for resource race battlegrounds (e.g., Silvershard Mines)
 *
 * Resource race BGs involve escorting/capturing mobile objectives (carts)
 * that travel along tracks. Points are scored when carts reach destinations.
 */
class ResourceRaceScriptBase : public BGScriptBase
{
public:
    virtual ~ResourceRaceScriptBase() = default;

    // ========== IDENTIFICATION ==========
    bool IsDomination() const override { return false; }

    // ========== LIFECYCLE ==========
    void OnLoad(BattlegroundCoordinator* coordinator) override;
    void OnUpdate(uint32 diff) override;
    void OnEvent(const BGScriptEventData& event) override;

    // ========== STRATEGY ==========
    RoleDistribution GetRecommendedRoles(const StrategicDecision& decision,
        float scoreAdvantage, uint32 timeRemaining) const override;
    void AdjustStrategy(StrategicDecision& decision, float scoreAdvantage,
        uint32 controlledCount, uint32 totalObjectives, uint32 timeRemaining) const override;
    uint8 GetObjectiveAttackPriority(uint32 objectiveId, BGObjectiveState state, uint32 faction) const override;
    uint8 GetObjectiveDefensePriority(uint32 objectiveId, BGObjectiveState state, uint32 faction) const override;

    // ========== RESOURCE RACE SPECIFIC ==========
    virtual uint32 GetCartCount() const = 0;
    virtual Position GetCartPosition(uint32 cartId) const = 0;
    virtual float GetCartProgress(uint32 cartId) const = 0;
    virtual uint32 GetCartController(uint32 cartId) const = 0;  // 0 = neutral, ALLIANCE, HORDE
    virtual bool IsCartContested(uint32 cartId) const = 0;
    virtual uint32 GetPointsPerCapture() const = 0;

    // Track information
    virtual uint32 GetTrackCount() const { return 1; }
    virtual std::vector<Position> GetTrackWaypoints(uint32 trackId) const = 0;
    virtual uint32 GetCartOnTrack(uint32 trackId) const = 0;

    // Intersection logic (for Silvershard Mines)
    virtual bool HasIntersections() const { return false; }
    virtual std::vector<uint32> GetIntersectionIds() const { return {}; }
    virtual uint32 GetIntersectionDecisionTime(uint32 /*intersectionId*/) const { return 0; }

protected:
    // Cart state tracking
    struct CartState
    {
        uint32 id;
        Position position;
        float progress;         // 0.0 to 1.0
        uint32 controller;      // 0 = neutral, faction ID otherwise
        bool contested;
        uint32 trackId;
        uint32 nearIntersectionId;  // 0 if not near intersection
    };

    std::vector<CartState> m_cartStates;
    uint32 m_totalCarts = 0;

    // Helper methods
    void UpdateCartStates();
    uint32 GetCartsControlledByFaction(uint32 faction) const;
    float GetAverageCartProgress(uint32 faction) const;
    CartState* GetMostProgressedCart(uint32 faction);
    CartState* GetMostContestedCart();
};

} // namespace Playerbot::Coordination::Battleground

#endif
