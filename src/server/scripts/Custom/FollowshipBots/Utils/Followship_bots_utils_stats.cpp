#include "followship_bots_utils_stats.h"
#include "followship_bots_utils.h"

constexpr float RAGE_FROM_DAMAGE_COEFF = 1.15f;

static constexpr FSB_ClassStats BotClassStatsTable[] =
{
    // WARRIOR
    {
        .classId = FSB_Class::Warrior,
        .powerType = POWER_RAGE,
        .baseHealth = 120,    // base HP
        .basePower = 0,      // base power
        .healthPerLevel = 60,     // HP per level
        .powerPerLevel = 0,      // Power per level
        .baseHpRegenOOC = 5,      // HP regen %
        .basePowerRegenOOC = -2,       // Power regen %
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 0,
        .baseAttackPower = 100,
        .baseRangedAttackPower = 50
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
        .basePowerRegenIC = 2,
        .baseAttackPower = 10,
        .baseRangedAttackPower = 0
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
        .basePowerRegenIC = 2,
        .baseAttackPower = 10,
        .baseRangedAttackPower = 0
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
        .basePowerRegenIC = 2,
        .baseAttackPower = 80,
        .baseRangedAttackPower = 50
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
        creature->SetClass(FSBToTCClass(botClass));

        Powers powerType = stats->powerType;
        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Bot {} stats powerType: {}", creature->GetName(), stats->powerType);
        uint32 maxHealth =
            stats->baseHealth +
            stats->healthPerLevel * (level - 1);

        uint32 maxPower =
            stats->basePower +
            stats->powerPerLevel * (level - 1);

        if (botClass == FSB_Class::Warrior)
            maxPower = 1000;

        //creature->SetOverrideDisplayPowerId(466);

        creature->SetPowerType(powerType, true);
        creature->SetMaxHealth(maxHealth);
        creature->SetHealth(maxHealth);
        creature->SetMaxPower(powerType, maxPower);

        if (stats->powerType == POWER_RAGE)
            creature->SetPower(powerType, 0, true);
        else creature->SetPower(powerType, maxPower);

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Base Stats set for bot: {}, Level= {}, MaxHealth= {}, Power= {}, MaxPower= {}, TC Class= {}, FSB Class= {}",
            creature->GetName(),
            level,
            maxHealth,
            creature->GetPowerType(),
            creature->GetMaxPower(powerType),
            creature->GetClass(),
            botClass);
    }

    Classes FSBToTCClass(FSB_Class botClass)
    {
        switch (botClass)
        {
        case FSB_Class::Warrior:            return CLASS_WARRIOR;
        case FSB_Class::Priest:             return CLASS_PRIEST;
        case FSB_Class::Mage:               return CLASS_MAGE;
        case FSB_Class::Rogue:              return CLASS_ROGUE;
        case FSB_Class::Druid:              return CLASS_DRUID;
        case FSB_Class::Paladin:            return CLASS_PALADIN;
        case FSB_Class::Hunter:             return CLASS_HUNTER;
        case FSB_Class::Warlock:            return CLASS_WARLOCK;
        case FSB_Class::None:               return CLASS_NONE;
        default:
            break;
        }

        return CLASS_NONE;
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

            //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Custom Regen tick for bot {} (HP={}) in combat: {}", unit->GetName(), amount, inCombat);
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

            //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Custom Regen tick for bot {} (MP={}) in combat: {}", unit->GetName(), amount, inCombat);
        }

        if (unit->GetPowerType() == POWER_RAGE && unit->GetPower(POWER_RAGE) > 0)
        {
            int32 basePowerRegen = inCombat
                ? classStats->basePowerRegenIC
                : classStats->basePowerRegenOOC;

            int32 maxPower = int32(unit->GetMaxPower(POWER_RAGE));
            int32 amount = (basePowerRegen * maxPower) / 100;

            if (amount != 0)
                unit->ModifyPower(POWER_RAGE, amount);

            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Custom Regen tick for bot {} (RP={}) in combat: {}", unit->GetName(), amount, inCombat);
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
        UpdateBotLevelDependantStats(bot);

        // Reset health & power
        //bot->SetHealth(bot->GetMaxHealth());
        //if(FSBUtils::GetBotClassForEntry(bot->GetEntry()) != FSB_Class::Warrior)
        //    bot->SetPower(FSBUtilsStats::GetBotPowerType(bot), bot->GetMaxPower(FSBUtilsStats::GetBotPowerType(bot)));

        // Recalculate any additional stats/modifiers
        RecalculateMods(bot, mods);
    }

    void UpdateBotLevelDependantStats(Creature* bot)
    {
        FSB_Class botClass = FSBUtils::GetBotClassForEntry(bot->GetEntry());
        auto const* stats = GetBotClassStats(botClass);
        if (!stats)
            return;

        uint8 level = bot->GetLevel();

        // health

        uint32 basehp = stats->baseHealth;
        uint32 health = uint32(basehp + (stats->healthPerLevel * level));

        bot->SetCreateHealth(health);
        bot->SetMaxHealth(health);
        bot->SetHealth(health);
        bot->ResetPlayerDamageReq();

        bot->SetStatFlatModifier(UNIT_MOD_HEALTH, BASE_VALUE, (float)health);

        // Powers
        Powers powerType = GetBotPowerType(bot);

        uint32 basePower = stats->basePower;
        uint32 power = uint32(basePower + (stats->powerPerLevel * level));

        if (botClass == FSB_Class::Warrior)
            power = 1000;

        bot->SetCreateMana(power);
        bot->SetStatPctModifier(UnitMods(UNIT_MOD_POWER_START + AsUnderlyingType(powerType)), BASE_PCT, bot->GetCreatureDifficulty()->ManaModifier);
        bot->SetPowerType(powerType, true, true);
        bot->SetMaxPower(powerType, power);
        bot->SetPower(powerType, power);

        // damage
        float basedamage = bot->GetBaseDamageForLevel(level);

        float weaponBaseMinDamage = basedamage;
        float weaponBaseMaxDamage = basedamage * 1.5f;

        bot->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, weaponBaseMinDamage);
        bot->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, weaponBaseMaxDamage);

        bot->SetBaseWeaponDamage(OFF_ATTACK, MINDAMAGE, weaponBaseMinDamage);
        bot->SetBaseWeaponDamage(OFF_ATTACK, MAXDAMAGE, weaponBaseMaxDamage);

        bot->SetBaseWeaponDamage(RANGED_ATTACK, MINDAMAGE, weaponBaseMinDamage);
        bot->SetBaseWeaponDamage(RANGED_ATTACK, MAXDAMAGE, weaponBaseMaxDamage);

        bot->SetStatFlatModifier(UNIT_MOD_ATTACK_POWER, BASE_VALUE, stats->baseAttackPower);
        bot->SetStatFlatModifier(UNIT_MOD_ATTACK_POWER_RANGED, BASE_VALUE, stats->baseRangedAttackPower);

        float armor = bot->GetBaseArmorForLevel(level);
        bot->SetStatFlatModifier(UNIT_MOD_ARMOR, BASE_VALUE, armor);
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

    Powers GetBotPowerType(Creature* bot)
    {
        if (!bot)
            return POWER_MANA;

        FSB_Class cls = FSBUtils::GetBotClassForEntry(bot->GetEntry());

        for (auto const& stats : BotClassStatsTable)
        {
            if (stats.classId == cls)
                return stats.powerType;
        }

        return POWER_MANA; // safe fallback
    }


    bool IsRageUser(Creature* bot)
    {
        return FSBUtils::GetBotClassForEntry(bot->GetEntry()) == FSB_Class::Warrior;
    }

    void GenerateRageFromDamageTaken(Creature* bot, uint32 damage)
    {
        if (!damage || !bot->IsAlive())
            return;

        if (!IsRageUser(bot))
            return;

        // Rage from damage taken
        uint32 rageGain = uint32(damage * RAGE_FROM_DAMAGE_COEFF);

        if (rageGain == 0)
            rageGain = 1; // always give *something*

        uint32 currentRage = bot->GetPower(POWER_RAGE);
        uint32 maxRage = bot->GetMaxPower(POWER_RAGE);

        uint32 newRage = std::min(currentRage + rageGain, maxRage);

        bot->ModifyPower(POWER_RAGE, rageGain, false);

        TC_LOG_DEBUG("scripts.ai.fsb",
            "FSB: {} gained {} rage from taking {} damage (now {})",
            bot->GetName(), rageGain, damage, newRage);
    }


}
