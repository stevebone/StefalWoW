/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "RestStrategy.h"
#include "../BotAI.h"
#include "Player.h"
#include "Item.h"
#include "Bag.h"
#include "ItemTemplate.h"
#include "SpellInfo.h"
#include "SpellAuraDefines.h"
#include "Log.h"
#include "SpellMgr.h"
#include "GameTime.h"

namespace Playerbot
{

RestStrategy::RestStrategy()
    : Strategy("rest")
{
    TC_LOG_DEBUG("module.playerbot.strategy", "RestStrategy: Initialized");
}

void RestStrategy::InitializeActions()
{
    TC_LOG_DEBUG("module.playerbot.strategy", "RestStrategy: No actions (direct rest control)");
}

void RestStrategy::InitializeTriggers()
{
    TC_LOG_DEBUG("module.playerbot.strategy", "RestStrategy: No triggers (using relevance system)");
}

void RestStrategy::InitializeValues()
{
    TC_LOG_DEBUG("module.playerbot.strategy", "RestStrategy: No values");
}

void RestStrategy::OnActivate(BotAI* ai)
{
    if (!ai || !ai->GetBot())
        return;

    Player* bot = ai->GetBot();
    if (!bot->IsInWorld())
        return;

    TC_LOG_INFO("module.playerbot.strategy", "Rest strategy activated for bot {}", bot->GetName());
    SetActive(true);
}

void RestStrategy::OnDeactivate(BotAI* ai)
{
    if (!ai || !ai->GetBot())
        return;

    Player* bot = ai->GetBot();
    if (!bot->IsInWorld())
        return;

    TC_LOG_INFO("module.playerbot.strategy", "Rest strategy deactivated for bot {}", bot->GetName());
    SetActive(false);

    _isEating = false;
    _isDrinking = false;
    _restStartTime = 0;
}

bool RestStrategy::IsActive(BotAI* ai) const
{
    if (!ai || !ai->GetBot())
        return false;

    Player* bot = ai->GetBot();

    // CRITICAL FIX: Safety check for worker thread access during bot destruction
    if (!bot->IsInWorld())
        return false;

    // NOT active during combat (can't eat/drink in combat)
    if (bot->IsInCombat())
        return false;

    // Active if explicitly activated and not in a group
    return _active && !bot->GetGroup();
}

float RestStrategy::GetRelevance(BotAI* ai) const
{
    if (!ai || !ai->GetBot())
        return 0.0f;

    Player* bot = ai->GetBot();

    // CRITICAL FIX: Safety check for worker thread access during bot destruction
    // Bot may be destroyed/logging out between null check and method calls
    // IsInWorld() returns false during destruction, preventing ACCESS_VIOLATION crash
    if (!bot->IsInWorld())
        return 0.0f;

    // Can't rest in combat
    if (bot->IsInCombat())
        return 0.0f;

    // Check if bot needs to rest
    bool needsFood = NeedsFood(ai);
    bool needsDrink = NeedsDrink(ai);

    if (!needsFood && !needsDrink)
        return 0.0f;

    // CRITICAL FIX: Only claim high priority if we CAN actually rest
    // If no consumables available, return 0 to let other strategies run
    // Bot will passively regenerate while questing/grinding
    bool canRestForFood = needsFood && (FindFood(ai) != nullptr);
    bool canRestForDrink = needsDrink && (FindDrink(ai) != nullptr);

    if (!canRestForFood && !canRestForDrink)
    {
        // Can't fulfill any rest needs - let other strategies run, bot regens naturally
        TC_LOG_DEBUG("module.playerbot.strategy", "RestStrategy: Bot {} needs rest but has no consumables, yielding priority",
                     bot->GetName());
        return 0.0f;
    }

    // High priority when we can actually rest (have consumables for at least one need)
    return 90.0f;
}

void RestStrategy::UpdateBehavior(BotAI* ai, uint32 diff)
{
    if (!ai || !ai->GetBot())
        return;
    Player* bot = ai->GetBot();

    // CRITICAL FIX: Safety check for worker thread access during bot destruction
    // Bot may be destroyed/logging out between null check and method calls
    // IsInWorld() returns false during destruction, preventing ACCESS_VIOLATION crash
    // This prevents RCX=0 null pointer dereference when bot's internal state is destroyed
    if (!bot->IsInWorld())
        return;

    // Can't rest in combat
    if (bot->IsInCombat())
    {
        _isEating = false;
        _isDrinking = false;
        _restStartTime = 0;
        return;
    }

    // Log current status
    float healthPct = bot->GetHealthPct();
    float manaPct = bot->GetMaxPower(POWER_MANA) > 0
        ? (bot->GetPower(POWER_MANA) * 100.0f / bot->GetMaxPower(POWER_MANA))
        : 100.0f;

    TC_LOG_TRACE("module.playerbot.strategy", "RestStrategy::UpdateBehavior: Bot {} health={:.1f}%, mana={:.1f}%, needsFood={}, needsDrink={}",
                 bot->GetName(), healthPct, manaPct, NeedsFood(ai), NeedsDrink(ai));

    uint32 currentTime = GameTime::GetGameTimeMS();

    // Check for rest timeout (prevent infinite resting)
    if (_restStartTime > 0 && (currentTime - _restStartTime) > _maxRestTime)
    {
        TC_LOG_WARN("module.playerbot.strategy", "RestStrategy: Bot {} rest timeout after 30s",
                    bot->GetName());
        _isEating = false;
        _isDrinking = false;
        _restStartTime = 0;
        return;
    }

    // Check if rest is complete (healthPct and manaPct already declared above)
    if (_isEating && healthPct >= _restCompleteHealth)
    {
        TC_LOG_DEBUG("module.playerbot.strategy", "RestStrategy: Bot {} finished eating ({:.1f}% health)",
                     bot->GetName(), healthPct);
        _isEating = false;
    }

    if (_isDrinking && manaPct >= _restCompleteMana)
    {
        TC_LOG_DEBUG("module.playerbot.strategy", "RestStrategy: Bot {} finished drinking ({:.1f}% mana)",
                     bot->GetName(), manaPct);
        _isDrinking = false;
    }
    // If both complete, stop resting
    if (!_isEating && !_isDrinking && _restStartTime > 0)
    {
        _restStartTime = 0;
        return;
    }

    // Start eating if needed
    // CRITICAL FIX: Check for existing food aura to prevent assertion failure in AuraApplication::_HandleEffect
    // Food auras can use EITHER SPELL_AURA_OBS_MOD_HEALTH OR SPELL_AURA_MOD_REGEN (see SpellInfo.cpp:2788)
    // The assertion "!(_effectMask & (1<<effIndex))" fails when trying to apply an aura effect that already exists
    // This can happen when _isEating flag gets out of sync with actual aura state
    bool hasFoodAura = bot->HasAuraType(SPELL_AURA_OBS_MOD_HEALTH) || bot->HasAuraType(SPELL_AURA_MOD_REGEN);
    if (NeedsFood(ai) && !_isEating && !bot->IsSitState() && !hasFoodAura)
    {
        Item* food = FindFood(ai);
        if (food)
        {
            TC_LOG_DEBUG("module.playerbot.strategy", "RestStrategy: Bot {} found food item {}, attempting to eat",
                         bot->GetName(), food->GetTemplate()->GetName(DEFAULT_LOCALE));
            if (EatFood(ai, food))
            {
                _isEating = true;
                if (_restStartTime == 0)
                    _restStartTime = currentTime;

                TC_LOG_DEBUG("module.playerbot.strategy", "RestStrategy: Bot {} started eating ({:.1f}% health)",
                             bot->GetName(), healthPct);
            }
            else
            {
                TC_LOG_WARN("module.playerbot.strategy", "RestStrategy: Bot {} failed to eat food {}",
                            bot->GetName(), food->GetTemplate()->GetName(DEFAULT_LOCALE));
            }
        }
        else
        {
            TC_LOG_ERROR("module.playerbot.strategy", "RestStrategy: Bot {} needs food but none found in inventory",
                         bot->GetName());
        }
    }
    // If already has food aura but _isEating is false, sync the flag
    else if ((bot->IsSitState() || hasFoodAura) && !_isEating && NeedsFood(ai))
    {
        _isEating = true;
        TC_LOG_DEBUG("module.playerbot.strategy", "RestStrategy: Bot {} already eating (has aura), syncing eat flag", bot->GetName());
    }

    // Start drinking if needed
    // CRITICAL FIX: Check for existing drink aura to prevent assertion failure in AuraApplication::_HandleEffect
    // Drink auras can use EITHER SPELL_AURA_OBS_MOD_POWER OR SPELL_AURA_MOD_POWER_REGEN (see SpellInfo.cpp:2793)
    // The assertion "!(_effectMask & (1<<effIndex))" fails when trying to apply an aura effect that already exists
    // This can happen when _isDrinking flag gets out of sync with actual aura state
    bool hasDrinkAura = bot->HasAuraType(SPELL_AURA_OBS_MOD_POWER) || bot->HasAuraType(SPELL_AURA_MOD_POWER_REGEN);
    if (NeedsDrink(ai) && !_isDrinking && !bot->IsSitState() && !hasDrinkAura)
    {
        Item* drink = FindDrink(ai);
        if (drink)
        {
            TC_LOG_DEBUG("module.playerbot.strategy", "RestStrategy: Bot {} found drink item {}, attempting to drink",
                         bot->GetName(), drink->GetTemplate()->GetName(DEFAULT_LOCALE));

            if (DrinkWater(ai, drink))
            {
                _isDrinking = true;
                if (_restStartTime == 0)
                    _restStartTime = currentTime;

                TC_LOG_DEBUG("module.playerbot.strategy", "RestStrategy: Bot {} started drinking ({:.1f}% mana)",
                             bot->GetName(), manaPct);
            }
            else
            {
                TC_LOG_WARN("module.playerbot.strategy", "RestStrategy: Bot {} failed to drink {}",
                            bot->GetName(), drink->GetTemplate()->GetName(DEFAULT_LOCALE));
            }
        }
        else
        {
            TC_LOG_DEBUG("module.playerbot.strategy", "RestStrategy: Bot {} needs drink but none found in inventory",
                         bot->GetName());
        }
    }
    // If already has drink aura but _isDrinking is false, sync the flag
    else if ((bot->IsSitState() || hasDrinkAura) && !_isDrinking && NeedsDrink(ai))
    {
        _isDrinking = true;
        TC_LOG_DEBUG("module.playerbot.strategy", "RestStrategy: Bot {} already drinking (has aura), syncing drink flag", bot->GetName());
    }

    // Use bandage if health critical and no food
    if (healthPct < 30.0f && !_isEating)
    {
        Item* bandage = FindBandage(ai);
        if (bandage && UseBandage(ai, bandage))
        {
            TC_LOG_DEBUG("module.playerbot.strategy", "RestStrategy: Bot {} used bandage ({:.1f}% health)",
                         bot->GetName(), healthPct);
        }
    }

    // Stop movement while resting
    if (IsResting(ai) && bot->isMoving())
    {
        bot->StopMoving();
    }
}

bool RestStrategy::NeedsFood(BotAI* ai) const
{
    if (!ai || !ai->GetBot())
        return false;

    Player* bot = ai->GetBot();

    // CRITICAL FIX: Safety check for worker thread access during bot destruction
    // Bot may be destroyed/logging out between null check and GetHealthPct() call
    // IsInWorld() returns false during destruction, preventing ACCESS_VIOLATION crash
    if (!bot->IsInWorld())
        return false;

    return bot->GetHealthPct() < _eatHealthThreshold;
}

bool RestStrategy::NeedsDrink(BotAI* ai) const
{
    if (!ai || !ai->GetBot())
        return false;

    Player* bot = ai->GetBot();

    // CRITICAL FIX: Safety check for worker thread access during bot destruction
    // Bot may be destroyed/logging out between null check and GetMaxPower() call
    // IsInWorld() returns false during destruction, preventing ACCESS_VIOLATION crash
    if (!bot->IsInWorld())
        return false;

    // Only mana users need to drink
    if (bot->GetMaxPower(POWER_MANA) == 0)
        return false;

    float manaPct = (bot->GetPower(POWER_MANA) * 100.0f) / bot->GetMaxPower(POWER_MANA);
    return manaPct < _drinkManaThreshold;
}

Item* RestStrategy::FindFood(BotAI* ai) const
{
    if (!ai || !ai->GetBot())
        return nullptr;

    Player* bot = ai->GetBot();

    // Scan inventory for food
    for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (!item)
            continue;

        ItemTemplate const* proto = item->GetTemplate();
        if (!proto)
            continue;

        // Check if item is food
    if (proto->GetClass() == ITEM_CLASS_CONSUMABLE && proto->GetSubClass() == ITEM_SUBCLASS_FOOD_DRINK)
        {
            // Verify it's actually food by checking spell specific type
    for (ItemEffectEntry const* effect : proto->Effects)
            {
                if (effect && effect->SpellID > 0)
                {
                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(effect->SpellID, DIFFICULTY_NONE);
                    if (spellInfo)
                    {
                        SpellSpecificType specificType = spellInfo->GetSpellSpecific();
                        if (specificType == SPELL_SPECIFIC_FOOD || specificType == SPELL_SPECIFIC_FOOD_AND_DRINK)
                            return item;
                    }
                }
            }
        }
    }

