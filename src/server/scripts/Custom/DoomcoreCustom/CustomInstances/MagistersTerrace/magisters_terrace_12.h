/*
 * DoomCore Scripts - Magister's Terrace 12.0.1 Revamp (Map 2811)
 */

#ifndef DEF_MAGISTERS_TERRACE_12_H
#define DEF_MAGISTERS_TERRACE_12_H

#include "CreatureAIImpl.h"

#define MT12ScriptName "instance_magisters_terrace_12"
#define MT12DataHeader "MT12"

uint32 const MT12EncounterCount = 4;

// ============================================================================
// Data Types
// ============================================================================
enum MT12DataTypes
{
    // Encounter states (0-3)
    DATA_ARCANOTRON_CUSTOS      = 0,
    DATA_SERANEL_SUNLASH        = 1,
    DATA_GEMELLUS               = 2,
    DATA_DEGENTRIUS             = 3,

    // Additional data
    DATA_MAGISTER_UMBRIC,
    DATA_GRAND_MAGISTER_ROMMATH,
    DATA_CYNOSURE_OF_TWILIGHT,
};

// ============================================================================
// Creature IDs
// ============================================================================
enum MT12CreatureIds
{
    // Bosses
    BOSS_ARCANOTRON_CUSTOS      = 231861,
    BOSS_SERANEL_SUNLASH        = 231863,
    BOSS_GEMELLUS               = 231864,
    BOSS_DEGENTRIUS             = 231865,

    // Boss-related
    NPC_ENERGY_ORB              = 234775,
    NPC_GEMELLUS_CLONE          = 239636,
    NPC_GEMELLUS_SPAWNER        = 251776,
    NPC_ORBITAL_STALKER         = 239638,
    NPC_VOID_MOTE_SOCKET        = 235476,
    NPC_NULL_BOMB_STALKER       = 236357,

    // Narrative NPCs
    NPC_MAGISTER_UMBRIC         = 234148,
    NPC_GRAND_MAGISTER_ROMMATH  = 245185,
    NPC_MAGICIAN_APPRENTICE     = 247982,

    // Scripted Trash
    NPC_ARCANE_SENTRY           = 234062,
    NPC_VIGILANT_LIBRARIAN      = 234067,
    NPC_BLAZING_PYROMANCER      = 251861,
    NPC_BLAZING_PYROMANCER_2    = 257161,
    NPC_LIGHTWARD_HEALER        = 234486,
    NPC_LIGHTWARD_HEALER_2      = 241324,
    NPC_SUNBLADE_ENFORCER       = 234124,
    NPC_SUNBLADE_ENFORCER_2     = 241325,
    NPC_ARCANE_MAGISTER         = 232369,
    NPC_ARCANE_MAGISTER_2       = 241326,
    NPC_ARCANE_MAGISTER_3       = 257644,
    NPC_LADY_SACROLASH          = 260076,
    NPC_GRAND_WARLOCK_ALYTHESS  = 260077,
    NPC_ANTORAN_INQUISITOR      = 258242,

    // Void Trash
    NPC_DREADFUL_VOIDWALKER     = 234064,
    NPC_HOLLOWSOUL_SHREDDER     = 234065,
    NPC_HOLLOWSOUL_SHREDDER_2   = 257447,
    NPC_DEVOURING_TYRANT        = 234066,
    NPC_SHADOWRIFT_VOIDCALLER   = 234068,
    NPC_VOIDLING                = 234069,
    NPC_ANIMATED_CODEX          = 234089,
    NPC_RUNED_SPELLBREAKER      = 240973,
    NPC_RUNED_SPELLBREAKER_2    = 241444,
    NPC_UNSTABLE_VOID_STALKER   = 235846,
    NPC_VOID_TERROR             = 249086,
    NPC_ANTORAN_JAILER          = 255147,
    NPC_UNSTABLE_VOIDLING       = 255376,
    NPC_BRIGHTSCALE_WYRM        = 232106,
    NPC_VOID_INFUSED_BRIGHTSCALE = 241354,

    // Misc
    NPC_ENERGY_CRYSTAL          = 232102,
    NPC_VOID_PORTAL             = 255613,
    NPC_VOID_PORTAL_2           = 255616,
    NPC_DEFENDER_OF_TRUTH       = 240059,
};

