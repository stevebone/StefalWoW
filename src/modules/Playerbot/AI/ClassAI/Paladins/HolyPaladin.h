/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Holy Paladin Refactored - Template-Based Implementation
 *
 * This file provides a complete, template-based implementation of Holy Paladin
 * using the HealerSpecialization with dual resource system (Mana + Holy Power).
 */

#pragma once


#include "../Common/StatusEffectTracker.h"
#include "../Common/CooldownManager.h"
#include "../Common/RotationHelpers.h"
#include "../CombatSpecializationTemplates.h"
#include "../ResourceTypes.h"
#include "../SpellValidation_WoW120_Part2.h"  // Central spell registry
#include "../HeroTalentDetector.h"
#include "Player.h"
#include "SpellMgr.h"
#include "SpellAuraEffects.h"
#include "Group.h"
#include "Log.h"
#include "../../Services/HealingTargetSelector.h"  // Phase 5B: Unified healing service

// Phase 5 Integration: Decision Systems
#include "../../Decision/ActionPriorityQueue.h"
#include "../../Decision/BehaviorTree.h"
#include "../../BotAI.h"

namespace Playerbot
{

// ============================================================================
// HOLY PALADIN SPELL ALIASES (WoW 12.0 - The War Within)
// Consolidated spell IDs from central registry - NO duplicates
// ============================================================================

namespace HolyPaladinSpells
{
    // Holy Power Generators
    constexpr uint32 HOLY_SHOCK               = WoW120Spells::Paladin::Holy::HOLY_SHOCK;
    constexpr uint32 CRUSADER_STRIKE_HOLY     = WoW120Spells::Paladin::CRUSADER_STRIKE;
    constexpr uint32 JUDGMENT_HOLY            = WoW120Spells::Paladin::Holy::JUDGMENT_HOLY;

    // Holy Power Spenders
    constexpr uint32 WORD_OF_GLORY            = WoW120Spells::Paladin::WORD_OF_GLORY;
    constexpr uint32 LIGHT_OF_DAWN            = WoW120Spells::Paladin::Holy::LIGHT_OF_DAWN;
    constexpr uint32 SHIELD_OF_THE_RIGHTEOUS_HOLY = WoW120Spells::Paladin::Protection::SHIELD_OF_THE_RIGHTEOUS;

    // Direct Heals
    constexpr uint32 FLASH_OF_LIGHT           = WoW120Spells::Paladin::FLASH_OF_LIGHT;
    constexpr uint32 HOLY_LIGHT               = WoW120Spells::Paladin::HOLY_LIGHT;
    constexpr uint32 DIVINE_TOLL              = WoW120Spells::Paladin::Protection::DIVINE_TOLL;

    // AoE Heals
    constexpr uint32 LIGHT_OF_THE_MARTYR      = WoW120Spells::Paladin::Holy::LIGHT_OF_THE_MARTYR;
    constexpr uint32 BEACON_OF_LIGHT          = WoW120Spells::Paladin::Holy::BEACON_OF_LIGHT;
    constexpr uint32 BEACON_OF_FAITH          = WoW120Spells::Paladin::Holy::BEACON_OF_FAITH;
    constexpr uint32 BEACON_OF_VIRTUE         = WoW120Spells::Paladin::Holy::BEACON_OF_VIRTUE;

    // Cooldowns
    constexpr uint32 AVENGING_WRATH_HOLY      = WoW120Spells::Paladin::Holy::AVENGING_WRATH_HOLY;
    constexpr uint32 AVENGING_CRUSADER        = WoW120Spells::Paladin::Holy::AVENGING_CRUSADER;
    constexpr uint32 HOLY_AVENGER             = WoW120Spells::Paladin::Holy::HOLY_AVENGER;
    constexpr uint32 DIVINE_PROTECTION        = WoW120Spells::Paladin::DIVINE_PROTECTION;
    constexpr uint32 BLESSING_OF_SACRIFICE    = WoW120Spells::Paladin::BLESSING_OF_SACRIFICE;

    // Utility
    constexpr uint32 CLEANSE                  = WoW120Spells::Paladin::CLEANSE;
    constexpr uint32 BLESSING_OF_FREEDOM      = WoW120Spells::Paladin::BLESSING_OF_FREEDOM;
    constexpr uint32 BLESSING_OF_PROTECTION   = WoW120Spells::Paladin::BLESSING_OF_PROTECTION;
    constexpr uint32 LAY_ON_HANDS             = WoW120Spells::Paladin::LAY_ON_HANDS;
    constexpr uint32 DIVINE_SHIELD            = WoW120Spells::Paladin::DIVINE_SHIELD;

    // Buffs/Auras
    constexpr uint32 INFUSION_OF_LIGHT        = WoW120Spells::Paladin::Holy::INFUSION_OF_LIGHT;
    constexpr uint32 GLIMMER_OF_LIGHT         = WoW120Spells::Paladin::Holy::GLIMMER_OF_LIGHT;
    constexpr uint32 AURA_MASTERY             = WoW120Spells::Paladin::Holy::AURA_MASTERY;

