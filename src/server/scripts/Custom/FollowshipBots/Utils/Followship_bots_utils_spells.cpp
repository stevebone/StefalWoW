#include <random>

#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "SpellMgr.h"
#include "ThreatManager.h"
#include "Unit.h"

#include "Followship_bots_priest.h"
#include "Followship_bots_utils.h"
#include "Followship_bots_utils_spells.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_utils_priest.h"
#include "Followship_bots_utils_combat.h"



using FSBSpellTable = std::vector<FSBSpellDefinition>;
static std::unordered_map<FSB_Class, FSBSpellTable const*> sBotSpellTables;

namespace FSBUtilsSpells
{
    int32 GetDrinkManaRegenPerLevel(uint8 level, DrinkManaPerLevel const* table, size_t tableSize)
    {
        for (size_t i = 0; i < tableSize; ++i)
        {
            if (level >= table[i].minLevel && level <= table[i].maxLevel)
                return table[i].mana;
        }

        return 0;
    }

    // Wrapper for GetDrinkManaRegenPerLevel
    int32 GetDrinkManaRegen(uint16 level)
    {
        return GetDrinkManaRegenPerLevel(
            level,
            DrinkManaRegenTable,
            std::size(DrinkManaRegenTable)
        );
    }

    // Gets the Mana Potion spell id on a provided level
    uint32 GetManaPotionSpellForLevel(uint8 level)
    {
        for (auto const& entry : ManaPotionTable)
        {
            if (level >= entry.minLevel && level <= entry.maxLevel)
                return entry.spellId;
        }
        return 0;
    }

    std::string GetSpellName(uint32 spellId)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo || !spellInfo->SpellName)
            return "unknown spell";

        std::string name = spellInfo->SpellName->Str[LOCALE_enUS];
        return name.empty() ? "unknown spell" : name;
    }

    bool CanCastNow(Unit* me, uint32 now, uint32 globalCooldownUntil)
    {
        return me
            && now >= globalCooldownUntil
            && !me->HasUnitState(UNIT_STATE_CASTING);
    }

    bool HasDispellableDebuff(Unit* unit)
    {
        if (!unit)
            return false;

        for (auto const& pair : unit->GetAppliedAuras())
        {
            AuraApplication const* aurApp = pair.second;
            if (!aurApp)
                continue;

            Aura const* aura = aurApp->GetBase();
            if (!aura)
                continue;

            // Skip positive auras
            if (aurApp->IsPositive())
                continue;

            SpellInfo const* info = aura->GetSpellInfo();
            if (!info)
                continue;

            // Check dispel type
            switch (info->Dispel)
            {
            case DISPEL_MAGIC:
            case DISPEL_DISEASE:
            case DISPEL_POISON:
            case DISPEL_CURSE:
                break;
            default:
                continue;
            }

            // Skip undispellable spells
            //if (info->HasAttribute(SPELL_ATTR0_CANT_BE_DISPELLED))
            //    continue;

            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Spell casting: found unit: {} with dispellable aura.", unit->GetName());

            return true;
        }

        return false;
    }

    bool IsSpellClassValid(Creature* bot, uint32 spellId, Unit* target)
    {
        switch (spellId)
        {

            // Can only be cast on more than 2/3 attackers
        case SPELL_PRIEST_PSYCHIC_SCREAM:
        {
            Player* player = FSBMgr::GetBotOwner(bot)->ToPlayer();

            if (!player)
            {
                uint8 attackers = FSBUtilsCombat::CountAttackersOn(bot);
                return attackers >= 2;
            }

            uint8 attackers =
                FSBUtilsCombat::CountAttackersOn(bot) +
                (player ? FSBUtilsCombat::CountAttackersOn(player) : 0);

            return attackers >= 3;
        }

        case SPELL_PRIEST_DESPERATE_PRAYER:
        {
            return target == bot;
        }

        case SPELL_PRIEST_PURIFY:
        {
            return target && HasDispellableDebuff(target);
        }

        default:
            return true;
        }
    }

    void GetBotBuffTargets(Creature* bot, uint32 buffSpellId, std::vector<Unit*> botGroup_, float maxRange, std::vector<Unit*>& outTargets)
    {
        if (!bot)
            return;

        for (Unit* member : botGroup_)
        {
            if (!member)
                continue;

            if (!member->IsAlive())
                continue;

            if (member->HasAura(buffSpellId))
                continue;

            if (!bot->IsWithinDistInMap(member, maxRange))
                continue;

            if (!bot->IsWithinLOSInMap(member))
                continue;

            outTargets.push_back(member);
        }
    }

    Unit* FindBotDeadResTarget(Creature* bot, std::vector<Unit*> const& botGroup_)
    {
        Player* owner = FSBMgr::GetBotOwner(bot);

        if (owner && !owner->IsAlive())
            return owner;

        for (Unit* unit : botGroup_)
        {
            if (!unit || unit == owner)
                continue;

            if (!unit->IsAlive())
                return unit;
        }

        return nullptr;
    }


    MountSpellList const* GetMountSpellsForLevel(uint8 level)
    {
        MountSpellList const* result = nullptr;

        for (auto const& [minLevel, spells] : MountSpellsByLevel)
        {
            if (level < minLevel)
                break;

            result = &spells;
        }

        return result;
    }

    uint32 GetRandomMountSpellForBot(Creature* me)
    {
        if (!me)
            return 0;

        MountSpellList const* spells = GetMountSpellsForLevel(me->GetLevel());
        if (!spells || spells->empty())
            return 0;

        return (*spells)[urand(0, spells->size() - 1)];
    }

    void CastRandomMountLevelSpell(Creature* me)
    {
        uint32 spellId = GetRandomMountSpellForBot(me);
        if (!spellId)
            return;

        me->CastSpell(me, spellId, true);
    }
}

