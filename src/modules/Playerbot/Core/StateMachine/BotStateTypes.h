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

#ifndef TRINITYCORE_BOT_STATE_TYPES_H
#define TRINITYCORE_BOT_STATE_TYPES_H

#include "Define.h"
#include <atomic>
#include <cstdint>
#include <string_view>

namespace Playerbot
{
namespace StateMachine
{
    /**
     * @brief Bot initialization state tracking for proper sequencing
     *
     * This enum tracks the one-time initialization sequence from bot creation
     * to operational readiness. It addresses Issue #1: "Bot already in group at login"
     * by ensuring proper state validation before group operations.
     *
     * @note This is distinct from BotAIState which tracks ongoing operational states
     */
    enum class BotInitState : uint8_t
    {

        CREATED
        = 0,  ///< Bot object instantiated but not initialized
        LOADING_CHARACTER    = 1,  ///< Loading character data from database

        IN_WORLD
        = 2,  ///< AddedToWorld() completed, IsInWorld() returns true
        CHECKING_GROUP      = 3,  ///< Verifying group membership state
        ACTIVATING_STRATEGIES = 4,  ///< OnGroupJoined() called, strategies being enabled

        READY
        = 5,  ///< Fully initialized and operational

        FAILED
        = 6   ///< Initialization failed, requires manual intervention
    };

    /**
     * @brief Event types for state machine transitions
     *
     * Comprehensive event system supporting bot lifecycle, group management,
     * combat, and movement events. Designed for forward compatibility with
     * Phase 4 event-driven architecture.
     */
    enum class EventType : uint16_t
    {
        // Bot lifecycle events (0-31)
        BOT_CREATED           = 0,   ///< Bot instance created

        BOT_LOGIN
        = 1,   ///< Bot login initiated

        BOT_LOGOUT
        = 2,   ///< Bot logout initiated
        BOT_ADDED_TO_WORLD    = 3,   ///< Bot added to world (IsInWorld() true)
        BOT_REMOVED_FROM_WORLD = 4,  ///< Bot removed from world
        BOT_DESTROYED         = 5,   ///< Bot instance being destroyed

        BOT_RESET
        = 6,   ///< Bot state reset requested
        BOT_TELEPORTED        = 7,   ///< Bot teleported to new location
        FIRST_LOGIN           = 8,   ///< Bot's first login (new character)
        PLAYER_LOGIN          = 9,   ///< Player login event
        PLAYER_LOGOUT         = 10,  ///< Player logout event
        PLAYER_REPOP          = 11,  ///< Player respawn at graveyard
        ZONE_CHANGED          = 12,  ///< Bot changed zones
        MAP_CHANGED           = 13,  ///< Bot changed maps
        PLAYER_LEVEL_UP       = 14,  ///< Player gained a level
        TALENT_POINTS_CHANGED = 15,  ///< Talent points gained/spent
        TALENTS_RESET         = 16,  ///< Talents were reset

        XP_GAINED
        = 17,  ///< Experience points gained
        REPUTATION_CHANGED    = 18,  ///< Reputation with faction changed

        // Group events (32-63) - Addresses Issues #1 and #4
        GROUP_JOINED          = 32,  ///< Bot joined a group

        GROUP_LEFT
        = 33,  ///< Bot left the group
        GROUP_DISBANDED       = 34,  ///< Group was disbanded
        LEADER_LOGGED_OUT     = 35,  ///< Group leader disconnected (Issue #4)
        LEADER_CHANGED        = 36,  ///< Group leader changed
        GROUP_LEADER_CHANGED  = 36,  ///< Alias for LEADER_CHANGED (script compatibility)
        GROUP_INVITE_RECEIVED = 37,  ///< Bot received group invitation

        GROUP_CHAT
        = 38,  ///< Group chat message received
        MEMBER_JOINED         = 39,  ///< New member joined group
        MEMBER_LEFT           = 40,  ///< Member left group
        GROUP_INVITE_DECLINED = 41,  ///< Bot declined group invitation
        RAID_CONVERTED        = 42,  ///< Group converted to raid
        GROUP_MEMBER_ATTACKED = 43,  ///< Group member was attacked - triggers assist

        // Combat events (64-95) - Addresses Issues #2 and #3
        COMBAT_STARTED        = 64,  ///< Combat initiated
        COMBAT_ENDED          = 65,  ///< Combat concluded
        TARGET_ACQUIRED       = 66,  ///< Valid target acquired (fixes NULL target)
        TARGET_LOST           = 67,  ///< Target no longer valid
        THREAT_GAINED         = 68,  ///< Bot gained threat on target
        THREAT_LOST           = 69,  ///< Bot lost threat on target
        DAMAGE_TAKEN          = 70,  ///< Bot received damage
        DAMAGE_DEALT          = 71,  ///< Bot dealt damage
        HEAL_RECEIVED         = 72,  ///< Bot received healing
        SPELL_CAST_START      = 73,  ///< Spell casting started
        SPELL_CAST_SUCCESS    = 74,  ///< Spell successfully cast
        SPELL_CAST_FAILED     = 75,  ///< Spell cast failed
        SPELL_INTERRUPTED     = 76,  ///< Spell casting interrupted

        HEAL_CAST
        = 77,  ///< Healing spell cast
        DUEL_STARTED          = 78,  ///< Duel began

        DUEL_WON
        = 79,  ///< Duel victory

