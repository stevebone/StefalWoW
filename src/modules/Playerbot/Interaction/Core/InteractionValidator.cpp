/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "InteractionValidator.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "Item.h"
#include "Bag.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "ObjectMgr.h"
#include "ReputationMgr.h"
#include "Trainer.h"
#include "Log.h"
#include "World.h"

namespace Playerbot
{
    InteractionValidator::InteractionValidator()
    {
        m_initialized = true;
    }

    InteractionValidator::~InteractionValidator()
    {
        // Cleanup handled automatically by smart pointers
    }

    bool InteractionValidator::CanInteract(::Player* bot, ::WorldObject* target, InteractionType type) const
    {
        if (!bot || !target)
            return false;

        // Check cache first
        auto cacheKey = bot->GetGUID();
        auto now = ::std::chrono::steady_clock::now();

        {
            ::std::lock_guard lock(m_mutex);
            auto lastCheck = m_lastValidation.find(cacheKey);
            if (lastCheck != m_lastValidation.end())
            {
                auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(now - lastCheck->second);
                if (elapsed.count() < CACHE_DURATION_MS)
                {
                    auto cached = m_validationCache.find(cacheKey);
                    if (cached != m_validationCache.end())
                        return cached->second;
                }
            }
        }

        bool result = true;

        // Basic checks
    if (!CheckRange(bot, target))
        {
            result = false;
        }
        else if (!CheckAliveState(bot, type != InteractionType::SpiritHealer))
        {
            result = false;
        }
        else if (!CheckCombatState(bot, type == InteractionType::SpiritHealer))
        {
            result = false;
        }
        else if (IsOnCooldown(bot, type))
        {
            result = false;
        }

        // Type-specific checks
    if (result)
        {
            switch (type)
            {
                case InteractionType::Vendor:
                    if (::Creature* vendor = target->ToCreature())
                        result = CheckVendorRequirements(bot, vendor);
                    break;

                case InteractionType::Trainer:
                    if (::Creature* trainer = target->ToCreature())
                        result = CheckTrainerRequirements(bot, trainer);
                    break;

                case InteractionType::FlightMaster:
                    result = bot->GetLevel() >= 10;  // Min level for flight
                    break;

                case InteractionType::Bank:
                case InteractionType::GuildBank:
                    result = CanAccessBank(bot);
                    break;

                case InteractionType::Mailbox:
                    result = CanUseMail(bot);
                    break;

                default:
                    break;
            }
        }

        // Cache result
        {
            ::std::lock_guard lock(m_mutex);
            m_lastValidation[cacheKey] = now;
            m_validationCache[cacheKey] = result;
        }

        RecordValidation(type, result);
        return result;
    }

    bool InteractionValidator::CheckRange(::Player* bot, ::WorldObject* target, float maxRange) const
    {
        if (!bot || !target)
            return false;

        if (maxRange <= 0.0f)
            maxRange = 5.0f;  // Default interaction range

        return bot->GetDistance(target) <= maxRange;
    }

    bool InteractionValidator::CheckFaction(::Player* bot, ::Creature* creature) const
    {
        if (!bot || !creature)
            return false;

        // Check if creature is hostile
    if (creature->IsHostileTo(bot))
            return false;

        // Check if creature is friendly enough for interaction
        return creature->IsFriendlyTo(bot) || creature->IsNeutralToAll();
    }

    bool InteractionValidator::CheckLevel(::Player* bot, uint32 minLevel, uint32 maxLevel) const
    {
        if (!bot)
            return false;

        uint32 level = bot->GetLevel();
        if (minLevel > 0 && level < minLevel)
            return false;

        if (maxLevel > 0 && level > maxLevel)
            return false;

        return true;
    }

    bool InteractionValidator::CheckReputation(::Player* bot, uint32 factionId, ReputationRank minRank) const
    {
        if (!bot || !factionId)
            return true;  // No faction requirement

        return bot->GetReputationRank(factionId) >= minRank;
    }

    bool InteractionValidator::CheckMoney(::Player* bot, uint32 amount) const
    {
        if (!bot)
            return false;

        return bot->GetMoney() >= amount;
    }

