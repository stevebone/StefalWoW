/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "TrainerInteractionManager.h"
#include "Player.h"
#include "Creature.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "ObjectMgr.h"
#include "Trainer.h"
#include "Log.h"
#include "SharedDefines.h"
#include <chrono>
#include <algorithm>

namespace Playerbot
{

// Budget allocation percentages
static constexpr float BUDGET_ESSENTIAL_PERCENT = 0.60f;  // 60% for essential spells
static constexpr float BUDGET_COMBAT_PERCENT    = 0.25f;  // 25% for combat utilities
static constexpr float BUDGET_UTILITY_PERCENT   = 0.10f;  // 10% for utility spells
static constexpr float BUDGET_DISCRETIONARY_PERCENT = 0.05f; // 5% for convenience

TrainerInteractionManager::TrainerInteractionManager(Player* bot)
    : m_bot(bot)
    , m_cpuUsage(0.0f)
    , m_totalTrainingTime(0)
    , m_trainingDecisionCount(0)
    , m_essentialCacheInitialized(false)
{
    if (m_bot)
    {
        InitializeEssentialSpellsCache();
    }
}

// ============================================================================
// Core Training Methods
// ============================================================================

bool TrainerInteractionManager::LearnSpell(Creature* trainer, uint32 spellId)
{
    if (!trainer || !m_bot || spellId == 0)
        return false;

    auto startTime = ::std::chrono::high_resolution_clock::now();

    // Check if can learn
    if (!CanLearnSpell(trainer, spellId))
    {
        TC_LOG_DEBUG("bot.playerbot", "TrainerInteractionManager: Bot %s cannot learn spell %u",
            m_bot->GetName().c_str(), spellId);
        m_stats.trainingFailures++;
        return false;
    }

    // Get trainer spell info
    Trainer::Trainer const* trainerData = sObjectMgr->GetTrainer(trainer->GetEntry());
    if (!trainerData)
    {
        m_stats.trainingFailures++;
        return false;
    }

    // Find the spell in trainer's list
    Trainer::Spell const* trainerSpell = nullptr;
    for (Trainer::Spell const& spell : trainerData->GetSpells())
    {
        if (spell.SpellId == spellId)
        {
            trainerSpell = &spell;
            break;
        }
    }

    if (!trainerSpell)
    {
        TC_LOG_DEBUG("bot.playerbot", "TrainerInteractionManager: Spell %u not found in trainer %u list",
            spellId, trainer->GetEntry());
        m_stats.trainingFailures++;
        return false;
    }

    // Check gold cost
    uint32 cost = trainerSpell->MoneyCost;
    if (!CanAffordTraining(cost))
    {
        TC_LOG_DEBUG("bot.playerbot", "TrainerInteractionManager: Bot %s cannot afford spell %u (cost: %u)",
            m_bot->GetName().c_str(), spellId, cost);
        m_stats.insufficientGold++;
        m_stats.trainingFailures++;
        return false;
    }

    // Execute training
    bool success = ExecuteTraining(trainer, spellId, cost);
    RecordTraining(spellId, cost, success);

    // Track performance
    auto endTime = ::std::chrono::high_resolution_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);
    m_totalTrainingTime += static_cast<uint32>(duration.count());
    m_trainingDecisionCount++;
    m_cpuUsage = m_trainingDecisionCount > 0 ? (float)m_totalTrainingTime / m_trainingDecisionCount / 1000.0f : 0.0f;

    return success;
}

