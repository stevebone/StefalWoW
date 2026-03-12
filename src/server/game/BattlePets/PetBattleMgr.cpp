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

#include "PetBattleMgr.h"
#include "BattlePetMgr.h"
#include "BattlePetPackets.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "DB2Stores.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "WorldSession.h"
#include <algorithm>

namespace PetBattles
{

PetBattleMgr::PetBattleMgr() = default;
PetBattleMgr::~PetBattleMgr() = default;

PetBattleMgr* PetBattleMgr::instance()
{
    static PetBattleMgr instance;
    return &instance;
}

void PetBattleMgr::Initialize()
{
    TC_LOG_INFO("server.loading", ">> Loading Pet Battle data...");

    uint32 speciesAbilityCount = 0;
    uint32 abilityTurnCount = 0;
    uint32 turnEffectCount = 0;

    // Build speciesID -> abilities index from BattlePetSpeciesXAbility DB2
    for (BattlePetSpeciesXAbilityEntry const* entry : sBattlePetSpeciesXAbilityStore)
    {
        _speciesAbilities[entry->BattlePetSpeciesID].push_back(entry->BattlePetAbilityID);
        _speciesAbilitiesFull[entry->BattlePetSpeciesID].push_back(entry);
        ++speciesAbilityCount;
    }

    // Build abilityID -> turns index from BattlePetAbilityTurn DB2
    for (BattlePetAbilityTurnEntry const* entry : sBattlePetAbilityTurnStore)
    {
        _abilityTurns[entry->BattlePetAbilityID].push_back(entry->ID);
        _abilityTurnsFull[entry->BattlePetAbilityID].push_back(entry);
        ++abilityTurnCount;
    }

    // Sort turns by OrderIndex for correct multi-turn sequencing
    for (auto& [abilityID, turns] : _abilityTurnsFull)
    {
        std::sort(turns.begin(), turns.end(),
            [](BattlePetAbilityTurnEntry const* a, BattlePetAbilityTurnEntry const* b)
            {
                return a->OrderIndex < b->OrderIndex;
            });
    }

    // Build turnID -> effects index from BattlePetAbilityEffect DB2
    for (BattlePetAbilityEffectEntry const* entry : sBattlePetAbilityEffectStore)
    {
        _turnEffects[entry->BattlePetAbilityTurnID].push_back(entry->ID);
        _turnEffectsFull[entry->BattlePetAbilityTurnID].push_back(entry);
        ++turnEffectCount;
    }

    // Sort effects by OrderIndex for correct execution order
    for (auto& [turnID, effects] : _turnEffectsFull)
    {
        std::sort(effects.begin(), effects.end(),
            [](BattlePetAbilityEffectEntry const* a, BattlePetAbilityEffectEntry const* b)
            {
                return a->OrderIndex < b->OrderIndex;
            });
    }

    // Build breed quality multiplier map from BattlePetBreedQuality DB2
    for (BattlePetBreedQualityEntry const* entry : sBattlePetBreedQualityStore)
        _breedQualityMultipliers[entry->QualityEnum] = entry->StateMultiplier;

    // Build breed base stats map from BattlePetBreedState DB2
    // StateID: 18=MaxHP (Stamina), 19=Power, 20=Speed
    for (BattlePetBreedStateEntry const* entry : sBattlePetBreedStateStore)
    {
        auto& [hp, power, speed] = _breedBaseStats[entry->BattlePetBreedID];
        switch (entry->BattlePetStateID)
        {
            case BattlePets::STATE_STAT_STAMINA: hp = entry->Value; break;
            case BattlePets::STATE_STAT_POWER:   power = entry->Value; break;
            case BattlePets::STATE_STAT_SPEED:   speed = entry->Value; break;
            default: break;
        }
    }

    TC_LOG_INFO("server.loading", ">> Loaded {} species abilities, {} ability turns, {} turn effects",
        speciesAbilityCount, abilityTurnCount, turnEffectCount);
    TC_LOG_INFO("server.loading", ">> Loaded {} breed quality entries, {} breed stat entries",
        uint32(_breedQualityMultipliers.size()), uint32(_breedBaseStats.size()));

    // Build the BattlePetEffectPropertiesID -> action type mapping
    BuildEffectActionMap();

    // Build weather ability ID set: find abilities whose effects use WEATHER_SET PropsIDs
    // Walk: WEATHER_SET PropsIDs → effects using those PropsIDs → turns → abilities
    {
        // Step 1: collect all PropsIDs classified as WEATHER_SET
        std::unordered_set<uint16> weatherPropsIDs;
        for (auto const& [propsID, action] : _effectActionMap)
            if (action == PET_BATTLE_EFFECT_ACTION_WEATHER_SET)
                weatherPropsIDs.insert(propsID);

        // Step 2: find all turns containing effects with weather PropsIDs
        std::unordered_set<uint32> weatherTurnIDs;
        for (BattlePetAbilityEffectEntry const* effect : sBattlePetAbilityEffectStore)
            if (weatherPropsIDs.count(effect->BattlePetEffectPropertiesID))
                weatherTurnIDs.insert(effect->BattlePetAbilityTurnID);

        // Step 3: find all abilities owning those turns
        for (BattlePetAbilityTurnEntry const* turn : sBattlePetAbilityTurnStore)
            if (weatherTurnIDs.count(turn->ID))
                _weatherAbilityIDs.insert(turn->BattlePetAbilityID);

        // Step 4: also add abilities referenced by AuraBattlePetAbilityID from weather turn effects
        // (the aura effects reference weather abilities via AuraBattlePetAbilityID)
        for (BattlePetAbilityEffectEntry const* effect : sBattlePetAbilityEffectStore)
        {
            if (weatherTurnIDs.count(effect->BattlePetAbilityTurnID) && effect->AuraBattlePetAbilityID != 0)
                _weatherAbilityIDs.insert(effect->AuraBattlePetAbilityID);
        }

        TC_LOG_INFO("server.loading", ">> Found {} weather ability IDs from {} weather PropsIDs",
            uint32(_weatherAbilityIDs.size()), uint32(weatherPropsIDs.size()));
        for (uint32 abilityID : _weatherAbilityIDs)
        {
            BattlePetAbilityEntry const* ability = sBattlePetAbilityStore.LookupEntry(abilityID);
            TC_LOG_INFO("server.loading", "  WeatherAbility: ID={} name={}", abilityID,
                ability ? ability->Name.Str[LOCALE_enUS] : "???");
        }
    }

    LoadNPCTeams();

    TC_LOG_INFO("server.loading", ">> Pet Battle system initialized");
}

void PetBattleMgr::BuildEffectActionMap()
{
    // Classify every BattlePetEffectProperties entry into an abstract action type
    // based on its ParamLabel signatures and BattlePetVisualID.
    // This runs once at startup so string comparisons are acceptable.

    uint32 mappedCount = 0;
    for (BattlePetEffectPropertiesEntry const* props : sBattlePetEffectPropertiesStore)
    {
        bool hasPoints = false, hasDuration = false, hasState = false;
        bool hasChance = false, hasPercentage = false;
        bool hasWeather = false, hasSwap = false, hasTrap = false;

        for (uint8 i = 0; i < 6; ++i)
        {
            if (!props->ParamLabel[i] || !props->ParamLabel[i][0])
                continue;
            std::string_view label(props->ParamLabel[i]);
            if (label == "Points") hasPoints = true;
            else if (label == "Percentage") { hasPoints = true; hasPercentage = true; }
            else if (label == "Duration") hasDuration = true;
            else if (label == "weatherState" || label == "WeatherState" || label == "weatherAura" || label == "WeatherAura") hasWeather = true;
            else if (label.find("State") != std::string_view::npos) hasState = true;
            else if (label == "Chance") hasChance = true;
            else if (label == "SwapIndex" || label == "Swap") hasSwap = true;
            else if (label == "TrapAbility") hasTrap = true;
        }

        PetBattleAbilityEffectAction action;

        if (hasWeather)
            action = PET_BATTLE_EFFECT_ACTION_WEATHER_SET;
        else if (hasSwap)
            action = PET_BATTLE_EFFECT_ACTION_PET_SWAP;
        else if (hasTrap)
            action = PET_BATTLE_EFFECT_ACTION_CATCH;
        else if (hasDuration && hasPoints && props->BattlePetVisualID == 38)
            action = PET_BATTLE_EFFECT_ACTION_PERIODIC_DAMAGE;
        else if (hasDuration && hasPoints && props->BattlePetVisualID != 38)
            action = PET_BATTLE_EFFECT_ACTION_PERIODIC_HEAL;
        else if (hasDuration)
            action = PET_BATTLE_EFFECT_ACTION_APPLY_AURA;
        else if (hasPoints && hasPercentage && props->BattlePetVisualID == 38)
            action = PET_BATTLE_EFFECT_ACTION_DAMAGE_PERCENTAGE;
        else if (hasPoints && hasPercentage)
            action = PET_BATTLE_EFFECT_ACTION_HEAL_PERCENTAGE;
        else if (hasPoints && props->BattlePetVisualID == 38)
            action = PET_BATTLE_EFFECT_ACTION_DAMAGE;
        else if (hasPoints)
            action = PET_BATTLE_EFFECT_ACTION_HEAL;
        else if (hasState)
            action = PET_BATTLE_EFFECT_ACTION_SET_STATE;
        else if (hasChance)
            action = PET_BATTLE_EFFECT_ACTION_DAMAGE; // Conditional — evaluate at use site, fall through to damage
        else
            action = PET_BATTLE_EFFECT_ACTION_DAMAGE; // Safe fallback — default handler checks basePower

        _effectActionMap[static_cast<uint16>(props->ID)] = action;
        ++mappedCount;
    }

    // Log all mapped entries for debugging
    {
        std::map<uint16, uint32> usageCounts;
        for (BattlePetAbilityEffectEntry const* entry : sBattlePetAbilityEffectStore)
            usageCounts[entry->BattlePetEffectPropertiesID]++;

        for (auto const& [propsID, action] : _effectActionMap)
        {
            BattlePetEffectPropertiesEntry const* props = sBattlePetEffectPropertiesStore.LookupEntry(propsID);
            std::string labels;
            if (props)
                for (uint8 i = 0; i < 6; ++i)
                    if (props->ParamLabel[i] && props->ParamLabel[i][0] != '\0')
                        labels += Trinity::StringFormat("[{}]={} ", i, props->ParamLabel[i]);
            TC_LOG_INFO("server.loading", "  EffectMap: PropsID={:3d} -> action={:2d} uses={:3d} visual={} labels: {}",
                propsID, uint16(action), usageCounts.count(propsID) ? usageCounts[propsID] : 0,
                props ? props->BattlePetVisualID : 0, labels);
        }
    }

    TC_LOG_INFO("server.loading", ">> Mapped {} BattlePetEffectProperties entries to action types", mappedCount);
}

PetBattleAbilityEffectAction PetBattleMgr::GetEffectAction(uint16 propsID) const
{
    auto it = _effectActionMap.find(propsID);
    if (it != _effectActionMap.end())
        return it->second;
    return PET_BATTLE_EFFECT_ACTION_DAMAGE; // Fallback for unmapped IDs
}

bool PetBattleMgr::IsWeatherAbility(uint32 abilityID) const
{
    return _weatherAbilityIDs.count(abilityID) > 0;
}

void PetBattleMgr::Update(uint32 diff)
{
    // Update all active battles
    std::vector<uint32> finishedBattles;
    for (auto& [battleID, battle] : _activeBattles)
    {
        battle->Update(diff);
        if (battle->IsFinished())
            finishedBattles.push_back(battleID);
    }

    // Clean up finished battles after iteration
    for (uint32 battleID : finishedBattles)
        RemoveBattle(battleID);

    // Try to match PvP queue players periodically (every 5 seconds)
    _queueMatchTimer += diff;
    if (_queueMatchTimer >= 5000)
    {
        _queueMatchTimer = 0;
        TryMatchPlayers();
    }
}

// ============================================================================
// Battle lifecycle
// ============================================================================

PetBattle* PetBattleMgr::CreateWildBattle(Player* player, ObjectGuid wildCreatureGUID)
{
    if (IsPlayerInBattle(player->GetGUID()))
        return nullptr;

    uint32 battleID = _nextBattleID++;
    auto battle = std::make_unique<PetBattle>();
    battle->SetBattleID(battleID);
    battle->InitWildBattle(player, wildCreatureGUID);

    PetBattle* ptr = battle.get();
    _activeBattles[battleID] = std::move(battle);
    _playerToBattle[player->GetGUID()] = battleID;

    TC_LOG_DEBUG("server.loading", "PetBattleMgr: Created wild battle {} for player {}",
        battleID, player->GetGUID().ToString());

    return ptr;
}

PetBattle* PetBattleMgr::CreatePvPBattle(Player* player1, Player* player2)
{
    if (IsPlayerInBattle(player1->GetGUID()) || IsPlayerInBattle(player2->GetGUID()))
        return nullptr;

    uint32 battleID = _nextBattleID++;
    auto battle = std::make_unique<PetBattle>();
    battle->SetBattleID(battleID);
    battle->InitPvPBattle(player1, player2);

    if (battle->IsFinished())
        return nullptr; // Validation failed in InitPvPBattle

    PetBattle* ptr = battle.get();
    _activeBattles[battleID] = std::move(battle);
    _playerToBattle[player1->GetGUID()] = battleID;
    _playerToBattle[player2->GetGUID()] = battleID;

    return ptr;
}

PetBattle* PetBattleMgr::CreateNPCBattle(Player* player, Creature* trainer)
{
    if (!player || !trainer)
        return nullptr;

    if (IsPlayerInBattle(player->GetGUID()))
        return nullptr;

    std::vector<NPCTeamPetInfo> const* npcTeam = GetNPCTeam(trainer->GetEntry());
    if (!npcTeam || npcTeam->empty())
        return nullptr;

    uint32 battleID = _nextBattleID++;
    auto battle = std::make_unique<PetBattle>();
    battle->SetBattleID(battleID);
    battle->InitNPCBattle(player, trainer, *npcTeam);

    PetBattle* ptr = battle.get();
    _activeBattles[battleID] = std::move(battle);
    _playerToBattle[player->GetGUID()] = battleID;

    TC_LOG_DEBUG("server.loading", "PetBattleMgr: Created NPC battle {} for player {} vs trainer {}",
        battleID, player->GetGUID().ToString(), trainer->GetEntry());

    return ptr;
}

void PetBattleMgr::RemoveBattle(uint32 battleID)
{
    auto it = _activeBattles.find(battleID);
    if (it == _activeBattles.end())
        return;

    PetBattle* battle = it->second.get();

    // Remove player mappings
    for (uint8 i = 0; i < MAX_PET_BATTLE_PLAYERS; ++i)
    {
        ObjectGuid playerGUID = battle->GetTeam(i).PlayerGUID;
        if (!playerGUID.IsEmpty())
            _playerToBattle.erase(playerGUID);
    }

    _activeBattles.erase(it);
}

PetBattle* PetBattleMgr::GetBattleByID(uint32 battleID)
{
    auto it = _activeBattles.find(battleID);
    return it != _activeBattles.end() ? it->second.get() : nullptr;
}

PetBattle* PetBattleMgr::GetBattleByPlayer(ObjectGuid playerGUID)
{
    auto it = _playerToBattle.find(playerGUID);
    if (it == _playerToBattle.end())
        return nullptr;
    return GetBattleByID(it->second);
}

bool PetBattleMgr::IsPlayerInBattle(ObjectGuid playerGUID) const
{
    return _playerToBattle.contains(playerGUID);
}

// ============================================================================
// DB2 index map accessors
// ============================================================================

std::vector<uint32> const* PetBattleMgr::GetSpeciesAbilities(uint32 speciesID) const
{
    auto it = _speciesAbilities.find(speciesID);
    return it != _speciesAbilities.end() ? &it->second : nullptr;
}

std::vector<uint32> const* PetBattleMgr::GetAbilityTurns(uint32 abilityID) const
{
    auto it = _abilityTurns.find(abilityID);
    return it != _abilityTurns.end() ? &it->second : nullptr;
}

std::vector<uint32> const* PetBattleMgr::GetTurnEffects(uint32 turnID) const
{
    auto it = _turnEffects.find(turnID);
    return it != _turnEffects.end() ? &it->second : nullptr;
}

std::vector<BattlePetSpeciesXAbilityEntry const*> const* PetBattleMgr::GetSpeciesAbilitiesFull(uint32 speciesID) const
{
    auto it = _speciesAbilitiesFull.find(speciesID);
    return it != _speciesAbilitiesFull.end() ? &it->second : nullptr;
}

std::vector<BattlePetAbilityTurnEntry const*> const* PetBattleMgr::GetAbilityTurnsFull(uint32 abilityID) const
{
    auto it = _abilityTurnsFull.find(abilityID);
    return it != _abilityTurnsFull.end() ? &it->second : nullptr;
}

std::vector<BattlePetAbilityEffectEntry const*> const* PetBattleMgr::GetTurnEffectsFull(uint32 turnID) const
{
    auto it = _turnEffectsFull.find(turnID);
    return it != _turnEffectsFull.end() ? &it->second : nullptr;
}

float PetBattleMgr::GetBreedQualityMultiplier(uint8 quality) const
{
    auto it = _breedQualityMultipliers.find(quality);
    return it != _breedQualityMultipliers.end() ? it->second : 1.0f;
}

void PetBattleMgr::GetBreedBaseStats(uint32 breedID, int32& outHP, int32& outPower, int32& outSpeed) const
{
    auto it = _breedBaseStats.find(breedID);
    if (it != _breedBaseStats.end())
    {
        auto const& [hp, power, speed] = it->second;
        outHP = hp;
        outPower = power;
        outSpeed = speed;
    }
    else
    {
        outHP = 100;
        outPower = 10;
        outSpeed = 10;
    }
}

// ============================================================================
// NPC Teams
// ============================================================================

void PetBattleMgr::LoadNPCTeams()
{
    _npcTeams.clear();

    QueryResult result = WorldDatabase.Query("SELECT npcEntry, slot, speciesId, level, breedId, quality, ability1, ability2, ability3, npcTeamMemberID, creatureId FROM battle_pet_npc_team ORDER BY npcEntry, slot");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 NPC pet battle teams. DB table `battle_pet_npc_team` is empty or missing.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 npcEntry = fields[0].GetUInt32();

        NPCTeamPetInfo pet;
        pet.SpeciesID = fields[2].GetUInt32();
        pet.Level = fields[3].GetUInt16();
        pet.BreedID = fields[4].GetUInt16();
        pet.Quality = fields[5].GetUInt8();
        pet.AbilityIDs[0] = fields[6].GetUInt32();
        pet.AbilityIDs[1] = fields[7].GetUInt32();
        pet.AbilityIDs[2] = fields[8].GetUInt32();
        pet.NpcTeamMemberID = fields[9].GetInt32();
        pet.CreatureID = fields[10].GetUInt32();

        _npcTeams[npcEntry].push_back(pet);
        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} NPC pet battle team members for {} trainers",
        count, uint32(_npcTeams.size()));
}

