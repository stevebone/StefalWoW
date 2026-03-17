# Social Systems Integration - Phase 3 Sprint 3D

## Trade, Auction House, Guild, and Social Interactions

## Overview

This document details the implementation of social systems for bots, including trading, auction house usage, guild participation, and general social interactions. These systems enable bots to engage in the game's economy and social aspects realistically.

## Trade System

### Automated Trading

```cpp
// src/modules/Playerbot/AI/Social/TradeManager.h

class BotTradeManager {
public:
    enum TradeDecision {
        ACCEPT_TRADE,
        DECLINE_TRADE,
        COUNTER_OFFER,
        WAIT_FOR_CHANGE
    };
    
    struct TradeEvaluation {
        float myValue;
        float theirValue;
        bool isFair;
        bool isBeneficial;
        std::string reason;
    };
    
    BotTradeManager(Player* bot) : _bot(bot) {}
    
    // Trade initiation
    bool ShouldInitiateTrade(Player* target);
    void RequestTrade(Player* target, const std::vector<Item*>& items);
    
    // Trade response
    TradeDecision EvaluateTrade();
    void RespondToTradeRequest(Player* trader);
    void HandleTradeWindow();
    
    // Item offering
    void OfferItems();
    void OfferGold(uint32 amount);
    bool ShouldTradeItem(Item* item);
    
    // Trade evaluation
    TradeEvaluation EvaluateCurrentTrade();
    float CalculateItemValue(Item* item);
    bool IsNeededItem(Item* item);
    
    // Enchanting/crafting trades
    void RequestEnchant(Player* enchanter, Item* item, uint32 enchantId);
    void OfferCraftingService(Player* customer);
    
private:
    Player* _bot;
    Player* _tradingPartner = nullptr;
    
    TradeEvaluation EvaluateCurrentTrade() {
        TradeEvaluation eval = {};
        
        if (!_tradingPartner)
            return eval;
        
        // Calculate my offered value
        for (uint8 i = 0; i < TRADE_SLOT_COUNT; ++i) {
            if (Item* item = _bot->GetTradeData()->GetItem(TradeSlots(i))) {
                eval.myValue += CalculateItemValue(item);
            }
        }
        eval.myValue += _bot->GetTradeData()->GetMoney() / 10000.0f;
        
        // Calculate their offered value
        for (uint8 i = 0; i < TRADE_SLOT_COUNT; ++i) {
            if (Item* item = _tradingPartner->GetTradeData()->GetItem(TradeSlots(i))) {
                eval.theirValue += CalculateItemValue(item);
                
                // Bonus if we need the item
                if (IsNeededItem(item)) {
                    eval.theirValue *= 1.5f;
                }
            }
        }
        eval.theirValue += _tradingPartner->GetTradeData()->GetMoney() / 10000.0f;
        
        // Evaluate fairness
        float ratio = eval.myValue > 0 ? eval.theirValue / eval.myValue : 999.0f;
        eval.isFair = ratio >= 0.8f && ratio <= 1.2f;
        eval.isBeneficial = ratio >= 1.0f;
        
        if (ratio < 0.5f) {
            eval.reason = "Trade heavily favors them";
        } else if (ratio < 0.8f) {
            eval.reason = "Trade slightly favors them";
        } else if (ratio <= 1.2f) {
            eval.reason = "Fair trade";
        } else if (ratio <= 2.0f) {
            eval.reason = "Trade slightly favors me";
        } else {
            eval.reason = "Trade heavily favors me";
        }
        
        return eval;
    }
    
    float CalculateItemValue(Item* item) {
        if (!item)
            return 0.0f;
        
        ItemTemplate const* proto = item->GetTemplate();
        float value = 0.0f;
        
        // Base value from vendor price
        value = proto->SellPrice / 10000.0f;
        
        // Adjust for rarity
        switch (proto->Quality) {
            case ITEM_QUALITY_POOR: value *= 0.5f; break;
            case ITEM_QUALITY_NORMAL: value *= 1.0f; break;
            case ITEM_QUALITY_UNCOMMON: value *= 2.0f; break;
            case ITEM_QUALITY_RARE: value *= 5.0f; break;
            case ITEM_QUALITY_EPIC: value *= 20.0f; break;
            case ITEM_QUALITY_LEGENDARY: value *= 100.0f; break;
        }
        
        // Adjust for usefulness
        if (item->IsEquipable() && _bot->CanUseItem(proto) == EQUIP_ERR_OK) {
            if (IsUpgrade(proto)) {
                value *= 2.0f;
            }
        }
        
        // Stack size
        value *= item->GetCount();
        
        return value;
    }
    
    void OfferItems() {
        // Offer items the trader might need
        std::vector<Item*> tradeableItems;
        
        // Check bags for tradeable items
        for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i) {
            if (Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i)) {
                if (!item->IsSoulBound() && ShouldTradeItem(item)) {
                    tradeableItems.push_back(item);
                }
            }
        }
        
        // Sort by value (ascending, trade less valuable first)
        std::sort(tradeableItems.begin(), tradeableItems.end(),
            [this](Item* a, Item* b) {
                return CalculateItemValue(a) < CalculateItemValue(b);
            });
        
        // Add items to trade
        uint8 slot = 0;
        for (Item* item : tradeableItems) {
            if (slot >= TRADE_SLOT_COUNT)
                break;
            
            if (IsUsefulForPlayer(item, _tradingPartner)) {
                _bot->GetTradeData()->SetItem(TradeSlots(slot), item);
                slot++;
            }
        }
    }
};
```