uint32 TrainerInteractionManager::LearnAllAvailableSpells(Creature* trainer)
{
    if (!trainer || !m_bot)
        return 0;

    m_stats.trainingSessions++;

    // Get trainer data
    Trainer::Trainer const* trainerData = sObjectMgr->GetTrainer(trainer->GetEntry());
    if (!trainerData)
        return 0;

    // Calculate budget
    TrainingBudget budget = CalculateBudget();

    // Evaluate all spells
    ::std::vector<SpellEvaluation> evaluations;
    for (Trainer::Spell const& spell : trainerData->GetSpells())
    {
        SpellEvaluation eval = EvaluateTrainerSpell(trainer, &spell);
        if (eval.canLearn && !eval.alreadyKnown)
            evaluations.push_back(eval);
    }

    // Sort by priority (highest priority first)
    ::std::sort(evaluations.begin(), evaluations.end(),
        [](SpellEvaluation const& a, SpellEvaluation const& b)
        {
            return static_cast<uint8>(a.priority) < static_cast<uint8>(b.priority);
        });

    // Learn spells in priority order within budget
    uint32 learnedCount = 0;
    for (SpellEvaluation const& eval : evaluations)
    {
        if (!FitsWithinBudget(eval.cost, eval.priority, budget))
        {
            TC_LOG_DEBUG("bot.playerbot", "TrainerInteractionManager: Spell %u doesn't fit budget (priority: %u, cost: %u)",
                eval.spellId, static_cast<uint8>(eval.priority), eval.cost);
            continue;
        }

        if (LearnSpell(trainer, eval.spellId))
        {
            learnedCount++;

            // Deduct from appropriate budget category
            switch (eval.priority)
            {
                case TrainingPriority::ESSENTIAL:
                    budget.reservedForEssential -= eval.cost;
                    break;
                case TrainingPriority::HIGH:
                    budget.reservedForCombat -= eval.cost;
                    break;
                case TrainingPriority::MEDIUM:
                    budget.reservedForUtility -= eval.cost;
                    break;
                case TrainingPriority::LOW:
                    budget.discretionary -= eval.cost;
                    break;
            }
        }
    }

    TC_LOG_DEBUG("bot.playerbot", "TrainerInteractionManager: Bot %s learned %u spells from trainer %u",
        m_bot->GetName().c_str(), learnedCount, trainer->GetEntry());

    return learnedCount;
}

uint32 TrainerInteractionManager::SmartTrain(Creature* trainer)
{
    if (!trainer || !m_bot)
        return 0;

    // First check for missing essential spells
    ::std::vector<uint32> missingEssential = GetMissingEssentialSpells();
    if (!missingEssential.empty())
    {
        TC_LOG_DEBUG("bot.playerbot", "TrainerInteractionManager: Bot %s has %zu missing essential spells",
            m_bot->GetName().c_str(), missingEssential.size());
    }

    // Learn all available spells (LearnAllAvailableSpells already prioritizes)
    return LearnAllAvailableSpells(trainer);
}

bool TrainerInteractionManager::LearnProfession(Creature* trainer, uint32 professionId)
{
    if (!trainer || !m_bot || professionId == 0)
        return false;

    // Get the profession spell
    // Different professions have different learning spells
    // This would need to be expanded with actual profession spell IDs
    uint32 professionSpellId = 0;

    switch (professionId)
    {
        case SKILL_ALCHEMY:
            professionSpellId = 2259;  // Alchemy
            break;
        case SKILL_BLACKSMITHING:
            professionSpellId = 2018;  // Blacksmithing
            break;
        case SKILL_ENCHANTING:
            professionSpellId = 7411;  // Enchanting
            break;
        case SKILL_ENGINEERING:
            professionSpellId = 4036;  // Engineering
            break;
        case SKILL_HERBALISM:
            professionSpellId = 2366;  // Herbalism
            break;
        case SKILL_INSCRIPTION:
            professionSpellId = 45357; // Inscription
            break;
        case SKILL_JEWELCRAFTING:
            professionSpellId = 25229; // Jewelcrafting
            break;
        case SKILL_LEATHERWORKING:
            professionSpellId = 2108;  // Leatherworking
            break;
        case SKILL_MINING:
            professionSpellId = 2575;  // Mining
            break;
        case SKILL_SKINNING:
            professionSpellId = 8613;  // Skinning
            break;
        case SKILL_TAILORING:
            professionSpellId = 3908;  // Tailoring
            break;
        case SKILL_COOKING:
            professionSpellId = 2550;  // Cooking
            break;
        case SKILL_FIRST_AID:
            professionSpellId = 3273;  // First Aid
            break;
        case SKILL_FISHING:
            professionSpellId = 7620;  // Fishing
            break;
        default:
            TC_LOG_DEBUG("bot.playerbot", "TrainerInteractionManager: Unknown profession ID %u", professionId);
            return false;
    }

    if (m_bot->HasSpell(professionSpellId))
    {
        TC_LOG_DEBUG("bot.playerbot", "TrainerInteractionManager: Bot %s already has profession spell %u",
            m_bot->GetName().c_str(), professionSpellId);
        return false;
    }

    return LearnSpell(trainer, professionSpellId);
}

bool TrainerInteractionManager::UnlearnProfession(Creature* trainer, uint32 professionId)
{
    if (!trainer || !m_bot || professionId == 0)
        return false;

    // Unlearning professions requires special handling
    // This typically needs to be done via a specific unlearn spell or NPC
    TC_LOG_DEBUG("bot.playerbot", "TrainerInteractionManager: Unlearn profession not fully implemented");
    return false;
}

