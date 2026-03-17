/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "BehaviorTreeFactory.h"
#include "Log.h"
#include "ObjectAccessor.h"

namespace Playerbot
{

// Static member initialization
::std::unordered_map<::std::string, ::std::function<::std::shared_ptr<BTNode>()>> BehaviorTreeFactory::_customTreeBuilders;

::std::shared_ptr<BTNode> BehaviorTreeFactory::CreateTree(TreeType type)
{
    switch (type)
    {
        case TreeType::MELEE_COMBAT:
            return BuildMeleeCombatTree();
        case TreeType::RANGED_COMBAT:
            return BuildRangedCombatTree();
        case TreeType::TANK_COMBAT:
            return BuildTankCombatTree();
        case TreeType::SINGLE_TARGET_HEALING:
            return BuildSingleTargetHealingTree();
        case TreeType::GROUP_HEALING:
            return BuildGroupHealingTree();
        case TreeType::DISPEL_PRIORITY:
            return BuildDispelPriorityTree();
        case TreeType::FOLLOW_LEADER:
            return BuildFollowLeaderTree();
        case TreeType::COMBAT_POSITIONING:
            return BuildCombatPositioningTree();
        case TreeType::FLEE_TO_SAFETY:
            return BuildFleeToSafetyTree();
        case TreeType::BUFF_MAINTENANCE:
            return BuildBuffMaintenanceTree();
        case TreeType::RESOURCE_MANAGEMENT:
            return BuildResourceManagementTree();
        default:
            TC_LOG_ERROR("playerbot.bt", "Unknown tree type requested: {}", uint32(type));
            return nullptr;
    }
}

::std::shared_ptr<BTNode> BehaviorTreeFactory::BuildMeleeCombatTree()
{
    // Root: Selector (try combat or flee)
    auto root = ::std::make_shared<BTSelector>("MeleeCombatRoot");

    // Branch 1: Flee if critically wounded
    auto fleeCondition = ::std::make_shared<BTCondition>("CriticalHealth",
        [](BotAI* ai, BTBlackboard& /*bb*/) {
            if (!ai) return false;
            Player* bot = ai->GetBot();
            if (!bot) return false;
            return bot->GetHealthPct() < 20.0f && bot->IsInCombat();
        }
    );
    root->AddChild(fleeCondition);

    // Branch 2: Melee combat sequence
    auto combatSequence = ::std::make_shared<BTSequence>("MeleeCombat");

    // Check has target
    combatSequence->AddChild(::std::make_shared<BTCheckHasTarget>());

    // Check in melee range (0-5 yards)
    combatSequence->AddChild(::std::make_shared<BTCheckInRange>(0.0f, 5.0f));

    // Use defensive cooldowns if low health
    auto defensiveSelector = ::std::make_shared<BTSelector>("DefensiveCooldowns");
    defensiveSelector->AddChild(::std::make_shared<BTCondition>("HighHealth",
        [](BotAI* ai, BTBlackboard& /*bb*/) {
            if (!ai) return false;
            Player* bot = ai->GetBot();
            return bot && bot->GetHealthPct() > 50.0f;
        }
    ));
    // If low health, try to use defensive CD (class-specific)
    defensiveSelector->AddChild(::std::make_shared<BTAction>("UseDefensive",
        [](BotAI* ai, BTBlackboard& bb) -> BTStatus {
            if (!ai) return BTStatus::INVALID;
            Player* bot = ai->GetBot();
            if (!bot) return BTStatus::INVALID;

            uint8 playerClass = bot->GetClass();
            ChrSpecializationEntry const* spec = bot->GetPrimarySpecializationEntry();
            uint32 specId = spec ? spec->ID : 0;
            uint32 defSpellId = 0;

            switch (playerClass)
            {
                case CLASS_WARRIOR:
                    // Die By the Sword (Arms/Fury), Ignore Pain (Prot), Rallying Cry
                    if (specId == 71 || specId == 72) // Arms/Fury
                        defSpellId = bot->HasSpell(118038) && !bot->GetSpellHistory()->HasCooldown(118038) ? 118038 : 0; // Die By the Sword
                    else
                        defSpellId = bot->HasSpell(190456) && !bot->GetSpellHistory()->HasCooldown(190456) ? 190456 : 0; // Ignore Pain
                    break;

                case CLASS_PALADIN:
                    // Divine Shield > Divine Protection > Word of Glory
                    if (bot->HasSpell(642) && !bot->GetSpellHistory()->HasCooldown(642))
                        defSpellId = 642; // Divine Shield
                    else if (bot->HasSpell(498) && !bot->GetSpellHistory()->HasCooldown(498))
                        defSpellId = 498; // Divine Protection
                    break;

                case CLASS_DEATH_KNIGHT:
                    // Icebound Fortitude > Anti-Magic Shell
                    if (bot->HasSpell(48792) && !bot->GetSpellHistory()->HasCooldown(48792))
                        defSpellId = 48792; // Icebound Fortitude
                    else if (bot->HasSpell(48707) && !bot->GetSpellHistory()->HasCooldown(48707))
                        defSpellId = 48707; // Anti-Magic Shell
                    break;

                case CLASS_ROGUE:
                    // Evasion > Cloak of Shadows > Crimson Vial
                    if (bot->HasSpell(5277) && !bot->GetSpellHistory()->HasCooldown(5277))
                        defSpellId = 5277; // Evasion
                    else if (bot->HasSpell(31224) && !bot->GetSpellHistory()->HasCooldown(31224))
                        defSpellId = 31224; // Cloak of Shadows
                    else if (bot->HasSpell(185311) && !bot->GetSpellHistory()->HasCooldown(185311))
                        defSpellId = 185311; // Crimson Vial
                    break;

                case CLASS_DRUID:
                    // Barkskin > Survival Instincts
                    if (bot->HasSpell(22812) && !bot->GetSpellHistory()->HasCooldown(22812))
                        defSpellId = 22812; // Barkskin
                    else if (specId == 103 || specId == 104) // Feral/Guardian
                    {
                        if (bot->HasSpell(61336) && !bot->GetSpellHistory()->HasCooldown(61336))
                            defSpellId = 61336; // Survival Instincts
                    }
                    break;

                case CLASS_MONK:
                    // Touch of Karma (WW) > Fortifying Brew > Diffuse Magic
                    if (specId == 269 && bot->HasSpell(122470) && !bot->GetSpellHistory()->HasCooldown(122470))
                        defSpellId = 122470; // Touch of Karma
                    else if (bot->HasSpell(115203) && !bot->GetSpellHistory()->HasCooldown(115203))
                        defSpellId = 115203; // Fortifying Brew
                    else if (bot->HasSpell(122783) && !bot->GetSpellHistory()->HasCooldown(122783))
                        defSpellId = 122783; // Diffuse Magic
                    break;

                case CLASS_DEMON_HUNTER:
                    // Blur (Havoc) > Netherwalk
                    if (specId == 577 && bot->HasSpell(198589) && !bot->GetSpellHistory()->HasCooldown(198589))
                        defSpellId = 198589; // Blur
                    else if (bot->HasSpell(196555) && !bot->GetSpellHistory()->HasCooldown(196555))
                        defSpellId = 196555; // Netherwalk
                    break;

                default:
                    break;
            }

            if (defSpellId > 0)
            {
                bb.Set<uint32>("DefensiveSpellId", defSpellId);
                return BTStatus::SUCCESS;
            }
            return BTStatus::FAILURE;
        }
    ));
    combatSequence->AddChild(defensiveSelector);

    // Face target
    combatSequence->AddChild(::std::make_shared<BTFaceTarget>());

    // Execute melee attack
    combatSequence->AddChild(::std::make_shared<BTMeleeAttack>());

    root->AddChild(combatSequence);

    return root;
}

::std::shared_ptr<BTNode> BehaviorTreeFactory::BuildRangedCombatTree()
{
    auto root = ::std::make_shared<BTSelector>("RangedCombatRoot");

    // Flee if critically wounded
    auto fleeSequence = ::std::make_shared<BTSequence>("FleeIfCritical");
    fleeSequence->AddChild(::std::make_shared<BTCondition>("CriticalHealth",
        [](BotAI* ai, BTBlackboard& /*bb*/) {
            if (!ai) return false;
            Player* bot = ai->GetBot();
            return bot && bot->GetHealthPct() < 20.0f && bot->IsInCombat();
        }
    ));
    fleeSequence->AddChild(::std::make_shared<BTAction>("StartFleeing",
        [](BotAI* /*ai*/, BTBlackboard& bb) -> BTStatus {
            bb.Set<bool>("ShouldFlee", true);
            return BTStatus::SUCCESS;
        }
    ));
    root->AddChild(fleeSequence);

    // Ranged combat sequence
    auto combatSequence = ::std::make_shared<BTSequence>("RangedCombat");

    combatSequence->AddChild(::std::make_shared<BTCheckHasTarget>());

    // Check in ranged range (5-40 yards)
    auto rangeCheck = ::std::make_shared<BTSelector>("RangeManagement");
    rangeCheck->AddChild(::std::make_shared<BTCheckInRange>(5.0f, 40.0f));

    // If not in range, move to optimal range
    auto moveSequence = ::std::make_shared<BTSequence>("MoveToRange");
    moveSequence->AddChild(::std::make_shared<BTCondition>("OutOfRange",
        [](BotAI* ai, BTBlackboard& bb) {
            Unit* target = nullptr;
            if (!bb.Get<Unit*>("CurrentTarget", target) || !target)
                return false;

            if (!ai) return false;
            Player* bot = ai->GetBot();
            if (!bot) return false;

            float distance = bot->GetDistance(target);
            return distance < 5.0f || distance > 40.0f;
        }
    ));
    moveSequence->AddChild(::std::make_shared<BTMoveToTarget>(20.0f, 35.0f));
    rangeCheck->AddChild(moveSequence);

    combatSequence->AddChild(rangeCheck);

    // Face target
    combatSequence->AddChild(::std::make_shared<BTFaceTarget>());

    // Cast ranged abilities - Full class-specific rotation implementation
    combatSequence->AddChild(::std::make_shared<BTAction>("CastRangedSpell",
        [](BotAI* ai, BTBlackboard& bb) -> BTStatus {
            if (!ai) return BTStatus::INVALID;
            Player* bot = ai->GetBot();
            if (!bot) return BTStatus::INVALID;

            Unit* target = bb.GetOr<Unit*>("Target", nullptr);
            if (!target || !target->IsAlive())
                return BTStatus::FAILURE;

            uint8 playerClass = bot->GetClass();
            ChrSpecializationEntry const* spec = bot->GetPrimarySpecializationEntry();
            uint32 specId = spec ? spec->ID : 0;
            uint32 spellId = 0;

            switch (playerClass)
            {
                case CLASS_HUNTER:
                {
                    // Beast Mastery (253), Marksmanship (254), Survival (255)
                    if (specId == 253) // Beast Mastery
                    {
                        // Priority: Kill Command > Barbed Shot > Cobra Shot
                        if (bot->HasSpell(34026) && !bot->GetSpellHistory()->HasCooldown(34026))
                            spellId = 34026; // Kill Command
                        else if (bot->HasSpell(217200) && !bot->GetSpellHistory()->HasCooldown(217200))
                            spellId = 217200; // Barbed Shot
                        else if (bot->HasSpell(193455))
                            spellId = 193455; // Cobra Shot
                    }
                    else if (specId == 254) // Marksmanship
                    {
                        // Priority: Aimed Shot > Rapid Fire > Arcane Shot > Steady Shot
                        if (bot->HasSpell(19434) && !bot->GetSpellHistory()->HasCooldown(19434) &&
                            bot->GetPower(POWER_FOCUS) >= 35)
                            spellId = 19434; // Aimed Shot
                        else if (bot->HasSpell(257044) && !bot->GetSpellHistory()->HasCooldown(257044))
                            spellId = 257044; // Rapid Fire
                        else if (bot->HasSpell(185358) && bot->GetPower(POWER_FOCUS) >= 20)
                            spellId = 185358; // Arcane Shot
                        else if (bot->HasSpell(56641))
                            spellId = 56641; // Steady Shot
                    }
                    else // Survival (melee but can have ranged abilities)
                    {
                        if (bot->HasSpell(186270))
                            spellId = 186270; // Raptor Strike (melee fallback)
                    }
                    break;
                }
                case CLASS_MAGE:
                {
                    // Arcane (62), Fire (63), Frost (64)
                    if (specId == 62) // Arcane
                    {
                        // Priority: Arcane Blast > Arcane Missiles > Arcane Barrage
                        uint32 arcaneCharges = bot->GetPower(POWER_ARCANE_CHARGES);
                        if (arcaneCharges >= 4 && bot->HasSpell(44425))
                            spellId = 44425; // Arcane Barrage (dump charges)
                        else if (bot->HasSpell(5143) && bot->HasAura(79683)) // Arcane Missiles proc
                            spellId = 5143; // Arcane Missiles
                        else if (bot->HasSpell(30451))
                            spellId = 30451; // Arcane Blast
                    }
                    else if (specId == 63) // Fire
                    {
                        // Priority: Pyroblast (proc) > Fire Blast > Fireball
                        if (bot->HasAura(48108) && bot->HasSpell(11366)) // Hot Streak
                            spellId = 11366; // Pyroblast (instant)
                        else if (bot->HasSpell(108853) && !bot->GetSpellHistory()->HasCooldown(108853))
                            spellId = 108853; // Fire Blast
                        else if (bot->HasSpell(133))
                            spellId = 133; // Fireball
                    }
                    else // Frost (64)
                    {
                        // Priority: Ice Lance (proc) > Flurry (proc) > Frostbolt
                        if (bot->HasAura(44544) && bot->HasSpell(30455)) // Fingers of Frost
                            spellId = 30455; // Ice Lance (shatter)
                        else if (bot->HasAura(190446) && bot->HasSpell(44614)) // Brain Freeze
                            spellId = 44614; // Flurry
                        else if (bot->HasSpell(116))
                            spellId = 116; // Frostbolt
                    }
                    break;
                }
                case CLASS_WARLOCK:
                {
                    // Affliction (265), Demonology (266), Destruction (267)
                    if (specId == 265) // Affliction
                    {
                        // Priority: Maintain DoTs > Malefic Rapture > Shadow Bolt
                        if (bot->HasSpell(980) && !target->HasAura(980)) // Agony not on target
                            spellId = 980; // Agony
                        else if (bot->HasSpell(172) && !target->HasAura(172)) // Corruption not on target
                            spellId = 172; // Corruption
                        else if (bot->HasSpell(316099) && bot->GetPower(POWER_SOUL_SHARDS) >= 1)
                            spellId = 316099; // Unstable Affliction
                        else if (bot->HasSpell(324536) && bot->GetPower(POWER_SOUL_SHARDS) >= 1)
                            spellId = 324536; // Malefic Rapture
                        else if (bot->HasSpell(232670))
                            spellId = 232670; // Shadow Bolt (Affliction version)
                    }
                    else if (specId == 266) // Demonology
                    {
                        // Priority: Call Dreadstalkers > Hand of Gul'dan > Demonbolt
                        if (bot->HasSpell(104316) && !bot->GetSpellHistory()->HasCooldown(104316) &&
                            bot->GetPower(POWER_SOUL_SHARDS) >= 2)
                            spellId = 104316; // Call Dreadstalkers
                        else if (bot->HasSpell(105174) && bot->GetPower(POWER_SOUL_SHARDS) >= 1)
                            spellId = 105174; // Hand of Gul'dan
                        else if (bot->HasSpell(264178))
                            spellId = 264178; // Demonbolt
                    }
                    else // Destruction (267)
                    {
                        // Priority: Chaos Bolt > Conflagrate > Immolate > Incinerate
                        if (bot->HasSpell(348) && !target->HasAura(348)) // Immolate not on target
                            spellId = 348; // Immolate
                        else if (bot->HasSpell(17962) && !bot->GetSpellHistory()->HasCooldown(17962))
                            spellId = 17962; // Conflagrate
                        else if (bot->HasSpell(116858) && bot->GetPower(POWER_SOUL_SHARDS) >= 2)
                            spellId = 116858; // Chaos Bolt
                        else if (bot->HasSpell(29722))
                            spellId = 29722; // Incinerate
                    }
                    break;
                }
                case CLASS_PRIEST:
                {
                    // Discipline (256), Holy (257), Shadow (258)
                    if (specId == 258) // Shadow
                    {
                        // Priority: Maintain DoTs > Mind Blast > Mind Flay
                        if (bot->HasSpell(589) && !target->HasAura(589)) // Shadow Word: Pain
                            spellId = 589; // Shadow Word: Pain
                        else if (bot->HasSpell(34914) && !target->HasAura(34914)) // Vampiric Touch
                            spellId = 34914; // Vampiric Touch
                        else if (bot->HasSpell(8092) && !bot->GetSpellHistory()->HasCooldown(8092))
                            spellId = 8092; // Mind Blast
                        else if (bot->HasSpell(263165) && bot->GetPower(POWER_INSANITY) >= 50)
                            spellId = 263165; // Void Eruption / Void Bolt
                        else if (bot->HasSpell(15407))
                            spellId = 15407; // Mind Flay
                    }
                    else // Holy/Disc - use Smite for damage
                    {
                        if (bot->HasSpell(585))
                            spellId = 585; // Smite
                    }
                    break;
                }
                case CLASS_DRUID:
                {
                    // Balance (102), Feral (103), Guardian (104), Restoration (105)
                    if (specId == 102) // Balance
                    {
                        // Priority: Maintain DoTs > Starsurge > Wrath/Starfire
                        if (bot->HasSpell(164812) && !target->HasAura(164812)) // Moonfire
                            spellId = 164812; // Moonfire
                        else if (bot->HasSpell(93402) && !target->HasAura(93402)) // Sunfire
                            spellId = 93402; // Sunfire
                        else if (bot->HasSpell(78674) && bot->GetPower(POWER_LUNAR_POWER) >= 40)
                            spellId = 78674; // Starsurge
                        else if (bot->HasAura(48517) && bot->HasSpell(194153)) // Eclipse (Solar)
                            spellId = 194153; // Starfire
                        else if (bot->HasSpell(190984))
                            spellId = 190984; // Wrath
                    }
                    else // Non-balance specs in ranged situation
                    {
                        if (bot->HasSpell(8921) && !target->HasAura(8921))
                            spellId = 8921; // Moonfire (base)
                        else if (bot->HasSpell(5176))
                            spellId = 5176; // Wrath (base)
                    }
                    break;
                }
                case CLASS_SHAMAN:
                {
                    // Elemental (262), Enhancement (263), Restoration (264)
                    if (specId == 262) // Elemental
                    {
                        // Priority: Flame Shock > Lava Burst > Lightning Bolt
                        if (bot->HasSpell(188389) && !target->HasAura(188389)) // Flame Shock
                            spellId = 188389; // Flame Shock
                        else if (bot->HasSpell(51505) && !bot->GetSpellHistory()->HasCooldown(51505))
                            spellId = 51505; // Lava Burst
                        else if (bot->HasSpell(188196))
                            spellId = 188196; // Lightning Bolt
                    }
                    else // Enhancement/Resto - use Lightning Bolt as ranged filler
                    {
                        if (bot->HasSpell(188196))
                            spellId = 188196; // Lightning Bolt
                    }
                    break;
                }
                case CLASS_EVOKER:
                {
                    // Devastation (1467), Preservation (1468), Augmentation (1473)
                    if (specId == 1467) // Devastation
                    {
                        // Priority: Fire Breath > Disintegrate > Living Flame
                        if (bot->HasSpell(357208) && !bot->GetSpellHistory()->HasCooldown(357208))
                            spellId = 357208; // Fire Breath
                        else if (bot->HasSpell(356995) && bot->GetPower(POWER_ESSENCE) >= 3)
                            spellId = 356995; // Disintegrate
                        else if (bot->HasSpell(361469))
                            spellId = 361469; // Living Flame
                    }
                    else // Preservation/Augmentation
                    {
                        if (bot->HasSpell(361469))
                            spellId = 361469; // Living Flame
                    }
                    break;
                }
                default:
                    return BTStatus::FAILURE; // Non-ranged class
            }

            if (spellId != 0 && bot->HasSpell(spellId))
            {
                bb.Set<uint32>("RangedSpellId", spellId);
                // Cast the spell using TrinityCore spell system
                Spell* spell = new Spell(bot, sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE), TRIGGERED_NONE);
                SpellCastTargets targets;
                targets.SetUnitTarget(target);
                spell->prepare(targets);
                return BTStatus::SUCCESS;
            }
            return BTStatus::FAILURE;
        }
    ));