        DUEL_LOST
        = 80,  ///< Duel defeat

        // Movement events (96-127)
        MOVEMENT_STARTED      = 96,  ///< Movement initiated
        MOVEMENT_STOPPED      = 97,  ///< Movement halted
        PATH_COMPLETE         = 98,  ///< Pathfinding completed successfully
        PATH_COMPLETED        = 99,  ///< Pathfinding completed (alias)
        PATH_FAILED           = 100, ///< Pathfinding failed
        PATH_GENERATED        = 101, ///< New path generated
        PATH_RECALCULATED     = 102, ///< Path recalculated
        POSITION_REACHED      = 103, ///< Destination reached
        POSITION_CHANGED      = 104, ///< Position changed

        TELEPORTED
        = 105, ///< Bot teleported
        MOUNT_CHANGED         = 106, ///< Mount status changed
        STUCK_DETECTED        = 107, ///< Bot detected as stuck
        STUCK_RESOLVED        = 108, ///< Bot recovered from stuck state

        FALLING
        = 109, ///< Bot is falling

        SWIMMING
        = 110, ///< Bot is swimming

        FLYING
        = 111, ///< Bot is flying
        FOLLOW_STARTED        = 112, ///< Started following target
        FOLLOW_STOPPED        = 113, ///< Stopped following target
        FOLLOW_TARGET_SET     = 114, ///< Follow target established
        FOLLOW_TARGET_LOST    = 115, ///< Follow target no longer valid
        FOLLOW_DISTANCE_CHANGED = 116, ///< Follow distance changed
        TACTICAL_POSITIONING  = 117, ///< Tactical positioning for combat

        KITING
        = 118, ///< Kiting enemy (ranged combat)

        RETREATING
        = 119, ///< Tactical retreat from combat

        // Quest events (128-159)
        QUEST_ACCEPTED        = 128, ///< Quest accepted
        QUEST_COMPLETED       = 129, ///< Quest objectives completed
        QUEST_TURNED_IN       = 130, ///< Quest turned in to NPC
        QUEST_ABANDONED       = 131, ///< Quest abandoned
        QUEST_FAILED          = 132, ///< Quest failed
        QUEST_STATUS_CHANGED  = 133, ///< Quest status updated
        QUEST_AVAILABLE       = 134, ///< Quest available to accept
        QUEST_OBJECTIVE_COMPLETE = 135, ///< Quest objective completed
        QUEST_OBJECTIVE_PROGRESS = 136, ///< Quest objective progress
        QUEST_ITEM_COLLECTED  = 137, ///< Quest item collected
        QUEST_CREATURE_KILLED = 138, ///< Quest creature killed
        QUEST_EXPLORATION     = 139, ///< Quest area explored
        QUEST_REWARD_RECEIVED = 140, ///< Quest reward received
        QUEST_REWARD_CHOSEN   = 141, ///< Quest reward chosen
        QUEST_EXPERIENCE_GAINED = 142, ///< XP from quest
        QUEST_REPUTATION_GAINED = 143, ///< Reputation from quest
        QUEST_CHAIN_ADVANCED  = 144, ///< Quest chain advanced
        DAILY_QUEST_RESET     = 145, ///< Daily quests reset
        WEEKLY_QUEST_RESET    = 146, ///< Weekly quests reset
        QUEST_SHARED          = 147, ///< Quest shared by party member
        GROUP_QUEST_UPDATE    = 148, ///< Group quest progress update

        // Trade events (160-191)
        TRADE_INITIATED       = 160, ///< Trade window opened
        TRADE_ACCEPTED        = 161, ///< Trade accepted
        TRADE_CANCELLED       = 162, ///< Trade cancelled
        TRADE_ITEM_ADDED      = 163, ///< Item added to trade window
        TRADE_GOLD_ADDED      = 164, ///< Gold added to trade window
        GOLD_CHANGED          = 165, ///< Gold amount changed
        GOLD_CAP_REACHED      = 166, ///< Maximum gold limit reached
        GOLD_RECEIVED         = 167, ///< Gold received from trade/mail/quest

        GOLD_SPENT
        = 168, ///< Gold spent on purchases/repairs
        LOW_GOLD_WARNING      = 169, ///< Gold below threshold
        AUCTION_BID_PLACED    = 170, ///< Bid placed on auction item
        AUCTION_WON           = 171, ///< Won auction item
        AUCTION_OUTBID        = 172, ///< Outbid on auction
        AUCTION_EXPIRED       = 173, ///< Auction listing expired
        AUCTION_SOLD          = 174, ///< Auction item sold
        MAIL_RECEIVED         = 175, ///< Mail received in mailbox

        MAIL_SENT
        = 176, ///< Mail sent to player
        COD_PAYMENT           = 177, ///< COD payment made
        VENDOR_PURCHASE       = 178, ///< Item purchased from vendor
        VENDOR_SALE           = 179, ///< Item sold to vendor
        REPAIR_COST           = 180, ///< Equipment repaired

