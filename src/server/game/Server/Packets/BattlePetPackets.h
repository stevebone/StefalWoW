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

#ifndef TRINITYCORE_BATTLE_PET_PACKETS_H
#define TRINITYCORE_BATTLE_PET_PACKETS_H

#include "Packet.h"
#include "PacketUtilities.h"
#include "ObjectGuid.h"
#include "Optional.h"
#include "Position.h"
#include "UnitDefines.h"
#include <array>
#include <memory>

namespace WorldPackets
{
    namespace BattlePet
    {
        struct BattlePetOwnerInfo
        {
            ObjectGuid Guid;
            uint32 PlayerVirtualRealm = 0;
            uint32 PlayerNativeRealm = 0;
        };

        struct BattlePet
        {
            ObjectGuid Guid;
            uint32 Species = 0;
            uint32 CreatureID = 0;
            uint32 DisplayID = 0;
            uint16 Breed = 0;
            uint16 Level = 0;
            uint16 Exp = 0;
            uint16 Flags = 0;
            uint32 Power = 0;
            uint32 Health = 0;
            uint32 MaxHealth = 0;
            uint32 Speed = 0;
            uint8 Quality = 0;
            Optional<BattlePetOwnerInfo> OwnerInfo;
            std::string Name;
            bool NoRename = false;
        };

        struct BattlePetSlot
        {
            BattlePet Pet;
            uint32 CollarID = 0;
            uint8 Index = 0;
            bool Locked = true;
        };

        class BattlePetJournal final : public ServerPacket
        {
        public:
            BattlePetJournal() : ServerPacket(SMSG_BATTLE_PET_JOURNAL) { }

            WorldPacket const* Write() override;

            uint16 Trap = 0;
            bool HasJournalLock = false;
            std::vector<std::reference_wrapper<BattlePetSlot>> Slots;
            std::vector<std::reference_wrapper<BattlePet>> Pets;
        };

        class BattlePetJournalLockAcquired final : public ServerPacket
        {
        public:
            BattlePetJournalLockAcquired() : ServerPacket(SMSG_BATTLE_PET_JOURNAL_LOCK_ACQUIRED, 0) { }

            WorldPacket const* Write() override { return &_worldPacket; }
        };

        class BattlePetJournalLockDenied final : public ServerPacket
        {
        public:
            BattlePetJournalLockDenied() : ServerPacket(SMSG_BATTLE_PET_JOURNAL_LOCK_DENIED, 0) { }

            WorldPacket const* Write() override { return &_worldPacket; }
        };

        class BattlePetRequestJournal final : public ClientPacket
        {
        public:
            explicit BattlePetRequestJournal(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PET_REQUEST_JOURNAL, std::move(packet)) { }

            void Read() override { }
        };

        class BattlePetRequestJournalLock final : public ClientPacket
        {
        public:
            explicit BattlePetRequestJournalLock(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PET_REQUEST_JOURNAL_LOCK, std::move(packet)) { }

            void Read() override { }
        };

        class BattlePetUpdates final : public ServerPacket
        {
        public:
            BattlePetUpdates() : ServerPacket(SMSG_BATTLE_PET_UPDATES) { }

            WorldPacket const* Write() override;

            std::vector<std::reference_wrapper<BattlePet const>> Pets;
            bool PetAdded = false;
        };

        class PetBattleSlotUpdates final : public ServerPacket
        {
        public:
            PetBattleSlotUpdates() : ServerPacket(SMSG_PET_BATTLE_SLOT_UPDATES) { }

            WorldPacket const* Write() override;

            std::vector<BattlePetSlot> Slots;
            bool AutoSlotted = false;
            bool NewSlot = false;
        };

        class BattlePetSetBattleSlot final : public ClientPacket
        {
        public:
            explicit BattlePetSetBattleSlot(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PET_SET_BATTLE_SLOT, std::move(packet)) { }

            void Read() override;

            ObjectGuid PetGuid;
            uint8 Slot = 0;
        };

        class BattlePetModifyName final : public ClientPacket
        {
        public:
            explicit BattlePetModifyName(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PET_MODIFY_NAME, std::move(packet)) { }

            void Read() override;

            ObjectGuid PetGuid;
            std::string Name;
            std::unique_ptr<DeclinedName> DeclinedNames;
        };

        class QueryBattlePetName final : public ClientPacket
        {
        public:
            explicit QueryBattlePetName(WorldPacket&& packet) : ClientPacket(CMSG_QUERY_BATTLE_PET_NAME, std::move(packet)) { }