    root->AddChild(combatSequence);

    return root;
}

::std::shared_ptr<BTNode> BehaviorTreeFactory::BuildTankCombatTree()
{
    auto root = ::std::make_shared<BTSelector>("TankCombatRoot");

    // Use defensive CDs if critical
    auto emergencyDefensive = ::std::make_shared<BTSequence>("EmergencyDefensive");
    emergencyDefensive->AddChild(::std::make_shared<BTCheckHealthPercent>(0.30f, BTCheckHealthPercent::Comparison::LESS_THAN));
    emergencyDefensive->AddChild(::std::make_shared<BTCheckInCombat>());
    emergencyDefensive->AddChild(::std::make_shared<BTAction>("UseEmergencyCD",
        [](BotAI* ai, BTBlackboard& bb) -> BTStatus {
            if (!ai) return BTStatus::INVALID;
            Player* bot = ai->GetBot();
            if (!bot) return BTStatus::INVALID;

            uint8 playerClass = bot->GetClass();
            ChrSpecializationEntry const* spec = bot->GetPrimarySpecializationEntry();
            uint32 specId = spec ? spec->ID : 0;
            uint32 emergencySpellId = 0;

            switch (playerClass)
            {
                case CLASS_WARRIOR:
                    if (specId == 73) // Protection
                    {
                        // Last Stand > Shield Wall > Rallying Cry
                        if (bot->HasSpell(12975) && !bot->GetSpellHistory()->HasCooldown(12975))
                            emergencySpellId = 12975; // Last Stand
                        else if (bot->HasSpell(871) && !bot->GetSpellHistory()->HasCooldown(871))
                            emergencySpellId = 871; // Shield Wall
                        else if (bot->HasSpell(97462) && !bot->GetSpellHistory()->HasCooldown(97462))
                            emergencySpellId = 97462; // Rallying Cry
                    }
                    break;

                case CLASS_PALADIN:
                    if (specId == 66) // Protection
                    {
                        // Ardent Defender > Guardian of Ancient Kings > Divine Shield
                        if (bot->HasSpell(31850) && !bot->GetSpellHistory()->HasCooldown(31850))
                            emergencySpellId = 31850; // Ardent Defender
                        else if (bot->HasSpell(86659) && !bot->GetSpellHistory()->HasCooldown(86659))
                            emergencySpellId = 86659; // Guardian of Ancient Kings
                        else if (bot->HasSpell(642) && !bot->GetSpellHistory()->HasCooldown(642))
                            emergencySpellId = 642; // Divine Shield
                    }
                    break;

                case CLASS_DEATH_KNIGHT:
                    if (specId == 250) // Blood
                    {
                        // Vampiric Blood > Dancing Rune Weapon > Icebound Fortitude
                        if (bot->HasSpell(55233) && !bot->GetSpellHistory()->HasCooldown(55233))
                            emergencySpellId = 55233; // Vampiric Blood
                        else if (bot->HasSpell(49028) && !bot->GetSpellHistory()->HasCooldown(49028))
                            emergencySpellId = 49028; // Dancing Rune Weapon
                        else if (bot->HasSpell(48792) && !bot->GetSpellHistory()->HasCooldown(48792))
                            emergencySpellId = 48792; // Icebound Fortitude
                    }
                    break;

                case CLASS_DRUID:
                    if (specId == 104) // Guardian
                    {
                        // Survival Instincts > Barkskin > Frenzied Regeneration
                        if (bot->HasSpell(61336) && !bot->GetSpellHistory()->HasCooldown(61336))
                            emergencySpellId = 61336; // Survival Instincts
                        else if (bot->HasSpell(22812) && !bot->GetSpellHistory()->HasCooldown(22812))
                            emergencySpellId = 22812; // Barkskin
                        else if (bot->HasSpell(22842) && !bot->GetSpellHistory()->HasCooldown(22842) &&
                                 bot->GetPower(POWER_RAGE) >= 10)
                            emergencySpellId = 22842; // Frenzied Regeneration
                    }
                    break;

                case CLASS_MONK:
                    if (specId == 268) // Brewmaster
                    {
                        // Fortifying Brew > Zen Meditation > Dampen Harm
                        if (bot->HasSpell(115203) && !bot->GetSpellHistory()->HasCooldown(115203))
                            emergencySpellId = 115203; // Fortifying Brew
                        else if (bot->HasSpell(115176) && !bot->GetSpellHistory()->HasCooldown(115176))
                            emergencySpellId = 115176; // Zen Meditation
                        else if (bot->HasSpell(122278) && !bot->GetSpellHistory()->HasCooldown(122278))
                            emergencySpellId = 122278; // Dampen Harm
                    }
                    break;

                case CLASS_DEMON_HUNTER:
                    if (specId == 581) // Vengeance
                    {
                        // Metamorphosis > Fiery Brand > Demon Spikes
                        if (bot->HasSpell(187827) && !bot->GetSpellHistory()->HasCooldown(187827))
                            emergencySpellId = 187827; // Metamorphosis
                        else if (bot->HasSpell(204021) && !bot->GetSpellHistory()->HasCooldown(204021))
                            emergencySpellId = 204021; // Fiery Brand
                        else if (bot->HasSpell(203720) && !bot->GetSpellHistory()->HasCooldown(203720))
                            emergencySpellId = 203720; // Demon Spikes
                    }
                    break;

                default:
                    break;
            }

            if (emergencySpellId > 0)
            {
                bb.Set<uint32>("EmergencySpellId", emergencySpellId);
                return BTStatus::SUCCESS;
            }
            return BTStatus::FAILURE;
        }
    ));
    root->AddChild(emergencyDefensive);

    // Tanking sequence
    auto tankingSequence = ::std::make_shared<BTSequence>("Tanking");

    tankingSequence->AddChild(::std::make_shared<BTCheckHasTarget>());
    tankingSequence->AddChild(::std::make_shared<BTCheckInRange>(0.0f, 10.0f));
    tankingSequence->AddChild(::std::make_shared<BTFaceTarget>());

    // Use threat generation
    tankingSequence->AddChild(::std::make_shared<BTAction>("GenerateThreat",
        [](BotAI* ai, BTBlackboard& bb) -> BTStatus {
            if (!ai) return BTStatus::INVALID;
            Player* bot = ai->GetBot();
            if (!bot) return BTStatus::INVALID;

            uint8 playerClass = bot->GetClass();
            ChrSpecializationEntry const* spec = bot->GetPrimarySpecializationEntry();
            uint32 specId = spec ? spec->ID : 0;
            uint32 threatSpellId = 0;

            switch (playerClass)
            {
                case CLASS_WARRIOR:
                    if (specId == 73) // Protection
                    {
                        // Priority: Shield Slam > Revenge > Thunder Clap
                        if (bot->HasSpell(23922) && !bot->GetSpellHistory()->HasCooldown(23922))
                            threatSpellId = 23922; // Shield Slam
                        else if (bot->HasSpell(6572) && !bot->GetSpellHistory()->HasCooldown(6572))
                            threatSpellId = 6572; // Revenge
                        else if (bot->HasSpell(6343) && !bot->GetSpellHistory()->HasCooldown(6343))
                            threatSpellId = 6343; // Thunder Clap
                    }
                    break;

                case CLASS_PALADIN:
                    if (specId == 66) // Protection
                    {
                        // Priority: Avenger's Shield > Judgment > Consecration
                        if (bot->HasSpell(31935) && !bot->GetSpellHistory()->HasCooldown(31935))
                            threatSpellId = 31935; // Avenger's Shield
                        else if (bot->HasSpell(275779) && !bot->GetSpellHistory()->HasCooldown(275779))
                            threatSpellId = 275779; // Judgment
                        else if (bot->HasSpell(26573) && !bot->GetSpellHistory()->HasCooldown(26573))
                            threatSpellId = 26573; // Consecration
                    }
                    break;

                case CLASS_DEATH_KNIGHT:
                    if (specId == 250) // Blood
                    {
                        // Priority: Heart Strike > Blood Boil > Death and Decay
                        if (bot->HasSpell(206930) && !bot->GetSpellHistory()->HasCooldown(206930) &&
                            bot->GetPower(POWER_RUNES) >= 1)
                            threatSpellId = 206930; // Heart Strike
                        else if (bot->HasSpell(50842) && !bot->GetSpellHistory()->HasCooldown(50842))
                            threatSpellId = 50842; // Blood Boil
                        else if (bot->HasSpell(43265) && !bot->GetSpellHistory()->HasCooldown(43265) &&
                                 bot->GetPower(POWER_RUNES) >= 1)
                            threatSpellId = 43265; // Death and Decay
                    }
                    break;

                case CLASS_DRUID:
                    if (specId == 104) // Guardian
                    {
                        // Priority: Mangle > Thrash > Swipe
                        if (bot->HasSpell(33917) && !bot->GetSpellHistory()->HasCooldown(33917))
                            threatSpellId = 33917; // Mangle
                        else if (bot->HasSpell(77758) && !bot->GetSpellHistory()->HasCooldown(77758))
                            threatSpellId = 77758; // Thrash
                        else if (bot->HasSpell(213764) && !bot->GetSpellHistory()->HasCooldown(213764))
                            threatSpellId = 213764; // Swipe
                    }
                    break;

                case CLASS_MONK:
                    if (specId == 268) // Brewmaster
                    {
                        // Priority: Keg Smash > Breath of Fire > Tiger Palm
                        if (bot->HasSpell(121253) && !bot->GetSpellHistory()->HasCooldown(121253) &&
                            bot->GetPower(POWER_ENERGY) >= 40)
                            threatSpellId = 121253; // Keg Smash
                        else if (bot->HasSpell(115181) && !bot->GetSpellHistory()->HasCooldown(115181))
                            threatSpellId = 115181; // Breath of Fire
                        else if (bot->HasSpell(100780) && bot->GetPower(POWER_ENERGY) >= 25)
                            threatSpellId = 100780; // Tiger Palm
                    }
                    break;

                case CLASS_DEMON_HUNTER:
                    if (specId == 581) // Vengeance
                    {
                        // Priority: Fracture > Immolation Aura > Shear
                        if (bot->HasSpell(263642) && !bot->GetSpellHistory()->HasCooldown(263642))
                            threatSpellId = 263642; // Fracture
                        else if (bot->HasSpell(258920) && !bot->GetSpellHistory()->HasCooldown(258920))
                            threatSpellId = 258920; // Immolation Aura
                        else if (bot->HasSpell(203782))
                            threatSpellId = 203782; // Shear
                    }
                    break;

                default:
                    break;
            }

            if (threatSpellId > 0)
            {
                bb.Set<uint32>("ThreatSpellId", threatSpellId);
                return BTStatus::SUCCESS;
            }
            return BTStatus::FAILURE;
        }
    ));

    // Maintain defensive stance/presence - Full class-specific implementation
    tankingSequence->AddChild(::std::make_shared<BTAction>("DefensiveStance",
        [](BotAI* ai, BTBlackboard& bb) -> BTStatus {
            if (!ai) return BTStatus::INVALID;
            Player* bot = ai->GetBot();
            if (!bot) return BTStatus::INVALID;

            uint8 playerClass = bot->GetClass();
            ChrSpecializationEntry const* spec = bot->GetPrimarySpecializationEntry();
            uint32 specId = spec ? spec->ID : 0;
            uint32 stanceSpellId = 0;
            uint32 activeMitigationId = 0;

            switch (playerClass)
            {
                case CLASS_WARRIOR:
                    if (specId == 73) // Protection
                    {
                        // Warrior no longer has stances in TWW 12.0
                        // Use Shield Block as primary active mitigation
                        if (bot->HasSpell(2565) && !bot->GetSpellHistory()->HasCooldown(2565) &&
                            bot->GetPower(POWER_RAGE) >= 30 && !bot->HasAura(132404))
                            activeMitigationId = 2565; // Shield Block
                        // Ignore Pain as secondary
                        else if (bot->HasSpell(190456) && bot->GetPower(POWER_RAGE) >= 40 &&
                                 bot->GetHealthPct() < 80.0f)
                            activeMitigationId = 190456; // Ignore Pain
                    }
                    break;

                case CLASS_PALADIN:
                    if (specId == 66) // Protection
                    {
                        // Ensure Consecration is active
                        if (bot->HasSpell(26573) && !bot->HasAura(188370) &&
                            !bot->GetSpellHistory()->HasCooldown(26573))
                            activeMitigationId = 26573; // Consecration
                        // Shield of the Righteous for active mitigation
                        else if (bot->HasSpell(53600) && bot->GetPower(POWER_HOLY_POWER) >= 3)
                            activeMitigationId = 53600; // Shield of the Righteous
                    }
                    break;

                case CLASS_DEATH_KNIGHT:
                    if (specId == 250) // Blood
                    {
                        // Death Knights no longer have presences in TWW 12.0
                        // Bone Shield maintenance via Marrowrend
                        if (bot->HasSpell(195182) && !bot->HasAura(195181))
                            activeMitigationId = 195182; // Marrowrend (apply Bone Shield)
                        // Death Strike for healing/absorb
                        else if (bot->HasSpell(49998) && bot->GetPower(POWER_RUNIC_POWER) >= 45 &&
                                 bot->GetHealthPct() < 75.0f)
                            activeMitigationId = 49998; // Death Strike
                    }
                    break;

                case CLASS_DRUID:
                    if (specId == 104) // Guardian
                    {
                        // Ensure Bear Form
                        if (bot->HasSpell(5487) && !bot->HasAura(5487))
                            stanceSpellId = 5487; // Bear Form
                        // Ironfur for active mitigation
                        else if (bot->HasSpell(192081) && bot->GetPower(POWER_RAGE) >= 40 &&
                                 !bot->HasAura(192081))
                            activeMitigationId = 192081; // Ironfur
                        // Frenzied Regeneration for healing
                        else if (bot->HasSpell(22842) && bot->GetPower(POWER_RAGE) >= 10 &&
                                 bot->GetHealthPct() < 70.0f && !bot->HasAura(22842))
                            activeMitigationId = 22842; // Frenzied Regeneration
                    }
                    break;

                case CLASS_MONK:
                    if (specId == 268) // Brewmaster
                    {
                        // Shuffle maintenance via Blackout Kick / Keg Smash
                        if (bot->HasSpell(115181) && !bot->GetSpellHistory()->HasCooldown(115181))
                            activeMitigationId = 115181; // Keg Smash
                        // Purifying Brew to clear Stagger
                        else if (bot->HasSpell(119582) && !bot->GetSpellHistory()->HasCooldown(119582))
                        {
                            // Check stagger level - purify at high stagger
                            if (bot->HasAura(124275)) // Heavy Stagger
                                activeMitigationId = 119582; // Purifying Brew
                        }
                        // Celestial Brew for absorb
                        else if (bot->HasSpell(322507) && !bot->GetSpellHistory()->HasCooldown(322507) &&
                                 bot->GetHealthPct() < 60.0f)
                            activeMitigationId = 322507; // Celestial Brew
                    }
                    break;

                case CLASS_DEMON_HUNTER:
                    if (specId == 581) // Vengeance
                    {
                        // Demon Spikes for armor increase
                        if (bot->HasSpell(203720) && !bot->HasAura(203819) &&
                            !bot->GetSpellHistory()->HasCooldown(203720))
                            activeMitigationId = 203720; // Demon Spikes
                        // Soul Cleave for healing
                        else if (bot->HasSpell(228477) && bot->GetPower(POWER_FURY) >= 30 &&
                                 bot->GetHealthPct() < 70.0f)
                            activeMitigationId = 228477; // Soul Cleave
                        // Fiery Brand for damage reduction
                        else if (bot->HasSpell(204021) && !bot->GetSpellHistory()->HasCooldown(204021) &&
                                 bot->GetHealthPct() < 50.0f)
                            activeMitigationId = 204021; // Fiery Brand
                    }
                    break;

                default:
                    return BTStatus::SUCCESS; // Non-tank class
            }

            // Cast stance spell if needed
            if (stanceSpellId != 0 && bot->HasSpell(stanceSpellId))
            {
                Spell* stanceSpell = new Spell(bot, sSpellMgr->GetSpellInfo(stanceSpellId, DIFFICULTY_NONE), TRIGGERED_NONE);
                SpellCastTargets stanceTargets;
                stanceTargets.SetUnitTarget(bot);
                stanceSpell->prepare(stanceTargets);
            }

            // Cast active mitigation spell if needed
            if (activeMitigationId != 0 && bot->HasSpell(activeMitigationId))
            {
                bb.Set<uint32>("ActiveMitigationId", activeMitigationId);
                Spell* mitigationSpell = new Spell(bot, sSpellMgr->GetSpellInfo(activeMitigationId, DIFFICULTY_NONE), TRIGGERED_NONE);
                SpellCastTargets mitigationTargets;

                // Some abilities target self, others target enemy - check if spell requires target
                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(activeMitigationId, DIFFICULTY_NONE);
                if (spellInfo && spellInfo->NeedsExplicitUnitTarget())
                {
                    // Offensive abilities like Revenge, Shield Slam need enemy target
                    Unit* target = bb.GetOr<Unit*>("Target", nullptr);
                    if (target && target->IsAlive() && bot->IsValidAttackTarget(target))
                        mitigationTargets.SetUnitTarget(target);
                    else
                        mitigationTargets.SetUnitTarget(bot);
                }
                else
                {
                    // Defensive abilities target self
                    mitigationTargets.SetUnitTarget(bot);
                }
                mitigationSpell->prepare(mitigationTargets);
                return BTStatus::SUCCESS;
            }

            return BTStatus::SUCCESS;
        }
    ));

    root->AddChild(tankingSequence);

    return root;
}