    bool InteractionValidator::CheckInventorySpace(::Player* bot, uint32 slotsNeeded) const
    {
        if (!bot)
            return false;

        uint32 freeSlots = 0;

        // Check all bags
    for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        {
            if (Bag* bag = bot->GetBagByPos(i))
            {
                for (uint32 j = 0; j < bag->GetBagSize(); ++j)
                {
                    if (!bag->GetItemByPos(j))
                        ++freeSlots;
                }
            }
        }

        // Check backpack
    for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        {
            if (!bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                ++freeSlots;
        }

        return freeSlots >= slotsNeeded;
    }

    bool InteractionValidator::CheckCombatState(::Player* bot, bool allowInCombat) const
    {
        if (!bot)
            return false;

        if (bot->IsInCombat() && !allowInCombat)
            return false;

        return true;
    }

    bool InteractionValidator::CheckAliveState(::Player* bot, bool requireAlive) const
    {
        if (!bot)
            return false;

        bool isAlive = bot->IsAlive();
        return requireAlive ? isAlive : !isAlive;
    }

    bool InteractionValidator::CheckQuestStatus(::Player* bot, uint32 questId) const
    {
        if (!bot || !questId)
            return true;  // No quest requirement

        return bot->GetQuestStatus(questId) == QUEST_STATUS_COMPLETE ||
               bot->GetQuestRewardStatus(questId);
    }

    bool InteractionValidator::CheckItemRequirement(::Player* bot, uint32 itemId, uint32 count) const
    {
        if (!bot || !itemId)
            return true;  // No item requirement

        return bot->HasItemCount(itemId, count);
    }

    bool InteractionValidator::CheckSpellKnown(::Player* bot, uint32 spellId) const
    {
        if (!bot || !spellId)
            return true;  // No spell requirement

        return bot->HasSpell(spellId);
    }

    bool InteractionValidator::CheckSkillLevel(::Player* bot, uint32 skillId, uint32 minValue) const
    {
        if (!bot || !skillId)
            return true;  // No skill requirement

        return bot->GetSkillValue(skillId) >= minValue;
    }

    bool InteractionValidator::ValidateVendor(::Player* bot, ::Creature* vendor) const
    {
        if (!bot || !vendor)
            return false;

        if (!vendor->IsVendor())
            return false;

        // Check faction
    if (!CheckFaction(bot, vendor))
            return false;

        // Check if vendor has items
        VendorItemData const* items = vendor->GetVendorItems();
        if (!items || items->Empty())
            return false;

        // In strict mode, check if vendor has useful items
    if (m_strictMode)
        {
            bool hasUsefulItem = false;
            for (auto const& item : items->m_items)
            {
                if (ShouldBuyItem(bot, item.item))
                {
                    hasUsefulItem = true;
                    break;
                }
            }
            if (!hasUsefulItem)
                return false;
        }

        return true;
    }

    bool InteractionValidator::ShouldBuyItem(::Player* bot, uint32 itemId) const
    {
        if (!bot || !itemId)
            return false;

        // Check cache
        {
            ::std::lock_guard lock(m_mutex);
            auto it = m_usefulItemCache.find(itemId);
            if (it != m_usefulItemCache.end())
                return it->second;
        }

        bool isUseful = false;

        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
        if (!itemTemplate)
            return false;

        // Class-specific consumables
        uint8 botClass = bot->GetClass();
        switch (itemTemplate->GetClass())
        {
            case ITEM_CLASS_CONSUMABLE:
                // Food/water always useful - ITEM_SUBCLASS_FOOD_DRINK = 5
    if (itemTemplate->GetSubClass() == 5 /* ITEM_SUBCLASS_FOOD_DRINK */)
                {
                    isUseful = true;
                }
                // Potions useful for all - ITEM_SUBCLASS_POTION = 1, ELIXIR = 2, FLASK = 3
                else if (itemTemplate->GetSubClass() == 1 /* ITEM_SUBCLASS_POTION */ ||
                         itemTemplate->GetSubClass() == 2 /* ITEM_SUBCLASS_ELIXIR */ ||
                         itemTemplate->GetSubClass() == 3 /* ITEM_SUBCLASS_FLASK */)
                {
                    isUseful = true;
                }
                // Bandages useful for non-healers - ITEM_SUBCLASS_BANDAGE = 7
                else if (itemTemplate->GetSubClass() == 7 /* ITEM_SUBCLASS_BANDAGE */)
                {
                    isUseful = (botClass != CLASS_PRIEST && botClass != CLASS_PALADIN);
                }
                break;

            case ITEM_CLASS_PROJECTILE:
                // Arrows/bullets for hunters
                isUseful = (botClass == CLASS_HUNTER);
                break;

            case ITEM_CLASS_REAGENT:
                // Class reagents
                isUseful = true;  // Let class-specific logic determine
                break;

            case ITEM_CLASS_RECIPE:
                // Recipes if bot has profession
    if (itemTemplate->GetSubClass() >= ITEM_SUBCLASS_LEATHERWORKING_PATTERN &&
                    itemTemplate->GetSubClass() <= ITEM_SUBCLASS_ENCHANTING_FORMULA)
                {
                    // Check if bot has corresponding profession
                    isUseful = true;  // Simplified - should check profession
                }
                break;

            default:
                // Check if it's an upgrade
    if (itemTemplate->GetInventoryType() != INVTYPE_NON_EQUIP)
                {
                    // Simple item level check for equipment
                    ::Item* currentItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, itemTemplate->GetInventoryType());
                    if (!currentItem || currentItem->GetTemplate()->GetBaseItemLevel() < itemTemplate->GetBaseItemLevel())
                    {
                        isUseful = true;
                    }
                }
                break;
        }

        // Cache result
        {
            ::std::lock_guard lock(m_mutex);
            m_usefulItemCache[itemId] = isUseful;
        }

        return isUseful;
    }

