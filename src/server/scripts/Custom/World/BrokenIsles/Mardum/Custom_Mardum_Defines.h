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

#include "Define.h"
#include "Position.h"

namespace Scripts::Custom::Mardum
{
    namespace Creatures
    {
        static constexpr uint32 KaynSunfury = 98229;
        static constexpr uint32 SevisBrightflameAshtongue = 99916;
        static constexpr uint32 FelSpreader = 97142;
        static constexpr uint32 FirstFelSpreader = 97154;
        static constexpr uint32 LegionCommunicator1 = 102223;
        static constexpr uint32 LegionCommunicator2 = 102224;
        static constexpr uint32 CyanaNightglaive    = 94377;
        static constexpr uint32 BelathDawnblade     = 94400;
        static constexpr uint32 SevisBrightflameCoilskar = 99917;
        static constexpr uint32 MannethrelDarkstar  = 93230;
        static constexpr uint32 IzalWhitemoon       = 93117;
        static constexpr uint32 AshtongueMystic     = 99914;
        static constexpr uint32 ELMGeneralPurposeBunny = 24021;
        static constexpr uint32 JaceDarkweaver      = 93759;
        static constexpr uint32 CoilskarSeaCaller   = 96884;
        static constexpr uint32 ShivarraDestroyer   = 96888;
        static constexpr uint32 DoomCommanderBeliash = 93221;
        static constexpr uint32 QueenTyranna        = 95048; // RP version beside Beliash; 93802 is the boss at the Seat of Command
        static constexpr uint32 BeliashKillCredit   = 106003;
        static constexpr uint32 LegionDevastator    = 100161;
        static constexpr uint32 LegionCommandCenter = 98191;
        static constexpr uint32 FelCannon           = 99003; // summoned by spell 194781 (triggered by 194783)
        // "Stop the Bombardment" (38727) - Illidari Banner sabotage
        static constexpr uint32 AshtongueStalker       = 96562; // trigger, Banner Doom Fortress
        static constexpr uint32 PriestessOfDementia    = 96564; // trigger, Banner Forge of Corruption
        static constexpr uint32 CoilskarHarpooner      = 96561; // trigger, Banner Soul Engine
        static constexpr uint32 DevastatorDoomFortress      = 96732;
        static constexpr uint32 DevastatorForgeOfCorruption = 96731;
        static constexpr uint32 DevastatorSoulEngine        = 93762;
        static constexpr uint32 BannerKillCreditDoomFortress      = 96734;
        static constexpr uint32 BannerKillCreditForgeOfCorruption = 96733;
        static constexpr uint32 BannerKillCreditSoulEngine        = 96692;
        // "Their Numbers Are Legion" (38819)
        static constexpr uint32 DoomFortressStabilizer     = 101947;
        static constexpr uint32 FirstStabilizerKillCredit  = 97409;
        static constexpr uint32 SpireOfWoeFire             = 101318;
        static constexpr uint32 SpireOfWoeFire2            = 101321;
        static constexpr uint32 SpireOfWoeBunny            = 101320;
        static constexpr uint32 OverseerBrutarg            = 97057;
        static constexpr uint32 CountNefarious             = 97058;
        static constexpr uint32 WellOfSouls                = 102916;
        static constexpr uint32 GeneralVolroth             = 97370;
        static constexpr uint32 CoilskarMyrmidon           = 96502; // Volroth captive
        static constexpr uint32 CoilskarSeaCallerCaptive   = 96503; // Volroth captive
        static constexpr uint32 SummonedAshtongueStalker   = 96877; // player companion summoned by 191662
        static constexpr uint32 WoundedDemonHunter         = 100549;
        static constexpr uint32 WoundedDemonHunter2        = 100545;
        static constexpr uint32 CreditWoundedDemonHunter   = 97395;
        static constexpr uint32 KorvasBloodthorn           = 98711;
        static constexpr uint32 Prolifica                  = 98986;
        static constexpr uint32 FelGeyser                  = 100257;
        static constexpr uint32 LadyStheno                 = 97676;
        static constexpr uint32 VileSoulmaster             = 102724;
        static constexpr uint32 VileSoulmaster2            = 97014;
        static constexpr uint32 BurningSoulstalker         = 96278;
        // "On Felbat Wings" (39663) - Illidari Foothold versions
        static constexpr uint32 KaynFoothold   = 93127;
        static constexpr uint32 CyanaFoothold  = 96420;
        static constexpr uint32 KorvasFoothold = 99045;
        static constexpr uint32 AllariFoothold = 96655;
        // "The Keystone" (38728) - Fel Hammer versions
        static constexpr uint32 CyanaFelHammer   = 97297;
        static constexpr uint32 KorvasFelHammer  = 98712;
        static constexpr uint32 JaceFelHammer    = 97959;
        static constexpr uint32 KaynFelHammer    = 97244;
        static constexpr uint32 AllariFelHammer  = 97962;
        static constexpr uint32 BroodQueenTyranna = 93802; // boss on the Fel Hammer; QueenTyranna (95048) is the Beliash RP version
        static constexpr uint32 SkitteringBroodling = 100333;
        static constexpr uint32 TyrannasSpawn       = 100334;
        static constexpr uint32 KeystoneKillCredit  = 101760;
        static constexpr uint32 KaynKeystone        = 97303;
        static constexpr uint32 KeystoneFelBeamBunny    = 23837;  // ELM General Purpose Bunny (phase 0 pair beside the keystone)
        static constexpr uint32 KeystoneActivatedCredit = 100651;
    }