            void Read() override;

            ObjectGuid BattlePetID;
            ObjectGuid UnitGUID;
        };

        class QueryBattlePetNameResponse final : public ServerPacket
        {
        public:
            QueryBattlePetNameResponse() : ServerPacket(SMSG_QUERY_BATTLE_PET_NAME_RESPONSE) { }

            WorldPacket const* Write() override;

            ObjectGuid BattlePetID;
            int32 CreatureID = 0;
            WorldPackets::Timestamp<> Timestamp;
            bool Allow = false;

            bool HasDeclined = false;
            DeclinedName DeclinedNames;
            std::string Name;
        };

        class BattlePetDeletePet final : public ClientPacket
        {
        public:
            explicit BattlePetDeletePet(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PET_DELETE_PET, std::move(packet)) { }

            void Read() override;

            ObjectGuid PetGuid;
        };

        class BattlePetSetFlags final : public ClientPacket
        {
        public:
            explicit BattlePetSetFlags(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PET_SET_FLAGS, std::move(packet)) { }

            void Read() override;

            ObjectGuid PetGuid;
            uint16 Flags = 0;
            uint8 ControlType = 0;
        };

        class BattlePetClearFanfare final : public ClientPacket
        {
        public:
            explicit BattlePetClearFanfare(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PET_CLEAR_FANFARE, std::move(packet)) { }

            void Read() override;

            ObjectGuid PetGuid;
        };

        class CageBattlePet final : public ClientPacket
        {
        public:
            explicit CageBattlePet(WorldPacket&& packet) : ClientPacket(CMSG_CAGE_BATTLE_PET, std::move(packet)) { }

            void Read() override;

            ObjectGuid PetGuid;
        };

        class BattlePetDeleted final : public ServerPacket
        {
        public:
            BattlePetDeleted() : ServerPacket(SMSG_BATTLE_PET_DELETED, 16) { }

            WorldPacket const* Write() override;

            ObjectGuid PetGuid;
        };

        class BattlePetError final : public ServerPacket
        {
        public:
            BattlePetError() : ServerPacket(SMSG_BATTLE_PET_ERROR, 5) { }

            WorldPacket const* Write() override;

            uint8 Result = 0;
            int32 CreatureID = 0;
        };

        class BattlePetSummon final : public ClientPacket
        {
        public:
            explicit BattlePetSummon(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PET_SUMMON, std::move(packet)) { }

            void Read() override;

            ObjectGuid PetGuid;
        };

        class BattlePetUpdateNotify final : public ClientPacket
        {
        public:
            explicit BattlePetUpdateNotify(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PET_UPDATE_NOTIFY, std::move(packet)) { }

            void Read() override;

            ObjectGuid PetGuid;
        };

        class BattlePetUpdateDisplayNotify final : public ClientPacket
        {
        public:
            explicit BattlePetUpdateDisplayNotify(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PET_UPDATE_DISPLAY_NOTIFY, std::move(packet)) { }

            void Read() override;

            ObjectGuid PetGuid;
        };

        // ============================================================================
        // Pet Battle Combat Packets (turn-based battle system)
        // ============================================================================

        // Shared sub-structures for pet battle packets
        struct PetBattleAbilityInfo
        {
            int32 AbilityID = 0;
            int16 CooldownRemaining = 0;
            int16 LockdownRemaining = 0;
            int8 AbilityIndex = 0;
            uint8 Pboid = 0;
        };

        struct PetBattleAuraInfo
        {
            int32 AbilityID = 0;
            uint32 InstanceID = 0;
            int32 RoundsRemaining = 0;
            int32 CurrentRound = 0;
            uint8 CasterPBOID = 0;
        };

        struct PetBattleStateInfo
        {
            uint32 StateID = 0;
            int32 StateValue = 0;
        };

        struct PetBattlePetUpdateInfo
        {
            ObjectGuid BattlePetGUID;
            int32 SpeciesID = 0;
            int32 CreatureID = 0;
            int32 DisplayID = 0;
            int16 Level = 0;
            int16 Xp = 0;
            int32 CurHealth = 0;
            int32 MaxHealth = 0;
            int32 Power = 0;
            int32 Speed = 0;
            int32 NpcTeamMemberID = 0;
            uint8 BreedQuality = 0;
            uint16 StatusFlags = 0;
            int8 Slot = 0;
            std::string CustomName;
            std::vector<PetBattleAbilityInfo> Abilities;
            std::vector<PetBattleAuraInfo> Auras;
            std::vector<PetBattleStateInfo> States;
        };

