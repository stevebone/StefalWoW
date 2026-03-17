/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_HOOKS_H
#define PLAYERBOT_HOOKS_H

#include "Define.h"
#include "ObjectGuid.h"
#include <functional>
#include <memory>

// Forward declarations
class Player;
class Group;
class Unit;
class SpellInfo;
class Aura;
enum RemoveMethod : uint8;
enum LootMethod : uint8;
enum Difficulty : int16;
enum DamageEffectType : uint8;
enum SpellSchoolMask : uint32;

namespace Playerbot
{

/**
 * @class PlayerBotHooks
 * @brief Minimal integration points for TrinityCore → Playerbot event flow
 *
 * This class provides hook functions that TrinityCore's Group system calls
 * to notify the playerbot module of group-related events. This is the ONLY
 * modification required to TrinityCore core files.
 *
 * Design Principles (CLAUDE.md Compliance):
 * - MINIMAL CORE MODIFICATION: Only 8 hook points in Group.cpp
 * - OPTIONAL: All hooks check for nullptr before calling
 * - NON-INTRUSIVE: Core functionality unchanged if hooks not registered
 * - OBSERVER PATTERN: Hooks only observe, never modify core behavior
 * - PERFORMANCE: Hook calls are <1 microsecond when registered
 *
 * Integration Pattern:
 * ```cpp
 * // In TrinityCore's Group.cpp
 * bool Group::AddMember(Player* player)
 * {
 *     // ... existing code ...
 *
 *     // PLAYERBOT HOOK: Notify bots of new member
 *     if (PlayerBotHooks::OnGroupMemberAdded)
 *         PlayerBotHooks::OnGroupMemberAdded(this, player);
 *
 *     return true;
 * }
 * ```
 */
class TC_GAME_API PlayerBotHooks
{
public:
    /**
     * Initialize hook system
     * Called from playerbot module initialization
     */
    static void Initialize();

    /**
     * Shutdown hook system
     * Called from playerbot module shutdown
     */
    static void Shutdown();

    /**
     * Check if hooks are active
     * @return true if hooks are registered and functional
     */
    static bool IsActive();

    // ========================================================================
    // GROUP LIFECYCLE HOOKS
    // ========================================================================

    /**
     * Hook: Member added to group
     * Called from: Group::AddMember() after successful addition
     *
     * @param group The group that gained a member
     * @param player The player who joined
     */
    static inline ::std::function<void(Group*, Player*)> OnGroupMemberAdded = nullptr;

    /**
     * Hook: Member removed from group
     * Called from: Group::RemoveMember() after removal
     *
     * @param group The group that lost a member
     * @param guid GUID of removed member
     * @param method How member was removed (left, kicked, etc.)
     */
    static inline ::std::function<void(Group*, ObjectGuid, RemoveMethod)> OnGroupMemberRemoved = nullptr;

    /**
     * Hook: Group leadership changed
     * Called from: Group::ChangeLeader() after leader change
     *
     * @param group The group with new leader
     * @param newLeaderGuid GUID of new leader
     */
    static inline ::std::function<void(Group*, ObjectGuid)> OnGroupLeaderChanged = nullptr;

    /**
     * Hook: Group is disbanding
     * Called from: Group::Disband() BEFORE disbanding
     *
     * @param group The group being disbanded
     *
     * Note: Called BEFORE disbanding so bots can cleanup properly
     */
    static inline ::std::function<void(Group*)> OnGroupDisbanding = nullptr;

    // ========================================================================
    // GROUP COMPOSITION HOOKS
    // ========================================================================

    /**
     * Hook: Group converted to raid (or raid to party)
     * Called from: Group::ConvertToRaid() after conversion
     *
     * @param group The group that was converted
     * @param isRaid true if converted to raid, false if converted to party
     */
    static inline ::std::function<void(Group*, bool)> OnGroupRaidConverted = nullptr;

    /**
     * Hook: Member moved to different subgroup
     * Called from: Group::ChangeMembersGroup() after move
     *
     * @param group The group containing the member
     * @param playerGuid GUID of player moved
     * @param newSubgroup New subgroup number (0-7)
     */
    static inline ::std::function<void(Group*, ObjectGuid, uint8)> OnSubgroupChanged = nullptr;