    namespace Factions
    {
        static constexpr uint32 Illidari = 2843; // friendly faction for freed Illidari
    }

    namespace Maps
    {
        static constexpr uint32 Mardum = 1481;
    }

    namespace Areas
    {
        static constexpr uint32 DespairRidge = 7741;
        static constexpr uint32 MoltenShore  = 7740;
    }

    namespace GameObjects
    {
        static constexpr uint32 LegionCommunicator1 = 244439;
        static constexpr uint32 LegionCommunicator2 = 244440;
        static constexpr uint32 BannerDoomFortress      = 243968;
        static constexpr uint32 BannerForgeOfCorruption = 243967;
        static constexpr uint32 BannerSoulEngine        = 243965;
        static constexpr uint32 SpireStabilizer         = 245995;
        static constexpr uint32 CylinderCollision       = 246280;
        static constexpr uint32 SecretTome              = 245169;
        static constexpr uint32 SargeriteKeystone       = 245728;
        static constexpr uint32 FelPortal               = 244466;
    }

    namespace Positions
    {
        static constexpr Position SargeriteKeystone = { 1629.44f, 1414.21f, 210.363f, 0.f };
    }

    namespace Scenes
    {
        static constexpr uint32 TheInvasionBeginsBannerPlanted = 1116;
    }

    namespace Quests
    {
        static constexpr uint32 TheInvasionBegins         = 40077;
        static constexpr uint32 EnterTheIllidariAshtongue = 40378;
        static constexpr uint32 EnterTheIllidariCoilskar  = 40379;
        static constexpr uint32 EnterTheIllidariShivarra  = 38765;
        static constexpr uint32 AssaultOnMardum           = 39279;
        static constexpr uint32 SetThemFree               = 38759;
        static constexpr uint32 EyeOnThePrize             = 39049;
        static constexpr uint32 MeetingWithTheQueen       = 39050;
        static constexpr uint32 BeforeWereOverun          = 38766;
        static constexpr uint32 SevisSacrificeTracker     = 40087;
        static constexpr uint32 StopTheBombardment        = 38727;
        static constexpr uint32 TheirNumbersAreLegion     = 38819;
        static constexpr uint32 IntoTheFoulCreche         = 38725;
        static constexpr uint32 TheImpMothersTome         = 40222;
        static constexpr uint32 OnFelbatWings             = 39663;
        static constexpr uint32 TheKeystone               = 38728;
        static constexpr uint32 ReturnToTheBlackTemple    = 38729;
    }

