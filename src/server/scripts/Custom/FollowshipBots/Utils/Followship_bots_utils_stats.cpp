#include "Followship_bots_utils_stats.h"
#include "Followship_bots_powers_handler.h"
#include "Followship_bots_paladin.h"
#include "Followship_bots_warlock.h"
#include "Followship_bots_warrior.h"

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

        // Health
        uint32 baseHealth = stats->baseHealth + stats->healthPerLevel * (level - 1);
        creature->SetStatFlatModifier(UNIT_MOD_HEALTH, BASE_VALUE, baseHealth);
        creature->SetMaxHealth(baseHealth);
        creature->SetHealth(baseHealth);

        // Power
        Powers powerType = stats->powerType;
        uint32 basePower = stats->basePower + stats->powerPerLevel * (level - 1);

        if (botClass == FSB_Class::Warrior)
            basePower = 1000;

        creature->SetPowerType(powerType, true);
        
        creature->SetStatFlatModifier(UnitMods(UNIT_MOD_POWER_START + AsUnderlyingType(powerType)), BASE_VALUE, (float)basePower);   
        creature->SetCreateMana(basePower);
        creature->SetMaxPower(powerType, basePower * 2);

        if (botClass == FSB_Class::Warrior)
            creature->SetPower(powerType, 0, true);
        else creature->SetPower(powerType, basePower * 2, true);

        //creature->SetOverrideDisplayPowerId(466);

        
        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Bot {} stats powerType: {}", creature->GetName(), stats->powerType);
        

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Base Stats set for bot: {}, Level= {}, MaxHealth= {}, Power= {}, MaxPower= {}, TC Class= {}, FSB Class= {}",
            creature->GetName(),
            level,
            baseHealth,
            creature->GetPowerType(),
            creature->GetMaxPower(powerType),
            creature->GetClass(),
            botClass);

        // damage
        
        creature->SetBaseAttackTime(BASE_ATTACK, stats->baseAttackTime);
        creature->SetBaseAttackTime(RANGED_ATTACK, stats->baseRangedAttackTime);

        float basedamage = creature->GetBaseDamageForLevel(level) * 0.5f * stats->baseClassDamageVariance;

        if(level >= 10)
            basedamage = creature->GetBaseDamageForLevel(level) * 1.2f * stats->baseClassDamageVariance;

        if (level >= 20)
            basedamage = creature->GetBaseDamageForLevel(level) * 1.5f * stats->baseClassDamageVariance;

        float baseAttackPower = stats->baseAttackPower;
        float baseRAttackPower = stats->baseRangedAttackPower;
        float attackPowerPerLevel = stats->attackPowerPerLevel;

        float multiplierAttackPower = creature->GetPctModifierValue(UNIT_MOD_ATTACK_POWER, TOTAL_PCT);
        float multiplierRAttackPower = creature->GetPctModifierValue(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_PCT);

        float weaponBaseMinDamage = basedamage;
        float weaponBaseMaxDamage = basedamage * 1.5f;

        creature->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, weaponBaseMinDamage);
        creature->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, weaponBaseMaxDamage);

        creature->SetBaseWeaponDamage(OFF_ATTACK, MINDAMAGE, weaponBaseMinDamage);
        creature->SetBaseWeaponDamage(OFF_ATTACK, MAXDAMAGE, weaponBaseMaxDamage);

        creature->SetBaseWeaponDamage(RANGED_ATTACK, MINDAMAGE, weaponBaseMinDamage);
        creature->SetBaseWeaponDamage(RANGED_ATTACK, MAXDAMAGE, weaponBaseMaxDamage);

        creature->SetStatFlatModifier(UNIT_MOD_ATTACK_POWER, BASE_VALUE, baseAttackPower);
        creature->SetStatFlatModifier(UNIT_MOD_ATTACK_POWER_RANGED, BASE_VALUE, baseRAttackPower);

        creature->SetRangedAttackPower(int32((baseRAttackPower + (level * attackPowerPerLevel)) * multiplierRAttackPower));
        creature->SetAttackPower(int32((baseAttackPower + (level * attackPowerPerLevel)) * multiplierAttackPower));

        creature->UpdateDamagePhysical(RANGED_ATTACK);
        creature->UpdateDamagePhysical(BASE_ATTACK);
        creature->UpdateDamagePhysical(OFF_ATTACK);

        float armor = creature->GetBaseArmorForLevel(level);
        creature->SetStatFlatModifier(UNIT_MOD_ARMOR, BASE_VALUE, armor);
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

    void ApplyBotAttackPower(Unit* unit)
    {
        if (!unit)
            return;

        FSB_Class botClass = FSBUtils::GetBotClassForEntry(unit->GetEntry());

        auto const* stats = GetBotClassStats(botClass);
        if (!stats)
            return;

        int32 level = unit->GetLevel();

        float attackPowerPerLevel = stats->attackPowerPerLevel;

        float baseAttackPower = unit->GetFlatModifierValue(UNIT_MOD_ATTACK_POWER, BASE_VALUE);
        float baseRAttackPower = unit->GetFlatModifierValue(UNIT_MOD_ATTACK_POWER_RANGED, BASE_VALUE);

        float multiplierAttackPower = unit->GetPctModifierValue(UNIT_MOD_ATTACK_POWER, TOTAL_PCT);
        float multiplierRAttackPower = unit->GetPctModifierValue(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_PCT);

        float totalAttackPowerMod = unit->GetFlatModifierValue(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE);
        float totalRAttackPowerMod = unit->GetFlatModifierValue(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE);

        int32 finalAttackPower = int32(((baseAttackPower + (level * attackPowerPerLevel)) + totalAttackPowerMod) * multiplierAttackPower);
        int32 finalRAttackPower = int32(((baseRAttackPower + (level * attackPowerPerLevel)) + totalRAttackPowerMod) * multiplierRAttackPower);

        unit->SetRangedAttackPower(finalRAttackPower);
        unit->SetAttackPower(finalAttackPower);

        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Stats AttackPower Bot: {} Has baseAP: {}, levelAP: {}, modAP: {}, multiAP: {}, totalAP: {}",
        //    unit->GetName(), baseAttackPower, level * attackPowerPerLevel, totalAttackPowerMod, multiplierAttackPower, finalAttackPower);
    }

    void ApplyBotDamage(Unit* unit)
    {
        if (!unit)
            return;

        unit->UpdateDamagePhysical(RANGED_ATTACK);
        unit->UpdateDamagePhysical(BASE_ATTACK);
        unit->UpdateDamagePhysical(OFF_ATTACK);
    }

    void RecalculateMods(Unit* unit/*, const FSBUtilsStatsMods& mods*/)
    {
        //ApplyMaxHealth(unit, mods);
        //ApplyMaxMana(unit, mods);
        ApplyBotAttackPower(unit);
        ApplyBotDamage(unit);
        unit->UpdateArmor();
        unit->UpdateMaxHealth();
        unit->UpdateMaxPower(unit->GetPowerType());
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
        RecalculateMods(bot);
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
        uint32 health = uint32(basehp + (stats->healthPerLevel * level-1));

        bot->SetStatFlatModifier(UNIT_MOD_HEALTH, BASE_VALUE, (float)health);

        float totalHealth = bot->GetTotalAuraModValue(UNIT_MOD_HEALTH);

        bot->SetCreateHealth(health);
        bot->SetMaxHealth(uint32(totalHealth));
        bot->SetHealth(uint32(totalHealth));

        // Power
        Powers powerType = stats->powerType;
        uint32 basePower = stats->basePower + stats->powerPerLevel * (level - 1);

        if (botClass == FSB_Class::Warrior)
            basePower = 1000;

        bot->SetPowerType(powerType, true);

        bot->SetStatFlatModifier(UnitMods(UNIT_MOD_POWER_START + AsUnderlyingType(powerType)), BASE_VALUE, (float)basePower);
        bot->SetCreateMana(basePower);
        float totalPower = bot->GetTotalAuraModValue(UnitMods(UNIT_MOD_POWER_START + AsUnderlyingType(powerType)));

        if (botClass == FSB_Class::Warrior)
        {
            totalPower = 1000;
            bot->SetMaxPower(powerType, totalPower);
            bot->SetPower(powerType, 0, true);
        }

        else
        {
            bot->SetMaxPower(powerType, totalPower + basePower);
            bot->SetPower(powerType, totalPower + basePower, true);
        }
        //bot->ResetPlayerDamageReq();

        // Att power
        ApplyBotAttackPower(bot);
        ApplyBotDamage(bot);
        
    }

    // This is applied in DamageTaken() so with every damage
    float ApplyBotDamageTakenReduction(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return 1.0f;

        float multiplier = 1.0f;

        if (FSBWarrior::BotHasDefensiveStance(bot))
            multiplier *= 0.85f;

        if (FSBWarrior::BotHasShieldWall(bot))
            multiplier *= 0.60f;

        if (FSBPaladin::BotHasDevotionAura(bot))
            multiplier *= 0.97f;

        if (FSBPaladin::BotHasGuardianOfAncientKings(bot))
            multiplier *= 0.50f;

        if (FSBPaladin::BotHasDivineProtection(bot))
            multiplier *= 0.80f;

        if (FSBPaladin::BotHasBlessingOfSacrifice(bot))
            multiplier *= 0.70f;

        if (FSBWarlock::BotHasUnendingResolve(bot))
            multiplier *= 0.75f;

        return multiplier;
    }

    // This is applied in DamageDealt() so with every damage bot does
    // This method can also increase damage done > 1.f
    float ApplyBotDamageDoneReduction(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return 1.0f;

        float multiplier = 1.0f;

        if (FSBWarrior::BotHasDefensiveStance(bot))
            multiplier *= 0.90f;

        if (FSBWarrior::BotHasBattleStance(bot))
            multiplier *= 1.03f;

        if (FSBWarrior::BotHasBerserkerStance(bot))
            multiplier *= 1.15f;

        if (FSBPaladin::BotHasAvengingWrath(bot))
            multiplier *= 1.2f;

        return multiplier;
    }


    





}