// ============================================================================
// GameObject IDs
// ============================================================================
enum MT12GameObjectIds
{
    GO_SUNWELL_DOOR_1           = 550374,
    GO_SUNWELL_DOOR_2           = 550375,
    GO_RAMPART_DOOR             = 556380,
    GO_ORB_OF_TRANSLOCATION_1   = 494914,
    GO_ORB_OF_TRANSLOCATION_2   = 626819,
    GO_ORB_OF_TRANSLOCATION_3   = 626820,
    GO_CYNOSURE_OF_TWILIGHT     = 245325,
};

// ============================================================================
// Spell IDs — Boss 1: Arcanotron Custos
// ============================================================================
enum ArcanotronSpells
{
    // Refueling Protocol
    SPELL_REFUELING_PROTOCOL        = 474345,   // Self-channel, triggers orb spawning
    SPELL_REFUELING_ENERGIZE        = 1253889,  // Self-energize for 100 Energy

    // Arcane Expulsion
    SPELL_ARCANE_EXPULSION          = 1214081,  // AoE knockback + residue (5 targets, SchoolMask 64)
    SPELL_ARCANE_EXPULSION_POOL     = 474414,   // Ground-targeted AoE, creates Arcane Residue

    // Ethereal Shackles
    SPELL_ETHEREAL_SHACKLES         = 1214032,  // Cast trigger
    SPELL_ETHEREAL_SHACKLES_DEBUFF  = 1214034,  // Root + DoT debuff

    // Arcane Empowerment (orb reaches boss)
    SPELL_ARCANE_EMPOWERMENT        = 1243905,  // Applied to orb/self, AoE explosion + damage buff

    // Repulsing Slam (tank hit)
    SPELL_REPULSING_SLAM            = 474496,   // Instant, single-target, Arcane damage + knockback

    // Energy Orb spell
    SPELL_ENERGY_ORB_VISUAL         = 474308,   // Energy Orb periodic (every 2-3s)
};

// ============================================================================
// Spell IDs — Boss 2: Seranel Sunlash
// ============================================================================
enum SeranelSpells
{
    // Runic Mark
    SPELL_RUNIC_MARK_TRIGGER        = 1225787,  // Self-cast initiator
    SPELL_RUNIC_MARK                = 1225792,  // 3s cast on player, moderate AoE damage
    SPELL_RUNIC_MARK_DEBUFF         = 1224842,  // Secondary invisible debuff (Feedback interaction)

    // Wave of Silence / Vow of Silence
    SPELL_VOW_OF_SILENCE            = 1225193,  // Parent spell, massive AoE + silence outside Suppression Zones
    SPELL_VOW_OF_SILENCE_DAMAGE     = 1225201,  // Chains from 1225193, damage/silence effect

    // Hastening Ward
    SPELL_HASTENING_WARD_MELEE      = 1225796,  // High-freq melee modifier (Arcane melee)
    SPELL_HASTENING_WARD_DAMAGE     = 1225135,  // Direct damage from empowered melee

    // Suppression Zone
    SPELL_SUPPRESSION_ZONE          = 1224903,  // Self-cast 3s, creates ground zone

    // Additional
    SPELL_SERANEL_ADDITIONAL        = 1271317,  // Periodic ability
};

// ============================================================================
// Spell IDs — Boss 3: Gemellus
// ============================================================================
enum GemellusSpells
{
    // Triplicate (HP-triggered: 90% and 50%)
    SPELL_TRIPLICATE                = 1223847,  // 2.5s cast, creates 2 clones (Entry 239636)

    // Synaptic Nexus (shared health)
    SPELL_SYNAPTIC_NEXUS            = 1224574,  // Self-buff on spawn/split
    SPELL_SYNAPTIC_NEXUS_VISUAL     = 1227222,  // Visual buff

    // Astral Grasp (100 energy)
    SPELL_ASTRAL_GRASP              = 1224299,  // 4s cast, pulls players toward Gemellus

    // Cosmic Radiation / Sting
    SPELL_COSMIC_RADIATION          = 1223670,  // Multi-school damage to pulled players
    SPELL_VOID_SECRETIONS           = 1223961,  // Pool at player location
    SPELL_COSMIC_STING              = 1224100,  // Shadow DoT