## Auction House System

### Automated Auction House

```cpp
// src/modules/Playerbot/AI/Social/AuctionHouseManager.h

class BotAuctionHouseManager {
public:
    struct AuctionStrategy {
        float buyoutMultiplier;    // Times vendor price
        float bidMultiplier;       // Times vendor price for starting bid
        uint32 shortDuration;      // 2 hours in seconds
        uint32 mediumDuration;     // 8 hours
        uint32 longDuration;       // 24 hours
        float undercutPercent;     // Undercut competition by this %
    };
    
    struct MarketData {
        uint32 itemId;
        uint32 averagePrice;
        uint32 lowestPrice;
        uint32 supply;
        uint32 demand;
        float priceHistory[7];     // Last 7 days
    };
    
    BotAuctionHouseManager(Player* bot) : _bot(bot) {
        LoadMarketData();
    }
    
    // Buying operations
    void ScanForDeals();
    bool ShouldBuyItem(uint32 itemId, uint32 price);
    void BuyAuction(AuctionEntry* auction);
    void SearchForItem(uint32 itemId);
    
    // Selling operations
    void PostAuctions();
    uint32 CalculateSellPrice(Item* item);
    uint32 SelectDuration(Item* item);
    void CreateAuction(Item* item, uint32 bid, uint32 buyout, uint32 duration);
    
    // Market analysis
    MarketData AnalyzeMarket(uint32 itemId);
    bool IsOverpriced(uint32 itemId, uint32 price);
    bool IsUnderpriced(uint32 itemId, uint32 price);
    float GetMarketTrend(uint32 itemId);
    
    // Inventory management
    std::vector<Item*> GetItemsToSell();
    bool ShouldSellItem(Item* item);
    bool ShouldKeepForUse(Item* item);
    
private:
    Player* _bot;
    AuctionStrategy _strategy;
    phmap::flat_hash_map<uint32, MarketData> _marketData;
    
    void ScanForDeals() {
        AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap(
            _bot->GetFaction());
        
        if (!auctionHouse)
            return;
        
        // Scan all auctions
        auctionHouse->BuildListAuctionItems(data, _bot,
            "", 0, 0, 0, 0, 0, 0, 
            AUCTION_SORT_PRICE, false);
        
        for (AuctionEntry* auction : auctions) {
            // Check if it's a good deal
            if (IsGoodDeal(auction)) {
                // Check if we have enough gold
                if (_bot->GetMoney() >= auction->buyout) {
                    BuyAuction(auction);
                    return;  // Buy one at a time
                }
            }
        }
    }
    
    bool IsGoodDeal(AuctionEntry* auction) {
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(auction->item_template);
        if (!proto)
            return false;
        
        // Get market data
        MarketData market = AnalyzeMarket(auction->item_template);
        
        // Check if significantly below market price
        if (auction->buyout > 0) {
            if (auction->buyout < market.averagePrice * 0.7f) {
                // 30% below average - good deal
                return true;
            }
        }
        
        // Check if we need the item
        if (_bot->CanUseItem(proto) == EQUIP_ERR_OK) {
            if (IsUpgrade(proto)) {
                // Willing to pay market price for upgrades
                return auction->buyout <= market.averagePrice;
            }
        }
        
        // Check for crafting materials we need
        if (proto->Class == ITEM_CLASS_TRADE_GOODS) {
            if (NeedForProfession(proto)) {
                return auction->buyout <= market.averagePrice * 0.9f;
            }
        }
        
        return false;
    }
    
    void PostAuctions() {
        auto itemsToSell = GetItemsToSell();
        
        for (Item* item : itemsToSell) {
            // Analyze market for this item
            MarketData market = AnalyzeMarket(item->GetEntry());
            
            // Skip if market is flooded
            if (market.supply > market.demand * 3) {
                continue;
            }
            
            // Calculate prices
            uint32 buyout = CalculateSellPrice(item);
            uint32 bid = buyout * 0.8f;  // Start bid at 80% of buyout
            
            // Undercut competition slightly
            if (market.lowestPrice > 0 && market.lowestPrice < buyout) {
                buyout = market.lowestPrice * (1.0f - _strategy.undercutPercent);
                bid = buyout * 0.8f;
            }
            
            // Select duration based on item type
            uint32 duration = SelectDuration(item);
            
            // Create auction
            CreateAuction(item, bid, buyout, duration);
        }
    }
    
    uint32 SelectDuration(Item* item) {
        ItemTemplate const* proto = item->GetTemplate();
        
        // Consumables - short duration
        if (proto->Class == ITEM_CLASS_CONSUMABLE) {
            return _strategy.shortDuration;
        }
        
        // Trade goods - medium duration
        if (proto->Class == ITEM_CLASS_TRADE_GOODS) {
            return _strategy.mediumDuration;
        }
        
        // Equipment - long duration
        if (proto->Class == ITEM_CLASS_WEAPON || 
            proto->Class == ITEM_CLASS_ARMOR) {
            return _strategy.longDuration;
        }
        
        return _strategy.mediumDuration;
    }
};
```