        struct PetBattlePlayerUpdateInfo
        {
            ObjectGuid CharacterID;
            int32 TrapAbilityID = 0;
            int32 TrapStatus = 0;
            uint16 RoundTimeSecs = 0;
            int8 FrontPet = 0;
            uint8 InputFlags = 0;
            std::vector<PetBattlePetUpdateInfo> Pets;
        };

        struct PetBattleEnviroInfo
        {
            std::vector<PetBattleAuraInfo> Auras;
            std::vector<PetBattleStateInfo> States;
        };

        struct PetBattleEffectTargetInfo
        {
            uint8 Type = 0;     // Written as 3 bits + ResetBitReader
            uint8 Petx = 0;
            // Variable-length params based on Type:
            // 0: nothing, 1: 4 int32s (aura), 2: 2 int32s (state), 3: 1 int32 (health),
            // 4: 1 int32 (stat), 5: 1 int32 (trigger), 6: 3 int32s (cooldown), 7: 1 int32 (broadcast)
            std::vector<int32> Params;
        };

        struct PetBattleEffectInfo
        {
            uint32 AbilityEffectID = 0;
            uint16 Flags = 0;
            uint16 SourceAuraInstanceID = 0;
            uint16 TurnInstanceID = 0;
            int8 PetBattleEffectType = 0;
            uint8 CasterPBOID = 0;
            uint8 StackDepth = 0;
            std::vector<PetBattleEffectTargetInfo> Targets;
        };

        struct PetBattleCooldownInfo
        {
            int32 AbilityID = 0;
            int16 CooldownRemaining = 0;
            int16 LockdownRemaining = 0;
            int8 AbilityIndex = 0;
            uint8 Pboid = 0;
        };

        struct PetBattleRoundPlayerData
        {
            uint8 NextInputFlags = 0;
            int8 NextTrapStatus = 0;
            uint16 RoundTimeSecs = 0;
        };

        // Shared location structure used by multiple pet battle packets
        struct PetBattleLocation
        {
            int32 LocationResult = 0;
            Position BattleOrigin;          // Only XYZ written
            float BattleFacing = 0.0f;
            std::array<Position, 2> PlayerPositions;  // Only XYZ written
        };

        // ---- CMSG packets ----

        class PetBattleRequestWild final : public ClientPacket
        {
        public:
            explicit PetBattleRequestWild(WorldPacket&& packet) : ClientPacket(CMSG_PET_BATTLE_REQUEST_WILD, std::move(packet)) { }

            void Read() override;

            ObjectGuid TargetGUID;
            PetBattleLocation Location;
        };

        class PetBattleInput final : public ClientPacket
        {
        public:
            explicit PetBattleInput(WorldPacket&& packet) : ClientPacket(CMSG_PET_BATTLE_INPUT, std::move(packet)) { }

            void Read() override;

            int32 MoveType = 0;
            int8 NewFrontPetIndex = -1;
            int32 DebugFlags = 0;
            uint8 BattleInterrupted = 0;
            int32 AbilityID = 0;
            int32 Round = 0;
            bool IgnoreAbandonPenalty = false;
        };

        class PetBattleReplaceFrontPet final : public ClientPacket
        {
        public:
            explicit PetBattleReplaceFrontPet(WorldPacket&& packet) : ClientPacket(CMSG_PET_BATTLE_REPLACE_FRONT_PET, std::move(packet)) { }

            void Read() override;

            int8 FrontPetIndex = 0;
        };

        class PetBattleQuitNotify final : public ClientPacket
        {
        public:
            explicit PetBattleQuitNotify(WorldPacket&& packet) : ClientPacket(CMSG_PET_BATTLE_QUIT_NOTIFY, std::move(packet)) { }

            void Read() override { }
        };

        class PetBattleFinalNotify final : public ClientPacket
        {
        public:
            explicit PetBattleFinalNotify(WorldPacket&& packet) : ClientPacket(CMSG_PET_BATTLE_FINAL_NOTIFY, std::move(packet)) { }

            void Read() override { }
        };

        class PetBattleRequestPVP final : public ClientPacket
        {
        public:
            explicit PetBattleRequestPVP(WorldPacket&& packet) : ClientPacket(CMSG_PET_BATTLE_REQUEST_PVP, std::move(packet)) { }

