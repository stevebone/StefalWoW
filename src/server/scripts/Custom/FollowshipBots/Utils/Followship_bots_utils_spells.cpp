#include <random>

#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "SpellMgr.h"
#include "ThreatManager.h"
#include "Unit.h"

#include "Followship_bots_mage.h"
#include "Followship_bots_paladin.h"
#include "Followship_bots_priest.h"
#include "Followship_bots_warrior.h"
#include "Followship_bots_utils.h"
#include "Followship_bots_utils_spells.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_utils_combat.h"
#include "Followship_bots_powers_handler.h"



using FSBSpellTable = std::vector<FSBSpellDefinition>;
static std::unordered_map<FSB_Class, FSBSpellTable const*> sBotSpellTables;

namespace FSBUtilsSpells
{
    

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

    bool HasPositiveDebuff(Unit* unit)
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

            // Skip non positive auras
            if (!aurApp->IsPositive())
                continue;

            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Spell casting: found unit: {} with positive aura.", unit->GetName());

            return true;
        }

        return false;
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
            // OR when 2/3 attackers are around
            // This way we limit spells to CC instead of general/spam use
        case SPELL_PALADIN_DIVINE_STORM:
        case SPELL_WARRIOR_COLOSSUS_SMASH:
        case SPELL_WARRIOR_CLEAVE:
        case SPELL_MAGE_FLAMESTRIKE:
        case SPELL_MAGE_ARCANE_EXPLOSION:
        case SPELL_MAGE_BLIZZARD:
        case SPELL_MAGE_FROST_NOVA:
        case SPELL_MAGE_POLYMORPH:
        case SPELL_PRIEST_PSYCHIC_SCREAM:
        {
            Player* player = FSBMgr::GetBotOwner(bot);

            if (!player)
            {
                uint8 attackers = FSBUtilsCombat::CountAttackersOn(bot);
                return attackers >= 2;
            }

            uint8 attackers =
                FSBUtilsCombat::CountActiveAttackers(bot) +
                (player ? FSBUtilsCombat::CountActiveAttackers(player) : 0);

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

        case SPELL_MAGE_ICE_BLOCK:
        {
            return target == bot && bot->GetHealthPct() < 10;
        }

        case SPELL_MAGE_EVOCATION:
            return bot->GetPowerPct(POWER_MANA) < 50;

        case SPELL_MAGE_ICE_BARRIER:
            return target == bot && bot->GetHealthPct() < 75;

        case SPELL_PALADIN_REBUKE:
        case SPELL_WARRIOR_PUMMEL:
        case SPELL_MAGE_COUNTERSPELL:
            return target && target->HasUnitState(UNIT_STATE_CASTING);

        case SPELL_MAGE_SPELL_STEAL:
            return target && HasPositiveDebuff(target);

        case SPELL_MAGE_BLINK:
            return !bot->GetMap()->IsDungeon();

        case SPELL_WARRIOR_EXECUTE:
            return target && target->GetHealthPct() <= 20;

        case SPELL_WARRIOR_LAST_STAND:
            return target == bot && bot->GetHealthPct() < 30;

        case SPELL_PALADIN_LAY_ON_HANDS:
        case SPELL_PALADIN_DIVINE_SHIELD:
            return bot->GetPowerPct(POWER_MANA) < 20;

        case SPELL_WARRIOR_GIFT_NAARU:
            return FSBUtils::GetBotRaceForEntry(bot->GetEntry()) == FSB_Race::Draenei;

        default:
            return true;
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
        sBotSpellTables[FSB_Class::Mage] = &MageSpellsTable;
        sBotSpellTables[FSB_Class::Warrior] = &WarriorSpellsTable;
        sBotSpellTables[FSB_Class::Paladin] = &PaladinSpellsTable;
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

        // =========================================================
        // ===================== ROLE / MODE =======================
        // =========================================================

        bool isHealer = (FSBUtils::GetRole(bot) == FSB_Roles::FSB_ROLE_HEALER);
        //bool isHybrid = (FSBUtils::GetRole(bot) == FSB_Roles::FSB_ROLE_ASSIST || (FSBUtils::GetBotClassForEntry(bot->GetEntry()) == FSB_Class::Paladin && !FSBUtils::GetRole(bot) == FSB_Roles::FSB_ROLE_HEALER));

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
        } else  mode = SpellMode::Hybrid;

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
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: SelectSpell - spell pool empty after filtering");
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

            // Chance roll
            uint32 roll = urand(0, 100);
            if (roll > spell->chance)
            {
                TC_LOG_DEBUG("scripts.ai.fsb", "FSB: SpellSkip - {} chance failed (roll={} chance={})", spell->spellId, roll, spell->chance);
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
                    else target = bot;
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

            if (spell->isSelfCast)
                target = bot;

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

            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: SelectSpell - no valid spell found after evaluation");
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

        if (runtime->def->manaCostOverride != 0.f && !FSBPowers::SpendManaPct(bot, runtime->def->manaCostOverride))
            return; // not enough mana

        if (def->spellId == SPELL_MAGE_POLYMORPH)
            target = FSBUtilsCombat::GetRandomAttacker(bot);

        Spell* spell = new Spell(bot, spellInfo, TRIGGERED_NONE);
        SpellCastTargets targets;

        if (spellInfo->HasTargetType(TARGET_DEST_DEST))
        //if(def->spellId == SPELL_MAGE_BLIZZARD)
        {
            // Ground-targeted spell (Blizzard, Rain of Fire, etc)
            Position pos = target->GetPosition();          

            targets.SetDst(pos);

            // Optional but recommended
            //bot->SetFacingTo(bot->GetAngle(pos.GetPositionX(), pos.GetPositionY()));
        }
        else
        {
            // Normal unit-target spell (existing logic)
            targets.SetUnitTarget(target);
        }

        if (targets.GetUnitTarget() || target)
        {
            bot->SetFacingToObject(target, true);

            SpellCastResult result = spell->prepare(targets);

            if (result != SPELL_CAST_OK)
            {
                TC_LOG_DEBUG("scripts.ai.fsb", "FSB Bot: {} SPELL CAST NOT OK", bot->GetName());
                return;
            }
        }

        if(target)
            TC_LOG_DEBUG("scripts.ai.fsb", "Bot: {} casted: {} on target: {}", bot->GetName(), FSBUtilsSpells::GetSpellName(def->spellId), target->GetName());

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
            if (!target || target == bot)
                return;

            FSBUtilsCombat::SayCombatMessage(bot, target, 0, FSBSayType::SpellOnTarget, def->spellId);

        }
    }

}