    // Check bags
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        if (Bag* pBag = bot->GetBagByPos(bag))
        {
            for (uint32 slot = 0; slot < pBag->GetBagSize(); ++slot)
            {
                Item* item = pBag->GetItemByPos(slot);
                if (!item)
                    continue;

                ItemTemplate const* proto = item->GetTemplate();
                if (!proto)
                    continue;

                if (proto->GetClass() == ITEM_CLASS_CONSUMABLE && proto->GetSubClass() == ITEM_SUBCLASS_FOOD_DRINK)
                {
                    // Verify it's actually food by checking spell specific type
    for (ItemEffectEntry const* effect : proto->Effects)
                    {
                        if (effect && effect->SpellID > 0)
                        {
                            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(effect->SpellID, DIFFICULTY_NONE);
                            if (spellInfo)
                            {
                                SpellSpecificType specificType = spellInfo->GetSpellSpecific();
                                if (specificType == SPELL_SPECIFIC_FOOD || specificType == SPELL_SPECIFIC_FOOD_AND_DRINK)
                                    return item;
                            }
                        }
                    }
                }
            }
        }
    }

    return nullptr;
}

Item* RestStrategy::FindDrink(BotAI* ai) const
{
    if (!ai || !ai->GetBot())
        return nullptr;

    Player* bot = ai->GetBot();

    // Scan inventory for drink
    for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (!item)
            continue;

        ItemTemplate const* proto = item->GetTemplate();
        if (!proto)
            continue;

        // Check if item is drink (restores mana)
    if (proto->GetClass() == ITEM_CLASS_CONSUMABLE)
        {
            // Check if spell is drink-specific
    for (ItemEffectEntry const* effect : proto->Effects)
            {
                if (effect && effect->SpellID > 0)
                {
                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(effect->SpellID, DIFFICULTY_NONE);
                    if (spellInfo)
                    {
                        SpellSpecificType specificType = spellInfo->GetSpellSpecific();
                        if (specificType == SPELL_SPECIFIC_DRINK || specificType == SPELL_SPECIFIC_FOOD_AND_DRINK)
                            return item;
                    }
                }
            }
        }
    }

    // Check bags
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        if (Bag* pBag = bot->GetBagByPos(bag))
        {
            for (uint32 slot = 0; slot < pBag->GetBagSize(); ++slot)
            {
                Item* item = pBag->GetItemByPos(slot);
                if (!item)
                    continue;

                ItemTemplate const* proto = item->GetTemplate();
                if (!proto)
                    continue;

                if (proto->GetClass() == ITEM_CLASS_CONSUMABLE)
                {
                    for (ItemEffectEntry const* effect : proto->Effects)
                    {
                        if (effect && effect->SpellID > 0)
                        {
                            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(effect->SpellID, DIFFICULTY_NONE);
                            if (spellInfo)
                            {
                                SpellSpecificType specificType = spellInfo->GetSpellSpecific();
                                if (specificType == SPELL_SPECIFIC_DRINK || specificType == SPELL_SPECIFIC_FOOD_AND_DRINK)
                                    return item;
                            }
                        }
                    }
                }
            }
        }
    }

    return nullptr;
}