// ============================================================================
// Trainer Analysis Methods
// ============================================================================

::std::vector<TrainerSpell const*> TrainerInteractionManager::GetTrainerSpells(Creature* trainer) const
{
    ::std::vector<TrainerSpell const*> spells;

    if (!trainer)
        return spells;

    Trainer::Trainer const* trainerData = sObjectMgr->GetTrainer(trainer->GetEntry());
    if (!trainerData)
        return spells;

    // Note: TrainerSpell is a different struct from Trainer::Spell
    // We return an empty vector since the types don't match
    // The caller should use Trainer::Trainer directly
    return spells;
}

TrainerInteractionManager::SpellEvaluation TrainerInteractionManager::EvaluateTrainerSpell(
    Creature* trainer, TrainerSpell const* trainerSpell) const
{
    SpellEvaluation eval;

    if (!trainer || !trainerSpell || !m_bot)
        return eval;

    // This overload handles the old TrainerSpell struct
    // For Trainer::Spell, use the other methods
    return eval;
}

TrainerInteractionManager::SpellEvaluation TrainerInteractionManager::EvaluateTrainerSpell(
    Creature* trainer, Trainer::Spell const* trainerSpell) const
{
    SpellEvaluation eval;

    if (!trainer || !trainerSpell || !m_bot)
        return eval;

    eval.spellId = trainerSpell->SpellId;
    eval.cost = trainerSpell->MoneyCost;
    eval.requiredLevel = trainerSpell->ReqLevel;
    eval.requiredSkill = trainerSpell->ReqSkillLine;
    eval.requiredSkillValue = trainerSpell->ReqSkillRank;

    // Check if already known
    eval.alreadyKnown = m_bot->HasSpell(trainerSpell->SpellId);

    // Check if can learn
    Trainer::Trainer const* trainerData = sObjectMgr->GetTrainer(trainer->GetEntry());
    if (trainerData)
    {
        eval.canLearn = (trainerData->GetSpellState(m_bot, trainerSpell) == Trainer::SpellState::Available);
    }

    // Calculate priority
    eval.priority = CalculateSpellPriority(trainerSpell->SpellId);

    // Generate reason
    switch (eval.priority)
    {
        case TrainingPriority::ESSENTIAL:
            eval.reason = "Essential class ability";
            break;
        case TrainingPriority::HIGH:
            eval.reason = "Important combat utility";
            break;
        case TrainingPriority::MEDIUM:
            eval.reason = "Useful utility spell";
            break;
        case TrainingPriority::LOW:
            eval.reason = "Convenience/optional";
            break;
    }

    return eval;
}

TrainerInteractionManager::TrainingPriority TrainerInteractionManager::CalculateSpellPriority(uint32 spellId) const
{
    if (!m_bot)
        return TrainingPriority::LOW;

    // Check cache first
    auto it = m_priorityCache.find(spellId);
    if (it != m_priorityCache.end())
        return it->second;

    TrainingPriority priority = TrainingPriority::LOW;

    // Check if essential
    if (IsEssentialSpell(spellId))
    {
        priority = TrainingPriority::ESSENTIAL;
    }
    // Check if combat utility
    else if (IsCombatUtility(spellId))
    {
        priority = TrainingPriority::HIGH;
    }
    // Check spell type
    else
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (spellInfo)
        {
            // Damage/heal spells are medium priority
            if (spellInfo->HasAttribute(SPELL_ATTR0_IS_ABILITY))
            {
                priority = TrainingPriority::MEDIUM;
            }
        }
    }

    // Cache the result
    const_cast<TrainerInteractionManager*>(this)->m_priorityCache[spellId] = priority;

    return priority;
}

bool TrainerInteractionManager::CanLearnSpell(Creature* trainer, uint32 spellId) const
{
    if (!trainer || !m_bot || spellId == 0)
        return false;

    // Check if already known
    if (m_bot->HasSpell(spellId))
        return false;

    // Get trainer data
    Trainer::Trainer const* trainerData = sObjectMgr->GetTrainer(trainer->GetEntry());
    if (!trainerData)
        return false;

    // Find the spell
    for (Trainer::Spell const& spell : trainerData->GetSpells())
    {
        if (spell.SpellId == spellId)
        {
            return trainerData->GetSpellState(m_bot, &spell) == Trainer::SpellState::Available;
        }
    }

    return false;
}