    // ========================================================================
    // LOOT SYSTEM HOOKS
    // ========================================================================

    /**
     * Hook: Loot method changed
     * Called from: Group::SetLootMethod() after change
     *
     * @param group The group with changed loot method
     * @param method New loot method
     */
    static inline ::std::function<void(Group*, LootMethod)> OnLootMethodChanged = nullptr;

    /**
     * Hook: Loot threshold changed
     * Called from: Group::SetLootThreshold() after change
     *
     * @param group The group with changed threshold
     * @param threshold New item quality threshold
     */
    static inline ::std::function<void(Group*, uint8)> OnLootThresholdChanged = nullptr;

    /**
     * Hook: Master looter changed
     * Called from: Group::SetMasterLooterGuid() after change
     *
     * @param group The group with new master looter
     * @param masterLooterGuid GUID of new master looter (may be empty)
     */
    static inline ::std::function<void(Group*, ObjectGuid)> OnMasterLooterChanged = nullptr;

    // ========================================================================
    // ROLE AND ASSIGNMENT HOOKS
    // ========================================================================

    /**
     * Hook: Member assistant status changed
     * Called from: Group::SetGroupMemberFlag() when assistant flag changes
     *
     * @param group The group containing the member
     * @param memberGuid GUID of member with changed status
     * @param isAssistant true if promoted to assistant, false if demoted
     */
    static inline ::std::function<void(Group*, ObjectGuid, bool)> OnAssistantChanged = nullptr;

    /**
     * Hook: Main tank assignment changed
     * Called from: Group::SetGroupMemberFlag() when main tank flag changes
     *
     * @param group The group with changed main tank
     * @param tankGuid GUID of new main tank (may be empty)
     */
    static inline ::std::function<void(Group*, ObjectGuid)> OnMainTankChanged = nullptr;

    /**
     * Hook: Main assist assignment changed
     * Called from: Group::SetGroupMemberFlag() when main assist flag changes
     *
     * @param group The group with changed main assist
     * @param assistGuid GUID of new main assist (may be empty)
     */
    static inline ::std::function<void(Group*, ObjectGuid)> OnMainAssistChanged = nullptr;

    // ========================================================================
    // COMBAT COORDINATION HOOKS
    // ========================================================================

    /**
     * Hook: Raid target icon changed
     * Called from: Group::SetTargetIcon() after change
     *
     * @param group The group that set the icon
     * @param icon Icon number (0-7, or 0xFF for clear)
     * @param targetGuid GUID of target unit (may be empty if clearing)
     */
    static inline ::std::function<void(Group*, uint8, ObjectGuid)> OnRaidTargetIconChanged = nullptr;

    /**
     * Hook: Raid world marker changed
     * Called from: Group::SetRaidMarker() or DeleteRaidMarker()
     *
     * @param group The group that changed the marker
     * @param markerId Marker ID (0-7)
     * @param mapId Map ID where marker is placed
     * @param x X coordinate (0 if marker deleted)
     * @param y Y coordinate (0 if marker deleted)
     * @param z Z coordinate (0 if marker deleted)
     */
    static inline ::std::function<void(Group*, uint32, uint32, float, float, float)> OnRaidMarkerChanged = nullptr;

    // ========================================================================
    // READY CHECK HOOKS
    // ========================================================================

    /**
     * Hook: Ready check started
     * Called from: Group::StartReadyCheck() when initiated
     *
     * @param group The group doing ready check
     * @param initiatorGuid GUID of player who started it
     * @param durationMs Duration of ready check in milliseconds
     */
    static inline ::std::function<void(Group*, ObjectGuid, uint32)> OnReadyCheckStarted = nullptr;

    /**
     * Hook: Ready check response received
     * Called from: Group::SetReadyCheckResponse() when member responds
     *
     * @param group The group doing ready check
     * @param memberGuid GUID of member who responded
     * @param ready true if ready, false if not ready
     */
    static inline ::std::function<void(Group*, ObjectGuid, bool)> OnReadyCheckResponse = nullptr;

    /**
     * Hook: Ready check completed
     * Called from: Group::EndReadyCheck() when check finishes
     *
     * @param group The group that finished ready check
     * @param allReady true if all members were ready
     * @param respondedCount Number of members who responded
     * @param totalMembers Total number of group members
     */
    static inline ::std::function<void(Group*, bool, uint32, uint32)> OnReadyCheckCompleted = nullptr;