            void Read() override;

            ObjectGuid TargetGUID;
            PetBattleLocation Location;
        };

        class JoinPetBattleQueue final : public ClientPacket
        {
        public:
            explicit JoinPetBattleQueue(WorldPacket&& packet) : ClientPacket(CMSG_JOIN_PET_BATTLE_QUEUE, std::move(packet)) { }

            void Read() override { }
        };

        class LeavePetBattleQueue final : public ClientPacket
        {
        public:
            explicit LeavePetBattleQueue(WorldPacket&& packet) : ClientPacket(CMSG_LEAVE_PET_BATTLE_QUEUE, std::move(packet)) { }

            void Read() override { }
        };

        class PetBattleQueueProposeMatchResult final : public ClientPacket
        {
        public:
            explicit PetBattleQueueProposeMatchResult(WorldPacket&& packet) : ClientPacket(CMSG_PET_BATTLE_QUEUE_PROPOSE_MATCH_RESULT, std::move(packet)) { }

            void Read() override;

            bool Accepted = false;
        };

        class PetBattleRequestUpdate final : public ClientPacket
        {
        public:
            explicit PetBattleRequestUpdate(WorldPacket&& packet) : ClientPacket(CMSG_PET_BATTLE_REQUEST_UPDATE, std::move(packet)) { }

            void Read() override;

            ObjectGuid TargetGUID;
            bool Canceled = false;
        };

        class PetBattleScriptErrorNotify final : public ClientPacket
        {
        public:
            explicit PetBattleScriptErrorNotify(WorldPacket&& packet) : ClientPacket(CMSG_PET_BATTLE_SCRIPT_ERROR_NOTIFY, std::move(packet)) { }

            void Read() override { }
        };

        class PetBattleWildLocationFail final : public ClientPacket
        {
        public:
            explicit PetBattleWildLocationFail(WorldPacket&& packet) : ClientPacket(CMSG_PET_BATTLE_WILD_LOCATION_FAIL, std::move(packet)) { }

            void Read() override { }
        };

        // ---- SMSG packets ----

        class PetBattleFinalizeLocation final : public ServerPacket
        {
        public:
            PetBattleFinalizeLocation() : ServerPacket(SMSG_PET_BATTLE_FINALIZE_LOCATION) { }

            WorldPacket const* Write() override;

            PetBattleLocation Location;
        };

        class PetBattleInitialUpdate final : public ServerPacket
        {
        public:
            PetBattleInitialUpdate() : ServerPacket(SMSG_PET_BATTLE_INITIAL_UPDATE) { }

            WorldPacket const* Write() override;

            std::array<PetBattlePlayerUpdateInfo, 2> Players;
            std::array<PetBattleEnviroInfo, 3> Enviros;
            uint16 WaitingForFrontPetsMaxSecs = 30;
            uint16 PvpMaxRoundTime = 30;
            int32 CurRound = 0;
            uint32 NpcCreatureID = 0;
            uint32 NpcDisplayID = 0;
            int8 CurPetBattleState = 0;
            uint8 ForfeitPenalty = 0;
            ObjectGuid InitialWildPetGUID;
            bool IsPVP = false;
            bool CanAwardXP = false;
        };

        class PetBattleFirstRound final : public ServerPacket
        {
        public:
            PetBattleFirstRound() : ServerPacket(SMSG_PET_BATTLE_FIRST_ROUND) { }

            WorldPacket const* Write() override;

            uint32 CurRound = 0;
            int8 NextPetBattleState = 0;
            std::array<PetBattleRoundPlayerData, 2> Players;
            std::vector<PetBattleEffectInfo> Effects;
            std::vector<PetBattleCooldownInfo> Cooldowns;
            std::vector<int8> PetXDied;
        };

        class PetBattleRoundResult final : public ServerPacket
        {
        public:
            PetBattleRoundResult() : ServerPacket(SMSG_PET_BATTLE_ROUND_RESULT) { }

            WorldPacket const* Write() override;

            uint32 CurRound = 0;
            int8 NextPetBattleState = 0;
            std::array<PetBattleRoundPlayerData, 2> Players;
            std::vector<PetBattleEffectInfo> Effects;
            std::vector<PetBattleCooldownInfo> Cooldowns;
            std::vector<int8> PetXDied;
        };

        class PetBattleReplacementsMade final : public ServerPacket
        {
        public:
            PetBattleReplacementsMade() : ServerPacket(SMSG_PET_BATTLE_REPLACEMENTS_MADE) { }

