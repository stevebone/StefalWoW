#include "Log.h"

#include "Followship_bots_config.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"

#include "Followship_bots_powers_handler.h"
#include "Followship_bots_stats_handler.h"

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

        bot->SetClass(FSBUtils::FSBToTCClass(botClass));

        // Health
        ApplyBotHealth(bot, botClass, true);        

        // Power
        ApplyBotBasePower(bot, botClass);

        // Attack Power
        bot->SetStatFlatModifier(UNIT_MOD_ATTACK_POWER, BASE_VALUE, stats->baseAttackPower);
        bot->SetStatFlatModifier(UNIT_MOD_ATTACK_POWER_RANGED, BASE_VALUE, stats->baseRangedAttackPower);
        ApplyBotAttackPower(bot, botClass);

        // Damage
        bot->SetBaseAttackTime(BASE_ATTACK, stats->baseAttackTime);
        bot->SetBaseAttackTime(RANGED_ATTACK, stats->baseRangedAttackTime);
        ApplyBotDamage(bot, botClass);

        // Armor
        ApplyBotArmor(bot, botClass);



        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Base Stats set for bot: {}, Level= {}, TC Class= {}, FSB Class= {}", bot->GetName(), bot->GetLevel(), bot->GetClass(), botClass);
    }

    void ApplyBotBasePower(Creature* bot, FSB_Class botClass)
    {
        if (!bot)
            return;

        auto const* stats = GetBotClassStats(botClass);
        if (!stats)
            return;

        int32 level = bot->GetLevel();

        float modifier = FollowshipBotsConfig::configFSBPowerRate;

        Powers basePowerType = stats->powerType;
        uint32 basePower = ((float)stats->basePower + ((float)stats->powerPerLevel * (level - 1) + (level * 3))) * modifier;

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

        Player* player = FSBMgr::Get()->GetBotOwner(bot);

        int32 level = bot->GetLevel();
        if (player)
            level = bot->GetLevelForTarget(player);

        float modifier = FollowshipBotsConfig::configFSBHealthRate;

        uint32 baseHealth = ((float)stats->baseHealth + ((float)stats->healthPerLevel * (level - 1) + ((float)level * 65))) * modifier;
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

        Player* player = FSBMgr::Get()->GetBotOwner(bot);

        int32 level = bot->GetLevel();
        if (player)
            level = bot->GetLevelForTarget(player);

        float modifier = FollowshipBotsConfig::configFSBPowerRate;

        Powers basePowerType = stats->powerType;
        uint32 basePower = ((float)stats->basePower + ((float)stats->powerPerLevel * (level - 1) + (level * 3))) * modifier;

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

        Player* player = FSBMgr::Get()->GetBotOwner(bot);

        int32 level = bot->GetLevel();
        if (player)
            level = bot->GetLevelForTarget(player);

        float attackPowerPerLevel = stats->attackPowerPerLevel;
        float baseAP = stats->baseAttackPower;
        float baseRAP = stats->baseRangedAttackPower;

        uint32 finalAP = baseAP + (attackPowerPerLevel * level);
        uint32 finalRAP = baseRAP + (attackPowerPerLevel * level);

        bot->SetStatFlatModifier(UNIT_MOD_ATTACK_POWER, BASE_VALUE, finalAP);
        bot->SetStatFlatModifier(UNIT_MOD_ATTACK_POWER_RANGED, BASE_VALUE, finalRAP);
        //bot->SetRangedAttackPower(finalRAttackPower);
        //bot->SetAttackPower(finalAttackPower);

        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Stats AttackPower Bot: {} Has baseAP: {}, levelAP: {}, modAP: {}, multiAP: {}, totalAP: {}",
        //    bot->GetName(), baseAttackPower, level * attackPowerPerLevel, totalAttackPowerMod, multiplierAttackPower, finalAttackPower);
    }

    void ApplyDynamicDamageDealt(Creature* bot, Unit* victim, uint32& damage)
    {
        if (!bot || !bot->IsAlive())
            return;

        if (!victim || !victim->IsAlive())
            return;

        if (damage <= 0)
            return;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return;

        auto stats = baseAI->botClassStats;
        if (!stats)
            return;

        uint8 effectiveLevel = bot->GetLevelForTarget(victim);
        float effectiveAttackPower = stats->baseAttackPower + (stats->attackPowerPerLevel * effectiveLevel);

        float basedamage = (bot->GetBaseDamageForLevel(effectiveLevel) * stats->baseClassDamageVariance) + (effectiveAttackPower / 3);

        float weaponBaseMinDamage = basedamage;
        float weaponBaseMaxDamage = basedamage * 1.5f;

        damage = urand(weaponBaseMinDamage, weaponBaseMaxDamage);
    }

    void ApplyBotDamage(Creature* bot, FSB_Class botClass)
    {
        if (!bot)
            return;

        auto const* stats = GetBotClassStats(botClass);
        if (!stats)
            return;

        Player* player = FSBMgr::Get()->GetBotOwner(bot);

        int32 level = bot->GetLevel();
        if (player)
            level = bot->GetLevelForTarget(player);

        float modifier = FollowshipBotsConfig::configFSBDamageRate;

        float effectiveAttackPower = stats->baseAttackPower + (stats->attackPowerPerLevel * level);

        //float basedamage = (bot->GetBaseDamageForLevel(level) * stats->baseClassDamageVariance) + effectiveAttackPower;
        float basedamage = effectiveAttackPower * modifier;
        
        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Bot {} has base damage {} for effective level {}", bot->GetName(), bot->GetBaseDamageForLevel(level), level);

        float weaponBaseMinDamage = basedamage; // / 90;
        float weaponBaseMaxDamage = weaponBaseMinDamage * 1.5f;

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

    void ApplyBotArmor(Creature* bot, FSB_Class botClass)
    {
        if (!bot)
            return;

        auto const* stats = GetBotClassStats(botClass);
        if (!stats)
            return;

        Player* player = FSBMgr::Get()->GetBotOwner(bot);

        int32 level = bot->GetLevel();
        if (player)
            level = bot->GetLevelForTarget(player);

        float modifier = FollowshipBotsConfig::configFSBArmorRate;

        float baseArmor = (float)(stats->armorPerLevel * level / 3) * modifier;
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

        FSB_Class cls = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());

        ApplyBotHealth(bot, cls, updateHealth);
        ApplyBotPower(bot, cls, updatePower);
        ApplyBotAttackPower(bot, cls);
        ApplyBotDamage(bot, cls);
        ApplyBotArmor(bot, cls);
    }

    int32 BotGetSpellPower(const Creature* bot)
    {
        if (!bot)
            return 0;

        FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());
        uint16 level = bot->GetLevel();

        Unit* target = nullptr;

        // Owner must exist, be a player, and be in world
        if (Unit* owner = bot->GetOwner())
        {
            if (Player* player = owner->ToPlayer())
            {
                if (player->IsInWorld())
                    target = player;
            }
        }

        // If no valid owner, try victim
        if (!target)
        {
            if (Unit* victim = bot->GetVictim())
            {
                if (victim->IsInWorld() && victim->IsAlive())
                    target = victim;
            }
        }

        if (target)
            level = bot->GetLevelForTarget(target);

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

        //if (level > 10) value = value * 1.5;
        //if (level > 20) value = value * 2;
        //if (level > 30) value = value * 2.5;

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

        if (FSBPriest::BotHasPainSuppression(bot))
            multiplier *= 0.6f;

        if (FSBMonk::BotHasFortifyingBrew(bot))
            multiplier *= 0.8f;

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

    float GetBotDamageMultiplier(uint8 level)
    {
        if (level < 1)
            level = 1;

        // Level 1-10: 0.5% ? 5%
        if (level <= 10)
        {
            return 0.005f + (level - 1) * 0.005f;
        }

        // Level 10-20: 30% ? 80%
        if (level <= 20)
        {
            return 0.30f + (level - 10) * 0.05f;
        }

        // Level 20-80: 80% ? 140%
        if (level <= 80)
        {
            return 0.80f + (level - 20) * 0.01f;
        }

        // Above 80: clamp or extend
        return 1.40f; // or extend formula if needed
    }
    int32 CalculateScaledBotDamage(Creature* bot, Unit* victim, int32 rawDamage)
    {
        if (!bot || !victim || rawDamage <= 0)
            return rawDamage;

        // Determine effective level for scaling
        uint8 level = bot->GetLevel();

        if (Player* owner = FSBMgr::Get()->GetBotOwner(bot))
        {
            // Bot hired by a player ? scale with owner
            level = bot->GetLevelForTarget(owner);
        }
        else if (Player* playerVictim = victim->ToPlayer())
        {
            // Bot attacking a player ? scale with the player's level
            level = bot->GetLevelForTarget(playerVictim);
        }

        // Apply level-based multiplier
        float mult = FSBStats::GetBotDamageMultiplier(level);
        int32 scaledDamage = int32(rawDamage * mult);

        TC_LOG_DEBUG("scripts.fsb.combat",
            "FSB: CalculateScaledBotDamage bot={} lvl={} mult={} raw={} scaled={}",
            bot->GetName(), level, mult, rawDamage, scaledDamage
        );

        return scaledDamage;
    }
}