std::vector<NPCTeamPetInfo> const* PetBattleMgr::GetNPCTeam(uint32 npcEntry) const
{
    auto it = _npcTeams.find(npcEntry);
    return it != _npcTeams.end() ? &it->second : nullptr;
}

// ============================================================================
// PvP Queue
// ============================================================================

void PetBattleMgr::JoinQueue(ObjectGuid playerGUID)
{
    // Don't add if already in queue
    for (auto const& entry : _pvpQueue)
    {
        if (entry.PlayerGUID == playerGUID)
        {
            if (Player* player = ObjectAccessor::FindPlayer(playerGUID))
            {
                WorldPackets::BattlePet::PetBattleQueueStatus status;
                status.Status = PET_BATTLE_QUEUE_STATUS_ALREADY_QUEUED;
                player->SendDirectMessage(status.Write());
            }
            return;
        }
    }

    // Don't add if already in a battle
    if (IsPlayerInBattle(playerGUID))
    {
        if (Player* player = ObjectAccessor::FindPlayer(playerGUID))
        {
            WorldPackets::BattlePet::PetBattleQueueStatus status;
            status.Status = PET_BATTLE_QUEUE_STATUS_JOIN_FAILED;
            player->SendDirectMessage(status.Write());
        }
        return;
    }

    // Check player has alive slotted pets and journal lock
    if (Player* player = ObjectAccessor::FindPlayer(playerGUID))
    {
        BattlePets::BattlePetMgr* petMgr = player->GetSession()->GetBattlePetMgr();
        if (!petMgr->HasJournalLock())
        {
            WorldPackets::BattlePet::PetBattleQueueStatus status;
            status.Status = PET_BATTLE_QUEUE_STATUS_JOIN_FAILED_JOURNAL_LOCK;
            player->SendDirectMessage(status.Write());
            return;
        }

        bool hasPet = false;
        for (uint8 i = 0; i < uint8(BattlePets::BattlePetSlot::Count); ++i)
        {
            WorldPackets::BattlePet::BattlePetSlot* slot = petMgr->GetSlot(BattlePets::BattlePetSlot(i));
            if (slot && !slot->Locked && !slot->Pet.Guid.IsEmpty())
            {
                BattlePets::BattlePet* pet = petMgr->GetPet(slot->Pet.Guid);
                if (pet && pet->PacketInfo.Health > 0)
                {
                    hasPet = true;
                    break;
                }
            }
        }

        if (!hasPet)
        {
            WorldPackets::BattlePet::PetBattleQueueStatus status;
            status.Status = PET_BATTLE_QUEUE_STATUS_JOIN_FAILED_SLOTS;
            player->SendDirectMessage(status.Write());
            return;
        }
    }

    PvPQueueEntry entry;
    entry.PlayerGUID = playerGUID;
    entry.EnqueueTime = 0;
    _pvpQueue.push_back(entry);

    if (Player* player = ObjectAccessor::FindPlayer(playerGUID))
    {
        WorldPackets::BattlePet::PetBattleQueueStatus status;
        status.Status = PET_BATTLE_QUEUE_STATUS_QUEUED;
        player->SendDirectMessage(status.Write());
    }

    TC_LOG_DEBUG("server.loading", "PetBattleMgr: Player {} joined PvP pet battle queue (queue size: {})",
        playerGUID.ToString(), _pvpQueue.size());
}

