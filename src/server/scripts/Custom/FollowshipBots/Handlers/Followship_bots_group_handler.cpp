#include "Log.h"
#include "ObjectAccessor.h"
#include "SpellAuras.h"

#include "Followship_bots_utils.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_db.h"

#include "Followship_bots_group_handler.h"

namespace FSBGroup
{
    void BuildLogicalBotGroup(Creature* bot, std::vector<Unit*>& outGroup)
    {
        outGroup.clear();

        Player* owner = FSBMgr::Get()->GetBotOwner(bot);

        if (!owner || !bot)
            return;

        outGroup.push_back(owner);

        auto botsPtr = FSBMgr::Get()->GetPersistentBotsForPlayer(owner);
        if (!botsPtr)
        {
            TC_LOG_DEBUG("scripts.fsb.general", "FSB: BuildLogicalBotGroup Player has no bots");
            return;
        }

        for (auto const& botData : *botsPtr)
        {
            if (botData.runtimeGuid.IsEmpty())
                continue;

            if (Unit* botUnit = ObjectAccessor::GetUnit(*owner, botData.runtimeGuid))
            {
                outGroup.push_back(botUnit);
                //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BuildLogicalGroup. Bot: {} got added {} to group of: {}", bot->GetName(), botUnit->GetName(), outGroup.size());  // TEMP-LOG
            }
        }

        
    }

    Unit* BotGetFirstMemberToAssist(Creature* bot, const std::vector<Unit*>& botGroup)
    {
        if (!bot || !bot->IsInWorld() || bot->IsDuringRemoveFromWorld())
            return nullptr;

        Unit* bestTarget = nullptr;
        float lowestHpPct = 100.f;

        for (Unit* member : botGroup)
        {
            if (!member || member == bot)
                continue;

            if (!member->IsAlive() || !member->IsInWorld() || member->IsDuringRemoveFromWorld())
                continue;

            // Case 1: Member is being attacked
            Unit* attacker = member->getAttackerForHelper();
            if (attacker && attacker->IsAlive() && attacker->IsInWorld() && !attacker->IsDuringRemoveFromWorld())
            {
                // Skip CC
                if (!attacker->HasBreakableByDamageCrowdControlAura() &&
                    !member->HasBreakableByDamageCrowdControlAura())
                {
                    float hpPct = member->GetHealthPct();
                    if (hpPct < lowestHpPct)
                    {
                        lowestHpPct = hpPct;
                        bestTarget = attacker;
                    }
                }
            }

            // Case 2: Member is attacking something (victim)
            Unit* victim = member->GetVictim();
            if (victim && victim->IsAlive() && victim->IsInWorld() && !victim->IsDuringRemoveFromWorld())
            {
                // Skip CC
                if (!victim->HasBreakableByDamageCrowdControlAura() &&
                    !member->HasBreakableByDamageCrowdControlAura())
                {
                    float hpPct = member->GetHealthPct();
                    if (hpPct < lowestHpPct)
                    {
                        lowestHpPct = hpPct;
                        bestTarget = victim;
                    }
                }
            }
        }

        return bestTarget;
    }

    Unit* BotGetFirstGroupHealer(const std::vector<Unit*>& group)
    {
        for (Unit* member : group)
        {
            if (!member || !member->IsAlive())
                continue;

            // Check healer role
            if (member->ToCreature() && FSBUtils::BotIsHealerClass(member->ToCreature()))
            {
                return member; // first healer found
            }
        }

        return nullptr; // no healer in group
    }

    Unit* BotGetFirstGroupTank(const std::vector<Unit*>& botGroup)
    {
        for (Unit* member : botGroup)
        {
            if (!member || !member->IsAlive())
                continue;

            FSB_Roles role = FSBMgr::Get()->GetRole(member->ToCreature());

            // Check healer role
            if (role == FSB_Roles::FSB_ROLE_TANK)
            {
                return member; // first healer found
            }
        }

        return nullptr; // no healer in group
    }

    Unit* BotGetFirstDeadMember(const std::vector<Unit*>& botGroup)
    {

        for (Unit* unit : botGroup)
        {
            if (!unit)
                continue;

            if (!unit->IsAlive())
                return unit;
        }

        return nullptr;
    }

    Unit* BotGetDispelMember(Creature* bot, const DispelAbility& ability)
    {
        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return nullptr;

        for (Unit* member : baseAI->botLogicalGroup)
        {
            if (!member || !member->IsInWorld())
                continue;

            // Scan all auras on the member
            for (auto const& auraPair : member->GetAppliedAuras())
            {
                Aura* aura = auraPair.second->GetBase();
                if (!aura)
                    continue;

                SpellInfo const* info = aura->GetSpellInfo();
                if (!info)
                    continue;

                // Only negative auras
                if (info->IsPositive())
                    continue;


                DispelType type = FSBSpellsUtils::ConvertAuraToDispelType(aura);
                if (std::find(ability.dispels.begin(), ability.dispels.end(), type) != ability.dispels.end())
                {
                    return member; // Found someone we can dispel
                }
            }
        }

        return nullptr;
    }

    std::vector<Unit*> BotGetMembersToHeal(const std::vector<Unit*>& group, float lowHpThreshold)
    {
        std::vector<Unit*> candidates;

        for (Unit* unit : group)
        {
            if (!unit || !unit->IsAlive())
                continue;

            // Emergency 1: HP below threshold
            if (unit->GetHealthPct() <= lowHpThreshold)
            {
                candidates.push_back(unit);
                //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Heal / Emergency Candidate: {}", unit->GetName());
            }
        }

        // Optional: sort by urgency (lowest HP first)
        std::sort(candidates.begin(), candidates.end(),
            [](Unit* a, Unit* b) { return a->GetHealthPct() < b->GetHealthPct(); });

        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Heal / Emergency list size: {}", candidates.size());
        return candidates;
    }

    float CalculateEmergencyPriority(Unit* unit)
    {
        if (!unit)
            return 0.f;

        // Get role (works for any bot)
        FSB_Roles role = FSBMgr::Get()->GetRole(unit->ToCreature());

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
                return CalculateEmergencyPriority(a) > CalculateEmergencyPriority(b);
            });
    }

    bool BotGroupIsHealthy_Average(Creature* bot, uint32 groupHP)
    {
        if (!bot || !bot->IsAlive())
            return true;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        auto& botGroup = baseAI->botLogicalGroup;

        if (botGroup.empty())
            return true;

        float totalPct = 0.f;
        uint32 count = 0;

        for (Unit* u : botGroup)
        {
            if (!u || !u->IsAlive())
                return false;

            totalPct += u->GetHealthPct();
            count++;
        }

        float avg = totalPct / count;
        return avg >= groupHP;
    }
}
