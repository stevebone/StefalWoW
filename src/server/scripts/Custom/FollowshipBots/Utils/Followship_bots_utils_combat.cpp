#include <algorithm>

#include "ScriptMgr.h"
#include "Unit.h"
#include "Creature.h"

#include "Followship_bots_utils_combat.h"
#include "followship_bots_utils.h"

namespace FSBUtilsCombat
{
    bool IsCombatActive(Unit* me)
    {
        if (!me)
            return false;

        Unit* owner = me->GetOwner();
        Player* player = owner ? owner->ToPlayer() : nullptr;

        return me->IsInCombat() || (player && player->IsInCombat());
    }

    uint8 CountActiveAttackers(Unit* me)
    {
        if (!me)
            return 0;

        uint8 count = 0;

        for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
        {
            Unit* u = ref->GetVictim();
            if (!u || !u->IsAlive())
                continue;

            if (!me->IsInMap(u))
                continue;

            ++count;
        }

        return count;
    }

    uint8 CountAttackersOn(Unit* who)
    {
        if (!who)
            return 0;

        uint8 count = 0;

        for (ThreatReference const* ref : who->GetThreatManager().GetUnsortedThreatList())
        {
            Unit* u = ref->GetVictim();
            if (u && u->IsAlive() && u->GetVictim() == who)
                ++count;
        }

        return count;
    }

    void SayCombatMessage(Creature* me, Unit* target, uint32 integer, FSBSayType sayType, uint32 spellId)
    {
        if (!me)
            return;

        uint32 now = getMSTime();

        // Cooldown to prevent spamming
        static uint32 nextSayMs = 0;
        if (now < nextSayMs)
            return;

        // Spell name lookup
        std::string spellName;
        if (spellId)
            spellName = FSBUtilsSpells::GetSpellName(spellId);

        // Target name
        std::string targetName;
        if (target)
            targetName = target->GetName();

        // Build the message
        std::string msg = FSBUtilsTexts::BuildNPCSayText(targetName, integer, sayType, spellName);
        me->Say(msg, LANG_UNIVERSAL);

        // Set next say cooldown: random between 3-5 minutes
        nextSayMs = now + urand(3 * MINUTE * IN_MILLISECONDS, 5 * MINUTE * IN_MILLISECONDS);
    }

