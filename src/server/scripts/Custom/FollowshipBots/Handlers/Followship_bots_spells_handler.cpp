#include "Followship_bots_utils.h"

#include "Followship_bots_druid.h"
#include "Followship_bots_mage.h"
#include "Followship_bots_paladin.h"
#include "Followship_bots_priest.h"
#include "Followship_bots_warlock.h"
#include "Followship_bots_warrior.h"

#include "Followship_bots_group_handler.h"
#include "Followship_bots_powers_handler.h"
#include "Followship_bots_spells_handler.h"

std::unordered_map<FSB_Class, FSBSpellTable const*> sBotSpellTables;

namespace FSBSpells
{
    void HandleOnSpellCast(Creature* bot, FSB_Class botClass, uint32 spellId)
    {
        if (!bot || !spellId)
            return;

        HandleOnSpellCastGeneric(bot, spellId);

        switch (botClass)
        {
        case FSB_Class::Warrior:
            FSBWarrior::HandleOnSpellCast(bot, spellId);
            break;
        case FSB_Class::Priest:
            break;
        case FSB_Class::Mage:
            break;
        case FSB_Class::Rogue:
            break;
        case FSB_Class::Druid:
            FSBDruid::HandleOnSpellCast(bot, spellId);
            break;
        case FSB_Class::Paladin:
            FSBPaladin::HandleOnSpellCast(bot, spellId);
            break;
        case FSB_Class::Hunter:
            break;
        case FSB_Class::Warlock:
            FSBWarlock::HandleOnSpellCast(bot, spellId);
            break;
        default:
            break;
        }

    }

    void HandleOnSpellCastGeneric(Creature* bot, uint32 spellId)
    {
        if (!bot || !spellId)
            return;

        // Health potions
        switch (spellId)
        {
        case 439:
            bot->ModifyHealth(120);
            break;
        case 440:
            bot->ModifyHealth(160);
            break;
        case 4042:
            bot->ModifyHealth(500);
            break;
        case 17534:
            bot->ModifyHealth(650);
            break;
        case 28495:
            bot->ModifyHealth(800);
            break;
        case 43185:
            bot->ModifyHealth(1000);
            break;
        default:
            break;
        }
    }

    void InitBotSpellTables()
    {
        sBotSpellTables[FSB_Class::Priest] = &PriestSpellsTable;
        sBotSpellTables[FSB_Class::Mage] = &MageSpellsTable;
        sBotSpellTables[FSB_Class::Warrior] = &WarriorSpellsTable;
        sBotSpellTables[FSB_Class::Paladin] = &PaladinSpellsTable;
        sBotSpellTables[FSB_Class::Warlock] = &WarlockSpellsTable;
        sBotSpellTables[FSB_Class::Druid] = &DruidSpellsTable;
    }

    void InitSpellRuntime(Creature* bot, std::vector<FSBSpellRuntime>& _runtimeSpells)
    {
        _runtimeSpells.clear();

        FSB_Class botClass = FSBUtils::GetBotClassForEntry(bot->GetEntry());
        auto table = GetBotSpellTableForClass(botClass);
        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: InitSpellRuntime() - Bot: {} has Class: {}", bot->GetName(), botClass);

        if (!table)
            return;

        for (FSBSpellDefinition const& def : *table)
        {
            FSBSpellRuntime runtime;
            runtime.def = &def;
            runtime.nextReadyMs = 0;
            _runtimeSpells.push_back(runtime);
        }
    }


    FSBSpellTable const* GetBotSpellTableForClass(FSB_Class botClass)
    {
        auto itr = sBotSpellTables.find(botClass);
        if (itr == sBotSpellTables.end())
        {
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: SpellTableForClass not found");
            return nullptr;
        }

        return itr->second;
    }

    std::vector<FSBSpellRuntime*> BotGetAvailableSpells(Creature* bot, std::vector<FSBSpellRuntime>& runtimeSpells, FSBSpellType desiredType, bool requireSelfCast)
    {
        std::vector<FSBSpellRuntime*> available;
        FSB_Roles botRole = FSBUtils::GetRole(bot);
        uint32 botRoleMask = RoleToMask(botRole);
        uint32 now = getMSTime();

        for (auto& runtime : runtimeSpells)
        {
            FSBSpellDefinition const* def = runtime.def;
            if (!def)
                continue;

            SpellInfo const* spellInfo =
                sSpellMgr->GetSpellInfo(def->spellId, bot->GetMap()->GetDifficultyID());
            if (!spellInfo)
                continue;

            // 1. Spell type filter
            if (desiredType != FSBSpellType::Any &&
                def->type != desiredType)
                continue;

            // 2. Self-cast filter
            if (requireSelfCast && !def->isSelfCast)
                continue;


            // Role check (STATIC)
            if (def->allowedRoles != FSB_ROLEMASK_ANY &&
                (def->allowedRoles & botRoleMask) == 0)
                continue;

            // Per-spell cooldown (RUNTIME)
            if (runtime.nextReadyMs > now)
                continue;

            TC_LOG_DEBUG("scripts.ai.fsb", "Bot: {} with role: {} has available spell: {}", bot->GetName(), botRole, FSBSpellsUtils::GetSpellName(def->spellId));
            available.push_back(&runtime);

        }

        return available;
    }

    FSBSpellRuntime* SelectBestHealSpell(const std::vector<FSBSpellRuntime*>& heals, Unit* target)
    {
        if (!target)
            return nullptr;

        float hpPct = target->GetHealthPct();

        FSBSpellRuntime* best = nullptr;
        float bestScore = -1.f;

        for (auto* runtime : heals)
        {
            auto* def = runtime->def;

            // Check if aura present
            if (target->HasAura(def->spellId))
                continue;

            // Check HP threshold
            if (hpPct > def->hpThreshold)
                continue;

            // Chance roll
            if (urand(0, 99) > def->chance)
                continue;

            // Score: lower threshold = more urgent
            float score = 100.f - def->hpThreshold;

            if (score > bestScore)
            {
                bestScore = score;
                best = runtime;
            }
        }

        return best;
    }