        // Loot & Reward events (200-230) - CRITICAL for dungeon/raid
        LOOT_ROLL_STARTED     = 200, ///< Need/Greed/Pass window opened
        LOOT_ROLL_WON         = 201, ///< Bot won a loot roll
        LOOT_ROLL_LOST        = 202, ///< Bot lost a loot roll
        LOOT_RECEIVED         = 203, ///< Item added to inventory from loot
        LOOT_MASTER_ASSIGNED  = 204, ///< Master looter assigned item
        LOOT_PERSONAL_DROPPED = 205, ///< Personal loot item dropped
        LOOT_BONUS_ROLL_USED  = 206, ///< Bonus roll token consumed
        LOOT_CHEST_OPENED     = 207, ///< M+ chest/delve chest opened
        LOOT_CURRENCY_GAINED  = 208, ///< Valor/Conquest/Flightstones gained
        GREAT_VAULT_AVAILABLE = 209, ///< Weekly vault ready
        GREAT_VAULT_SELECTED  = 210, ///< Item chosen from vault

        // Aura & Buff/Debuff events (231-260) - CRITICAL for combat
        AURA_APPLIED          = 231, ///< Any buff/debuff applied
        AURA_REMOVED          = 232, ///< Buff/debuff removed
        AURA_REFRESHED        = 233, ///< Duration reset
        AURA_STACKS_CHANGED   = 234, ///< Stack count modified

        CC_APPLIED
        = 235, ///< Stun/Fear/Polymorph etc

        CC_BROKEN
        = 236, ///< CC effect broken
        DISPELLABLE_DETECTED  = 237, ///< Dispellable debuff on bot
        INTERRUPT_NEEDED      = 238, ///< Enemy casting interruptible spell
        DEFENSIVE_NEEDED      = 239, ///< Low health, need defensive CD
        BLOODLUST_ACTIVATED   = 240, ///< Heroism/Bloodlust/Time Warp active
        ENRAGE_DETECTED       = 241, ///< Enemy enraged (needs soothe/tranq)
        IMMUNITY_DETECTED     = 242, ///< Target immune to damage
        SHIELD_ABSORBED       = 243, ///< Absorb shield active
        DOT_APPLIED           = 244, ///< Damage over time effect
        HOT_APPLIED           = 245, ///< Heal over time effect

        // Death & Resurrection events (261-275) - CRITICAL for recovery
        PLAYER_DIED           = 261, ///< Bot died
        RESURRECTION_PENDING  = 262, ///< Resurrection cast on bot
        RESURRECTION_ACCEPTED = 263, ///< Bot accepted res
        SPIRIT_RELEASED       = 264, ///< Released to graveyard
        CORPSE_REACHED        = 265, ///< Arrived at corpse location
        BATTLE_REZ_AVAILABLE  = 266, ///< Combat res can be used
        ANKH_AVAILABLE        = 267, ///< Shaman self-res ready
        SOULSTONE_AVAILABLE   = 268, ///< Warlock soulstone active

        // Instance & Dungeon events (276-300) - HIGH priority
        INSTANCE_ENTERED      = 276, ///< Entered dungeon/raid
        INSTANCE_LEFT         = 277, ///< Left instance
        INSTANCE_RESET        = 278, ///< Instance reset occurred
        DIFFICULTY_CHANGED    = 279, ///< Instance difficulty changed
        BOSS_ENGAGED          = 280, ///< Boss combat started
        BOSS_PHASE_TRANSITION = 281, ///< Boss entered new phase
        BOSS_DEFEATED         = 282, ///< Boss killed

        BOSS_WIPE
        = 283, ///< Group wipe on boss
        BOSS_ABILITY_CAST     = 284, ///< Boss cast important ability
        WIPE_DETECTED         = 285, ///< Group wipe occurred
        MYTHIC_PLUS_STARTED   = 286, ///< Keystone activated
        KEYSTONE_ACTIVATED    = 287, ///< Keystone activated (alias)
        MYTHIC_PLUS_TIMER_UPDATE = 288, ///< M+ timer updated
        MYTHIC_PLUS_DEATH     = 289, ///< Death in M+ (time penalty)
        MYTHIC_PLUS_COMPLETED = 290, ///< M+ timer success
        KEYSTONE_COMPLETED    = 291, ///< Keystone completed (alias)
        MYTHIC_PLUS_DEPLETED  = 292, ///< M+ timer failed
        AFFIX_ACTIVATED       = 293, ///< M+ affix mechanic triggered
        MYTHIC_PLUS_AFFIX_TRIGGER = 294, ///< M+ affix trigger (alias)
        RAID_MARKER_PLACED    = 295, ///< Raid marker on target/ground
        RAID_MARKER_REMOVED   = 296, ///< Raid marker removed
        READY_CHECK_STARTED   = 297, ///< Ready check initiated
        ROLE_CHECK_STARTED    = 298, ///< Role check for LFG
        LOCKOUT_WARNING       = 299, ///< About to be saved to ID
        PULL_TIMER_STARTED    = 300, ///< DBM/BigWigs pull timer

        // PvP events (301-320) - MEDIUM priority
        PVP_FLAG_CHANGED      = 301, ///< PvP status changed
        PVP_ZONE_ENTERED      = 302, ///< Entered PvP zone
        PVP_ZONE_LEFT         = 303, ///< Left PvP zone
        WAR_MODE_ENABLED      = 304, ///< War Mode enabled
        WAR_MODE_DISABLED     = 305, ///< War Mode disabled
        WAR_MODE_TOGGLED      = 306, ///< War Mode enabled/disabled (alias)
        ARENA_ENTERED         = 307, ///< Arena match started
        ARENA_MATCH_STARTED   = 308, ///< Arena match started (alias)
        ARENA_ROUND_STARTED   = 309, ///< Arena round started (Solo Shuffle)
        ARENA_MATCH_ENDED     = 310, ///< Arena match ended
        ARENA_ENDED           = 311, ///< Arena match completed (alias)
        ARENA_RATING_CHANGED  = 312, ///< Arena rating changed

