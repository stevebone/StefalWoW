/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Template-Based Combat Specialization Implementation
 *
 * This file contains explicit template instantiations and common
 * implementation code for the combat specialization templates.
 */

#include "CombatSpecializationTemplates.h"
#include "ResourceTypes.h"
#include "Player.h"
#include "SpellMgr.h"
#include "ObjectAccessor.h"
#include "Group.h"
#include "Log.h"
#include "GameTime.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5F: Thread-safe queries

namespace Playerbot
{

// ============================================================================
// EXPLICIT TEMPLATE INSTANTIATIONS
// ============================================================================

// This ensures templates are compiled for all resource types we use
// Reduces compilation time in other translation units

// Simple resource instantiations
template class CombatSpecializationTemplate<uint32>; // Mana/Rage/Energy/Focus

// Complex resource instantiations
template class CombatSpecializationTemplate<RuneSystem>;
template class CombatSpecializationTemplate<ComboPointSystem>;
template class CombatSpecializationTemplate<HolyPowerSystem>;
template class CombatSpecializationTemplate<ChiSystem>;
template class CombatSpecializationTemplate<SoulShardSystem>;

// Role-based instantiations for common resource types
template class MeleeDpsSpecialization<uint32>;
template class MeleeDpsSpecialization<RuneSystem>;
template class MeleeDpsSpecialization<ComboPointSystem>;

template class RangedDpsSpecialization<uint32>;
template class RangedDpsSpecialization<SoulShardSystem>;

template class TankSpecialization<uint32>;
template class TankSpecialization<RuneSystem>;

template class HealerSpecialization<uint32>;
template class HealerSpecialization<ChiSystem>;

// ============================================================================
// SPECIALIZATION FACTORY
// ============================================================================

/**
 * Factory for creating appropriate specialization based on class and spec
 */
::std::unique_ptr<ClassAI> CombatSpecializationFactory::CreateSpecialization(
    Player* bot, Classes botClass, uint32 specId)
{
    switch (botClass)
    {
        case CLASS_WARRIOR:
        {
            switch (specId)
            {
                case SPEC_WARRIOR_ARMS:
                case SPEC_WARRIOR_FURY:
                    return ::std::make_unique<MeleeDpsSpecialization<uint32>>(bot);
                case SPEC_WARRIOR_PROTECTION:
                    return ::std::make_unique<TankSpecialization<uint32>>(bot);
            }
            break;
        }

        case CLASS_PALADIN:
        {
            switch (specId)
            {
                case SPEC_PALADIN_HOLY:
                    return ::std::make_unique<HealerSpecialization<uint32>>(bot);
                case SPEC_PALADIN_PROTECTION:
                    return ::std::make_unique<TankSpecialization<uint32>>(bot);
                case SPEC_PALADIN_RETRIBUTION:
                    return ::std::make_unique<MeleeDpsSpecialization<uint32>>(bot);
            }
            break;
        }

        case CLASS_HUNTER:
        {
            // All hunter specs are ranged DPS with Focus
            return ::std::make_unique<RangedDpsSpecialization<uint32>>(bot);
        }

        case CLASS_ROGUE:
        {
            // All rogue specs are melee DPS with Energy/Combo Points
            return ::std::make_unique<MeleeDpsSpecialization<ComboPointSystem>>(bot);
        }

        case CLASS_PRIEST:
        {
            switch (specId)
            {
                case SPEC_PRIEST_DISCIPLINE:
                    // Discipline is hybrid DPS/Healer
                    return ::std::make_unique<HybridDpsHealerSpecialization<uint32>>(bot);
                case SPEC_PRIEST_HOLY:
                    return ::std::make_unique<HealerSpecialization<uint32>>(bot);
                case SPEC_PRIEST_SHADOW:
                    return ::std::make_unique<RangedDpsSpecialization<uint32>>(bot);
            }
            break;
        }

        case CLASS_DEATH_KNIGHT:
        {
            switch (specId)
            {
                case SPEC_DEATH_KNIGHT_BLOOD:
                    return ::std::make_unique<TankSpecialization<RuneSystem>>(bot);
                case SPEC_DEATH_KNIGHT_FROST:
                case SPEC_DEATH_KNIGHT_UNHOLY:
                    return ::std::make_unique<MeleeDpsSpecialization<RuneSystem>>(bot);
            }
            break;
        }

        case CLASS_SHAMAN:
        {
            switch (specId)
            {
                case SPEC_SHAMAN_ELEMENTAL:
                    return ::std::make_unique<RangedDpsSpecialization<uint32>>(bot);
                case SPEC_SHAMAN_ENHANCEMENT:
                    return ::std::make_unique<MeleeDpsSpecialization<uint32>>(bot);
                case SPEC_SHAMAN_RESTORATION:
                    return ::std::make_unique<HealerSpecialization<uint32>>(bot);
            }
            break;
        }

        case CLASS_MAGE:
        {
            // All mage specs are ranged DPS with Mana
            return ::std::make_unique<RangedDpsSpecialization<uint32>>(bot);
        }

        case CLASS_WARLOCK:
        {
            // All warlock specs use soul shards
            return ::std::make_unique<RangedDpsSpecialization<SoulShardSystem>>(bot);
        }

        case CLASS_MONK:
        {
            switch (specId)
            {
                case SPEC_MONK_BREWMASTER:
                    return ::std::make_unique<AvoidanceTankSpecialization<ChiSystem>>(bot);
                case SPEC_MONK_MISTWEAVER:
                    return ::std::make_unique<HealerSpecialization<ChiSystem>>(bot);
                case SPEC_MONK_WINDWALKER:
                    return ::std::make_unique<MeleeDpsSpecialization<ChiSystem>>(bot);
            }
            break;
        }

        case CLASS_DRUID:
        {
            switch (specId)
            {
                case SPEC_DRUID_BALANCE:
                    return ::std::make_unique<RangedDpsSpecialization<uint32>>(bot);
                case SPEC_DRUID_FERAL:
                    return ::std::make_unique<MeleeDpsSpecialization<ComboPointSystem>>(bot);
                case SPEC_DRUID_GUARDIAN:
                    return ::std::make_unique<TankSpecialization<uint32>>(bot);
                case SPEC_DRUID_RESTORATION:
                    return ::std::make_unique<HealerSpecialization<uint32>>(bot);
            }
            break;
        }

        case CLASS_DEMON_HUNTER:
        {
            switch (specId)
            {
                case SPEC_DEMON_HUNTER_HAVOC:
                    return ::std::make_unique<MeleeDpsSpecialization<uint32>>(bot); // Fury resource
                case SPEC_DEMON_HUNTER_VENGEANCE:
                    return ::std::make_unique<TankSpecialization<uint32>>(bot); // Pain resource
            }
            break;
        }
    }

    // Fallback to basic template
    LOG_WARN("module.playerbot", "No specific template for class {} spec {}, using default",
             botClass, specId);
    return ::std::make_unique<CombatSpecializationTemplate<uint32>>(bot);
}

// ============================================================================
// PERFORMANCE MONITORING
// ============================================================================

/**
 * Global performance monitor for template system
 */
class TemplatePerformanceMonitor
{
public:
    static TemplatePerformanceMonitor& Instance()
    {
        static TemplatePerformanceMonitor instance;
        return instance;
    }

