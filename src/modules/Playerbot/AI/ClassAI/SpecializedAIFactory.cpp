/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Enterprise-Grade Specialized AI Factory
 *
 * This factory creates specialization-specific AI instances with full
 * Phase 5 decision system integration (ActionPriorityQueue + BehaviorTree).
 * All 40 specializations are supported with proper initialization.
 */

#include "SpecializedAIFactory.h"
#include "Player.h"
#include "Log.h"

// Include all Refactored specialization headers
#include "Warriors/ArmsWarrior.h"
#include "Warriors/FuryWarrior.h"
#include "Warriors/ProtectionWarrior.h"

#include "Paladins/HolyPaladin.h"
#include "Paladins/ProtectionPaladin.h"
#include "Paladins/RetributionPaladin.h"

#include "Hunters/BeastMasteryHunter.h"
#include "Hunters/MarksmanshipHunter.h"
#include "Hunters/SurvivalHunter.h"

#include "Rogues/AssassinationRogue.h"
#include "Rogues/OutlawRogue.h"
#include "Rogues/SubtletyRogue.h"

#include "Priests/DisciplinePriest.h"
#include "Priests/HolyPriest.h"
#include "Priests/ShadowPriest.h"

#include "DeathKnights/BloodDeathKnight.h"
#include "DeathKnights/FrostDeathKnight.h"
#include "DeathKnights/UnholyDeathKnight.h"

#include "Shamans/ElementalShaman.h"
#include "Shamans/EnhancementShaman.h"
#include "Shamans/RestorationShaman.h"

#include "Mages/ArcaneMage.h"
#include "Mages/FireMage.h"
#include "Mages/FrostMage.h"

#include "Warlocks/AfflictionWarlock.h"
#include "Warlocks/DemonologyWarlock.h"
#include "Warlocks/DestructionWarlock.h"

#include "Monks/BrewmasterMonk.h"
#include "Monks/MistweaverMonk.h"
#include "Monks/WindwalkerMonk.h"

#include "Druids/BalanceDruid.h"
#include "Druids/FeralDruid.h"
#include "Druids/GuardianDruid.h"
#include "Druids/RestorationDruid.h"

#include "DemonHunters/HavocDemonHunter.h"
#include "DemonHunters/VengeanceDemonHunter.h"

#include "Evokers/DevastationEvoker.h"
#include "Evokers/PreservationEvoker.h"
#include "Evokers/AugmentationEvoker.h"