Item* RestStrategy::FindBandage(BotAI* ai) const
{
    if (!ai || !ai->GetBot())
        return nullptr;

    Player* bot = ai->GetBot();

    // Bandages are usually ITEM_CLASS_CONSUMABLE, ITEM_SUBCLASS_BANDAGE
    // Scan inventory
    for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (!item)
            continue;

        ItemTemplate const* proto = item->GetTemplate();
        if (!proto)
            continue;

        // Check item name contains "Bandage"
        ::std::string itemName = proto->GetName(DEFAULT_LOCALE);
        if (itemName.find("Bandage") != ::std::string::npos)
            return item;
    }

    // Check bags
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        if (Bag* pBag = bot->GetBagByPos(bag))
        {
            for (uint32 slot = 0; slot < pBag->GetBagSize(); ++slot)
            {
                Item* item = pBag->GetItemByPos(slot);
                if (!item)
                    continue;

                ItemTemplate const* proto = item->GetTemplate();
                if (!proto)
                    continue;

                ::std::string itemName = proto->GetName(DEFAULT_LOCALE);
                if (itemName.find("Bandage") != ::std::string::npos)
                    return item;
            }
        }
    }

    return nullptr;
}

bool RestStrategy::EatFood(BotAI* ai, Item* food)
{
    if (!ai || !ai->GetBot() || !food)
        return false;

    Player* bot = ai->GetBot();

    // ========================================================================
    // CRITICAL FIX (2026-01-15): Prevent duplicate aura assertion crash
    // Check for existing food aura INSIDE this function (not just in caller)
    // to prevent race conditions where another thread/process applies food
    // between the caller's check and this function's execution.
    //
    // Crash: SpellAuras.cpp:174 ASSERTION FAILED: !(_effectMask & (1<<effIndex))
    // Cause: Food spell applied while already having food aura
    // ========================================================================
    if (bot->HasAuraType(SPELL_AURA_OBS_MOD_HEALTH) || bot->HasAuraType(SPELL_AURA_MOD_REGEN))
    {
        TC_LOG_DEBUG("module.playerbot.strategy", "RestStrategy::EatFood: Bot {} already has food aura, skipping",
                     bot->GetName());
        return false;
    }

    // Use the food item
    SpellCastTargets targets;
    targets.SetUnitTarget(bot);

    // WoW 12.0: CastItemUseSpell signature changed to std::array<int32, 3>
    std::array<int32, 3> misc = { 0, 0, 0 };
    bot->CastItemUseSpell(food, targets, ObjectGuid::Empty, misc);

    _foodConsumed++;
    return true;
}