    // Auras
    constexpr uint32 DEVOTION_AURA            = WoW120Spells::Paladin::DEVOTION_AURA;
    constexpr uint32 CONCENTRATION_AURA       = WoW120Spells::Paladin::CONCENTRATION_AURA;
    constexpr uint32 RETRIBUTION_AURA         = WoW120Spells::Paladin::RETRIBUTION_AURA;

    // Talents
    constexpr uint32 DIVINE_FAVOR             = WoW120Spells::Paladin::Holy::DIVINE_FAVOR;
    constexpr uint32 AWAKENING                = WoW120Spells::Paladin::Holy::AWAKENING;
    constexpr uint32 UNBREAKABLE_SPIRIT       = WoW120Spells::Paladin::Holy::UNBREAKABLE_SPIRIT;
}
using namespace HolyPaladinSpells;

// Import BehaviorTree helper functions (avoid conflict with Playerbot::Action)
using bot::ai::Sequence;
using bot::ai::Selector;
using bot::ai::Condition;
using bot::ai::Inverter;
using bot::ai::Repeater;
using bot::ai::NodeStatus;
using bot::ai::SpellPriority;
using bot::ai::SpellCategory;

// Note: bot::ai::Action() conflicts with Playerbot::Action, use bot::ai::Action() explicitly

// Dual resource type for Paladin - defined once with include guard
#ifndef PLAYERBOT_RESOURCE_TYPES_MANA_HOLY_POWER
#define PLAYERBOT_RESOURCE_TYPES_MANA_HOLY_POWER
struct ManaHolyPowerResource
{
    uint32 mana{0};
    uint32 holyPower{0};
    uint32 maxMana{100000};
    uint32 maxHolyPower{5};

    bool available{true};
    bool Consume(uint32 manaCost)
    {
        if (mana >= manaCost) {
            mana -= manaCost;
            return true;
        }
        return false;
    }
    void Regenerate(uint32 diff)
    {
        // Resource regeneration logic (simplified)
        available = true;
    }

    [[nodiscard]] uint32 GetAvailable() const {
        return 100; // Simplified - override in specific implementations
    }

    [[nodiscard]] uint32 GetMax() const {
        return 100; // Simplified - override in specific implementations
    }


    void Initialize(Player* /*bot*/)
    {
        // CRITICAL: NEVER call GetPower/GetMaxPower during construction!
        // Even with IsInWorld() check, power data may not be initialized during bot login.
        // Use static defaults here; refresh from player data in UpdateRotation later.
        maxMana = 100000;  // Standard max mana - will be refreshed when player data is ready
        mana = 100000;
        holyPower = 0;
    }

    // Call this from UpdateRotation when bot is fully ready
    void RefreshFromPlayer(Player* bot)
    {
        if (bot && bot->IsInWorld())
        {
            maxMana = bot->GetMaxPower(POWER_MANA);
            mana = bot->GetPower(POWER_MANA);
            holyPower = bot->GetPower(POWER_HOLY_POWER);
        }
    }
};
#endif // PLAYERBOT_RESOURCE_TYPES_MANA_HOLY_POWER

// ============================================================================
// HOLY PALADIN BEACON TRACKER
// ============================================================================

class HolyPaladinBeaconTracker
{
public:
    HolyPaladinBeaconTracker()
        : _primaryBeaconGuid()
        , _secondaryBeaconGuid()
        , _hasBeaconOfFaith(false)
    {
    }

    void SetPrimaryBeacon(ObjectGuid guid)
    {
        _primaryBeaconGuid = guid;
    }

    void SetSecondaryBeacon(ObjectGuid guid)
    {
        if (_hasBeaconOfFaith)
            _secondaryBeaconGuid = guid;
    }

    bool HasPrimaryBeacon() const { return !_primaryBeaconGuid.IsEmpty(); }
    bool HasSecondaryBeacon() const { return _hasBeaconOfFaith && !_secondaryBeaconGuid.IsEmpty(); }

    ObjectGuid GetPrimaryBeacon() const { return _primaryBeaconGuid; }
    ObjectGuid GetSecondaryBeacon() const { return _secondaryBeaconGuid; }

    void EnableBeaconOfFaith() { _hasBeaconOfFaith = true; }