namespace FSBUtilsCombatSpells
{
    void InitBotSpellTables()
    {
        sBotSpellTables[FSB_Class::Priest] = &PriestSpellsTable;
        //sBotSpellTables[FSB_Class::Mage] = &MageSpellsTable;
        //sBotSpellTables[FSB_Class::Warrior] = &WarriorSpellsTable;
    }

    void InitSpellRuntime(Creature* bot, std::vector<FSBSpellRuntime>& _runtimeSpells)
    {
        _runtimeSpells.clear();

        FSB_Class botClass = FSBUtils::GetBotClassForEntry(bot->GetEntry());
        auto table = FSBUtilsCombatSpells::GetBotSpellTableForClass(botClass);
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

    std::vector<FSBSpellRuntime*> BotGetAvailableSpells(Creature* bot, std::vector<FSBSpellRuntime>& runtimeSpells, uint32& globalCooldownUntil)
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

            // Role check (STATIC)
            if (def->allowedRoles != FSB_ROLEMASK_ANY &&
                (def->allowedRoles & botRoleMask) == 0)
                continue;

            // Global cooldown (BOT-level)
            if (!FSBUtilsSpells::CanCastNow(bot, now, globalCooldownUntil))
                continue;

            // Per-spell cooldown (RUNTIME)
            if (runtime.nextReadyMs > now)
                continue;

            //TC_LOG_DEBUG("scripts.ai.fsb", "Bot: {} with role: {} has available spell: {}", bot->GetName(), botRole, FSBUtilsSpells::GetSpellName(def->spellId));
            available.push_back(&runtime);

        }