    namespace Objectives
    {
        static constexpr uint32 FelSpreaderDestroyed      = 279930; // quest 39279
        static constexpr uint32 LegionCommunicator1       = 281333; // quest 39279
        static constexpr uint32 LegionCommunicator2       = 281334; // quest 39279
        static constexpr uint32 SoulSacrificed            = 280770; // quest 40379
        static constexpr uint32 CoilskarForces            = 280771; // quest 40379
        static constexpr uint32 FirstStabilizerDestroyed  = 280006; // quest 38819
        static constexpr uint32 KeystoneMakeYourWayDownstairs = 278675; // quest 38728
    }

    namespace Spells
    {
        static constexpr uint32 DestroyingFelSpreader  = 191827;
        static constexpr uint32 FelSpreaderExplosion   = 199617;
        static constexpr uint32 AshtongueMysticSacrifice = 196724;
        static constexpr uint32 PermanentFeignDeath    = 159474;
        static constexpr uint32 ShivarraSoulMissiles02 = 191664;
        static constexpr uint32 FelChannelling         = 188485;
        static constexpr uint32 SummonCoilskarSeaCaller  = 191668;
        static constexpr uint32 SeaCallerLightningBolt   = 197745;
        static constexpr uint32 SeaCallerHealingWave     = 197744;
        static constexpr uint32 ShadowBlaze              = 195401;
        static constexpr uint32 ShadowBoltVolley         = 196403;
        static constexpr uint32 ShadowRetreat            = 196625;
        static constexpr uint32 BroodQueenTyrannaTeleport = 188658;
        static constexpr uint32 TheKeystoneQuestCompletion = 193387; // 'The Keystone: Quest Completion - Play Metamorphosis Scene'
        static constexpr uint32 KeystoneFelBeam            = 189952;
        static constexpr uint32 BackToBlackTemple          = 192140;
        static constexpr uint32 TeleportVaultOfWardens     = 205395;
        static constexpr uint32 FelBombardment         = 194689;
        static constexpr uint32 CommandCenterFelBeam   = 194783; // instant self-cast, triggers 194781 (Fel Cannon summon)
        static constexpr uint32 CommandCenterFelBeamChannel = 194810; // 5s channel on the summoned Fel Cannon
        static constexpr uint32 FelBeamDamage            = 194811; // self-cast by units inside areatrigger 5127
        static constexpr uint32 AcidSpit               = 198163;
        static constexpr uint32 StopBombardmentDevastator = 191454;
        static constexpr uint32 StopBombardmentKnockback  = 198908;
        static constexpr uint32 DestroyingStabilizer      = 201112;
        static constexpr uint32 StabilizerFelExplosion    = 202948;
        static constexpr uint32 SpireOfWoeFelBeam         = 199707;
        static constexpr uint32 BrutargCharge                  = 198497;
        static constexpr uint32 BrutargEquipSword              = 198573;
        static constexpr uint32 BrutargMeatGrinder             = 198518;
        static constexpr uint32 BrutargDizzy                   = 198524;
        static constexpr uint32 SummonShivarraDestroyer        = 191671;
        static constexpr uint32 SummonMatronMotherMalevolence  = 203369;
        static constexpr uint32 WoundedDemonHunterRescue       = 192262;
        static constexpr uint32 NefariousVampiricCleave        = 198894;
        static constexpr uint32 NefariousCarrionStorm          = 198947;
        static constexpr uint32 NefariousCarrionStormVisual    = 199032;
        static constexpr uint32 NefariousCarrionStormAura      = 199035; // applied by AT 10195
        static constexpr uint32 NefariousFelShield             = 191170; // 199103;
        static constexpr uint32 WellOfSoulsOpening             = 198185;
        static constexpr uint32 NefariousTorrentOfSouls        = 192714;
        static constexpr uint32 NefariousTorrentOfSoulsBeam    = 192715;
        static constexpr uint32 IllidariReinforcements         = 191466; // periodic dummy, summons one of the 3 Illidari helpers in combat
        static constexpr uint32 SummonAshtongueStalker         = 191662;
        static constexpr uint32 VolrothBurnWithin              = 197749;
        static constexpr uint32 VolrothFelBreath               = 197750;
        static constexpr uint32 ProlificaFelGeyser             = 197217;
        static constexpr uint32 ProlificaGaseousBreath         = 197240;
        static constexpr uint32 CoilskarThrowSpear             = 198723;
        static constexpr uint32 SthenoAqueousShield            = 201268;
        static constexpr uint32 FelSecretsHavocChosen          = 194939;
        static constexpr uint32 FelSecretsVengeanceChosen      = 194940;
        static constexpr uint32 QueensBite                     = 197486;
        static constexpr uint32 BroodSwarm                     = 197627;
        static constexpr uint32 IntoTheShadows                 = 197414;
    }

