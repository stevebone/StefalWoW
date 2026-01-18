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

            TC_LOG_DEBUG("scripts.ai.fsb", "Bot: {} with role: {} has available spell: {}", bot->GetName(), botRole, FSBUtilsSpells::GetSpellName(def->spellId));
            available.push_back(&runtime);

        }

        return available;
    }

    FSBSpellRuntime* BotSelectSpell(Creature* bot, std::vector<FSBSpellRuntime*>& availableSpells, std::vector<Unit*> botGroup_, Unit*& outTarget)
    {
        outTarget = nullptr;

        if (availableSpells.empty())
        {
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BotSelectSpell - no available spell");
            return nullptr;
        }

        // =====================================
        // ============= HEALER LOGIC ==========

        std::vector<Unit*> emergencyTargets;
        if (FSBUtils::GetRole(bot) == FSB_Roles::FSB_ROLE_HEALER)
        {
            emergencyTargets = FSBUtilsCombat::GetEmergencyCandidates(botGroup_, 30.f); // e.g., 30% HP threshold

            if (!emergencyTargets.empty())
            {
                TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BotSelectSpell - Emergency Detected - filtering available spells.");

                // Filter availableSpells to only keep healing spells
                availableSpells.erase(
                    std::remove_if(availableSpells.begin(), availableSpells.end(),
                        [](FSBSpellRuntime* runtime)
                        {
                            return runtime->def->type != FSBSpellType::Heal;
                        }),
                    availableSpells.end()
                );

                // Sort by priority
                FSBUtilsCombat::SortEmergencyTargets(emergencyTargets);
            }
        }

        // Sort by priority descending
        std::sort(availableSpells.begin(), availableSpells.end(),
            [](FSBSpellRuntime* a, FSBSpellRuntime* b)
            {
                return a->def->priority > b->def->priority;
            });

        for (FSBSpellRuntime* runtime : availableSpells)
        {
            FSBSpellDefinition const* spell = runtime->def;

            // Roll chance
            if (urand(0, 100) > spell->chance)
                continue;

            // Determine target
            Unit* target = nullptr;

            // === EMERGENCY PATH ===
            if (!emergencyTargets.empty() && spell->type == FSBSpellType::Heal)
            {
                // Pick the first emergency target (or do more advanced selection)
                target = emergencyTargets.front();
            }
            else
            {

                if (spell->type == FSBSpellType::Heal)
                {
                    if (spell->isSelfCast)
                        target = bot;
                    else
                        target = FSBMgr::GetBotOwner(bot);

                    if (!target || !target->IsAlive())
                        continue;

                    // Check HP%
                    float hpPct = target->GetHealthPct();
                    if (hpPct > spell->hpThreshold)
                        continue;
                }
                else if (spell->type == FSBSpellType::Damage)
                {
                    target = bot->GetVictim();
                    if (!target || !target->IsAlive())
                    {
                        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BotSelectSpell - no target for spell");
                        continue;
                    }

                    // Optionally check distance
                    if (bot->GetDistance(target) > spell->dist)
                    {
                        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: BotSelectSpell - target too far");
                        continue;
                    }

                }

                if (!FSBUtilsSpells::IsSpellClassValid(bot, spell->spellId, target))
                    continue;
            }

            outTarget = target;

            //if(target)
              return runtime;
        }

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
        {
            if (def->type == FSBSpellType::Heal)
                target = def->isSelfCast ? bot : FSBMgr::GetBotOwner(bot)->ToUnit();
            else
                target = bot->GetVictim();
        }

        if (!target)
            return;

        if (target->HasAura(def->spellId))
            return;

        Spell* spell = new Spell(bot, spellInfo, TRIGGERED_NONE);
        SpellCastTargets targets;
        targets.SetUnitTarget(target);
        //spell->prepare(targets);

        SpellCastResult result = spell->prepare(targets);

        if (result != SPELL_CAST_OK)
            return;

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