bool RestStrategy::DrinkWater(BotAI* ai, Item* drink)
{
    if (!ai || !ai->GetBot() || !drink)
        return false;

    Player* bot = ai->GetBot();

    // ========================================================================
    // CRITICAL FIX (2026-01-15): Prevent duplicate aura assertion crash
    // Check for existing drink aura INSIDE this function to prevent race
    // conditions. Drink auras use SPELL_AURA_OBS_MOD_POWER (mana regen)
    // or SPELL_AURA_MOD_POWER_REGEN.
    //
    // Crash: SpellAuras.cpp:174 ASSERTION FAILED: !(_effectMask & (1<<effIndex))
    // ========================================================================
    if (bot->HasAuraType(SPELL_AURA_OBS_MOD_POWER) || bot->HasAuraType(SPELL_AURA_MOD_POWER_REGEN))
    {
        TC_LOG_DEBUG("module.playerbot.strategy", "RestStrategy::DrinkWater: Bot {} already has drink aura, skipping",
                     bot->GetName());
        return false;
    }

    // Use the drink item
    SpellCastTargets targets;
    targets.SetUnitTarget(bot);

    // WoW 12.0: CastItemUseSpell signature changed to std::array<int32, 3>
    std::array<int32, 3> misc = { 0, 0, 0 };
    bot->CastItemUseSpell(drink, targets, ObjectGuid::Empty, misc);

    _drinkConsumed++;
    return true;
}

bool RestStrategy::UseBandage(BotAI* ai, Item* bandage)
{
    if (!ai || !ai->GetBot() || !bandage)
        return false;

    Player* bot = ai->GetBot();

    // Use bandage on self
    SpellCastTargets targets;
    targets.SetUnitTarget(bot);

    // WoW 12.0: CastItemUseSpell signature changed to std::array<int32, 3>
    std::array<int32, 3> misc = { 0, 0, 0 };
    bot->CastItemUseSpell(bandage, targets, ObjectGuid::Empty, misc);

    _bandagesUsed++;
    return true;
}

bool RestStrategy::IsResting(BotAI* ai) const
{
    return _isEating || _isDrinking;
}

} // namespace Playerbot
