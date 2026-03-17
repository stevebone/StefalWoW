/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#include "BotAI.h"
#include "Player.h"
#include "Log.h"
#include "ClassAI/SpecializedAIFactory.h"

// Legacy class-specific AI headers (kept for fallback)
#include "ClassAI/Warriors/WarriorAI.h"
#include "ClassAI/Paladins/PaladinAI.h"
#include "ClassAI/Hunters/HunterAI.h"
#include "ClassAI/Rogues/RogueAI.h"
#include "ClassAI/Priests/PriestAI.h"
#include "ClassAI/Shamans/ShamanAI.h"
#include "ClassAI/Mages/MageAI.h"
#include "ClassAI/Warlocks/WarlockAI.h"
#include "ClassAI/Monks/MonkAI.h"
#include "ClassAI/Druids/DruidAI.h"
#include "ClassAI/DemonHunters/DemonHunterAI.h"
#include "ClassAI/DeathKnights/DeathKnightAI.h"
#include "ClassAI/Evokers/EvokerAI.h"

namespace Playerbot {

BotAIFactory* BotAIFactory::instance()
{
    static BotAIFactory _instance;
    return &_instance;
}

::std::unique_ptr<BotAI> BotAIFactory::CreateAI(Player* bot)
{

    // Note: Use GetGUID().GetCounter() instead of GetName() - Player name may not be loaded yet during login
    TC_LOG_DEBUG("module.playerbot.ai", "Creating specialized AI for bot GUID {} (class: {})",
                 bot->GetGUID().GetCounter(), bot->GetClass());

    // Use SpecializedAIFactory to create spec-specific Refactored AI
    ::std::unique_ptr<BotAI> specializedAI = SpecializedAIFactory::CreateSpecializedAI(bot);

    if (specializedAI)
    {
        TC_LOG_INFO("module.playerbot.ai", "Successfully created specialized AI for bot GUID {}", bot->GetGUID().GetCounter());
        return specializedAI;
    }

    // Fallback to legacy class-based AI if specialized creation fails
    TC_LOG_WARN("module.playerbot.ai", "Specialized AI creation failed, falling back to legacy AI for bot GUID {}", bot->GetGUID().GetCounter());
    return CreateClassAI(bot, bot->GetClass());
}

::std::unique_ptr<BotAI> BotAIFactory::CreateClassAI(Player* bot, uint8 classId)
{

    try
    {
        ::std::unique_ptr<BotAI> botAI;

        switch (classId)
        {
            case CLASS_WARRIOR:
                botAI = ::std::make_unique<WarriorAI>(bot);
                TC_LOG_INFO("module.playerbot.ai", "Created WarriorAI for bot GUID {}", bot->GetGUID().GetCounter());
                break;

            case CLASS_PALADIN:
                botAI = ::std::make_unique<PaladinAI>(bot);
                TC_LOG_INFO("module.playerbot.ai", "Created PaladinAI for bot GUID {}", bot->GetGUID().GetCounter());
                break;

            case CLASS_HUNTER:
                botAI = ::std::make_unique<HunterAI>(bot);
                TC_LOG_INFO("module.playerbot.ai", "Created HunterAI for bot GUID {}", bot->GetGUID().GetCounter());
                break;

            case CLASS_ROGUE:
                botAI = ::std::make_unique<RogueAI>(bot);
                TC_LOG_INFO("module.playerbot.ai", "Created RogueAI for bot GUID {}", bot->GetGUID().GetCounter());
                break;

            case CLASS_PRIEST:
                botAI = ::std::make_unique<PriestAI>(bot);
                TC_LOG_INFO("module.playerbot.ai", "Created PriestAI for bot GUID {}", bot->GetGUID().GetCounter());
                break;

            case CLASS_SHAMAN:
                botAI = ::std::make_unique<ShamanAI>(bot);
                TC_LOG_INFO("module.playerbot.ai", "Created ShamanAI for bot GUID {}", bot->GetGUID().GetCounter());
                break;

            case CLASS_MAGE:
                botAI = ::std::make_unique<MageAI>(bot);
                TC_LOG_INFO("module.playerbot.ai", "Created MageAI for bot GUID {}", bot->GetGUID().GetCounter());
                break;

            case CLASS_WARLOCK:
                botAI = ::std::make_unique<WarlockAI>(bot);
                TC_LOG_INFO("module.playerbot.ai", "Created WarlockAI for bot GUID {}", bot->GetGUID().GetCounter());
                break;

            case CLASS_MONK:
                botAI = ::std::make_unique<MonkAI>(bot);
                TC_LOG_INFO("module.playerbot.ai", "Created MonkAI for bot GUID {}", bot->GetGUID().GetCounter());
                break;

            case CLASS_DRUID:
                botAI = ::std::make_unique<DruidAI>(bot);
                TC_LOG_INFO("module.playerbot.ai", "Created DruidAI for bot GUID {}", bot->GetGUID().GetCounter());
                break;

            case CLASS_DEMON_HUNTER:
                botAI = ::std::make_unique<DemonHunterAI>(bot);
                TC_LOG_INFO("module.playerbot.ai", "Created DemonHunterAI for bot GUID {}", bot->GetGUID().GetCounter());
                break;

            case CLASS_DEATH_KNIGHT:
                botAI = ::std::make_unique<DeathKnightAI>(bot);
                TC_LOG_INFO("module.playerbot.ai", "Created DeathKnightAI for bot GUID {}", bot->GetGUID().GetCounter());
                break;

            case CLASS_EVOKER:
                botAI = ::std::make_unique<EvokerAI>(bot);
                TC_LOG_INFO("module.playerbot.ai", "Created EvokerAI for bot GUID {}", bot->GetGUID().GetCounter());
                break;

            default:
                TC_LOG_WARN("module.playerbot.ai", "Unknown class {} for bot GUID {}, creating DefaultBotAI",
                           classId, bot->GetGUID().GetCounter());
                botAI = ::std::make_unique<DefaultBotAI>(bot);
                break;
        }

        return botAI;
    }
    catch (const ::std::exception& e)
    {
        TC_LOG_ERROR("module.playerbot.ai", "Exception creating class AI for bot GUID {}: {}",
                     bot->GetGUID().GetCounter(), e.what());
        TC_LOG_WARN("module.playerbot.ai", "Falling back to DefaultBotAI for bot GUID {}", bot->GetGUID().GetCounter());
        return ::std::make_unique<DefaultBotAI>(bot);
    }
    catch (...)
    {
        TC_LOG_ERROR("module.playerbot.ai", "Unknown exception creating class AI for bot GUID {}",
                     bot->GetGUID().GetCounter());
        TC_LOG_WARN("module.playerbot.ai", "Falling back to DefaultBotAI for bot GUID {}", bot->GetGUID().GetCounter());
        return ::std::make_unique<DefaultBotAI>(bot);
    }
}

::std::unique_ptr<BotAI> BotAIFactory::CreateClassAI(Player* bot, uint8 classId, uint8 /*spec*/)
{
    // For now, create based on class and let the class AI handle specialization internally
    return CreateClassAI(bot, classId);
}

::std::unique_ptr<BotAI> BotAIFactory::CreateSpecializedAI(Player* bot, ::std::string const& /*type*/)
{

    // Implement specialized AI types as needed
    return CreateClassAI(bot, bot->GetClass());
}

::std::unique_ptr<BotAI> BotAIFactory::CreatePvPAI(Player* bot)
{

    // For now, use class-specific AI with PvP strategies
    return CreateClassAI(bot, bot->GetClass());
}

::std::unique_ptr<BotAI> BotAIFactory::CreatePvEAI(Player* bot)
{

    // For now, use class-specific AI with PvE strategies
    return CreateClassAI(bot, bot->GetClass());
}

::std::unique_ptr<BotAI> BotAIFactory::CreateRaidAI(Player* bot)
{

    // For now, use class-specific AI with raid strategies
    return CreateClassAI(bot, bot->GetClass());
}

void BotAIFactory::InitializeDefaultTriggers(BotAI* ai)
{
    if (!ai)
        return;

    // Initialize default triggers for basic bot automation
    // This will be expanded to register specific triggers for different behaviors
    TC_LOG_DEBUG("module.playerbot.ai", "Initialized default triggers for bot");
}


void BotAIFactory::RegisterAICreator(::std::string const& type,
                                     ::std::function<::std::unique_ptr<BotAI>(Player*)> creator)
{
    if (type.empty() || !creator)
    {
        TC_LOG_ERROR("module.playerbot.ai", "BotAIFactory::RegisterAICreator: Invalid type or creator");
        return;
    }

    auto result = _creators.emplace(type, ::std::move(creator));
    if (result.second)
    {
        TC_LOG_INFO("module.playerbot.ai", "Registered AI creator for type: {}", type);
    }
    else
    {
        TC_LOG_WARN("module.playerbot.ai", "AI creator for type '{}' already registered, replacing", type);
        _creators[type] = ::std::move(creator);
    }
}

void BotAIFactory::InitializeDefaultValues(BotAI* ai)
{
    if (!ai)
        return;

    // Initialize default AI configuration values
    // This sets up reasonable defaults for bot behavior

    // Initialize default strategies based on class
    if (Player* bot = ai->GetBot())
    {
        InitializeDefaultStrategies(ai);
        InitializeClassStrategies(ai, bot->GetClass(), static_cast<uint8>(bot->GetPrimarySpecialization()));
    }

    TC_LOG_DEBUG("module.playerbot.ai", "Initialized default values for bot AI");
}

void BotAIFactory::InitializeDefaultStrategies(BotAI* ai)
{
    if (!ai)
        return;

    // Set up generic strategies applicable to all bots
    // Combat, movement, targeting defaults
    TC_LOG_DEBUG("module.playerbot.ai", "Initialized default strategies for bot");
}

void BotAIFactory::InitializeClassStrategies(BotAI* ai, uint8 classId, uint8 spec)
{
    if (!ai)
        return;

    // Set up class-specific strategies based on class and specialization
    TC_LOG_DEBUG("module.playerbot.ai", "Initialized class strategies for class {} spec {}", classId, spec);
}

} // namespace Playerbot
