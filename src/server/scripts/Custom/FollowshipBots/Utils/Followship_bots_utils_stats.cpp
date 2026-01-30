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
        0,       // Power regen %
        0,
        0
    },

    // PRIEST
    {
        .classId = FSB_Class::Priest,
        .powerType = POWER_MANA,
        .baseHealth = 90,             // base HP
        .basePower = 160,            // base Power
        .healthPerLevel = 48,             // HP per level
        .powerPerLevel = 30,             // Power per level
        .baseHpRegenOOC = 3,              // HP regen %
        .basePowerRegenOOC = 4,              // Power regen %
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 2
    },

    // MAGE
    {
        .classId = FSB_Class::Mage,
        .powerType = POWER_MANA,
        .baseHealth = 80,            
        .basePower = 200,            
        .healthPerLevel = 45,        
        .powerPerLevel = 35,         
        .baseHpRegenOOC = 2,         
        .basePowerRegenOOC = 6,      
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 2
    },

    // ROGUE
    {
        .classId = FSB_Class::Rogue,
        .powerType = POWER_ENERGY,
        .baseHealth = 100,
        .basePower = 100,
        .healthPerLevel = 22,
        .powerPerLevel = 10,
        .baseHpRegenOOC = 4,
        .basePowerRegenOOC = 10,
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 2
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

    void ApplyBotRegen(Unit* unit, FSB_Class botClass, const FSBUtilsStatsMods& mods, bool doHealth, bool doMana)
    {
        if (!unit || !unit->IsAlive())
            return;

        bool inCombat = unit->IsInCombat() || unit->HasUnitFlag(UNIT_FLAG_IN_COMBAT);

        // Fetch class base stats
        FSB_ClassStats const* classStats = GetBotClassStats(botClass);
        if (!classStats)
            return;

        // ---------- HEALTH ----------
        if (doHealth && unit->GetHealthPct() < 100)
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

            if (mods.pctHealthPerTick > 0.0f)
            {
                uint32 maxHp = unit->GetMaxHealth();
                amount += int32(maxHp * mods.pctHealthPerTick);
            }

            if (amount > 0)
                unit->ModifyHealth(amount);

            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Custom Regen tick for bot {} (HP={}) in combat: {}", unit->GetName(), amount, inCombat);
        }

        // ---------- POWER ----------
        if (doMana && unit->GetPowerType() == POWER_MANA && unit->GetPowerPct(POWER_MANA) < 100)
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

            if (mods.pctManaPerTick > 0.0f)
            {
                uint32 maxPower = unit->GetMaxPower(POWER_MANA);
                amount += int32(maxPower * mods.pctManaPerTick);
            }

            if (amount > 0)
                unit->ModifyPower(POWER_MANA, amount);

            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Custom Regen tick for bot {} (MP={}) in combat: {}", unit->GetName(), amount, inCombat);
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

        }
    }
    

    void ApplyMaxHealth(Unit* unit, const FSBUtilsStatsMods& mods)
    {
        if (!unit)
            return;

        //uint32 baseMaxHealth = unit->GetCreateHealth();
        uint32 baseMaxHealth = unit->GetMaxHealth();
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

    void ApplyMaxMana(Unit* unit, const FSBUtilsStatsMods& mods)
    {
        if (!unit)
            return;

        //uint32 baseMaxHealth = unit->GetCreateHealth();
        uint32 baseMaxPower = unit->GetMaxPower(POWER_MANA);
        int32 bonusFlat = mods.flatMaxMana;
        float bonusPct = mods.pctMaxManaBonus;

        int32 newMaxPower = baseMaxPower + bonusFlat;
        if (bonusPct > 0.0f)
            newMaxPower += uint32(float(baseMaxPower) * bonusPct);

        if (unit->GetMaxPower(POWER_MANA) != newMaxPower)
        {
            unit->SetMaxPower(POWER_MANA, newMaxPower);

            //if (unit->GetHealth() > newMaxHealth)
            //    unit->SetHealth(newMaxHealth);
        }
    }

    void RecalculateMods(Unit* unit, const FSBUtilsStatsMods& mods)
    {
        ApplyMaxHealth(unit, mods);
        ApplyMaxMana(unit, mods);
        // ApplyArmor(...)
        // ApplySpellPower(...)
    }

    void UpdateBotLevelToPlayer(Creature* bot, const FSBUtilsStatsMods& mods)
    {
        if (!bot || !bot->GetOwner())
            return;

        Player* player = bot->GetOwner()->ToPlayer();
        if (!player)
            return; // <-- Prevent crash if owner is gone

        uint8 pLevel = player->GetLevel();
        if (bot->GetLevel() == pLevel)
        {
            //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: UpdateBotLevel: Nothing to do");
            return; // nothing to do
        }


        bot->SetLevel(pLevel);

        // Update stats for new level
        bot->UpdateLevelDependantStats();

        // Reset health & power
        bot->SetHealth(bot->GetMaxHealth());
        bot->SetPower(bot->GetPowerType(), bot->GetMaxPower(bot->GetPowerType()));

        // Recalculate any additional stats/modifiers
        RecalculateMods(bot, mods);
    }

    bool SpendManaPct(Creature* bot, float pct)
    {
        if (!bot || pct <= 0.0f)
            return false;

        uint32 maxMana = bot->GetMaxPower(POWER_MANA);
        if (!maxMana)
            return false;

        int32 cost = uint32(maxMana * pct);

        if (bot->GetPower(POWER_MANA) < cost)
            return false;

        bot->ModifyPower(POWER_MANA, -int32(cost));

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Combat spellcasting: Reduced power mana by {} for bot: {}", cost, bot->GetName());

        return true;
    }

    bool SpendManaFlat(Creature* bot, int32 cost)
    {
        if (!bot)
            return false;

        if (bot->GetPower(POWER_MANA) < cost)
            return false;

        bot->ModifyPower(POWER_MANA, -int32(cost));
        return true;
    }

}
