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

namespace Scripts::Custom::VaultOfTheWardens
{
    namespace Creatures
    {
        // "Breaking Out" (38672) - Vault of Silence
        static constexpr uint32 MaievShadowsong    = 92718;
        static constexpr uint32 WardOfTheHunters   = 100168;
        static constexpr uint32 AltruisCell        = 103655;
        static constexpr uint32 KaynCell           = 103658;
        static constexpr uint32 AltruisTheSufferer = 99632;
        static constexpr uint32 KaynSunfury        = 99631;
        // post-reward questgiver entries at the main-hall door (the clones
        // spawned at the rally point replace these statics and run to them)
        static constexpr uint32 AltruisTheSuffererPostReward = 92986;
        static constexpr uint32 KaynSunfuryPostReward        = 92980;
        // "Rise of the Illidari" (38690) - named Illidari freed from the warden cells
        static constexpr uint32 AllariTheSouleater = 97614;
        static constexpr uint32 JaceDarkweaver     = 97615;
        static constexpr uint32 CyanaNightglaive   = 97616;
        static constexpr uint32 AshaRavensong      = 97617;
        static constexpr uint32 FalaraNightsong    = 97618;
        static constexpr uint32 MariusFelbane      = 97619;
        static constexpr uint32 LyanaDarksorrow    = 97620;
        static constexpr uint32 KorvasBloodthorn   = 97622;
        // Illidari Ward mini-boss - demon rare fighting the wardens in the cell corridor
        static constexpr uint32 WrathLordLekos = 97069;
        // warden fighting the demon waves in the Vault of Silence
        static constexpr uint32 DrelanimWhisperwind = 96847;
        // demon trash waves - their deaths release souls for "Fel Infusion" (38689)
        static constexpr uint32 SavageFelstalker     = 92782;
        static constexpr uint32 FelShocktrooper      = 92776;
        static constexpr uint32 MariusFelbaneFighter = 100298; // combat clone of 97619
        // kill-credit proxy for quest 38689 ("Fel Energy regained" x100)
        static constexpr uint32 SoulFragment = 89297;

        static constexpr uint32 MiranaStarlight      = 99451;
    }

    namespace GameObjects
    {
        // "Rise of the Illidari" (38690) - the 8 interactable warden cells
        static constexpr uint32 WardenCell = 244588;
    }

    namespace Maps
    {
        static constexpr uint32 WardenPrisonDH = 1468;
    }

    namespace Areas
    {
        static constexpr uint32 VaultOfSilence = 7873;
    }

    namespace Quests
    {
        static constexpr uint32 BreakingOut            = 38672;
        // "Vault of the Wardens" (39742) - bonus quest added to the log when
        // 38672 is rewarded (sniffed quest-log update in the reward packet)
        static constexpr uint32 VaultOfTheWardensQuest = 39742;
        // follow-up quests accepted from the hall statics 92980 / 92986
        static constexpr uint32 RiseOfTheIllidari = 38690; // Kayn
        static constexpr uint32 FelInfusion       = 38689; // Altruis
    }

    namespace Texts
    {
        // creature_text GroupID 0
        static constexpr uint8 SayMaievFreed   = 0; // 92718
        static constexpr uint8 SayAltruisFreed = 0; // 99632
        static constexpr uint8 SayKaynFreed    = 0; // 99631
        // creature_text GroupID 1 - post-completion exchange at the rally point
        static constexpr uint8 SayKaynRally    = 1; // 99631
        static constexpr uint8 SayAltruisRally = 1; // 99632
        // creature_text GroupID 0 - post-reward clones (92980 / 92986)
        static constexpr uint8 SayKaynPostReward    = 0; // 92980 "Why would Maiev free us?..."
        static constexpr uint8 SayAltruisPostReward = 0; // 92986 "The Legion is here."
        // creature_text GroupID 1 - quest-accept clones (92980 / 92986)
        static constexpr uint8 SayKaynAccept    = 1; // 92980 "The demons aim to destroy our world..."
        static constexpr uint8 SayAltruisAccept = 1; // 92986 "Time for answers later..."
        // creature_text GroupID 0 - freed named Illidari whisper (97614-97622)
        static constexpr uint8 SayNamedIllidariFreed = 0;
        // creature_text GroupID 0 - Wrath-Lord Lekos (97069) death line
        // 'All goes according to HIS plan.' (sniffed on his death blow)
        static constexpr uint8 SayLekosDeath = 0;
        // creature_text GroupID 1 - Wrath-Lord Lekos (97069) player-engage line
        // 'Lord Gul'dan will lead us to victory. You cannot stop us.'
        static constexpr uint8 SayLekosAggro = 1;
        // creature_text GroupID 0 - Drelanim Whisperwind (96847)
        // 'I hope Maiev was right about freeing you, demon hunter. Go on - I
        // will hold the line.' (sniffed whisper when the player crosses the
        // Vault of Silence areatrigger zone)
        static constexpr uint8 SayDrelanimHoldTheLine = 0;
    }

