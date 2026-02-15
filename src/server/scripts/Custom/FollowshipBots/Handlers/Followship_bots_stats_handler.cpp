#include "Followship_bots_utils.h"

#include "Followship_bots_powers_handler.h"
#include "Followship_bots_stats_handler.h"

#include "Followship_bots_druid.h"
#include "Followship_bots_paladin.h"
#include "Followship_bots_warlock.h"
#include "Followship_bots_warrior.h"

namespace FSBStats
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

    void ApplyBotBaseClassStats(Creature* bot, FSB_Class botClass)
    {
        auto const* stats = GetBotClassStats(botClass);
        if (!stats)
            return;

        
        bot->SetClass(FSBStatsUtils::FSBToTCClass(botClass));

        // Health
        ApplyBotHealth(bot, botClass, true);        

        // Power
        ApplyBotBasePower(bot, botClass);

        // Attack Power
        ApplyBotAttackPower(bot, botClass);

        // Damage
        ApplyBotDamage(bot, botClass);

        // Armor
        ApplyBotArmor(bot);



        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Base Stats set for bot: {}, Level= {}, TC Class= {}, FSB Class= {}", bot->GetName(), bot->GetLevel(), bot->GetClass(), botClass);
    }

    void ApplyBotBasePower(Creature* bot, FSB_Class botClass)
    {
        if (!bot)
            return;

        auto const* stats = GetBotClassStats(botClass);
        if (!stats)
            return;

        uint8 level = bot->GetLevel();

        Powers basePowerType = stats->powerType;
        uint32 basePower = stats->basePower + stats->powerPerLevel * (level - 1);

        if (FSBPowers::IsRageUser(bot))
        {
            basePowerType = POWER_RAGE;
            basePower = 1000;
        }

        if (FSBPowers::IsEnergyUser(bot))
        {
            basePowerType = POWER_ENERGY;
            basePower = 100;
        }

        bot->SetPowerType(basePowerType, true);

        bot->SetStatFlatModifier(UnitMods(UNIT_MOD_POWER_START + AsUnderlyingType(basePowerType)), BASE_VALUE, (float)basePower);
        bot->SetCreateMana(basePower);
        float totalPower = bot->GetTotalAuraModValue(UnitMods(UNIT_MOD_POWER_START + AsUnderlyingType(basePowerType)));
        bot->SetMaxPower(basePowerType, totalPower);

        if (FSBPowers::IsRageUser(bot))
            bot->SetPower(basePowerType, 0, true);
        else bot->SetPower(basePowerType, totalPower, true);

        //creature->SetOverrideDisplayPowerId(466);

        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Bot {} statsHandler BASE powerType: {}, base: {}, total: {}", bot->GetName(), basePowerType, basePower, totalPower);
    }

    void ApplyBotHealth(Creature* bot, FSB_Class botClass, bool updateHealth)
    {
        if (!bot)
            return;

        auto const* stats = GetBotClassStats(botClass);
        if (!stats)
            return;

        uint8 level = bot->GetLevel();

        uint32 baseHealth = stats->baseHealth + stats->healthPerLevel * (level - 1);
        bot->SetStatFlatModifier(UNIT_MOD_HEALTH, BASE_VALUE, baseHealth);
        float totalHealth = bot->GetTotalAuraModValue(UNIT_MOD_HEALTH);

        bot->SetCreateHealth(baseHealth);
        bot->SetMaxHealth(uint32(totalHealth));

        if(updateHealth)
            bot->SetHealth(uint32(totalHealth));

        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Bot {} statsHandler health base: {}, total: {}", bot->GetName(), baseHealth, totalHealth);
    }

    void ApplyBotPower(Creature* bot, FSB_Class botClass, bool updatePower)
    {
        if (!bot)
            return;

        auto const* stats = GetBotClassStats(botClass);
        if (!stats)
            return;

        uint8 level = bot->GetLevel();

        Powers basePowerType = stats->powerType;
        uint32 basePower = stats->basePower + stats->powerPerLevel * (level - 1);

        if (FSBPowers::IsRageUser(bot))
        {
            basePowerType = POWER_RAGE;
            basePower = 1000;
        }

        if (FSBPowers::IsEnergyUser(bot))
        {
            basePowerType = POWER_ENERGY;
            basePower = 100;
        }

        bot->SetPowerType(basePowerType, true);

        bot->SetStatFlatModifier(UnitMods(UNIT_MOD_POWER_START + AsUnderlyingType(basePowerType)), BASE_VALUE, (float)basePower);
        bot->SetCreateMana(basePower);
        float totalPower = bot->GetTotalAuraModValue(UnitMods(UNIT_MOD_POWER_START + AsUnderlyingType(basePowerType)));
        bot->SetMaxPower(basePowerType, totalPower);
            

        if (updatePower)
        {
            if (!FSBPowers::IsRageUser(bot))
                bot->SetPower(basePowerType, totalPower, true);
        }
        else bot->SetPower(basePowerType, bot->GetPower(basePowerType), true);
        

        //creature->SetOverrideDisplayPowerId(466);

        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Bot {} statsHandler powerType: {}, base: {}, total: {}", bot->GetName(), basePowerType, basePower, totalPower);
    }

    void ApplyBotAttackPower(Creature* bot, FSB_Class botClass)
    {
        if (!bot)
            return;

        auto const* stats = GetBotClassStats(botClass);
        if (!stats)
            return;

        int32 level = bot->GetLevel();

        float attackPowerPerLevel = stats->attackPowerPerLevel;

        float baseAttackPower = bot->GetFlatModifierValue(UNIT_MOD_ATTACK_POWER, BASE_VALUE);
        float baseRAttackPower = bot->GetFlatModifierValue(UNIT_MOD_ATTACK_POWER_RANGED, BASE_VALUE);

        float multiplierAttackPower = bot->GetPctModifierValue(UNIT_MOD_ATTACK_POWER, TOTAL_PCT);
        float multiplierRAttackPower = bot->GetPctModifierValue(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_PCT);

        float totalAttackPowerMod = bot->GetFlatModifierValue(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE);
        float totalRAttackPowerMod = bot->GetFlatModifierValue(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE);

        int32 finalAttackPower = int32(((baseAttackPower + (level * attackPowerPerLevel)) + totalAttackPowerMod) * multiplierAttackPower);
        int32 finalRAttackPower = int32(((baseRAttackPower + (level * attackPowerPerLevel)) + totalRAttackPowerMod) * multiplierRAttackPower);

        bot->SetRangedAttackPower(finalRAttackPower);
        bot->SetAttackPower(finalAttackPower);

        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Stats AttackPower Bot: {} Has baseAP: {}, levelAP: {}, modAP: {}, multiAP: {}, totalAP: {}",
        //    unit->GetName(), baseAttackPower, level * attackPowerPerLevel, totalAttackPowerMod, multiplierAttackPower, finalAttackPower);
    }

    void ApplyBotDamage(Creature* bot, FSB_Class botClass)
    {
        if (!bot)
            return;

        auto const* stats = GetBotClassStats(botClass);
        if (!stats)
            return;

        uint8 level = bot->GetLevel();

        bot->SetBaseAttackTime(BASE_ATTACK, stats->baseAttackTime);
        bot->SetBaseAttackTime(RANGED_ATTACK, stats->baseRangedAttackTime);

        float basedamage = bot->GetBaseDamageForLevel(level) * 0.5f * stats->baseClassDamageVariance;

        if (level >= 10)
            basedamage = bot->GetBaseDamageForLevel(level) * 1.2f * stats->baseClassDamageVariance;

        if (level >= 20)
            basedamage = bot->GetBaseDamageForLevel(level) * 1.5f * stats->baseClassDamageVariance;

        float weaponBaseMinDamage = basedamage;
        float weaponBaseMaxDamage = basedamage * 1.5f;

        bot->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, weaponBaseMinDamage);
        bot->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, weaponBaseMaxDamage);

        bot->SetBaseWeaponDamage(OFF_ATTACK, MINDAMAGE, weaponBaseMinDamage);
        bot->SetBaseWeaponDamage(OFF_ATTACK, MAXDAMAGE, weaponBaseMaxDamage);

        bot->SetBaseWeaponDamage(RANGED_ATTACK, MINDAMAGE, weaponBaseMinDamage);
        bot->SetBaseWeaponDamage(RANGED_ATTACK, MAXDAMAGE, weaponBaseMaxDamage);

        bot->UpdateDamagePhysical(RANGED_ATTACK);
        bot->UpdateDamagePhysical(BASE_ATTACK);
        bot->UpdateDamagePhysical(OFF_ATTACK);
    }

    void ApplyBotArmor(Creature* bot)
    {
        if (!bot)
            return;

        uint8 level = bot->GetLevel();

        float baseArmor = bot->GetBaseArmorForLevel(level);
        bot->SetStatFlatModifier(UNIT_MOD_ARMOR, BASE_VALUE, baseArmor);
        float totalArmor = bot->GetTotalAuraModValue(UNIT_MOD_ARMOR);

        bot->SetArmor(baseArmor, totalArmor - baseArmor);

        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Bot {} statsHandler armor base: {}, total: {}", bot->GetName(), baseArmor, totalArmor);
    }

    void UpdateBotLevelToPlayer(Creature* bot)
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

        RecalculateStats(bot, true, true);
    }

    void RecalculateStats(Creature* bot, bool updateHealth, bool updatePower)
    {
        if (!bot)
            return;

        FSB_Class cls = FSBUtils::GetBotClassForEntry(bot->GetEntry());

        ApplyBotHealth(bot, cls, updateHealth);
        ApplyBotPower(bot, cls, updatePower);
        ApplyBotAttackPower(bot, cls);
        ApplyBotDamage(bot, cls);
        ApplyBotArmor(bot);
    }

    int32 BotGetSpellPower(const Creature* bot)
    {
        if (!bot)
            return 0;

        FSB_Class botClass = FSBUtils::GetBotClassForEntry(bot->GetEntry());
        uint16 level = bot->GetLevel();

        auto const* stats = GetBotClassStats(botClass);
        if (!stats)
            return 0;

        int32 baseSP = stats->baseSpellPower;
        float spPct = 1.f;
        int32 levelSP = stats->spellPowerPerLevel * (level - 1);

        if (FSB_BaseAI* ai = CAST_AI(FSB_BaseAI, bot->AI()))
        {
            spPct += ai->botStats.spellPowerPct;
        }

        int32 value = (baseSP + levelSP) * spPct;

        return value;

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

        if (FSBDruid::BotHasMarkWild(bot))
            multiplier *= 0.985f;

        if (FSBDruid::BotHasIronbark(bot))
            multiplier *= 0.8f;

        if (FSBDruid::BotHasSurvivalInstincts(bot))
            multiplier *= 0.5f;

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

        if (FSBDruid::BotHasMarkWild(bot))
            multiplier *= 1.03f;

        return multiplier;
    }
}