uint32 TrainerInteractionManager::GetTotalTrainingCost(Creature* trainer) const
{
    if (!trainer || !m_bot)
        return 0;

    uint32 totalCost = 0;

    Trainer::Trainer const* trainerData = sObjectMgr->GetTrainer(trainer->GetEntry());
    if (!trainerData)
        return 0;

    for (Trainer::Spell const& spell : trainerData->GetSpells())
    {
        if (trainerData->GetSpellState(m_bot, &spell) == Trainer::SpellState::Available)
        {
            totalCost += spell.MoneyCost;
        }
    }

    return totalCost;
}

bool TrainerInteractionManager::CanAffordTraining(uint32 cost) const
{
    if (!m_bot)
        return false;

    return m_bot->GetMoney() >= cost;
}

// ============================================================================
// Budget Management
// ============================================================================

TrainerInteractionManager::TrainingBudget TrainerInteractionManager::CalculateBudget() const
{
    TrainingBudget budget;

    if (!m_bot)
        return budget;

    budget.totalAvailable = m_bot->GetMoney();

    // Reserve based on percentages
    budget.reservedForEssential = static_cast<uint64>(budget.totalAvailable * BUDGET_ESSENTIAL_PERCENT);
    budget.reservedForCombat = static_cast<uint64>(budget.totalAvailable * BUDGET_COMBAT_PERCENT);
    budget.reservedForUtility = static_cast<uint64>(budget.totalAvailable * BUDGET_UTILITY_PERCENT);
    budget.discretionary = static_cast<uint64>(budget.totalAvailable * BUDGET_DISCRETIONARY_PERCENT);

    return budget;
}

bool TrainerInteractionManager::FitsWithinBudget(uint32 cost, TrainingPriority priority, TrainingBudget const& budget) const
{
    switch (priority)
    {
        case TrainingPriority::ESSENTIAL:
            return cost <= budget.reservedForEssential;
        case TrainingPriority::HIGH:
            return cost <= budget.reservedForCombat;
        case TrainingPriority::MEDIUM:
            return cost <= budget.reservedForUtility;
        case TrainingPriority::LOW:
            return cost <= budget.discretionary;
        default:
            return false;
    }
}

// ============================================================================
// Specialty Methods
// ============================================================================

::std::vector<uint32> TrainerInteractionManager::GetEssentialSpells() const
{
    return ::std::vector<uint32>(m_essentialSpellsCache.begin(), m_essentialSpellsCache.end());
}

::std::vector<uint32> TrainerInteractionManager::GetMissingEssentialSpells() const
{
    ::std::vector<uint32> missing;

    if (!m_bot)
        return missing;

    for (uint32 spellId : m_essentialSpellsCache)
    {
        if (!m_bot->HasSpell(spellId))
            missing.push_back(spellId);
    }

    return missing;
}

bool TrainerInteractionManager::IsClassTrainer(Creature* trainer) const
{
    if (!trainer || !m_bot)
        return false;

    Trainer::Trainer const* trainerData = sObjectMgr->GetTrainer(trainer->GetEntry());
    if (!trainerData)
        return false;

    return trainerData->GetTrainerType() == Trainer::Type::Class;
}

bool TrainerInteractionManager::IsProfessionTrainer(Creature* trainer) const
{
    if (!trainer)
        return false;

    Trainer::Trainer const* trainerData = sObjectMgr->GetTrainer(trainer->GetEntry());
    if (!trainerData)
        return false;

    return trainerData->GetTrainerType() == Trainer::Type::Tradeskill;
}

uint32 TrainerInteractionManager::GetTrainerProfession(Creature* trainer) const
{
    if (!trainer)
        return 0;

    Trainer::Trainer const* trainerData = sObjectMgr->GetTrainer(trainer->GetEntry());
    if (!trainerData)
        return 0;

    if (trainerData->GetTrainerType() != Trainer::Type::Tradeskill)
        return 0;

    // Return the requirement (profession skill) if set
    return trainerData->GetTrainerRequirement();
}

// ============================================================================
// Internal Helper Methods
// ============================================================================

bool TrainerInteractionManager::IsEssentialSpell(uint32 spellId) const
{
    return m_essentialSpellsCache.find(spellId) != m_essentialSpellsCache.end();
}

bool TrainerInteractionManager::IsCombatUtility(uint32 spellId) const
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Check for interrupt effects
    if (spellInfo->HasEffect(SPELL_EFFECT_INTERRUPT_CAST))
        return true;

    // Check for defensive abilities
    if (spellInfo->HasAura(SPELL_AURA_SCHOOL_IMMUNITY) ||
        spellInfo->HasAura(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN))
        return true;

    // Check for CC abilities
    if (spellInfo->HasAura(SPELL_AURA_MOD_STUN) ||
        spellInfo->HasAura(SPELL_AURA_MOD_ROOT))
        return true;

    return false;
}