void PetBattleMgr::LeaveQueue(ObjectGuid playerGUID)
{
    auto it = std::find_if(_pvpQueue.begin(), _pvpQueue.end(),
        [playerGUID](PvPQueueEntry const& entry) { return entry.PlayerGUID == playerGUID; });

    if (it != _pvpQueue.end())
        _pvpQueue.erase(it);

    // Cancel pending proposal if this player was in one
    if (_pendingProposal)
    {
        if (_pendingProposal->Player1 == playerGUID || _pendingProposal->Player2 == playerGUID)
        {
            ObjectGuid otherGUID = (_pendingProposal->Player1 == playerGUID) ?
                _pendingProposal->Player2 : _pendingProposal->Player1;

            if (Player* other = ObjectAccessor::FindPlayer(otherGUID))
            {
                WorldPackets::BattlePet::PetBattleQueueStatus status;
                status.Status = PET_BATTLE_QUEUE_STATUS_MATCH_OPPONENT_DECLINED;
                other->SendDirectMessage(status.Write());
            }

            // Re-add the other player to the queue
            PvPQueueEntry requeue;
            requeue.PlayerGUID = otherGUID;
            _pvpQueue.push_back(requeue);

            _pendingProposal.reset();
        }
    }

    if (Player* player = ObjectAccessor::FindPlayer(playerGUID))
    {
        WorldPackets::BattlePet::PetBattleQueueStatus status;
        status.Status = PET_BATTLE_QUEUE_STATUS_REMOVED;
        player->SendDirectMessage(status.Write());
    }
}

