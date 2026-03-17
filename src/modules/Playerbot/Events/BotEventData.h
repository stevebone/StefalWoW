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

#ifndef TRINITYCORE_BOT_EVENT_DATA_H
#define TRINITYCORE_BOT_EVENT_DATA_H

#include "Define.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <cstdint>
#include <string>
#include <variant>
#include <optional>

namespace Playerbot
{
namespace Events
{
    /**
     * @brief Specialized event data structures for type-safe event payloads
     *
     * Each event type has a corresponding data structure that carries
     * the specific information needed for that event.
     */

    // ========================================================================
    // LOOT EVENT DATA
    // ========================================================================

    struct LootRollData
    {
        uint32 itemEntry = 0;
        uint32 itemCount = 1;
        ObjectGuid lootGuid;
        uint8 rollType = 0;  // 0=pass, 1=need, 2=greed, 3=disenchant
        bool won = false;
    };

    struct LootReceivedData
    {
        uint32 itemEntry = 0;
        uint32 itemCount = 1;
        uint32 itemQuality = 0;
        ObjectGuid sourceGuid;  // Creature/chest that dropped it
        bool isPersonalLoot = true;
    };

    struct CurrencyGainedData
    {
        uint32 currencyId = 0;
        uint32 amount = 0;
        ::std::string currencyName;
    };

    // ========================================================================
    // AURA EVENT DATA
    // ========================================================================

    struct AuraEventData
    {
        uint32 spellId = 0;
        ObjectGuid casterGuid;
        ObjectGuid targetGuid;
        uint8 stackCount = 1;
        uint32 duration = 0;      // Milliseconds
        uint32 maxDuration = 0;
        bool isBuff = true;
        bool isDispellable = false;
        uint32 dispelType = 0;    // Magic, Curse, Poison, Disease, etc.
    };

    struct CCEventData
    {
        uint32 spellId = 0;
        ObjectGuid casterGuid;
        uint32 duration = 0;
        uint32 ccType = 0;  // Stun=1, Fear=2, Charm=3, Poly=4, etc.
        bool isDiminished = false;
    };

    struct InterruptData
    {
        ObjectGuid casterGuid;
        uint32 spellBeingCast = 0;
        uint32 castTimeRemaining = 0;  // MS
        bool isChanneled = false;
        uint8 interruptPriority = 100;  // 0-255, higher = more important
    };

    // ========================================================================
    // DEATH & RESURRECTION EVENT DATA
    // ========================================================================

    struct DeathEventData
    {
        ObjectGuid killerGuid;
        uint32 killingSpellId = 0;
        bool isInInstance = false;
        bool canReleaseSpirit = true;
        uint32 durabilityLoss = 10;  // Percentage
    };

    struct ResurrectionEventData
    {
        ObjectGuid resserGuid;
        uint32 resurrectionSpellId = 0;
        uint8 healthPercent = 100;
        uint8 manaPercent = 100;
        bool isBattleRez = false;
        bool isSoulstone = false;
        bool isAnkh = false;
    };

    // ========================================================================
    // INSTANCE & DUNGEON EVENT DATA
    // ========================================================================

    struct InstanceEventData
    {
        uint32 mapId = 0;
        uint32 instanceId = 0;
        uint32 difficulty = 0;
        bool isRaid = false;
        bool isMythicPlus = false;
        uint8 keystoneLevel = 0;
    };

    struct BossEventData
    {
        ObjectGuid bossGuid;
        uint32 creatureEntry = 0;
        ::std::string bossName;
        uint8 healthPercent = 100;
        uint32 encounterTime = 0;  // Milliseconds since engage
    };

    struct MythicPlusData
    {
        uint8 keystoneLevel = 0;
        uint32 timeLimit = 0;       // Seconds
        uint32 timeElapsed = 0;
        uint32 deathCount = 0;
        uint32 timeAdded = 0;       // Seconds added from deaths
        ::std::vector<uint32> activeAffixes;
        bool isUpgrade = false;     // +1, +2, or +3
    };

    struct RaidMarkerData
    {
        uint8 markerIndex = 0;      // 0-7 (skull, cross, square, etc.)
        ObjectGuid targetGuid;
        Position worldPosition;
        bool isWorldMarker = false;
    };

    // ========================================================================
    // RESOURCE MANAGEMENT EVENT DATA
    // ========================================================================

    struct ResourceEventData
    {
        uint8 resourceType = 0;     // 0=health, 1=mana, 2=rage, 3=energy, etc.
        uint32 currentAmount = 0;
        uint32 maxAmount = 0;
        int32 changeAmount = 0;     // Positive=gain, negative=loss
        uint8 percentRemaining = 100;
    };

    struct ComboPointsData
    {
        uint8 currentPoints = 0;
        uint8 maxPoints = 5;
        ObjectGuid targetGuid;
    };