        BG_ENTERED
        = 313, ///< Battleground joined
        BATTLEGROUND_STARTED  = 314, ///< Battleground started (alias)
        BATTLEGROUND_OBJECTIVE_CAPTURED = 315, ///< BG objective captured
        BATTLEGROUND_FLAG_PICKED_UP = 316, ///< BG flag picked up
        BATTLEGROUND_FLAG_CAPTURED = 317, ///< BG flag captured
        BATTLEGROUND_ENDED    = 318, ///< Battleground ended

        BG_ENDED
        = 319, ///< Battleground completed (alias)
        BLITZ_BATTLEGROUND_STARTED = 320, ///< Blitz BG started (8v8)
        HONORABLE_KILL        = 321, ///< Honorable kill earned
        HONOR_GAINED          = 322, ///< Honor points earned
        CONQUEST_GAINED       = 323, ///< Conquest points earned
        CONQUEST_CAP_RESET    = 324, ///< Weekly conquest cap reset
        PVP_TALENT_ACTIVATED  = 325, ///< PvP talent became active
        DUEL_REQUESTED        = 326, ///< Duel invitation received

        // Resource Management events (330-350) - HIGH priority
        HEALTH_CRITICAL       = 330, ///< Health below 30%

        HEALTH_LOW
        = 331, ///< Health below 50%

        MANA_LOW
        = 332, ///< Mana below 30%
        RESOURCE_CAPPED       = 333, ///< Energy/Rage/etc at max
        RESOURCE_DEPLETED     = 334, ///< Out of primary resource
        COMBO_POINTS_MAX      = 335, ///< At max combo points
        HOLY_POWER_MAX        = 336, ///< Paladin at max HP
        SOUL_SHARDS_MAX       = 337, ///< Warlock at max shards
        RUNES_AVAILABLE       = 338, ///< DK runes ready

        CHI_MAX
        = 339, ///< Monk at max chi

        // War Within specific events (341-370) - HIGH priority
        DELVE_ENTERED         = 341, ///< Entered delve instance
        DELVE_COMPLETED       = 342, ///< Delve objectives done
        DELVE_OBJECTIVE_COMPLETE = 343, ///< Single delve objective completed
        DELVE_TIER_INCREASED  = 344, ///< Delve difficulty up
        ZEKVIR_APPEARED       = 345, ///< Zekvir boss spawn
        BRANN_LEVEL_UP        = 346, ///< Companion leveled
        HERO_TALENT_ACTIVATED = 347, ///< Hero talent point spent
        WARBAND_ACHIEVEMENT   = 348, ///< Account-wide achievement
        WARBAND_REPUTATION_UP = 349, ///< Warband rep increased
        DYNAMIC_FLIGHT_ENABLED = 350, ///< Dragonriding activated
        WORLD_EVENT_STARTED   = 351, ///< World event active
        WORLD_EVENT_COMPLETED = 352, ///< World event finished
        CATALYST_CHARGE_GAINED = 353, ///< Revival Catalyst charge
        VAULT_KEY_OBTAINED    = 354, ///< M+ vault key earned
        CREST_FRAGMENT_GAINED = 355, ///< Upgrade currency obtained

        // Social & Communication events (371-390) - LOW priority
        CHAT_RECEIVED         = 371, ///< General chat message
        WHISPER_RECEIVED      = 372, ///< Private message
        EMOTE_RECEIVED        = 373, ///< Emote received
        SAY_DETECTED          = 374, ///< /say in range
        PARTY_CHAT_RECEIVED   = 375, ///< Party message
        RAID_CHAT_RECEIVED    = 376, ///< Raid chat message
        GUILD_CHAT_RECEIVED   = 377, ///< Guild chat message
        BOT_COMMAND_RECEIVED  = 378, ///< Bot command from master
        RAID_WARNING_RECEIVED = 379, ///< Raid warning message
        EMOTE_TARGETED        = 380, ///< Emote directed at bot
        EMOTE_PERFORMED       = 381, ///< Bot performed emote
        FRIEND_ONLINE         = 382, ///< Friend logged in
        FRIEND_OFFLINE        = 383, ///< Friend logged out
        FRIEND_REQUEST_RECEIVED = 384, ///< Friend request received
        FRIEND_ADDED          = 385, ///< Friend added to list
        FRIEND_REMOVED        = 386, ///< Friend removed from list
        GUILD_INVITE_RECEIVED = 387, ///< Guild invitation
        GUILD_JOINED          = 388, ///< Joined guild

        GUILD_LEFT
        = 389, ///< Left guild
        GUILD_RANK_CHANGED    = 390, ///< Guild rank changed
        COMMAND_RECEIVED      = 391, ///< Bot command from master
        CONVERTED_TO_RAID     = 392, ///< Group converted to raid

        // Equipment & Inventory events (410-430) - MEDIUM priority
        ITEM_EQUIPPED         = 410, ///< Item equipped
        ITEM_UNEQUIPPED       = 411, ///< Item removed
        ITEM_BROKEN           = 412, ///< Durability at 0
        ITEM_REPAIRED         = 413, ///< Items repaired

