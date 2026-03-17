/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 */

#include "RaidCooldownRotation.h"
#include "RaidCoordinator.h"
#include "Player.h"
#include "Log.h"

namespace Playerbot {

// Bloodlust spell IDs
static constexpr uint32 SPELL_BLOODLUST = 2825;
static constexpr uint32 SPELL_HEROISM = 32182;
static constexpr uint32 SPELL_TIME_WARP = 80353;

// Raid defensive spell IDs
static constexpr uint32 SPELL_SPIRIT_LINK = 98008;
static constexpr uint32 SPELL_RALLYING_CRY = 97462;
static constexpr uint32 SPELL_AURA_MASTERY = 31821;

RaidCooldownRotation::RaidCooldownRotation(RaidCoordinator* coordinator)
    : _coordinator(coordinator)
{
}

void RaidCooldownRotation::Initialize()
{
    Reset();
    BuildCooldownList();
    TC_LOG_DEBUG("playerbots.raid", "RaidCooldownRotation::Initialize - Initialized with %zu cooldowns",
        _cooldowns.size());
}

void RaidCooldownRotation::Update(uint32 diff)
{
    UpdateCooldowns(diff);
}

void RaidCooldownRotation::Reset()
{
    _cooldowns.clear();
    _cooldownPlan.clear();
    _bloodlustUsed = false;
    _battleRezCharges = _maxBattleRezCharges;
}

void RaidCooldownRotation::UseBloodlust()
{
    if (_bloodlustUsed)
        return;

    ObjectGuid provider = GetBloodlustProvider();
    if (provider.IsEmpty())
        return;

    TC_LOG_DEBUG("playerbots.raid", "RaidCooldownRotation::UseBloodlust - Bloodlust used!");
    _bloodlustUsed = true;
}

bool RaidCooldownRotation::ShouldUseBloodlust() const
{
    if (_bloodlustUsed)
        return false;

    float bossHealth = _coordinator->GetBossHealthPercent();
    return bossHealth <= _bloodlustThreshold;
}

bool RaidCooldownRotation::IsBloodlustAvailable() const
{
    return !_bloodlustUsed && !GetBloodlustProvider().IsEmpty();
}

ObjectGuid RaidCooldownRotation::GetBloodlustProvider() const
{
    for (const auto& cd : _cooldowns)
    {
        if (cd.type == CooldownType::BLOODLUST && cd.isAvailable)
        {
            Player* player = _coordinator->GetPlayer(cd.playerGuid);
            if (player && player->IsAlive())
                return cd.playerGuid;
        }
    }
    return ObjectGuid();
}

void RaidCooldownRotation::UseRaidDefensive()
{
    ObjectGuid provider = GetNextRaidDefensiveProvider();
    if (provider.IsEmpty())
        return;

    RaidCooldownEntry* cd = FindCooldown(provider, CooldownType::RAID_DEFENSIVE);
    if (cd)
    {
        cd->isAvailable = false;
        cd->remainingCooldown = cd->cooldownDuration;
    }

    TC_LOG_DEBUG("playerbots.raid", "RaidCooldownRotation::UseRaidDefensive - Defensive used!");
}

bool RaidCooldownRotation::ShouldUseRaidDefensive() const
{
    float raidHealth = _coordinator->GetRaidHealthPercent();
    return raidHealth <= _defensiveThreshold;
}

ObjectGuid RaidCooldownRotation::GetNextRaidDefensiveProvider() const
{
    for (const auto& cd : _cooldowns)
    {
        if (cd.type == CooldownType::RAID_DEFENSIVE && cd.isAvailable)
        {
            Player* player = _coordinator->GetPlayer(cd.playerGuid);
            if (player && player->IsAlive())
                return cd.playerGuid;
        }
    }
    return ObjectGuid();
}

std::vector<ObjectGuid> RaidCooldownRotation::GetAvailableRaidDefensiveProviders() const
{
    std::vector<ObjectGuid> providers;
    for (const auto& cd : _cooldowns)
    {
        if (cd.type == CooldownType::RAID_DEFENSIVE && cd.isAvailable)
        {
            Player* player = _coordinator->GetPlayer(cd.playerGuid);
            if (player && player->IsAlive())
                providers.push_back(cd.playerGuid);
        }
    }
    return providers;
}

void RaidCooldownRotation::UseBattleRez(ObjectGuid /*target*/)
{
    if (_battleRezCharges == 0)
        return;

    ObjectGuid provider = GetBattleRezProvider();
    if (provider.IsEmpty())
        return;

    _battleRezCharges--;
    TC_LOG_DEBUG("playerbots.raid", "RaidCooldownRotation::UseBattleRez - Battle rez used, %u remaining",
        _battleRezCharges);
}

bool RaidCooldownRotation::HasBattleRezAvailable() const
{
    return _battleRezCharges > 0 && !GetBattleRezProvider().IsEmpty();
}

uint32 RaidCooldownRotation::GetBattleRezCharges() const
{
    return _battleRezCharges;
}

ObjectGuid RaidCooldownRotation::GetBattleRezProvider() const
{
    for (const auto& cd : _cooldowns)
    {
        if (cd.type == CooldownType::BATTLE_REZ && cd.isAvailable)
        {
            Player* player = _coordinator->GetPlayer(cd.playerGuid);
            if (player && player->IsAlive())
                return cd.playerGuid;
        }
    }
    return ObjectGuid();
}

void RaidCooldownRotation::LoadCooldownPlan(const ::std::vector<CooldownPlan>& plan)
{
    _cooldownPlan = plan;
}

void RaidCooldownRotation::ClearCooldownPlan()
{
    _cooldownPlan.clear();
}

void RaidCooldownRotation::OnPhaseChange(uint8 phase)
{
    for (auto& plan : _cooldownPlan)
    {
        if (!plan.used && plan.phaseOrTime == phase)
        {
            if (plan.type == CooldownType::BLOODLUST)
                UseBloodlust();
            else if (plan.type == CooldownType::RAID_DEFENSIVE)
                UseRaidDefensive();

            plan.used = true;
        }
    }
}

void RaidCooldownRotation::OnSpellEvent(const CombatEvent& event)
{
    if (event.type != CombatEventType::SPELL_CAST_SUCCESS)
        return;

    if (event.spellId == SPELL_BLOODLUST || event.spellId == SPELL_HEROISM ||
        event.spellId == SPELL_TIME_WARP)
    {
        _bloodlustUsed = true;
    }

    for (auto& cd : _cooldowns)
    {
        if (cd.playerGuid == event.source && cd.spellId == event.spellId)
        {
            cd.isAvailable = false;
            cd.remainingCooldown = cd.cooldownDuration;
        }
    }
}

void RaidCooldownRotation::BuildCooldownList()
{
    _cooldowns.clear();

    for (ObjectGuid guid : _coordinator->GetAllMembers())
    {
        Player* player = _coordinator->GetPlayer(guid);
        if (!player)
            continue;

        uint8 playerClass = player->GetClass();

        // Bloodlust providers (Shaman, Mage, Hunter)
        if (playerClass == 7 || playerClass == 8 || playerClass == 3)
        {
            RaidCooldownEntry cd;
            cd.playerGuid = guid;
            cd.type = CooldownType::BLOODLUST;
            cd.cooldownDuration = 300000; // 5 min
            cd.isAvailable = true;
            _cooldowns.push_back(cd);
        }

        // Raid defensive providers
        if (playerClass == 7) // Shaman - Spirit Link
        {
            RaidCooldownEntry cd;
            cd.playerGuid = guid;
            cd.spellId = SPELL_SPIRIT_LINK;
            cd.type = CooldownType::RAID_DEFENSIVE;
            cd.cooldownDuration = 180000;
            cd.isAvailable = true;
            _cooldowns.push_back(cd);
        }
        if (playerClass == 1) // Warrior - Rallying Cry
        {
            RaidCooldownEntry cd;
            cd.playerGuid = guid;
            cd.spellId = SPELL_RALLYING_CRY;
            cd.type = CooldownType::RAID_DEFENSIVE;
            cd.cooldownDuration = 180000;
            cd.isAvailable = true;
            _cooldowns.push_back(cd);
        }

        // Battle rez providers (Druid, DK, Warlock, Hunter)
        if (playerClass == 11 || playerClass == 6 || playerClass == 9 || playerClass == 3)
        {
            RaidCooldownEntry cd;
            cd.playerGuid = guid;
            cd.type = CooldownType::BATTLE_REZ;
            cd.isAvailable = true;
            _cooldowns.push_back(cd);
        }
    }
}

void RaidCooldownRotation::UpdateCooldowns(uint32 diff)
{
    for (auto& cd : _cooldowns)
    {
        if (cd.remainingCooldown > 0)
        {
            if (cd.remainingCooldown > diff)
                cd.remainingCooldown -= diff;
            else
            {
                cd.remainingCooldown = 0;
                cd.isAvailable = true;
            }
        }
    }
}

RaidCooldownEntry* RaidCooldownRotation::FindCooldown(ObjectGuid player, CooldownType type)
{
    for (auto& cd : _cooldowns)
    {
        if (cd.playerGuid == player && cd.type == type)
            return &cd;
    }
    return nullptr;
}

} // namespace Playerbot