    // Neural Link
    SPELL_NEURAL_LINK               = 1224129,  // Links to player
    SPELL_NEURAL_LINK_DEBUFF_1      = 1284954,  // Reduced damage taken debuff
    SPELL_NEURAL_LINK_DEBUFF_2      = 1284958,  // Increased damage taken debuff

    // Additional
    SPELL_GEMELLUS_MISC_1           = 1224337,
    SPELL_GEMELLUS_MISC_2           = 1244456,  // Both main and clone cast
    SPELL_GEMELLUS_CLONE_ONLY       = 1265609,  // Only from Entry 239636
    SPELL_GEMELLUS_CLONE_2          = 1224104,  // Clone ability
};

// ============================================================================
// Spell IDs — Boss 4: Degentrius
// ============================================================================
enum DegentriusSpells
{
    // Unstable Void Essence (bouncing soak)
    SPELL_UNSTABLE_VOID_ESSENCE     = 1243602,  // Main cast, bounces to 4 locations
    SPELL_UVE_BOUNCE_1              = 1215027,  // Bounce position 1
    SPELL_UVE_BOUNCE_2              = 1215028,  // Bounce position 2
    SPELL_UVE_BOUNCE_3              = 1215029,  // Bounce position 3
    SPELL_VOID_DESTRUCTION          = 1215163,  // Failed soak AoE (231114 Shadow)
    SPELL_NULL_BOMB                 = 1243134,  // Mythic: 5th bounce, 7s timer

    // Devouring Entropy (DoT on 2 players)
    SPELL_DEVOURING_ENTROPY         = 1215897,  // 23164 Shadow/s for 20s
    SPELL_ENTROPY_PERIODIC          = 1215893,  // Entropy periodic component
    SPELL_ENTROPY_ORB               = 1215896,  // Entropy Orb on removal

    // Void Mote / Void Torrent
    SPELL_VOID_MOTE_1               = 1215067,  // Mote placement
    SPELL_VOID_MOTE_2               = 1215087,  // Mote placement
    SPELL_VOID_MOTE_3               = 1215099,  // Mote placement
    SPELL_VOID_TORRENT              = 1270923,  // Channeled beam from mote to boss
    SPELL_VOID_MOTE_MISC            = 1253784,  // Mote related

    // Umbral Eruption
    SPELL_UMBRAL_ERUPTION           = 1271066,  // Ground eruption, 138668 Shadow + knockback
    SPELL_UMBRAL_ERUPTION_AOE       = 1269647,  // AoE component

    // Entropy Blast (tank hit)
    SPELL_ENTROPY_BLAST             = 1280119,  // Physical damage
    SPELL_ENTROPY_BLAST_2           = 1280106,  // Variant
    SPELL_ENTROPY_BLAST_3           = 1280107,  // Variant
    SPELL_ENTROPY_BLAST_4           = 1280113,  // Variant

    // Additional
    SPELL_DEGENTRIUS_OPENING_BUFF   = 434023,   // Combat start self-buff
    SPELL_DEGENTRIUS_DEBUFF         = 474488,   // Debuff/utility
    SPELL_DEGENTRIUS_MISC_1         = 1279817,  // Mechanic spell
    SPELL_DEGENTRIUS_MISC_2         = 1284558,  // Additional mechanic
};

// ============================================================================
// Spell IDs — Trash Mobs
// ============================================================================
enum MT12TrashSpells
{
    // Arcane Sentry
    SPELL_SENTRY_1                  = 1282050,
    SPELL_SENTRY_2                  = 1282051,
    SPELL_SENTRY_3                  = 1282053,
    SPELL_SENTRY_4                  = 473258,

    // Blazing Pyromancer
    SPELL_PYROMANCER_1              = 1254294,
    SPELL_PYROMANCER_2              = 1254338,
    SPELL_PYROMANCER_CHANNEL        = 1255199,  // Channeled fire
    SPELL_PYROMANCER_4              = 427472,

    // Arcane Magister
    SPELL_MAGISTER_1                = 1253861,
    SPELL_MAGISTER_CHANNEL          = 1255209,  // Channeled arcane
    SPELL_MAGISTER_3                = 468962,