    namespace AnimKits
    {
        // "Breaking Out" (38672) - ward dissolve anim played by the personal ward clone
        static constexpr uint16 WardOpen     = 8930;
        // reward-time ward clone one-shot anim (sniffed SMSG_PLAY_ONE_SHOT_ANIM_KIT)
        static constexpr uint16 WardDissolve = 3761;
        // prison cell cage-open anim played by the personal cell clone
        static constexpr uint16 CellOpen = 22328;
    }

    namespace Spells
    {
        // "Glaive Anim Replacement" - self-cast by the freed prisoner after
        // picking up their warglaives (sniffed SPELL_START on the private clone)
        static constexpr uint32 GlaiveAnimReplacement = 222589;
        // Wrath-Lord Lekos (97069) - spell rotation (sniffed casts, all mechanics
        // are native to the spell data - no spell scripts needed)
        static constexpr uint32 SpikedBulwark         = 200936; // 10s uninterruptible channel: frontal 100% block + reflects 200950 at attackers
        static constexpr uint32 FuriousRoar           = 200929; // instant AoE Physical damage + dummy aura on nearby enemies
        static constexpr uint32 FelShieldBlastInstant = 200919; // instant shield-toss visual, cast same tick as 200918
        static constexpr uint32 FelShieldBlast        = 200918; // 2.5s cast at victim: Fire damage + frontal knockback
        // Legion Portal (99501) - sniffed lifecycle
        static constexpr uint32 DestroyingLegionPortal = 202064; // spellclick cast the player lands on the portal
        static constexpr uint32 PortalExplosion        = 196084; // portal self-cast after 202064 hits (visuals are spell-side)
        static constexpr uint32 LegionPortalSpawn      = 158342; // instant self-cast at spawn, "finished opening" signal
        // Mirana Starlight (99451) - Throwing Blade: the parent cast's DUMMY
        // effects hold the missile variants in their base points; one is picked
        // per target (sniffed pairs are never both)
        static constexpr uint32 ThrowingBlade          = 191029;
        static constexpr uint32 ThrowingBladeMissile    = 190898;
        static constexpr uint32 ThrowingBladeMissileAlt = 191026;
        // Drelanim Whisperwind whisper gate - retail self-casts this 15-min
        // dummy aura when crossing the Vault of Silence AT; its presence is
        // the per-player cooldown for the talk
        static constexpr uint32 CsaAreaTriggerDummyTimerAuraB = 138041;
        // "Fel Infusion" (38689) soul absorb - sniffed chain: the player gains
        // 133508 on accept, dead demons self-cast 133578, and the player casts
        // 202154 which parents one 133511 kill credit per absorbed soul
        static constexpr uint32 FelInfusionAura       = 133508;
        static constexpr uint32 FelInfusionKillCredit = 133511;
        static constexpr uint32 FillFelForcecast      = 133578;
        static constexpr uint32 FillFel               = 202154;
        // 133509 energizes the player's alternate power bar: child of 133508
        // once on accept (amount 0 - empty bar), then one cast per 202154 wave
        // energizing by the number of souls absorbed
        static constexpr uint32 FillFelBar            = 133509;
    }

    namespace Events
    {
        // Wrath-Lord Lekos (97069) - EventMap IDs
        static constexpr int8 LekosSpikedBulwark  = 1;
        static constexpr int8 LekosFuriousRoar    = 2;
        static constexpr int8 LekosFelShieldBlast = 3;
    }