    bool InteractionValidator::ShouldSellItem(::Player* bot, ::Item* item) const
    {
        if (!bot || !item)
            return false;

        ItemTemplate const* itemTemplate = item->GetTemplate();
        if (!itemTemplate)
            return false;

        // Never sell bound items
    if (item->IsSoulBound())
            return false;

        // Always sell gray quality items (junk)
    if (itemTemplate->GetQuality() == ITEM_QUALITY_POOR)
            return true;

        // Never sell quest items
    if (itemTemplate->GetStartQuest())
            return false;

        // Check if equipped
    if (bot->GetItemByGuid(item->GetGUID()) == item && item->IsEquipped())
            return false;

        // Never sell items with high vendor price (likely important)
    if (itemTemplate->GetSellPrice() > 10000)  // 1 gold
            return false;

        // Sell excess consumables
    if (itemTemplate->GetClass() == ITEM_CLASS_CONSUMABLE)
        {
            uint32 totalCount = bot->GetItemCount(itemTemplate->GetId());
            if (totalCount > 40)  // Keep max 40 of any consumable
                return true;
        }

        return false;
    }

    bool InteractionValidator::NeedsRepair(::Player* bot, float threshold) const
    {
        if (!bot)
            return false;

        uint32 totalDurability = 0;
        uint32 totalMaxDurability = 0;

        for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
        {
            ::Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
            if (!item)
                continue;

            // Access durability through update fields
            uint32 maxDurability = *item->m_itemData->MaxDurability;
            if (maxDurability > 0)
            {
                totalMaxDurability += maxDurability;
                totalDurability += *item->m_itemData->Durability;
            }
        }

        if (totalMaxDurability == 0)
            return false;

        float durabilityPercent = (float)totalDurability / totalMaxDurability * 100.0f;
        return durabilityPercent < threshold;
    }

    bool InteractionValidator::ValidateTrainer(::Player* bot, ::Creature* trainer) const
    {
        if (!bot || !trainer)
            return false;

        // Get trainer ID from creature
        uint32 trainerId = trainer->GetTrainerId();
        if (!trainerId)
            return false;

        // Check faction
    if (!CheckFaction(bot, trainer))
            return false;

        // Get trainer data from ObjectMgr
        ::Trainer::Trainer const* trainerData = sObjectMgr->GetTrainer(trainerId);
        if (!trainerData)
            return false;

        // In TrinityCore 12.0, trainer spell validation is handled internally
        // We just check if trainer exists and is accessible
        // The actual spell learning validation happens in Trainer::SendSpells
        return true;
    }