## Guild System

### Guild Participation

```cpp
// src/modules/Playerbot/AI/Social/GuildManager.h

class BotGuildManager {
public:
    struct GuildActivity {
        enum Type {
            GUILD_CHAT,
            GUILD_EVENT,
            GUILD_DUNGEON,
            GUILD_RAID,
            GUILD_PVP,
            GUILD_CRAFTING
        };
        
        Type type;
        uint32 scheduledTime;
        std::vector<ObjectGuid> participants;
        std::string description;
    };
    
    BotGuildManager(Player* bot) : _bot(bot) {}
    
    // Guild membership
    void AcceptGuildInvite();
    void RequestGuildInvite(Guild* guild);
    bool ShouldJoinGuild(Guild* guild);
    
    // Guild bank
    void ContributeToGuildBank();
    void RequestFromGuildBank(uint32 itemId);
    bool CanWithdrawItem(uint32 tab, uint8 slot);
    
    // Guild activities
    void ParticipateInGuildEvent(const GuildActivity& activity);
    void RespondToGuildChat(const std::string& message);
    void OfferCraftingServices();
    
    // Guild progression
    void ContributeToGuildXP();
    void CompleteGuildQuests();
    void DonateGuildMaterials();
    
    // Social interaction
    void GreetGuildMembers();
    void CongratulateAchievement(Player* member, uint32 achievementId);
    void OfferHelp(Player* member);
    
private:
    Player* _bot;
    
    void RespondToGuildChat(const std::string& message) {
        // Parse message for keywords
        std::string response;
        
        // Greetings
        if (ContainsGreeting(message)) {
            response = GenerateGreeting();
        }
        // Help requests
        else if (ContainsHelpRequest(message)) {
            if (CanHelp(message)) {
                response = "I can help with that!";
                OfferAssistance();
            }
        }
        // LFG/LFM
        else if (ContainsLFG(message)) {
            if (IsAvailable()) {
                response = "I'm available to join!";
            }
        }
        // Congratulations
        else if (ContainsAchievement(message)) {
            response = "Congratulations!";
        }
        
        if (!response.empty()) {
            Guild* guild = _bot->GetGuild();
            if (guild) {
                guild->BroadcastToGuild(_bot->GetSession(), false, 
                    response, LANG_UNIVERSAL);
            }
        }
    }
    
    void ContributeToGuildBank() {
        Guild* guild = _bot->GetGuild();
        if (!guild)
            return;
        
        // Find items to donate
        std::vector<Item*> donateItems;
        
        for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i) {
            if (Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i)) {
                if (ShouldDonate(item)) {
                    donateItems.push_back(item);
                }
            }
        }
        
        // Deposit items
        for (Item* item : donateItems) {
            for (uint8 tab = 0; tab < guild->GetPurchasedTabsSize(); ++tab) {
                if (guild->CanStoreItem(tab, item)) {
                    guild->SwapItems(_bot, tab, 
                        guild->GetFirstFreeSlot(tab), 
                        item->GetBagSlot(), item->GetSlot(), 
                        item->GetCount());
                    break;
                }
            }
        }
        
        // Donate gold if wealthy
        if (_bot->GetMoney() > 10000 * GOLD) {
            uint32 donateAmount = (_bot->GetMoney() - 10000 * GOLD) / 10;
            guild->HandleMemberDepositMoney(_bot->GetSession(), 
                donateAmount, false);
        }
    }
    
    bool ShouldDonate(Item* item) {
        ItemTemplate const* proto = item->GetTemplate();
        
        // Don't donate soulbound items
        if (item->IsSoulBound())
            return false;
        
        // Don't donate items we need
        if (IsNeeded(item))
            return false;
        
        // Donate trade goods
        if (proto->Class == ITEM_CLASS_TRADE_GOODS)
            return true;
        
        // Donate excess consumables
        if (proto->Class == ITEM_CLASS_CONSUMABLE) {
            uint32 count = _bot->GetItemCount(item->GetEntry());
            if (count > 20)
                return true;
        }
        
        // Donate BoE items we can't use
        if (!item->IsSoulBound() && 
            _bot->CanUseItem(proto) != EQUIP_ERR_OK) {
            return true;
        }
        
        return false;
    }
};
```