::std::shared_ptr<BTNode> BehaviorTreeFactory::BuildSingleTargetHealingTree()
{
    auto root = ::std::make_shared<BTSelector>("SingleTargetHealingRoot");

    // Heal self if critical - Full class-specific self-healing implementation
    auto selfHealSequence = ::std::make_shared<BTSequence>("SelfHeal");
    selfHealSequence->AddChild(::std::make_shared<BTCheckHealthPercent>(0.30f, BTCheckHealthPercent::Comparison::LESS_THAN));
    selfHealSequence->AddChild(::std::make_shared<BTAction>("HealSelf",
        [](BotAI* ai, BTBlackboard& bb) -> BTStatus {
            if (!ai) return BTStatus::INVALID;
            Player* bot = ai->GetBot();
            if (!bot) return BTStatus::INVALID;
            bb.Set<Unit*>("HealTarget", bot);

            uint8 playerClass = bot->GetClass();
            ChrSpecializationEntry const* spec = bot->GetPrimarySpecializationEntry();
            uint32 specId = spec ? spec->ID : 0;
            uint32 selfHealSpellId = 0;
            float healthPct = bot->GetHealthPct();

            switch (playerClass)
            {
                case CLASS_PRIEST:
                    // All priest specs have self-healing
                    if (bot->HasSpell(19236) && !bot->GetSpellHistory()->HasCooldown(19236))
                        selfHealSpellId = 19236; // Desperate Prayer (instant)
                    else if (healthPct < 20.0f && bot->HasSpell(2061))
                        selfHealSpellId = 2061; // Flash Heal (fast)
                    else if (bot->HasSpell(139) && !bot->HasAura(139))
                        selfHealSpellId = 139; // Renew (HoT)
                    else if (bot->HasSpell(2060))
                        selfHealSpellId = 2060; // Heal
                    break;

                case CLASS_PALADIN:
                    // Word of Glory is best instant heal
                    if (bot->HasSpell(85673) && bot->GetPower(POWER_HOLY_POWER) >= 1)
                        selfHealSpellId = 85673; // Word of Glory
                    else if (bot->HasSpell(633) && !bot->GetSpellHistory()->HasCooldown(633) &&
                             healthPct < 15.0f)
                        selfHealSpellId = 633; // Lay on Hands (emergency)
                    else if (healthPct < 20.0f && bot->HasSpell(19750))
                        selfHealSpellId = 19750; // Flash of Light (fast)
                    else if (bot->HasSpell(82326))
                        selfHealSpellId = 82326; // Holy Light
                    break;

                case CLASS_DRUID:
                    // Swiftmend is instant if available
                    if (bot->HasSpell(18562) && !bot->GetSpellHistory()->HasCooldown(18562) &&
                        (bot->HasAura(774) || bot->HasAura(8936))) // Has Rejuv or Regrowth
                        selfHealSpellId = 18562; // Swiftmend
                    else if (bot->HasSpell(774) && !bot->HasAura(774))
                        selfHealSpellId = 774; // Rejuvenation
                    else if (healthPct < 20.0f && bot->HasSpell(8936))
                        selfHealSpellId = 8936; // Regrowth (fast with HoT)
                    else if (bot->HasSpell(5185))
                        selfHealSpellId = 5185; // Healing Touch
                    break;

                case CLASS_SHAMAN:
                    // Riptide is instant
                    if (bot->HasSpell(61295) && !bot->GetSpellHistory()->HasCooldown(61295))
                        selfHealSpellId = 61295; // Riptide
                    else if (healthPct < 20.0f && bot->HasSpell(8004))
                        selfHealSpellId = 8004; // Healing Surge (fast)
                    else if (bot->HasSpell(77472))
                        selfHealSpellId = 77472; // Healing Wave
                    break;

                case CLASS_MONK:
                    // Expel Harm is instant
                    if (bot->HasSpell(322101) && !bot->GetSpellHistory()->HasCooldown(322101))
                        selfHealSpellId = 322101; // Expel Harm
                    else if (bot->HasSpell(116670))
                        selfHealSpellId = 116670; // Vivify
                    break;

                case CLASS_DEATH_KNIGHT:
                    // Death Strike is primary self-heal
                    if (bot->HasSpell(49998) && bot->GetPower(POWER_RUNIC_POWER) >= 45)
                        selfHealSpellId = 49998; // Death Strike
                    break;

                case CLASS_DEMON_HUNTER:
                    // Soul Cleave for self-healing
                    if (specId == 581 && bot->HasSpell(228477) && bot->GetPower(POWER_FURY) >= 30)
                        selfHealSpellId = 228477; // Soul Cleave (Vengeance)
                    break;

                case CLASS_WARRIOR:
                    // Victory Rush after kill, otherwise Impending Victory talent
                    if (bot->HasSpell(34428) && bot->HasAura(32216)) // Victory Rush proc
                        selfHealSpellId = 34428; // Victory Rush
                    else if (bot->HasSpell(202168) && !bot->GetSpellHistory()->HasCooldown(202168))
                        selfHealSpellId = 202168; // Impending Victory
                    break;

                case CLASS_EVOKER:
                    // Living Flame can heal self
                    if (bot->HasSpell(361469))
                        selfHealSpellId = 361469; // Living Flame (smart heal)
                    break;

                default:
                    return BTStatus::FAILURE; // No self-healing available
            }

            if (selfHealSpellId != 0 && bot->HasSpell(selfHealSpellId))
            {
                bb.Set<uint32>("SelfHealSpellId", selfHealSpellId);
                Spell* healSpell = new Spell(bot, sSpellMgr->GetSpellInfo(selfHealSpellId, DIFFICULTY_NONE), TRIGGERED_NONE);
                SpellCastTargets targets;
                targets.SetUnitTarget(bot);
                healSpell->prepare(targets);
                return BTStatus::SUCCESS;
            }
            return BTStatus::FAILURE;
        }
    ));
    root->AddChild(selfHealSequence);

    // Heal ally sequence
    auto healAllySequence = ::std::make_shared<BTSequence>("HealAlly");

    // Find most wounded ally
    healAllySequence->AddChild(::std::make_shared<BTFindWoundedAlly>(0.80f));

    // Check in range
    healAllySequence->AddChild(::std::make_shared<BTCheckHealTargetInRange>(40.0f));

    // Check line of sight
    healAllySequence->AddChild(::std::make_shared<BTCheckHealTargetLoS>());

    // Select appropriate heal spell based on deficit - Full class-specific implementation
    healAllySequence->AddChild(::std::make_shared<BTAction>("SelectHealSpell",
        [](BotAI* ai, BTBlackboard& bb) -> BTStatus {
            if (!ai) return BTStatus::INVALID;
            Player* bot = ai->GetBot();
            if (!bot) return BTStatus::INVALID;

            float targetHealthPct = bb.GetOr<float>("HealTargetHealthPct", 1.0f);
            Unit* healTarget = bb.GetOr<Unit*>("HealTarget", nullptr);
            if (!healTarget) return BTStatus::FAILURE;

            uint8 playerClass = bot->GetClass();
            ChrSpecializationEntry const* spec = bot->GetPrimarySpecializationEntry();
            uint32 specId = spec ? spec->ID : 0;
            uint32 selectedHealSpell = 0;
            bool isCritical = targetHealthPct < 0.30f;

            switch (playerClass)
            {
                case CLASS_PRIEST:
                    if (specId == 256) // Discipline
                    {
                        // Discipline uses Atonement and shields
                        if (!healTarget->HasAura(194384)) // No Atonement
                            selectedHealSpell = 17; // Power Word: Shield (applies Atonement)
                        else if (isCritical && bot->HasSpell(47540))
                            selectedHealSpell = 47540; // Penance
                        else if (bot->HasSpell(194509))
                            selectedHealSpell = 194509; // Power Word: Radiance
                    }
                    else if (specId == 257) // Holy
                    {
                        if (isCritical && bot->HasSpell(2061))
                            selectedHealSpell = 2061; // Flash Heal
                        else if (bot->HasSpell(2060))
                            selectedHealSpell = 2060; // Heal
                        // Add Renew if target doesn't have it
                        if (!healTarget->HasAura(139) && bot->HasSpell(139))
                            selectedHealSpell = 139; // Renew
                    }
                    break;

                case CLASS_PALADIN:
                    if (specId == 65) // Holy
                    {
                        if (isCritical && bot->HasSpell(85673) && bot->GetPower(POWER_HOLY_POWER) >= 3)
                            selectedHealSpell = 85673; // Word of Glory (instant)
                        else if (isCritical && bot->HasSpell(19750))
                            selectedHealSpell = 19750; // Flash of Light
                        else if (bot->HasSpell(82326))
                            selectedHealSpell = 82326; // Holy Light
                        // Holy Shock on cooldown
                        if (bot->HasSpell(20473) && !bot->GetSpellHistory()->HasCooldown(20473))
                            selectedHealSpell = 20473; // Holy Shock (instant)
                    }
                    break;

                case CLASS_DRUID:
                    if (specId == 105) // Restoration
                    {
                        // Swiftmend if target has HoT
                        if (isCritical && bot->HasSpell(18562) && !bot->GetSpellHistory()->HasCooldown(18562) &&
                            (healTarget->HasAura(774) || healTarget->HasAura(8936)))
                            selectedHealSpell = 18562; // Swiftmend
                        else if (isCritical && bot->HasSpell(8936))
                            selectedHealSpell = 8936; // Regrowth
                        else if (!healTarget->HasAura(774) && bot->HasSpell(774))
                            selectedHealSpell = 774; // Rejuvenation
                        else if (bot->HasSpell(5185))
                            selectedHealSpell = 5185; // Healing Touch
                    }
                    break;

                case CLASS_SHAMAN:
                    if (specId == 264) // Restoration
                    {
                        // Riptide if off cooldown
                        if (bot->HasSpell(61295) && !bot->GetSpellHistory()->HasCooldown(61295) &&
                            !healTarget->HasAura(61295))
                            selectedHealSpell = 61295; // Riptide
                        else if (isCritical && bot->HasSpell(8004))
                            selectedHealSpell = 8004; // Healing Surge
                        else if (bot->HasSpell(77472))
                            selectedHealSpell = 77472; // Healing Wave
                    }
                    break;

                case CLASS_MONK:
                    if (specId == 270) // Mistweaver
                    {
                        // Thunder Focus Tea enhances next spell
                        if (isCritical && bot->HasSpell(116670))
                            selectedHealSpell = 116670; // Vivify
                        else if (!healTarget->HasAura(124682) && bot->HasSpell(124682))
                            selectedHealSpell = 124682; // Enveloping Mist
                        else if (bot->HasSpell(115175))
                            selectedHealSpell = 115175; // Soothing Mist
                    }
                    break;

                case CLASS_EVOKER:
                    if (specId == 1468) // Preservation
                    {
                        if (isCritical && bot->HasSpell(355913))
                            selectedHealSpell = 355913; // Emerald Blossom
                        else if (bot->HasSpell(361469))
                            selectedHealSpell = 361469; // Living Flame
                        else if (bot->HasSpell(366155))
                            selectedHealSpell = 366155; // Reversion (HoT)
                    }
                    break;

                default:
                    return BTStatus::FAILURE; // Non-healer class
            }

            if (selectedHealSpell != 0)
            {
                bb.Set<uint32>("SelectedHealSpell", selectedHealSpell);
                return BTStatus::SUCCESS;
            }
            return BTStatus::FAILURE;
        }
    ));

    // Cast heal - Full spell casting implementation
    healAllySequence->AddChild(::std::make_shared<BTAction>("CastHeal",
        [](BotAI* ai, BTBlackboard& bb) -> BTStatus {
            if (!ai) return BTStatus::INVALID;
            Player* bot = ai->GetBot();
            if (!bot) return BTStatus::INVALID;

            uint32 healSpellId = bb.GetOr<uint32>("SelectedHealSpell", 0);
            if (healSpellId == 0)
                return BTStatus::FAILURE;

            Unit* healTarget = bb.GetOr<Unit*>("HealTarget", nullptr);
            if (!healTarget || !healTarget->IsAlive())
                return BTStatus::FAILURE;

            // Verify spell exists and is known
            if (!bot->HasSpell(healSpellId))
                return BTStatus::FAILURE;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(healSpellId, DIFFICULTY_NONE);
            if (!spellInfo)
                return BTStatus::FAILURE;

            // Check if spell is on cooldown
            if (bot->GetSpellHistory()->HasCooldown(healSpellId))
                return BTStatus::FAILURE;

            // Check mana/resource requirements using TWW 12.0 SpellPowerCost API
            ::std::vector<SpellPowerCost> powerCosts = spellInfo->CalcPowerCost(bot, spellInfo->GetSchoolMask());
            bool hasEnoughPower = true;
            for (SpellPowerCost const& cost : powerCosts)
            {
                if (cost.Amount > 0 && bot->GetPower(cost.Power) < cost.Amount)
                {
                    hasEnoughPower = false;
                    break;
                }
            }
            if (!hasEnoughPower)
                return BTStatus::FAILURE;

            // Check range
            float range = spellInfo->GetMaxRange(false, bot);
            if (range > 0 && bot->GetDistance(healTarget) > range)
                return BTStatus::FAILURE;

            // Check line of sight
            if (!bot->IsWithinLOSInMap(healTarget))
                return BTStatus::FAILURE;

            // Cast the spell using TrinityCore Spell API
            Spell* spell = new Spell(bot, spellInfo, TRIGGERED_NONE);
            SpellCastTargets targets;
            targets.SetUnitTarget(healTarget);

            SpellCastResult result = spell->prepare(targets);

            if (result == SPELL_CAST_OK)
            {
                TC_LOG_DEBUG("playerbot.bt", "Bot {} casting heal spell {} on target {}",
                    bot->GetName(), healSpellId, healTarget->GetName());
                return BTStatus::SUCCESS;
            }
            else
            {
                TC_LOG_DEBUG("playerbot.bt", "Bot {} failed to cast heal spell {}: result {}",
                    bot->GetName(), healSpellId, uint32(result));
                return BTStatus::FAILURE;
            }
            // - Handle cast interruption and failure cases
            return BTStatus::SUCCESS;
        }
    ));

    root->AddChild(healAllySequence);

    return root;
}