    namespace Conversations
    {
        static constexpr uint32 FelSpreaderDestroyed       = 581;
        static constexpr uint32 LegionCommunicatorReport1  = 558;
        static constexpr uint32 LegionCommunicatorReport2  = 583;
        static constexpr uint32 DoomCommanderBeliash       = 531;
        static constexpr uint32 FelBombardments            = 747;
        static constexpr uint32 IllidariFoothold           = 569;
        static constexpr uint32 KingVorasDeath             = 567;
        static constexpr uint32 DoomFortressStabilizer     = 573;
        static constexpr uint32 CountNefarious             = 576;
        static constexpr uint32 ProlificaDeath             = 585;
    }

    namespace PlayerChoices
    {
        // PlayerChoice 231 - Fel Secrets (quest 40051) - ResponseIds
        static constexpr uint32 FelSecretsHavocResponse     = 478;
        static constexpr uint32 FelSecretsVengeanceResponse = 479;
    }

    namespace Movies
    {
        // Movie.db2 IDs played by SPELL_EFFECT_PLAY_MOVIE
        static constexpr uint32 BackToBlackTemple = 471; // triggered by spell 192140
    }

    namespace CreatureText
    {
        // Kayn Sunfury (98229) - creature_text GroupIDs (Mardum_CreatureText.sql)
        static constexpr uint8 KaynFindKeystone    = 2; // 'Cyana, Jace, Allari... find the keystone.'
        static constexpr uint8 KaynActivateGateway = 3; // 'Now, let's see about activating that gateway.'

        // Sevis Brightflame (99916) - creature_text GroupID
        static constexpr uint8 SevisAshtongueGreeting = 0;

        // Cyana Nightglaive (94377) - creature_text GroupID
        static constexpr uint8 CyanaCaptiveGreeting = 0;

        // Belath Dawnblade (94400) - creature_text GroupID
        static constexpr uint8 BelathCaptiveGreeting = 0;

        // Mannethrel Darkstar (93230) - creature_text GroupID
        static constexpr uint8 MannethrelCaptiveGreeting = 0;

        // Izal Whitemoon (93117) - creature_text GroupID
        static constexpr uint8 IzalCaptiveGreeting = 0;

        // Sevis Brightflame (99917) - creature_text GroupIDs
        static constexpr uint8 SevisCoilskarGreeting = 0;
        static constexpr uint8 SevisCoilskarFollowUp = 1;

        // Jace Darkweaver (93759) - creature_text GroupIDs (Mardum_CreatureText.sql)
        static constexpr uint8 JaceEyeOnThePrizeGreeting    = 0; // 'I sense greater power within you...'
        static constexpr uint8 JaceMeetingWithTheQueenAccept = 1; // 'Use the crucible to complete the ritual.'
        static constexpr uint8 JaceBeforeWereOverunAccept = 2; // 'Beliash is protected by...'
        static constexpr uint8 JaceBeforeWereOverunAccept2 = 3; // 'Good luck, $n. I'll see you up in the volcano.'

        // Coilskar Sea-Caller (96884) - creature_text GroupIDs
        static constexpr uint8 SeaCallerGreeting = 0; // 'Lady S'theno requested I join you.'

        // Doom Commander Beliash (93221) - creature_text GroupID
        static constexpr uint8 BeliashAggro = 0; // 'They will die.'
        static constexpr uint8 BeliashAggro2 = 1; // 'You wont survive'

