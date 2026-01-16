#include "followship_bots_utils_stats.h"

static constexpr FSB_ClassStats BotClassStatsTable[] =
{
    // WARRIOR
    {
        FSB_Class::Warrior,
        POWER_RAGE,
        120,    // base HP
        0,      // base power
        30,     // HP per level
        0,      // Power per level
        5,      // HP regen %
        0       // Power regen %
    },

    // PRIEST
    {
        FSB_Class::Priest,
        POWER_MANA,
        90,             // base HP
        160,            // base Power
        18,             // HP per level
        30,             // Power per level
        3,              // HP regen %
        12              // Power regen %
    },

    // MAGE
    {
        FSB_Class::Mage,
        POWER_MANA,
        80,
        200,
        15,
        35,
        2,
        14
    },

    // ROGUE
    {
        FSB_Class::Rogue,
        POWER_ENERGY,
        100,
        100,
        22,
        10,
        4,
        10
    },
};

namespace FSBUtilsStats
{
    // Get (base) stats for bot depending on class
    FSB_ClassStats const* GetBotClassStats(FSB_Class botClass)
    {
        for (auto const& entry : BotClassStatsTable)
        {
            if (entry.classId == botClass)
                return &entry;
        }

        return nullptr;
    }

    void ApplyBotBaseClassStats(Creature* creature, FSB_Class botClass)
    {
        auto const* stats = GetBotClassStats(botClass);
        if (!stats)
            return;

        uint8 level = creature->GetLevel();

        uint32 maxHealth =
            stats->baseHealth +
            stats->healthPerLevel * (level - 1);

        uint32 maxPower =
            stats->basePower +
            stats->powerPerLevel * (level - 1);

        creature->SetPowerType(stats->powerType);
        creature->SetMaxHealth(maxHealth);
        creature->SetHealth(maxHealth);

        // TO-DO set conditions for rage and energy
        creature->SetMaxPower(stats->powerType, maxPower);
        creature->SetPower(stats->powerType, maxPower);

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Base Stats set for bot: {}, Level= {}, MaxHealth= {}, Power= {}, MaxPower= {}",
            creature->GetName(),
            level,
            maxHealth,
            creature->GetPowerType(),
            maxPower);
    }

    void ApplyBotBaseRegen(Creature* me, FSBUtilsStatsMods& mods, FSB_Class botClass)
    {
        if (!me->IsAlive())
            return;

        auto const* stats = GetBotClassStats(botClass);
        if (!stats)
            return;

        bool inCombat = me->IsInCombat();

        if (!inCombat)
        {
            mods.flatHealthPerTick += stats->baseHpRegenOOC;
            mods.flatManaPerTick += stats->basePowerRegenOOC;
        }
        else
        {
            mods.flatHealthPerTick += stats->baseHpRegenIC;
            mods.flatManaPerTick += stats->basePowerRegenIC;
        }
    }

    void ApplyBotRegen(Unit* unit, FSB_Class botClass, const FSBUtilsStatsMods& mods, bool doHealth, bool doMana)
    {
        if (!unit || !unit->IsAlive())
            return;

        bool inCombat = unit->IsInCombat();

        // Fetch class base stats
        FSB_ClassStats const* classStats = GetBotClassStats(botClass);
        if (!classStats)
            return;

        // ---------- HEALTH ----------
        if (doHealth)
        {
            int32 baseHpRegen = inCombat
                ? classStats->baseHpRegenIC
                : classStats->baseHpRegenOOC;

            int32 amount = baseHpRegen + mods.flatHealthPerTick;

            if (mods.pctMaxHealthBonus > 0.0f)
            {
                uint32 maxHp = unit->GetMaxHealth();
                amount += int32(maxHp * mods.pctMaxHealthBonus);
            }

            if (amount > 0)
                unit->ModifyHealth(amount);
        }

        // ---------- POWER ----------
        if (doMana && unit->GetPowerType() == POWER_MANA)
        {
            int32 basePowerRegen = inCombat
                ? classStats->basePowerRegenIC
                : classStats->basePowerRegenOOC;

            int32 amount = basePowerRegen + mods.flatManaPerTick;

            if (mods.pctMaxManaBonus > 0.0f)
            {
                uint32 maxPower = unit->GetMaxPower(POWER_MANA);
                amount += int32(maxPower * mods.pctMaxManaBonus);
            }

            if (amount > 0)
                unit->ModifyPower(POWER_MANA, amount);
        }
    }

    void ProcessBotCustomRegenTick(Creature* creature, FSB_Class botClass, const FSBUtilsStatsMods& _baseStatsMods, const FSBUtilsStatsMods& _statsMods)
    {
        if (!creature || !creature->IsBot() || !creature->IsAlive())
            return;

        // Only apply regen if HP or power is not full
        if (creature->GetHealthPct() < 100 || creature->GetPowerPct(creature->GetPowerType()) < 100)
        {
            // Merge base + aura mods
            FSBUtilsStatsMods finalMods = _baseStatsMods;
            finalMods += _statsMods;

            // Apply regen
            ApplyBotRegen(creature, botClass, finalMods, true, true);

            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Custom Regen tick for bot {} (HP={} MP={})",
                creature->GetName(), finalMods.flatHealthPerTick, finalMods.flatManaPerTick);
        }
    }
    

    void ApplyMaxHealth(Unit* unit, const FSBUtilsStatsMods& mods)
    {
        if (!unit)
            return;

        uint32 baseMaxHealth = unit->GetCreateHealth();
        int32 bonusFlat = mods.flatMaxHealth;
        float bonusPct = mods.pctMaxHealthBonus;

        uint32 newMaxHealth = baseMaxHealth + bonusFlat;
        if (bonusPct > 0.0f)
            newMaxHealth += uint32(float(baseMaxHealth) * bonusPct);

        if (unit->GetMaxHealth() != newMaxHealth)
        {
            unit->SetMaxHealth(newMaxHealth);

            if (unit->GetHealth() > newMaxHealth)
                unit->SetHealth(newMaxHealth);
        }
    }

    void RecalculateMods(Unit* unit, const FSBUtilsStatsMods& mods)
    {
        ApplyMaxHealth(unit, mods);
        // Future:
        // ApplyArmor(...)
        // ApplySpellPower(...)
    }

    void UpdateBotLevelToPlayer(Creature* bot, Player* player, const FSBUtilsStatsMods& mods)
    {
        if (!bot || !player)
            return;

        uint8 pLevel = player->GetLevel();
        if (bot->GetLevel() == pLevel)
            return; // nothing to do

        bot->SetLevel(pLevel);

        // Update stats for new level
        bot->UpdateLevelDependantStats();

        // Reset health & power
        bot->SetHealth(bot->GetMaxHealth());
        bot->SetPower(bot->GetPowerType(), bot->GetMaxPower(bot->GetPowerType()));

        // Recalculate any additional stats/modifiers
        RecalculateMods(bot, mods);
    }
}
