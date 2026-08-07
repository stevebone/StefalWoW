/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "PathOfAscension.h"
#include "DB2Stores.h"
#include "DB2Structure.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Garrison.h"
#include "GarrisonMgr.h"
#include "Log.h"
#include "Player.h"
#include <algorithm>

uint8 AscensionMemoryTemplate::GetTrialTier(AscensionTrial trial) const
{
    switch (trial)
    {
        case ASCENSION_TRIAL_COURAGE:   return CourageTier;
        case ASCENSION_TRIAL_LOYALTY:   return LoyaltyTier;
        case ASCENSION_TRIAL_WISDOM:    return WisdomTier;
        case ASCENSION_TRIAL_HUMILITY:  return HumilityTier;
        default:                        return 0;
    }
}

PathOfAscension::PathOfAscension(Player* owner) : _owner(owner)
{
}

uint32 PathOfAscension::GetTrialDifficultyId(AscensionTrial trial)
{
    // Difficulty.db2 rows 168-171, "Path of Ascension: Courage / Loyalty / Wisdom / Humility"
    // (InstanceType 5 = MAP_SCENARIO, MinPlayers = MaxPlayers = 1).
    switch (trial)
    {
        case ASCENSION_TRIAL_COURAGE:   return 168;
        case ASCENSION_TRIAL_LOYALTY:   return 169;
        case ASCENSION_TRIAL_WISDOM:    return 170;
        case ASCENSION_TRIAL_HUMILITY:  return 171;
        default:                        return 0;
    }
}

char const* PathOfAscension::GetTrialName(AscensionTrial trial)
{
    switch (trial)
    {
        case ASCENSION_TRIAL_COURAGE:   return "Trial of Courage";
        case ASCENSION_TRIAL_LOYALTY:   return "Trial of Loyalty";
        case ASCENSION_TRIAL_WISDOM:    return "Trial of Wisdom";
        case ASCENSION_TRIAL_HUMILITY:  return "Trial of Humility";
        default:                        return "none";
    }
}

uint32 PathOfAscension::GetAscensionTreeId() const
{
    if (!_owner || _owner->GetActiveCovenant() != COVENANT_ID_KYRIAN)
        return 0;

    if (std::vector<GarrTalentTreeEntry const*> const* trees = sGarrisonMgr.GetTalentTreesForGarrType(GARRISON_TYPE_COVENANT))
        for (GarrTalentTreeEntry const* tree : *trees)
            if (tree->FeatureTypeIndex == GARR_TALENT_FEATURE_UNIQUE && tree->FeatureSubtypeIndex == COVENANT_ID_KYRIAN)
                return tree->ID;

    return 0;
}

uint32 PathOfAscension::GetResearchedTiers() const
{
    uint32 const treeId = GetAscensionTreeId();
    if (!treeId)
        return 0;

    Garrison const* garrison = _owner->GetGarrison(GARRISON_TYPE_COVENANT);
    if (!garrison)
        return 0;

    std::vector<GarrTalentEntry const*> const* talents = sGarrisonMgr.GetTalentsForTree(treeId);
    if (!talents)
        return 0;

    // A talent counts as completed at Rank >= 1, the same test the rest of the sanctum uses.
    uint32 unlocked = 0;
    for (GarrTalentEntry const* talent : *talents)
        if (Garrison::Talent const* owned = garrison->GetTalent(talent->ID))
            if (owned->Rank >= 1)
                ++unlocked;

    return std::min<uint32>(unlocked, ASCENSION_MAX_TIERS);
}

bool PathOfAscension::IsAccessible() const
{
    return GetResearchedTiers() > 0;
}

uint32 PathOfAscension::GetMemoryCapacity() const
{
    // Talent 1091 "First Steps": "the capture of six Shadowlands memories".
    // Talent 1092 "Sacred Trials": "Allows the capture of four more". Nothing above tier 2 adds any.
    uint32 const tiers = GetResearchedTiers();
    if (!tiers)
        return 0;

    return tiers == 1 ? uint32(ASCENSION_MEMORIES_AT_FIRST_TIER) : uint32(ASCENSION_MEMORIES_AT_SECOND_TIER);
}

AscensionTrial PathOfAscension::GetMaxTrial() const
{
    // The ceiling the talent descriptions state, tier by tier:
    //   1 "First Steps"         - "the sacred trial"                        -> Courage
    //   2 "Sacred Trials"       - "access to their second trial"            -> Loyalty
    //   3 "Continued Training"  - "the rest of the Trials of Loyalty as well
    //                             as the first of the Trials of Wisdom"     -> Wisdom
    //   4 "Teachings of Wisdom" - "the remaining Trials of Wisdom"          -> Wisdom
    //   5 "Trials of Humility"  - "the final trial ... Trial of Humility"   -> Humility
    switch (GetResearchedTiers())
    {
        case 0:  return ASCENSION_TRIAL_NONE;
        case 1:  return ASCENSION_TRIAL_COURAGE;
        case 2:  return ASCENSION_TRIAL_LOYALTY;
        case 3:
        case 4:  return ASCENSION_TRIAL_WISDOM;
        default: return ASCENSION_TRIAL_HUMILITY;
    }
}