## Chat System

### Intelligent Chat Responses

```cpp
// src/modules/Playerbot/AI/Social/ChatManager.h

class BotChatManager {
public:
    struct ChatContext {
        Player* sender;
        std::string message;
        uint32 type;
        uint32 language;
        time_t timestamp;
    };
    
    struct ChatResponse {
        std::string text;
        uint32 delay;  // Response delay in ms
        float confidence;
    };
    
    BotChatManager(Player* bot) : _bot(bot) {
        LoadChatPatterns();
    }
    
    // Message handling
    void HandleWhisper(Player* sender, const std::string& message);
    void HandleSay(Player* sender, const std::string& message);
    void HandlePartyChat(Player* sender, const std::string& message);
    void HandleRaidChat(Player* sender, const std::string& message);
    
    // Response generation
    ChatResponse GenerateResponse(const ChatContext& context);
    std::string GenerateGreeting();
    std::string GenerateFarewell();
    std::string GenerateThanks();
    
    // Context understanding
    bool IsQuestion(const std::string& message);
    bool IsGreeting(const std::string& message);
    bool IsCommand(const std::string& message);
    
    // Emotes
    void RespondWithEmote(uint32 emoteId);
    uint32 SelectAppropriateEmote(const ChatContext& context);
    
private:
    Player* _bot;
    std::deque<ChatContext> _chatHistory;
    phmap::flat_hash_map<std::string, std::vector<std::string>> _responsePatterns;
    
    void LoadChatPatterns() {
        // Greetings
        _responsePatterns["greeting"] = {
            "Hello!",
            "Hi there!",
            "Hey!",
            "Greetings!",
            "Hello, friend!"
        };
        
        // Thanks
        _responsePatterns["thanks"] = {
            "You're welcome!",
            "No problem!",
            "Happy to help!",
            "Anytime!",
            "My pleasure!"
        };
        
        // Agreement
        _responsePatterns["agree"] = {
            "I agree",
            "Sounds good",
            "Sure thing",
            "Absolutely",
            "Let's do it"
        };
        
        // Disagreement
        _responsePatterns["disagree"] = {
            "I'm not sure about that",
            "Maybe we should reconsider",
            "I have a different idea",
            "Let me think about it",
            "Hmm, I don't know"
        };
        
        // Combat
        _responsePatterns["combat_start"] = {
            "Let's go!",
            "For the Alliance!",  // or Horde
            "Attack!",
            "Charge!",
            "Here we go!"
        };
        
        // Load more patterns...
    }
    
    ChatResponse GenerateResponse(const ChatContext& context) {
        ChatResponse response;
        
        // Analyze message
        std::string lowerMsg = ToLower(context.message);
        
        // Check for patterns
        if (IsGreeting(lowerMsg)) {
            response.text = SelectRandom(_responsePatterns["greeting"]);
            response.delay = RandomDelay(500, 2000);
            response.confidence = 0.9f;
        }
        else if (Contains(lowerMsg, {"thanks", "thank you", "ty", "thx"})) {
            response.text = SelectRandom(_responsePatterns["thanks"]);
            response.delay = RandomDelay(1000, 3000);
            response.confidence = 0.9f;
        }
        else if (IsQuestion(lowerMsg)) {
            response = AnswerQuestion(context);
        }
        else if (IsCommand(lowerMsg)) {
            response = ProcessCommand(context);
        }
        else {
            // Default response or stay silent
            if (RandomChance(0.3f)) {  // 30% chance to respond
                response.text = GenerateContextualResponse(context);
                response.delay = RandomDelay(2000, 5000);
                response.confidence = 0.5f;
            }
        }
        
        return response;
    }
    
    ChatResponse AnswerQuestion(const ChatContext& context) {
        ChatResponse response;
        std::string lowerMsg = ToLower(context.message);
        
        // Location questions
        if (Contains(lowerMsg, {"where", "location"})) {
            response.text = fmt::format("I'm in {}", 
                GetAreaName(_bot->GetAreaId()));
        }
        // Level questions
        else if (Contains(lowerMsg, {"level", "lvl"})) {
            response.text = fmt::format("I'm level {}", _bot->getLevel());
        }
        // Class questions
        else if (Contains(lowerMsg, {"class", "spec"})) {
            response.text = fmt::format("I'm a {} {}", 
                GetSpecName(_bot), GetClassName(_bot));
        }
        // Ready check
        else if (Contains(lowerMsg, {"ready", "rdy", "r?"})) {
            response.text = "Ready!";
            RespondWithEmote(EMOTE_ONESHOT_NOD);
        }
        
        response.delay = RandomDelay(1500, 3500);
        response.confidence = 0.7f;
        
        return response;
    }
};
```