    Unit* FSBUtilsCombat::SelectDamageTarget(Creature* me, const std::vector<Unit*>& group, Unit* playerTarget)
    {
        if (!me)
            return nullptr;

        Unit* owner = me->GetOwner();
        Player* player = owner ? owner->ToPlayer() : nullptr;

        FSB_Roles role = FSBUtils::GetRole(me);

        std::vector<Unit*> combatTargets;
        auto const& threatList = me->GetThreatManager().GetSortedThreatList();
        for (ThreatReference const* ref : threatList)
        {
            if (Unit* u = ref->GetVictim())
                combatTargets.push_back(u);
        }

        

        // Player target exists?
        if (playerTarget && playerTarget->IsAlive())
        {
            if (role == FSB_ROLE_DAMAGE || role == FSB_ROLE_ASSIST || role == FSB_ROLE_TANK)
                return playerTarget;
        }

        switch (role)
        {
        case FSB_ROLE_TANK:
        {
            // Tank bots prefer player target first
            if (playerTarget && playerTarget->IsAlive())
                return playerTarget;

            if (player)
            {
                auto const& pthreatList = player->GetThreatManager().GetSortedThreatList();
                for (ThreatReference const* ref : pthreatList)
                {
                    if (Unit* u = ref->GetVictim())
                        combatTargets.push_back(u);
                }
            }

            std::sort(combatTargets.begin(), combatTargets.end());
            combatTargets.erase(std::unique(combatTargets.begin(), combatTargets.end()), combatTargets.end());

            // Then bot's threat list
            if (!combatTargets.empty())
                return combatTargets.front(); // could prioritize highest threat

            break;
        }

        case FSB_ROLE_HEALER:
        {
            // Healers prioritize tank
            Unit* tank = nullptr;
            for (Unit* u : group)
            {
                if (!u || !u->IsAlive())
                    continue;
                if (FSBUtils::GetRole(u->ToCreature()) == FSB_ROLE_TANK)
                {
                    tank = u;
                    break;
                }
            }

            if (tank)
                return tank->GetVictim();

            if (playerTarget && playerTarget->IsAlive())
                return playerTarget;

            std::sort(combatTargets.begin(), combatTargets.end());
            combatTargets.erase(std::unique(combatTargets.begin(), combatTargets.end()), combatTargets.end());

            // Fallback: bot's own threat list
            if (!combatTargets.empty())
                return combatTargets.front();

            break;
        }

        case FSB_ROLE_DAMAGE:
        case FSB_ROLE_ASSIST:
        default:
        {
            if (playerTarget && playerTarget->IsAlive())
                return playerTarget;

            Unit* tank = nullptr;
            for (Unit* u : group)
            {
                if (!u || !u->IsAlive())
                    continue;
                if (FSBUtils::GetRole(u->ToCreature()) == FSB_ROLE_TANK)
                {
                    tank = u;
                    break;
                }
            }

            if (tank)
                return tank->GetVictim();
            else
            {

                if (player)
                {
                    auto const& pthreatList = player->GetThreatManager().GetSortedThreatList();
                    for (ThreatReference const* ref : pthreatList)
                    {
                        if (Unit* u = ref->GetVictim())
                            combatTargets.push_back(u);
                    }
                }
            }

            std::sort(combatTargets.begin(), combatTargets.end());
            combatTargets.erase(std::unique(combatTargets.begin(), combatTargets.end()), combatTargets.end());

            if (!combatTargets.empty())
                return combatTargets.front();

            break;
        }
        }

        // Fallback: pick any alive enemy in threat list
        for (Unit* u : combatTargets)
        {
            if (u && u->IsAlive())
                return u;
        }

        return nullptr;
    }
    Unit* SelectHealTarget(Unit* me, const std::vector<Unit*>& group)
    {
        TC_LOG_DEBUG("scripts.ai.fsb", "FSB Combat: SelectHealTarget Triggered");
        TC_LOG_DEBUG("scripts.ai.fsb", "SelectHealTarget called: me={}, role={}, hp={}", me->GetName(), FSBUtils::GetRole(me->ToCreature()), me->GetHealthPct());

        if (!me)
            return nullptr;

        FSB_Roles myRole = FSBUtils::GetRole(me->ToCreature());
        Player* owner = me->GetOwner() ? me->GetOwner()->ToPlayer() : nullptr;

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB Combat: SelectHealTarget Bot Role: {}", myRole);

        // -------------------------------
        // ROLE: HEALER
        // -------------------------------
        if (myRole == FSB_ROLE_HEALER)
        {
            // 1. Tank @ 80%
            for (Unit* u : group)
            {
                if (!u || !u->IsAlive())
                    continue;

                if (FSBUtils::GetRole(u->ToCreature()) == FSB_ROLE_TANK &&
                    u->GetHealthPct() < 80.0f)
                {
                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB Combat: SelectHealTarget Found Tank to heal");
                    return u;
                }
            }

            // 2. Owner player @ 80%
            if (owner && owner->IsAlive() && owner->GetHealthPct() < 80.0f)
            {
                TC_LOG_DEBUG("scripts.ai.fsb", "FSB Combat: SelectHealTarget Found Owner to heal");
                return owner;
            }

            // 3. Any other group member @ 50%
            Unit* lowest = nullptr;
            float lowestPct = 50.0f;

            for (Unit* u : group)
            {
                if (!u || !u->IsAlive())
                    continue;

                if (u->GetHealthPct() < lowestPct)
                {
                    lowestPct = u->GetHealthPct();
                    lowest = u;
                }
            }

            return lowest; // nullptr if nobody qualifies
        }

        // -------------------------------
        // ROLE: ASSIST
        // -------------------------------
        if (myRole == FSB_ROLE_ASSIST)
        {
            Unit* lowest = nullptr;
            float lowestPct = 50.0f;

            for (Unit* u : group)
            {
                if (!u || !u->IsAlive())
                    continue;

                if (u->GetHealthPct() < lowestPct)
                {
                    lowestPct = u->GetHealthPct();
                    lowest = u;
                }
            }

            return lowest;
        }

        // -------------------------------
        // ROLE: TANK
        // -------------------------------
        if (myRole == FSB_ROLE_TANK)
        {
            if (me->GetHealthPct() < 80.0f)
                return me;

            return nullptr;
        }

        // -------------------------------
        // ROLE: DAMAGE
        // -------------------------------
        if (myRole == FSB_ROLE_DAMAGE)
        {
            if (me->GetHealthPct() < 50.0f)
            {
                TC_LOG_DEBUG("scripts.ai.fsb", "FSB Combat Damage : Bot Role Damage HP < 50. ");
                return me;
            }

            return nullptr;
        }

        return nullptr;
    }