    void RecordUpdate(uint32 botGuid, uint32 updateTimeUs)
    {
        ::std::lock_guard lock(_mutex);

        auto& stats = _botStats[botGuid];
        stats.totalUpdates++;
        stats.totalTimeUs += updateTimeUs;
        stats.maxTimeUs = ::std::max(stats.maxTimeUs, updateTimeUs);
        stats.minTimeUs = ::std::min(stats.minTimeUs, updateTimeUs);
    }

    void RecordTemplateInstantiation(const ::std::string& templateType)
    {
        ::std::lock_guard lock(_mutex);
        _templateInstantiations[templateType]++;
    }

    void PrintStatistics()
    {
        ::std::lock_guard lock(_mutex);

        LOG_INFO("module.playerbot", "=== Template Performance Statistics ===");

        // Overall statistics
        uint64 totalUpdates = 0;
        uint64 totalTimeUs = 0;

        for (const auto& [guid, stats] : _botStats)
        {
            totalUpdates += stats.totalUpdates;
            totalTimeUs += stats.totalTimeUs;
        }

        if (totalUpdates > 0)
        {
            LOG_INFO("module.playerbot", "Total Updates: {}", totalUpdates);
            LOG_INFO("module.playerbot", "Average Update Time: {} us",
                     totalTimeUs / totalUpdates);
        }

        // Template instantiation counts
        LOG_INFO("module.playerbot", "Template Instantiations:");
        for (const auto& [type, count] : _templateInstantiations)
        {
            LOG_INFO("module.playerbot", "  {}: {}", type, count);
        }

        // Memory usage estimation
        size_t estimatedMemory = _botStats.size() * sizeof(CombatSpecializationTemplate<uint32>);
        LOG_INFO("module.playerbot", "Estimated Memory Usage: {} KB",
                 estimatedMemory / 1024);
    }

    void Reset()
    {
        ::std::lock_guard lock(_mutex);
        _botStats.clear();
        _templateInstantiations.clear();
    }

private:
    struct BotStatistics
    {
        uint32 totalUpdates = 0;
        uint64 totalTimeUs = 0;
        uint32 maxTimeUs = 0;
        uint32 minTimeUs = UINT32_MAX;
    };