uint32 PathOfAscension::GetWeeklyQuestSlots() const
{
    uint32 const tiers = GetResearchedTiers();
    if (tiers >= ASCENSION_WEEKLY_SLOTS_TIER_SECOND)
        return 2;   // talent 1094 "Unlocks access to a second weekly quest."
    if (tiers >= ASCENSION_WEEKLY_SLOTS_TIER_FIRST)
        return 1;   // talent 1092 "Unlocks access to weekly quests."
    return 0;
}

uint32 PathOfAscension::GetActiveBraziers() const
{
    uint32 const tiers = GetResearchedTiers();
    uint32 braziers = 0;
    if (tiers >= ASCENSION_BRAZIER_LESSONS_TIER)
        ++braziers;     // talent 1093 - Brazier of Lessons Learned
    if (tiers >= ASCENSION_BRAZIER_INWARD_TIER)
        ++braziers;     // talent 1095 - Brazier of Inward Reflection
    return braziers;
}

bool PathOfAscension::HasMemory(uint32 memoryId) const
{
    return _memories.find(memoryId) != _memories.end();
}

AscensionMemory const* PathOfAscension::GetMemory(uint32 memoryId) const
{
    auto itr = _memories.find(memoryId);
    return itr != _memories.end() ? &itr->second : nullptr;
}

std::vector<AscensionMemory const*> PathOfAscension::GetMemories() const
{
    std::vector<AscensionMemory const*> memories;
    memories.reserve(_memories.size());
    for (auto const& [memoryId, memory] : _memories)
        memories.push_back(&memory);

    std::sort(memories.begin(), memories.end(),
        [](AscensionMemory const* l, AscensionMemory const* r) { return l->CapturedTime < r->CapturedTime; });
    return memories;
}

AscensionTrial PathOfAscension::GetHighestTrialWon(uint32 memoryId) const
{
    AscensionMemory const* memory = GetMemory(memoryId);
    return memory ? AscensionTrial(memory->HighestTrialWon) : ASCENSION_TRIAL_NONE;
}

uint32 PathOfAscension::GetTotalTrialWins() const
{
    uint32 total = 0;
    for (auto const& [memoryId, memory] : _memories)
        total += memory.HighestTrialWon;
    return total;
}

bool PathOfAscension::IsTrialAvailable(uint32 memoryId, AscensionTrial trial) const
{
    return CheckTrial(memoryId, trial) == ASCENSION_OK;
}

PathOfAscensionError PathOfAscension::CaptureMemory(uint32 memoryId)
{
    if (!_owner || _owner->GetActiveCovenant() != COVENANT_ID_KYRIAN)
        return ASCENSION_ERROR_NOT_KYRIAN;

    uint32 const tiers = GetResearchedTiers();
    if (!tiers)
        return ASCENSION_ERROR_NOT_UNLOCKED;

    if (sGarrisonMgr.GetAscensionMemories().empty())
        return ASCENSION_ERROR_NO_MEMORY_DATA;

    AscensionMemoryTemplate const* memoryTemplate = sGarrisonMgr.GetAscensionMemory(memoryId);
    if (!memoryTemplate)
        return ASCENSION_ERROR_UNKNOWN_MEMORY;

    if (HasMemory(memoryId))
        return ASCENSION_ERROR_ALREADY_CAPTURED;

    if (tiers < memoryTemplate->RequiredTier)
        return ASCENSION_ERROR_MEMORY_TIER_TOO_LOW;

    if (uint32(_memories.size()) >= GetMemoryCapacity())
        return ASCENSION_ERROR_MEMORY_CAPACITY;

    AscensionMemory& memory = _memories[memoryId];
    memory.MemoryId = memoryId;
    memory.CapturedTime = GameTime::GetGameTime();
    memory.HighestTrialWon = ASCENSION_TRIAL_NONE;
    memory.LastCompletedTime = 0;
    MarkChanged();

    TC_LOG_DEBUG("garrison", "PathOfAscension: player {} captured memory {} (creature {}); {} of {} held.",
        _owner->GetGUID().ToString(), memoryId, memoryTemplate->CreatureId, uint32(_memories.size()), GetMemoryCapacity());

    return ASCENSION_OK;
}

PathOfAscensionError PathOfAscension::CheckTrial(uint32 memoryId, AscensionTrial trial) const
{
    if (!_owner || _owner->GetActiveCovenant() != COVENANT_ID_KYRIAN)
        return ASCENSION_ERROR_NOT_KYRIAN;

    uint32 const tiers = GetResearchedTiers();
    if (!tiers)
        return ASCENSION_ERROR_NOT_UNLOCKED;

    if (trial < ASCENSION_TRIAL_COURAGE || trial > ASCENSION_TRIAL_MAX)
        return ASCENSION_ERROR_INVALID_TRIAL;

    if (sGarrisonMgr.GetAscensionMemories().empty())
        return ASCENSION_ERROR_NO_MEMORY_DATA;

    AscensionMemoryTemplate const* memoryTemplate = sGarrisonMgr.GetAscensionMemory(memoryId);
    if (!memoryTemplate)
        return ASCENSION_ERROR_UNKNOWN_MEMORY;

    if (!HasMemory(memoryId))
        return ASCENSION_ERROR_NOT_CAPTURED;

    // Two independent gates, and both are data: the sanctum-wide ceiling the talents state, and the per-memory
    // tier the authored row carries (the talents' "some / the rest / the first / the remaining").
    if (trial > GetMaxTrial())
        return ASCENSION_ERROR_TRIAL_LOCKED;

    uint8 const requiredTier = memoryTemplate->GetTrialTier(trial);
    if (!requiredTier || tiers < requiredTier)
        return ASCENSION_ERROR_TRIAL_LOCKED;

    return ASCENSION_OK;
}