namespace Playerbot
{

::std::unique_ptr<BotAI> SpecializedAIFactory::CreateSpecializedAI(Player* bot)
{

    uint8 classId = bot->GetClass();
    uint8 specId = static_cast<uint8>(bot->GetPrimarySpecialization());

    // Note: Use GetGUID().GetCounter() instead of GetName() - Player name may not be loaded yet
    TC_LOG_DEBUG("module.playerbot.ai.factory",
                 "Creating specialized AI for bot GUID {} (class: {}, spec: {})",
                 bot->GetGUID().GetCounter(), classId, specId);

    try
    {
        ::std::unique_ptr<BotAI> specializedAI;

        switch (classId)
        {
            case CLASS_WARRIOR:
                specializedAI = CreateWarriorAI(bot, specId);
                break;

            case CLASS_PALADIN:
                specializedAI = CreatePaladinAI(bot, specId);
                break;

            case CLASS_HUNTER:
                specializedAI = CreateHunterAI(bot, specId);
                break;

            case CLASS_ROGUE:
                specializedAI = CreateRogueAI(bot, specId);
                break;

            case CLASS_PRIEST:
                specializedAI = CreatePriestAI(bot, specId);
                break;

            case CLASS_DEATH_KNIGHT:
                specializedAI = CreateDeathKnightAI(bot, specId);
                break;

            case CLASS_SHAMAN:
                specializedAI = CreateShamanAI(bot, specId);
                break;

            case CLASS_MAGE:
                specializedAI = CreateMageAI(bot, specId);
                break;

            case CLASS_WARLOCK:
                specializedAI = CreateWarlockAI(bot, specId);
                break;

            case CLASS_MONK:
                specializedAI = CreateMonkAI(bot, specId);
                break;

            case CLASS_DRUID:
                specializedAI = CreateDruidAI(bot, specId);
                break;

            case CLASS_DEMON_HUNTER:
                specializedAI = CreateDemonHunterAI(bot, specId);
                break;

            case CLASS_EVOKER:
                specializedAI = CreateEvokerAI(bot, specId);
                break;

            default:
                TC_LOG_ERROR("module.playerbot.ai.factory",
                            "Unknown class {} for bot GUID {}", classId, bot->GetGUID().GetCounter());
                return nullptr;
        }

        if (specializedAI)
        {
            TC_LOG_INFO("module.playerbot.ai.factory",
                       "Successfully created specialized AI for bot GUID {} (class: {}, spec: {})",
                       bot->GetGUID().GetCounter(), classId, specId);
        }

        return specializedAI;
    }
    catch (const ::std::exception& e)
    {
        TC_LOG_ERROR("module.playerbot.ai.factory",
                     "Exception creating specialized AI for bot GUID {}: {}",
                     bot->GetGUID().GetCounter(), e.what());
        return nullptr;
    }
}

// ============================================================================
// WARRIOR SPECIALIZATIONS (3)
// ============================================================================

::std::unique_ptr<BotAI> SpecializedAIFactory::CreateWarriorAI(Player* bot, uint8 specId)
{
    switch (specId)
    {
        case 0: // Arms
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating ArmsWarriorRefactored for {}", bot->GetName());
            return ::std::make_unique<ArmsWarriorRefactored>(bot);

        case 1: // Fury
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating FuryWarriorRefactored for {}", bot->GetName());
            return ::std::make_unique<FuryWarriorRefactored>(bot);

        case 2: // Protection
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating ProtectionWarriorRefactored for {}", bot->GetName());
            return ::std::make_unique<ProtectionWarriorRefactored>(bot);

        default:
            TC_LOG_WARN("module.playerbot.ai.factory", "Unknown Warrior spec {}, defaulting to Arms", specId);
            return ::std::make_unique<ArmsWarriorRefactored>(bot);
    }
}

// ============================================================================
// PALADIN SPECIALIZATIONS (3)
// ============================================================================

::std::unique_ptr<BotAI> SpecializedAIFactory::CreatePaladinAI(Player* bot, uint8 specId)
{
    switch (specId)
    {
        case 0: // Holy
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating HolyPaladinRefactored for {}", bot->GetName());
            return ::std::make_unique<HolyPaladinRefactored>(bot);

        case 1: // Protection
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating ProtectionPaladinRefactored for {}", bot->GetName());
            return ::std::make_unique<ProtectionPaladinRefactored>(bot);

        case 2: // Retribution
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating RetributionPaladinRefactored for {}", bot->GetName());
            return ::std::make_unique<RetributionPaladinRefactored>(bot);

        default:
            TC_LOG_WARN("module.playerbot.ai.factory", "Unknown Paladin spec {}, defaulting to Retribution", specId);
            return ::std::make_unique<RetributionPaladinRefactored>(bot);
    }
}

// ============================================================================
// HUNTER SPECIALIZATIONS (3)
// ============================================================================

::std::unique_ptr<BotAI> SpecializedAIFactory::CreateHunterAI(Player* bot, uint8 specId)
{
    switch (specId)
    {
        case 0: // Beast Mastery
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating BeastMasteryHunterRefactored for {}", bot->GetName());
            return ::std::make_unique<BeastMasteryHunterRefactored>(bot);

        case 1: // Marksmanship
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating MarksmanshipHunterRefactored for {}", bot->GetName());
            return ::std::make_unique<MarksmanshipHunterRefactored>(bot);

        case 2: // Survival
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating SurvivalHunterRefactored for {}", bot->GetName());
            return ::std::make_unique<SurvivalHunterRefactored>(bot);

        default:
            TC_LOG_WARN("module.playerbot.ai.factory", "Unknown Hunter spec {}, defaulting to Beast Mastery", specId);
            return ::std::make_unique<BeastMasteryHunterRefactored>(bot);
    }
}

// ============================================================================
// ROGUE SPECIALIZATIONS (3)
// ============================================================================

::std::unique_ptr<BotAI> SpecializedAIFactory::CreateRogueAI(Player* bot, uint8 specId)
{
    switch (specId)
    {
        case 0: // Assassination
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating AssassinationRogueRefactored for {}", bot->GetName());
            return ::std::make_unique<AssassinationRogueRefactored>(bot);

        case 1: // Outlaw
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating OutlawRogueRefactored for {}", bot->GetName());
            return ::std::make_unique<OutlawRogueRefactored>(bot);

        case 2: // Subtlety
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating SubtletyRogueRefactored for {}", bot->GetName());
            return ::std::make_unique<SubtletyRogueRefactored>(bot);

        default:
            TC_LOG_WARN("module.playerbot.ai.factory", "Unknown Rogue spec {}, defaulting to Assassination", specId);
            return ::std::make_unique<AssassinationRogueRefactored>(bot);
    }
}

// ============================================================================
// PRIEST SPECIALIZATIONS (3)
// ============================================================================

::std::unique_ptr<BotAI> SpecializedAIFactory::CreatePriestAI(Player* bot, uint8 specId)
{
    switch (specId)
    {
        case 0: // Discipline
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating DisciplinePriestRefactored for {}", bot->GetName());
            return ::std::make_unique<DisciplinePriestRefactored>(bot);

        case 1: // Holy
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating HolyPriestRefactored for {}", bot->GetName());
            return ::std::make_unique<HolyPriestRefactored>(bot);

        case 2: // Shadow
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating ShadowPriestRefactored for {}", bot->GetName());
            return ::std::make_unique<ShadowPriestRefactored>(bot);

        default:
            TC_LOG_WARN("module.playerbot.ai.factory", "Unknown Priest spec {}, defaulting to Holy", specId);
            return ::std::make_unique<HolyPriestRefactored>(bot);
    }
}

// ============================================================================
// DEATH KNIGHT SPECIALIZATIONS (3)
// ============================================================================

::std::unique_ptr<BotAI> SpecializedAIFactory::CreateDeathKnightAI(Player* bot, uint8 specId)
{
    switch (specId)
    {
        case 0: // Blood
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating BloodDeathKnightRefactored for {}", bot->GetName());
            return ::std::make_unique<BloodDeathKnightRefactored>(bot);

        case 1: // Frost
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating FrostDeathKnightRefactored for {}", bot->GetName());
            return ::std::make_unique<FrostDeathKnightRefactored>(bot);

        case 2: // Unholy
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating UnholyDeathKnightRefactored for {}", bot->GetName());
            return ::std::make_unique<UnholyDeathKnightRefactored>(bot);

        default:
            TC_LOG_WARN("module.playerbot.ai.factory", "Unknown Death Knight spec {}, defaulting to Blood", specId);
            return ::std::make_unique<BloodDeathKnightRefactored>(bot);
    }
}

// ============================================================================
// SHAMAN SPECIALIZATIONS (3)
// ============================================================================

::std::unique_ptr<BotAI> SpecializedAIFactory::CreateShamanAI(Player* bot, uint8 specId)
{
    switch (specId)
    {
        case 0: // Elemental
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating ElementalShamanRefactored for {}", bot->GetName());
            return ::std::make_unique<ElementalShamanRefactored>(bot);

        case 1: // Enhancement
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating EnhancementShamanRefactored for {}", bot->GetName());
            return ::std::make_unique<EnhancementShamanRefactored>(bot);

        case 2: // Restoration
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating RestorationShamanRefactored for {}", bot->GetName());
            return ::std::make_unique<RestorationShamanRefactored>(bot);

        default:
            TC_LOG_WARN("module.playerbot.ai.factory", "Unknown Shaman spec {}, defaulting to Elemental", specId);
            return ::std::make_unique<ElementalShamanRefactored>(bot);
    }
}

// ============================================================================
// MAGE SPECIALIZATIONS (3)
// ============================================================================

::std::unique_ptr<BotAI> SpecializedAIFactory::CreateMageAI(Player* bot, uint8 specId)
{
    switch (specId)
    {
        case 0: // Arcane
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating ArcaneMageRefactored for {}", bot->GetName());
            return ::std::make_unique<ArcaneMageRefactored>(bot);

        case 1: // Fire
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating FireMageRefactored for {}", bot->GetName());
            return ::std::make_unique<FireMageRefactored>(bot);

        case 2: // Frost
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating FrostMageRefactored for {}", bot->GetName());
            return ::std::make_unique<FrostMageRefactored>(bot);

        default:
            TC_LOG_WARN("module.playerbot.ai.factory", "Unknown Mage spec {}, defaulting to Frost", specId);
            return ::std::make_unique<FrostMageRefactored>(bot);
    }
}

// ============================================================================
// WARLOCK SPECIALIZATIONS (3)
// ============================================================================

::std::unique_ptr<BotAI> SpecializedAIFactory::CreateWarlockAI(Player* bot, uint8 specId)
{
    switch (specId)
    {
        case 0: // Affliction
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating AfflictionWarlockRefactored for {}", bot->GetName());
            return ::std::make_unique<AfflictionWarlockRefactored>(bot);

        case 1: // Demonology
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating DemonologyWarlockRefactored for {}", bot->GetName());
            return ::std::make_unique<DemonologyWarlockRefactored>(bot);

        case 2: // Destruction
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating DestructionWarlockRefactored for {}", bot->GetName());
            return ::std::make_unique<DestructionWarlockRefactored>(bot);

        default:
            TC_LOG_WARN("module.playerbot.ai.factory", "Unknown Warlock spec {}, defaulting to Affliction", specId);
            return ::std::make_unique<AfflictionWarlockRefactored>(bot);
    }
}

// ============================================================================
// MONK SPECIALIZATIONS (3)
// ============================================================================

::std::unique_ptr<BotAI> SpecializedAIFactory::CreateMonkAI(Player* bot, uint8 specId)
{
    switch (specId)
    {
        case 0: // Brewmaster
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating BrewmasterMonkRefactored for {}", bot->GetName());
            return ::std::make_unique<BrewmasterMonkRefactored>(bot);

        case 1: // Mistweaver
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating MistweaverMonkRefactored for {}", bot->GetName());
            return ::std::make_unique<MistweaverMonkRefactored>(bot);

        case 2: // Windwalker
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating WindwalkerMonkRefactored for {}", bot->GetName());
            return ::std::make_unique<WindwalkerMonkRefactored>(bot);

        default:
            TC_LOG_WARN("module.playerbot.ai.factory", "Unknown Monk spec {}, defaulting to Windwalker", specId);
            return ::std::make_unique<WindwalkerMonkRefactored>(bot);
    }
}

// ============================================================================
// DRUID SPECIALIZATIONS (4)
// ============================================================================

::std::unique_ptr<BotAI> SpecializedAIFactory::CreateDruidAI(Player* bot, uint8 specId)
{
    switch (specId)
    {
        case 0: // Balance
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating BalanceDruidRefactored for {}", bot->GetName());
            return ::std::make_unique<BalanceDruidRefactored>(bot);

        case 1: // Feral
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating FeralDruidRefactored for {}", bot->GetName());
            return ::std::make_unique<FeralDruidRefactored>(bot);

        case 2: // Guardian
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating GuardianDruidRefactored for {}", bot->GetName());
            return ::std::make_unique<GuardianDruidRefactored>(bot);

        case 3: // Restoration
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating RestorationDruidRefactored for {}", bot->GetName());
            return ::std::make_unique<RestorationDruidRefactored>(bot);

        default:
            TC_LOG_WARN("module.playerbot.ai.factory", "Unknown Druid spec {}, defaulting to Balance", specId);
            return ::std::make_unique<BalanceDruidRefactored>(bot);
    }
}

// ============================================================================
// DEMON HUNTER SPECIALIZATIONS (2)
// ============================================================================

::std::unique_ptr<BotAI> SpecializedAIFactory::CreateDemonHunterAI(Player* bot, uint8 specId)
{
    switch (specId)
    {
        case 0: // Havoc
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating HavocDemonHunterRefactored for {}", bot->GetName());
            return ::std::make_unique<HavocDemonHunterRefactored>(bot);

        case 1: // Vengeance
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating VengeanceDemonHunterRefactored for {}", bot->GetName());
            return ::std::make_unique<VengeanceDemonHunterRefactored>(bot);

        default:
            TC_LOG_WARN("module.playerbot.ai.factory", "Unknown Demon Hunter spec {}, defaulting to Havoc", specId);
            return ::std::make_unique<HavocDemonHunterRefactored>(bot);
    }
}

// ============================================================================
// EVOKER SPECIALIZATIONS (3)
// ============================================================================

::std::unique_ptr<BotAI> SpecializedAIFactory::CreateEvokerAI(Player* bot, uint8 specId)
{
    switch (specId)
    {
        case 0: // Devastation
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating DevastationEvokerRefactored for {}", bot->GetName());
            return ::std::make_unique<DevastationEvokerRefactored>(bot);

        case 1: // Preservation
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating PreservationEvokerRefactored for {}", bot->GetName());
            return ::std::make_unique<PreservationEvokerRefactored>(bot);

        case 2: // Augmentation
            TC_LOG_INFO("module.playerbot.ai.factory", "Creating AugmentationEvokerRefactored for {}", bot->GetName());
            return ::std::make_unique<AugmentationEvokerRefactored>(bot);

        default:
            TC_LOG_WARN("module.playerbot.ai.factory", "Unknown Evoker spec {}, defaulting to Devastation", specId);
            return ::std::make_unique<DevastationEvokerRefactored>(bot);
    }
}

} // namespace Playerbot
