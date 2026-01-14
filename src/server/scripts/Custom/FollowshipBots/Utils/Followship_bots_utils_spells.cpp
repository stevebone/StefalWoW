#include <random>

#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "SpellMgr.h"
#include "ThreatManager.h"
#include "Unit.h"

#include "followship_bots_utils.h"
#include "followship_bots_utils_spells.h"

// Mana Potion Spells
struct ManaPotionSpell
{
    uint8 minLevel;
    uint8 maxLevel;
    uint32 spellId;
};

static constexpr ManaPotionSpell ManaPotionTable[] =
{
    {  1,  15,  437 },   // Minor Mana Potion 120
    { 16,  29,  438 },   // Lesser Mana Potion 159
    { 30,  40,  17530 }, // Superior Mana Potion 494
    { 41,  50,  17531 }, // Major Mana Potion 512
    { 51,  60,  28499 }, // Super Mana Potion 682
    { 61,  80,  43186 } // Runic Mana Potion 956
};



// Gets The Spell name based on a provided spell id
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

            return true;
        }

        return false;
    }

    FSBSpellCandidate* SelectSpell(Unit* caster, Unit* target, std::vector<FSBSpellCandidate>& spells,
        FSBSpellType type, bool preferSelfCast)
    {
        if (!caster || !target)
            return nullptr;

        uint32 now = getMSTime();
        FSB_Roles botRole = FSBUtils::GetRole(caster->ToCreature());
        uint32 botRoleMask = RoleToMask(botRole);

        // ----- First pass: self-cast preferred -----
        if (preferSelfCast)
        {
            for (auto& spell : spells)
            {
                if (spell.type != type)
                    continue;

                // Role check
                if (spell.allowedRoles != FSB_ROLEMASK_ANY &&
                    (spell.allowedRoles & botRoleMask) == 0)
                    continue;

                if (!spell.isSelfCast)
                    continue;

                if (!FSBUtilsSpells::IsSpellReady(spell, now))
                    continue;

                if (target->HasAura(spell.spellId))
                    continue;

                return &spell;
            }
        }

        // ----- Second pass: highest priority -----
        FSBSpellCandidate* best = nullptr;
        float highestPriority = -1.0f;

        for (auto& spell : spells)
        {
            if (spell.type != type)
                continue;

            if (spell.allowedRoles != FSB_ROLEMASK_ANY &&
                (spell.allowedRoles & botRoleMask) == 0)
                continue;

            if (!FSBUtilsSpells::IsSpellReady(spell, now))
                continue;

            if (spell.chance < FSBUtilsSpells::Frand(0.f, 100.f))
                continue;

            if (type == FSBSpellType::Heal)
            {
                Unit* actualTarget = spell.isSelfCast ? caster : target;
                float hpPct = actualTarget->GetHealthPct();
                if (hpPct > spell.hpThreshold)
                    continue;
            }

            if (target->HasAura(spell.spellId))
                continue;

            if (spell.priority > highestPriority)
            {
                highestPriority = spell.priority;
                best = &spell;
            }
        }

        return best;
    }

    bool TryCast(Unit* caster, Unit* target, FSBSpellCandidate* spell, uint32& globalCooldownUntil)
    {
        if (!caster || !target || !spell)
            return false;

        uint32 now = getMSTime();

        // Check global cooldown
        if (!FSBUtilsSpells::CanCastNow(caster, now, globalCooldownUntil))
            return false;

        // Check spell-specific cooldown
        if (!FSBUtilsSpells::IsSpellReady(*spell, now))
            return false;

        // Prevent casting if aura already exists
        if (!spell->isSelfCast && target->HasAura(spell->spellId))
            return false;

        if (spell->isSelfCast && caster->HasAura(spell->spellId))
            return false;

        // Self-cast handling
        Unit* actualTarget = spell->isSelfCast ? caster : target;

        // Target validity
        if (!actualTarget->IsAlive())
            return false;

        // LOS check
        if (!spell->isSelfCast && !caster->IsWithinLOSInMap(actualTarget))
            return false;

        // Range check (should have been handled with EnsureInRange)
        float dist = spell->dist;
        if (!spell->isSelfCast && caster->GetDistance(actualTarget) > dist)
            return false;

        // Everything passed ? cast the spell
        caster->CastSpell(actualTarget, spell->spellId, true);

        // Trigger spell cooldown
        FSBUtilsSpells::TriggerCooldown(*spell, now);

        // Trigger GCD
        globalCooldownUntil = now + BOT_GCD_MS;

        return true;
    }

    float Frand(float min, float max)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen);
    }

    bool IsSpellReady(const FSBSpellCandidate& spell, uint32 now)
    {
        return now >= spell.nextReadyMs;
    }

    void TriggerCooldown(FSBSpellCandidate& spell, uint32 now)
    {
        spell.nextReadyMs = now + spell.cooldownMs;
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