        BAG_FULL
        = 414, ///< No inventory space
        ITEM_UPGRADED         = 415, ///< Item ilvl increased
        GEM_SOCKETED          = 416, ///< Gem inserted
        ENCHANT_APPLIED       = 417, ///< Enchantment added
        ITEM_COMPARISON       = 418, ///< Better item available

        ITEM_USED
        = 419, ///< Item activated/used
        ITEM_EXPIRED          = 420, ///< Temporary item expired
        ITEM_REMOVED          = 421, ///< Item removed from inventory
        VEHICLE_ENTERED       = 422, ///< Mounted vehicle/creature
        VEHICLE_EXITED        = 423, ///< Dismounted from vehicle

        // Environmental Hazard events (450-470) - MEDIUM priority
        FALL_DAMAGE_IMMINENT  = 450, ///< About to take fall damage
        DROWNING_START        = 451, ///< Breath timer started
        DROWNING_DAMAGE       = 452, ///< Taking drowning damage
        FIRE_DAMAGE_TAKEN     = 453, ///< Standing in fire
        ENVIRONMENTAL_DAMAGE  = 454, ///< Generic environmental damage
        VOID_ZONE_DETECTED    = 455, ///< Bad ground effect nearby
        KNOCKBACK_RECEIVED    = 456, ///< Knocked back
        TELEPORT_REQUIRED     = 457, ///< Mechanic requires teleport
        SAFE_SPOT_NEEDED      = 458, ///< Need to move to safe area

        // Queue events (471-496) - JIT Bot Creation
        // BG Queue events
        BG_QUEUE_JOIN         = 471, ///< Player joined BG queue
        BG_QUEUE_LEAVE        = 472, ///< Player left BG queue
        BG_QUEUE_SHORTAGE     = 473, ///< BG queue shortage detected
        BG_INVITATION_RECEIVED = 474, ///< BG invitation received
        BG_QUEUE_UPDATE       = 475, ///< BG queue status update

        // LFG Queue events
        LFG_QUEUE_JOIN        = 480, ///< Player joined LFG queue
        LFG_QUEUE_LEAVE       = 481, ///< Player left LFG queue
        LFG_QUEUE_SHORTAGE    = 482, ///< LFG role shortage detected
        LFG_QUEUE_UPDATE      = 483, ///< LFG queue status update
        LFG_PROPOSAL          = 484, ///< LFG group proposal received
        LFG_PROPOSAL_ACCEPTED = 485, ///< LFG proposal accepted
        LFG_PROPOSAL_DECLINED = 486, ///< LFG proposal declined

        // Arena Queue events
        ARENA_QUEUE_JOIN      = 490, ///< Player joined Arena queue
        ARENA_QUEUE_LEAVE     = 491, ///< Player left Arena queue
        ARENA_QUEUE_SHORTAGE  = 492, ///< Arena queue shortage detected
        ARENA_INVITATION_RECEIVED = 493, ///< Arena invitation received
        ARENA_QUEUE_UPDATE    = 494, ///< Arena queue status update

        // Custom events (1000+)
        CUSTOM_BASE           = 1000 ///< Base for user-defined events
    };

    /**
     * @brief Result codes for state transition attempts
     */
    enum class StateTransitionResult : uint8_t
    {

        SUCCESS
        = 0, ///< Transition completed successfully
        INVALID_FROM_STATE    = 1, ///< Current state doesn't allow this transition
        INVALID_TO_STATE      = 2, ///< Target state is not valid
        PRECONDITION_FAILED   = 3, ///< Required conditions not met (e.g., not IsInWorld())
        ALREADY_IN_STATE      = 4, ///< Already in the target state
        CONCURRENT_TRANSITION = 5, ///< Another transition is in progress
        NOT_INITIALIZED       = 6, ///< State machine not initialized
        SYSTEM_ERROR          = 7  ///< Internal system error
    };

    /**
     * @brief Transition validation result with detailed information
     */
    struct TransitionValidation
    {
        StateTransitionResult result;     ///< The validation result
        ::std::string_view reason;          ///< Human-readable explanation

        uint32 errorCode = 0;
        ///< Optional error code for debugging

        /// Check if transition is allowed
        constexpr bool IsValid() const noexcept { return result == StateTransitionResult::SUCCESS; }

        /// Implicit conversion to bool for convenience
        constexpr operator bool() const noexcept { return IsValid(); }
    };

    /**
     * @brief Priority levels for behavior coordination
     *
     * Works with Phase 2's BehaviorManager for coordinated updates.
     * Higher values indicate higher priority.
     */
    using Priority = uint8_t;

    /// Critical priority - System errors, crashes, emergency states
    constexpr Priority PRIORITY_CRITICAL  = 255;

    /// Very high priority - Combat actions, survival behaviors
    constexpr Priority PRIORITY_VERY_HIGH = 200;

    /// High priority - Movement, following
    constexpr Priority PRIORITY_HIGH      = 150;

    /// Normal priority - Standard behaviors
    constexpr Priority PRIORITY_NORMAL    = 100;

    /// Low priority - Gathering, trading, social
    constexpr Priority PRIORITY_LOW       = 50;

    /// Very low priority - Background tasks, optimization
    constexpr Priority PRIORITY_VERY_LOW  = 10;

    /// Idle priority - No active behavior
    constexpr Priority PRIORITY_IDLE      = 0;

    /**
     * @brief State flags for enhanced state queries
     *
     * Complements Phase 2's BehaviorManager atomic flags with
     * state machine specific flags.
     */
    enum class StateFlags : uint32_t
    {