    ::std::recursive_mutex _mutex;
    ::std::unordered_map<uint32, BotStatistics> _botStats;
    ::std::unordered_map<::std::string, uint32> _templateInstantiations;
};

// ============================================================================
// TEMPLATE VALIDATION
// ============================================================================

/**
 * Compile-time validation of template requirements
 */
template<typename T>
constexpr bool ValidateResourceType()
{
    if constexpr (SimpleResource<T>)
    {
        static_assert(sizeof(T) <= 4, "Simple resource must be 4 bytes or less");
        static_assert(::std::is_arithmetic_v<T>, "Simple resource must be arithmetic");
        return true;
    }
    else if constexpr (ComplexResource<T>)
    {
        // Complex resources are validated through concepts
        return true;
    }
    else
    {
        static_assert(ValidResource<T>, "Type does not satisfy resource requirements");
        return false;
    }
}

// Validate all our resource types at compile time
static_assert(ValidateResourceType<uint32>());
static_assert(ValidateResourceType<RuneSystem>());
static_assert(ValidateResourceType<ComboPointSystem>());
static_assert(ValidateResourceType<HolyPowerSystem>());
static_assert(ValidateResourceType<ChiSystem>());
static_assert(ValidateResourceType<SoulShardSystem>());

// ============================================================================
// Specialization Combat Templates — Packet-Based Spell Casting Helpers
// ============================================================================

/**
 * Helper to migrate from old specialization to template-based
 */
class SpecializationMigrator
{
public:
    static bool ShouldUseTemplate(Classes botClass, uint32 specId)
    {
        // Check configuration for which specs to use templates
        // This allows gradual migration

        // For now, enable templates for specific specs that are complete
    switch (botClass)
        {
            case CLASS_PALADIN:
                return specId == SPEC_PALADIN_RETRIBUTION; // Migrated as example

            case CLASS_DEATH_KNIGHT:
                return true; // All DK specs migrated

            case CLASS_ROGUE:
                return true; // All rogue specs migrated

            default:
                return false; // Others still using old system
        }
    }

    static void MigrateSpecialization(Player* bot, ClassAI* oldSpec, ClassAI* newSpec)
    {
        // Copy relevant state from old to new
        // This allows hot-swapping during runtime

        // Note: Most state is rebuilt, but we can preserve some things
        // like target selection, combat timers, etc.

        LOG_DEBUG("module.playerbot", "Migrated {} from old to template specialization",
                  bot->GetName());
    }
};

// ============================================================================
// DEBUGGING AND DIAGNOSTICS
// ============================================================================

/**
 * Template diagnostics for development
 */
class TemplateDiagnostics
{
public:
    template<typename T>
    static void PrintResourceInfo()
    {
        LOG_DEBUG("module.playerbot", "Resource Type Information:");
        LOG_DEBUG("module.playerbot", "  Is Simple: {}", ResourceTraits<T>::is_simple);
        LOG_DEBUG("module.playerbot", "  Is Complex: {}", ResourceTraits<T>::is_complex);
        LOG_DEBUG("module.playerbot", "  Regenerates: {}", ResourceTraits<T>::regenerates);
        LOG_DEBUG("module.playerbot", "  Regen Rate: {} ms", ResourceTraits<T>::regen_rate_ms);
        LOG_DEBUG("module.playerbot", "  Critical Threshold: {}", ResourceTraits<T>::critical_threshold);
        LOG_DEBUG("module.playerbot", "  Name: {}", ResourceTraits<T>::name);
    }

    static void PrintAllResourceInfo()
    {
        PrintResourceInfo<uint32>();
        PrintResourceInfo<RuneSystem>();
        PrintResourceInfo<ComboPointSystem>();
        PrintResourceInfo<HolyPowerSystem>();
        PrintResourceInfo<ChiSystem>();
        PrintResourceInfo<SoulShardSystem>();
    }

    template<typename T>
    static size_t GetTemplateSize()
    {
        return sizeof(CombatSpecializationTemplate<T>);
    }

    static void PrintMemoryUsage()
    {
        LOG_DEBUG("module.playerbot", "Template Memory Usage:");
        LOG_DEBUG("module.playerbot", "  Base<uint32>: {} bytes", GetTemplateSize<uint32>());
        LOG_DEBUG("module.playerbot", "  Base<RuneSystem>: {} bytes", GetTemplateSize<RuneSystem>());
        LOG_DEBUG("module.playerbot", "  MeleeDps<uint32>: {} bytes", sizeof(MeleeDpsSpecialization<uint32>));
        LOG_DEBUG("module.playerbot", "  Tank<RuneSystem>: {} bytes", sizeof(TankSpecialization<RuneSystem>));
    }
};

} // namespace Playerbot