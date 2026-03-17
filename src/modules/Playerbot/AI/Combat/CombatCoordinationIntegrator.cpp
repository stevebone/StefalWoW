/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Sprint 3: Combat Coordination Integration Layer Implementation
 */

#include "CombatCoordinationIntegrator.h"
#include "InterruptCoordinator.h"
#include "AI/CombatBehaviors/DefensiveBehaviorManager.h"
#include "CrowdControlManager.h"
#include "AI/CombatBehaviors/DispelCoordinator.h"
#include "AI/BotAI.h"
#include "Group/GroupRoleEnums.h"
#include "Player.h"
#include "Group.h"
#include "Unit.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellHistory.h"
#include "ObjectAccessor.h"
#include "GameTime.h"
#include "Log.h"

namespace Playerbot
{

// ============================================================================
// EXTERNAL CD DATABASE
// ============================================================================

static std::unordered_map<uint32, ExternalCDInfo> s_externalCDDatabase;
static bool s_databaseInitialized = false;

const std::unordered_map<uint32, ExternalCDInfo>& ExternalCDInfo::GetDatabase()
{
    if (!s_databaseInitialized)
    {
        // Major External CDs (single target, life-saving)
        s_externalCDDatabase[47788] = { 47788, ExternalCDTier::TIER_MAJOR, 180000, 10000, false, true };  // Guardian Spirit
        s_externalCDDatabase[33206] = { 33206, ExternalCDTier::TIER_MAJOR, 180000, 8000, false, true };   // Pain Suppression
        s_externalCDDatabase[102342] = { 102342, ExternalCDTier::TIER_MAJOR, 90000, 12000, false, true }; // Ironbark
        s_externalCDDatabase[116849] = { 116849, ExternalCDTier::TIER_MAJOR, 120000, 12000, false, true }; // Life Cocoon

        // Moderate External CDs
        s_externalCDDatabase[6940] = { 6940, ExternalCDTier::TIER_MODERATE, 120000, 12000, false, true }; // Blessing of Sacrifice
        s_externalCDDatabase[114030] = { 114030, ExternalCDTier::TIER_MODERATE, 120000, 12000, false, true }; // Vigilance

        // Minor/Group CDs
        s_externalCDDatabase[62618] = { 62618, ExternalCDTier::TIER_MINOR, 180000, 10000, true, false };  // Power Word: Barrier
        s_externalCDDatabase[196718] = { 196718, ExternalCDTier::TIER_MINOR, 180000, 8000, true, false }; // Darkness
        s_externalCDDatabase[51052] = { 51052, ExternalCDTier::TIER_MINOR, 120000, 10000, true, false };  // Anti-Magic Zone

        // Raid CDs
        s_externalCDDatabase[97462] = { 97462, ExternalCDTier::TIER_RAID, 180000, 10000, true, false };   // Rallying Cry
        s_externalCDDatabase[98008] = { 98008, ExternalCDTier::TIER_RAID, 180000, 6000, true, false };    // Spirit Link Totem
        s_externalCDDatabase[31821] = { 31821, ExternalCDTier::TIER_RAID, 180000, 8000, true, false };    // Aura Mastery
        s_externalCDDatabase[108280] = { 108280, ExternalCDTier::TIER_RAID, 180000, 10000, true, false }; // Healing Tide Totem

        s_databaseInitialized = true;
    }
    return s_externalCDDatabase;
}

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

CombatCoordinationIntegrator::CombatCoordinationIntegrator(BotAI* ai)
    : _ai(ai)
    , _bot(ai ? ai->GetBot() : nullptr)
{
}

CombatCoordinationIntegrator::~CombatCoordinationIntegrator()
{
    Shutdown();
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void CombatCoordinationIntegrator::Initialize(
    InterruptCoordinatorFixed* interruptCoord,
    DefensiveBehaviorManager* defensiveMgr,
    CrowdControlManager* ccMgr,
    DispelCoordinator* dispelCoord)
{
    _interruptCoord = interruptCoord;
    _defensiveMgr = defensiveMgr;
    _ccMgr = ccMgr;
    _dispelCoord = dispelCoord;

    // Ensure database is initialized
    ExternalCDInfo::GetDatabase();

    // Get group GUID for message bus subscription
    if (_bot && _bot->GetGroup())
    {
        _groupGuid = _bot->GetGroup()->GetGUID();
        SubscribeToMessageBus();
    }

    TC_LOG_DEBUG("playerbot.combat", "CombatCoordinationIntegrator::Initialize - Bot {} initialized",
        _bot ? _bot->GetGUID().ToString() : "null");
}

void CombatCoordinationIntegrator::Shutdown()
{
    UnsubscribeFromMessageBus();
    _activeClaims.clear();
    _protectionWindows.clear();

    _interruptCoord = nullptr;
    _defensiveMgr = nullptr;
    _ccMgr = nullptr;
    _dispelCoord = nullptr;
}

void CombatCoordinationIntegrator::Update(uint32 diff)
{
    if (!_bot || !_bot->IsAlive())
        return;

    uint32 now = GameTime::GetGameTimeMS();
    if (now - _lastUpdate < UPDATE_INTERVAL_MS)
        return;
    _lastUpdate = now;

    // Update protection windows (expire old ones)
    UpdateProtectionWindows();

    // Check if group changed
    if (_bot->GetGroup())
    {
        ObjectGuid newGroupGuid = _bot->GetGroup()->GetGUID();
        if (newGroupGuid != _groupGuid)
        {
            UnsubscribeFromMessageBus();
            _groupGuid = newGroupGuid;
            SubscribeToMessageBus();
        }
    }

    // Expire old claims
    auto claimIt = _activeClaims.begin();
    while (claimIt != _activeClaims.end())
    {
        auto elapsed = std::chrono::steady_clock::now() - claimIt->second.submitTime;
        if (elapsed > std::chrono::milliseconds(_config.claimTimeoutMs * 10)) // 10x timeout for cleanup
        {
            claimIt = _activeClaims.erase(claimIt);
        }
        else
        {
            ++claimIt;
        }
    }
}

// ============================================================================
// INTERRUPT COORDINATION (S3.4)
// ============================================================================

bool CombatCoordinationIntegrator::RequestInterrupt(ObjectGuid targetGuid, uint32 spellId, ClaimPriority priority)
{
    if (!_config.enableInterruptClaims || !_bot || _groupGuid.IsEmpty())
        return false;

    // Create interrupt claim message
    BotMessage msg = BotMessage::ClaimInterrupt(
        _bot->GetGUID(),
        _groupGuid,
        targetGuid,
        spellId,
        priority
    );

    // Submit through BotMessageBus
    auto callback = [this](BotMessage const& m, ClaimStatus s) {
        OnClaimResolved(m, s);
    };

    ClaimStatus status = sBotMessageBus->PublishClaim(msg, callback);

    // Track the claim
    uint64 claimKey = std::hash<ObjectGuid>{}(targetGuid) ^ (static_cast<uint64>(BotMessageType::CLAIM_INTERRUPT) << 56);
    ActiveClaim claim;
    claim.type = BotMessageType::CLAIM_INTERRUPT;
    claim.targetGuid = targetGuid;
    claim.spellOrAuraId = spellId;
    claim.status = status;
    claim.submitTime = std::chrono::steady_clock::now();
    _activeClaims[claimKey] = claim;

    _metrics.interruptClaimsSubmitted++;

    return status != ClaimStatus::DENIED;
}

bool CombatCoordinationIntegrator::ShouldInterrupt(ObjectGuid& targetGuid, uint32& spellId) const
{
    // Find active interrupt claim that was accepted
    for (auto const& [key, claim] : _activeClaims)
    {
        if (claim.type == BotMessageType::CLAIM_INTERRUPT &&
            claim.status == ClaimStatus::GRANTED)
        {
            targetGuid = claim.targetGuid;
            spellId = GetInterruptSpell();
            return spellId != 0;
        }
    }
    return false;
}

void CombatCoordinationIntegrator::OnInterruptExecuted(ObjectGuid targetGuid, uint32 spellId, bool success)
{
    // Remove the claim
    uint64 claimKey = std::hash<ObjectGuid>{}(targetGuid) ^ (static_cast<uint64>(BotMessageType::CLAIM_INTERRUPT) << 56);
    _activeClaims.erase(claimKey);

    // Announce via message bus
    if (_bot && !_groupGuid.IsEmpty())
    {
        BotMessage msg;
        msg.type = BotMessageType::ANNOUNCE_CD_USAGE;
        msg.senderGuid = _bot->GetGUID();
        msg.groupGuid = _groupGuid;
        msg.targetGuid = targetGuid;
        msg.spellId = spellId;
        msg.timestamp = std::chrono::steady_clock::now();

        sBotMessageBus->Publish(msg);
    }
}

// ============================================================================
// DISPEL COORDINATION (S3.2 - GAP 2 Fix)
// ============================================================================

bool CombatCoordinationIntegrator::RequestDispel(ObjectGuid targetGuid, uint32 auraId, ClaimPriority priority)
{
    if (!_config.enableDispelClaims || !_bot || _groupGuid.IsEmpty())
        return false;

    BotMessage msg = BotMessage::ClaimDispel(
        _bot->GetGUID(),
        _groupGuid,
        targetGuid,
        auraId,
        priority
    );

    auto callback = [this](BotMessage const& m, ClaimStatus s) {
        OnClaimResolved(m, s);
    };

    ClaimStatus status = sBotMessageBus->PublishClaim(msg, callback);

    uint64 claimKey = std::hash<ObjectGuid>{}(targetGuid) ^ (static_cast<uint64>(BotMessageType::CLAIM_DISPEL) << 56) ^ auraId;
    ActiveClaim claim;
    claim.type = BotMessageType::CLAIM_DISPEL;
    claim.targetGuid = targetGuid;
    claim.spellOrAuraId = auraId;
    claim.status = status;
    claim.submitTime = std::chrono::steady_clock::now();
    _activeClaims[claimKey] = claim;

    _metrics.dispelClaimsSubmitted++;

    return status != ClaimStatus::DENIED;
}

bool CombatCoordinationIntegrator::ShouldDispel(ObjectGuid& targetGuid, uint32& auraId) const
{
    for (auto const& [key, claim] : _activeClaims)
    {
        if (claim.type == BotMessageType::CLAIM_DISPEL &&
            claim.status == ClaimStatus::GRANTED)
        {
            targetGuid = claim.targetGuid;
            auraId = claim.spellOrAuraId;
            return true;
        }
    }
    return false;
}

void CombatCoordinationIntegrator::OnDispelExecuted(ObjectGuid targetGuid, uint32 auraId, bool success)
{
    uint64 claimKey = std::hash<ObjectGuid>{}(targetGuid) ^ (static_cast<uint64>(BotMessageType::CLAIM_DISPEL) << 56) ^ auraId;
    _activeClaims.erase(claimKey);
}

float CombatCoordinationIntegrator::CalculateDispelPriority(ObjectGuid targetGuid, uint32 auraId) const
{
    if (!_bot)
        return 0.0f;

    float priority = 0.0f;

    // Base priority from aura danger
    if (_dispelCoord)
    {
        if (_dispelCoord->ShouldDispel(auraId))
            priority += 50.0f;
    }

    // Can dispel type bonus (+100)
    SpellInfo const* auraSpell = sSpellMgr->GetSpellInfo(auraId, DIFFICULTY_NONE);
    if (auraSpell && CanDispelType(auraSpell->Dispel))
        priority += 100.0f;

    // Distance bonus (+50 if in range)
    float distance = GetDistanceToTarget(targetGuid);
    if (distance < 40.0f)
        priority += 50.0f;

    // CD available bonus (+200 if not on CD)
    // This would check dispel cooldown

    // Healer bonus (+30)
    if (_bot && IsPlayerHealer(_bot))
        priority += 30.0f;

    // GCD free bonus (+20)
    // Would check GCD state

    return priority;
}

// ============================================================================
// EXTERNAL DEFENSIVE CD COORDINATION (S3.3 - GAP 3 Fix)
// ============================================================================

bool CombatCoordinationIntegrator::RequestExternalDefensive(ObjectGuid targetGuid, DangerLevel danger)
{
    if (!_config.enableDefensiveClaims || !_bot || _groupGuid.IsEmpty())
        return false;

    // Don't request if target is already protected
    if (IsTargetProtected(targetGuid))
        return false;

    // Determine appropriate CD tier for danger level
    ExternalCDTier maxTier = GetAppropriateCDTier(danger);

    // Get available CDs we can provide
    std::vector<uint32> availableCDs = GetAvailableExternalCDs(maxTier);
    if (availableCDs.empty())
        return false;

    // Select best CD for the situation
    uint32 cdSpellId = availableCDs[0]; // Simple: use first available

    ClaimPriority priority = ClaimPriority::MEDIUM;
    if (danger >= DangerLevel::CRITICAL)
        priority = ClaimPriority::CRITICAL;
    else if (danger >= DangerLevel::HIGH)
        priority = ClaimPriority::HIGH;

    BotMessage msg = BotMessage::ClaimDefensiveCD(
        _bot->GetGUID(),
        _groupGuid,
        targetGuid,
        cdSpellId,
        priority
    );

    auto callback = [this](BotMessage const& m, ClaimStatus s) {
        OnClaimResolved(m, s);
    };

    ClaimStatus status = sBotMessageBus->PublishClaim(msg, callback);

    uint64 claimKey = std::hash<ObjectGuid>{}(targetGuid) ^ (static_cast<uint64>(BotMessageType::CLAIM_DEFENSIVE_CD) << 56);
    ActiveClaim claim;
    claim.type = BotMessageType::CLAIM_DEFENSIVE_CD;
    claim.targetGuid = targetGuid;
    claim.spellOrAuraId = cdSpellId;
    claim.status = status;
    claim.submitTime = std::chrono::steady_clock::now();
    _activeClaims[claimKey] = claim;

    _metrics.defensiveClaimsSubmitted++;

    return status != ClaimStatus::DENIED;
}

bool CombatCoordinationIntegrator::ShouldProvideExternalCD(ObjectGuid& targetGuid, uint32& spellId) const
{
    for (auto const& [key, claim] : _activeClaims)
    {
        if (claim.type == BotMessageType::CLAIM_DEFENSIVE_CD &&
            claim.status == ClaimStatus::GRANTED)
        {
            targetGuid = claim.targetGuid;
            spellId = claim.spellOrAuraId;
            return true;
        }
    }
    return false;
}

void CombatCoordinationIntegrator::OnExternalCDUsed(ObjectGuid targetGuid, uint32 spellId)
{
    // Remove claim
    uint64 claimKey = std::hash<ObjectGuid>{}(targetGuid) ^ (static_cast<uint64>(BotMessageType::CLAIM_DEFENSIVE_CD) << 56);
    _activeClaims.erase(claimKey);

    // Create protection window (GAP 3 fix)
    auto& db = ExternalCDInfo::GetDatabase();
    auto it = db.find(spellId);

    ProtectionWindow window;
    window.targetGuid = targetGuid;
    window.protectorGuid = _bot ? _bot->GetGUID() : ObjectGuid::Empty;
    window.spellId = spellId;
    window.tier = (it != db.end()) ? it->second.tier : ExternalCDTier::TIER_MINOR;
    window.startTime = std::chrono::steady_clock::now();
    window.endTime = window.startTime + std::chrono::milliseconds(_config.protectionWindowMs);

    _protectionWindows.push_back(window);
    _metrics.protectionWindowsCreated++;

    // Announce CD usage
    if (_bot && !_groupGuid.IsEmpty())
    {
        BotMessage msg;
        msg.type = BotMessageType::ANNOUNCE_CD_USAGE;
        msg.senderGuid = _bot->GetGUID();
        msg.groupGuid = _groupGuid;
        msg.targetGuid = targetGuid;
        msg.spellId = spellId;
        msg.timestamp = std::chrono::steady_clock::now();

        sBotMessageBus->Publish(msg);
    }
}

bool CombatCoordinationIntegrator::IsTargetProtected(ObjectGuid targetGuid) const
{
    for (auto const& window : _protectionWindows)
    {
        if (window.targetGuid == targetGuid && window.IsActive())
            return true;
    }
    return false;
}

uint32 CombatCoordinationIntegrator::GetProtectionRemaining(ObjectGuid targetGuid) const
{
    for (auto const& window : _protectionWindows)
    {
        if (window.targetGuid == targetGuid && window.IsActive())
            return window.GetRemainingMs();
    }
    return 0;
}

DangerLevel CombatCoordinationIntegrator::AssessDanger(ObjectGuid targetGuid) const
{
    Unit* target = ObjectAccessor::GetUnit(*_bot, targetGuid);
    if (!target || !target->IsAlive())
        return DangerLevel::NONE;

    float healthPct = target->GetHealthPct() / 100.0f;

    if (healthPct < _config.dangerHealthCritical)
        return DangerLevel::CRITICAL;
    if (healthPct < _config.dangerHealthHigh)
        return DangerLevel::HIGH;
    if (healthPct < _config.dangerHealthModerate)
        return DangerLevel::MODERATE;

    return DangerLevel::NONE;
}

ExternalCDTier CombatCoordinationIntegrator::GetAppropriateCDTier(DangerLevel danger) const
{
    // Per GAP 3: Don't waste major CDs on moderate danger
    switch (danger)
    {
        case DangerLevel::CRITICAL:
        case DangerLevel::PRE_DANGER:
            return ExternalCDTier::TIER_MAJOR;
        case DangerLevel::HIGH:
            return ExternalCDTier::TIER_MODERATE;
        case DangerLevel::MODERATE:
            return ExternalCDTier::TIER_MINOR;
        default:
            return ExternalCDTier::TIER_MINOR;
    }
}

// ============================================================================
// CC COORDINATION (S3.6)
// ============================================================================

bool CombatCoordinationIntegrator::RequestCC(ObjectGuid targetGuid, uint32 spellId, ClaimPriority priority)
{
    if (!_config.enableCCClaims || !_bot || _groupGuid.IsEmpty())
        return false;

    BotMessage msg = BotMessage::ClaimCC(
        _bot->GetGUID(),
        _groupGuid,
        targetGuid,
        spellId,
        priority
    );

    auto callback = [this](BotMessage const& m, ClaimStatus s) {
        OnClaimResolved(m, s);
    };

    ClaimStatus status = sBotMessageBus->PublishClaim(msg, callback);

    uint64 claimKey = std::hash<ObjectGuid>{}(targetGuid) ^ (static_cast<uint64>(BotMessageType::CLAIM_CC) << 56);
    ActiveClaim claim;
    claim.type = BotMessageType::CLAIM_CC;
    claim.targetGuid = targetGuid;
    claim.spellOrAuraId = spellId;
    claim.status = status;
    claim.submitTime = std::chrono::steady_clock::now();
    _activeClaims[claimKey] = claim;

    _metrics.ccClaimsSubmitted++;

    return status != ClaimStatus::DENIED;
}

bool CombatCoordinationIntegrator::ShouldCC(ObjectGuid& targetGuid, uint32& spellId) const
{
    for (auto const& [key, claim] : _activeClaims)
    {
        if (claim.type == BotMessageType::CLAIM_CC &&
            claim.status == ClaimStatus::GRANTED)
        {
            targetGuid = claim.targetGuid;
            spellId = claim.spellOrAuraId;
            return true;
        }
    }
    return false;
}

void CombatCoordinationIntegrator::OnCCExecuted(ObjectGuid targetGuid, uint32 spellId, bool success)
{
    uint64 claimKey = std::hash<ObjectGuid>{}(targetGuid) ^ (static_cast<uint64>(BotMessageType::CLAIM_CC) << 56);
    _activeClaims.erase(claimKey);

    // Update CC manager's DR tracking
    if (_ccMgr && success)
    {
        _ccMgr->OnCCApplied(targetGuid, spellId);
    }
}

// ============================================================================
// CLAIM CALLBACKS
// ============================================================================

void CombatCoordinationIntegrator::OnClaimResolved(BotMessage const& message, ClaimStatus status)
{
    uint64 claimKey = 0;

    switch (message.type)
    {
        case BotMessageType::CLAIM_INTERRUPT:
            claimKey = std::hash<ObjectGuid>{}(message.targetGuid) ^ (static_cast<uint64>(BotMessageType::CLAIM_INTERRUPT) << 56);
            if (status == ClaimStatus::GRANTED)
                _metrics.interruptClaimsWon++;
            else
                _metrics.interruptClaimsLost++;
            break;

        case BotMessageType::CLAIM_DISPEL:
            claimKey = std::hash<ObjectGuid>{}(message.targetGuid) ^ (static_cast<uint64>(BotMessageType::CLAIM_DISPEL) << 56) ^ message.auraId;
            if (status == ClaimStatus::GRANTED)
                _metrics.dispelClaimsWon++;
            else
                _metrics.dispelClaimsLost++;
            break;

        case BotMessageType::CLAIM_DEFENSIVE_CD:
            claimKey = std::hash<ObjectGuid>{}(message.targetGuid) ^ (static_cast<uint64>(BotMessageType::CLAIM_DEFENSIVE_CD) << 56);
            if (status == ClaimStatus::GRANTED)
                _metrics.defensiveClaimsWon++;
            else
                _metrics.defensiveClaimsLost++;
            break;

        case BotMessageType::CLAIM_CC:
            claimKey = std::hash<ObjectGuid>{}(message.targetGuid) ^ (static_cast<uint64>(BotMessageType::CLAIM_CC) << 56);
            if (status == ClaimStatus::GRANTED)
                _metrics.ccClaimsWon++;
            else
                _metrics.ccClaimsLost++;
            break;

        default:
            return;
    }

    // Update claim status
    auto it = _activeClaims.find(claimKey);
    if (it != _activeClaims.end())
    {
        it->second.status = status;
        it->second.resolveTime = std::chrono::steady_clock::now();
    }
}

// ============================================================================
// INTERNAL METHODS
// ============================================================================

void CombatCoordinationIntegrator::SubscribeToMessageBus()
{
    if (_subscribed || _groupGuid.IsEmpty() || !_bot)
        return;

    uint8 role = _ai ? static_cast<uint8>(GetPlayerSpecRole(_bot)) : 0;
    uint8 subgroup = _bot->GetSubGroup();

    std::vector<BotMessageType> types; // empty = subscribe to all message types
    sBotMessageBus->Subscribe(_ai, _groupGuid, types, role, subgroup);

    _subscribed = true;
}

void CombatCoordinationIntegrator::UnsubscribeFromMessageBus()
{
    if (!_subscribed)
        return;

    if (_bot)
        sBotMessageBus->Unsubscribe(_bot->GetGUID(), _groupGuid);
    _subscribed = false;
}

void CombatCoordinationIntegrator::OnBotMessage(BotMessage const& message)
{
    // Handle incoming messages from other bots
    switch (message.type)
    {
        case BotMessageType::ANNOUNCE_CD_USAGE:
            // Track that another bot used a CD - could inform our CD planning
            break;

        case BotMessageType::REQUEST_HEAL:
            // Could trigger external defensive if we have one
            break;

        case BotMessageType::REQUEST_EXTERNAL_CD:
            // Another bot is requesting external CD
            {
                DangerLevel danger = AssessDanger(message.targetGuid);
                if (danger >= DangerLevel::HIGH)
                {
                    RequestExternalDefensive(message.targetGuid, danger);
                }
            }
            break;

        default:
            break;
    }
}

void CombatCoordinationIntegrator::UpdateProtectionWindows()
{
    auto it = _protectionWindows.begin();
    while (it != _protectionWindows.end())
    {
        if (!it->IsActive())
        {
            it = _protectionWindows.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

std::vector<uint32> CombatCoordinationIntegrator::GetAvailableExternalCDs(ExternalCDTier maxTier) const
{
    std::vector<uint32> result;

    if (!_bot)
        return result;

    auto const& db = ExternalCDInfo::GetDatabase();
    for (auto const& [spellId, info] : db)
    {
        // Check tier
        if (static_cast<uint8>(info.tier) > static_cast<uint8>(maxTier))
            continue;

        // Check if bot knows spell and it's not on cooldown
        if (_bot->HasSpell(spellId) && !_bot->GetSpellHistory()->HasCooldown(spellId))
        {
            result.push_back(spellId);
        }
    }

    return result;
}

bool CombatCoordinationIntegrator::CanDispelType(uint32 dispelMask) const
{
    if (!_bot)
        return false;

    Classes botClass = static_cast<Classes>(_bot->GetClass());

    // Check by class what dispel types are available
    switch (botClass)
    {
        case CLASS_PRIEST:
            // Priests can dispel Magic, Disease
            return (dispelMask == DISPEL_MAGIC) || (dispelMask == DISPEL_DISEASE);
        case CLASS_PALADIN:
            // Paladins can dispel Disease, Poison, Magic (if Holy)
            return (dispelMask == DISPEL_DISEASE) || (dispelMask == DISPEL_POISON) || (dispelMask == DISPEL_MAGIC);
        case CLASS_DRUID:
            // Druids can dispel Curse, Poison, Magic (if Resto)
            return (dispelMask == DISPEL_CURSE) || (dispelMask == DISPEL_POISON) || (dispelMask == DISPEL_MAGIC);
        case CLASS_SHAMAN:
            // Shamans can dispel Curse, Magic (if Resto)
            return (dispelMask == DISPEL_CURSE) || (dispelMask == DISPEL_MAGIC);
        case CLASS_MAGE:
            // Mages can dispel Curse
            return (dispelMask == DISPEL_CURSE);
        case CLASS_MONK:
            // Monks can dispel Disease, Poison, Magic (if Mistweaver)
            return (dispelMask == DISPEL_DISEASE) || (dispelMask == DISPEL_POISON) || (dispelMask == DISPEL_MAGIC);
        case CLASS_EVOKER:
            // Evokers can dispel Magic, Poison (Preservation)
            return (dispelMask == DISPEL_MAGIC) || (dispelMask == DISPEL_POISON);
        default:
            return false;
    }
}

uint32 CombatCoordinationIntegrator::GetInterruptSpell() const
{
    if (!_bot)
        return 0;

    Classes botClass = static_cast<Classes>(_bot->GetClass());

    // Return primary interrupt spell by class
    switch (botClass)
    {
        case CLASS_WARRIOR: return 6552;    // Pummel
        case CLASS_PALADIN: return 96231;   // Rebuke
        case CLASS_HUNTER: return 147362;   // Counter Shot
        case CLASS_ROGUE: return 1766;      // Kick
        case CLASS_PRIEST: return 0;        // No interrupt (Silence is 15487 but long CD)
        case CLASS_DEATH_KNIGHT: return 47528; // Mind Freeze
        case CLASS_SHAMAN: return 57994;    // Wind Shear
        case CLASS_MAGE: return 2139;       // Counterspell
        case CLASS_WARLOCK: return 119910;  // Spell Lock (pet)
        case CLASS_MONK: return 116705;     // Spear Hand Strike
        case CLASS_DRUID: return 106839;    // Skull Bash
        case CLASS_DEMON_HUNTER: return 183752; // Disrupt
        case CLASS_EVOKER: return 351338;   // Quell
        default: return 0;
    }
}

float CombatCoordinationIntegrator::GetDistanceToTarget(ObjectGuid targetGuid) const
{
    if (!_bot)
        return 999.0f;

    Unit* target = ObjectAccessor::GetUnit(*_bot, targetGuid);
    if (!target)
        return 999.0f;

    return _bot->GetDistance(target);
}

} // namespace Playerbot