        NONE
        = 0,
        ///< No flags set
        INITIALIZING        = (1 << 0),  ///< Currently in initialization sequence

        READY
        = (1 << 1),  ///< Passed all initialization checks
        IN_TRANSITION      = (1 << 2),  ///< Currently transitioning between states
        ERROR_STATE        = (1 << 3),  ///< Error has occurred
        REQUIRES_VALIDATION = (1 << 4),  ///< State needs revalidation
        SAFE_TO_UPDATE     = (1 << 5),  ///< Bot is safe for AI updates

        LOCKED
        = (1 << 6),  ///< State is locked (no transitions allowed)
        DEFERRED_TRANSITION = (1 << 7),  ///< Transition deferred until conditions met
        CLEANUP_REQUIRED   = (1 << 8),  ///< Cleanup needed before next transition
        DEBUG_MODE         = (1 << 9)   ///< Enhanced debugging enabled
    };

    /// Enable bitwise operations for StateFlags
    constexpr StateFlags operator|(StateFlags a, StateFlags b) noexcept
    {
        return static_cast<StateFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    constexpr StateFlags operator&(StateFlags a, StateFlags b) noexcept
    {
        return static_cast<StateFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    constexpr StateFlags operator^(StateFlags a, StateFlags b) noexcept
    {
        return static_cast<StateFlags>(static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b));
    }

    constexpr StateFlags operator~(StateFlags a) noexcept
    {
        return static_cast<StateFlags>(~static_cast<uint32_t>(a));
    }

    /**
     * @brief Thread-safe container for initialization state information
     *
     * All members are atomic to ensure thread-safe access without locks.
     * Used by BotInitStateMachine for tracking initialization progress.
     */
    struct InitStateInfo
    {
        ::std::atomic<BotInitState> currentState{BotInitState::CREATED};     ///< Current initialization state
        ::std::atomic<BotInitState> previousState{BotInitState::CREATED};    ///< Previous state (for rollback)
        ::std::atomic<StateFlags> flags{StateFlags::INITIALIZING};           ///< Current state flags

        ::std::atomic<uint64_t> transitionCount{0};
        ///< Total transitions performed

        ::std::atomic<uint64_t> lastTransitionTime{0};
        ///< Time of last transition (getMSTime)

        ::std::atomic<uint32_t> errorCount{0};
        ///< Number of errors encountered

        ::std::atomic<uint32_t> retryCount{0};
        ///< Number of retry attempts
        ::std::atomic<EventType> lastEvent{EventType::BOT_CREATED};         ///< Last event processed

        ::std::atomic<uint32_t> stateStartTime{0};
        ///< When entered current state (getMSTime)

        /// Check if state is terminal (READY or FAILED)
        bool IsTerminal() const noexcept
        {

            BotInitState state = currentState.load(::std::memory_order_acquire);

            return state == BotInitState::READY || state == BotInitState::FAILED;
        }

        /// Check if initialization succeeded
        bool IsReady() const noexcept
        {

            return currentState.load(::std::memory_order_acquire) == BotInitState::READY;
        }

        /// Check if initialization failed
        bool IsFailed() const noexcept
        {

            return currentState.load(::std::memory_order_acquire) == BotInitState::FAILED;
        }

        /// Get time spent in current state (milliseconds)
        uint32_t GetTimeInCurrentState(uint32_t currentTime) const noexcept
        {

            uint32_t startTime = stateStartTime.load(::std::memory_order_acquire);

            return startTime > 0 ? (currentTime - startTime) : 0;
        }
    };

    /**
     * @brief Convert BotInitState to string representation
     */
    constexpr ::std::string_view ToString(BotInitState state) noexcept
    {
        switch (state)
        {

            case BotInitState::CREATED:
            return "CREATED";

            case BotInitState::LOADING_CHARACTER:
            return "LOADING_CHARACTER";

            case BotInitState::IN_WORLD:
            return "IN_WORLD";

            case BotInitState::CHECKING_GROUP:
            return "CHECKING_GROUP";

            case BotInitState::ACTIVATING_STRATEGIES: return "ACTIVATING_STRATEGIES";

            case BotInitState::READY:
            return "READY";

            case BotInitState::FAILED:
            return "FAILED";

            default:
            return "UNKNOWN";
        }
    }

    /**
     * @brief Convert EventType to string representation
     */
    constexpr ::std::string_view ToString(EventType event) noexcept
    {
        switch (event)
        {
            // Lifecycle events

            case EventType::BOT_CREATED:
            return "BOT_CREATED";

            case EventType::BOT_LOGIN:
            return "BOT_LOGIN";

            case EventType::BOT_LOGOUT:
            return "BOT_LOGOUT";

            case EventType::BOT_ADDED_TO_WORLD:
            return "BOT_ADDED_TO_WORLD";

            case EventType::BOT_REMOVED_FROM_WORLD: return "BOT_REMOVED_FROM_WORLD";

            case EventType::BOT_DESTROYED:
            return "BOT_DESTROYED";

            case EventType::BOT_RESET:
            return "BOT_RESET";

            case EventType::BOT_TELEPORTED:
            return "BOT_TELEPORTED";

            case EventType::FIRST_LOGIN:
            return "FIRST_LOGIN";

            case EventType::PLAYER_LOGIN:
            return "PLAYER_LOGIN";

            case EventType::PLAYER_LOGOUT:
            return "PLAYER_LOGOUT";

            case EventType::PLAYER_REPOP:
            return "PLAYER_REPOP";

            case EventType::ZONE_CHANGED:
            return "ZONE_CHANGED";

            case EventType::MAP_CHANGED:
            return "MAP_CHANGED";

            case EventType::PLAYER_LEVEL_UP:
            return "PLAYER_LEVEL_UP";

            case EventType::TALENT_POINTS_CHANGED: return "TALENT_POINTS_CHANGED";

            case EventType::TALENTS_RESET:
            return "TALENTS_RESET";

            case EventType::XP_GAINED:
            return "XP_GAINED";

            case EventType::REPUTATION_CHANGED:
            return "REPUTATION_CHANGED";

            // Group events

            case EventType::GROUP_JOINED:
            return "GROUP_JOINED";

            case EventType::GROUP_LEFT:
            return "GROUP_LEFT";

            case EventType::GROUP_DISBANDED:
            return "GROUP_DISBANDED";

            case EventType::LEADER_LOGGED_OUT:
            return "LEADER_LOGGED_OUT";

            case EventType::LEADER_CHANGED:
            return "LEADER_CHANGED";

            case EventType::GROUP_INVITE_RECEIVED: return "GROUP_INVITE_RECEIVED";

            case EventType::GROUP_CHAT:
            return "GROUP_CHAT";

            case EventType::MEMBER_JOINED:
            return "MEMBER_JOINED";

            case EventType::MEMBER_LEFT:
            return "MEMBER_LEFT";

            case EventType::GROUP_INVITE_DECLINED: return "GROUP_INVITE_DECLINED";

            case EventType::RAID_CONVERTED:
            return "RAID_CONVERTED";

            case EventType::GROUP_MEMBER_ATTACKED:
            return "GROUP_MEMBER_ATTACKED";

            // Combat events

            case EventType::COMBAT_STARTED:
            return "COMBAT_STARTED";

            case EventType::COMBAT_ENDED:
            return "COMBAT_ENDED";

            case EventType::TARGET_ACQUIRED:
            return "TARGET_ACQUIRED";

            case EventType::TARGET_LOST:
            return "TARGET_LOST";

            case EventType::THREAT_GAINED:
            return "THREAT_GAINED";

            case EventType::THREAT_LOST:
            return "THREAT_LOST";

            case EventType::DAMAGE_TAKEN:
            return "DAMAGE_TAKEN";

            case EventType::DAMAGE_DEALT:
            return "DAMAGE_DEALT";

            case EventType::SPELL_CAST_START:
            return "SPELL_CAST_START";

            case EventType::SPELL_CAST_SUCCESS:
            return "SPELL_CAST_SUCCESS";

            case EventType::SPELL_CAST_FAILED:
            return "SPELL_CAST_FAILED";

            case EventType::SPELL_INTERRUPTED:
            return "SPELL_INTERRUPTED";

            case EventType::HEAL_CAST:
            return "HEAL_CAST";

            case EventType::DUEL_STARTED:
            return "DUEL_STARTED";

            case EventType::DUEL_WON:
            return "DUEL_WON";

            case EventType::DUEL_LOST:
            return "DUEL_LOST";

            // Movement events

            case EventType::MOVEMENT_STARTED:
            return "MOVEMENT_STARTED";

            case EventType::MOVEMENT_STOPPED:
            return "MOVEMENT_STOPPED";

            case EventType::PATH_COMPLETE:
            return "PATH_COMPLETE";

            case EventType::PATH_FAILED:
            return "PATH_FAILED";

            case EventType::FOLLOW_TARGET_SET:
            return "FOLLOW_TARGET_SET";

            case EventType::FOLLOW_TARGET_LOST:
            return "FOLLOW_TARGET_LOST";

            case EventType::POSITION_REACHED:
            return "POSITION_REACHED";

            case EventType::STUCK_DETECTED:
            return "STUCK_DETECTED";

            // Quest events

            case EventType::QUEST_ACCEPTED:
            return "QUEST_ACCEPTED";

            case EventType::QUEST_COMPLETED:
            return "QUEST_COMPLETED";

            case EventType::QUEST_TURNED_IN:
            return "QUEST_TURNED_IN";

            case EventType::QUEST_ABANDONED:
            return "QUEST_ABANDONED";

            case EventType::QUEST_FAILED:
            return "QUEST_FAILED";

            case EventType::QUEST_STATUS_CHANGED:  return "QUEST_STATUS_CHANGED";

            // Trade events

            case EventType::TRADE_INITIATED:
            return "TRADE_INITIATED";

            case EventType::TRADE_ACCEPTED:
            return "TRADE_ACCEPTED";

            case EventType::TRADE_CANCELLED:
            return "TRADE_CANCELLED";

            case EventType::GOLD_CHANGED:
            return "GOLD_CHANGED";

            case EventType::GOLD_CAP_REACHED:
            return "GOLD_CAP_REACHED";

            // Social events

            case EventType::EMOTE_RECEIVED:
            return "EMOTE_RECEIVED";

            // Queue events (JIT Bot Creation)
            case EventType::BG_QUEUE_JOIN:         return "BG_QUEUE_JOIN";
            case EventType::BG_QUEUE_LEAVE:        return "BG_QUEUE_LEAVE";
            case EventType::BG_QUEUE_SHORTAGE:     return "BG_QUEUE_SHORTAGE";
            case EventType::BG_INVITATION_RECEIVED: return "BG_INVITATION_RECEIVED";
            case EventType::BG_QUEUE_UPDATE:       return "BG_QUEUE_UPDATE";
            case EventType::LFG_QUEUE_JOIN:        return "LFG_QUEUE_JOIN";
            case EventType::LFG_QUEUE_LEAVE:       return "LFG_QUEUE_LEAVE";
            case EventType::LFG_QUEUE_SHORTAGE:    return "LFG_QUEUE_SHORTAGE";
            case EventType::LFG_QUEUE_UPDATE:      return "LFG_QUEUE_UPDATE";
            case EventType::LFG_PROPOSAL:          return "LFG_PROPOSAL";
            case EventType::LFG_PROPOSAL_ACCEPTED: return "LFG_PROPOSAL_ACCEPTED";
            case EventType::LFG_PROPOSAL_DECLINED: return "LFG_PROPOSAL_DECLINED";
            case EventType::ARENA_QUEUE_JOIN:      return "ARENA_QUEUE_JOIN";
            case EventType::ARENA_QUEUE_LEAVE:     return "ARENA_QUEUE_LEAVE";
            case EventType::ARENA_QUEUE_SHORTAGE:  return "ARENA_QUEUE_SHORTAGE";
            case EventType::ARENA_INVITATION_RECEIVED: return "ARENA_INVITATION_RECEIVED";
            case EventType::ARENA_QUEUE_UPDATE:    return "ARENA_QUEUE_UPDATE";

            default:

                if (static_cast<uint16_t>(event) >= static_cast<uint16_t>(EventType::CUSTOM_BASE))

                    return "CUSTOM_EVENT";

                return "UNKNOWN_EVENT";
        }
    }

    /**
     * @brief Convert StateTransitionResult to string representation
     */
    constexpr ::std::string_view ToString(StateTransitionResult result) noexcept
    {
        switch (result)
        {

            case StateTransitionResult::SUCCESS:
            return "SUCCESS";

            case StateTransitionResult::INVALID_FROM_STATE:
            return "INVALID_FROM_STATE";

            case StateTransitionResult::INVALID_TO_STATE:
            return "INVALID_TO_STATE";

            case StateTransitionResult::PRECONDITION_FAILED:   return "PRECONDITION_FAILED";

            case StateTransitionResult::ALREADY_IN_STATE:
            return "ALREADY_IN_STATE";

            case StateTransitionResult::CONCURRENT_TRANSITION: return "CONCURRENT_TRANSITION";

            case StateTransitionResult::NOT_INITIALIZED:
            return "NOT_INITIALIZED";

            case StateTransitionResult::SYSTEM_ERROR:
            return "SYSTEM_ERROR";

            default:
            return "UNKNOWN_RESULT";
        }
    }

    /**
     * @brief Convert StateFlags to string representation
     * @note Returns primary flag only; use for debugging
     */
    constexpr ::std::string_view ToString(StateFlags flags) noexcept
    {

        if (flags == StateFlags::NONE)
        return "NONE";
        if ((flags & StateFlags::INITIALIZING) != StateFlags::NONE)  return "INITIALIZING";
        if ((flags & StateFlags::READY) != StateFlags::NONE)         return "READY";
        if ((flags & StateFlags::IN_TRANSITION) != StateFlags::NONE) return "IN_TRANSITION";
        if ((flags & StateFlags::ERROR_STATE) != StateFlags::NONE)   return "ERROR_STATE";
        if ((flags & StateFlags::REQUIRES_VALIDATION) != StateFlags::NONE) return "REQUIRES_VALIDATION";
        if ((flags & StateFlags::SAFE_TO_UPDATE) != StateFlags::NONE) return "SAFE_TO_UPDATE";
        if ((flags & StateFlags::LOCKED) != StateFlags::NONE)        return "LOCKED";
        if ((flags & StateFlags::DEFERRED_TRANSITION) != StateFlags::NONE) return "DEFERRED_TRANSITION";
        if ((flags & StateFlags::CLEANUP_REQUIRED) != StateFlags::NONE) return "CLEANUP_REQUIRED";
        if ((flags & StateFlags::DEBUG_MODE) != StateFlags::NONE)    return "DEBUG_MODE";
        return "MULTIPLE_FLAGS";
    }

    /**
     * @brief Example usage documentation
     *
     * @code
     * // Initialize state tracking
     * InitStateInfo stateInfo;
     *
     * // Check current state
     * if (stateInfo.IsReady())
     * {
     *     // Bot is fully initialized
     * }
     *
     * // Transition to new state
     * stateInfo.previousState = stateInfo.currentState.load();
     * stateInfo.currentState = BotInitState::IN_WORLD;
     * stateInfo.transitionCount++;
     * stateInfo.lastTransitionTime = GameTime::GetGameTimeMS();
     *
     * // Handle events
     * void OnEvent(EventType event)
     * {
     *     stateInfo.lastEvent = event;
     *
     *     switch (event)
     *     {
     *         case EventType::BOT_ADDED_TO_WORLD:

     *
     // Transition to IN_WORLD state

     *
     break;
     *         case EventType::GROUP_JOINED:

     *
     // Activate strategies

     *
     break;
     *     }
     * }
     * @endcode
     */

} // namespace StateMachine
} // namespace Playerbot

#endif // TRINITYCORE_BOT_STATE_TYPES_H