/*
 * Copyright (C) 2025+ TrinityCore Playerbot Integration
 */

#include "ResourceRaceScriptBase.h"
#include "BattlegroundCoordinator.h"

namespace Playerbot::Coordination::Battleground
{

void ResourceRaceScriptBase::OnLoad(BattlegroundCoordinator* coordinator)
{
    BGScriptBase::OnLoad(coordinator);
    m_cartStates.clear();
    m_totalCarts = GetCartCount();

    // Initialize cart states
    for (uint32 i = 0; i < m_totalCarts; ++i)
    {
        CartState state;
        state.id = i;
        state.position = GetCartPosition(i);
        state.progress = 0.0f;
        state.controller = 0;
        state.contested = false;
        state.trackId = 0;
        state.nearIntersectionId = 0;
        m_cartStates.push_back(state);
    }
}

void ResourceRaceScriptBase::OnUpdate(uint32 diff)
{
    BGScriptBase::OnUpdate(diff);
    UpdateCartStates();
}

void ResourceRaceScriptBase::OnEvent(const BGScriptEventData& event)
{
    BGScriptBase::OnEvent(event);

    switch (event.eventType)
    {
        case BGScriptEvent::CART_CAPTURED:
            // Cart reached destination
            if (event.objectiveId < m_cartStates.size())
            {
                m_cartStates[event.objectiveId].progress = 0.0f;
                m_cartStates[event.objectiveId].controller = 0;
            }
            break;

        case BGScriptEvent::OBJECTIVE_CAPTURED:
            // Cart control changed
            if (event.objectiveId < m_cartStates.size())
            {
                m_cartStates[event.objectiveId].controller =
                    (event.newState == BGObjectiveState::ALLIANCE_CONTROLLED) ? ALLIANCE :
                    (event.newState == BGObjectiveState::HORDE_CONTROLLED) ? HORDE : 0;
                m_cartStates[event.objectiveId].contested = false;
            }
            break;

        case BGScriptEvent::OBJECTIVE_CONTESTED:
            if (event.objectiveId < m_cartStates.size())
            {
                m_cartStates[event.objectiveId].contested = true;
            }
            break;

        default:
            break;
    }
}

void ResourceRaceScriptBase::UpdateCartStates()
{
    for (auto& cart : m_cartStates)
    {
        cart.position = GetCartPosition(cart.id);
        cart.progress = GetCartProgress(cart.id);
        cart.controller = GetCartController(cart.id);
        cart.contested = IsCartContested(cart.id);
    }
}

RoleDistribution ResourceRaceScriptBase::GetRecommendedRoles(
    const StrategicDecision& decision, float /*scoreAdvantage*/, uint32 /*timeRemaining*/) const
{
    RoleDistribution dist;

    // Resource race BGs need cart escorts and interceptors
    switch (decision.strategy)
    {
        case BGStrategy::AGGRESSIVE:
            dist.roleCounts[BGRole::CART_PUSHER] = 40;
            dist.roleCounts[BGRole::NODE_ATTACKER] = 30;  // Contest enemy carts
            dist.roleCounts[BGRole::ROAMER] = 20;
            dist.roleCounts[BGRole::NODE_DEFENDER] = 10;
            dist.reasoning = "Aggressive cart push";
            break;

        case BGStrategy::DEFENSIVE:
            dist.roleCounts[BGRole::CART_PUSHER] = 50;
            dist.roleCounts[BGRole::NODE_DEFENDER] = 25;
            dist.roleCounts[BGRole::ROAMER] = 15;
            dist.roleCounts[BGRole::NODE_ATTACKER] = 10;
            dist.reasoning = "Defensive cart control";
            break;

        case BGStrategy::ALL_IN:
            dist.roleCounts[BGRole::CART_PUSHER] = 35;
            dist.roleCounts[BGRole::NODE_ATTACKER] = 40;
            dist.roleCounts[BGRole::ROAMER] = 25;
            dist.reasoning = "All-in cart push";
            break;

        default:  // BALANCED
            dist.roleCounts[BGRole::CART_PUSHER] = 35;
            dist.roleCounts[BGRole::NODE_ATTACKER] = 25;
            dist.roleCounts[BGRole::NODE_DEFENDER] = 20;
            dist.roleCounts[BGRole::ROAMER] = 20;
            dist.reasoning = "Balanced cart control";
            break;
    }

    return dist;
}

void ResourceRaceScriptBase::AdjustStrategy(StrategicDecision& decision, float scoreAdvantage,
    uint32 /*controlledCount*/, uint32 /*totalObjectives*/, uint32 timeRemaining) const
{
    uint32 faction = m_coordinator ? m_coordinator->GetFaction() : ALLIANCE;
    uint32 ourCarts = GetCartsControlledByFaction(faction);
    uint32 theirCarts = GetCartsControlledByFaction(faction == ALLIANCE ? HORDE : ALLIANCE);

    // Cart control comparison
    if (ourCarts >= 2 && scoreAdvantage > 0)
    {
        decision.strategy = BGStrategy::DEFENSIVE;
        decision.reasoning = "Controlling majority of carts - escort them";
        decision.defenseAllocation = 60;
        decision.offenseAllocation = 40;
    }
    else if (theirCarts > ourCarts)
    {
        decision.strategy = BGStrategy::AGGRESSIVE;
        decision.reasoning = "Need to contest enemy carts";
        decision.offenseAllocation = 65;
        decision.defenseAllocation = 35;
    }
    else if (scoreAdvantage < -0.2f && timeRemaining < 180000)
    {
        decision.strategy = BGStrategy::ALL_IN;
        decision.reasoning = "Behind with little time - contest everything!";
        decision.offenseAllocation = 80;
        decision.defenseAllocation = 20;
    }
    else
    {
        decision.strategy = BGStrategy::BALANCED;
        decision.reasoning = "Balanced cart control";
        decision.offenseAllocation = 50;
        decision.defenseAllocation = 50;
    }

    // Check for contested carts
    bool anyContested = false;
    for (const auto& cart : m_cartStates)
    {
        if (cart.contested)
        {
            anyContested = true;
            break;
        }
    }

    if (anyContested)
    {
        decision.reasoning += " + contested cart!";
        decision.offenseAllocation += 10;
    }
}

uint8 ResourceRaceScriptBase::GetObjectiveAttackPriority(uint32 objectiveId,
    BGObjectiveState state, uint32 faction) const
{
    if (objectiveId >= m_cartStates.size())
        return 0;

    const auto& cart = m_cartStates[objectiveId];

    // High priority for enemy carts near completion
    if (cart.controller != faction && cart.controller != 0)
    {
        if (cart.progress > 0.75f)
            return 10;  // Very high priority
        if (cart.progress > 0.5f)
            return 8;
        return 6;
    }

    // Medium priority for neutral carts
    if (cart.controller == 0)
        return 5;

    return 3;
}

uint8 ResourceRaceScriptBase::GetObjectiveDefensePriority(uint32 objectiveId,
    BGObjectiveState state, uint32 faction) const
{
    if (objectiveId >= m_cartStates.size())
        return 0;

    const auto& cart = m_cartStates[objectiveId];

    // High priority for our carts near completion
    if (cart.controller == faction)
    {
        if (cart.contested)
            return 10;  // Highest priority if contested
        if (cart.progress > 0.75f)
            return 9;   // Very close to scoring
        if (cart.progress > 0.5f)
            return 7;
        return 5;
    }

    return 2;
}

uint32 ResourceRaceScriptBase::GetCartsControlledByFaction(uint32 faction) const
{
    uint32 count = 0;
    for (const auto& cart : m_cartStates)
    {
        if (cart.controller == faction)
            ++count;
    }
    return count;
}

float ResourceRaceScriptBase::GetAverageCartProgress(uint32 faction) const
{
    float total = 0.0f;
    uint32 count = 0;

    for (const auto& cart : m_cartStates)
    {
        if (cart.controller == faction)
        {
            total += cart.progress;
            ++count;
        }
    }

    return count > 0 ? total / count : 0.0f;
}

ResourceRaceScriptBase::CartState* ResourceRaceScriptBase::GetMostProgressedCart(uint32 faction)
{
    CartState* best = nullptr;
    float bestProgress = -1.0f;

    for (auto& cart : m_cartStates)
    {
        if (cart.controller == faction && cart.progress > bestProgress)
        {
            best = &cart;
            bestProgress = cart.progress;
        }
    }

    return best;
}

ResourceRaceScriptBase::CartState* ResourceRaceScriptBase::GetMostContestedCart()
{
    for (auto& cart : m_cartStates)
    {
        if (cart.contested)
            return &cart;
    }
    return nullptr;
}

} // namespace Playerbot::Coordination::Battleground