        // Brood Queen Tyranna (95048) - creature_text GroupID
        static constexpr uint8 QueenTyrannaAggro = 0;

        // King Voras (97059) - creature_text GroupIDs
        static constexpr uint8 VorasAggro      = 0; // 'Want your little, broken draenei back?'
        static constexpr uint8 VorasLowHealth  = 1; // 'My queen's brood will hatch soon.'
        static constexpr uint8 VorasDeathEmote = 2; // 'Nearby [Spider Eggs] will hatch soon if not destroyed!'
        static constexpr uint8 VorasDeath      = 3; // 'They've slain me, my queen...'

        // Overseer Brutarg (97057) - creature_text GroupIDs
        static constexpr uint8 BrutargAggro      = 0;
        static constexpr uint8 BrutargEquipSword = 1;
        static constexpr uint8 BrutargDeath      = 2;

        // Wounded Demon Hunter (100549) - creature_text GroupID
        static constexpr uint8 WoundedDemonHunterAwakened = 0;

        // Count Nefarious (97058) - creature_text GroupIDs
        static constexpr uint8 NefariousAggro              = 0;
        static constexpr uint8 NefariousCarrionAfterCast   = 1;
        static constexpr uint8 NefariousCarrionBeforeCast  = 2;

        // General Volroth (97370) - creature_text GroupIDs
        static constexpr uint8 VolrothAggro      = 0;
        static constexpr uint8 VolrothBurnWithin = 1;
        static constexpr uint8 VolrothDeath      = 2;
        static constexpr uint8 VolrothCaptiveReleased = 0; // PLACEHOLDER: captive creature_text GroupID

        // Kor'vas Bloodthorn (98711) - creature_text GroupIDs
        static constexpr uint8 KorvasImpMothersTomeAccept  = 0;
        static constexpr uint8 KorvasImpMothersTomeAccept2 = 1;

        // Prolifica (98986) - creature_text GroupIDs
        static constexpr uint8 ProlificaAggro     = 0;
        static constexpr uint8 ProlificaLowHealth = 1;
        static constexpr uint8 ProlificaDeath     = 2;

        // Lady S'theno (97676) - creature_text GroupIDs
        static constexpr uint8 SthenoGreeting = 0;

        // Cyana Nightglaive (97297) - creature_text GroupIDs (Fel Hammer)
        static constexpr uint8 CyanaFelHammerAccept  = 0; // 'I'll secure the exit portal...'
        static constexpr uint8 CyanaFelHammerAccept2 = 1; // 'Hurry. Kayn has already launched the attack on Tyranna.'

        // Brood Queen Tyranna (93802) - creature_text GroupIDs (Fel Hammer)
        static constexpr uint8 TyrannaGreeting       = 0; // '$n! You and your demon hunters are nothing.'
        static constexpr uint8 TyrannaGreeting2      = 1; // 'The keystone was entrusted to me by Sargeras himself...'
        static constexpr uint8 TyrannaQueensBite     = 2; // 'None can resist my kiss.'
        static constexpr uint8 TyrannaBroodSwarm     = 3; // 'Come forth, my children...'
        static constexpr uint8 TyrannaIntoTheShadows = 4; // '%s disappears Into the Shadows!'
        static constexpr uint8 TyrannaDeath          = 5; // 'My Lord Sargeras, no!!!'
    }

    namespace Events
    {
        // Coilskar Sea-Caller (96884) - EventMap IDs
        static constexpr int8 SeaCallerGreeting      = 1;
        static constexpr int8 SeaCallerSpellRotation = 2;

        // Doom Commander Beliash (93221) - EventMap IDs
        static constexpr int8 BeliashShadowBlaze      = 1;
        static constexpr int8 BeliashShadowBoltVolley = 2;
        static constexpr int8 BeliashShadowRetreat    = 3;

        // Legion Devastator (100161) - EventMap IDs
        static constexpr int8 DevastatorBombardment             = 1;
        static constexpr int8 DevastatorBombardmentConversation = 2;

        // Legion Command Center (98191) - EventMap IDs
        static constexpr int8 CommandCenterBombardment     = 1;
        static constexpr int8 CommandCenterFelBeam         = 2;
        static constexpr int8 CommandCenterFelBeamChannel  = 3;

