/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "TradeManager.h"
#include "AI/BotAI.h"
#include "Events/BotEventData.h"
#include "Events/BotEventTypes.h"
#include "Player.h"
#include "Log.h"
#include <any>

namespace Playerbot
{
    using namespace Events;
    using namespace StateMachine;

    /**
     * TradeManager::OnEventInternal Implementation
     *
     * Handles 11 trade and gold-related events dispatched from observers.
     * Phase 7.2: Complete implementation - extracts event data and calls manager methods
     *
     * Trade Events:
     * - TRADE_INITIATED, TRADE_ACCEPTED, TRADE_CANCELLED
     * - TRADE_ITEM_ADDED, TRADE_GOLD_ADDED
     * - GOLD_RECEIVED, GOLD_SPENT, LOW_GOLD_WARNING
     * - VENDOR_PURCHASE, VENDOR_SALE, REPAIR_COST
     */
    void TradeManager::OnEventInternal(Events::BotEvent const& event)
    {
        // Early exit for non-trade events
    if (!event.IsTradeEvent())
            return;

        Player* bot = GetBot();
        if (!bot || !bot->IsInWorld())
            return;

        // Handle trade events with full implementation
    switch (event.type)
        {
            case StateMachine::EventType::TRADE_INITIATED:
            {
                // Extract trade initiation data
    if (!event.eventData.has_value())
                {
                    TC_LOG_WARN("module.playerbot", "TradeManager::OnEventInternal: TRADE_INITIATED event {} missing data", event.eventId);
                    ForceUpdate();
                    return;
                }

                TradeEventData tradeData;
                try
                {
                    tradeData = ::std::any_cast<TradeEventData>(event.eventData);
                }
                catch (::std::bad_any_cast const& e)
                {
                    TC_LOG_ERROR("module.playerbot", "TradeManager::OnEventInternal: Failed to cast TRADE_INITIATED data: {}", e.what());
                    ForceUpdate();
                    return;
                }

                TC_LOG_INFO("module.playerbot", "TradeManager: Bot {} initiated trade with partner {}",
                    bot->GetName(), tradeData.partnerGuid.ToString());

                // Call manager method to handle trade window opened
                // OnTradeStarted is called by TrinityCore hooks, so we just update state
                ForceUpdate();
                break;
            }
            case StateMachine::EventType::TRADE_ACCEPTED:
            {
                // Extract trade acceptance data
    if (event.eventData.has_value())
                {
                    try
                    {
                        TradeEventData tradeData = ::std::any_cast<TradeEventData>(event.eventData);
                        TC_LOG_INFO("module.playerbot", "TradeManager: Bot {} accepted trade with {} (Gold offered: {}, Gold received: {}, Items: {})",
                            bot->GetName(), tradeData.partnerGuid.ToString(),
                            tradeData.goldOffered, tradeData.goldReceived, tradeData.itemCount);

                        // Validate trade fairness before final acceptance
    if (!EvaluateTradeFairness())
                        {
                            TC_LOG_WARN("module.playerbot", "TradeManager: Bot {} trade may be unfair, considering cancellation",
                                bot->GetName());

                            // Trade validation failed - consider cancelling
    if (IsTradeScam())
                            {
                                CancelTrade("Potential scam detected");
                                return;
                            }
                        }

                        // Trade accepted successfully - OnTradeAccepted() called by core
                    }
                    catch (::std::bad_any_cast const&)
                    {
                        TC_LOG_WARN("module.playerbot", "TradeManager: Bot {} accepted trade (no details)", bot->GetName());
                    }
                }

                ForceUpdate();
                break;
            }
            case StateMachine::EventType::TRADE_CANCELLED:
            {
                // Extract cancellation data
    if (event.eventData.has_value())
                {
                    try
                    {
                        TradeEventData tradeData = ::std::any_cast<TradeEventData>(event.eventData);
                        TC_LOG_INFO("module.playerbot", "TradeManager: Bot {} trade cancelled with partner {}",
                            bot->GetName(), tradeData.partnerGuid.ToString());
                    }
                    catch (::std::bad_any_cast const&)
                    {
                        TC_LOG_INFO("module.playerbot", "TradeManager: Bot {} trade cancelled", bot->GetName());
                    }
                }

                // OnTradeCancelled() called by core hooks
                ForceUpdate();
                break;
            }

            case StateMachine::EventType::TRADE_ITEM_ADDED:
            {
                // Extract item addition data
    if (event.eventData.has_value())
                {
                    try
                    {
                        TradeEventData tradeData = ::std::any_cast<TradeEventData>(event.eventData);
                        TC_LOG_DEBUG("module.playerbot", "TradeManager: Item added to trade for bot {} (total items: {})",
                            bot->GetName(), tradeData.itemCount);

                        // Validate items in trade
    if (!ValidateTradeItems())
                        {
                            TC_LOG_WARN("module.playerbot", "TradeManager: Bot {} trade items validation failed", bot->GetName());
                        }
                    }
                    catch (::std::bad_any_cast const&) { }
                }

                ForceUpdate();
                break;
            }

            case StateMachine::EventType::TRADE_GOLD_ADDED:
            {
                // Extract gold addition data
    if (event.eventData.has_value())
                {
                    try
                    {
                        TradeEventData tradeData = ::std::any_cast<TradeEventData>(event.eventData);
                        TC_LOG_DEBUG("module.playerbot", "TradeManager: Gold added to trade for bot {} (offered: {}, received: {})",
                            bot->GetName(), tradeData.goldOffered, tradeData.goldReceived);

                        // Validate gold amounts
    if (tradeData.goldOffered > 0 && !ValidateTradeGold(tradeData.goldOffered))
                        {
                            TC_LOG_WARN("module.playerbot", "TradeManager: Bot {} cannot afford gold amount {}",
                                bot->GetName(), tradeData.goldOffered);
                            CancelTrade("Insufficient gold");
                            return;
                        }
                    }
                    catch (::std::bad_any_cast const&) { }
                }
                ForceUpdate();
                break;
            }

            case StateMachine::EventType::GOLD_RECEIVED:
            {
                // Extract gold received data
    if (event.eventData.has_value())
                {
                    try
                    {
                        GoldTransactionData goldData = ::std::any_cast<GoldTransactionData>(event.eventData);
                        TC_LOG_INFO("module.playerbot", "TradeManager: Bot {} received {} copper (source: {})",
                            bot->GetName(), goldData.amount,
                            goldData.source == 0 ? "quest" :
                            goldData.source == 1 ? "loot" :
                            goldData.source == 2 ? "auction" :
                            goldData.source == 3 ? "trade" : "vendor");

                        // Update statistics
                        // Statistics tracking is handled in TradeManager Update() cycle
                    }
                    catch (::std::bad_any_cast const&)
                    {
                        TC_LOG_DEBUG("module.playerbot", "TradeManager: Bot {} received gold (no details)", bot->GetName());
                    }
                }

                ForceUpdate();
                break;
            }

            case StateMachine::EventType::GOLD_SPENT:
            {
                // Extract gold spent data
    if (event.eventData.has_value())
                {
                    try
                    {
                        GoldTransactionData goldData = ::std::any_cast<GoldTransactionData>(event.eventData);
                        TC_LOG_INFO("module.playerbot", "TradeManager: Bot {} spent {} copper (source: {})",
                            bot->GetName(), goldData.amount,
                            goldData.source == 2 ? "auction" :
                            goldData.source == 3 ? "trade" : "vendor");

                        // Check if bot is running low on gold after this transaction
                        uint64 currentGold = bot->GetMoney();
                        if (currentGold < 1000000) // Less than 100g
                        {
                            TC_LOG_DEBUG("module.playerbot", "TradeManager: Bot {} gold level low: {} copper",
                                bot->GetName(), currentGold);
                        }
                    }
                    catch (::std::bad_any_cast const&)
                    {
                        TC_LOG_DEBUG("module.playerbot", "TradeManager: Bot {} spent gold (no details)", bot->GetName());
                    }
                }

                ForceUpdate();
                break;
            }
            case StateMachine::EventType::LOW_GOLD_WARNING:
            {
                // Handle low gold warning
                uint64 currentGold = bot->GetMoney();
                TC_LOG_WARN("module.playerbot", "TradeManager: Bot {} low gold warning (current: {} copper)",
                    bot->GetName(), currentGold);

                // Bot should prioritize gold generation activities
                // This is handled by higher-level strategy systems

                ForceUpdate();
                break;
            }

            case StateMachine::EventType::VENDOR_PURCHASE:
            {
                // Extract vendor purchase data
    if (event.eventData.has_value())
                {
                    try
                    {
                        VendorTransactionData vendorData = ::std::any_cast<VendorTransactionData>(event.eventData);
                        TC_LOG_INFO("module.playerbot", "TradeManager: Bot {} purchased item {} from vendor {} (Price: {} copper, Qty: {})",
                            bot->GetName(), vendorData.itemEntry, vendorData.vendorGuid.ToString(),
                            vendorData.price, vendorData.quantity);

                        // Update purchase statistics
                        // Statistics handled in TradeManager Update() cycle
                    }
                    catch (::std::bad_any_cast const&)
                    {
                        TC_LOG_DEBUG("module.playerbot", "TradeManager: Bot {} purchased from vendor (no details)", bot->GetName());
                    }
                }

                ForceUpdate();
                break;
            }

            case StateMachine::EventType::VENDOR_SALE:
            {
                // Extract vendor sale data
    if (event.eventData.has_value())
                {
                    try
                    {
                        VendorTransactionData vendorData = ::std::any_cast<VendorTransactionData>(event.eventData);
                        TC_LOG_INFO("module.playerbot", "TradeManager: Bot {} sold item {} to vendor {} (Price: {} copper, Qty: {})",
                            bot->GetName(), vendorData.itemEntry, vendorData.vendorGuid.ToString(),
                            vendorData.price, vendorData.quantity);

                        // Update sale statistics
                    }
                    catch (::std::bad_any_cast const&)
                    {
                        TC_LOG_DEBUG("module.playerbot", "TradeManager: Bot {} sold to vendor (no details)", bot->GetName());
                    }
                }

                ForceUpdate();
                break;
            }

            case StateMachine::EventType::REPAIR_COST:
            {
                // Extract repair cost data
    if (event.eventData.has_value())
                {
                    try
                    {
                        VendorTransactionData vendorData = ::std::any_cast<VendorTransactionData>(event.eventData);
                        TC_LOG_INFO("module.playerbot", "TradeManager: Bot {} paid repair cost {} copper to vendor {}",
                            bot->GetName(), vendorData.price, vendorData.vendorGuid.ToString());

                        // Check if repair cost was significant
    if (vendorData.price > 100000) // More than 10g
                        {
                            TC_LOG_WARN("module.playerbot", "TradeManager: Bot {} high repair cost: {} copper",
                                bot->GetName(), vendorData.price);
                        }

                        // Update repair statistics
                    }
                    catch (::std::bad_any_cast const&)
                    {
                        TC_LOG_DEBUG("module.playerbot", "TradeManager: Bot {} paid repair cost (no details)", bot->GetName());
                    }
                }

                ForceUpdate();
                break;
            }

            default:
                break;
        }
    }

} // namespace Playerbot
