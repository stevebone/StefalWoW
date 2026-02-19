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

#include "BattlePetPackets.h"
#include "MovementPackets.h"
#include "PacketOperators.h"

namespace WorldPackets::BattlePet
{
ByteBuffer& operator<<(ByteBuffer& data, BattlePetSlot const& slot)
{
    data << (slot.Pet.Guid.IsEmpty() ? ObjectGuid::Create<HighGuid::BattlePet>(0) : slot.Pet.Guid);
    data << uint32(slot.CollarID);
    data << uint8(slot.Index);
    data << Bits<1>(slot.Locked);
    data.FlushBits();

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, BattlePetOwnerInfo const& owner)
{
    data << owner.Guid;
    data << uint32(owner.PlayerVirtualRealm);
    data << uint32(owner.PlayerNativeRealm);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, BattlePet const& pet)
{
    data << pet.Guid;
    data << uint32(pet.Species);
    data << uint32(pet.CreatureID);
    data << uint32(pet.DisplayID);
    data << uint16(pet.Breed);
    data << uint16(pet.Level);
    data << uint16(pet.Exp);
    data << uint16(pet.Flags);
    data << uint32(pet.Power);
    data << uint32(pet.Health);
    data << uint32(pet.MaxHealth);
    data << uint32(pet.Speed);
    data << uint8(pet.Quality);
    data << SizedString::BitsSize<7>(pet.Name);
    data << OptionalInit(pet.OwnerInfo);
    data << Bits<1>(pet.NoRename);
    data.FlushBits();

    data << SizedString::Data(pet.Name);

    if (pet.OwnerInfo)
        data << *pet.OwnerInfo;

    return data;
}

WorldPacket const* BattlePetJournal::Write()
{
    _worldPacket << uint16(Trap);
    _worldPacket << Size<uint32>(Slots);
    _worldPacket << Size<uint32>(Pets);
    _worldPacket << Bits<1>(HasJournalLock);
    _worldPacket.FlushBits();

    for (BattlePetSlot const& slot : Slots)
        _worldPacket << slot;

    for (BattlePet const& pet : Pets)
        _worldPacket << pet;

    return &_worldPacket;
}

WorldPacket const* BattlePetUpdates::Write()
{
    _worldPacket << Size<uint32>(Pets);
    _worldPacket << Bits<1>(PetAdded);
    _worldPacket.FlushBits();

    for (BattlePet const& pet : Pets)
        _worldPacket << pet;

    return &_worldPacket;
}

WorldPacket const* PetBattleSlotUpdates::Write()
{
    _worldPacket << Size<uint32>(Slots);
    _worldPacket << Bits<1>(NewSlot);
    _worldPacket << Bits<1>(AutoSlotted);
    _worldPacket.FlushBits();

    for (BattlePetSlot const& slot : Slots)
        _worldPacket << slot;

    return &_worldPacket;
}

void BattlePetSetBattleSlot::Read()
{
    _worldPacket >> PetGuid;
    _worldPacket >> Slot;
}

void BattlePetModifyName::Read()
{
    _worldPacket >> PetGuid;
    _worldPacket >> SizedString::BitsSize<7>(Name);
    _worldPacket >> OptionalInit(DeclinedNames);

    if (DeclinedNames)
    {
        for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            _worldPacket >> SizedString::BitsSize<7>(DeclinedNames->name[i]);

        for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            _worldPacket >> SizedString::Data(DeclinedNames->name[i]);
    }

    _worldPacket >> SizedString::Data(Name);
}

void QueryBattlePetName::Read()
{
    _worldPacket >> BattlePetID;
    _worldPacket >> UnitGUID;
}

WorldPacket const* QueryBattlePetNameResponse::Write()
{
    _worldPacket << BattlePetID;
    _worldPacket << int32(CreatureID);
    _worldPacket << Timestamp;

    _worldPacket << Bits<1>(Allow);

    if (Allow)
    {
        _worldPacket << SizedString::BitsSize<8>(Name);
        _worldPacket << Bits<1>(HasDeclined);

        for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            _worldPacket << SizedString::BitsSize<7>(DeclinedNames.name[i]);

        _worldPacket.FlushBits();

        for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            _worldPacket << SizedString::Data(DeclinedNames.name[i]);

        _worldPacket << SizedString::Data(Name);
    }
    else
        _worldPacket.FlushBits();

    return &_worldPacket;
}

void BattlePetDeletePet::Read()
{
    _worldPacket >> PetGuid;
}

void BattlePetSetFlags::Read()
{
    _worldPacket >> PetGuid;
    _worldPacket >> Flags;
    _worldPacket >> Bits<2>(ControlType);
}

void BattlePetClearFanfare::Read()
{
    _worldPacket >> PetGuid;
}

void CageBattlePet::Read()
{
    _worldPacket >> PetGuid;
}

WorldPacket const* BattlePetDeleted::Write()
{
    _worldPacket << PetGuid;

    return &_worldPacket;
}

WorldPacket const* BattlePetError::Write()
{
    _worldPacket << Bits<4>(Result);
    _worldPacket << int32(CreatureID);

    return &_worldPacket;
}

void BattlePetSummon::Read()
{
    _worldPacket >> PetGuid;
}

void BattlePetUpdateNotify::Read()
{
    _worldPacket >> PetGuid;
}

void BattlePetUpdateDisplayNotify::Read()
{
    _worldPacket >> PetGuid;
}

// ============================================================================
// Pet Battle Combat Packet Serialization
// ============================================================================

ByteBuffer& operator<<(ByteBuffer& data, PetBattleAbilityInfo const& ability)
{
    data << int32(ability.AbilityID);
    data << int16(ability.CooldownRemaining);
    data << int16(ability.LockdownRemaining);
    data << int8(ability.AbilityIndex);
    data << uint8(ability.Pboid);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, PetBattleAuraInfo const& aura)
{
    data << int32(aura.AbilityID);
    data << uint32(aura.InstanceID);
    data << int32(aura.RoundsRemaining);
    data << int32(aura.CurrentRound);
    data << uint8(aura.CasterPBOID);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, PetBattleStateInfo const& state)
{
    data << uint32(state.StateID);
    data << int32(state.StateValue);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, PetBattlePetUpdateInfo const& pet)
{
    data << pet.BattlePetGUID;
    data << int32(pet.SpeciesID);
    data << int32(pet.CreatureID);
    data << int32(pet.DisplayID);
    data << int16(pet.Level);
    data << int16(pet.Xp);
    data << int32(pet.CurHealth);
    data << int32(pet.MaxHealth);
    data << int32(pet.Power);
    data << int32(pet.Speed);
    data << int32(pet.NpcTeamMemberID);
    data << uint8(pet.BreedQuality);
    data << uint16(pet.StatusFlags);
    data << int8(pet.Slot);

    data << uint32(pet.Abilities.size());
    data << uint32(pet.Auras.size());
    data << uint32(pet.States.size());

    for (PetBattleAbilityInfo const& ability : pet.Abilities)
        data << ability;

    for (PetBattleAuraInfo const& aura : pet.Auras)
        data << aura;

    for (PetBattleStateInfo const& state : pet.States)
        data << state;

    data << SizedString::BitsSize<7>(pet.CustomName);
    data.FlushBits();
    data << SizedString::Data(pet.CustomName);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, PetBattlePlayerUpdateInfo const& player)
{
    data << player.CharacterID;
    data << int32(player.TrapAbilityID);
    data << int32(player.TrapStatus);
    data << uint16(player.RoundTimeSecs);
    data << int8(player.FrontPet);
    data << uint8(player.InputFlags);

    data << Bits<2>(player.Pets.size());
    data.FlushBits();

    for (PetBattlePetUpdateInfo const& pet : player.Pets)
        data << pet;

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, PetBattleEnviroInfo const& enviro)
{
    data << uint32(enviro.Auras.size());
    data << uint32(enviro.States.size());

    for (PetBattleAuraInfo const& aura : enviro.Auras)
        data << aura;

    for (PetBattleStateInfo const& state : enviro.States)
        data << state;

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, PetBattleEffectTargetInfo const& target)
{
    data << uint8(target.Type);
    data << uint8(target.Petx);
    data << int32(target.Param1);
    data << int32(target.Param2);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, PetBattleEffectInfo const& effect)
{
    data << uint32(effect.AbilityEffectID);
    data << uint16(effect.Flags);
    data << uint8(effect.SourceAuraPetInstance);
    data << uint8(effect.StackDepth);
    data << uint8(effect.PetBattleEffectType);
    data << int8(effect.CasterPBOID);

    data << uint32(effect.Targets.size());
    for (PetBattleEffectTargetInfo const& target : effect.Targets)
        data << target;

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, PetBattleCooldownInfo const& cd)
{
    data << int32(cd.AbilityID);
    data << int16(cd.CooldownRemaining);
    data << int16(cd.LockdownRemaining);

    return data;
}

static void WriteRoundPlayerData(ByteBuffer& data, PetBattleRoundPlayerData const& player)
{
    data << uint8(player.NextInputFlags);
    data << int8(player.NextTrapStatus);
    data << uint16(player.RoundTimeSecs);

    data << uint32(player.PetCooldowns.size());
    for (std::vector<PetBattleCooldownInfo> const& petCds : player.PetCooldowns)
    {
        data << uint32(petCds.size());
        for (PetBattleCooldownInfo const& cd : petCds)
            data << cd;
    }
}

static void WriteRoundEffects(ByteBuffer& data, std::vector<PetBattleEffectInfo> const& effects)
{
    data << uint32(effects.size());
    for (PetBattleEffectInfo const& effect : effects)
        data << effect;
}

// ---- CMSG Read() implementations ----

void PetBattleRequestWild::Read()
{
    _worldPacket >> TargetGUID;
    _worldPacket >> LocationInfo;
    BattleOrigin = LocationInfo.pos;
}

void PetBattleInput::Read()
{
    _worldPacket >> MoveType;
    _worldPacket >> AbilityID;
    _worldPacket >> NewFrontPetIndex;
    _worldPacket >> Round;
    _worldPacket >> Bits<1>(IgnoreAbandonPenalty);
}

void PetBattleReplaceFrontPet::Read()
{
    _worldPacket >> FrontPetIndex;
}

void PetBattleRequestPVP::Read()
{
    _worldPacket >> TargetGUID;
    _worldPacket >> LocationInfo;
    BattleOrigin = LocationInfo.pos;
}

void PetBattleQueueProposeMatchResult::Read()
{
    _worldPacket >> Bits<1>(Accepted);
}

void PetBattleRequestUpdate::Read()
{
    _worldPacket >> TargetGUID;
    _worldPacket >> Bits<1>(Canceled);
}

// ---- SMSG Write() implementations ----

WorldPacket const* PetBattleFinalizeLocation::Write()
{
    _worldPacket << Location.m_positionX;
    _worldPacket << Location.m_positionY;
    _worldPacket << Location.m_positionZ;
    _worldPacket << Location.GetOrientation();

    return &_worldPacket;
}

WorldPacket const* PetBattleInitialUpdate::Write()
{
    // Wire format matches BaseEntity.cpp:494-588
    for (std::size_t i = 0; i < 2; ++i)
    {
        _worldPacket << Players[i].CharacterID;
        _worldPacket << int32(Players[i].TrapAbilityID);
        _worldPacket << int32(Players[i].TrapStatus);
        _worldPacket << uint16(Players[i].RoundTimeSecs);
        _worldPacket << int8(Players[i].FrontPet);
        _worldPacket << uint8(Players[i].InputFlags);

        _worldPacket << Bits<2>(Players[i].Pets.size());
        _worldPacket.FlushBits();

        for (PetBattlePetUpdateInfo const& pet : Players[i].Pets)
            _worldPacket << pet;
    }

    for (std::size_t i = 0; i < 3; ++i)
        _worldPacket << Enviros[i];

    _worldPacket << uint16(WaitingForFrontPetsMaxSecs);
    _worldPacket << uint16(PvpMaxRoundTime);
    _worldPacket << int32(CurRound);
    _worldPacket << uint32(NpcCreatureID);
    _worldPacket << uint32(NpcDisplayID);
    _worldPacket << int8(CurPetBattleState);
    _worldPacket << uint8(ForfeitPenalty);
    _worldPacket << InitialWildPetGUID;
    _worldPacket << Bits<1>(IsPVP);
    _worldPacket << Bits<1>(CanAwardXP);
    _worldPacket.FlushBits();

    return &_worldPacket;
}

WorldPacket const* PetBattleFirstRound::Write()
{
    _worldPacket << uint32(CurRound);
    _worldPacket << uint8(NextPetBattleState);

    for (std::size_t i = 0; i < 2; ++i)
        WriteRoundPlayerData(_worldPacket, Players[i]);

    WriteRoundEffects(_worldPacket, Effects);

    return &_worldPacket;
}

WorldPacket const* PetBattleRoundResult::Write()
{
    _worldPacket << uint32(CurRound);
    _worldPacket << uint8(NextPetBattleState);

    for (std::size_t i = 0; i < 2; ++i)
        WriteRoundPlayerData(_worldPacket, Players[i]);

    WriteRoundEffects(_worldPacket, Effects);

    return &_worldPacket;
}

WorldPacket const* PetBattleReplacementsMade::Write()
{
    _worldPacket << uint32(CurRound);
    _worldPacket << uint8(NextPetBattleState);

    for (std::size_t i = 0; i < 2; ++i)
        WriteRoundPlayerData(_worldPacket, Players[i]);

    return &_worldPacket;
}

WorldPacket const* PetBattleFinalRound::Write()
{
    _worldPacket << uint32(CurRound);
    _worldPacket << uint8(NextPetBattleState);

    for (std::size_t i = 0; i < 2; ++i)
        WriteRoundPlayerData(_worldPacket, Players[i]);

    WriteRoundEffects(_worldPacket, Effects);

    _worldPacket << Bits<1>(AbandonedPVP);
    _worldPacket << Bits<1>(PVPWinner);
    for (std::size_t i = 0; i < 2; ++i)
        _worldPacket << Bits<1>(Winners[i]);
    _worldPacket.FlushBits();

    for (std::size_t i = 0; i < 2; ++i)
        _worldPacket << uint32(NpcCreatureID[i]);

    return &_worldPacket;
}

WorldPacket const* PetBattleRequestFailed::Write()
{
    _worldPacket << uint8(Reason);

    return &_worldPacket;
}

WorldPacket const* PetBattlePVPChallenge::Write()
{
    _worldPacket << ChallengerGUID;
    _worldPacket << BattleOrigin.m_positionX;
    _worldPacket << BattleOrigin.m_positionY;
    _worldPacket << BattleOrigin.m_positionZ;
    _worldPacket << BattleOrigin.GetOrientation();

    return &_worldPacket;
}

WorldPacket const* PetBattleQueueStatus::Write()
{
    _worldPacket << uint32(Status);
    _worldPacket << uint32(SlotResult[0]);
    _worldPacket << uint32(SlotResult[1]);
    _worldPacket << OptionalInit(ClientWaitTime);
    _worldPacket << OptionalInit(AvgWaitTime);
    _worldPacket.FlushBits();

    if (ClientWaitTime)
        _worldPacket << uint32(*ClientWaitTime);
    if (AvgWaitTime)
        _worldPacket << uint32(*AvgWaitTime);

    return &_worldPacket;
}

WorldPacket const* BattlePetTrapLevel::Write()
{
    _worldPacket << uint16(TrapLevel);
    return &_worldPacket;
}
}