void PetBattleMgr::HandleProposalResult(ObjectGuid playerGUID, bool accepted)
{
    if (!_pendingProposal)
        return;

    if (_pendingProposal->Player1 == playerGUID)
        _pendingProposal->Player1Accepted = accepted;
    else if (_pendingProposal->Player2 == playerGUID)
        _pendingProposal->Player2Accepted = accepted;
    else
        return;

    if (!accepted)
    {
        // Send declined status to the declining player
        if (Player* player = ObjectAccessor::FindPlayer(playerGUID))
        {
            WorldPackets::BattlePet::PetBattleQueueStatus status;
            status.Status = PET_BATTLE_QUEUE_STATUS_MATCH_DECLINED;
            player->SendDirectMessage(status.Write());
        }

        // Notify the other player and re-queue them
        ObjectGuid otherGUID = (_pendingProposal->Player1 == playerGUID) ?
            _pendingProposal->Player2 : _pendingProposal->Player1;

        PvPQueueEntry requeue;
        requeue.PlayerGUID = otherGUID;
        _pvpQueue.push_back(requeue);

        if (Player* other = ObjectAccessor::FindPlayer(otherGUID))
        {
            WorldPackets::BattlePet::PetBattleQueueStatus status;
            status.Status = PET_BATTLE_QUEUE_STATUS_MATCH_OPPONENT_DECLINED;
            other->SendDirectMessage(status.Write());
        }

        _pendingProposal.reset();
        return;
    }

    // Send accepted status to this player
    if (Player* player = ObjectAccessor::FindPlayer(playerGUID))
    {
        WorldPackets::BattlePet::PetBattleQueueStatus status;
        status.Status = PET_BATTLE_QUEUE_STATUS_MATCH_ACCEPTED;
        player->SendDirectMessage(status.Write());
    }

    // Check if both accepted
    if (_pendingProposal->Player1Accepted && _pendingProposal->Player2Accepted)
    {
        Player* p1 = ObjectAccessor::FindPlayer(_pendingProposal->Player1);
        Player* p2 = ObjectAccessor::FindPlayer(_pendingProposal->Player2);

        if (p1 && p2)
        {
            PetBattle* battle = CreatePvPBattle(p1, p2);
            if (battle)
            {
                battle->Start();

                static constexpr float PET_BATTLE_HALF_DISTANCE = 5.0f;
                WorldPackets::BattlePet::PetBattleFinalizeLocation finalizeLocation;
                finalizeLocation.Location.LocationResult = PET_BATTLE_REQUEST_FAIL_OK;
                Position pvpMidpoint;
                pvpMidpoint.m_positionX = (p1->GetPositionX() + p2->GetPositionX()) / 2.0f;
                pvpMidpoint.m_positionY = (p1->GetPositionY() + p2->GetPositionY()) / 2.0f;
                pvpMidpoint.m_positionZ = (p1->GetPositionZ() + p2->GetPositionZ()) / 2.0f;
                float pvpFacing = p1->GetAbsoluteAngle(p2);
                finalizeLocation.Location.BattleOrigin = pvpMidpoint;
                finalizeLocation.Location.BattleFacing = pvpFacing;
                finalizeLocation.Location.PlayerPositions[0] = Position(pvpMidpoint.m_positionX - PET_BATTLE_HALF_DISTANCE * std::cos(pvpFacing),
                                                                     pvpMidpoint.m_positionY - PET_BATTLE_HALF_DISTANCE * std::sin(pvpFacing),
                                                                     pvpMidpoint.m_positionZ);
                finalizeLocation.Location.PlayerPositions[1] = Position(pvpMidpoint.m_positionX + PET_BATTLE_HALF_DISTANCE * std::cos(pvpFacing),
                                                                     pvpMidpoint.m_positionY + PET_BATTLE_HALF_DISTANCE * std::sin(pvpFacing),
                                                                     pvpMidpoint.m_positionZ);
                p1->SendDirectMessage(finalizeLocation.Write());
                p2->SendDirectMessage(finalizeLocation.Write());
            }
        }

        _pendingProposal.reset();
    }
}