## Emote System

### Dynamic Emote Usage

```cpp
// src/modules/Playerbot/AI/Social/EmoteManager.h

class BotEmoteManager {
public:
    struct EmoteContext {
        enum Trigger {
            COMBAT_START,
            COMBAT_END,
            LEVEL_UP,
            DEATH,
            RESURRECTION,
            LOOT_EPIC,
            ACHIEVEMENT,
            GREETING,
            FAREWELL,
            THANKS,
            IDLE
        };
        
        Trigger trigger;
        Unit* target;
        float emotion;  // -1.0 (negative) to 1.0 (positive)
    };
    
    BotEmoteManager(Player* bot) : _bot(bot) {}
    
    // Emote selection
    void PlayEmote(const EmoteContext& context);
    uint32 SelectEmote(const EmoteContext& context);
    
    // Contextual emotes
    void EmoteCombatVictory();
    void EmoteDeath();
    void EmoteLevelUp();
    void EmoteGreeting(Player* target);
    
    // Random idle emotes
    void PlayIdleEmote();
    
private:
    Player* _bot;
    uint32 _lastEmoteTime = 0;
    
    uint32 SelectEmote(const EmoteContext& context) {
        std::vector<uint32> candidates;
        
        switch (context.trigger) {
            case EmoteContext::COMBAT_START:
                candidates = {EMOTE_ONESHOT_ROAR, EMOTE_ONESHOT_BATTLEROAR,
                            EMOTE_ONESHOT_ATTACKUNARMED};
                break;
                
            case EmoteContext::COMBAT_END:
                if (context.emotion > 0) {
                    candidates = {EMOTE_ONESHOT_CHEER, EMOTE_ONESHOT_VICTORY,
                                EMOTE_ONESHOT_FLEX};
                } else {
                    candidates = {EMOTE_ONESHOT_CRY, EMOTE_ONESHOT_KNEEL};
                }
                break;
                
            case EmoteContext::GREETING:
                candidates = {EMOTE_ONESHOT_WAVE, EMOTE_ONESHOT_SALUTE,
                            EMOTE_ONESHOT_BOW};
                break;
                
            case EmoteContext::IDLE:
                candidates = {EMOTE_ONESHOT_SIT, EMOTE_ONESHOT_EAT,
                            EMOTE_ONESHOT_DRINK, EMOTE_ONESHOT_READ};
                break;
        }
        
        if (!candidates.empty()) {
            return SelectRandom(candidates);
        }
        
        return 0;
    }
};
```