        return available;
    }

    FSBSpellRuntime* BotSelectSpell(Creature* bot, std::vector<FSBSpellRuntime*>& availableSpells, std::vector<Unit*> botGroup_, Unit*& outTarget)
    {
        outTarget = nullptr;

        if (availableSpells.empty())
        {
            //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BotSelectSpell - no available spell");
            return nullptr;
        }

        // =========================================================
        // ===================== ROLE / MODE =======================
        // =========================================================

        bool isHealer = (FSBUtils::GetRole(bot) == FSB_Roles::FSB_ROLE_HEALER);
        bool isHybrid = (FSBUtils::GetRole(bot) == FSB_Roles::FSB_ROLE_ASSIST);

        std::vector<Unit*> emergencyTargets;
        std::vector<Unit*> healTargets;

        if (isHealer)
        {
            emergencyTargets = FSBUtilsCombat::BotGetHealCandidates(botGroup_, 40.f);
            healTargets = FSBUtilsCombat::BotGetHealCandidates(botGroup_, 70.f);
        }

        enum class SpellMode
        {
            EmergencyHeal,
            NormalHeal,
            DamageOnly,
            Hybrid
        };

        SpellMode mode = SpellMode::DamageOnly;

        if (isHealer)
        {
            if (!emergencyTargets.empty())
                mode = SpellMode::EmergencyHeal;
            else if (!healTargets.empty())
                mode = SpellMode::NormalHeal;
            else
                mode = SpellMode::DamageOnly;
        }

        if (isHybrid)
            mode = SpellMode::Hybrid;

        /*
        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: SelectSpell - role={} mode={} emergencyTargets={} healTargets={}",
            isHealer ? "HEALER" : "NON-HEALER",
            mode == SpellMode::EmergencyHeal ? "EMERGENCY" :
            mode == SpellMode::NormalHeal ? "HEAL" :
            mode == SpellMode::DamageOnly ? "DAMAGE" : "HYBRID",
            emergencyTargets.size(),
            healTargets.size()
        );
        */

        // =========================================================
        // ================== BUILD SPELL POOL =====================
        // =========================================================

        std::vector<FSBSpellRuntime*> spellPool;

        for (FSBSpellRuntime* runtime : availableSpells)
        {
            switch (mode)
            {
            case SpellMode::EmergencyHeal:
            case SpellMode::NormalHeal:
                if (runtime->def->type == FSBSpellType::Heal)
                    spellPool.push_back(runtime);
                break;

            case SpellMode::DamageOnly:
                if (runtime->def->type == FSBSpellType::Damage)
                    spellPool.push_back(runtime);
                break;

            case SpellMode::Hybrid:
                spellPool.push_back(runtime);
                break;
            }
        }

        if (spellPool.empty())
        {
            //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: SelectSpell - spell pool empty after filtering");
            return nullptr;
        }

        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: SelectSpell - spellPool size after filtering: {}", spellPool.size());

        // =========================================================
        // ===================== SPELLS PICK =======================
        // =========================================================
        Trinity::Containers::RandomShuffle(spellPool);

        for (FSBSpellRuntime* runtime : spellPool)
        {
            FSBSpellDefinition const* spell = runtime->def;

            // Chance roll
            uint32 roll = urand(0, 100);
            if (roll > spell->chance)
            {
                //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: SpellSkip - {} chance failed (roll={} chance={})", spell->spellId, roll, spell->chance);
                continue;
            }

            Unit* target = nullptr;

            // =================================================
            // ================= TARGET PICK ===================
            // =================================================

            if (spell->type == FSBSpellType::Heal)
            {
                if (mode == SpellMode::EmergencyHeal)
                {
                    target = emergencyTargets.front();
                }
                else if (mode == SpellMode::NormalHeal)
                {
                    target = healTargets.front();
                }
                else if (mode == SpellMode::Hybrid)
                {
                    auto candidates = FSBUtilsCombat::BotGetHealCandidates(botGroup_, spell->hpThreshold);
                    if (!candidates.empty())
                        target = candidates.front();
                }

                if (!target)
                {
                    //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: SpellSkip - heal {} no valid target", spell->spellId);
                    continue;
                }

                float hp = target->GetHealthPct();
                if (hp > spell->hpThreshold)
                {
                    //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: SpellSkip - heal {} target hp {} > threshold {}", spell->spellId, hp, spell->hpThreshold);
                    continue;
                }
            }
            else if (spell->type == FSBSpellType::Damage)
            {
                target = bot->GetVictim();

                if (!target || !target->IsAlive())
                {
                    //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: SpellSkip - damage {} no valid victim", spell->spellId);
                    continue;
                }

                float dist = bot->GetDistance(target);
                if (dist > spell->dist)
                {
                    //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: SpellSkip - damage {} target too far ({}/{})", spell->spellId, dist, spell->dist);
                    continue;
                }
            }

            

                // Class Spells validation
                if (!FSBUtilsSpells::IsSpellClassValid(bot, spell->spellId, target)) 
                {
                    //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: SpellSkip - class invalid for spell {}", spell->spellId);
                    continue;
                }

                // ================= SUCCESS =================
                outTarget = target;

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

            //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: SelectSpell - no valid spell found after evaluation");
            return nullptr;
    }

    void BotCastSpell(Creature* bot, Unit* target, FSBSpellRuntime* runtime, uint32& globalCooldownUntil)
    {
        if (!runtime || !runtime->def)
            return;

        uint32 now = getMSTime();

        FSBSpellDefinition const* def = runtime->def;
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(def->spellId, bot->GetMap()->GetDifficultyID());
        if (!spellInfo)
            return;

        if (!target)
            return;

        if (target->HasAura(def->spellId))
            return;

        Spell* spell = new Spell(bot, spellInfo, TRIGGERED_NONE);
        SpellCastTargets targets;
        targets.SetUnitTarget(target);

        SpellCastResult result = spell->prepare(targets);

        if (result != SPELL_CAST_OK)
            return;

        //TC_LOG_DEBUG("scripts.ai.fsb", "Bot: {} casted: {} on target: {}", bot->GetName(), FSBUtilsSpells::GetSpellName(def->spellId), target->GetName());

        // ? Per-spell cooldown
        runtime->nextReadyMs = now + def->cooldownMs;

        // Trigger GCD
        globalCooldownUntil = now + BOT_GCD_MS;


        // Bot Say after spell cast - TO-DO transform this into its own method
        if (def->type == FSBSpellType::Heal)
        {
            if (!target)
                return;

            if(target == bot)
                FSBUtilsCombat::SayCombatMessage(bot, target, 0, FSBSayType::HealSelf, def->spellId);
            else FSBUtilsCombat::SayCombatMessage(bot, target, 0, FSBSayType::HealTarget, def->spellId);
        }
        else if (def->type == FSBSpellType::Damage)
        {
            if (!target)
                return;

            FSBUtilsCombat::SayCombatMessage(bot, target, 0, FSBSayType::SpellOnTarget, def->spellId);

        }
    }

}