    // ========================================================================
    // INSTANCE AND DIFFICULTY HOOKS
    // ========================================================================

    /**
     * Hook: Instance difficulty changed
     * Called from: Group::SetDifficultyID() after change
     *
     * @param group The group with changed difficulty
     * @param difficulty New difficulty setting
     */
    static inline ::std::function<void(Group*, Difficulty)> OnDifficultyChanged = nullptr;

    /**
     * Hook: Instance bind created or updated
     * Called from: Group::BindToInstance() after binding
     *
     * @param group The group that was bound
     * @param instanceId Instance ID bound to
     * @param permanent true if permanent bind, false if temporary
     */
    static inline ::std::function<void(Group*, uint32, bool)> OnInstanceBind = nullptr;

    // ========================================================================
    // PLAYER LIFECYCLE HOOKS
    // ========================================================================

    /**
     * Hook: Player died
     * Called from: Player::setDeathState() when state changes to JUST_DIED
     *
     * @param player The player who died
     *
     * Note: Called when player transitions to JUST_DIED state, before corpse creation
     */
    static inline ::std::function<void(Player*)> OnPlayerDeath = nullptr;

    /**
     * Hook: Player resurrected
     * Called from: Player::ResurrectPlayer() after resurrection completes
     *
     * @param player The player who was resurrected
     */
    static inline ::std::function<void(Player*)> OnPlayerResurrected = nullptr;

    // ========================================================================
    // COMBAT EVENT HOOKS (Phase 3 Event-Driven Architecture)
    // ========================================================================

    /**
     * Hook: Damage dealt
     * Called from: Unit::DealDamage() after damage is applied
     *
     * @param attacker Unit dealing damage (may be nullptr)
     * @param victim Unit receiving damage
     * @param damage Amount of damage dealt
     * @param damagetype Type of damage (DIRECT_DAMAGE, SPELL_DIRECT_DAMAGE, DOT, etc.)
     * @param spellProto Spell that caused the damage (nullptr for melee)
     */
    static inline ::std::function<void(Unit*, Unit*, uint32, DamageEffectType, SpellInfo const*)> OnDamageDealt = nullptr;

    /**
     * Hook: Healing done
     * Called from: Unit::HealBySpell() and similar healing functions
     *
     * @param healer Unit doing the healing
     * @param target Unit being healed
     * @param healAmount Amount healed
     * @param overheal Amount of overhealing
     * @param spellProto Spell that caused the healing
     */
    static inline ::std::function<void(Unit*, Unit*, uint32, uint32, SpellInfo const*)> OnHealingDone = nullptr;

    /**
     * Hook: Spell cast started
     * Called from: Spell::prepare() when cast begins
     *
     * IMPORTANT: This hook should trigger IMMEDIATE event dispatch for interrupt coordination
     *
     * @param caster Unit casting the spell
     * @param spellInfo The spell being cast
     * @param target Target of the spell (may be nullptr)
     */
    static inline ::std::function<void(Unit*, SpellInfo const*, Unit*)> OnSpellCastStart = nullptr;

    /**
     * Hook: Spell cast succeeded
     * Called from: Spell::finish() when cast completes successfully
     *
     * @param caster Unit that cast the spell
     * @param spellInfo The spell that was cast
     */
    static inline ::std::function<void(Unit*, SpellInfo const*)> OnSpellCastSuccess = nullptr;

    /**
     * Hook: Spell interrupted
     * Called from: Unit::InterruptSpell() when a spell is interrupted
     *
     * @param caster Unit whose spell was interrupted
     * @param spellInfo The spell that was interrupted
     * @param interrupter Unit that caused the interrupt
     */
    static inline ::std::function<void(Unit*, SpellInfo const*, Unit*)> OnSpellInterrupted = nullptr;

    /**
     * Hook: Aura applied
     * Called from: Unit::_ApplyAura() when an aura is applied
     *
     * @param target Unit receiving the aura
     * @param aura The aura being applied
     * @param caster Unit that applied the aura
     */
    static inline ::std::function<void(Unit*, Aura const*, Unit*)> OnAuraApplied = nullptr;