        // King Voras (97059) - EventMap IDs
        static constexpr int8 VorasAcidSpit = 1;

        // Overseer Brutarg (97057) - EventMap IDs
        static constexpr int8 BrutargMeatGrinder = 1;

        // Count Nefarious (97058) - EventMap IDs
        static constexpr int8 NefariousVampiricCleave = 1;
        static constexpr int8 NefariousCarrionStorm   = 2;
        static constexpr int8 NefariousFelShield      = 3;
        static constexpr int8 NefariousConversation   = 4;

        // General Volroth (97370) - EventMap IDs
        static constexpr int8 VolrothBurnWithin = 1;
        static constexpr int8 VolrothFelBreath  = 2;

        // Prolifica (98986) - EventMap IDs
        static constexpr int8 ProlificaFelGeyser     = 1;
        static constexpr int8 ProlificaGaseousBreath = 2;

        // Lady S'theno (97676) - EventMap IDs
        static constexpr int8 SthenoAqueousShield = 1;
        static constexpr int8 SthenoHealingWave   = 2;

        // Brood Queen Tyranna (93802) - EventMap IDs
        static constexpr int8 TyrannaQueensBite = 1;
        static constexpr int8 TyrannaBroodSwarm = 2;
    }

    namespace Movement
    {
        static constexpr uint32 ReachedDevastator = 1;
        static constexpr uint32 CaptivesDespawn = 2;
    }

    namespace Misc
    {
        static constexpr float SearchRange         = 50.0f;
        static constexpr float FelGeyserSearchRange = 30.0f;
        static constexpr uint32 SthenoAttackerCount    = 5;
        static constexpr float  SthenoAttackerSpawnRange = 5.0f;
        static constexpr float TalkDistance        = 5.0f;
        static constexpr float DevastatorStandoff  = 5.0f;
        static constexpr uint32 BannerTriggered    = 1; // SetData id: banner used -> trigger npc
        static constexpr uint32 WellOfSoulsOpened  = 1; // SetData id: well of souls opened -> count nefarious
        static constexpr float CaptiveGreetingRange = 10.0f;
        static constexpr float JaceGreetingRange    = 10.0f;
        static constexpr float BeliashConversationRange = 100.0f;
        static constexpr float BombardmentRange         = 450.0f;
        static constexpr float FelCannonSearchRange      = 550.0f; // 194783 picks a player within 400y + slack
        static constexpr float LegionCCFlightSpeed       = 7.0f;
        // sniff: the Command Center orbits a perfect circle, all 16 spline nodes at
        // exactly r=400 around this center, flat Z, clockwise
        static constexpr float LegionCCOrbitCenterX      = 1075.2f;
        static constexpr float LegionCCOrbitCenterY      = 2646.54f;
        static constexpr float LegionCCOrbitCenterZ      = 243.1888f;
        static constexpr float LegionCCOrbitRadius       = 400.0f;
        static constexpr uint8 LegionCCOrbitSteps        = 16;
        static constexpr uint32 MysticGossipMenu         = 19015;
        static constexpr uint32 MysticVisualKit          = 25111;
        static constexpr uint32 MysticAnimKit            = 9256;
        static constexpr uint32 SoulMissileTargetSpawnId = 6000619;
        static constexpr uint32 SoulMissileTargetEntry   = 24021;
        static constexpr uint32 FelSaberMountDisplay       = 64385;
        static constexpr uint32 FelbatWingsPathId        = 9312700;
        static constexpr float  FelbatWingsFollowDist    = 2.0f;
        static constexpr float  FelHammerSearchRange     = 150.0f; // Cyana -> Tyranna is ~103y on the Fel Hammer deck
        static constexpr float  TyrannaBiteTargetRange   = 50.0f;
        static constexpr float  KeystoneKillCreditRange  = 10.0f;
        static constexpr float  KeystoneBunnySearchRange = 30.0f;
        static constexpr uint32 KeystoneReturnBindArea   = 7873;
    }
}