    namespace KillCredits
    {
        // "Breaking Out" (38672) objectives - sniffed SMSG_QUEST_UPDATE_ADD_CREDIT ObjectIDs
        static constexpr uint32 AltruisFreed   = 92849;  // objective 278274 - granted when the cell-open cast ends
        static constexpr uint32 KaynFreed      = 92848;  // objective 278275 - granted when the cell-open cast ends
        static constexpr uint32 AltruisWeapons = 112277; // objective 286137 - Altruis's Warglaives (kneel)
        static constexpr uint32 KaynWeapons    = 112276; // objective 286136 - Kayn's Warglaives (kneel)
        static constexpr uint32 AltruisEnd     = 112287; // objective 286143 - Altruis reaches the rally point
        static constexpr uint32 KaynEnd        = 99326;  // objective 280526 - Kayn reaches the rally point
        // "Rise of the Illidari" (38690) - sniffed SMSG_QUEST_UPDATE_ADD_CREDIT ObjectID
        static constexpr uint32 ImprisonedIllidariFreed = 96762; // objective 278285 - generic counter, granted per cell use
    }

    namespace Phases
    {
        // added when the freed prisoner reaches the rally point (sniffed phase swap)
        static constexpr uint32 AltruisFreed = 5124;
        static constexpr uint32 KaynFreed    = 5125;
        // sealed-door phase (ward static + collision wall) - dropped at reward
        static constexpr uint32 WardSealed   = 5966;
    }

    namespace Paths
    {
        // "Breaking Out" (38672) - freed Maiev walks out of her cell (sniffed @MOVID+4)
        static constexpr uint32 MaievVaultOfSilenceFreed = 9271800;
        // freed prisoners: cell -> warglaives, then warglaives -> rally point
        static constexpr uint32 AltruisToWeapons = 9963200;
        static constexpr uint32 AltruisToRally   = 9963201;
        static constexpr uint32 KaynToWeapons    = 9963100;
        static constexpr uint32 KaynToRally      = 9963101;
        // post-reward clones run from the rally point to the main-hall door
        static constexpr uint32 KaynPostReward    = 9298000;
        static constexpr uint32 AltruisPostReward = 9298600;
        // quest-accept clones run from the hall statics into the north-hall battle
        static constexpr uint32 KaynAccept    = 9298001; // east flank (4335.8, -503.7)
        static constexpr uint32 AltruisAccept = 9298601; // west flank (4312.2, -500.9)

        // "Rise of the Illidari" (38690) - freed named Illidari walk from their
        // warden cell to the east-hall rally (sniffed @MOVID+41/42/45/50/59/63/68/73)
        static constexpr uint32 AllariFreed = 9761400;
        static constexpr uint32 JaceFreed   = 9761500;
        static constexpr uint32 CyanaFreed  = 9761600;
        static constexpr uint32 AshaFreed   = 9761700;
        static constexpr uint32 FalaraFreed = 9761800;
        static constexpr uint32 MariusFreed = 9761900;
        static constexpr uint32 LyanaFreed  = 9762000;
        static constexpr uint32 KorvasFreed = 9762200;
    }

    namespace Waypoints
    {
        // last node id of each leg (multi-node paths fire MovementInform per node)
        static constexpr uint32 AltruisToWeaponsLastNode = 3;
        static constexpr uint32 KaynToWeaponsLastNode    = 4;
        static constexpr uint32 ToRallyLastNode          = 2;
        // post-reward paths to the main-hall door
        static constexpr uint32 KaynPostRewardLastNode    = 1;
        static constexpr uint32 AltruisPostRewardLastNode = 14;
        // quest-accept paths into the north-hall battle
        static constexpr uint32 KaynAcceptLastNode    = 5;
        static constexpr uint32 AltruisAcceptLastNode = 30;
    }

    namespace Positions
    {
        // in-cell spawn spots of the freed-prisoner clones (sniffed create data;
        // retail has no in-cell statics - the cell model shows the caged prisoner)
        static constexpr Position AltruisCellPrisoner = { 4309.94f, -589.618f, -281.407f, 6.131255f };
        static constexpr Position KaynCellPrisoner    = { 4343.16f, -589.579f, -281.408f, 3.38493f };
        // rally-point spawn spots of the post-reward clones (sniffed create data)
        static constexpr Position KaynPostRewardSpawn    = { 4328.46f, -583.66f, -281.924f, 3.36849f };
        static constexpr Position AltruisPostRewardSpawn = { 4323.64f, -583.668f, -281.924f, 6.12611f };
    }
}
