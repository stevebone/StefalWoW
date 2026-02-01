#include "followship_bots_utils_stats.h"
#include "followship_bots_powers_handler.h"

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
        Powers powerType = FSBPowers::GetBotPowerType(bot);

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

    

    


    





}