void PetBattleMgr::TryMatchPlayers()
{
    if (_pendingProposal)
        return; // Already have a pending proposal

    if (_pvpQueue.size() < 2)
        return;

    // Match first two players in queue
    PvPQueueEntry p1 = _pvpQueue[0];
    PvPQueueEntry p2 = _pvpQueue[1];

    // Remove from queue
    _pvpQueue.erase(_pvpQueue.begin(), _pvpQueue.begin() + 2);

    // Validate both players still exist and aren't in battle
    Player* player1 = ObjectAccessor::FindPlayer(p1.PlayerGUID);
    Player* player2 = ObjectAccessor::FindPlayer(p2.PlayerGUID);

    if (!player1 || !player2 ||
        IsPlayerInBattle(p1.PlayerGUID) || IsPlayerInBattle(p2.PlayerGUID))
    {
        // Re-add valid players
        if (player1 && !IsPlayerInBattle(p1.PlayerGUID))
            _pvpQueue.push_back(p1);
        if (player2 && !IsPlayerInBattle(p2.PlayerGUID))
            _pvpQueue.push_back(p2);
        return;
    }

    // Create proposal
    _pendingProposal = std::make_unique<PvPMatchProposal>();
    _pendingProposal->Player1 = p1.PlayerGUID;
    _pendingProposal->Player2 = p2.PlayerGUID;

    // Send matchmaking then proposal status to both players
    for (Player* matchPlayer : { player1, player2 })
    {
        WorldPackets::BattlePet::PetBattleQueueStatus matchStatus;
        matchStatus.Status = PET_BATTLE_QUEUE_STATUS_MATCHMAKING;
        matchPlayer->SendDirectMessage(matchStatus.Write());
    }

    WorldPackets::BattlePet::PetBattleQueueProposeMatch proposeMatch;
    player1->SendDirectMessage(proposeMatch.Write());
    player2->SendDirectMessage(proposeMatch.Write());

    TC_LOG_DEBUG("server.loading", "PetBattleMgr: Proposed PvP match between {} and {}",
        p1.PlayerGUID.ToString(), p2.PlayerGUID.ToString());
}

} // namespace PetBattles