            WorldPacket const* Write() override;

            uint32 CurRound = 0;
            int8 NextPetBattleState = 0;
            std::array<PetBattleRoundPlayerData, 2> Players;
            std::vector<PetBattleEffectInfo> Effects;
            std::vector<PetBattleCooldownInfo> Cooldowns;
            std::vector<int8> PetXDied;
        };

        struct PetBattleFinalPet
        {
            ObjectGuid Guid;
            uint16 Level = 0;
            uint16 Xp = 0;
            int32 Health = 0;
            int32 MaxHealth = 0;
            uint16 InitialLevel = 0;
            uint8 Pboid = 0;
            bool Captured = false;
            bool Caged = false;
            bool SeenAction = false;
            bool AwardedXP = false;
        };

        class PetBattleFinalRound final : public ServerPacket
        {
        public:
            PetBattleFinalRound() : ServerPacket(SMSG_PET_BATTLE_FINAL_ROUND) { }

            WorldPacket const* Write() override;

            bool Abandoned = false;
            bool PvpBattle = false;
            std::array<bool, 2> Winners = {};
            std::array<int32, 2> NpcCreatureID = {};
            std::vector<PetBattleFinalPet> Pets;
        };

        class PetBattleFinished final : public ServerPacket
        {
        public:
            PetBattleFinished() : ServerPacket(SMSG_PET_BATTLE_FINISHED, 0) { }

            WorldPacket const* Write() override { return &_worldPacket; }
        };

        class PetBattleRequestFailed final : public ServerPacket
        {
        public:
            PetBattleRequestFailed() : ServerPacket(SMSG_PET_BATTLE_REQUEST_FAILED) { }

            WorldPacket const* Write() override;

            uint8 Reason = 0;
        };

        class PetBattlePVPChallenge final : public ServerPacket
        {
        public:
            PetBattlePVPChallenge() : ServerPacket(SMSG_PET_BATTLE_PVP_CHALLENGE) { }

            WorldPacket const* Write() override;

            ObjectGuid ChallengerGUID;
            PetBattleLocation Location;
        };

        class PetBattleQueueStatus final : public ServerPacket
        {
        public:
            PetBattleQueueStatus() : ServerPacket(SMSG_PET_BATTLE_QUEUE_STATUS) { }

            WorldPacket const* Write() override;

            uint32 Status = 0;
            std::array<uint32, 2> SlotResult = {};
            Optional<uint32> ClientWaitTime;
            Optional<uint32> AvgWaitTime;
        };

        class PetBattleQueueProposeMatch final : public ServerPacket
        {
        public:
            PetBattleQueueProposeMatch() : ServerPacket(SMSG_PET_BATTLE_QUEUE_PROPOSE_MATCH, 0) { }

            WorldPacket const* Write() override { return &_worldPacket; }
        };

        class PetBattleMaxGameLengthWarning final : public ServerPacket
        {
        public:
            PetBattleMaxGameLengthWarning() : ServerPacket(SMSG_PET_BATTLE_MAX_GAME_LENGTH_WARNING, 0) { }

            WorldPacket const* Write() override { return &_worldPacket; }
        };

        class PetBattleChatRestricted final : public ServerPacket
        {
        public:
            PetBattleChatRestricted() : ServerPacket(SMSG_PET_BATTLE_CHAT_RESTRICTED, 0) { }

            WorldPacket const* Write() override { return &_worldPacket; }
        };

        class PetBattleDebugQueueDumpResponse final : public ServerPacket
        {
        public:
            PetBattleDebugQueueDumpResponse() : ServerPacket(SMSG_PET_BATTLE_DEBUG_QUEUE_DUMP_RESPONSE, 0) { }

            WorldPacket const* Write() override { return &_worldPacket; }
        };

        class BattlePetsHealed final : public ServerPacket
        {
        public:
            BattlePetsHealed() : ServerPacket(SMSG_BATTLE_PETS_HEALED, 0) { }

            WorldPacket const* Write() override { return &_worldPacket; }
        };

        class BattlePetTrapLevel final : public ServerPacket
        {
        public:
            BattlePetTrapLevel() : ServerPacket(SMSG_BATTLE_PET_TRAP_LEVEL) { }

            WorldPacket const* Write() override;

            uint16 TrapLevel = 0;
        };
    }
}

#endif // TRINITYCORE_BATTLE_PET_PACKETS_H