    struct RunesData
    {
        uint8 bloodRunes = 0;
        uint8 frostRunes = 0;
        uint8 unholyRunes = 0;
        uint32 runicPower = 0;
    };

    // ========================================================================
    // WAR WITHIN SPECIFIC EVENT DATA
    // ========================================================================

    struct DelveEventData
    {
        uint32 delveId = 0;
        uint8 tier = 1;             // 1-11
        uint32 objectivesComplete = 0;
        uint32 objectivesTotal = 0;
        bool hasZekvir = false;
        uint32 brannLevel = 1;
    };

    struct HeroTalentData
    {
        uint32 talentId = 0;
        uint32 heroTreeId = 0;
        ::std::string heroTreeName;   // e.g., "Deathbringer" for DK
        uint8 pointsSpent = 0;
    };

    struct WarbandData
    {
        uint32 achievementId = 0;
        uint32 reputationId = 0;
        uint32 reputationGain = 0;
        ::std::string factionName;
    };

    // ========================================================================
    // SOCIAL & COMMUNICATION EVENT DATA
    // ========================================================================

    struct ChatEventData
    {
        ObjectGuid senderGuid;
        ::std::string senderName;
        ::std::string message;
        uint8 chatType = 0;         // 0=say, 1=whisper, 2=party, 3=raid
        uint8 language = 0;
    };

    struct CommandEventData
    {
        ObjectGuid commanderGuid;   // Who issued the command
        ::std::string command;
        ::std::vector<::std::string> args;
        uint32 commandId = 0;       // For command tracking
    };

    struct GroupEventData
    {
        ObjectGuid leaderGuid;
        uint8 memberCount = 0;
        uint8 maxMembers = 5;           // 5 for party, 40 for raid
        bool isRaid = false;
        uint32 groupId = 0;
    };

    struct GuildEventData
    {
        ObjectGuid guildGuid;
        ::std::string guildName;
        uint32 guildId = 0;
        uint8 guildRank = 0;
        uint32 guildMemberCount = 0;
    };

    struct FriendEventData
    {
        ObjectGuid friendGuid;
        ::std::string friendName;
        bool isOnline = false;
        uint8 level = 0;
        uint32 zoneId = 0;
    };

    struct EmoteEventData
    {
        uint32 emoteId = 0;
        ObjectGuid targetGuid;
        ::std::string emoteName;
        bool isTextEmote = false;
    };

    // ========================================================================
    // EQUIPMENT & INVENTORY EVENT DATA
    // ========================================================================

    struct ItemEventData
    {
        uint32 itemEntry = 0;
        uint32 itemCount = 1;
        ObjectGuid itemGuid;
        uint8 slot = 255;           // Equipment slot
        uint32 itemLevel = 0;
        uint32 quality = 0;
        bool isBetterThanEquipped = false;
    };

    struct UpgradeEventData
    {
        ObjectGuid itemGuid;
        uint32 oldItemLevel = 0;
        uint32 newItemLevel = 0;
        uint32 upgradeItemUsed = 0;  // Crest entry
    };

    // ========================================================================
    // COMBAT DAMAGE & THREAT EVENT DATA
    // ========================================================================

    struct DamageEventData
    {
        uint32 amount = 0;
        uint32 spellId = 0;
        bool isCrit = false;
        uint32 overkill = 0;
    };

    struct ThreatEventData
    {
        float threatAmount = 0.0f;
        bool isTanking = false;
    };

    // ========================================================================
    // QUEST EVENT DATA (Phase 6.1)
    // ========================================================================

    struct QuestEventData
    {
        uint32 questId = 0;
        uint32 objectiveIndex = 0;
        uint32 objectiveCount = 0;
        uint32 objectiveRequired = 0;
        bool isComplete = false;
        bool isDaily = false;
        bool isWeekly = false;
        uint32 rewardItemId = 0;
        uint32 experienceGained = 0;
        uint32 goldReward = 0;
        uint32 reputationGained = 0;
        uint32 chainId = 0;
        uint32 nextQuestId = 0;
    };

    // ========================================================================
    // MOVEMENT EVENT DATA (Phase 6.2)
    // ========================================================================

    struct MovementEventData
    {
        Position oldPosition;
        Position newPosition;
        float distance = 0.0f;
        float velocity = 0.0f;          // Yards per second
        uint32 movementFlags = 0;       // MovementFlags
        bool isSignificant = false;     // Movement > 0.5 yards
    };

    struct PathfindingEventData
    {
        Position startPos;
        Position endPos;
        uint32 pathLength = 0;          // Number of waypoints
        uint32 generationTime = 0;      // Milliseconds
        bool pathComplete = false;
        bool pathFailed = false;
        uint32 failureReason = 0;       // 0=success, 1=no path, 2=too far, 3=invalid target
    };

    struct StuckEventData
    {
        Position stuckPosition;
        uint32 stuckDuration = 0;       // Milliseconds stuck
        uint32 consecutiveDetections = 0;
        float distanceMoved = 0.0f;     // Total distance moved while stuck
        bool resolved = false;
    };

