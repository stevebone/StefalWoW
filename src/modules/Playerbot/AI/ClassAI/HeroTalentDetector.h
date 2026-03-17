/*
 * Copyright (C) 2025-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * Hero Talent Tree Detection System
 *
 * Provides runtime detection of which hero talent tree a bot has selected
 * by checking for signature spells unique to each tree. This enables
 * rotation code to branch based on hero talent tree for optimal ability usage.
 *
 * Detection Strategy:
 *   Each hero talent tree has 2-4 signature abilities. We check HasSpell()
 *   for the first (keystone) ability of each tree. If the bot has it, that
 *   tree is active. Since a bot can only have one hero talent tree per spec,
 *   the first match wins.
 */

#pragma once

#include "SpellValidation_WoW120.h"
#include "SpellValidation_WoW120_Part2.h"
#include "Player.h"
#include <cstdint>

namespace Playerbot
{

/// Identifies a specific hero talent tree across all classes and specs
enum class HeroTalentTree : uint8
{
    NONE = 0,

    // Death Knight
    DEATHBRINGER,
    SANLAYN,
    RIDER_OF_THE_APOCALYPSE,

    // Demon Hunter
    ALDRACHI_REAVER,
    FEL_SCARRED,

    // Druid
    KEEPER_OF_THE_GROVE,
    ELUNES_CHOSEN,
    DRUID_OF_THE_CLAW,
    WILDSTALKER,

    // Evoker
    FLAMESHAPER,
    SCALECOMMANDER,
    CHRONOWARDEN,

    // Hunter
    PACK_LEADER,
    DARK_RANGER,
    SENTINEL,

    // Mage
    SPELLSLINGER,
    SUNFURY,
    FROSTFIRE,

    // Monk
    MASTER_OF_HARMONY,
    SHADO_PAN,
    CONDUIT_OF_THE_CELESTIALS,

    // Paladin
    HERALD_OF_THE_SUN,
    LIGHTSMITH,
    TEMPLAR,

    // Priest
    ORACLE,
    VOIDWEAVER,
    ARCHON,

    // Rogue
    DEATHSTALKER,
    FATEBOUND,
    TRICKSTER,

    // Shaman
    FARSEER,
    STORMBRINGER,
    TOTEMIC,

    // Warlock
    HELLCALLER,
    SOUL_HARVESTER,
    DIABOLIST,

    // Warrior
    SLAYER,
    COLOSSUS,
    MOUNTAIN_THANE,