bool TrainerInteractionManager::ExecuteTraining(Creature* trainer, uint32 spellId, uint32 cost)
{
    if (!trainer || !m_bot)
        return false;

    // Deduct gold
    if (cost > 0)
    {
        if (!m_bot->HasEnoughMoney(cost))
            return false;
        m_bot->ModifyMoney(-static_cast<int64>(cost));
    }

    // Learn the spell directly
    m_bot->LearnSpell(spellId, false);

    TC_LOG_DEBUG("bot.playerbot", "TrainerInteractionManager: Bot %s learned spell %u for %u copper",
        m_bot->GetName().c_str(), spellId, cost);

    return true;
}

void TrainerInteractionManager::RecordTraining(uint32 spellId, uint32 cost, bool success)
{
    if (success)
    {
        m_stats.spellsLearned++;
        m_stats.totalGoldSpent += cost;
    }
    else
    {
        m_stats.trainingFailures++;
    }
}

void TrainerInteractionManager::InitializeEssentialSpellsCache()
{
    if (!m_bot || m_essentialCacheInitialized)
        return;

    m_essentialSpellsCache.clear();

    // Add class-specific essential spells
    // These are placeholder spell IDs - should be replaced with actual WoW 12.0 spell IDs
    uint8 classId = m_bot->GetClass();

    switch (classId)
    {
        case CLASS_WARRIOR:
            // Core abilities for each spec
            // Arms/Fury: Mortal Strike, Bloodthirst, Execute
            // Prot: Shield Slam, Devastate, Thunder Clap
            break;

        case CLASS_PALADIN:
            // Holy: Flash of Light, Holy Light
            // Prot: Shield of the Righteous, Hammer of the Righteous
            // Ret: Templar's Verdict, Crusader Strike
            break;

        case CLASS_HUNTER:
            // All: Kill Command, Cobra Shot, Multi-Shot
            break;

        case CLASS_ROGUE:
            // All: Sinister Strike, Eviscerate, Slice and Dice
            break;

        case CLASS_PRIEST:
            // Holy/Disc: Flash Heal, Power Word: Shield
            // Shadow: Mind Blast, Shadow Word: Pain, Vampiric Touch
            break;

        case CLASS_DEATH_KNIGHT:
            // Blood: Death Strike, Heart Strike
            // Frost: Frost Strike, Obliterate
            // Unholy: Festering Strike, Scourge Strike
            break;

        case CLASS_SHAMAN:
            // Elemental: Lightning Bolt, Lava Burst
            // Enhancement: Stormstrike, Lava Lash
            // Restoration: Healing Wave, Riptide
            break;

        case CLASS_MAGE:
            // Arcane: Arcane Blast, Arcane Missiles
            // Fire: Fireball, Pyroblast
            // Frost: Frostbolt, Ice Lance
            break;

        case CLASS_WARLOCK:
            // Affliction: Corruption, Agony
            // Demonology: Hand of Gul'dan, Demonbolt
            // Destruction: Chaos Bolt, Incinerate
            break;

        case CLASS_MONK:
            // Brewmaster: Keg Smash, Tiger Palm
            // Mistweaver: Vivify, Renewing Mist
            // Windwalker: Rising Sun Kick, Blackout Kick
            break;

        case CLASS_DRUID:
            // Balance: Wrath, Starfire, Starsurge
            // Feral: Shred, Rake, Ferocious Bite
            // Guardian: Mangle, Thrash
            // Restoration: Rejuvenation, Lifebloom
            break;

        case CLASS_DEMON_HUNTER:
            // Havoc: Demon's Bite, Chaos Strike
            // Vengeance: Shear, Soul Cleave
            break;

        case CLASS_EVOKER:
            // Devastation: Fire Breath, Disintegrate
            // Preservation: Dream Breath, Reversion
            // Augmentation: Eruption, Upheaval
            break;

        default:
            break;
    }

    m_essentialCacheInitialized = true;
}

size_t TrainerInteractionManager::GetMemoryUsage() const
{
    size_t memory = sizeof(*this);
    memory += m_priorityCache.size() * (sizeof(uint32) + sizeof(TrainingPriority));
    memory += m_essentialSpellsCache.size() * sizeof(uint32);
    return memory;
}

} // namespace Playerbot