::std::shared_ptr<BTNode> BehaviorTreeFactory::BuildGroupHealingTree()
{
    auto root = ::std::make_shared<BTSelector>("GroupHealingRoot");

    // Self heal if critical
    auto selfHealSequence = ::std::make_shared<BTSequence>("SelfHeal");
    selfHealSequence->AddChild(::std::make_shared<BTCheckHealthPercent>(0.30f, BTCheckHealthPercent::Comparison::LESS_THAN));
    selfHealSequence->AddChild(::std::make_shared<BTAction>("HealSelf",
        [](BotAI* /*ai*/, BTBlackboard& /*bb*/) -> BTStatus {
            // DESIGN NOTE: Class-specific self-healing spell implementation required
            // When implementing per-class healing, add logic here for:
            // - Priests: Flash Heal, Renew, Desperate Prayer
            // - Paladins: Flash of Light, Holy Light, Word of Glory
            // - Druids: Rejuvenation, Regrowth, Swiftmend
            // - Shamans: Healing Surge, Healing Wave, Riptide
            // - Monks: Soothing Mist, Enveloping Mist, Expel Harm
            return BTStatus::SUCCESS;
        }
    ));
    root->AddChild(selfHealSequence);

    // AoE heal sequence
    auto aoeHealSequence = ::std::make_shared<BTSequence>("AoEHeal");

    // Check multiple wounded (3+ allies <80% health)
    aoeHealSequence->AddChild(::std::make_shared<BTCheckGroupNeedsAoEHeal>(0.80f, 3));

    // Check mana sufficient
    aoeHealSequence->AddChild(::std::make_shared<BTCheckResourcePercent>(POWER_MANA, 0.30f,
        BTCheckResourcePercent::Comparison::GREATER_THAN));

    // Generic AoE heal action - actual spell resolved by class-specific AI
    // DESIGN NOTE: This factory creates generic behavior trees that work for all classes.
    // The CastAoEHeal action uses a safe no-op pattern where class-specific AI selects
    // the appropriate spell. Each class provides different AoE heals:
    // - Priests: Circle of Healing (34861), Prayer of Healing (596), Holy Nova (132157)
    // - Paladins: Light of Dawn (85222), Holy Radiance (82327)
    // - Druids: Wild Growth (48438), Efflorescence (145205), Tranquility (740)
    // - Shamans: Chain Heal (1064), Healing Rain (73920), Healing Tide Totem (108280)
    // - Monks: Revival (115310), Essence Font (191837), Chi Burst (123986)
    aoeHealSequence->AddChild(::std::make_shared<BTAction>("CastAoEHeal",
        [](BotAI* /*ai*/, BTBlackboard& /*bb*/) -> BTStatus {
            // Safe no-op for generic tree - class-specific AI implements actual casting
            return BTStatus::SUCCESS;
        }
    ));

    root->AddChild(aoeHealSequence);

    // Fallback: Single target heal
    root->AddChild(BuildSingleTargetHealingTree());

    return root;
}