    bool NeedsBeaconRefresh(Player* bot, Unit* target) const
    {
        if (!target || !bot)
            return false;

        // Check if target has beacon aura from this bot
        return !target->HasAura(BEACON_OF_LIGHT, bot->GetGUID());
    }

private:
    CooldownManager _cooldowns;
    ObjectGuid _primaryBeaconGuid;
    ObjectGuid _secondaryBeaconGuid;
    bool _hasBeaconOfFaith;
};

// ============================================================================
// HOLY PALADIN REFACTORED
// ============================================================================

class HolyPaladinRefactored : public HealerSpecialization<ManaHolyPowerResource>
{
public:
    // Use base class members with type alias for cleaner syntax
    using Base = HealerSpecialization<ManaHolyPowerResource>;
    using Base::GetBot;
    using Base::CastSpell;
    using Base::CanCastSpell;
    using Base::_resource;
    explicit HolyPaladinRefactored(Player* bot)        : HealerSpecialization<ManaHolyPowerResource>(bot)
        , _beaconTracker()
        , _avengingWrathActive(false)
        , _avengingWrathEndTime(0)
        , _infusionOfLightActive(false)
        , _lastHolyShockTime(0)
    {
        // Initialize mana/holy power resources
        this->_resource.Initialize(bot);

        // Check if bot has Beacon of Faith talent
        if (bot->HasSpell(BEACON_OF_FAITH))
            _beaconTracker.EnableBeaconOfFaith();

        // Initialize Phase 5 systems
        InitializeHolyPaladinMechanics();

        // Register healing spell efficiency tiers
        GetEfficiencyManager().RegisterSpell(HOLY_LIGHT, HealingSpellTier::VERY_HIGH, "Holy Light");
        GetEfficiencyManager().RegisterSpell(FLASH_OF_LIGHT, HealingSpellTier::HIGH, "Flash of Light");
        GetEfficiencyManager().RegisterSpell(HOLY_SHOCK, HealingSpellTier::VERY_HIGH, "Holy Shock");
        GetEfficiencyManager().RegisterSpell(WORD_OF_GLORY, HealingSpellTier::VERY_HIGH, "Word of Glory");
        GetEfficiencyManager().RegisterSpell(LIGHT_OF_DAWN, HealingSpellTier::MEDIUM, "Light of Dawn");
        GetEfficiencyManager().RegisterSpell(LAY_ON_HANDS, HealingSpellTier::EMERGENCY, "Lay on Hands");
        GetEfficiencyManager().RegisterSpell(DIVINE_SHIELD, HealingSpellTier::EMERGENCY, "Divine Shield");
        GetEfficiencyManager().RegisterSpell(BEACON_OF_LIGHT, HealingSpellTier::VERY_HIGH, "Beacon of Light");

        // Note: Do NOT call bot->GetName() here - Player data may not be loaded yet
        TC_LOG_DEBUG("playerbot", "HolyPaladinRefactored created for bot GUID: {}",
            bot ? bot->GetGUID().GetCounter() : 0);
    }

    void UpdateRotation(::Unit* target) override
    {
        if (!this->GetBot())
            return;

        // Lazy hero talent detection (once per bot lifetime until respec)
        if (!_heroTalents.detected)
            _heroTalents.Refresh(this->GetBot());

        // Update Holy Paladin state
        UpdateHolyPaladinState();

        // Healers focus on group healing, not target damage
        ExecuteHealingRotation();
    }

    void UpdateBuffs() override
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        // Maintain Devotion Aura
        if (!bot->HasAura(DEVOTION_AURA) && this->CanCastSpell(DEVOTION_AURA, bot))
        {
            this->CastSpell(DEVOTION_AURA, bot);
        }

        // Emergency defensive
        if (bot->GetHealthPct() < 20.0f && this->CanCastSpell(DIVINE_SHIELD, bot))
        {
            this->CastSpell(DIVINE_SHIELD, bot);
        }
    }

protected:
    void ExecuteHealingRotation()
    {
        Player* bot = this->GetBot();
        Group* group = bot->GetGroup();        // Update beacon targets
        UpdateBeacons(group);

        // Hero talent tree rotation branching
        // Holy Paladin has access to: Herald of the Sun / Lightsmith
        if (_heroTalents.IsTree(HeroTalentTree::HERALD_OF_THE_SUN))
        {
            // Herald of the Sun: Dawnlight empowers healing with solar energy
            // Cast Dawnlight on injured allies for HoT effect
            Unit* dawnlightTarget = SelectHealingTarget(group);
            if (dawnlightTarget && dawnlightTarget->GetHealthPct() < 80.0f)
            {
                if (this->CanCastSpell(WoW120Spells::Paladin::Holy::DAWNLIGHT, dawnlightTarget))
                {
                    this->CastSpell(WoW120Spells::Paladin::Holy::DAWNLIGHT, dawnlightTarget);
                    return;
                }
            }
            // Eternal Flame for sustained healing
            if (dawnlightTarget && dawnlightTarget->GetHealthPct() < 70.0f)
            {
                if (this->CanCastSpell(WoW120Spells::Paladin::Holy::ETERNAL_FLAME, dawnlightTarget))
                {
                    this->CastSpell(WoW120Spells::Paladin::Holy::ETERNAL_FLAME, dawnlightTarget);
                    return;
                }
            }
        }
        else if (_heroTalents.IsTree(HeroTalentTree::LIGHTSMITH))
        {
            // Lightsmith: Holy Armament creates sacred weapons for allies
            // Apply Sacred Weapon buff to tank or highest-priority ally
            Player* tank = GetMainTank(group);
            if (tank && !tank->HasAura(WoW120Spells::Paladin::Holy::SACRED_WEAPON))
            {
                if (this->CanCastSpell(WoW120Spells::Paladin::Holy::HOLY_ARMAMENT, tank))
                {
                    this->CastSpell(WoW120Spells::Paladin::Holy::HOLY_ARMAMENT, tank);
                    return;
                }
            }
        }

        // Emergency healing
        if (HandleEmergencyHealing(group))
            return;

        // Use Holy Power spenders
        if (this->_resource.holyPower >= 3)
        {
            if (ExecuteHolyPowerSpender(group))
                return;
        }

        // Use major cooldowns
        if (ShouldUseAvengingWrath(group))
        {
            if (this->CanCastSpell(AVENGING_WRATH_HOLY, bot))
            {
                this->CastSpell(AVENGING_WRATH_HOLY, bot);
                _avengingWrathActive = true;
                _avengingWrathEndTime = GameTime::GetGameTimeMS() + 20000;
                return;
            }
        }

        // Generate Holy Power with Holy Shock
        if (this->_resource.holyPower < 5)
        {
            Unit* healTarget = SelectHealingTarget(group);
            if (healTarget && this->CanCastSpell(HOLY_SHOCK, healTarget))
            {
                this->CastSpell(HOLY_SHOCK, healTarget);
                _lastHolyShockTime = GameTime::GetGameTimeMS();
                GenerateHolyPower(1);
                return;
            }
        }

        // Direct healing
        Unit* healTarget = SelectHealingTarget(group);
        if (healTarget)
        {
            ExecuteDirectHealing(healTarget);
        }
    }

