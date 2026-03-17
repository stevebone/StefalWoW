/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * WoW 12.0 Interrupt Database Implementation
 */

#include "InterruptDatabase.h"
#include "Player.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Log.h"
#include "GameTime.h"

namespace Playerbot
{

// Static member definitions
::std::unordered_map<uint32, SpellInterruptConfig> InterruptDatabase::_spellDatabase;
::std::unordered_map<uint32, ::std::unordered_map<uint32, InterruptPriority>> InterruptDatabase::_dungeonOverrides;
bool InterruptDatabase::_initialized = false;

void InterruptDatabase::Initialize()
{
    if (_initialized)
        return;

    LoadGeneralSpells();
    LoadDungeonSpells();
    LoadRaidSpells();
    LoadPvPSpells();
    LoadClassSpells();

    _initialized = true;
    TC_LOG_INFO("playerbot.interrupt", "Loaded {} interrupt configurations", _spellDatabase.size());
}

void InterruptDatabase::LoadGeneralSpells()
{
    // Healing spells - CRITICAL priority
    _spellDatabase[2061] = {2061, "Flash Heal", InterruptPriority::CRITICAL,
                           SpellCategory::HEAL_SINGLE, true, false, 0, 1.5f, false, 0x02, "Quick heal"};
    _spellDatabase[2060] = {2060, "Greater Heal", InterruptPriority::CRITICAL,
                           SpellCategory::HEAL_SINGLE, true, false, 0, 2.5f, false, 0x02, "Big heal"};
    _spellDatabase[596] = {596, "Prayer of Healing", InterruptPriority::CRITICAL,
                          SpellCategory::HEAL_GROUP, true, false, 0, 2.0f, false, 0x02, "Group heal"};
    _spellDatabase[48782] = {48782, "Holy Light", InterruptPriority::CRITICAL,
                            SpellCategory::HEAL_SINGLE, true, false, 0, 2.5f, false, 0x02, "Paladin heal"};
    _spellDatabase[82326] = {82326, "Divine Light", InterruptPriority::CRITICAL,
                            SpellCategory::HEAL_SINGLE, true, false, 0, 2.5f, false, 0x02, "Paladin big heal"};
    _spellDatabase[8936] = {8936, "Regrowth", InterruptPriority::HIGH,
                           SpellCategory::HEAL_SINGLE, false, false, 0, 1.5f, false, 0x08, "Druid heal"};
    _spellDatabase[5185] = {5185, "Healing Touch", InterruptPriority::HIGH,
                           SpellCategory::HEAL_SINGLE, false, false, 0, 2.5f, false, 0x08, "Druid heal"};

    // Crowd Control - CRITICAL priority
    _spellDatabase[118] = {118, "Polymorph", InterruptPriority::CRITICAL,
                          SpellCategory::CC_HARD, true, false, 0, 1.5f, true, 0x40, "Must interrupt"};
    _spellDatabase[51514] = {51514, "Hex", InterruptPriority::CRITICAL,
                            SpellCategory::CC_HARD, true, false, 0, 1.5f, true, 0x08, "Shaman CC"};
    _spellDatabase[5782] = {5782, "Fear", InterruptPriority::CRITICAL,
                           SpellCategory::CC_HARD, true, false, 0, 1.5f, true, 0x20, "Warlock fear"};
    _spellDatabase[605] = {605, "Mind Control", InterruptPriority::CRITICAL,
                          SpellCategory::CC_HARD, true, false, 0, 3.0f, true, 0x20, "Priest MC"};
    _spellDatabase[710] = {710, "Banish", InterruptPriority::HIGH,
                          SpellCategory::CC_HARD, false, false, 0, 1.5f, false, 0x20, "Warlock banish"};
    _spellDatabase[20066] = {20066, "Repentance", InterruptPriority::HIGH,
                            SpellCategory::CC_HARD, false, false, 0, 1.5f, false, 0x02, "Paladin CC"};

    // Major damage spells - HIGH priority
    _spellDatabase[116858] = {116858, "Chaos Bolt", InterruptPriority::HIGH,
                             SpellCategory::DAMAGE_NUKE, false, false, 0, 2.5f, false, 0x04, "Warlock nuke"};
    _spellDatabase[133] = {133, "Fireball", InterruptPriority::MODERATE,
                          SpellCategory::DAMAGE_NUKE, false, false, 0, 2.0f, false, 0x04, "Mage fireball"};
    _spellDatabase[11366] = {11366, "Pyroblast", InterruptPriority::HIGH,
                            SpellCategory::DAMAGE_NUKE, false, false, 0, 3.5f, false, 0x04, "Mage pyroblast"};
    _spellDatabase[203286] = {203286, "Greater Pyroblast", InterruptPriority::CRITICAL,
                             SpellCategory::DAMAGE_NUKE, true, false, 0, 4.0f, true, 0x04, "Must interrupt"};
    _spellDatabase[116] = {116, "Frostbolt", InterruptPriority::MODERATE,
                          SpellCategory::DAMAGE_NUKE, false, false, 0, 1.8f, false, 0x10, "Mage frostbolt"};
    _spellDatabase[30451] = {30451, "Arcane Blast", InterruptPriority::MODERATE,
                            SpellCategory::DAMAGE_NUKE, false, false, 0, 2.0f, false, 0x40, "Mage arcane"};

    // Channeled spells - MODERATE to HIGH
    _spellDatabase[5143] = {5143, "Arcane Missiles", InterruptPriority::MODERATE,
                           SpellCategory::DAMAGE_CHANNEL, false, false, 0, 0.0f, false, 0x40, "Channeled"};
    _spellDatabase[15407] = {15407, "Mind Flay", InterruptPriority::MODERATE,
                            SpellCategory::DAMAGE_CHANNEL, false, false, 0, 0.0f, false, 0x20, "Priest channel"};
    _spellDatabase[48181] = {48181, "Haunt", InterruptPriority::HIGH,
                            SpellCategory::DAMAGE_DOT, false, false, 0, 1.5f, false, 0x20, "Warlock haunt"};
    _spellDatabase[64843] = {64843, "Divine Hymn", InterruptPriority::CRITICAL,
                            SpellCategory::HEAL_GROUP, true, false, 0, 0.0f, true, 0x02, "Priest big heal"};
    _spellDatabase[64901] = {64901, "Hymn of Hope", InterruptPriority::HIGH,
                            SpellCategory::BUFF_UTILITY, false, false, 0, 0.0f, false, 0x02, "Mana channel"};

    // Defensive buffs - HIGH priority
    _spellDatabase[104773] = {104773, "Unending Resolve", InterruptPriority::HIGH,
                             SpellCategory::BUFF_DEFENSIVE, false, false, 0, 0.0f, false, 0x00, "Warlock def"};
    _spellDatabase[47788] = {47788, "Guardian Spirit", InterruptPriority::HIGH,
                            SpellCategory::BUFF_DEFENSIVE, false, false, 0, 0.0f, false, 0x02, "Priest save"};
    _spellDatabase[33206] = {33206, "Pain Suppression", InterruptPriority::HIGH,
                            SpellCategory::BUFF_DEFENSIVE, false, false, 0, 0.0f, false, 0x02, "Priest def"};

    // Resurrect spells - Context dependent
    _spellDatabase[2006] = {2006, "Resurrection", InterruptPriority::CRITICAL,
                           SpellCategory::RESURRECT, true, false, 0, 10.0f, false, 0x02, "Priest rez"};
    _spellDatabase[7328] = {7328, "Redemption", InterruptPriority::CRITICAL,
                           SpellCategory::RESURRECT, true, false, 0, 10.0f, false, 0x02, "Paladin rez"};
    _spellDatabase[50769] = {50769, "Revive", InterruptPriority::CRITICAL,
                            SpellCategory::RESURRECT, true, false, 0, 10.0f, false, 0x08, "Druid rez"};
    _spellDatabase[20484] = {20484, "Rebirth", InterruptPriority::CRITICAL,
                            SpellCategory::RESURRECT, true, false, 0, 2.0f, true, 0x08, "Druid brez"};
}

void InterruptDatabase::LoadDungeonSpells()
{
    using namespace CriticalSpells::Dungeons;

    // The Stonevault
    _spellDatabase[VOID_DISCHARGE] = {VOID_DISCHARGE, "Void Discharge", InterruptPriority::CRITICAL,
                                     SpellCategory::DAMAGE_AOE, true, false, 0, 2.5f, true, 0x20, "Stonevault"};
    _spellDatabase[SEISMIC_WAVE] = {SEISMIC_WAVE, "Seismic Wave", InterruptPriority::HIGH,
                                   SpellCategory::DAMAGE_AOE, false, true, 10, 3.0f, false, 0x08, "Stonevault M10+"};
    _spellDatabase[MOLTEN_MORTAR] = {MOLTEN_MORTAR, "Molten Mortar", InterruptPriority::HIGH,
                                    SpellCategory::DAMAGE_AOE, false, true, 7, 2.0f, false, 0x04, "Stonevault M7+"};

    // City of Threads
    _spellDatabase[UMBRAL_WEAVE] = {UMBRAL_WEAVE, "Umbral Weave", InterruptPriority::CRITICAL,
                                   SpellCategory::CC_HARD, true, false, 0, 2.0f, true, 0x20, "City of Threads"};
    _spellDatabase[DARK_BARRAGE] = {DARK_BARRAGE, "Dark Barrage", InterruptPriority::HIGH,
                                   SpellCategory::DAMAGE_CHANNEL, false, true, 5, 0.0f, false, 0x20, "City of Threads"};
    _spellDatabase[SHADOWY_DECAY] = {SHADOWY_DECAY, "Shadowy Decay", InterruptPriority::HIGH,
                                    SpellCategory::DEBUFF_DAMAGE, false, true, 10, 2.5f, false, 0x20, "City of Threads M10+"};

    // Ara-Kara, City of Echoes
    _spellDatabase[ECHOING_HOWL] = {ECHOING_HOWL, "Echoing Howl", InterruptPriority::CRITICAL,
                                   SpellCategory::CC_HARD, true, false, 0, 2.5f, true, 0x20, "Ara-Kara"};
    _spellDatabase[WEB_WRAP] = {WEB_WRAP, "Web Wrap", InterruptPriority::CRITICAL,
                               SpellCategory::CC_HARD, true, false, 0, 1.5f, true, 0x08, "Ara-Kara CC"};
    _spellDatabase[POISON_BOLT] = {POISON_BOLT, "Poison Bolt", InterruptPriority::MODERATE,
                                  SpellCategory::DAMAGE_NUKE, false, true, 7, 2.0f, false, 0x08, "Ara-Kara"};

    // The Dawnbreaker
    _spellDatabase[SHADOW_SHROUD] = {SHADOW_SHROUD, "Shadow Shroud", InterruptPriority::HIGH,
                                    SpellCategory::BUFF_DEFENSIVE, false, false, 0, 2.0f, false, 0x20, "Dawnbreaker"};
    _spellDatabase[ABYSSAL_BLAST] = {ABYSSAL_BLAST, "Abyssal Blast", InterruptPriority::CRITICAL,
                                    SpellCategory::DAMAGE_AOE, true, false, 0, 3.0f, true, 0x20, "Dawnbreaker"};
    _spellDatabase[DARK_ORB] = {DARK_ORB, "Dark Orb", InterruptPriority::HIGH,
                               SpellCategory::DAMAGE_NUKE, false, true, 10, 2.5f, false, 0x20, "Dawnbreaker M10+"};

    // Cinderbrew Meadery
    _spellDatabase[HONEY_MARINADE] = {HONEY_MARINADE, "Honey Marinade", InterruptPriority::HIGH,
                                     SpellCategory::DEBUFF_DAMAGE, false, true, 5, 2.0f, false, 0x08, "Cinderbrew"};
    _spellDatabase[CINDERBREW_TOSS] = {CINDERBREW_TOSS, "Cinderbrew Toss", InterruptPriority::MODERATE,
                                      SpellCategory::DAMAGE_AOE, false, false, 0, 2.5f, false, 0x04, "Cinderbrew"};

    // Darkflame Cleft
    _spellDatabase[SHADOW_VOLLEY] = {SHADOW_VOLLEY, "Shadow Volley", InterruptPriority::HIGH,
                                    SpellCategory::DAMAGE_AOE, false, true, 7, 2.5f, false, 0x20, "Darkflame"};
    _spellDatabase[DARK_EMPOWERMENT] = {DARK_EMPOWERMENT, "Dark Empowerment", InterruptPriority::CRITICAL,
                                       SpellCategory::BUFF_DAMAGE, true, false, 0, 2.0f, true, 0x20, "Darkflame buff"};

    // The Rookery
    _spellDatabase[TEMPEST] = {TEMPEST, "Tempest", InterruptPriority::HIGH,
                              SpellCategory::DAMAGE_AOE, false, true, 10, 3.0f, false, 0x08, "Rookery M10+"};
    _spellDatabase[LIGHTNING_TORRENT] = {LIGHTNING_TORRENT, "Lightning Torrent", InterruptPriority::HIGH,
                                        SpellCategory::DAMAGE_CHANNEL, false, true, 7, 0.0f, false, 0x08, "Rookery"};

    // Priory of the Sacred Flame
    _spellDatabase[HOLY_SMITE] = {HOLY_SMITE, "Holy Smite", InterruptPriority::MODERATE,
                                 SpellCategory::DAMAGE_NUKE, false, false, 0, 2.0f, false, 0x02, "Priory"};
    _spellDatabase[INNER_FLAME] = {INNER_FLAME, "Inner Flame", InterruptPriority::HIGH,
                                  SpellCategory::BUFF_DAMAGE, false, true, 10, 2.5f, false, 0x04, "Priory M10+"};
}

void InterruptDatabase::LoadRaidSpells()
{
    using namespace CriticalSpells::Raids;

    // Nerub-ar Palace
    _spellDatabase[VENOMOUS_RAIN] = {VENOMOUS_RAIN, "Venomous Rain", InterruptPriority::CRITICAL,
                                    SpellCategory::DAMAGE_AOE, true, false, 0, 3.0f, true, 0x08, "Nerub-ar Palace"};
    _spellDatabase[WEB_TERROR] = {WEB_TERROR, "Web Terror", InterruptPriority::CRITICAL,
                                 SpellCategory::CC_HARD, true, false, 0, 2.0f, true, 0x08, "Nerub-ar Palace"};
    _spellDatabase[SILKEN_TOMB] = {SILKEN_TOMB, "Silken Tomb", InterruptPriority::CRITICAL,
                                  SpellCategory::CC_HARD, true, false, 0, 2.5f, true, 0x08, "Nerub-ar Palace"};
    _spellDatabase[VOID_DEGENERATION] = {VOID_DEGENERATION, "Void Degeneration", InterruptPriority::CRITICAL,
                                        SpellCategory::DEBUFF_DAMAGE, true, false, 0, 3.0f, true, 0x20, "Nerub-ar Palace"};

    // Queen Ansurek
    _spellDatabase[REACTIVE_TOXIN] = {REACTIVE_TOXIN, "Reactive Toxin", InterruptPriority::CRITICAL,
                                     SpellCategory::DEBUFF_DAMAGE, true, false, 0, 2.5f, true, 0x08, "Queen Ansurek"};
    _spellDatabase[VENOM_NOVA] = {VENOM_NOVA, "Venom Nova", InterruptPriority::CRITICAL,
                                 SpellCategory::DAMAGE_AOE, true, false, 0, 3.0f, true, 0x08, "Queen Ansurek"};
    _spellDatabase[FEAST] = {FEAST, "Feast", InterruptPriority::CRITICAL,
                           SpellCategory::SPECIAL_MECHANIC, true, false, 0, 4.0f, true, 0x00, "Queen Ansurek wipe"};
    _spellDatabase[ABYSSAL_INFUSION] = {ABYSSAL_INFUSION, "Abyssal Infusion", InterruptPriority::HIGH,
                                       SpellCategory::BUFF_DAMAGE, false, false, 0, 2.5f, false, 0x20, "Queen Ansurek"};
}

void InterruptDatabase::LoadPvPSpells()
{
    using namespace CriticalSpells::PvP;

    _spellDatabase[GREATER_HEAL] = {GREATER_HEAL, "Greater Heal", InterruptPriority::CRITICAL,
                                   SpellCategory::HEAL_SINGLE, true, false, 0, 2.5f, true, 0x02, "PvP big heal"};
    _spellDatabase[CHAOS_BOLT] = {CHAOS_BOLT, "Chaos Bolt", InterruptPriority::HIGH,
                                 SpellCategory::DAMAGE_NUKE, false, false, 0, 2.5f, false, 0x04, "PvP warlock"};
    _spellDatabase[GREATER_PYROBLAST] = {GREATER_PYROBLAST, "Greater Pyroblast", InterruptPriority::CRITICAL,
                                        SpellCategory::DAMAGE_NUKE, true, false, 0, 4.0f, true, 0x04, "PvP mage"};
    _spellDatabase[CONVOKE_SPIRITS] = {CONVOKE_SPIRITS, "Convoke the Spirits", InterruptPriority::CRITICAL,
                                      SpellCategory::SPECIAL_MECHANIC, true, false, 0, 0.0f, true, 0x00, "Druid convoke"};
    _spellDatabase[DIVINE_HYMN] = {DIVINE_HYMN, "Divine Hymn", InterruptPriority::CRITICAL,
                                  SpellCategory::HEAL_GROUP, true, false, 0, 0.0f, true, 0x02, "Priest mass heal"};
}

void InterruptDatabase::LoadClassSpells()
{
    // Mythic+ Affix spells
    using namespace CriticalSpells::Affixes;

    _spellDatabase[INCORPOREAL_CAST] = {INCORPOREAL_CAST, "Incorporeal Being", InterruptPriority::CRITICAL,
                                       SpellCategory::SPECIAL_MECHANIC, true, true, 2, 10.0f, true, 0x00, "M+ Incorporeal"};
    _spellDatabase[AFFLICTED_CRY] = {AFFLICTED_CRY, "Afflicted Cry", InterruptPriority::CRITICAL,
                                    SpellCategory::SPECIAL_MECHANIC, true, true, 2, 0.0f, true, 0x00, "M+ Afflicted"};
    _spellDatabase[SPITEFUL_FIXATE] = {SPITEFUL_FIXATE, "Spiteful Fixation", InterruptPriority::LOW,
                                      SpellCategory::SPECIAL_MECHANIC, false, true, 2, 0.0f, false, 0x00, "M+ Spiteful"};
}

SpellInterruptConfig const* InterruptDatabase::GetSpellConfig(uint32 spellId)
{
    if (!_initialized)
        Initialize();

    auto it = _spellDatabase.find(spellId);
    return it != _spellDatabase.end() ? &it->second : nullptr;
}

InterruptPriority InterruptDatabase::GetSpellPriority(uint32 spellId, uint8 mythicLevel)
{
    if (!_initialized)
        Initialize();

    auto config = GetSpellConfig(spellId);
    if (!config)
        return InterruptPriority::IGNORE;

    // Check if this spell should be interrupted at this M+ level
    if (config->mythicPlusOnly && mythicLevel < config->minMythicLevel)
        return InterruptPriority::IGNORE;

    // Apply M+ scaling to priority if needed
    if (mythicLevel > 0)
        return MythicPlusInterruptScaling::AdjustPriorityForLevel(config->basePriority, mythicLevel);

    return config->basePriority;
}

bool InterruptDatabase::ShouldAlwaysInterrupt(uint32 spellId)
{
    auto config = GetSpellConfig(spellId);
    return config && config->alwaysInterrupt;
}

bool InterruptDatabase::IsQuickResponseRequired(uint32 spellId)
{
    auto config = GetSpellConfig(spellId);
    return config && config->requiresQuickResponse;
}

::std::vector<uint32> InterruptDatabase::GetCriticalSpells()
{
    if (!_initialized)
        Initialize();

    ::std::vector<uint32> criticalSpells;
    for (auto const& [spellId, config] : _spellDatabase)
    {
        if (config.basePriority == InterruptPriority::CRITICAL || config.alwaysInterrupt)
            criticalSpells.push_back(spellId);
    }
    return criticalSpells;
}

::std::vector<uint32> InterruptDatabase::GetSpellsByCategory(SpellCategory category)
{
    if (!_initialized)
        Initialize();

    ::std::vector<uint32> spells;
    for (auto const& [spellId, config] : _spellDatabase)
    {
        if (config.category == category)
            spells.push_back(spellId);
    }
    return spells;
}

// Class interrupt abilities implementation
namespace InterruptAbilities
{

::std::vector<ClassInterruptAbility> GetClassInterrupts(uint8 playerClass, uint32 spec)
{
    ::std::vector<ClassInterruptAbility> abilities;

    switch (playerClass)
    {
        case CLASS_DEATH_KNIGHT:
            abilities.push_back({MIND_FREEZE, "Mind Freeze", CLASS_DEATH_KNIGHT, 0,
                               InterruptMethod::SPELL_INTERRUPT, 15.0f, 15.0f, 3000, 0xFFFF,
                               false, 0, POWER_RUNIC_POWER, 0, false, 1});
            abilities.push_back({STRANGULATE, "Strangulate", CLASS_DEATH_KNIGHT, 0,
                               InterruptMethod::SILENCE, 30.0f, 120.0f, 5000, 0xFFFF,
                               false, 0, POWER_RUNES, 1, false, 1});
            abilities.push_back({ASPHYXIATE, "Asphyxiate", CLASS_DEATH_KNIGHT, 0,
                               InterruptMethod::STUN, 20.0f, 120.0f, 5000, 0,
                               false, 0, POWER_RUNES, 1, false, 1});
            break;

        case CLASS_DEMON_HUNTER:
            abilities.push_back({DISRUPT, "Disrupt", CLASS_DEMON_HUNTER, 0,
                               InterruptMethod::SPELL_INTERRUPT, 10.0f, 15.0f, 3000, 0xFFFF,
                               false, 0, POWER_FURY, 30, false, 1});
            abilities.push_back({CHAOS_NOVA, "Chaos Nova", CLASS_DEMON_HUNTER, 0,
                               InterruptMethod::STUN, 8.0f, 60.0f, 5000, 0,
                               false, 0, POWER_FURY, 30, false, 1});
            abilities.push_back({SIGIL_OF_SILENCE, "Sigil of Silence", CLASS_DEMON_HUNTER, 0,
                               InterruptMethod::SILENCE, 20.0f, 90.0f, 2000, 0xFFFF,
                               false, 0, POWER_FURY, 0, false, 1});
            break;

        case CLASS_DRUID:
            abilities.push_back({SKULL_BASH, "Skull Bash", CLASS_DRUID, 0,
                               InterruptMethod::SPELL_INTERRUPT, 13.0f, 15.0f, 4000, 0xFFFF,
                               false, 0, POWER_RAGE, 10, false, 1});
            abilities.push_back({SOLAR_BEAM, "Solar Beam", CLASS_DRUID, 103, // Balance only
                               InterruptMethod::SILENCE, 45.0f, 60.0f, 8000, 0xFFFF,
                               false, 0, POWER_MANA, 0, false, 1});
            abilities.push_back({TYPHOON, "Typhoon", CLASS_DRUID, 0,
                               InterruptMethod::KNOCKBACK, 15.0f, 30.0f, 0, 0,
                               false, 0, POWER_MANA, 0, true, 1});
            break;

        case CLASS_EVOKER:
            abilities.push_back({QUELL, "Quell", CLASS_EVOKER, 0,
                               InterruptMethod::SPELL_INTERRUPT, 25.0f, 40.0f, 4000, 0xFFFF,
                               false, 0, POWER_ESSENCE, 0, false, 1});
            abilities.push_back({TAIL_SWIPE, "Tail Swipe", CLASS_EVOKER, 0,
                               InterruptMethod::KNOCKBACK, 6.0f, 90.0f, 0, 0,
                               false, 0, POWER_ESSENCE, 0, false, 1});
            abilities.push_back({OPPRESSING_ROAR, "Oppressing Roar", CLASS_EVOKER, 0,
                               InterruptMethod::SILENCE, 10.0f, 120.0f, 3000, 0xFFFF,
                               false, 0, POWER_ESSENCE, 3, false, 1});
            break;

        case CLASS_HUNTER:
            abilities.push_back({COUNTER_SHOT, "Counter Shot", CLASS_HUNTER, 0,
                               InterruptMethod::SPELL_INTERRUPT, 40.0f, 24.0f, 3000, 0xFFFF,
                               false, 0, POWER_FOCUS, 0, false, 1});
            abilities.push_back({MUZZLE, "Muzzle", CLASS_HUNTER, 253, // BM pet ability
                               InterruptMethod::SPELL_INTERRUPT, 5.0f, 15.0f, 3000, 0xFFFF,
                               false, 0, POWER_FOCUS, 0, false, 1});
            abilities.push_back({FREEZING_TRAP, "Freezing Trap", CLASS_HUNTER, 0,
                               InterruptMethod::STUN, 40.0f, 30.0f, 8000, 0,
                               false, 0, POWER_FOCUS, 0, false, 1});
            break;

        case CLASS_MAGE:
            abilities.push_back({COUNTERSPELL, "Counterspell", CLASS_MAGE, 0,
                               InterruptMethod::SPELL_INTERRUPT, 40.0f, 24.0f, 6000, 0xFFFF,
                               false, 0, POWER_MANA, 0, false, 1});
            abilities.push_back({DRAGONS_BREATH, "Dragon's Breath", CLASS_MAGE, 63, // Fire only
                               InterruptMethod::STUN, 12.0f, 45.0f, 4000, 0,
                               false, 0, POWER_MANA, 0, false, 1});
            abilities.push_back({RING_OF_FROST, "Ring of Frost", CLASS_MAGE, 0,
                               InterruptMethod::STUN, 30.0f, 120.0f, 10000, 0,
                               false, 0, POWER_MANA, 0, false, 1});
            break;

        case CLASS_MONK:
            abilities.push_back({SPEAR_HAND_STRIKE, "Spear Hand Strike", CLASS_MONK, 0,
                               InterruptMethod::SPELL_INTERRUPT, 5.0f, 15.0f, 4000, 0xFFFF,
                               false, 0, POWER_CHI, 0, false, 1});
            abilities.push_back({PARALYSIS, "Paralysis", CLASS_MONK, 0,
                               InterruptMethod::STUN, 20.0f, 45.0f, 4000, 0,
                               false, 0, POWER_ENERGY, 20, false, 1});
            abilities.push_back({LEG_SWEEP, "Leg Sweep", CLASS_MONK, 0,
                               InterruptMethod::STUN, 5.0f, 60.0f, 3000, 0,
                               false, 0, POWER_CHI, 0, false, 1});
            break;

        case CLASS_PALADIN:
            abilities.push_back({REBUKE, "Rebuke", CLASS_PALADIN, 0,
                               InterruptMethod::SPELL_INTERRUPT, 5.0f, 15.0f, 4000, 0xFFFF,
                               false, 0, POWER_MANA, 0, false, 1});
            abilities.push_back({HAMMER_OF_JUSTICE, "Hammer of Justice", CLASS_PALADIN, 0,
                               InterruptMethod::STUN, 10.0f, 60.0f, 6000, 0,
                               false, 0, POWER_MANA, 0, false, 1});
            abilities.push_back({BLINDING_LIGHT, "Blinding Light", CLASS_PALADIN, 0,
                               InterruptMethod::STUN, 10.0f, 90.0f, 4000, 0,
                               false, 0, POWER_MANA, 0, false, 1});
            if (spec == 66) // Protection
            {
                abilities.push_back({AVENGERS_SHIELD, "Avenger's Shield", CLASS_PALADIN, 66,
                                   InterruptMethod::SILENCE, 30.0f, 15.0f, 3000, 0xFFFF,
                                   false, 0, POWER_MANA, 0, false, 1});
            }
            break;

        case CLASS_PRIEST:
            if (spec == 258) // Shadow
            {
                abilities.push_back({SILENCE, "Silence", CLASS_PRIEST, 258,
                                   InterruptMethod::SILENCE, 30.0f, 45.0f, 5000, 0xFFFF,
                                   false, 0, POWER_MANA, 0, false, 1});
                abilities.push_back({PSYCHIC_HORROR, "Psychic Horror", CLASS_PRIEST, 258,
                                   InterruptMethod::STUN, 30.0f, 45.0f, 4000, 0,
                                   false, 0, POWER_INSANITY, 30, false, 1});
            }
            abilities.push_back({PSYCHIC_SCREAM, "Psychic Scream", CLASS_PRIEST, 0,
                               InterruptMethod::FEAR, 8.0f, 30.0f, 8000, 0,
                               false, 0, POWER_MANA, 0, false, 1});
            break;

        case CLASS_ROGUE:
            abilities.push_back({KICK, "Kick", CLASS_ROGUE, 0,
                               InterruptMethod::SPELL_INTERRUPT, 5.0f, 15.0f, 5000, 0xFFFF,
                               false, 0, POWER_ENERGY, 15, false, 1});
            abilities.push_back({CHEAP_SHOT, "Cheap Shot", CLASS_ROGUE, 0,
                               InterruptMethod::STUN, 5.0f, 0.0f, 4000, 0,
                               false, 0, POWER_ENERGY, 40, false, 1});
            abilities.push_back({KIDNEY_SHOT, "Kidney Shot", CLASS_ROGUE, 0,
                               InterruptMethod::STUN, 5.0f, 20.0f, 6000, 0,
                               false, 0, POWER_COMBO_POINTS, 1, false, 1});
            abilities.push_back({BLIND, "Blind", CLASS_ROGUE, 0,
                               InterruptMethod::STUN, 15.0f, 120.0f, 8000, 0,
                               false, 0, POWER_ENERGY, 15, false, 1});
            break;

        case CLASS_SHAMAN:
            abilities.push_back({WIND_SHEAR, "Wind Shear", CLASS_SHAMAN, 0,
                               InterruptMethod::SPELL_INTERRUPT, 30.0f, 12.0f, 3000, 0xFFFF,
                               false, 0, POWER_MANA, 0, false, 1});
            abilities.push_back({CAPACITOR_TOTEM, "Capacitor Totem", CLASS_SHAMAN, 0,
                               InterruptMethod::STUN, 40.0f, 60.0f, 3000, 0,
                               false, 0, POWER_MANA, 0, false, 1});
            abilities.push_back({THUNDERSTORM, "Thunderstorm", CLASS_SHAMAN, 262, // Elemental
                               InterruptMethod::KNOCKBACK, 10.0f, 45.0f, 0, 0,
                               false, 0, POWER_MANA, 0, true, 1});
            break;

        case CLASS_WARLOCK:
            abilities.push_back({SPELL_LOCK, "Spell Lock", CLASS_WARLOCK, 0,
                               InterruptMethod::SPELL_INTERRUPT, 40.0f, 24.0f, 3000, 0xFFFF,
                               false, 0, POWER_MANA, 0, false, 1}); // Pet ability
            abilities.push_back({SHADOW_FURY, "Shadowfury", CLASS_WARLOCK, 0,
                               InterruptMethod::STUN, 30.0f, 30.0f, 3000, 0,
                               false, 0, POWER_MANA, 0, false, 1});
            abilities.push_back({MORTAL_COIL, "Mortal Coil", CLASS_WARLOCK, 0,
                               InterruptMethod::STUN, 20.0f, 45.0f, 3000, 0,
                               false, 0, POWER_MANA, 0, false, 1});
            break;

        case CLASS_WARRIOR:
            abilities.push_back({PUMMEL, "Pummel", CLASS_WARRIOR, 0,
                               InterruptMethod::SPELL_INTERRUPT, 5.0f, 15.0f, 4000, 0xFFFF,
                               false, 0, POWER_RAGE, 0, false, 1});
            abilities.push_back({STORM_BOLT, "Storm Bolt", CLASS_WARRIOR, 0,
                               InterruptMethod::STUN, 20.0f, 30.0f, 4000, 0,
                               false, 0, POWER_RAGE, 5, false, 1});
            abilities.push_back({SHOCKWAVE, "Shockwave", CLASS_WARRIOR, 73, // Protection
                               InterruptMethod::STUN, 10.0f, 40.0f, 4000, 0,
                               false, 0, POWER_RAGE, 10, false, 1});
            abilities.push_back({INTIMIDATING_SHOUT, "Intimidating Shout", CLASS_WARRIOR, 0,
                               InterruptMethod::FEAR, 8.0f, 90.0f, 8000, 0,
                               false, 0, POWER_RAGE, 0, false, 1});
            // Mountain Thane Hero Talent
            abilities.push_back({DISRUPTING_SHOUT, "Disrupting Shout", CLASS_WARRIOR, 0,
                               InterruptMethod::SPELL_INTERRUPT, 10.0f, 75.0f, 4000, 0xFFFF,
                               true, 0, POWER_RAGE, 0, false, 1});
            break;
    }

    return abilities;
}

float GetOptimalRange(uint8 playerClass)
{
    switch (playerClass)
    {
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
        case CLASS_ROGUE:
        case CLASS_DEATH_KNIGHT:
        case CLASS_MONK:
        case CLASS_DEMON_HUNTER:
            return 5.0f; // Melee range

        case CLASS_HUNTER:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
        case CLASS_EVOKER:
            return 40.0f; // Long range

        case CLASS_SHAMAN:
        case CLASS_PRIEST:
        case CLASS_DRUID:
            return 30.0f; // Medium range

        default:
            return 20.0f;
    }
}

uint32 GetSchoolLockoutDuration(uint32 spellId)
{
    // Standard lockout durations for different abilities
    switch (spellId)
    {
        case COUNTERSPELL:
            return 6000; // 6 seconds
        case KICK:
        case PUMMEL:
        case REBUKE:
        case MIND_FREEZE:
        case DISRUPT:
        case SPEAR_HAND_STRIKE:
            return 4000; // 4 seconds
        case WIND_SHEAR:
        case COUNTER_SHOT:
        case SKULL_BASH:
            return 3000; // 3 seconds
        case QUELL:
            return 4000; // 4 seconds
        case SPELL_LOCK:
            return 3000; // 3 seconds
        default:
            return 4000; // Default 4 seconds
    }
}

} // namespace InterruptAbilities

// Mythic+ scaling implementation
::std::unordered_map<uint8, MythicPlusConfig> MythicPlusInterruptScaling::_configs = {
    {2,  {2,  0.95f, 1.0f, false, 1}},
    {3,  {3,  0.93f, 1.0f, false, 1}},
    {4,  {4,  0.91f, 1.0f, false, 1}},
    {5,  {5,  0.89f, 1.1f, false, 1}},
    {6,  {6,  0.87f, 1.1f, false, 1}},
    {7,  {7,  0.85f, 1.2f, true,  2}},
    {8,  {8,  0.83f, 1.2f, true,  2}},
    {9,  {9,  0.81f, 1.3f, true,  2}},
    {10, {10, 0.80f, 1.3f, true,  2}},
    {11, {11, 0.78f, 1.4f, true,  2}},
    {12, {12, 0.76f, 1.4f, true,  3}},
    {13, {13, 0.74f, 1.5f, true,  3}},
    {14, {14, 0.72f, 1.5f, true,  3}},
    {15, {15, 0.70f, 1.6f, true,  3}},
    {16, {16, 0.68f, 1.6f, true,  3}},
    {17, {17, 0.66f, 1.7f, true,  3}},
    {18, {18, 0.64f, 1.7f, true,  3}},
    {19, {19, 0.62f, 1.8f, true,  4}},
    {20, {20, 0.60f, 1.8f, true,  4}}
};

MythicPlusConfig const* MythicPlusInterruptScaling::GetConfig(uint8 level)
{
    auto it = _configs.find(level);
    return it != _configs.end() ? &it->second : nullptr;
}

float MythicPlusInterruptScaling::GetReactionTimeModifier(uint8 level)
{
    auto config = GetConfig(level);
    return config ? config->interruptWindowReduction : 1.0f;
}

InterruptPriority MythicPlusInterruptScaling::AdjustPriorityForLevel(InterruptPriority base, uint8 level)
{
    if (level >= 10 && base == InterruptPriority::HIGH)
        return InterruptPriority::CRITICAL;
    if (level >= 7 && base == InterruptPriority::MODERATE)
        return InterruptPriority::HIGH;
    return base;
}

bool MythicPlusInterruptScaling::RequiresCoordinatedInterrupts(uint8 level)
{
    auto config = GetConfig(level);
    return config ? config->requiresRotation : false;
}

uint32 MythicPlusInterruptScaling::GetRequiredInterrupters(uint8 level)
{
    auto config = GetConfig(level);
    return config ? config->minInterruptersRequired : 1;
}

// Rotation templates
namespace RotationTemplates
{

RotationTemplate const MELEE_HEAVY = {
    "Melee Heavy",
    {CLASS_WARRIOR, CLASS_ROGUE, CLASS_DEATH_KNIGHT, CLASS_DEMON_HUNTER, CLASS_MONK},
    {{InterruptAbilities::PUMMEL, CLASS_WARRIOR}, {InterruptAbilities::KICK, CLASS_ROGUE}},
    8000,
    true
};

RotationTemplate const RANGED_HEAVY = {
    "Ranged Heavy",
    {CLASS_HUNTER, CLASS_MAGE, CLASS_WARLOCK, CLASS_EVOKER},
    {{InterruptAbilities::COUNTER_SHOT, CLASS_HUNTER}, {InterruptAbilities::COUNTERSPELL, CLASS_MAGE}},
    12000,
    true
};

RotationTemplate const BALANCED = {
    "Balanced",
    {CLASS_WARRIOR, CLASS_MAGE, CLASS_PRIEST, CLASS_HUNTER, CLASS_SHAMAN},
    {{InterruptAbilities::PUMMEL, CLASS_WARRIOR}, {InterruptAbilities::COUNTERSPELL, CLASS_MAGE},
     {InterruptAbilities::WIND_SHEAR, CLASS_SHAMAN}},
    10000,
    true
};

RotationTemplate const* GetOptimalTemplate(::std::vector<Player*> const& group)
{
    uint32 meleeCount = 0;
    uint32 rangedCount = 0;

    for (Player* member : group)
    {
        if (!member)
            continue;

        switch (member->GetClass())
        {
            case CLASS_WARRIOR:
            case CLASS_PALADIN:
            case CLASS_ROGUE:
            case CLASS_DEATH_KNIGHT:
            case CLASS_MONK:
            case CLASS_DEMON_HUNTER:
                ++meleeCount;
                break;
            default:
                ++rangedCount;
                break;
        }
    }

    if (meleeCount >= 3)
        return &MELEE_HEAVY;
    if (rangedCount >= 3)
        return &RANGED_HEAVY;
    return &BALANCED;
}

} // namespace RotationTemplates

// Performance optimizer
::std::unordered_map<uint32, InterruptPerformanceData> InterruptOptimizer::_performanceData;

void InterruptOptimizer::RecordInterruptAttempt(uint32 spellId, bool success, float reactionTime)
{
    auto& data = _performanceData[spellId];

    if (success)
        ++data.successCount;
    else
        ++data.failCount;

    uint32 total = data.successCount + data.failCount;
    data.averageReactionTime = (data.averageReactionTime * (total - 1) + reactionTime) / total;
    data.successRate = static_cast<float>(data.successCount) / total;
    data.lastUpdated = GameTime::GetGameTimeMS();
    data.spellId = spellId;
}

float InterruptOptimizer::GetOptimalTiming(uint32 spellId)
{
    auto it = _performanceData.find(spellId);
    if (it == _performanceData.end() || (it->second.successCount + it->second.failCount) < MIN_SAMPLES)
        return 0.5f; // Default 500ms into cast

    // Return the average successful reaction time
    return it->second.averageReactionTime;
}

} // namespace Playerbot