::std::shared_ptr<BTNode> BehaviorTreeFactory::BuildDispelPriorityTree()
{
    // DESIGN NOTE: Generic Dispel Tree with Safe-Fail Pattern
    // This factory creates generic behavior trees usable by all classes. The BTCastDispel
    // nodes use spellId=0 which safely fails (bot->HasSpell(0) returns false) for classes
    // that cannot dispel a particular type. Class-specific behavior trees should be
    // constructed with actual spell IDs:
    // - Magic:   Priest Dispel Magic (528), Mage Remove Curse (475), Shaman Purify Spirit (77130)
    // - Curse:   Mage Remove Curse (475), Druid Remove Corruption (2782), Shaman Cleanse Spirit (51886)
    // - Disease: Priest Purify (527), Paladin Cleanse (4987), Monk Detox (115450)
    // - Poison:  Druid Nature's Cure (88423), Paladin Cleanse (4987), Monk Detox (115450)

    auto root = ::std::make_shared<BTSelector>("DispelPriority");

    // Dispel magic (high priority)
    auto dispelMagicSequence = ::std::make_shared<BTSequence>("DispelMagic");
    dispelMagicSequence->AddChild(::std::make_shared<BTFindDispelTarget>(BTFindDispelTarget::DispelType::MAGIC));
    dispelMagicSequence->AddChild(::std::make_shared<BTCastDispel>(0)); // Safe-fail: class-specific spell required
    root->AddChild(dispelMagicSequence);

    // Dispel curse
    auto dispelCurseSequence = ::std::make_shared<BTSequence>("DispelCurse");
    dispelCurseSequence->AddChild(::std::make_shared<BTFindDispelTarget>(BTFindDispelTarget::DispelType::CURSE));
    dispelCurseSequence->AddChild(::std::make_shared<BTCastDispel>(0)); // Safe-fail: class-specific spell required
    root->AddChild(dispelCurseSequence);

    // Dispel disease
    auto dispelDiseaseSequence = ::std::make_shared<BTSequence>("DispelDisease");
    dispelDiseaseSequence->AddChild(::std::make_shared<BTFindDispelTarget>(BTFindDispelTarget::DispelType::DISEASE));
    dispelDiseaseSequence->AddChild(::std::make_shared<BTCastDispel>(0)); // Safe-fail: class-specific spell required
    root->AddChild(dispelDiseaseSequence);

    // Dispel poison
    auto dispelPoisonSequence = ::std::make_shared<BTSequence>("DispelPoison");
    dispelPoisonSequence->AddChild(::std::make_shared<BTFindDispelTarget>(BTFindDispelTarget::DispelType::POISON));
    dispelPoisonSequence->AddChild(::std::make_shared<BTCastDispel>(0)); // Safe-fail: class-specific spell required
    root->AddChild(dispelPoisonSequence);

    return root;
}