    bool InteractionValidator::ShouldLearnSpell(::Player* bot, ::SpellInfo const* spellInfo) const
    {
        if (!bot || !spellInfo)
            return false;

        // Learn mount/riding skills - check spell name or attributes
        // Note: SPELL_EFFECT_SUMMON_MOUNT may not exist in TrinityCore 12.0
        // Alternative: Check if spell creates a mount aura or has mount-related attributes
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (spellInfo->GetEffect(static_cast<SpellEffIndex>(i)).ApplyAuraName == SPELL_AURA_MOUNTED)
                return true;
        }

        // Learn profession spells if bot has profession
        // This would need more complex logic to check professions

        // For class abilities, rely on trainer validation system
        // TrinityCore 12.0 handles spell family validation internally
        return true;
    }

    bool InteractionValidator::CanLearnSpell(::Player* bot, uint32 spellId) const
    {
        if (!bot || !spellId)
            return false;

        // Check if already known
    if (bot->HasSpell(spellId))
            return false;

        ::SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo)
            return false;

        // Check level requirement
    if (spellInfo->BaseLevel > bot->GetLevel())
            return false;

        // Check if bot has required spell (prerequisite chain)
        // Note: PrevRanks is not a direct field in SpellInfo - prerequisite checking
        // would require more complex logic through spell requirements

        // Check class requirement - SpellFamilyName validation
        // In TrinityCore 12.0, spell family validation is handled internally by spell learning system
        // We rely on the trainer system to validate spell availability
        return true;
    }

    bool InteractionValidator::CanUseFlight(::Player* bot, uint32 nodeId) const
    {
        if (!bot)
            return false;

        // Check level requirement
    if (bot->GetLevel() < 10)
            return false;

        // Check if node is discovered
    if (!IsFlightNodeDiscovered(bot, nodeId))
            return false;

        // Check if bot has enough money for flight
        // This would need taxi cost calculation

        return true;
    }

    bool InteractionValidator::IsFlightNodeDiscovered(::Player* bot, uint32 nodeId) const
    {
        if (!bot || !nodeId)
            return false;

        // Check taxi mask for discovered nodes
        return bot->m_taxi.IsTaximaskNodeKnown(nodeId);
    }

    bool InteractionValidator::CanAccessBank(::Player* bot) const
    {
        if (!bot)
            return false;

        // Banks are generally accessible to all players
        // Could add level or reputation checks here if needed

        return true;
    }

    bool InteractionValidator::ShouldBankItem(::Player* bot, ::Item* item) const
    {
        if (!bot || !item)
            return false;

        ItemTemplate const* itemTemplate = item->GetTemplate();
        if (!itemTemplate)
            return false;

        // Bank valuable items
    if (itemTemplate->GetQuality() >= ITEM_QUALITY_RARE)
            return true;

        // Bank profession materials
    if (itemTemplate->GetClass() == ITEM_CLASS_TRADE_GOODS)
            return true;

        // Bank items for other specs/situations
        // This would need more complex logic

        return false;
    }

    bool InteractionValidator::CanUseMail(::Player* bot) const
    {
        if (!bot)
            return false;

        // Mail is generally accessible to all players
        // Could add level checks here if needed

        return true;
    }

    bool InteractionValidator::ShouldTakeMail(::Player* bot, uint32 mailId) const
    {
        if (!bot || !mailId)
            return false;

        // Always take mail with items or money
        // Would need to check mail content

        return true;
    }

    bool InteractionValidator::ValidateRequirements(::Player* bot, const InteractionRequirement& requirements) const
    {
        return requirements.CheckRequirements(bot);
    }

    ::std::vector<::std::string> InteractionValidator::GetMissingRequirements(::Player* bot, ::WorldObject* target,
                                                                          InteractionType type) const
    {
        ::std::vector<::std::string> missing;

        if (!bot || !target)
        {
            missing.push_back("Invalid bot or target");
            return missing;
        }

        if (!CheckRange(bot, target))
            missing.push_back("Too far away");

        if (!CheckAliveState(bot, type != InteractionType::SpiritHealer))
            missing.push_back(type == InteractionType::SpiritHealer ? "Must be dead" : "Must be alive");

        if (!CheckCombatState(bot, type == InteractionType::SpiritHealer))
            missing.push_back("Cannot interact in combat");

        if (IsOnCooldown(bot, type))
            missing.push_back("Interaction on cooldown");

        // Add type-specific requirements
    switch (type)
        {
            case InteractionType::Vendor:
                if (!CheckInventorySpace(bot, 1))
                    missing.push_back("Inventory full");
                break;

            case InteractionType::Trainer:
                if (!CheckMoney(bot, 100))  // Min training cost
                    missing.push_back("Not enough gold");
                break;

            case InteractionType::FlightMaster:
                if (bot->GetLevel() < 10)
                    missing.push_back("Must be level 10+");
                break;

            default:
                break;
        }

        return missing;
    }

    int32 InteractionValidator::GetInteractionPriority(::Player* bot, InteractionType type) const
    {
        if (!bot)
            return 0;

        int32 priority = 50;  // Base priority
    switch (type)
        {
            case InteractionType::SpiritHealer:
                if (!bot->IsAlive())
                    priority = 100;  // Highest priority when dead
                break;

            case InteractionType::Vendor:
                if (NeedsRepair(bot, 20.0f))
                    priority = 90;  // Critical repairs
                else if (NeedsRepair(bot, 50.0f))
                    priority = 70;  // Normal repairs
    if (!CheckInventorySpace(bot, 5))
                    priority += 20;  // Need to sell
                break;

            case InteractionType::Trainer:
                // Higher priority at level milestones
    if (bot->GetLevel() % 10 == 0)
                    priority = 80;
                break;

            case InteractionType::Innkeeper:
                // Higher priority if no hearthstone bound
    if (!bot->HasSpell(8690))  // Hearthstone spell
                    priority = 75;
                break;

            case InteractionType::Mailbox:
                // Would check pending mail count
                priority = 40;
                break;

            case InteractionType::Bank:
                if (!CheckInventorySpace(bot, 10))
                    priority = 60;  // Need bank space
                break;

            default:
                break;
        }

        return priority;
    }

    InteractionValidator::ValidationMetrics InteractionValidator::GetMetrics() const
    {
        ::std::lock_guard lock(m_mutex);
        return m_metrics;
    }

    void InteractionValidator::ResetMetrics()
    {
        ::std::lock_guard lock(m_mutex);
        m_metrics = ValidationMetrics();
    }

    bool InteractionValidator::CheckVendorRequirements(::Player* bot, ::Creature* vendor) const
    {
        if (!ValidateVendor(bot, vendor))
            return false;

        // Check if bot has money to buy anything
    if (bot->GetMoney() < 1)
            return false;

        return true;
    }

    bool InteractionValidator::CheckTrainerRequirements(::Player* bot, ::Creature* trainer) const
    {
        if (!ValidateTrainer(bot, trainer))
            return false;

        // Check if bot has money to train
    if (bot->GetMoney() < 100)  // Min training cost
            return false;

        return true;
    }

    bool InteractionValidator::IsOnCooldown(::Player* bot, InteractionType type) const
    {
        if (!bot)
            return false;

        ::std::lock_guard lock(m_mutex);

        auto botIt = m_cooldowns.find(bot->GetGUID());
        if (botIt == m_cooldowns.end())
            return false;

        const auto& botCooldowns = botIt->second;
        auto typeIt = botCooldowns.find(type);
        if (typeIt == botCooldowns.end())
            return false;

        auto cooldownIt = m_cooldownDurations.find(type);
        if (cooldownIt == m_cooldownDurations.end())
            return false;

        auto now = ::std::chrono::steady_clock::now();
        auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(now - typeIt->second);

        return elapsed.count() < static_cast<int64>(cooldownIt->second);
    }

    void InteractionValidator::RecordValidation(InteractionType type, bool passed) const
    {
        ::std::lock_guard lock(m_mutex);

        ++m_metrics.totalValidations;
        if (passed)
            ++m_metrics.passedValidations;
        else
        {
            ++m_metrics.failedValidations;
            ++m_metrics.failuresByType[type];
        }

        // Update cooldown if passed
    if (passed)
        {
            auto now = ::std::chrono::steady_clock::now();
            // Note: We need the bot GUID here, but it's not passed to this function
            // In a real implementation, this would be handled differently
        }
    }
}