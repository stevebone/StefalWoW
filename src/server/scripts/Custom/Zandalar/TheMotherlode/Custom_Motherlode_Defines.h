/*
 * This file is part of the Stefal WoW Project.
 * It is designed to work exclusively with the TrinityCore framework.
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
 * This code is provided for personal and educational use within the
 * Stefal WoW Project. It is not intended for commercial distribution,
 * resale, or any form of monetization.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CUSTOM_MOTHERLODE_DEFINES_H
#define CUSTOM_MOTHERLODE_DEFINES_H

#include "Creature.h"
#include "Position.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Unit.h"

namespace Scripts::Zandalar::TheMotherlode
{
    namespace Misc
    {
        constexpr char const* ScriptName = "custom_instance_the_motherlode";
        constexpr char const* DataHeader = "ML";
        static constexpr uint32 MapId = 1594;
        static constexpr int8 EncounterCount = 4;
    }

    namespace DataTypes
    {
        static constexpr int8 BOSS_COIN_OPERATED_CROWD_PUMMELER = 0;
        static constexpr int8 BOSS_AZEROKK = 1;
        static constexpr int8 BOSS_RIXXA_FLUXFLAME = 2;
        static constexpr int8 BOSS_MOGUL_RAZDUNK = 3;
    }

    namespace Encounters
    {
        // DungeonEncounter ids (12.x DB2)
        static constexpr uint32 CrowdPummeler = 2105;
        static constexpr uint32 Azerokk = 2106;
        static constexpr uint32 RixxaFluxflame = 2107;
        static constexpr uint32 MogulRazdunk = 2108;
    }

    // world_state rows 14432/14435/14437/14439 live in TheMotherlode_Main.sql -
    // BossAI and the client drive them, scripts do not set them explicitly.
    namespace Creatures
    {
        // Bosses
        static constexpr uint32 CoinOperatedCrowdPummeler = 129214;
        static constexpr uint32 Azerokk = 129227;
        static constexpr uint32 RixxaFluxflame = 129231;
        static constexpr uint32 MogulRazdunk = 129232;
        static constexpr uint32 MogulRazdunkVehicle = 132713;

        // Encounter adds
        static constexpr uint32 Footbomb = 129246;       // kickable ball (Pummeler)
        static constexpr uint32 CoinPile = 140636;       // coin piles thrown by the crowd (Pummeler)
        static constexpr uint32 Earthrager = 129802;     // fixating add (Azerokk)
        static constexpr uint32 BoombaDrone = 141303;    // B.O.O.M.B.A. drone (Razdunk, Heroic+)
        static constexpr uint32 Skyscorcher = 131247;    // stage-two add (Razdunk)
    }

    // Telegraph areatriggers: areatrigger_create_properties row ids passed to
    // AreaTrigger::CreateAreaTrigger({ id, isCustom }). Ids 1409/5500 are retail
    // rows (IsCustom = 0 -> call sites pass false); the rest are custom
    // SpellForVisuals-driven warning circles (IsCustom = 1). SQL in
    // TheMotherlode_Main.sql.
    namespace AreaTriggerTelegraphs
    {
        static constexpr uint32 CoinImpact = 15623;       // visual spell 287073
        static constexpr uint32 FootbombBlast = 14739;    // visual spell 277289
        static constexpr uint32 ShockingClaw = 15262;     // visual spell 284285
        static constexpr uint32 TectonicSmash = 1409;     // RETAIL row -> AT 16446 (268078)
        static constexpr uint32 CatalystFire = 12208;     // visual spell 259787
        static constexpr uint32 PropellantJet = 12307;    // visual spell 260669
        static constexpr uint32 MissileBlast = 5500;      // RETAIL row -> AT 10202 (263521)
        static constexpr uint32 BigRedRocket = 12803;     // visual spell 264456
        static constexpr uint32 DrillSmash = 15055;       // visual spell 282463
        static constexpr uint32 BoombaMissile = 14739;    // reuses the footbomb visual
        static constexpr uint32 GatlingGun = 5500;        // reuses missile visual, RETAIL row
    }

    namespace Pummeler
    {
        namespace Spells
        {
            static constexpr uint32 CoinToss = 1217286;
            static constexpr uint32 CoinMagnet = 271903;
            static constexpr uint32 PaidToWin = 271867;        // damage-taken stacks on the boss
            static constexpr uint32 FootbombBlast = 256137;
            static constexpr uint32 BlazingAzerite = 256493;   // +damage taken debuff
            static constexpr uint32 StaticPulse = 262347;
            static constexpr uint32 ShockingClaw = 1217294;
            static constexpr uint32 ShockingClawHit = 1217296;
            static constexpr uint32 OutOfOrder = 267547;       // spawn stun aura
        }

        namespace Events
        {
            static constexpr uint32 CoinMagnet = 1;
            static constexpr uint32 StaticPulse = 2;
            static constexpr uint32 ShockingClaw = 3;
            static constexpr uint32 Footbomb = 4;
            static constexpr uint32 CoinToss = 5;
        }

        // creature_text GroupIDs (canonical TALK_* layout, see TheMotherlode_CreatureText.sql)
        namespace Voice
        {
            static constexpr uint8 Aggro = 0;
            static constexpr uint8 StaticPulse = 1;
            static constexpr uint8 FootbombLauncher = 2;   // emote row
            static constexpr uint8 ShockingClaw = 3;
            static constexpr uint8 CoinMagnet = 4;
            static constexpr uint8 Death = 5;
            static constexpr uint8 Slay = 6;
        }
    }

    namespace Azerokk
    {
        namespace Spells
        {
            static constexpr uint32 CallEarthrager = 257593;
            static constexpr uint32 RagingGaze = 257582;       // fixate marker
            static constexpr uint32 JaggedCut = 257544;        // stacking bleed on fixate target
            static constexpr uint32 AzeriteInfusion = 257597;  // empower a rager
            static constexpr uint32 InfusionPulse = 1213142;
            static constexpr uint32 ResonantQuake = 258622;
            static constexpr uint32 ResonantQuakeHit = 471894;
            static constexpr uint32 Aftershock = 258627;
            static constexpr uint32 AzeriteAftershock = 1214673;
            static constexpr uint32 TectonicSmash = 275907;
        }

        namespace Events
        {
            static constexpr uint32 CallEarthrager = 1;
            static constexpr uint32 AzeriteInfusion = 2;
            static constexpr uint32 ResonantQuake = 3;
            static constexpr uint32 TectonicSmash = 4;
        }

        // creature_text GroupIDs - real retail BroadcastText lines recovered from
        // hotfixes (see TheMotherlode_CreatureText.sql)
        namespace Voice
        {
            static constexpr uint8 Aggro = 0;
            static constexpr uint8 CallEarthrager = 1;
            static constexpr uint8 AzeriteInfusion = 2;
            static constexpr uint8 ResonantQuake = 3;
            static constexpr uint8 TectonicSmash = 4;
            static constexpr uint8 Slay = 5;
            static constexpr uint8 Death = 6;
        }
    }

    namespace Rixxa
    {
        namespace Spells
        {
            static constexpr uint32 GushingCatalyst = 275992;
            static constexpr uint32 SearingReagent = 259474;
            static constexpr uint32 AzeriteCatalyst = 259022;
            static constexpr uint32 CatalystFire = 259533;     // NOTE: aura self-targets - ground fire is simulated
            static constexpr uint32 ChemicalBurn = 259853;
            static constexpr uint32 PropellantBlast = 259940;
            static constexpr uint32 PropellantJet = 1217672;
            static constexpr uint32 PropellantHit = 260103;
        }

        namespace Events
        {
            static constexpr uint32 AzeriteCatalyst = 1;
            static constexpr uint32 ChemicalBurn = 2;
            static constexpr uint32 SearingReagent = 3;
            static constexpr uint32 PropellantBlast = 4;
            static constexpr uint32 GushingCatalyst = 5;
        }

        // creature_text GroupIDs - real retail BroadcastText lines packed into a
        // canonical 0-N layout (the repack's rows for her were placeholder stubs).
        namespace Voice
        {
            static constexpr uint8 Intro = 0;             // approach line, fires once
            static constexpr uint8 Aggro = 1;
            static constexpr uint8 AzeriteCatalyst = 2;
            static constexpr uint8 PropellantBlast = 3;
            static constexpr uint8 ChemicalBurn = 4;
            static constexpr uint8 Death = 5;
        }
    }

    namespace Razdunk
    {
        namespace Spells
        {
            static constexpr uint32 AlphaCannon = 260318;
            static constexpr uint32 AlphaCannonHit = 260323;
            static constexpr uint32 GatlingGun = 260279;
            static constexpr uint32 HomingMissile = 260811;
            static constexpr uint32 MissileBlast = 260838;
            static constexpr uint32 Boomba = 276234;
            static constexpr uint32 DrillSmash = 260202;
            static constexpr uint32 DrillSmashHit = 270926;
            static constexpr uint32 BigRedRocket = 270277;
            static constexpr uint32 BusterShot = 260372;
        }

        // Stage One events are < 10, Stage Two events are >= 10 (kit convention kept for clarity)
        namespace Events
        {
            static constexpr uint32 AlphaCannon = 1;
            static constexpr uint32 GatlingGun = 2;
            static constexpr uint32 HomingMissile = 3;
            static constexpr uint32 BoombaDrone = 4;
            static constexpr uint32 DrillSmash = 11;
            static constexpr uint32 BigRedRocket = 12;
            static constexpr uint32 Skyscorchers = 13;
        }

        namespace Phases
        {
            static constexpr uint32 BigGuns = 0;   // stage one
            static constexpr uint32 Drill = 1;     // stage two, <= 50% hp
        }

        // creature_text GroupIDs (canonical layout, extended: 11 = Big Red Rocket
        // calls, 12 = death line - neither exists in the 0-10 set)
        namespace Voice
        {
            static constexpr uint8 Aggro = 0;
            static constexpr uint8 Slay = 1;
            static constexpr uint8 GatlingGunMin = 2;     // callouts 2-3, pick randomly
            static constexpr uint8 GatlingGunMax = 3;
            static constexpr uint8 BoombaDrone = 4;
            static constexpr uint8 DrillSmashEmote = 6;   // targeted warning emote
            static constexpr uint8 Insurance = 7;         // stage-two transition
            static constexpr uint8 DrillSmashMin = 5;     // callouts 5, 8, 9 - pick randomly
            static constexpr uint8 DrillSmashAlt1 = 8;
            static constexpr uint8 DrillSmashAlt2 = 9;
            static constexpr uint8 YoullPay = 10;         // alternate slay taunt
            static constexpr uint8 BigRedRocket = 11;
            static constexpr uint8 Death = 12;
        }
    }

    // Deals damage as a share of the victim's max health while attributing the combat-log
    // entry to `spellId` - used where the real spell cannot carry the scripted effect.
    namespace Combat
    {
        inline void PercentHit(Unit* source, Unit* victim, uint32 spellId, float fraction, SpellSchoolMask school)
        {
            if (!source || !victim || !victim->IsAlive())
                return;

            SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId, source->GetMap()->GetDifficultyID());
            uint32 amount = uint32(std::max<uint64>(1, victim->CountPctFromMaxHealth(int32(std::max(1.0f, fraction * 100.0f)))));
            if (!info)
            {
                Unit::DealDamage(source, victim, amount, nullptr, SPELL_DIRECT_DAMAGE, school);
                return;
            }

            SpellNonMeleeDamage log(source, victim, info, { info->GetSpellXSpellVisualId(source), 0 }, school);
            log.damage = amount;
            Unit::DealDamageMods(source, victim, log.damage, &log.absorb);
            source->SendSpellNonMeleeDamageLog(&log);
            source->DealSpellDamage(&log, true);
        }
    }
}

#endif // CUSTOM_MOTHERLODE_DEFINES_H