    // Lightward Healer
    SPELL_HEALER_HEAL               = 1255187,

    // Sunblade Enforcer
    SPELL_ENFORCER_1                = 1252910,

    // Vigilant Librarian
    SPELL_LIBRARIAN_AT              = 1270750,  // Creates AreaTrigger (Entry 41944, Cylinder R=2 H=4)

    // Lady Sacrolash
    SPELL_SACROLASH_1               = 1282507,

    // Antoran Inquisitor
    SPELL_INQUISITOR_1              = 1254595,
    SPELL_INQUISITOR_2              = 1264951,
    SPELL_INQUISITOR_3              = 1277099,
    SPELL_INQUISITOR_4              = 1280457,
    SPELL_INQUISITOR_5              = 1280460,
};

// ============================================================================
// Actions
// ============================================================================
enum MT12Actions
{
    // Instance → Umbric
    ACTION_UMBRIC_INTRO             = 1,
    ACTION_UMBRIC_BOSS1_INTRO,
    ACTION_UMBRIC_BOSS1_DETAINED,
    ACTION_UMBRIC_BOSS1_DONE,
    ACTION_UMBRIC_LIBRARY,
    ACTION_UMBRIC_BOSS2_INTRO,
    ACTION_UMBRIC_BOSS2_DONE,
    ACTION_UMBRIC_BOSS3_DONE,
    ACTION_UMBRIC_BOSS4_INTRO,
    ACTION_UMBRIC_BOSS4_DONE,

    // Instance → Seranel
    ACTION_SERANEL_START_RP,

    // Boss 1 → Instance
    ACTION_ARCANOTRON_REFUELING,
};

// ============================================================================
// Talk Lines (creature_text indices)
// ============================================================================

// Arcanotron Custos
enum ArcanotronTalk
{
    SAY_ARCANOTRON_AGGRO            = 0,  // "All students must follow policy."
    SAY_ARCANOTRON_POLICY           = 1,  // "Policy demands compliance."
    SAY_ARCANOTRON_IMPAIRMENT       = 2,  // "Proceeding with impairment."
    SAY_ARCANOTRON_DETAINMENT       = 3,  // "Enjoy detainment."
    SAY_ARCANOTRON_SUPPRESSIVE      = 4,  // "Suppressive measures necessary."
    SAY_ARCANOTRON_STILL            = 5,  // "Please remain still."
    SAY_ARCANOTRON_COMPLIANCE       = 6,  // "Compliance is not optional."
    SAY_ARCANOTRON_REFUELING        = 7,  // "Refueling. Please stand by."
    SAY_ARCANOTRON_RESUME           = 8,  // "Resuming disciplinary measures."
    SAY_ARCANOTRON_DEATH            = 9,  // "Security... failure..."
};

// Seranel Sunlash
enum SeranelTalk
{
    SAY_SERANEL_PREFIGHT_1          = 0,  // "They must not reach the Cynosure of Twilight!"
    SAY_SERANEL_PREFIGHT_2          = 1,  // "Stop them!"
    SAY_SERANEL_AGGRO_1             = 2,  // "Liar! None shall lay hands on the Cynosure!"
    SAY_SERANEL_AGGRO_2             = 3,  // "The Cynosure remains here! Diel fin'al!"
    SAY_SERANEL_RUNIC_MARK          = 4,  // "Your fall is written!" / "Marked for defeat!"
    SAY_SERANEL_SILENCE_1           = 5,  // "Speak no more!"
    SAY_SERANEL_SILENCE_2           = 6,  // "Silence!"
    SAY_SERANEL_UNYIELDING          = 7,  // "I am unyielding!" / "I am unrelenting!"
    SAY_SERANEL_DEATH               = 8,  // "You shall not--"
    EMOTE_SERANEL_RUNIC_MARK        = 9,  // RaidBossEmote: readies glaive [Runic Mark]
    EMOTE_SERANEL_WAVE_OF_SILENCE   = 10, // RaidBossEmote: begins to cast [Wave of Silence]
};