    bool HandleEmergencyHealing(Group* group)
    {
        Player* bot = this->GetBot();

        // Self emergency        if (bot->GetHealthPct() < 25.0f)
        {
            // Lay on Hands
            if (this->CanCastSpell(LAY_ON_HANDS, bot))
            {
                this->CastSpell(LAY_ON_HANDS, bot);
                return true;
            }

            // Word of Glory
            if (this->_resource.holyPower >= 3 && this->CanCastSpell(WORD_OF_GLORY, bot))
            {
                this->CastSpell(WORD_OF_GLORY, bot);
                ConsumeHolyPower(3);
                return true;
            }
        }

        // Group emergency
        if (group)
        {
            for (GroupReference const& ref : group->GetMembers())
            {
                if (Player* member = ref.GetSource())                {
                    if (member->IsAlive() && member->GetHealthPct() < 20.0f)
                    {
                        // Lay on Hands on critical ally
                        if (this->CanCastSpell(LAY_ON_HANDS, member))
                        {
                            this->CastSpell(LAY_ON_HANDS, member);
                            return true;
                        }

                        // Flash of Light for speed
                        if (_infusionOfLightActive && IsHealAllowedByMana(FLASH_OF_LIGHT) && this->CanCastSpell(FLASH_OF_LIGHT, member))
                        {
                            this->CastSpell(FLASH_OF_LIGHT, member);
                            _infusionOfLightActive = false;
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    bool ExecuteHolyPowerSpender(Group* group)
    {
        // Check for multiple injured allies
        uint32 injuredCount = CountInjuredAllies(group, 0.7f);

        if (injuredCount >= 3)
        {
            // Light of Dawn for AoE healing
            if (IsHealAllowedByMana(LIGHT_OF_DAWN) && this->CanCastSpell(LIGHT_OF_DAWN, this->GetBot()))
            {
                this->CastSpell(LIGHT_OF_DAWN, this->GetBot());
                ConsumeHolyPower(3);
                return true;
            }
        }

        // Single target Word of Glory
        Unit* target = SelectHealingTarget(group);
        if (target && target->GetHealthPct() < 80.0f)
        {
            if (this->CanCastSpell(WORD_OF_GLORY, target))
            {
                this->CastSpell(WORD_OF_GLORY, target);
                ConsumeHolyPower(3);
                return true;
            }
        }

        return false;
    }

    void ExecuteDirectHealing(Unit* target)
    {
        if (!target)
            return;

        float healthPct = target->GetHealthPct();

        // Critical: Flash of Light
        if (healthPct < 50.0f)
        {
            if (IsHealAllowedByMana(FLASH_OF_LIGHT) && this->CanCastSpell(FLASH_OF_LIGHT, target))
            {
                this->CastSpell(FLASH_OF_LIGHT, target);
                return;
            }
        }

        // Moderate: Holy Light
        if (healthPct < 80.0f)
        {
            if (this->CanCastSpell(HOLY_LIGHT, target))
            {
                this->CastSpell(HOLY_LIGHT, target);
                return;
            }
        }
    }

    void UpdateBeacons(Group* group)
    {
        if (!group)
            return;

        // Assign beacon to main tank
        Player* tank = GetMainTank(group);
        if (tank && _beaconTracker.NeedsBeaconRefresh(this->GetBot(), tank))
        {
            if (this->CanCastSpell(BEACON_OF_LIGHT, tank))
            {
                this->CastSpell(BEACON_OF_LIGHT, tank);
                _beaconTracker.SetPrimaryBeacon(tank->GetGUID());
            }
        }

        // Assign second beacon if talented
        if (_beaconTracker.HasSecondaryBeacon())
        {
            Player* secondTank = GetOffTank(group);
            if (secondTank && _beaconTracker.NeedsBeaconRefresh(this->GetBot(), secondTank))
            {
                if (this->CanCastSpell(BEACON_OF_FAITH, secondTank))
                {
                    this->CastSpell(BEACON_OF_FAITH, secondTank);
                    _beaconTracker.SetSecondaryBeacon(secondTank->GetGUID());
                }
            }
        }
    }

    Unit* SelectHealingTarget(Group* group)
    {
        // Use unified HealingTargetSelector service (Phase 5B integration)
        // Eliminates 20+ lines of duplicated healing target logic
        Unit* target = bot::ai::HealingTargetSelector::SelectTarget(this->GetBot());
        return target ? target : this->GetBot();
    }

private:
    void UpdateHolyPaladinState()
    {
        uint32 now = GameTime::GetGameTimeMS();

        // Update Avenging Wrath
        if (_avengingWrathActive && now >= _avengingWrathEndTime)
        {
            _avengingWrathActive = false;
            _avengingWrathEndTime = 0;
        }

        // Update Infusion of Light proc
        if (_infusionOfLightActive)
        {
            // Check if buff expired
            if (!this->GetBot()->HasAura(INFUSION_OF_LIGHT))
                _infusionOfLightActive = false;
        }
        else
        {
            // Check if proc occurred
            if (this->GetBot()->HasAura(INFUSION_OF_LIGHT))
                _infusionOfLightActive = true;
        }

        // Update Holy Power from bot
        if (this->GetBot())
            this->_resource.holyPower = this->GetBot()->GetPower(POWER_HOLY_POWER);
    }

    bool ShouldUseAvengingWrath(Group* group) const
    {
        // Use when multiple allies injured
        uint32 injuredCount = CountInjuredAllies(group, 0.6f);
        return injuredCount >= 3;
    }

    uint32 CountInjuredAllies(Group* group, float threshold) const
    {
        if (!group)
            return 0;

        uint32 count = 0;
        for (GroupReference const& ref : group->GetMembers())
        {
            if (Player* member = ref.GetSource())
            {
                if (member->IsAlive() && member->GetHealthPct() < (threshold * 100.0f))
                
                    count++;
            }
        }
        return count;
    }

    Player* GetMainTank(Group* group) const
    {
        if (!group)
            return nullptr;

        // Find tank role
        for (GroupReference const& ref : group->GetMembers())
        {
            if (Player* member = ref.GetSource())
            {
                if (IsTank(member))
                    return member;
            }
        }
        return nullptr;
    }

    Player* GetOffTank(Group* group) const
    {
        if (!group)
            return nullptr;

        Player* mainTank = GetMainTank(group);
        for (GroupReference const& ref : group->GetMembers())
        {
            if (Player* member = ref.GetSource())
            {
                if (IsTank(member) && member != mainTank)
                    return member;
            }
        }
        return nullptr;
    }

    bool IsTank(Player* player) const
    {
        if (!player)
            return false;

        // Simplified tank detection - check if player is currently tanking
        // A more robust implementation would check spec, but talent API is deprecated
        if (Unit* victim = player->GetVictim())
            return victim->GetVictim() == player;
            return false;
    }

    void GenerateHolyPower(uint32 amount)
    {
        this->_resource.holyPower = ::std::min(this->_resource.holyPower + amount, this->_resource.maxHolyPower);
    }

    void ConsumeHolyPower(uint32 amount)
    {
        this->_resource.holyPower = (this->_resource.holyPower > amount) ? this->_resource.holyPower - amount : 0;
    }

    void InitializeHolyPaladinMechanics()
    {        // REMOVED: using namespace bot::ai; (conflicts with ::bot::ai::)
        // REMOVED: using namespace BehaviorTreeBuilder; (not needed)
        // ========================================================================
        // PHASE 5 INTEGRATION: ActionPriorityQueue (Healer Focus)
        // ========================================================================
        BotAI* ai = this;
        if (!ai)
            return;

        auto* queue = ai->GetActionPriorityQueue();
        if (queue)
        {
            // ====================================================================
            // EMERGENCY TIER - Life-saving heals
            // ====================================================================
            queue->RegisterSpell(LAY_ON_HANDS,
                SpellPriority::EMERGENCY,
                SpellCategory::HEALING);
            queue->AddCondition(LAY_ON_HANDS,
                [](Player* bot, Unit* target) {
                    return target && target->GetHealthPct() < 20.0f;
                },
                "Target HP < 20% (Lay on Hands)");

            queue->RegisterSpell(DIVINE_SHIELD,
                SpellPriority::EMERGENCY,
                SpellCategory::DEFENSIVE);
            queue->AddCondition(DIVINE_SHIELD,
                [](Player* bot, Unit*) {
                    return bot->GetHealthPct() < 15.0f;
                },
                "Self HP < 15% (Divine Shield)");

            // ====================================================================
            // CRITICAL TIER - Holy Power spenders and fast heals
            // ====================================================================
            queue->RegisterSpell(WORD_OF_GLORY,
                SpellPriority::CRITICAL,
                SpellCategory::HEALING);
            queue->AddCondition(WORD_OF_GLORY,
                [this](Player* bot, Unit* target) {
                    // Use Word of Glory when we have 3+ HP and target needs healing
                    return this->_resource.holyPower >= 3 &&
                           target && target->GetHealthPct() < 70.0f;
                },
                "3+ HP and target < 70%");

            queue->RegisterSpell(LIGHT_OF_DAWN,
                SpellPriority::CRITICAL,
                SpellCategory::HEALING);
            queue->AddCondition(LIGHT_OF_DAWN,
                [this](Player* bot, Unit*) {
                    // AoE heal when 3+ HP and multiple injured
                    Group* group = bot->GetGroup();
                    uint32 injured = this->CountInjuredAllies(group, 0.7f);
                    return this->_resource.holyPower >= 3 && injured >= 3;
                },
                "3+ HP and 3+ allies injured");

            queue->RegisterSpell(FLASH_OF_LIGHT,
                SpellPriority::CRITICAL,
                SpellCategory::HEALING);
            queue->AddCondition(FLASH_OF_LIGHT,
                [](Player* bot, Unit* target) {
                    return target && target->GetHealthPct() < 40.0f;
                },
                "Target HP < 40% (fast heal)");

            // ====================================================================
            // HIGH TIER - Holy Power generation and emergency defensive
            // ====================================================================
            queue->RegisterSpell(HOLY_SHOCK,
                SpellPriority::HIGH,
                SpellCategory::HEALING);
            queue->AddCondition(HOLY_SHOCK,
                [this](Player* bot, Unit* target) {
                    // Generate HP when below max, or use for emergency healing
                    return (this->_resource.holyPower < 5 && target && target->GetHealthPct() < 90.0f) ||
                           (target && target->GetHealthPct() < 60.0f);
                },
                "HP < 5 or target < 60%");

            queue->RegisterSpell(DIVINE_PROTECTION,
                SpellPriority::HIGH,
                SpellCategory::DEFENSIVE);
            queue->AddCondition(DIVINE_PROTECTION,
                [](Player* bot, Unit*) {
                    return bot->GetHealthPct() < 50.0f;
                },
                "Self HP < 50% (damage reduction)");

            queue->RegisterSpell(BLESSING_OF_SACRIFICE,
                SpellPriority::HIGH,
                SpellCategory::UTILITY);
            queue->AddCondition(BLESSING_OF_SACRIFICE,
                [](Player* bot, Unit* target) {
                    // Redirect tank damage when they're in danger
                    return target && target->GetHealthPct() < 35.0f &&
                           bot->GetHealthPct() > 60.0f;
                },
                "Tank < 35% and self > 60%");

            // ====================================================================
            // MEDIUM TIER - Standard healing and cooldowns
            // ====================================================================
            queue->RegisterSpell(HOLY_LIGHT,
                SpellPriority::MEDIUM,
                SpellCategory::HEALING);
            queue->AddCondition(HOLY_LIGHT,
                [](Player* bot, Unit* target) {
                    return target && target->GetHealthPct() < 85.0f;
                },
                "Target HP < 85% (efficient heal)");

            queue->RegisterSpell(AVENGING_WRATH_HOLY,
                SpellPriority::MEDIUM,
                SpellCategory::OFFENSIVE);
            queue->AddCondition(AVENGING_WRATH_HOLY,
                [this](Player* bot, Unit*) {
                    // Use when multiple allies injured
                    Group* group = bot->GetGroup();
                    return this->CountInjuredAllies(group, 0.6f) >= 3;
                },
                "3+ allies injured (healing boost)");

            queue->RegisterSpell(BEACON_OF_LIGHT,
                SpellPriority::MEDIUM,
                SpellCategory::UTILITY);
            queue->AddCondition(BEACON_OF_LIGHT,
                [this](Player* bot, Unit* target) {
                    return target && this->_beaconTracker.NeedsBeaconRefresh(bot, target);
                },
                "Beacon needs refresh");

            queue->RegisterSpell(DIVINE_TOLL,
                SpellPriority::MEDIUM,
                SpellCategory::HEALING);
            queue->AddCondition(DIVINE_TOLL,
                [this](Player* bot, Unit*) {
                    // Burst HP generation
                    Group* group = bot->GetGroup();
                    return this->_resource.holyPower < 3 &&
                           this->CountInjuredAllies(group, 0.7f) >= 2;
                },
                "HP < 3 and 2+ injured");

            // ====================================================================
            // LOW TIER - Utility and maintenance
            // ====================================================================
            queue->RegisterSpell(CLEANSE,
                SpellPriority::LOW,
                SpellCategory::UTILITY);

            queue->RegisterSpell(BLESSING_OF_FREEDOM,
                SpellPriority::LOW,
                SpellCategory::UTILITY);

            queue->RegisterSpell(BLESSING_OF_PROTECTION,
                SpellPriority::LOW,
                SpellCategory::DEFENSIVE);

            TC_LOG_INFO("module.playerbot", " HOLY PALADIN: Registered {} spells in ActionPriorityQueue",
                queue->GetSpellCount());
        }

        // ========================================================================
        // PHASE 5 INTEGRATION: BehaviorTree (Healer Flow)
        // ========================================================================
        auto* behaviorTree = ai->GetBehaviorTree();
        if (behaviorTree)
        {
            auto root = Selector("Holy Paladin Healer", {
                // ================================================================
                // TIER 1: EMERGENCY HEALING (HP < 20%)
                // ================================================================
                Sequence("Emergency Healing", {
                    Condition("Critical HP < 20%", [](Player* bot, Unit*) {
                        // Check self or group for critical HP
                        if (bot->GetHealthPct() < 20.0f)
                            return true;

                        Group* group = bot->GetGroup();
                        if (!group)
                            return false;

                        for (GroupReference const& ref : group->GetMembers())
                        {
                            if (Player* member = ref.GetSource())
                            {
                                if (member->IsAlive() && member->GetHealthPct() < 20.0f)
                                    return true;
                            }
                        }
                        return false;
                    }),
                    Selector("Emergency Response", {
                        // Lay on Hands for critical allies
                        bot::ai::Action("Cast Lay on Hands", [this](Player* bot, Unit*) {
                            Unit* criticalTarget = this->SelectHealingTarget(bot->GetGroup());
                            if (criticalTarget && criticalTarget->GetHealthPct() < 20.0f &&
                                this->CanCastSpell(LAY_ON_HANDS, criticalTarget))
                            {
                                this->CastSpell(LAY_ON_HANDS, criticalTarget);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        }),
                        // Divine Shield for self
                        bot::ai::Action("Cast Divine Shield", [this](Player* bot, Unit*) {
                            if (bot->GetHealthPct() < 15.0f &&
                                this->CanCastSpell(DIVINE_SHIELD, bot))
                            {
                                this->CastSpell(DIVINE_SHIELD, bot);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        }),
                        // Word of Glory emergency spend
                        bot::ai::Action("Cast Word of Glory", [this](Player* bot, Unit*) {
                            if (this->_resource.holyPower >= 3)
                            {
                                Unit* healTarget = this->SelectHealingTarget(bot->GetGroup());
                                if (healTarget && healTarget->GetHealthPct() < 30.0f &&
                                    this->CanCastSpell(WORD_OF_GLORY, healTarget))
                                {
                                    this->CastSpell(WORD_OF_GLORY, healTarget);
                                    this->ConsumeHolyPower(3);
                                    return NodeStatus::SUCCESS;
                                }
                            }
                            return NodeStatus::FAILURE;
                        }),
                        // Flash of Light spam
                        bot::ai::Action("Cast Flash of Light", [this](Player* bot, Unit*) {
                            Unit* healTarget = this->SelectHealingTarget(bot->GetGroup());
                            if (healTarget && healTarget->GetHealthPct() < 25.0f &&
                                this->CanCastSpell(FLASH_OF_LIGHT, healTarget))
                            {
                                this->CastSpell(FLASH_OF_LIGHT, healTarget);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        })
                    })
                }),

                // ================================================================
                // TIER 2: HOLY POWER MANAGEMENT
                // ================================================================
                Sequence("Holy Power Management", {
                    Selector("HP Generation and Spending", {
                        // Spend HP when at max
                        Sequence("Spend Holy Power", {
                            Condition("HP >= 3", [this](Player* bot, Unit*) {
                                return this->_resource.holyPower >= 3;
                            }),
                            Selector("HP Spender Priority", {
                                // Light of Dawn for AoE
                                Sequence("Light of Dawn AoE", {
                                    Condition("3+ injured", [this](Player* bot, Unit*) {
                                        Group* group = bot->GetGroup();
                                        return this->CountInjuredAllies(group, 0.7f) >= 3;
                                    }),
                                    bot::ai::Action("Cast Light of Dawn", [this](Player* bot, Unit*) {
                                        if (this->CanCastSpell(LIGHT_OF_DAWN, bot))
                                        {
                                            this->CastSpell(LIGHT_OF_DAWN, bot);
                                            this->ConsumeHolyPower(3);
                                            return NodeStatus::SUCCESS;
                                        }
                                        return NodeStatus::FAILURE;
                                    })
                                }),
                                // Word of Glory single target
                                bot::ai::Action("Cast Word of Glory", [this](Player* bot, Unit*) {
                                    Unit* healTarget = this->SelectHealingTarget(bot->GetGroup());
                                    if (healTarget && healTarget->GetHealthPct() < 80.0f &&
                                        this->CanCastSpell(WORD_OF_GLORY, healTarget))
                                    {
                                        this->CastSpell(WORD_OF_GLORY, healTarget);
                                        this->ConsumeHolyPower(3);
                                        return NodeStatus::SUCCESS;
                                    }
                                    return NodeStatus::FAILURE;
                                })
                            })
                        }),
                        // Generate HP
                        Sequence("Generate Holy Power", {
                            Condition("HP < 5", [this](Player* bot, Unit*) {
                                return this->_resource.holyPower < 5;
                            }),
                            Selector("HP Generator Priority", {
                                // Holy Shock
                                bot::ai::Action("Cast Holy Shock", [this](Player* bot, Unit*) {
                                    Unit* healTarget = this->SelectHealingTarget(bot->GetGroup());
                                    if (healTarget && healTarget->GetHealthPct() < 90.0f &&
                                        this->CanCastSpell(HOLY_SHOCK, healTarget))
                                    {
                                        this->CastSpell(HOLY_SHOCK, healTarget);
                                        this->GenerateHolyPower(1);
                                        return NodeStatus::SUCCESS;
                                    }
                                    return NodeStatus::FAILURE;
                                }),
                                // Divine Toll burst
                                bot::ai::Action("Cast Divine Toll", [this](Player* bot, Unit*) {
                                    if (this->_resource.holyPower < 3 &&
                                        this->CanCastSpell(DIVINE_TOLL, bot))
                                    {
                                        this->CastSpell(DIVINE_TOLL, bot);
                                        return NodeStatus::SUCCESS;
                                    }
                                    return NodeStatus::FAILURE;
                                })
                            })
                        })
                    })
                }),

                // ================================================================
                // TIER 3: BEACON MAINTENANCE
                // ================================================================
                Sequence("Beacon Maintenance", {
                    Condition("Has group", [](Player* bot, Unit*) {
                        return bot->GetGroup() != nullptr;
                    }),
                    Selector("Beacon Priority", {
                        // Primary beacon on tank
                        bot::ai::Action("Cast Beacon of Light", [this](Player* bot, Unit*) {
                            Player* tank = this->GetMainTank(bot->GetGroup());
                            if (tank && this->_beaconTracker.NeedsBeaconRefresh(bot, tank) &&
                                this->CanCastSpell(BEACON_OF_LIGHT, tank))
                            {
                                this->CastSpell(BEACON_OF_LIGHT, tank);
                                this->_beaconTracker.SetPrimaryBeacon(tank->GetGUID());
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        }),
                        // Secondary beacon if talented
                        bot::ai::Action("Cast Beacon of Faith", [this](Player* bot, Unit*) {
                            if (this->_beaconTracker.HasSecondaryBeacon())
                            {
                                Player* secondTank = this->GetOffTank(bot->GetGroup());
                                if (secondTank && this->_beaconTracker.NeedsBeaconRefresh(bot, secondTank) &&
                                    this->CanCastSpell(BEACON_OF_FAITH, secondTank))
                                {
                                    this->CastSpell(BEACON_OF_FAITH, secondTank);
                                    this->_beaconTracker.SetSecondaryBeacon(secondTank->GetGUID());
                                    return NodeStatus::SUCCESS;
                                }
                            }
                            return NodeStatus::FAILURE;
                        })
                    })
                }),

                // ================================================================
                // TIER 4: STANDARD HEALING ROTATION
                // ================================================================
                Sequence("Standard Healing", {
                    Selector("Healing Priority", {
                        // Cooldown usage
                        Sequence("Use Avenging Wrath", {
                            Condition("3+ injured", [this](Player* bot, Unit*) {
                                Group* group = bot->GetGroup();
                                return this->CountInjuredAllies(group, 0.6f) >= 3;
                            }),
                            bot::ai::Action("Cast Avenging Wrath", [this](Player* bot, Unit*) {
                                if (this->CanCastSpell(AVENGING_WRATH_HOLY, bot))
                                {
                                    this->CastSpell(AVENGING_WRATH_HOLY, bot);
                                    this->_avengingWrathActive = true;
                                    this->_avengingWrathEndTime = GameTime::GetGameTimeMS() + 20000;
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),

                        // Flash of Light for moderate damage
                        Sequence("Flash of Light", {
                            Condition("Target < 50%", [this](Player* bot, Unit*) {
                                Unit* healTarget = this->SelectHealingTarget(bot->GetGroup());
                                return healTarget && healTarget->GetHealthPct() < 50.0f;
                            }),
                            bot::ai::Action("Cast Flash of Light", [this](Player* bot, Unit*) {
                                Unit* healTarget = this->SelectHealingTarget(bot->GetGroup());
                                if (healTarget && this->CanCastSpell(FLASH_OF_LIGHT, healTarget))
                                {
                                    this->CastSpell(FLASH_OF_LIGHT, healTarget);
                                    return NodeStatus::SUCCESS;
                                }
                                return NodeStatus::FAILURE;
                            })
                        }),

                        // Holy Light for efficient healing
                        bot::ai::Action("Cast Holy Light", [this](Player* bot, Unit*) {
                            Unit* healTarget = this->SelectHealingTarget(bot->GetGroup());
                            if (healTarget && healTarget->GetHealthPct() < 85.0f &&
                                this->CanCastSpell(HOLY_LIGHT, healTarget))
                            {
                                this->CastSpell(HOLY_LIGHT, healTarget);
                                return NodeStatus::SUCCESS;
                            }
                            return NodeStatus::FAILURE;
                        })
                    })
                })
            });

            behaviorTree->SetRoot(root);
            TC_LOG_INFO("module.playerbot", " HOLY PALADIN: BehaviorTree initialized with healer flow");
        }
    }



private:
    HolyPaladinBeaconTracker _beaconTracker;
    bool _avengingWrathActive;
    uint32 _avengingWrathEndTime;
    bool _infusionOfLightActive;
    uint32 _lastHolyShockTime;
    HeroTalentCache _heroTalents;
};

} // namespace Playerbot