    MAX_HERO_TALENT_TREE
};

/// Returns a human-readable name for a hero talent tree
inline const char* GetHeroTalentTreeName(HeroTalentTree tree)
{
    switch (tree)
    {
        case HeroTalentTree::NONE:                       return "None";
        case HeroTalentTree::DEATHBRINGER:               return "Deathbringer";
        case HeroTalentTree::SANLAYN:                    return "San'layn";
        case HeroTalentTree::RIDER_OF_THE_APOCALYPSE:    return "Rider of the Apocalypse";
        case HeroTalentTree::ALDRACHI_REAVER:            return "Aldrachi Reaver";
        case HeroTalentTree::FEL_SCARRED:                return "Fel-Scarred";
        case HeroTalentTree::KEEPER_OF_THE_GROVE:        return "Keeper of the Grove";
        case HeroTalentTree::ELUNES_CHOSEN:              return "Elune's Chosen";
        case HeroTalentTree::DRUID_OF_THE_CLAW:          return "Druid of the Claw";
        case HeroTalentTree::WILDSTALKER:                return "Wildstalker";
        case HeroTalentTree::FLAMESHAPER:                return "Flameshaper";
        case HeroTalentTree::SCALECOMMANDER:             return "Scalecommander";
        case HeroTalentTree::CHRONOWARDEN:               return "Chronowarden";
        case HeroTalentTree::PACK_LEADER:                return "Pack Leader";
        case HeroTalentTree::DARK_RANGER:                return "Dark Ranger";
        case HeroTalentTree::SENTINEL:                   return "Sentinel";
        case HeroTalentTree::SPELLSLINGER:               return "Spellslinger";
        case HeroTalentTree::SUNFURY:                    return "Sunfury";
        case HeroTalentTree::FROSTFIRE:                  return "Frostfire";
        case HeroTalentTree::MASTER_OF_HARMONY:          return "Master of Harmony";
        case HeroTalentTree::SHADO_PAN:                  return "Shado-Pan";
        case HeroTalentTree::CONDUIT_OF_THE_CELESTIALS:  return "Conduit of the Celestials";
        case HeroTalentTree::HERALD_OF_THE_SUN:          return "Herald of the Sun";
        case HeroTalentTree::LIGHTSMITH:                 return "Lightsmith";
        case HeroTalentTree::TEMPLAR:                    return "Templar";
        case HeroTalentTree::ORACLE:                     return "Oracle";
        case HeroTalentTree::VOIDWEAVER:                 return "Voidweaver";
        case HeroTalentTree::ARCHON:                     return "Archon";
        case HeroTalentTree::DEATHSTALKER:               return "Deathstalker";
        case HeroTalentTree::FATEBOUND:                  return "Fatebound";
        case HeroTalentTree::TRICKSTER:                  return "Trickster";
        case HeroTalentTree::FARSEER:                    return "Farseer";
        case HeroTalentTree::STORMBRINGER:               return "Stormbringer";
        case HeroTalentTree::TOTEMIC:                    return "Totemic";
        case HeroTalentTree::HELLCALLER:                 return "Hellcaller";
        case HeroTalentTree::SOUL_HARVESTER:             return "Soul Harvester";
        case HeroTalentTree::DIABOLIST:                  return "Diabolist";
        case HeroTalentTree::SLAYER:                     return "Slayer";
        case HeroTalentTree::COLOSSUS:                   return "Colossus";
        case HeroTalentTree::MOUNTAIN_THANE:             return "Mountain Thane";
        default:                                         return "Unknown";
    }
}

/**
 * Detects a bot's active hero talent tree by checking signature spells.
 *
 * Each hero talent tree has a keystone ability that is unique to that tree.
 * We check HasSpell() for these keystone abilities. Since a bot can only
 * have one hero talent tree per spec, the first match determines the tree.
 *
 * This function is designed to be called once at combat start or talent
 * application and cached, NOT every tick.
 *
 * @param bot       The bot player to check
 * @param classId   The bot's class (from Player::getClass())
 * @param spec      The bot's active specialization
 * @return The detected HeroTalentTree, or NONE if no hero talents detected
 */
inline HeroTalentTree DetectHeroTalentTree(Player const* bot, uint8 classId, ChrSpecialization spec)
{
    if (!bot)
        return HeroTalentTree::NONE;

    // Hero talents require level 71+
    if (bot->GetLevel() < 71)
        return HeroTalentTree::NONE;

    using namespace WoW120Spells;

    switch (classId)
    {
        case CLASS_DEATH_KNIGHT:
        {
            // Blood: Deathbringer / San'layn
            if (spec == ChrSpecialization::DeathKnightBlood)
            {
                if (bot->HasSpell(DeathKnight::Blood::REAPER_MARK))
                    return HeroTalentTree::DEATHBRINGER;
                if (bot->HasSpell(DeathKnight::Blood::VAMPIRIC_STRIKE))
                    return HeroTalentTree::SANLAYN;
            }
            // Frost: Rider of the Apocalypse / Deathbringer
            else if (spec == ChrSpecialization::DeathKnightFrost)
            {
                if (bot->HasSpell(DeathKnight::Frost::APOCALYPSE_NOW))
                    return HeroTalentTree::RIDER_OF_THE_APOCALYPSE;
                if (bot->HasSpell(DeathKnight::Frost::FROST_REAPER_MARK))
                    return HeroTalentTree::DEATHBRINGER;
            }
            // Unholy: San'layn / Rider of the Apocalypse
            else if (spec == ChrSpecialization::DeathKnightUnholy)
            {
                if (bot->HasSpell(DeathKnight::Unholy::UNHOLY_VAMPIRIC_STRIKE))
                    return HeroTalentTree::SANLAYN;
                if (bot->HasSpell(DeathKnight::Unholy::UNHOLY_APOCALYPSE))
                    return HeroTalentTree::RIDER_OF_THE_APOCALYPSE;
            }
            break;
        }

        case CLASS_DEMON_HUNTER:
        {
            // Havoc: Aldrachi Reaver / Fel-Scarred
            if (spec == ChrSpecialization::DemonHunterHavoc)
            {
                if (bot->HasSpell(DemonHunter::Havoc::ALDRACHI_TACTICS))
                    return HeroTalentTree::ALDRACHI_REAVER;
                if (bot->HasSpell(DemonHunter::Havoc::FEL_SCARRED_METAMORPHOSIS))
                    return HeroTalentTree::FEL_SCARRED;
            }
            // Vengeance: Aldrachi Reaver / Fel-Scarred
            else if (spec == ChrSpecialization::DemonHunterVengeance)
            {
                if (bot->HasSpell(DemonHunter::Vengeance::VENG_ALDRACHI_TACTICS))
                    return HeroTalentTree::ALDRACHI_REAVER;
                if (bot->HasSpell(DemonHunter::Vengeance::VENG_FEL_SCARRED))
                    return HeroTalentTree::FEL_SCARRED;
            }
            break;
        }

        case CLASS_DRUID:
        {
            // Balance: Keeper of the Grove / Elune's Chosen
            if (spec == ChrSpecialization::DruidBalance)
            {
                if (bot->HasSpell(Druid::Balance::POWER_OF_THE_DREAM))
                    return HeroTalentTree::KEEPER_OF_THE_GROVE;
                if (bot->HasSpell(Druid::Balance::LUNAR_CALLING))
                    return HeroTalentTree::ELUNES_CHOSEN;
            }
            // Feral: Druid of the Claw / Wildstalker
            else if (spec == ChrSpecialization::DruidFeral)
            {
                if (bot->HasSpell(Druid::Feral::RAVAGE))
                    return HeroTalentTree::DRUID_OF_THE_CLAW;
                if (bot->HasSpell(Druid::Feral::WILDSHAPE_MASTERY))
                    return HeroTalentTree::WILDSTALKER;
            }
            // Guardian: Elune's Chosen / Druid of the Claw
            else if (spec == ChrSpecialization::DruidGuardian)
            {
                if (bot->HasSpell(Druid::Guardian::LUNAR_BEAM_ENHANCED))
                    return HeroTalentTree::ELUNES_CHOSEN;
                if (bot->HasSpell(Druid::Guardian::URSINE_ADEPT))
                    return HeroTalentTree::DRUID_OF_THE_CLAW;
            }
            // Restoration: Keeper of the Grove / Wildstalker
            else if (spec == ChrSpecialization::DruidRestoration)
            {
                if (bot->HasSpell(Druid::Restoration::DREAM_OF_CENARIUS))
                    return HeroTalentTree::KEEPER_OF_THE_GROVE;
                if (bot->HasSpell(Druid::Restoration::EMPOWERED_SHAPESHIFTING))
                    return HeroTalentTree::WILDSTALKER;
            }
            break;
        }

        case CLASS_EVOKER:
        {
            // Devastation: Flameshaper / Scalecommander
            if (spec == ChrSpecialization::EvokerDevastation)
            {
                if (bot->HasSpell(Evoker::Devastation::ENGULF))
                    return HeroTalentTree::FLAMESHAPER;
                if (bot->HasSpell(Evoker::Devastation::MASS_DISINTEGRATE))
                    return HeroTalentTree::SCALECOMMANDER;
            }
            // Preservation: Chronowarden / Flameshaper
            else if (spec == ChrSpecialization::EvokerPreservation)
            {
                if (bot->HasSpell(Evoker::Preservation::CHRONO_FLAMES))
                    return HeroTalentTree::CHRONOWARDEN;
                if (bot->HasSpell(Evoker::Preservation::PRES_ENGULF))
                    return HeroTalentTree::FLAMESHAPER;
            }
            // Augmentation: Chronowarden / Scalecommander
            else if (spec == ChrSpecialization::EvokerAugmentation)
            {
                if (bot->HasSpell(Evoker::Augmentation::CHRONO_MAGIC))
                    return HeroTalentTree::CHRONOWARDEN;
                if (bot->HasSpell(Evoker::Augmentation::MIGHT_OF_THE_BLACK_DRAGONFLIGHT))
                    return HeroTalentTree::SCALECOMMANDER;
            }
            break;
        }

        case CLASS_HUNTER:
        {
            // Beast Mastery: Pack Leader / Dark Ranger
            if (spec == ChrSpecialization::HunterBeastMastery)
            {
                if (bot->HasSpell(Hunter::BeastMastery::VICIOUS_HUNT))
                    return HeroTalentTree::PACK_LEADER;
                if (bot->HasSpell(Hunter::BeastMastery::BLACK_ARROW))
                    return HeroTalentTree::DARK_RANGER;
            }
            // Marksmanship: Sentinel / Dark Ranger
            else if (spec == ChrSpecialization::HunterMarksmanship)
            {
                if (bot->HasSpell(Hunter::Marksmanship::SENTINEL_OWL))
                    return HeroTalentTree::SENTINEL;
                if (bot->HasSpell(Hunter::Marksmanship::MM_BLACK_ARROW))
                    return HeroTalentTree::DARK_RANGER;
            }
            // Survival: Pack Leader / Sentinel
            else if (spec == ChrSpecialization::HunterSurvival)
            {
                if (bot->HasSpell(Hunter::Survival::SV_VICIOUS_HUNT))
                    return HeroTalentTree::PACK_LEADER;
                if (bot->HasSpell(Hunter::Survival::SV_SENTINEL))
                    return HeroTalentTree::SENTINEL;
            }
            break;
        }

        case CLASS_MAGE:
        {
            // Arcane: Spellslinger / Sunfury
            if (spec == ChrSpecialization::MageArcane)
            {
                if (bot->HasSpell(Mage::Arcane::SPLINTERSTORM))
                    return HeroTalentTree::SPELLSLINGER;
                if (bot->HasSpell(Mage::Arcane::GLORIOUS_INCANDESCENCE))
                    return HeroTalentTree::SUNFURY;
            }
            // Fire: Frostfire / Sunfury
            else if (spec == ChrSpecialization::MageFire)
            {
                if (bot->HasSpell(Mage::Fire::FROSTFIRE_BOLT))
                    return HeroTalentTree::FROSTFIRE;
                if (bot->HasSpell(Mage::Fire::FIRE_GLORIOUS_INCANDESCENCE))
                    return HeroTalentTree::SUNFURY;
            }
            // Frost: Frostfire / Spellslinger
            else if (spec == ChrSpecialization::MageFrost)
            {
                if (bot->HasSpell(Mage::Frost::FROST_FROSTFIRE_BOLT))
                    return HeroTalentTree::FROSTFIRE;
                if (bot->HasSpell(Mage::Frost::FROST_SPLINTERSTORM))
                    return HeroTalentTree::SPELLSLINGER;
            }
            break;
        }

        case CLASS_MONK:
        {
            // Brewmaster: Master of Harmony / Shado-Pan
            if (spec == ChrSpecialization::MonkBrewmaster)
            {
                if (bot->HasSpell(Monk::Brewmaster::ASPECT_OF_HARMONY))
                    return HeroTalentTree::MASTER_OF_HARMONY;
                if (bot->HasSpell(Monk::Brewmaster::FLURRY_STRIKES))
                    return HeroTalentTree::SHADO_PAN;
            }
            // Mistweaver: Master of Harmony / Conduit of the Celestials
            else if (spec == ChrSpecialization::MonkMistweaver)
            {
                if (bot->HasSpell(Monk::Mistweaver::MW_ASPECT_OF_HARMONY))
                    return HeroTalentTree::MASTER_OF_HARMONY;
                if (bot->HasSpell(Monk::Mistweaver::CELESTIAL_CONDUIT))
                    return HeroTalentTree::CONDUIT_OF_THE_CELESTIALS;
            }
            // Windwalker: Shado-Pan / Conduit of the Celestials
            else if (spec == ChrSpecialization::MonkWindwalker)
            {
                if (bot->HasSpell(Monk::Windwalker::WW_FLURRY_STRIKES))
                    return HeroTalentTree::SHADO_PAN;
                if (bot->HasSpell(Monk::Windwalker::WW_CELESTIAL_CONDUIT))
                    return HeroTalentTree::CONDUIT_OF_THE_CELESTIALS;
            }
            break;
        }

        case CLASS_PALADIN:
        {
            // Holy: Herald of the Sun / Lightsmith
            if (spec == ChrSpecialization::PaladinHoly)
            {
                if (bot->HasSpell(Paladin::Holy::DAWNLIGHT))
                    return HeroTalentTree::HERALD_OF_THE_SUN;
                if (bot->HasSpell(Paladin::Holy::HOLY_ARMAMENT))
                    return HeroTalentTree::LIGHTSMITH;
            }
            // Protection: Lightsmith / Templar
            else if (spec == ChrSpecialization::PaladinProtection)
            {
                if (bot->HasSpell(Paladin::Protection::PROT_HOLY_ARMAMENT))
                    return HeroTalentTree::LIGHTSMITH;
                if (bot->HasSpell(Paladin::Protection::LIGHTS_GUIDANCE))
                    return HeroTalentTree::TEMPLAR;
            }
            // Retribution: Templar / Herald of the Sun
            else if (spec == ChrSpecialization::PaladinRetribution)
            {
                if (bot->HasSpell(Paladin::Retribution::RADIANT_GLORY))
                    return HeroTalentTree::TEMPLAR;
                if (bot->HasSpell(Paladin::Retribution::RET_DAWNLIGHT))
                    return HeroTalentTree::HERALD_OF_THE_SUN;
            }
            break;
        }

        case CLASS_PRIEST:
        {
            // Discipline: Oracle / Voidweaver
            if (spec == ChrSpecialization::PriestDiscipline)
            {
                if (bot->HasSpell(Priest::Discipline::PREEMPTIVE_CARE))
                    return HeroTalentTree::ORACLE;
                if (bot->HasSpell(Priest::Discipline::VOID_BLAST))
                    return HeroTalentTree::VOIDWEAVER;
            }
            // Holy: Oracle / Archon
            else if (spec == ChrSpecialization::PriestHoly)
            {
                if (bot->HasSpell(Priest::HolyPriest::HOLY_PREEMPTIVE_CARE))
                    return HeroTalentTree::ORACLE;
                if (bot->HasSpell(Priest::HolyPriest::POWER_OF_THE_LIGHT))
                    return HeroTalentTree::ARCHON;
            }
            // Shadow: Voidweaver / Archon
            else if (spec == ChrSpecialization::PriestShadow)
            {
                if (bot->HasSpell(Priest::Shadow::SHADOW_VOID_BLAST))
                    return HeroTalentTree::VOIDWEAVER;
                if (bot->HasSpell(Priest::Shadow::SHADOW_POWER_OF_THE_LIGHT))
                    return HeroTalentTree::ARCHON;
            }
            break;
        }

        case CLASS_ROGUE:
        {
            // Assassination: Deathstalker / Fatebound
            if (spec == ChrSpecialization::RogueAssassination)
            {
                if (bot->HasSpell(Rogue::Assassination::DEATHSTALKERS_MARK))
                    return HeroTalentTree::DEATHSTALKER;
                if (bot->HasSpell(Rogue::Assassination::HAND_OF_FATE))
                    return HeroTalentTree::FATEBOUND;
            }
            // Outlaw: Trickster / Fatebound
            else if (spec == ChrSpecialization::RogueOutlaw)
            {
                if (bot->HasSpell(Rogue::Outlaw::UNSEEN_BLADE))
                    return HeroTalentTree::TRICKSTER;
                if (bot->HasSpell(Rogue::Outlaw::OUTLAW_HAND_OF_FATE))
                    return HeroTalentTree::FATEBOUND;
            }
            // Subtlety: Deathstalker / Trickster
            else if (spec == ChrSpecialization::RogueSubtely)
            {
                if (bot->HasSpell(Rogue::Subtlety::SUB_DEATHSTALKERS_MARK))
                    return HeroTalentTree::DEATHSTALKER;
                if (bot->HasSpell(Rogue::Subtlety::SUB_UNSEEN_BLADE))
                    return HeroTalentTree::TRICKSTER;
            }
            break;
        }

        case CLASS_SHAMAN:
        {
            // Elemental: Farseer / Stormbringer
            if (spec == ChrSpecialization::ShamanElemental)
            {
                if (bot->HasSpell(Shaman::Elemental::ANCESTRAL_SWIFTNESS))
                    return HeroTalentTree::FARSEER;
                if (bot->HasSpell(Shaman::Elemental::TEMPEST_STRIKES))
                    return HeroTalentTree::STORMBRINGER;
            }
            // Enhancement: Totemic / Stormbringer
            else if (spec == ChrSpecialization::ShamanEnhancement)
            {
                if (bot->HasSpell(Shaman::Enhancement::SURGING_TOTEM))
                    return HeroTalentTree::TOTEMIC;
                if (bot->HasSpell(Shaman::Enhancement::ENH_TEMPEST_STRIKES))
                    return HeroTalentTree::STORMBRINGER;
            }
            // Restoration: Farseer / Totemic
            else if (spec == ChrSpecialization::ShamanRestoration)
            {
                if (bot->HasSpell(Shaman::Restoration::RESTO_ANCESTRAL_SWIFTNESS))
                    return HeroTalentTree::FARSEER;
                if (bot->HasSpell(Shaman::Restoration::RESTO_SURGING_TOTEM))
                    return HeroTalentTree::TOTEMIC;
            }
            break;
        }

        case CLASS_WARLOCK:
        {
            // Affliction: Hellcaller / Soul Harvester
            if (spec == ChrSpecialization::WarlockAffliction)
            {
                if (bot->HasSpell(Warlock::Affliction::WITHER))
                    return HeroTalentTree::HELLCALLER;
                if (bot->HasSpell(Warlock::Affliction::DEMONIC_SOUL))
                    return HeroTalentTree::SOUL_HARVESTER;
            }
            // Demonology: Diabolist / Soul Harvester
            else if (spec == ChrSpecialization::WarlockDemonology)
            {
                if (bot->HasSpell(Warlock::Demonology::DIABOLIC_RITUAL))
                    return HeroTalentTree::DIABOLIST;
                if (bot->HasSpell(Warlock::Demonology::DEMO_DEMONIC_SOUL))
                    return HeroTalentTree::SOUL_HARVESTER;
            }
            // Destruction: Hellcaller / Diabolist
            else if (spec == ChrSpecialization::WarlockDestruction)
            {
                if (bot->HasSpell(Warlock::Destruction::DESTRO_WITHER))
                    return HeroTalentTree::HELLCALLER;
                if (bot->HasSpell(Warlock::Destruction::DESTRO_DIABOLIC_RITUAL))
                    return HeroTalentTree::DIABOLIST;
            }
            break;
        }

        case CLASS_WARRIOR:
        {
            // Arms: Slayer / Colossus
            if (spec == ChrSpecialization::WarriorArms)
            {
                if (bot->HasSpell(Warrior::Arms::SLAYERS_STRIKE))
                    return HeroTalentTree::SLAYER;
                if (bot->HasSpell(Warrior::Arms::DEMOLISH))
                    return HeroTalentTree::COLOSSUS;
            }
            // Fury: Slayer / Mountain Thane
            else if (spec == ChrSpecialization::WarriorFury)
            {
                if (bot->HasSpell(Warrior::Fury::SLAYERS_STRIKE))
                    return HeroTalentTree::SLAYER;
                if (bot->HasSpell(Warrior::Fury::THUNDER_BLAST))
                    return HeroTalentTree::MOUNTAIN_THANE;
            }
            // Protection: Colossus / Mountain Thane
            else if (spec == ChrSpecialization::WarriorProtection)
            {
                if (bot->HasSpell(Warrior::Protection::DEMOLISH))
                    return HeroTalentTree::COLOSSUS;
                if (bot->HasSpell(Warrior::Protection::THUNDER_BLAST))
                    return HeroTalentTree::MOUNTAIN_THANE;
            }
            break;
        }

        default:
            break;
    }

    return HeroTalentTree::NONE;
}

/**
 * Helper struct for caching hero talent tree detection results.
 * Should be stored as a member in each ClassAI or specialization class.
 * Call Refresh() once at combat start or after talent changes, then
 * use GetTree() in rotation code for O(1) branching.
 */
struct HeroTalentCache
{
    HeroTalentTree tree = HeroTalentTree::NONE;
    bool detected = false;

    /// Detect and cache the hero talent tree for the given bot
    void Refresh(Player const* bot)
    {
        if (!bot)
        {
            tree = HeroTalentTree::NONE;
            detected = true;
            return;
        }

        tree = DetectHeroTalentTree(bot, bot->GetClass(), bot->GetPrimarySpecialization());
        detected = true;
    }

    /// Get the cached hero talent tree. Returns NONE if not yet detected.
    HeroTalentTree GetTree() const { return tree; }

    /// Check if a specific tree is active
    bool IsTree(HeroTalentTree check) const { return tree == check; }

    /// Check if any hero talent tree is active
    bool HasHeroTalents() const { return tree != HeroTalentTree::NONE; }

    /// Invalidate the cache (forces re-detection on next Refresh())
    void Invalidate() { detected = false; tree = HeroTalentTree::NONE; }
};

} // namespace Playerbot