// Gemellus
enum GemellusTalk
{
    EMOTE_GEMELLUS_TRIPLICATE       = 0,  // "Gemellus begins to [Triplicate]!"
    EMOTE_GEMELLUS_ASTRAL_GRASP     = 1,  // "Gemellus begins to cast [Astral Grasp]!"
};

// Degentrius
enum DegentriusTalk
{
    SAY_DEGENTRIUS_AGGRO            = 0,  // "You dare? Then die!"
    SAY_DEGENTRIUS_SHADOWS_SURGE    = 1,  // "Shadows surge!"
    SAY_DEGENTRIUS_DARKNESS         = 2,  // "Darkness devours!"
    SAY_DEGENTRIUS_SEALED           = 3,  // "Sealed in darkness."
    SAY_DEGENTRIUS_SHATTERING       = 4,  // "Shattering shadows!"
    SAY_DEGENTRIUS_UMBRAL           = 5,  // "Umbral cascade!"
    SAY_DEGENTRIUS_VOID_HUNGERS     = 6,  // "The Void hungers!"
    SAY_DEGENTRIUS_DEATH            = 7,  // "You... how..."
    EMOTE_DEGENTRIUS_UVE            = 8,  // RaidBossEmote: creates [Unstable Void Essence]
    EMOTE_DEGENTRIUS_ENTROPY        = 9,  // RaidBossEmote: afflicted by [Devouring Entropy]
};

// Magister Umbric
enum UmbricTalk
{
    SAY_UMBRIC_INTRO                = 0,  // "The Terrace holds what we require..."
    SAY_UMBRIC_SENTRY_REACT         = 1,  // "Of course he left a message for me..."
    SAY_UMBRIC_INCAPACITATE         = 2,  // "I'm afraid you'll have to incapacitate the guards."
    SAY_UMBRIC_ITEMS                = 3,  // "I've a few items I'd like to pick up..."
    SAY_UMBRIC_BEWARE_LIBRARIANS    = 4,  // "Oh, and beware of the librarians..."
    SAY_UMBRIC_LIBRARY              = 5,  // "How I've missed this library..."
    SAY_UMBRIC_DETAINED             = 6,  // "I seem to be... detained..."
    SAY_UMBRIC_POST_BOSS1           = 7,  // "A crude solution, but effective..."
    SAY_UMBRIC_SERANEL_CONFRONT     = 8,  // "Seranel! Stand down! I have permission..."
    SAY_UMBRIC_POST_BOSS2_1         = 9,  // "Help me remove the wards..."
    SAY_UMBRIC_POST_BOSS2_2         = 10, // "Something's gone terribly wrong..."
    SAY_UMBRIC_POST_BOSS2_3         = 11, // "Seranel, the Void has broken through!..."
    SAY_UMBRIC_PRE_BOSS4            = 12, // "$p, I know what to do!..."
    SAY_UMBRIC_POST_BOSS4_1         = 13, // "It is done! We have our way..."
    SAY_UMBRIC_POST_BOSS4_2         = 14, // "...and perhaps offer an apology..."
    SAY_UMBRIC_READY_TO_LEAVE       = 15, // "We've no time to waste..."
};

// Arcane Sentry
enum ArcaneSentryTalk
{
    SAY_SENTRY_AGGRO                = 0,  // "Unauthorized entry."
    SAY_SENTRY_COMBAT               = 1,  // "Unauthorized entry. Swift action will be taken."
    SAY_SENTRY_DEATH                = 2,  // "Have... a... pleasant... day..."
};

// Blazing Pyromancer
enum BlazingPyromancerTalk
{
    SAY_PYRO_AGGRO                  = 0,  // "Who is causing all this noise in the library?"
    SAY_PYRO_COMBAT                 = 1,  // "Shhh!"
    SAY_PYRO_COMBAT_2               = 2,  // "Fires consume."
};

// ============================================================================
// Template Helper
// ============================================================================
template <class AI, class T>
inline AI* GetMagistersTerrace12AI(T* obj)
{
    return GetInstanceAI<AI>(obj, MT12ScriptName);
}

#define RegisterMagistersTerrace12CreatureAI(ai_name) \
    RegisterCreatureAIWithFactory(ai_name, GetMagistersTerrace12AI)

#endif // DEF_MAGISTERS_TERRACE_12_H