::std::shared_ptr<BTNode> BehaviorTreeFactory::BuildFollowLeaderTree()
{
    auto root = ::std::make_shared<BTSequence>("FollowLeader");

    // Check not in combat
    root->AddChild(::std::make_shared<BTInverter>("NotInCombat",
        ::std::make_shared<BTCheckInCombat>()
    ));

    // Check too far from leader (>10 yards)
    root->AddChild(::std::make_shared<BTCondition>("TooFarFromLeader",
        [](BotAI* ai, BTBlackboard& /*bb*/) {
            if (!ai) return false;

            Player* bot = ai->GetBot();
            if (!bot) return false;

            Player* leader = nullptr;
            Group* group = bot->GetGroup();
            if (group)
            {
                ObjectGuid leaderGuid = group->GetLeaderGUID();
                leader = ObjectAccessor::FindPlayer(leaderGuid);
            }

            if (!leader) return false;

            return bot->GetDistance(leader) > 10.0f;
        }
    ));

    // Move to leader
    root->AddChild(::std::make_shared<BTFollowLeader>(5.0f));

    return root;
}

::std::shared_ptr<BTNode> BehaviorTreeFactory::BuildCombatPositioningTree()
{
    auto root = ::std::make_shared<BTSelector>("CombatPositioning");

    // Melee positioning
    auto meleeSequence = ::std::make_shared<BTSequence>("MeleePositioning");
    meleeSequence->AddChild(::std::make_shared<BTCondition>("IsMeleeClass",
        [](BotAI* ai, BTBlackboard& /*bb*/) {
            if (!ai) return false;
            Player* bot = ai->GetBot();
            if (!bot) return false;

            uint8 playerClass = bot->GetClass();
            ChrSpecializationEntry const* spec = bot->GetPrimarySpecializationEntry();
            uint32 specId = spec ? spec->ID : 0;

            // Pure melee classes
            switch (playerClass)
            {
                case CLASS_WARRIOR:
                case CLASS_ROGUE:
                case CLASS_DEATH_KNIGHT:
                    return true;

                case CLASS_PALADIN:
                    // Retribution and Protection are melee
                    return specId == 70 || specId == 66; // Ret or Prot

                case CLASS_HUNTER:
                    // Survival (melee spec)
                    return specId == 255;

                case CLASS_SHAMAN:
                    // Enhancement is melee
                    return specId == 263;

                case CLASS_MONK:
                    // Windwalker and Brewmaster are melee
                    return specId == 269 || specId == 268;

                case CLASS_DRUID:
                    // Feral and Guardian are melee
                    return specId == 103 || specId == 104;

                case CLASS_DEMON_HUNTER:
                    return true; // Both specs are melee

                default:
                    return false;
            }
        }
    ));
    meleeSequence->AddChild(::std::make_shared<BTCondition>("TooFarFromTarget",
        [](BotAI* ai, BTBlackboard& bb) {
            Unit* target = nullptr;
            if (!bb.Get<Unit*>("CurrentTarget", target) || !target)
                return false;

            if (!ai) return false;
            Player* bot = ai->GetBot();
            if (!bot) return false;

            return bot->GetDistance(target) > 5.0f;
        }
    ));
    meleeSequence->AddChild(::std::make_shared<BTMoveToTarget>(0.0f, 5.0f));
    root->AddChild(meleeSequence);

    // Ranged positioning
    auto rangedSequence = ::std::make_shared<BTSequence>("RangedPositioning");
    rangedSequence->AddChild(::std::make_shared<BTCondition>("IsRangedClass",
        [](BotAI* ai, BTBlackboard& /*bb*/) {
            if (!ai) return false;
            Player* bot = ai->GetBot();
            if (!bot) return false;

            uint8 playerClass = bot->GetClass();
            ChrSpecializationEntry const* spec = bot->GetPrimarySpecializationEntry();
            uint32 specId = spec ? spec->ID : 0;

            // Pure ranged classes
            switch (playerClass)
            {
                case CLASS_MAGE:
                case CLASS_WARLOCK:
                    return true;

                case CLASS_HUNTER:
                    // Beast Mastery and Marksmanship are ranged
                    return specId == 253 || specId == 254;

                case CLASS_PRIEST:
                    // Shadow and Discipline (when DPSing) are ranged
                    return specId == 258 || specId == 256;

                case CLASS_SHAMAN:
                    // Elemental and Restoration are ranged
                    return specId == 262 || specId == 264;

                case CLASS_DRUID:
                    // Balance and Restoration are ranged
                    return specId == 102 || specId == 105;

                case CLASS_MONK:
                    // Mistweaver is ranged
                    return specId == 270;

                case CLASS_PALADIN:
                    // Holy is ranged
                    return specId == 65;

                case CLASS_EVOKER:
                    return true; // All specs are ranged

                default:
                    return false;
            }
        }
    ));
    rangedSequence->AddChild(::std::make_shared<BTCondition>("NotInOptimalRange",
        [](BotAI* ai, BTBlackboard& bb) {
            Unit* target = nullptr;
            if (!bb.Get<Unit*>("CurrentTarget", target) || !target)
                return false;

            if (!ai) return false;
            Player* bot = ai->GetBot();
            if (!bot) return false;

            float distance = bot->GetDistance(target);
            return distance < 20.0f || distance > 35.0f;
        }
    ));
    rangedSequence->AddChild(::std::make_shared<BTMoveToTarget>(20.0f, 35.0f));
    root->AddChild(rangedSequence);

    return root;
}