    std::vector<Unit*> GetEmergencyCandidates(const std::vector<Unit*>& group, float lowHpThreshold)
    {
        std::vector<Unit*> emergencies;

        for (Unit* unit : group)
        {
            if (!unit || !unit->IsAlive())
                continue;

            // Emergency 1: HP below threshold
            if (unit->GetHealthPct() <= lowHpThreshold)
            {
                emergencies.push_back(unit);
                continue;
            }

            // Emergency 2: dispellable debuff
            if (FSBUtilsSpells::HasDispellableDebuff(unit))
            {
                emergencies.push_back(unit);
                continue;
            }

            // You can add more emergency checks here in the future
            // e.g., specific negative auras, debuffs, status effects
        }

        // Optional: sort by urgency (lowest HP first)
        std::sort(emergencies.begin(), emergencies.end(),
            [](Unit* a, Unit* b) { return a->GetHealthPct() < b->GetHealthPct(); });

        return emergencies;
    }
    float CalculateEmergencyPriority(Unit* unit)
    {
        if (!unit)
            return 0.f;

        // Get role (works for any bot)
        FSB_Roles role = FSBUtils::GetRole(unit->ToCreature());

        // Assign priority values (higher = more urgent)
        if (role == FSB_ROLE_HEALER)           // healer
            return 80.f;
        if (role == FSB_ROLE_TANK)             // tank
            return 100.f;

        // If player, slightly lower than tank
        if (unit->IsPlayer())
            return 90.f;

        // Default: based on missing health percentage
        return 50.f + (100.f - unit->GetHealthPct()); // 50..150 based on HP
    }
    void SortEmergencyTargets(std::vector<Unit*>& targets)
    {
        std::sort(targets.begin(), targets.end(), [](Unit* a, Unit* b)
            {
                return FSBUtilsCombat::CalculateEmergencyPriority(a) > FSBUtilsCombat::CalculateEmergencyPriority(b);
            });
    }

    void BuildBotGroup(Unit* me, std::vector<Unit*>& outGroup, float searchRange)
    {
        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BuildBotGroup triggered"); // TEMP-LOG

        outGroup.clear();

        if (!me || !me->ToCreature()->IsBot())
            return;

        Player* owner = me->GetOwner() ? me->GetOwner()->ToPlayer() : nullptr;
        if (!owner)
            return;

        // Always include owner and self
        outGroup.push_back(owner);
        outGroup.push_back(me);

        // Find nearby allies
        std::list<Unit*> nearbyAllies;

        Trinity::AnyFriendlyUnitInObjectRangeCheck checker(me, me, searchRange, false);
        Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(me, nearbyAllies, checker);

        Cell::VisitAllObjects(me, searcher, searchRange);

        for (Unit* unit : nearbyAllies)
        {
            if (!unit || unit == me || unit == owner)
                continue;

            if (!unit->IsBot())
                continue;

            if (unit->GetOwnerGUID() != owner->GetGUID())
                continue;

            outGroup.push_back(unit);
        }

        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BuildBotGroup. We got allies of {}", nearbyAllies.size());  // TEMP-LOG
        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BuildBotGroup. We got group of {}", outGroup.size());  // TEMP-LOG
    }
}