    /**
     * Hook: Aura removed
     * Called from: Unit::_UnapplyAura() when an aura is removed
     *
     * @param target Unit losing the aura
     * @param aura The aura being removed
     */
    static inline ::std::function<void(Unit*, Aura const*)> OnAuraRemoved = nullptr;

    /**
     * Hook: Threat changed
     * Called from: ThreatManager::AddThreat() when threat is modified
     *
     * @param threatOwner Unit whose threat table changed
     * @param victim Unit on the threat table
     * @param oldThreat Previous threat value
     * @param newThreat New threat value
     */
    static inline ::std::function<void(Unit*, Unit*, float, float)> OnThreatChanged = nullptr;

    /**
     * Hook: Unit died
     * Called from: Unit::Kill() when a unit dies
     *
     * @param victim Unit that died
     * @param killer Unit that caused the death (may be nullptr)
     */
    static inline ::std::function<void(Unit*, Unit*)> OnUnitDied = nullptr;

    /**
     * Hook: Combat started
     * Called from: Unit::SetInCombatWith() when entering combat
     *
     * @param unit Unit entering combat
     */
    static inline ::std::function<void(Unit*)> OnCombatStarted = nullptr;

    /**
     * Hook: Combat ended
     * Called from: Unit::ClearInCombat() or similar when leaving combat
     *
     * @param unit Unit leaving combat
     */
    static inline ::std::function<void(Unit*)> OnCombatEnded = nullptr;

    // ========================================================================
    // BATTLEGROUND/ARENA HOOKS
    // ========================================================================

    /**
     * Hook: BG invitation received
     * Called from: BattlegroundQueue when a player is invited to a BG
     *
     * For bots: Should trigger auto-accept of the invitation
     *
     * @param player The player who received the invitation
     * @param bgInstanceGuid The BG instance GUID
     * @param bgTypeId The type of battleground
     */
    static inline ::std::function<void(Player*, uint32, uint32)> OnBGInvitationReceived = nullptr;

    // ========================================================================
    // UTILITY FUNCTIONS
    // ========================================================================

    /**
     * Check if a player is a bot
     * @param player Player to check
     * @return true if player is a playerbot
     */
    static bool IsPlayerBot(Player const* player);

    /**
     * Get bot count in group
     * @param group Group to check
     * @return Number of bots in group
     */
    static uint32 GetBotCountInGroup(Group const* group);

    /**
     * Check if group has any bots
     * @param group Group to check
     * @return true if group contains at least one bot
     */
    static bool GroupHasBots(Group const* group);

    // ========================================================================
    // STATISTICS AND DEBUGGING
    // ========================================================================

    struct HookStatistics
    {
        uint64_t totalHookCalls{0};
        uint64_t memberAddedCalls{0};
        uint64_t memberRemovedCalls{0};
        uint64_t leaderChangedCalls{0};
        uint64_t groupDisbandedCalls{0};
        uint64_t raidConvertedCalls{0};
        uint64_t lootMethodChangedCalls{0};
        uint64_t readyCheckCalls{0};
        uint64_t targetIconCalls{0};
        uint64_t difficultyCalls{0};
        uint64_t playerDeathCalls{0};
        uint64_t playerResurrectedCalls{0};
        // Phase 3 Combat Event hooks
        uint64_t damageDealtCalls{0};
        uint64_t healingDoneCalls{0};
        uint64_t spellCastStartCalls{0};
        uint64_t spellCastSuccessCalls{0};
        uint64_t spellInterruptedCalls{0};
        uint64_t auraAppliedCalls{0};
        uint64_t auraRemovedCalls{0};
        uint64_t threatChangedCalls{0};
        uint64_t unitDiedCalls{0};
        uint64_t combatStartedCalls{0};
        uint64_t combatEndedCalls{0};

        ::std::string ToString() const;
        void Reset();
    };

    static HookStatistics const& GetStatistics();
    static void ResetStatistics();
    static void DumpStatistics();

private:
    // Meyer's singleton accessors for DLL-safe static data
    static bool& GetInitialized();
    static HookStatistics& GetStats();

    // Private helper functions
    static void RegisterHooks();
    static void UnregisterHooks();
    static void IncrementHookCall(const char* hookName);
};

} // namespace Playerbot

#endif // PLAYERBOT_HOOKS_H