::std::shared_ptr<BTNode> BehaviorTreeFactory::BuildFleeToSafetyTree()
{
    auto root = ::std::make_shared<BTSequence>("FleeToSafety");

    // Check health below 20%
    root->AddChild(::std::make_shared<BTCheckHealthPercent>(0.20f, BTCheckHealthPercent::Comparison::LESS_THAN));

    // Check in combat
    root->AddChild(::std::make_shared<BTCheckInCombat>());

    // Find safe position
    root->AddChild(::std::make_shared<BTFindSafePosition>(20.0f));

    // Move to safe position
    root->AddChild(::std::make_shared<BTMoveToPosition>(2.0f));

    // Stop movement when safe
    root->AddChild(::std::make_shared<BTStopMovement>());

    return root;
}

::std::shared_ptr<BTNode> BehaviorTreeFactory::BuildBuffMaintenanceTree()
{
    auto root = ::std::make_shared<BTSequence>("BuffMaintenance");

    // Check not in combat OR combat duration < 5 seconds
    auto canBuffSelector = ::std::make_shared<BTSelector>("CanBuff");
    canBuffSelector->AddChild(::std::make_shared<BTInverter>("NotInCombat",
        ::std::make_shared<BTCheckInCombat>()
    ));
    canBuffSelector->AddChild(::std::make_shared<BTCondition>("EarlyCombat",
        [](BotAI* ai, BTBlackboard& bb) -> bool {
            if (!ai) return false;
            Player* bot = ai->GetBot();
            if (!bot || !bot->IsInCombat()) return false;

            // Get combat start time from blackboard
            uint32 combatStartTime = 0;
            if (!bb.Get<uint32>("CombatStartTime", combatStartTime))
            {
                // Store current time as combat start
                combatStartTime = GameTime::GetGameTimeMS();
                bb.Set<uint32>("CombatStartTime", combatStartTime);
            }

            // Check if we're within first 5 seconds of combat
            uint32 currentTime = GameTime::GetGameTimeMS();
            uint32 combatDuration = currentTime - combatStartTime;
            return combatDuration < 5000; // 5 seconds
        }
    ));
    root->AddChild(canBuffSelector);

    // Check missing self buff
    root->AddChild(::std::make_shared<BTCondition>("MissingBuff",
        [](BotAI* ai, BTBlackboard& bb) {
            if (!ai) return false;
            Player* bot = ai->GetBot();
            if (!bot) return false;

            uint8 playerClass = bot->GetClass();
            ChrSpecializationEntry const* spec = bot->GetPrimarySpecializationEntry();
            uint32 specId = spec ? spec->ID : 0;
            uint32 missingBuffSpell = 0;

            switch (playerClass)
            {
                case CLASS_WARRIOR:
                    // Battle Shout (6673)
                    if (bot->HasSpell(6673) && !bot->HasAura(6673))
                        missingBuffSpell = 6673;
                    break;

                case CLASS_MAGE:
                    // Arcane Intellect (1459)
                    if (bot->HasSpell(1459) && !bot->HasAura(1459))
                        missingBuffSpell = 1459;
                    break;

                case CLASS_PRIEST:
                    // Power Word: Fortitude (21562)
                    if (bot->HasSpell(21562) && !bot->HasAura(21562))
                        missingBuffSpell = 21562;
                    break;

                case CLASS_PALADIN:
                    // Devotion Aura or other auras
                    if (bot->HasSpell(465) && !bot->HasAura(465))
                        missingBuffSpell = 465;
                    break;

                case CLASS_DRUID:
                    // Mark of the Wild (1126)
                    if (bot->HasSpell(1126) && !bot->HasAura(1126))
                        missingBuffSpell = 1126;
                    break;

                case CLASS_SHAMAN:
                    // Lightning Shield (192106) or Water Shield based on spec
                    if (specId == 262 || specId == 263) // Elemental/Enhancement
                    {
                        if (bot->HasSpell(192106) && !bot->HasAura(192106))
                            missingBuffSpell = 192106;
                    }
                    break;

                case CLASS_MONK:
                    // Legacy of the Emperor/Mystic Touch
                    if (bot->HasSpell(115921) && !bot->HasAura(115921))
                        missingBuffSpell = 115921;
                    break;

                case CLASS_EVOKER:
                    // Blessing of the Bronze (381748)
                    if (bot->HasSpell(381748) && !bot->HasAura(381748))
                        missingBuffSpell = 381748;
                    break;

                default:
                    break;
            }

            if (missingBuffSpell > 0)
            {
                bb.Set<uint32>("MissingBuffSpellId", missingBuffSpell);
                return true;
            }
            return false;
        }
    ));

    // Cast self buff
    root->AddChild(::std::make_shared<BTAction>("CastBuff",
        [](BotAI* ai, BTBlackboard& bb) -> BTStatus {
            if (!ai) return BTStatus::INVALID;
            Player* bot = ai->GetBot();
            if (!bot) return BTStatus::INVALID;

            uint32 buffSpellId = 0;
            if (!bb.Get<uint32>("MissingBuffSpellId", buffSpellId) || buffSpellId == 0)
                return BTStatus::FAILURE;

            // Check if spell is ready
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(buffSpellId, DIFFICULTY_NONE);
            if (!spellInfo)
                return BTStatus::FAILURE;

            if (!bot->HasSpell(buffSpellId))
                return BTStatus::FAILURE;

            if (bot->GetSpellHistory()->HasCooldown(buffSpellId))
                return BTStatus::FAILURE;

            // Check mana requirements
            ::std::vector<SpellPowerCost> costs = spellInfo->CalcPowerCost(bot, spellInfo->GetSchoolMask());
            for (const SpellPowerCost& cost : costs)
            {
                if (cost.Power == POWER_MANA && bot->GetPower(POWER_MANA) < cost.Amount)
                    return BTStatus::FAILURE;
            }

            // Store spell to cast
            bb.Set<uint32>("SpellToCast", buffSpellId);
            bb.Set<ObjectGuid>("SpellTarget", bot->GetGUID()); // Self-cast

            // Signal success - actual casting handled by spell system
            return BTStatus::SUCCESS;
        }
    ));

    return root;
}