    struct FollowEventData
    {
        ObjectGuid targetGuid;
        float followDistance = 5.0f;
        float currentDistance = 0.0f;
        bool targetInRange = true;
        bool targetVisible = true;
    };

    struct TacticalMovementData
    {
        uint8 movementType = 0;         // 0=positioning, 1=kiting, 2=retreating
        Position targetPosition;
        ObjectGuid enemyGuid;
        float optimalRange = 0.0f;
        bool inOptimalPosition = false;
    };

    // ========================================================================
    // TRADE & ECONOMY EVENT DATA (Phase 6.3)
    // ========================================================================

    struct TradeEventData
    {
        ObjectGuid partnerGuid;
        uint32 goldOffered = 0;              // Copper
        uint32 goldReceived = 0;             // Copper
        uint32 itemCount = 0;                // Number of items in trade
        bool tradeAccepted = false;
        bool tradeCancelled = false;
    };

    struct AuctionEventData
    {
        uint32 auctionId = 0;
        uint32 itemEntry = 0;
        uint32 bidPrice = 0;                 // Copper
        uint32 buyoutPrice = 0;              // Copper
        ObjectGuid bidderGuid;
        bool won = false;
        bool outbid = false;
        bool expired = false;
    };

    struct MailEventData
    {
        uint32 mailId = 0;
        ObjectGuid senderGuid;
        ::std::string subject;
        uint32 goldAttached = 0;             // Copper
        uint32 codAmount = 0;                // Copper for COD
        uint32 itemCount = 0;
        bool hasItems = false;
        bool isCOD = false;
    };

    struct GoldTransactionData
    {
        uint32 amount = 0;                   // Copper
        uint8 source = 0;                    // 0=quest, 1=loot, 2=auction, 3=trade, 4=vendor
        ObjectGuid sourceGuid;
        bool isIncome = true;                // true=received, false=spent
    };

    struct VendorTransactionData
    {
        ObjectGuid vendorGuid;
        uint32 itemEntry = 0;
        uint32 price = 0;                    // Copper
        uint32 quantity = 1;
        bool isPurchase = true;              // true=buy, false=sell
        bool isRepair = false;
    };

    // ========================================================================
    // ENVIRONMENTAL HAZARD EVENT DATA
    // ========================================================================

    struct EnvironmentalEventData
    {
        uint8 hazardType = 0;       // 0=fall, 1=drown, 2=fire, 3=lava, 4=void
        Position hazardLocation;
        uint32 estimatedDamage = 0;
        bool isFatal = false;
        Position safeLocation;      // Recommended safe spot
    };

    struct VoidZoneData
    {
        ObjectGuid creatorGuid;
        Position center;
        float radius = 0.0f;
        uint32 tickDamage = 0;
        uint32 spellId = 0;
    };

    // ========================================================================
    // PVP EVENT DATA
    // ========================================================================

    struct PvPEventData
    {
        uint32 mapId = 0;
        uint8 pvpType = 0;          // 0=world, 1=BG, 2=arena
        uint8 bracketId = 0;        // 2v2, 3v3, 5v5, etc.
        uint32 rating = 0;
        bool isRanked = false;
    };

    struct HonorEventData
    {
        uint32 honorGained = 0;
        uint32 totalHonor = 0;
        ObjectGuid killedPlayerGuid;
        bool isHonorableKill = true;
    };

    // ========================================================================
    // EVENT DATA VARIANT
    // ========================================================================

    /**
     * @brief Type-safe event data variant
     *
     * Holds any of the specialized event data structures.
     * Use std::get<T> or std::holds_alternative<T> to access.
     */
    using EventDataVariant = ::std::variant<
        ::std::monostate,             // No data
        LootRollData,
        LootReceivedData,
        CurrencyGainedData,
        AuraEventData,
        CCEventData,
        InterruptData,
        DeathEventData,
        ResurrectionEventData,
        InstanceEventData,
        BossEventData,
        MythicPlusData,
        RaidMarkerData,
        ResourceEventData,
        ComboPointsData,
        RunesData,
        DelveEventData,
        HeroTalentData,
        WarbandData,
        ChatEventData,
        CommandEventData,
        GroupEventData,
        GuildEventData,
        FriendEventData,
        EmoteEventData,
        ItemEventData,
        UpgradeEventData,
        DamageEventData,
        ThreatEventData,
        QuestEventData,
        MovementEventData,
        PathfindingEventData,
        StuckEventData,
        FollowEventData,
        TacticalMovementData,
        TradeEventData,
        AuctionEventData,
        MailEventData,
        GoldTransactionData,
        VendorTransactionData,
        EnvironmentalEventData,
        VoidZoneData,
        PvPEventData,
        HonorEventData
    >;

} // namespace Events
} // namespace Playerbot

#endif // TRINITYCORE_BOT_EVENT_DATA_H