## Testing Framework

```cpp
// src/modules/Playerbot/Tests/SocialSystemTest.cpp

TEST_F(SocialSystemTest, TradeEvaluation) {
    auto bot = CreateTestBot();
    auto trader = CreateTestPlayer();
    BotTradeManager tradeMgr(bot);
    
    // Setup trade
    auto botItem = CreateTestItem(ITEM_UNCOMMON_SWORD, 1);
    auto traderItem = CreateTestItem(ITEM_RARE_ARMOR, 1);
    
    bot->GetTradeData()->SetItem(TRADE_SLOT_NONTRADED, botItem);
    trader->GetTradeData()->SetItem(TRADE_SLOT_NONTRADED, traderItem);
    
    // Evaluate trade
    auto eval = tradeMgr.EvaluateCurrentTrade();
    
    // Rare item worth more than uncommon
    EXPECT_TRUE(eval.isBeneficial);
    EXPECT_GT(eval.theirValue, eval.myValue);
}

TEST_F(SocialSystemTest, AuctionHousePricing) {
    auto bot = CreateTestBot();
    BotAuctionHouseManager ahMgr(bot);
    
    // Create test item
    auto item = CreateTestItem(ITEM_TRADE_CLOTH, 20);
    
    // Market data
    BotAuctionHouseManager::MarketData market;
    market.averagePrice = 1000;  // 10 silver each
    market.lowestPrice = 900;
    
    // Calculate sell price
    uint32 price = ahMgr.CalculateSellPrice(item);
    
    // Should undercut lowest price
    EXPECT_LT(price, market.lowestPrice);
    EXPECT_GT(price, market.lowestPrice * 0.9f);
}

TEST_F(SocialSystemTest, GuildBankContribution) {
    auto bot = CreateTestBot();
    auto guild = CreateTestGuild();
    bot->SetInGuild(guild->GetId());
    
    BotGuildManager guildMgr(bot);
    
    // Give bot excess items
    for (int i = 0; i < 5; ++i) {
        auto item = CreateTestItem(ITEM_TRADE_CLOTH, 20);
        bot->AddItem(item);
    }
    
    // Contribute to bank
    guildMgr.ContributeToGuildBank();
    
    // Check guild bank received items
    EXPECT_GT(guild->GetBankItemCount(), 0);
}

TEST_F(SocialSystemTest, ChatResponse) {
    auto bot = CreateTestBot();
    BotChatManager chatMgr(bot);
    
    // Test greeting response
    ChatContext context;
    context.sender = CreateTestPlayer();
    context.message = "Hello there!";
    
    auto response = chatMgr.GenerateResponse(context);
    
    EXPECT_FALSE(response.text.empty());
    EXPECT_GT(response.confidence, 0.5f);
    EXPECT_GT(response.delay, 0);
}
```

## Next Steps

1. **Implement Trade Manager** - Automated trading
2. **Add Auction House** - Market participation
3. **Create Guild Manager** - Guild activities
4. **Add Chat System** - Social interactions
5. **Testing** - Social scenarios

---

**Status**: Ready for Implementation
**Dependencies**: Core systems
**Estimated Time**: Sprint 3D Days 7-9