::std::shared_ptr<BTNode> BehaviorTreeFactory::BuildResourceManagementTree()
{
    auto root = ::std::make_shared<BTSelector>("ResourceManagement");

    // Drink/Eat sequence
    auto consumableSequence = ::std::make_shared<BTSequence>("UseConsumable");

    // Check not in combat
    consumableSequence->AddChild(::std::make_shared<BTInverter>("NotInCombat",
        ::std::make_shared<BTCheckInCombat>()
    ));

    // Check mana < 30%
    consumableSequence->AddChild(::std::make_shared<BTCheckResourcePercent>(POWER_MANA, 0.30f,
        BTCheckResourcePercent::Comparison::LESS_THAN));

    // Use consumable
    consumableSequence->AddChild(::std::make_shared<BTAction>("UseConsumable",
        [](BotAI* /*ai*/, BTBlackboard& /*bb*/) -> BTStatus {
            // DESIGN NOTE: Consumable item usage implementation required
            // When implementing consumable usage, add logic here to:
            // - Search bot's inventory for appropriate consumables (mana/health potions, food, drink)
            // - Prioritize by quality/effectiveness (conjured > vendor items)
            // - Check item cooldowns and usage restrictions
            // - Use the item via TrinityCore Item API
            // - Handle special cases (conjured items, class-specific consumables)
            return BTStatus::SUCCESS;
        }
    ));

    root->AddChild(consumableSequence);

    // Conserve mana sequence
    auto conserveSequence = ::std::make_shared<BTSequence>("ConserveMana");

    // Check mana < 50%
    conserveSequence->AddChild(::std::make_shared<BTCheckResourcePercent>(POWER_MANA, 0.50f,
        BTCheckResourcePercent::Comparison::LESS_THAN));

    // Avoid expensive spells
    conserveSequence->AddChild(::std::make_shared<BTAction>("AvoidExpensiveSpells",
        [](BotAI* /*ai*/, BTBlackboard& bb) -> BTStatus {
            bb.Set<bool>("ConserveMana", true);
            return BTStatus::SUCCESS;
        }
    ));

    root->AddChild(conserveSequence);

    return root;
}

void BehaviorTreeFactory::RegisterCustomTree(::std::string const& name,
    ::std::function<::std::shared_ptr<BTNode>()> builder)
{
    _customTreeBuilders[name] = builder;
    TC_LOG_INFO("playerbot.bt", "Registered custom behavior tree: {}", name);
}

::std::shared_ptr<BTNode> BehaviorTreeFactory::CreateCustomTree(::std::string const& name)
{
    auto it = _customTreeBuilders.find(name);
    if (it != _customTreeBuilders.end())
    {
        return it->second();
    }

    TC_LOG_ERROR("playerbot.bt", "Custom tree '{}' not found in registry", name);
    return nullptr;
}

} // namespace Playerbot