    FSBSpellRuntime* SelectBestDamageSpell(Creature* bot, const std::vector<FSBSpellRuntime*>& damageSpells, Unit* target)
    {
        if (!bot || !bot->IsAlive())
            return nullptr;

        if (!target || !target->IsAlive())
            return nullptr;

        if (damageSpells.empty())
        {
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BotSelectSpell - no available spell");
            return nullptr;
        }

        // =========================================================
        // ================== BUILD SPELL POOL =====================
        // =========================================================

        std::vector<FSBSpellRuntime*> spellPool;

        for (FSBSpellRuntime* runtime : damageSpells)
        {
            if (!runtime || !runtime->def)
                continue;

            if (runtime->def->type == FSBSpellType::Damage)
                spellPool.push_back(runtime);
        }

        if (spellPool.empty())
        {
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: SelectSpell - damage spell pool empty after filtering");
            return nullptr;
        }

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: SelectSpell - spellPool size after filtering: {}", spellPool.size());

        // =========================================================
        // ===================== SPELLS PICK =======================
        // =========================================================
        Trinity::Containers::RandomShuffle(spellPool);

        for (FSBSpellRuntime* runtime : spellPool)
        {
            FSBSpellDefinition const* spell = runtime->def;

            if (!runtime || !runtime->def)
                continue;

            if (spell->isSelfCast)
                target = bot;

            // Check target has aura
            if(target->HasAura(spell->spellId))
                continue;

            // Chance roll
            uint32 roll = urand(0, 100);
            if (roll > spell->chance)
            {
                TC_LOG_DEBUG("scripts.ai.fsb", "FSB: SpellSkip - {} chance failed (roll={} chance={})", spell->spellId, roll, spell->chance);
                continue;
            }

            // Class Spells validation
            if (!FSBUtilsSpells::IsSpellClassValid(bot, spell->spellId, target))
            {
                //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: SpellSkip - class invalid for spell {}", spell->spellId);
                continue;
            }

            // ================= SUCCESS =================

            /*
            TC_LOG_DEBUG("scripts.ai.fsb",
                "FSB: SpellSelect SUCCESS - spell={} type={} target={} hp={} threshold={}",
                spell->spellId,
                spell->type == FSBSpellType::Heal ? "HEAL" : "DAMAGE",
                target->GetName(),
                target->GetHealthPct(),
                spell->hpThreshold
            );
            */

            return runtime;
        }

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: SelectSpell - no valid spell found after evaluation");
        return nullptr;
    }

    bool BotTryDispel(Creature* bot)
    {
        if (!bot)
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        FSB_Class cls = baseAI->botClass;

        auto it = DispelTable.find(cls);
        if (it == DispelTable.end())
            return false; // class cannot dispel anything

        const DispelAbility& ability = it->second;

        if (!FSBSpellsUtils::IsSpellReady(ability.spellId))
            return false;

        Unit* target = FSBGroup::BotGetDispelMember(bot, ability);
        if (!target)
            return false;

        if (bot->GetDistance(target) > 30.0f)
            return false;

        return BotCastSpellWithCooldown(bot, ability.spellId, target, ability.cooldown);
    }

    bool BotTryOffensiveDispel(Creature* bot)
    {
        if (!bot)
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        FSB_Class cls = baseAI->botClass;

        auto it = OffensiveDispelTable.find(cls);
        if (it == OffensiveDispelTable.end())
            return false; // class cannot offensively dispel

        const OffensiveDispelAbility& ability = it->second;

        Unit* target = bot->GetVictim();
        if (!target)
            return false;

        // Check if target has a buff we can remove/steal
        Aura* buff = FSBSpellsUtils::FindEnemyBuffToDispel(target);
        if (!buff)
            return false;

        // Optional: avoid dispelling trivial buffs
        if (buff->GetSpellInfo()->Dispel != DISPEL_MAGIC)
            return false;

        // Cast the spell
        return BotCastSpell(bot, ability.spellId, target);
    }

    bool BotCastSpell(Creature* bot, uint32 spellId, Unit* target)
    {
        if (!bot || !target)
            return false;

        if (!spellId)
            return false;

        bot->SetFacingToObject(target, true);

        SpellCastResult result = bot->CastSpell(target, spellId, true);

        if (result == SPELL_CAST_OK)
        {
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB Bot {} casted spell {} on target {}", bot->GetName(), FSBSpellsUtils::GetSpellName(spellId), target->GetName());
            return true;
        }
        else TC_LOG_DEBUG("scripts.ai.fsb", "FSB Bot {} Unable to cast spell {} on target {} with result {}", bot->GetName(), FSBSpellsUtils::GetSpellName(spellId), target->GetName(), result);

        return false;
    }

    bool BotCastSpellWithCooldown(Creature* bot, uint32 spellId, Unit* target, uint32 cooldown)
    {
        if (!bot || !target)
            return false;

        if (!spellId)
            return false;

        bot->SetFacingToObject(target, true);

        SpellCastResult result = bot->CastSpell(target, spellId, true);

        if (result == SPELL_CAST_OK)
        {
            FSBSpellsUtils::PutSpellOnCooldown(spellId, cooldown);
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB Bot {} casted spell {} on target {}", bot->GetName(), FSBSpellsUtils::GetSpellName(spellId), target->GetName());
            return true;
        }

        return false;
    }
}