PathOfAscensionError PathOfAscension::StartTrial(uint32 memoryId, AscensionTrial trial) const
{
    if (PathOfAscensionError result = CheckTrial(memoryId, trial); result != ASCENSION_OK)
        return result;

    // Scenario 1803 on map 2375 is real client data, but nothing in this world DB instantiates it: no
    // `scenarios` row, no spawn, no instance_template. Starting a run the player could never finish - or
    // worse, quietly "awarding" one - would be a lie. Refuse until the arena is authored.
    if (!sGarrisonMgr.IsAscensionArenaAuthored())
        return ASCENSION_ERROR_NO_ARENA_CONTENT;

    return ASCENSION_OK;
}

PathOfAscensionError PathOfAscension::CompleteTrial(uint32 memoryId, AscensionTrial trial)
{
    if (PathOfAscensionError result = CheckTrial(memoryId, trial); result != ASCENSION_OK)
        return result;

    AscensionMemory& memory = _memories[memoryId];
    memory.LastCompletedTime = GameTime::GetGameTime();
    if (trial > memory.HighestTrialWon)
        memory.HighestTrialWon = uint8(trial);
    MarkChanged();

    // The reward is not invented here. Each memory's own quest (QuestSortID -595, e.g. 63168 "Path of
    // Ascension: Echthra") carries a kill-credit objective on that memory's creature and its own reward item,
    // so crediting the creature makes the ordinary quest system pay exactly what the client data says.
    if (AscensionMemoryTemplate const* memoryTemplate = sGarrisonMgr.GetAscensionMemory(memoryId))
        if (memoryTemplate->CreatureId)
            _owner->KilledMonsterCredit(memoryTemplate->CreatureId);

    TC_LOG_DEBUG("garrison", "PathOfAscension: player {} won the {} against memory {} (highest now {}).",
        _owner->GetGUID().ToString(), GetTrialName(trial), memoryId, GetTrialName(AscensionTrial(memory.HighestTrialWon)));

    return ASCENSION_OK;
}

void PathOfAscension::Update()
{
    if (!_owner)
        return;

    // A talent reset can lower the researched tier under a memory that is already held. Nothing is deleted -
    // the capture stands and re-researching restores access - but a trial won above the new ceiling must not
    // keep reading as available, so clamp the high-water mark to what the tier still supports.
    AscensionTrial const ceiling = GetMaxTrial();
    for (auto& [memoryId, memory] : _memories)
    {
        if (memory.HighestTrialWon > ceiling)
        {
            memory.HighestTrialWon = uint8(ceiling);
            MarkChanged();
        }
    }
}

void PathOfAscension::LoadFromDB(PreparedQueryResult result)
{
    _memories.clear();
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint32 const memoryId = fields[0].GetUInt32();

        // An authored row can disappear between sessions; keeping the character row would leave a memory that
        // no gate can evaluate. Drop it loudly rather than carry an unresolvable id.
        if (!sGarrisonMgr.GetAscensionMemory(memoryId))
        {
            TC_LOG_ERROR("garrison", "PathOfAscension: dropping stored memory {} for player {} - it has no row in "
                "`garrison_ascension_memory`.", memoryId, _owner->GetGUID().ToString());
            continue;
        }

        AscensionMemory& memory = _memories[memoryId];
        memory.MemoryId = memoryId;
        memory.CapturedTime = fields[1].GetInt64();
        memory.HighestTrialWon = std::min<uint8>(fields[2].GetUInt8(), uint8(ASCENSION_TRIAL_MAX));
        memory.LastCompletedTime = fields[3].GetInt64();
    } while (result->NextRow());
}

void PathOfAscension::SaveToDB(CharacterDatabaseTransaction trans) const
{
    // Nothing was ever captured and nothing changed - skip the delete/insert churn.
    if (!_needsSave && _memories.empty())
        return;

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER_GARRISON_ASCENSION);
    stmt->setUInt64(0, _owner->GetGUID().GetCounter());
    trans->Append(stmt);

    for (auto const& [memoryId, memory] : _memories)
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_GARRISON_ASCENSION);
        stmt->setUInt64(0, _owner->GetGUID().GetCounter());
        stmt->setUInt32(1, memory.MemoryId);
        stmt->setInt64(2, memory.CapturedTime);
        stmt->setUInt8(3, memory.HighestTrialWon);
        stmt->setInt64(4, memory.LastCompletedTime);
        trans->Append(stmt);
    }
}
