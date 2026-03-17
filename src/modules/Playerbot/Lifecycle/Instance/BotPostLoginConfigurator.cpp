/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotPostLoginConfigurator.h"
#include "BotTemplateRepository.h"
#include "Equipment/BotGearFactory.h"
#include "LFG/LFGBotManager.h"
#include "PvP/BGBotManager.h"
#include "Session/BotWorldSessionMgr.h"
#include "BattlegroundMgr.h"
#include "Player.h"
#include "Item.h"
#include "Log.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "ObjectMgr.h"
#include "DB2Stores.h"
#include "World.h"
#include <algorithm>

namespace Playerbot
{

// ============================================================================
// SINGLETON
// ============================================================================

BotPostLoginConfigurator* BotPostLoginConfigurator::Instance()
{
    static BotPostLoginConfigurator instance;
    return &instance;
}

// ============================================================================
// LIFECYCLE
// ============================================================================

bool BotPostLoginConfigurator::Initialize()
{
    if (_initialized.load())
        return true;

    TC_LOG_INFO("module.playerbot", "BotPostLoginConfigurator: Initializing...");

    _pendingConfigs.clear();
    ResetStats();

    _initialized.store(true);
    TC_LOG_INFO("module.playerbot", "BotPostLoginConfigurator: Initialization complete");
    return true;
}

void BotPostLoginConfigurator::Shutdown()
{
    if (!_initialized.load())
        return;

    TC_LOG_INFO("module.playerbot", "BotPostLoginConfigurator: Shutting down...");

    {
        std::lock_guard<std::mutex> lock(_configMutex);
        _pendingConfigs.clear();
        _recentlyConfiguredBots.clear();
    }

    _initialized.store(false);
    TC_LOG_INFO("module.playerbot", "BotPostLoginConfigurator: Shutdown complete");
}

// ============================================================================
// CONFIGURATION REGISTRATION
// ============================================================================

void BotPostLoginConfigurator::RegisterPendingConfig(BotPendingConfiguration config)
{
    std::lock_guard<std::mutex> lock(_configMutex);

    TC_LOG_INFO("module.playerbot.configurator",
        "Registering pending config for bot {} - Template: {}, Level: {}, GS: {}",
        config.botGuid.ToString(), config.templateId, config.targetLevel, config.targetGearScore);

    _pendingConfigs[config.botGuid] = std::move(config);
    _stats.pendingConfigs.fetch_add(1);
}

bool BotPostLoginConfigurator::HasPendingConfiguration(ObjectGuid botGuid) const
{
    std::lock_guard<std::mutex> lock(_configMutex);
    bool found = _pendingConfigs.find(botGuid) != _pendingConfigs.end();

    TC_LOG_INFO("module.playerbot.configurator",
        "HasPendingConfiguration: GUID={}, Found={}, TotalPendingConfigs={}",
        botGuid.ToString(), found ? "YES" : "NO", _pendingConfigs.size());

    // DEBUG: Log first few registered GUIDs for comparison when not found
    if (!found && !_pendingConfigs.empty())
    {
        std::string registeredGuids;
        uint32 count = 0;
        for (auto const& [guid, config] : _pendingConfigs)
        {
            if (count++ >= 5) break;  // Limit to 5 GUIDs
            if (!registeredGuids.empty()) registeredGuids += ", ";
            registeredGuids += guid.ToString();
        }
        TC_LOG_WARN("module.playerbot.configurator",
            "HasPendingConfiguration: GUID {} NOT FOUND! RegisteredGUIDs=[{}]",
            botGuid.ToString(), registeredGuids);
    }

    return found;
}

BotPendingConfiguration const* BotPostLoginConfigurator::GetPendingConfiguration(ObjectGuid botGuid) const
{
    std::lock_guard<std::mutex> lock(_configMutex);
    auto it = _pendingConfigs.find(botGuid);
    return it != _pendingConfigs.end() ? &it->second : nullptr;
}

void BotPostLoginConfigurator::RemovePendingConfiguration(ObjectGuid botGuid)
{
    std::lock_guard<std::mutex> lock(_configMutex);
    auto it = _pendingConfigs.find(botGuid);
    if (it != _pendingConfigs.end())
    {
        _pendingConfigs.erase(it);
        _stats.pendingConfigs.fetch_sub(1);
    }
}

bool BotPostLoginConfigurator::WasRecentlyConfigured(ObjectGuid botGuid) const
{
    std::lock_guard<std::mutex> lock(_configMutex);
    bool found = _recentlyConfiguredBots.find(botGuid) != _recentlyConfiguredBots.end();

    if (found)
    {
        TC_LOG_INFO("module.playerbot.configurator",
            "WasRecentlyConfigured: GUID={} -> YES (in recently configured set)",
            botGuid.ToString());
    }

    return found;
}

void BotPostLoginConfigurator::ClearRecentlyConfigured(ObjectGuid botGuid)
{
    std::lock_guard<std::mutex> lock(_configMutex);
    auto erased = _recentlyConfiguredBots.erase(botGuid);

    if (erased > 0)
    {
        TC_LOG_INFO("module.playerbot.configurator",
            "ClearRecentlyConfigured: Removed GUID={} from recently configured set (remaining: {})",
            botGuid.ToString(), _recentlyConfiguredBots.size());
    }
}

// ============================================================================
// CONFIGURATION APPLICATION
// ============================================================================

bool BotPostLoginConfigurator::ApplyPendingConfiguration(Player* player)
{
    if (!player)
    {
        TC_LOG_ERROR("module.playerbot.configurator", "ApplyPendingConfiguration: null player");
        return false;
    }

    ObjectGuid playerGuid = player->GetGUID();

    TC_LOG_INFO("module.playerbot.configurator",
        "ApplyPendingConfiguration: Checking for bot {} (GUID={}, Level={})",
        player->GetName(), playerGuid.ToString(), player->GetLevel());

    // Get pending configuration
    BotPendingConfiguration config;
    {
        std::lock_guard<std::mutex> lock(_configMutex);
        auto it = _pendingConfigs.find(playerGuid);
        if (it == _pendingConfigs.end())
        {
            // No pending configuration - could be normal for regular bots, but warn for JIT bots
            TC_LOG_WARN("module.playerbot.configurator",
                "ApplyPendingConfiguration: NO CONFIG found for bot {} (GUID={}) - TotalPendingConfigs={}",
                player->GetName(), playerGuid.ToString(), _pendingConfigs.size());
            return false;
        }
        config = it->second;
    }

    auto startTime = std::chrono::steady_clock::now();

    TC_LOG_INFO("module.playerbot.configurator",
        "Applying post-login configuration for bot {} - Template: {}, Level: {}, GS: {}",
        player->GetName(), config.templateId, config.targetLevel, config.targetGearScore);

    _stats.totalConfigured.fetch_add(1);

    bool success = true;

    // Get template if not cached
    // NOTE: Template is OPTIONAL - warm pool bots may not have a template
    // In that case, we still apply level, spec, and use BotGearFactory for equipment
    BotTemplate const* tmpl = config.templatePtr;
    if (!tmpl && config.templateId > 0)
    {
        tmpl = sBotTemplateRepository->GetTemplateById(config.templateId);
        if (!tmpl)
        {
            TC_LOG_WARN("module.playerbot.configurator",
                "Template {} not found for bot {} - will use BotGearFactory fallback",
                config.templateId, player->GetName());
        }
    }

    // If no template is available, log info and continue with fallback behavior
    if (!tmpl)
    {
        TC_LOG_INFO("module.playerbot.configurator",
            "No template for bot {} (templateId={}) - using BotGearFactory for equipment",
            player->GetName(), config.templateId);
    }

    // Step 1: Apply Level
    if (config.targetLevel > 1 && config.targetLevel != player->GetLevel())
    {
        TC_LOG_INFO("module.playerbot.configurator",
            "Applying level {} to bot {} (current: {})",
            config.targetLevel, player->GetName(), player->GetLevel());

        if (!ApplyLevel(player, config.targetLevel))
        {
            TC_LOG_WARN("module.playerbot.configurator",
                "Failed to apply level {} to bot {}",
                config.targetLevel, player->GetName());
            success = false;
        }
    }

    // Step 2: Apply Specialization (must be before talents)
    // Use config.specId if set, otherwise fallback to template spec (if available)
    uint32 specId = config.specId > 0 ? config.specId : (tmpl ? tmpl->specId : 0);
    if (specId > 0)
    {
        TC_LOG_INFO("module.playerbot.configurator",
            "Applying specialization {} to bot {}",
            specId, player->GetName());

        if (!ApplySpecialization(player, specId))
        {
            TC_LOG_WARN("module.playerbot.configurator",
                "Failed to apply specialization {} to bot {}",
                specId, player->GetName());
            // Continue anyway - some operations may still work
        }
    }

    // Step 3: Learn class spells for level
    TC_LOG_INFO("module.playerbot.configurator",
        "Applying class spells to bot {}",
        player->GetName());

    ApplyClassSpells(player);

    // Step 4: Apply Talents (only if template exists with talents)
    if (tmpl && !tmpl->talents.talentIds.empty())
    {
        TC_LOG_INFO("module.playerbot.configurator",
            "Applying {} talents to bot {}",
            tmpl->talents.talentIds.size(), player->GetName());

        if (!ApplyTalents(player, tmpl))
        {
            TC_LOG_WARN("module.playerbot.configurator",
                "Some talents failed to apply for bot {}",
                player->GetName());
            // Continue anyway
        }
    }

    // Step 5: Apply Gear
    // CRITICAL FIX: Always apply gear for instance bots
    // BotGearFactory will handle gear generation when template is null/empty/has no gear
    // Previous condition was broken because:
    //   - targetGearScore=0 (templates set this to 0)
    //   - hasTemplateGear=false (no gear sets in database)
    //   - tmpl != nullptr (templates exist)
    // Result: shouldApplyGear was always false, bots had no gear!
    bool hasTemplateGear = tmpl && !tmpl->gearSets.empty();
    // ALWAYS apply gear - if no template gear exists, BotGearFactory will generate appropriate gear
    bool shouldApplyGear = true;  // Always equip gear for instance bots

    if (shouldApplyGear)
    {
        TC_LOG_INFO("module.playerbot.configurator",
            "Applying gear to bot {} (targetGS={}, hasTemplate={}, hasTemplateGear={})",
            player->GetName(), config.targetGearScore, tmpl != nullptr, hasTemplateGear);

        if (!ApplyGear(player, tmpl, config.targetGearScore))
        {
            TC_LOG_WARN("module.playerbot.configurator",
                "Some gear failed to apply for bot {}",
                player->GetName());
            // Continue anyway
        }
    }

    // Step 6: Apply Action Bars (only if template exists with action bars)
    if (tmpl && !tmpl->actionBars.buttons.empty())
    {
        TC_LOG_INFO("module.playerbot.configurator",
            "Applying {} action buttons to bot {}",
            tmpl->actionBars.buttons.size(), player->GetName());

        if (!ApplyActionBars(player, tmpl))
        {
            TC_LOG_WARN("module.playerbot.configurator",
                "Some action bars failed to apply for bot {}",
                player->GetName());
            // Continue anyway
        }
    }

    // Update all stats after configuration
    player->UpdateAllStats();

    // Set full health and power
    player->SetFullHealth();
    for (uint8 i = 0; i < MAX_POWERS; ++i)
    {
        Powers power = Powers(i);
        if (player->GetMaxPower(power) > 0)
            player->SetFullPower(power);
    }

    // Step 7: Queue for content if this was a JIT-created bot
    if (config.dungeonIdToQueue > 0)
    {
        TC_LOG_INFO("module.playerbot.configurator",
            "Queueing JIT bot {} for dungeon {} after configuration",
            player->GetName(), config.dungeonIdToQueue);

        if (sLFGBotManager->QueueJITBot(player, config.dungeonIdToQueue))
        {
            TC_LOG_INFO("module.playerbot.configurator",
                "Successfully queued bot {} for dungeon {}",
                player->GetName(), config.dungeonIdToQueue);
        }
        else
        {
            TC_LOG_WARN("module.playerbot.configurator",
                "Failed to queue bot {} for dungeon {}",
                player->GetName(), config.dungeonIdToQueue);
        }
    }

    // Step 8: Queue for battleground if this was a JIT-created bot
    if (config.battlegroundIdToQueue > 0)
    {
        BattlegroundTypeId bgTypeId = static_cast<BattlegroundTypeId>(config.battlegroundIdToQueue);

        // CRITICAL FIX: Resurrect dead bots BEFORE attempting BG queue.
        // Warm pool bots may have died in a previous session and are still dead at login.
        // IsBotAvailable() rejects dead bots, causing 10/19 warm pool bots to silently fail
        // to queue, leaving the BG with insufficient players to start.
        if (player->isDead())
        {
            player->ResurrectPlayer(1.0f);
            player->SpawnCorpseBones();
            TC_LOG_INFO("module.playerbot.configurator",
                "Resurrected dead bot {} before BG queue attempt", player->GetName());
        }

        TC_LOG_INFO("module.playerbot.configurator",
            "Queueing JIT bot {} for battleground {} after configuration",
            player->GetName(), config.battlegroundIdToQueue);

        // Get the BG template to find the map ID
        BattlegroundTemplate const* bgTemplate = sBattlegroundMgr->GetBattlegroundTemplateByTypeId(bgTypeId);
        if (bgTemplate && !bgTemplate->MapIDs.empty())
        {
            // Determine bracket from bot's level
            PVPDifficultyEntry const* bracketEntry = DB2Manager::GetBattlegroundBracketByLevel(
                bgTemplate->MapIDs.front(), player->GetLevel());

            if (bracketEntry)
            {
                BattlegroundBracketId bracketId = bracketEntry->GetBracketId();

                // CRITICAL FIX: Use QueueBotForBGWithTracking to register in _queuedBots
                // This ensures OnInvitationReceived processes the invitation and bot enters BG
                // Without this, bots receive invitations but never teleport into the BG
                if (!config.humanPlayerGuid.IsEmpty())
                {
                    if (sBGBotManager->QueueBotForBGWithTracking(player, bgTypeId, bracketId, config.humanPlayerGuid))
                    {
                        TC_LOG_INFO("module.playerbot.configurator",
                            "Successfully queued bot {} for BG {} bracket {} (with tracking for human {})",
                            player->GetName(), config.battlegroundIdToQueue, static_cast<uint8>(bracketId),
                            config.humanPlayerGuid.ToString());
                    }
                    else
                    {
                        TC_LOG_WARN("module.playerbot.configurator",
                            "Failed to queue bot {} for BG {} with tracking",
                            player->GetName(), config.battlegroundIdToQueue);
                    }
                }
                else
                {
                    // Fallback to non-tracking queue (bot won't auto-accept invitation properly)
                    TC_LOG_WARN("module.playerbot.configurator",
                        "No humanPlayerGuid for bot {} - using non-tracking BG queue (invitation handling may fail)",
                        player->GetName());
                    if (sBGBotManager->QueueBotForBG(player, bgTypeId, bracketId))
                    {
                        TC_LOG_INFO("module.playerbot.configurator",
                            "Successfully queued bot {} for BG {} bracket {} (WITHOUT tracking)",
                            player->GetName(), config.battlegroundIdToQueue, static_cast<uint8>(bracketId));
                    }
                    else
                    {
                        TC_LOG_WARN("module.playerbot.configurator",
                            "Failed to queue bot {} for BG {}",
                            player->GetName(), config.battlegroundIdToQueue);
                    }
                }
            }
            else
            {
                TC_LOG_WARN("module.playerbot.configurator",
                    "Could not determine BG bracket for bot {} (level {}) on map {}",
                    player->GetName(), player->GetLevel(), bgTemplate->MapIDs.front());
            }
        }
        else
        {
            TC_LOG_WARN("module.playerbot.configurator",
                "Could not find BG template for type {}",
                config.battlegroundIdToQueue);
        }
    }

    // Arena queueing deferred to reactive join — see InstanceBotPool::WarmUpBot for details.

    // Calculate timing
    auto endTime = std::chrono::steady_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    _stats.totalConfigTimeMs.fetch_add(durationMs);

    if (success)
    {
        _stats.successfulConfigs.fetch_add(1);
        TC_LOG_INFO("module.playerbot.configurator",
            "Successfully configured bot {} in {}ms - Level: {}, Spec: {}, GS: {}",
            player->GetName(), durationMs, player->GetLevel(), specId, config.targetGearScore);
    }
    else
    {
        _stats.failedConfigs.fetch_add(1);
        TC_LOG_WARN("module.playerbot.configurator",
            "Partially configured bot {} in {}ms (some steps failed)",
            player->GetName(), durationMs);
    }

    // Step 9: Mark as instance bot if flagged
    // CRITICAL FIX (2026-02-03): Instance bot marking must happen AFTER login completes
    // Previously, MarkAsInstanceBot() was called immediately after AddPlayerBot() in JITBotFactory,
    // but AddPlayerBot() only queues the spawn - the session doesn't exist yet!
    // Now we mark the bot here, where the session is guaranteed to exist.
    if (config.markAsInstanceBot)
    {
        sBotWorldSessionMgr->MarkAsInstanceBot(playerGuid);
        TC_LOG_INFO("module.playerbot.configurator",
            "Marked bot {} as INSTANCE BOT (idle timeout enabled, restricted behavior)",
            player->GetName());
    }

    // CRITICAL: Add to recently configured set BEFORE removing pending config
    // This prevents the race condition where:
    // 1. We remove pending config
    // 2. BotWorldSessionMgr checks HasPendingConfiguration() - returns FALSE
    // 3. Bot gets submitted to BotLevelManager which re-levels it
    {
        std::lock_guard<std::mutex> lock(_configMutex);
        _recentlyConfiguredBots.insert(playerGuid);
        TC_LOG_INFO("module.playerbot.configurator",
            "Added bot {} to recently configured set (size: {})",
            player->GetName(), _recentlyConfiguredBots.size());
    }

    // Remove pending configuration
    RemovePendingConfiguration(playerGuid);

    return success;
}

// ============================================================================
// INDIVIDUAL CONFIGURATION STEPS
// ============================================================================

bool BotPostLoginConfigurator::ApplyLevel(Player* player, uint32 targetLevel)
{
    if (!player || targetLevel < 1 || targetLevel > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        return false;

    uint32 currentLevel = player->GetLevel();

    if (currentLevel == targetLevel)
        return true;

    // Use GiveLevel for proper stat calculation
    // GiveLevel automatically handles:
    // - Stat recalculation
    // - Talent tier unlocking
    // - Skill updates
    // - Specialization spells
    // - Health/mana restoration
    player->GiveLevel(targetLevel);

    TC_LOG_INFO("module.playerbot.configurator",
        "Applied level {} to bot {} (was: {})",
        targetLevel, player->GetName(), currentLevel);

    return player->GetLevel() == targetLevel;
}

bool BotPostLoginConfigurator::ApplySpecialization(Player* player, uint32 specId)
{
    if (!player || specId == 0)
        return false;

    TC_LOG_INFO("module.playerbot.configurator",
        "ApplySpecialization: Starting for bot {} (class={}) with specId={}",
        player->GetName(), player->GetClass(), specId);

    // Validate specialization exists
    ChrSpecializationEntry const* specEntry = sChrSpecializationStore.LookupEntry(specId);
    if (!specEntry)
    {
        TC_LOG_WARN("module.playerbot.configurator",
            "Invalid specialization ID {} for bot {}",
            specId, player->GetName());
        return false;
    }

    TC_LOG_INFO("module.playerbot.configurator",
        "ApplySpecialization: specEntry found - ClassID={}, specName index={}",
        specEntry->ClassID, specEntry->ID);

    // Validate class matches
    if (specEntry->ClassID != player->GetClass())
    {
        TC_LOG_WARN("module.playerbot.configurator",
            "Specialization {} is for class {} but bot {} is class {}",
            specId, specEntry->ClassID, player->GetName(), player->GetClass());
        return false;
    }

    // Set the primary specialization
    TC_LOG_INFO("module.playerbot.configurator",
        "ApplySpecialization: About to call SetPrimarySpecialization({}) for bot {}",
        specId, player->GetName());

    try
    {
        player->SetPrimarySpecialization(specId);
    }
    catch (std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.configurator",
            "ApplySpecialization: SetPrimarySpecialization CRASHED for bot {}: {}",
            player->GetName(), e.what());
        return false;
    }

    TC_LOG_INFO("module.playerbot.configurator",
        "ApplySpecialization: SetPrimarySpecialization completed, current spec={}",
        static_cast<uint32>(player->GetPrimarySpecialization()));

    // Learn specialization spells - SKIP for now as it causes crashes
    // The crash happens in LearnSpell() -> SendDirectMessage() when the bot
    // doesn't have a fully initialized session/packet handling
    TC_LOG_INFO("module.playerbot.configurator",
        "ApplySpecialization: SKIPPING LearnSpecializationSpells() for bot {} - will learn via ApplyClassSpells later",
        player->GetName());

    // NOTE: We skip this because:
    // 1. LearnSpell calls SendDirectMessage which can crash with invalid session
    // 2. ApplyClassSpells() is called after this and will handle spell learning
    // 3. The bot AI will also learn necessary spells when initializing
    //
    // If needed in future, check these conditions before calling:
    // - player->IsInWorld()
    // - player->GetSession() != nullptr
    // - player->GetSession()->IsConnected()

    TC_LOG_INFO("module.playerbot.configurator",
        "Applied specialization {} to bot {}",
        specId, player->GetName());

    return true;
}

bool BotPostLoginConfigurator::ApplyTalents(Player* player, BotTemplate const* tmpl)
{
    if (!player || !tmpl)
        return false;

    // Must have specialization set first
    if (player->GetPrimarySpecialization() == ChrSpecialization::None)
    {
        TC_LOG_WARN("module.playerbot.configurator",
            "Cannot apply talents - bot {} has no specialization",
            player->GetName());
        return false;
    }

    // Temporarily allow talent changes without rest area
    player->SetUnitFlag2(UNIT_FLAG2_ALLOW_CHANGING_TALENTS);

    uint32 talentsLearned = 0;
    uint32 talentsFailed = 0;

    for (uint32 talentId : tmpl->talents.talentIds)
    {
        if (LearnTalent(player, talentId))
            ++talentsLearned;
        else
            ++talentsFailed;
    }

    // Remove temporary flag
    player->RemoveUnitFlag2(UNIT_FLAG2_ALLOW_CHANGING_TALENTS);

    TC_LOG_INFO("module.playerbot.configurator",
        "Applied talents to bot {}: {} learned, {} failed",
        player->GetName(), talentsLearned, talentsFailed);

    return talentsFailed == 0;
}

bool BotPostLoginConfigurator::LearnTalent(Player* player, uint32 talentId)
{
    if (!player || talentId == 0)
        return false;

    // Validate talent exists
    TalentEntry const* talentEntry = sTalentStore.LookupEntry(talentId);
    if (!talentEntry)
    {
        TC_LOG_TRACE("module.playerbot.configurator",
            "Invalid talent ID {} for bot {}",
            talentId, player->GetName());
        return false;
    }

    // Check class match
    if (talentEntry->ClassID != player->GetClass())
    {
        TC_LOG_TRACE("module.playerbot.configurator",
            "Talent {} is for class {} but bot {} is class {}",
            talentId, talentEntry->ClassID, player->GetName(), player->GetClass());
        return false;
    }

    // Try to learn the talent
    int32 spellOnCooldown = 0;
    TalentLearnResult result = player->LearnTalent(talentId, &spellOnCooldown);

    if (result == TALENT_LEARN_OK)
    {
        TC_LOG_TRACE("module.playerbot.configurator",
            "Bot {} learned talent {}",
            player->GetName(), talentId);
        return true;
    }

    // Log specific failure reason at trace level
    switch (result)
    {
        case TALENT_FAILED_AFFECTING_COMBAT:
            TC_LOG_TRACE("module.playerbot.configurator",
                "Talent {} failed for bot {}: in combat", talentId, player->GetName());
            break;
        case TALENT_FAILED_CANT_DO_THAT_RIGHT_NOW:
            TC_LOG_TRACE("module.playerbot.configurator",
                "Talent {} failed for bot {}: dead", talentId, player->GetName());
            break;
        case TALENT_FAILED_NO_PRIMARY_TREE_SELECTED:
            TC_LOG_TRACE("module.playerbot.configurator",
                "Talent {} failed for bot {}: no spec", talentId, player->GetName());
            break;
        case TALENT_FAILED_REST_AREA:
            TC_LOG_TRACE("module.playerbot.configurator",
                "Talent {} failed for bot {}: not in rest area", talentId, player->GetName());
            break;
        default:
            TC_LOG_TRACE("module.playerbot.configurator",
                "Talent {} failed for bot {}: unknown ({})", talentId, player->GetName(), int(result));
            break;
    }

    return false;
}

bool BotPostLoginConfigurator::ApplyGear(Player* player, BotTemplate const* tmpl, uint32 targetGearScore)
{
    if (!player)
        return false;

    // First, try to use template gear set if available and has valid items
    bool useTemplateGear = false;
    GearSetTemplate const* gearSet = nullptr;

    if (tmpl)
    {
        gearSet = SelectGearSet(tmpl, targetGearScore);
        if (gearSet)
        {
            // Check if template has any valid item IDs (non-zero)
            for (auto const& slotData : gearSet->slots)
            {
                if (slotData.itemId != 0)
                {
                    useTemplateGear = true;
                    break;
                }
            }
        }
    }

    // If template has valid items, use them
    if (useTemplateGear && gearSet)
    {
        TC_LOG_INFO("module.playerbot.configurator",
            "Using template gear set iLvl {} (actual GS: {}) for bot {}",
            gearSet->targetItemLevel, gearSet->actualGearScore, player->GetName());

        uint32 itemsEquipped = 0;
        uint32 itemsFailed = 0;

        for (uint8 slot = 0; slot < EQUIPMENT_SLOT_END; ++slot)
        {
            if (slot >= gearSet->slots.size())
                break;

            GearSlotTemplate const& slotData = gearSet->slots[slot];
            if (slotData.itemId == 0)
                continue;

            if (EquipItem(player, slot, slotData.itemId))
                ++itemsEquipped;
            else
                ++itemsFailed;
        }

        TC_LOG_INFO("module.playerbot.configurator",
            "Applied template gear to bot {}: {} equipped, {} failed",
            player->GetName(), itemsEquipped, itemsFailed);

        return itemsFailed == 0;
    }

    // FALLBACK: Use BotGearFactory to generate and apply gear dynamically
    // This handles cases where:
    // 1. Template has no gear sets
    // 2. Template gear sets have placeholder items (itemId = 0)
    // 3. No template was provided
    TC_LOG_INFO("module.playerbot.configurator",
        "Template has no valid gear - using BotGearFactory for bot {} (level {}, class {}, spec {})",
        player->GetName(), player->GetLevel(), player->GetClass(),
        static_cast<uint32>(player->GetPrimarySpecialization()));

    if (!sBotGearFactory->IsReady())
    {
        TC_LOG_WARN("module.playerbot.configurator",
            "BotGearFactory not ready - cannot generate gear for bot {}",
            player->GetName());
        return false;
    }

    // Determine faction
    TeamId faction = player->GetTeamId();

    // Build gear set using BotGearFactory
    GearSet generatedGear = sBotGearFactory->BuildGearSet(
        player->GetClass(),
        static_cast<uint32>(player->GetPrimarySpecialization()),
        player->GetLevel(),
        faction
    );

    // Apply the generated gear set
    if (!sBotGearFactory->ApplyGearSet(player, generatedGear))
    {
        TC_LOG_WARN("module.playerbot.configurator",
            "BotGearFactory failed to apply gear to bot {}",
            player->GetName());
        return false;
    }

    TC_LOG_INFO("module.playerbot.configurator",
        "BotGearFactory successfully equipped bot {} with generated gear",
        player->GetName());

    return true;
}

bool BotPostLoginConfigurator::EquipItem(Player* player, uint8 slot, uint32 itemId)
{
    if (!player || itemId == 0)
        return false;

    // Validate item exists
    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
    if (!itemTemplate)
    {
        TC_LOG_TRACE("module.playerbot.configurator",
            "Invalid item ID {} for slot {} on bot {}",
            itemId, slot, player->GetName());
        return false;
    }

    // Remove existing item in slot
    Item* existingItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
    if (existingItem)
    {
        player->DestroyItem(INVENTORY_SLOT_BAG_0, slot, true);
    }

    // Create and equip new item
    // EquipNewItem handles: item creation, validation, stat application
    Item* newItem = player->EquipNewItem(slot, itemId, ItemContext::NONE, true);

    if (newItem)
    {
        TC_LOG_TRACE("module.playerbot.configurator",
            "Bot {} equipped item {} in slot {}",
            player->GetName(), itemId, slot);
        return true;
    }

    TC_LOG_TRACE("module.playerbot.configurator",
        "Failed to equip item {} in slot {} for bot {}",
        itemId, slot, player->GetName());
    return false;
}

bool BotPostLoginConfigurator::ApplyActionBars(Player* player, BotTemplate const* tmpl)
{
    if (!player || !tmpl)
        return false;

    uint32 buttonsSet = 0;

    for (ActionBarButton const& button : tmpl->actionBars.buttons)
    {
        // Calculate global slot: bar * 12 + slot
        uint8 globalSlot = static_cast<uint8>(button.actionBar * 12 + button.slot);

        // Set the action button
        player->AddActionButton(globalSlot, button.actionId, button.actionType);
        ++buttonsSet;
    }

    TC_LOG_INFO("module.playerbot.configurator",
        "Applied {} action buttons to bot {}",
        buttonsSet, player->GetName());

    return true;
}

bool BotPostLoginConfigurator::ApplyClassSpells(Player* player)
{
    if (!player)
        return false;

    // Learn default skills and spells for level
    player->LearnDefaultSkills();
    player->UpdateSkillsForLevel();

    // ========================================================================
    // ESSENTIAL CLASS SPELLS - Teach core abilities directly
    // ========================================================================
    // Standard TrinityCore learning (LearnDefaultSkills, SkillRewardedSpells) only teaches
    // spells with AcquireMethod::Automatic*. Many core abilities require trainer visits.
    // For bots, we teach them directly to ensure rotation functionality.
    // ========================================================================
    LearnEssentialClassSpells(player);

    // ========================================================================
    // SPECIALIZATION SPELL LEARNING
    // ========================================================================
    // Use standard TrinityCore method when possible:
    // - If NOT in world: LearnSpecializationSpells() won't send packets (IsInWorld check)
    // - If IN world: Use AddSpell directly to avoid log spam from failed packet sends
    //   (Bot sessions have no socket, so SendDirectMessage logs errors but doesn't crash)
    // ========================================================================
    if (player->GetPrimarySpecialization() != ChrSpecialization::None)
    {
        if (!player->IsInWorld())
        {
            // Standard TrinityCore path - safe because packets won't be sent
            player->LearnSpecializationSpells();

            TC_LOG_INFO("module.playerbot.configurator",
                "ApplyClassSpells: Bot {} learned specialization spells via standard method (not in world)",
                player->GetName());
        }
        else
        {
            // Bot is already in world - use AddSpell directly to avoid log spam
            uint32 specId = static_cast<uint32>(player->GetPrimarySpecialization());
            uint32 spellsLearned = 0;

            if (std::vector<SpecializationSpellsEntry const*> const* specSpells = sDB2Manager.GetSpecializationSpells(specId))
            {
                for (SpecializationSpellsEntry const* specSpell : *specSpells)
                {
                    if (!specSpell)
                        continue;

                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(specSpell->SpellID, DIFFICULTY_NONE);
                    if (!spellInfo || spellInfo->SpellLevel > player->GetLevel())
                        continue;

                    if (player->AddSpell(specSpell->SpellID, true, true, false, false, false, 0, false, {}))
                    {
                        ++spellsLearned;
                        if (specSpell->OverridesSpellID)
                            player->AddOverrideSpell(specSpell->OverridesSpellID, specSpell->SpellID);
                    }
                }
            }

            // Learn mastery spells
            if (player->CanUseMastery())
            {
                ChrSpecializationEntry const* spec = sChrSpecializationStore.LookupEntry(specId);
                if (spec)
                {
                    for (uint32 i = 0; i < MAX_MASTERY_SPELLS; ++i)
                    {
                        if (uint32 masterySpellId = spec->MasterySpellID[i])
                        {
                            if (player->AddSpell(masterySpellId, true, true, false, false, false, 0, false, {}))
                                ++spellsLearned;
                        }
                    }
                }
            }

            TC_LOG_INFO("module.playerbot.configurator",
                "ApplyClassSpells: Bot {} (spec={}) learned {} spells via AddSpell (already in world)",
                player->GetName(), specId, spellsLearned);
        }
    }
    else
    {
        TC_LOG_INFO("module.playerbot.configurator",
            "ApplyClassSpells: Bot {} has no specialization set - skipping spec spells",
            player->GetName());
    }

    TC_LOG_INFO("module.playerbot.configurator",
        "Applied class spells to bot {} (level {})",
        player->GetName(), player->GetLevel());

    return true;
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

GearSetTemplate const* BotPostLoginConfigurator::SelectGearSet(BotTemplate const* tmpl, uint32 targetGearScore) const
{
    if (!tmpl || tmpl->gearSets.empty())
        return nullptr;

    // If target gear score is 0, use the lowest available set
    if (targetGearScore == 0)
    {
        GearSetTemplate const* lowest = nullptr;
        for (auto const& pair : tmpl->gearSets)
        {
            if (!lowest || pair.second.targetItemLevel < lowest->targetItemLevel)
                lowest = &pair.second;
        }
        return lowest;
    }

    // Find the closest gear set to target
    GearSetTemplate const* bestMatch = nullptr;
    uint32 bestDiff = std::numeric_limits<uint32>::max();

    for (auto const& pair : tmpl->gearSets)
    {
        uint32 setGS = pair.second.actualGearScore;
        uint32 diff = (setGS > targetGearScore) ? (setGS - targetGearScore) : (targetGearScore - setGS);

        if (diff < bestDiff)
        {
            bestDiff = diff;
            bestMatch = &pair.second;
        }
    }

    return bestMatch;
}

// ============================================================================
// SPELL LEARNING VERIFICATION - Modern WoW 12.0 Approach
// ============================================================================
// In modern WoW (since Patch 5.0.4 / MoP 2012), ALL combat spells are learned
// automatically on level up. Class trainers no longer exist for combat abilities.
//
// TrinityCore's native spell learning system handles this via:
// 1. LearnDefaultSkills() - learns class skills and auto-granted spells
// 2. LearnSpecializationSpells() - learns spec-specific spells from SpecializationSpells.db2
//
// This function exists only for DIAGNOSTIC purposes - to verify that spells
// were learned correctly and log any missing spells for investigation.
//
// If bots are missing spells, the issue is likely:
// - Spec not set before LearnSpecializationSpells() was called
// - DB2 data missing entries (SpecializationSpells.db2 or SkillLineAbility.db2)
// - Timing issue with when spell learning occurs in bot login flow
// ============================================================================

// Helper: Get key diagnostic spells for each class to verify spell learning
static std::vector<std::pair<uint32, std::string>> GetDiagnosticSpells(uint8 playerClass)
{
    switch (playerClass)
    {
        case CLASS_WARRIOR:
            return {
                {6673, "Battle Shout"},
                {100, "Charge"},
                {5308, "Execute"},
                {12294, "Mortal Strike"},      // Arms
                {23881, "Bloodthirst"},        // Fury
                {23922, "Shield Slam"},        // Protection
                {1680, "Whirlwind"},
                {6552, "Pummel"},
            };
        case CLASS_PALADIN:
            return {
                {35395, "Crusader Strike"},
                {20271, "Judgment"},
                {853, "Hammer of Justice"},
                {85256, "Templar's Verdict"},  // Retribution
                {85673, "Word of Glory"},
                {31935, "Avenger's Shield"},   // Protection
            };
        case CLASS_HUNTER:
            return {
                {185358, "Arcane Shot"},
                {257620, "Multi-Shot"},
                {19434, "Aimed Shot"},         // Marksmanship
                {34026, "Kill Command"},       // Beast Mastery
                {781, "Disengage"},
            };
        case CLASS_ROGUE:
            return {
                {1752, "Sinister Strike"},
                {196819, "Eviscerate"},
                {1856, "Vanish"},
                {408, "Kidney Shot"},
                {703, "Garrote"},
            };
        case CLASS_PRIEST:
            return {
                {585, "Smite"},
                {589, "Shadow Word: Pain"},
                {17, "Power Word: Shield"},
                {2061, "Flash Heal"},
                {34914, "Vampiric Touch"},     // Shadow
            };
        case CLASS_DEATH_KNIGHT:
            return {
                {49998, "Death Strike"},
                {47541, "Death Coil"},
                {49576, "Death Grip"},
                {49020, "Obliterate"},         // Frost
                {55090, "Scourge Strike"},     // Unholy
            };
        case CLASS_SHAMAN:
            return {
                {188196, "Lightning Bolt"},
                {188389, "Flame Shock"},
                {51505, "Lava Burst"},
                {8004, "Healing Surge"},
                {17364, "Stormstrike"},        // Enhancement
            };
        case CLASS_MAGE:
            return {
                {116, "Frostbolt"},
                {133, "Fireball"},
                {30451, "Arcane Blast"},
                {1953, "Blink"},
                {2139, "Counterspell"},
            };
        case CLASS_WARLOCK:
            return {
                {686, "Shadow Bolt"},
                {172, "Corruption"},
                {348, "Immolate"},
                {5782, "Fear"},
                {980, "Agony"},
            };
        case CLASS_MONK:
            return {
                {100780, "Tiger Palm"},
                {100784, "Blackout Kick"},
                {109132, "Roll"},
                {113656, "Fists of Fury"},     // Windwalker
                {115175, "Soothing Mist"},     // Mistweaver
            };
        case CLASS_DRUID:
            return {
                {5176, "Wrath"},
                {8921, "Moonfire"},
                {774, "Rejuvenation"},
                {5221, "Shred"},
                {33917, "Mangle"},
            };
        case CLASS_DEMON_HUNTER:
            return {
                {162243, "Demon's Bite"},
                {198013, "Eye Beam"},
                {195072, "Fel Rush"},
                {185245, "Torment"},           // Vengeance
                {179057, "Chaos Nova"},
            };
        case CLASS_EVOKER:
            return {
                {361469, "Living Flame"},
                {362969, "Azure Strike"},
                {357208, "Fire Breath"},
                {355913, "Emerald Blossom"},
            };
        default:
            return {};
    }
}

void BotPostLoginConfigurator::LearnEssentialClassSpells(Player* player)
{
    if (!player)
        return;

    uint8 playerClass = player->GetClass();
    uint32 playerLevel = player->GetLevel();
    ChrSpecializationEntry const* spec = player->GetPrimarySpecializationEntry();

    TC_LOG_INFO("module.playerbot.configurator",
        "=== SPELL DIAGNOSTIC: Bot {} (class={}, level={}, spec={}) ===",
        player->GetName(), playerClass, playerLevel, spec ? spec->ID : 0);

    // Log spec details
    if (spec)
    {
        TC_LOG_INFO("module.playerbot.configurator",
            "  Spec: ID={}, Name={}, ClassID={}, OrderIndex={}",
            spec->ID, spec->Name[DEFAULT_LOCALE], spec->ClassID, spec->OrderIndex);
    }
    else
    {
        TC_LOG_WARN("module.playerbot.configurator",
            "  WARNING: No valid specialization entry!");
    }

    // Count spells BEFORE we call native methods
    uint32 spellCountBefore = 0;
    for (auto const& spellPair : player->GetSpellMap())
    {
        if (spellPair.second.active && !spellPair.second.disabled)
            ++spellCountBefore;
    }

    TC_LOG_INFO("module.playerbot.configurator",
        "  Spells before native learning: {}", spellCountBefore);

    // Call native methods as safety net
    if (spec && spec->ID > 0)
    {
        player->LearnSpecializationSpells();
    }
    player->LearnDefaultSkills();
    player->UpdateSkillsForLevel();

    // Count spells AFTER
    uint32 spellCountAfter = 0;
    for (auto const& spellPair : player->GetSpellMap())
    {
        if (spellPair.second.active && !spellPair.second.disabled)
            ++spellCountAfter;
    }

    TC_LOG_INFO("module.playerbot.configurator",
        "  Spells after native learning: {} (added {})",
        spellCountAfter, spellCountAfter - spellCountBefore);

    // Check diagnostic spells for this class
    auto diagnosticSpells = GetDiagnosticSpells(playerClass);
    uint32 hasCount = 0;
    uint32 missingCount = 0;

    TC_LOG_INFO("module.playerbot.configurator",
        "  --- Checking {} key spells for class {} ---", diagnosticSpells.size(), playerClass);

    for (auto const& [spellId, spellName] : diagnosticSpells)
    {
        bool hasSpell = player->HasSpell(spellId);
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);

        if (hasSpell)
        {
            ++hasCount;
            TC_LOG_DEBUG("module.playerbot.configurator",
                "    [OK] {} ({}) - LEARNED", spellName, spellId);
        }
        else
        {
            ++missingCount;
            if (spellInfo)
            {
                uint32 reqLevel = std::max(spellInfo->SpellLevel, spellInfo->BaseLevel);
                if (reqLevel > playerLevel)
                {
                    TC_LOG_DEBUG("module.playerbot.configurator",
                        "    [--] {} ({}) - Not learned (requires level {}, bot is {})",
                        spellName, spellId, reqLevel, playerLevel);
                }
                else
                {
                    TC_LOG_WARN("module.playerbot.configurator",
                        "    [MISSING] {} ({}) - Should be learned! (req level {} <= bot level {})",
                        spellName, spellId, reqLevel, playerLevel);
                }
            }
            else
            {
                TC_LOG_ERROR("module.playerbot.configurator",
                    "    [ERROR] {} ({}) - Spell ID not found in SpellMgr!",
                    spellName, spellId);
            }
        }
    }

    TC_LOG_INFO("module.playerbot.configurator",
        "  Summary: {}/{} diagnostic spells present, {} missing",
        hasCount, diagnosticSpells.size(), missingCount);

    // Log what spells are in SpecializationSpells.db2 for this spec
    if (spec && spec->ID > 0)
    {
        if (std::vector<SpecializationSpellsEntry const*> const* specSpells =
            sDB2Manager.GetSpecializationSpells(spec->ID))
        {
            TC_LOG_INFO("module.playerbot.configurator",
                "  --- SpecializationSpells.db2 has {} entries for spec {} ---",
                specSpells->size(), spec->ID);

            uint32 specSpellsLearned = 0;
            uint32 specSpellsMissing = 0;
            for (SpecializationSpellsEntry const* entry : *specSpells)
            {
                if (!entry) continue;

                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(entry->SpellID, DIFFICULTY_NONE);
                char const* spellName = (spellInfo && spellInfo->SpellName) ? (*spellInfo->SpellName)[DEFAULT_LOCALE] : "Unknown";
                bool hasIt = player->HasSpell(entry->SpellID);

                if (hasIt)
                {
                    ++specSpellsLearned;
                    TC_LOG_TRACE("module.playerbot.configurator",
                        "    [OK] SpellID {} ({}) - learned",
                        entry->SpellID, spellName);
                }
                else
                {
                    ++specSpellsMissing;
                    uint32 reqLevel = spellInfo ? std::max(spellInfo->SpellLevel, spellInfo->BaseLevel) : 0;
                    if (reqLevel > playerLevel)
                    {
                        TC_LOG_TRACE("module.playerbot.configurator",
                            "    [--] SpellID {} ({}) - requires level {}",
                            entry->SpellID, spellName, reqLevel);
                    }
                    else
                    {
                        TC_LOG_WARN("module.playerbot.configurator",
                            "    [MISSING] SpellID {} ({}) - SHOULD be learned! (req {})",
                            entry->SpellID, spellName, reqLevel);
                    }
                }
            }
            TC_LOG_INFO("module.playerbot.configurator",
                "  Spec spells: {}/{} learned, {} missing (may be level-gated)",
                specSpellsLearned, specSpells->size(), specSpellsMissing);
        }
        else
        {
            TC_LOG_ERROR("module.playerbot.configurator",
                "  ERROR: No SpecializationSpells.db2 entries found for spec {}!", spec->ID);
        }
    }

    TC_LOG_INFO("module.playerbot.configurator",
        "=== END SPELL DIAGNOSTIC for {} ===", player->GetName());
}

void BotPostLoginConfigurator::ResetStats()
{
    _stats.totalConfigured.store(0);
    _stats.successfulConfigs.store(0);
    _stats.failedConfigs.store(0);
    _stats.pendingConfigs.store(0);
    _stats.totalConfigTimeMs.store(0);
}

} // namespace Playerbot
