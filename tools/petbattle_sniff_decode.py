#!/usr/bin/env python3
"""Decode every pet-battle packet in the sniff extract and print a round-by-round combat log.

Wire formats are byte-for-byte verified against IDA 12.0.5 in earlier work.
Sniff was captured against 12.0.1.66017 — pet-battle wire layout unchanged in 0.4 patch.
"""
from __future__ import annotations

import re
import struct
import sys
from collections import OrderedDict
from dataclasses import dataclass, field
from pathlib import Path
from typing import List, Optional

EXTRACT = Path(r"C:\sniff\pet_battle\dumps\pet_battle_packets_extracted.txt")

# PetBattleEffectType enum (verified vs IDA switch table)
EFFECT_TYPE_NAMES = {
    0: "SET_HEALTH",
    1: "AURA_APPLY",
    2: "AURA_CANCEL",
    3: "AURA_CHANGE",
    4: "PET_SWAP",
    5: "STATUS_CHANGE",
    6: "SET_STATE",
    7: "SET_MAX_HEALTH",
    8: "SET_SPEED",
    9: "SET_POWER",
    10: "TRIGGER_ABILITY",
    11: "ABILITY_CHANGE",
    12: "NPC_EMOTE",
    13: "AURA_PROCESSING_BEGIN",
    14: "AURA_PROCESSING_END",
    15: "REPLACE_PET",
    16: "OVERRIDE_ABILITY",
    17: "WORLD_STATE_UPDATE",
}

# Effect-target type → param count (verified vs Deserialize_PetBattleEffectTargetInfo)
TARGET_TYPE_PARAMS = {0: 0, 1: 4, 2: 2, 3: 1, 4: 1, 5: 1, 6: 3, 7: 1, 8: 0, 9: 2}
TARGET_TYPE_NAMES = {0: "none", 1: "aura", 2: "state", 3: "health",
                     4: "stat", 5: "trigger", 6: "cooldown", 7: "broadcast",
                     8: "petUpdate", 9: "twoparams"}


@dataclass
class Packet:
    number: int
    direction: str
    opcode_hex: str
    opcode_name: str
    payload: bytes


def parse_packets(text: str) -> List[Packet]:
    pkts = []
    for block in re.split(r"^--- Packet ", text, flags=re.MULTILINE)[1:]:
        m = re.search(r"#(\d+)", block)
        num = int(m.group(1)) if m else -1
        d = re.search(r"Direction:\s*(\S+)", block)
        op = re.search(r"Opcode:\s*(0x[0-9A-Fa-f]+)\s*\(([A-Z_]+)\)", block)
        if not (d and op):
            continue
        # The extract truncates large payloads — record the *announced* size, not just what's dumped
        size_match = re.search(r"Payload \(first (\d+) of (\d+) bytes\):", block)
        # Parse hex dump rows into bytes
        payload = bytearray()
        for hex_line in re.findall(r"^\s*[0-9a-fA-F]+:\s*([0-9A-Fa-f ]+?)\s{2,}", block, flags=re.MULTILINE):
            payload.extend(bytes.fromhex(hex_line.replace(" ", "")))
        pkt = Packet(num, d.group(1), op.group(1), op.group(2), bytes(payload))
        if size_match:
            pkt.announced_size = int(size_match.group(2))  # type: ignore[attr-defined]
        pkts.append(pkt)
    return pkts


class Reader:
    """Sequential byte reader with bit-buffer for packed-size strings."""
    def __init__(self, data: bytes):
        self.data = data
        self.pos = 0

    def remaining(self) -> int:
        return len(self.data) - self.pos

    def u8(self) -> int:
        v = self.data[self.pos]
        self.pos += 1
        return v

    def i8(self) -> int:
        v = struct.unpack_from("<b", self.data, self.pos)[0]
        self.pos += 1
        return v

    def u16(self) -> int:
        v = struct.unpack_from("<H", self.data, self.pos)[0]
        self.pos += 2
        return v

    def i16(self) -> int:
        v = struct.unpack_from("<h", self.data, self.pos)[0]
        self.pos += 2
        return v

    def u32(self) -> int:
        v = struct.unpack_from("<I", self.data, self.pos)[0]
        self.pos += 4
        return v

    def i32(self) -> int:
        v = struct.unpack_from("<i", self.data, self.pos)[0]
        self.pos += 4
        return v

    def f32(self) -> float:
        v = struct.unpack_from("<f", self.data, self.pos)[0]
        self.pos += 4
        return v

    def raw(self, n: int) -> bytes:
        v = self.data[self.pos:self.pos + n]
        self.pos += n
        return v


def decode_target(r: Reader) -> dict:
    type_byte = r.u8()
    ttype = type_byte >> 4
    out = {"type": ttype, "type_name": TARGET_TYPE_NAMES.get(ttype, f"unk{ttype}")}
    if ttype == 8:
        out["embedded_pet_update"] = "<not-decoded-here>"
        # Skip embedded — too long to decode in this script
    out["remaining"] = r.i32()
    nparams = TARGET_TYPE_PARAMS.get(ttype, 0)
    out["params"] = [r.i32() for _ in range(nparams)]
    return out


def decode_effect(r: Reader) -> dict:
    eff = OrderedDict()
    eff["AbilityEffectID"] = r.i32()
    eff["Flags"] = r.i32()
    eff["SourceAuraInstanceID"] = r.i16()
    eff["TurnInstanceID"] = r.i16()
    eff["EffectIndex"] = r.i32()  # actually PetBattleEffectType
    eff["effect_type_name"] = EFFECT_TYPE_NAMES.get(eff["EffectIndex"], f"unk{eff['EffectIndex']}")
    eff["CasterPBOID"] = r.i32()
    eff["StackDepth"] = r.u8()
    target_count = r.u32()
    eff["targets"] = [decode_target(r) for _ in range(target_count)]
    return eff


def decode_round_body(payload: bytes, has_replacements_marker=False) -> dict:
    r = Reader(payload)
    out = OrderedDict()
    out["CurRound"] = r.u32()
    out["NextState"] = r.u8()
    effect_count = r.u32()
    out["effect_count_announced"] = effect_count
    teams = []
    for _ in range(2):
        teams.append({
            "NextInputFlags": r.u8(),
            "NextTrapStatus": r.i8() if False else r.u8(),  # signed/unsigned same byte
            "RoundTimeSecs": r.u16(),
        })
    out["teams"] = teams
    cooldown_count = r.u32()
    out["cooldowns"] = []
    for _ in range(cooldown_count):
        out["cooldowns"].append({
            "AbilityID": r.i32(),
            "CooldownRemaining": r.i16(),
            "LockdownRemaining": r.i16(),
            "AbilityIndex": r.u8(),
            "Pboid": r.u8(),
        })
    pet_x_died_byte = r.u8()
    pet_x_died_count = pet_x_died_byte >> 5
    out["pet_x_died_byte"] = f"0x{pet_x_died_byte:02X}"
    out["pet_x_died_count"] = pet_x_died_count
    out["effects"] = []
    for _ in range(effect_count):
        if r.remaining() < 25:  # min effect header
            break
        try:
            out["effects"].append(decode_effect(r))
        except (struct.error, IndexError):
            break
    out["dead_pet_pboids"] = []
    for _ in range(pet_x_died_count):
        if r.remaining() < 1:
            break
        out["dead_pet_pboids"].append(r.u8())
    out["bytes_consumed"] = r.pos
    out["bytes_total"] = len(payload)
    return out


def decode_input(payload: bytes) -> dict:
    r = Reader(payload)
    return {
        "MoveType": r.i32(),
        "NewFrontPetIndex": struct.unpack("<b", payload[4:5])[0],
        "DebugFlags": r.i32() if False else struct.unpack_from("<i", payload, 5)[0],
        "BattleInterrupted": payload[9],
        "AbilityID": struct.unpack_from("<i", payload, 10)[0],
        "Round": struct.unpack_from("<i", payload, 14)[0],
        "IgnoreAbandonPenalty_byte": payload[18],
    }


def main():
    text = EXTRACT.read_text(encoding="utf-8", errors="replace")
    pkts = parse_packets(text)
    print(f"Parsed {len(pkts)} packets\n")

    move_types = {0: "Quit", 1: "Ability", 2: "Swap", 3: "Trap", 4: "FinalRoundOk", 5: "Pass"}

    # Pair CMSG inputs with the next ROUND_RESULT for context
    for i, p in enumerate(pkts):
        if p.opcode_name == "CMSG_PET_BATTLE_INPUT":
            inp = decode_input(p.payload)
            mt = move_types.get(inp["MoveType"], f"unk{inp['MoveType']}")
            print(f"--- Pkt #{p.number} CMSG_INPUT round={inp['Round']} move={mt} abilityID={inp['AbilityID']} swapTo={inp['NewFrontPetIndex']}")
        elif p.opcode_name == "CMSG_PET_BATTLE_REPLACE_FRONT_PET":
            r = Reader(p.payload)
            print(f"--- Pkt #{p.number} CMSG_REPLACE_FRONT_PET → frontPetIdx={r.u8()}")
        elif p.opcode_name in ("SMSG_PET_BATTLE_FIRST_ROUND",
                               "SMSG_PET_BATTLE_ROUND_RESULT",
                               "SMSG_PET_BATTLE_REPLACEMENTS_MADE"):
            print(f"\n=== Pkt #{p.number} {p.opcode_name} ({len(p.payload)} bytes) ===")
            announced = getattr(p, "announced_size", len(p.payload))
            if announced != len(p.payload):
                print(f"  ⚠ TRUNCATED: dump has {len(p.payload)} of {announced} bytes — partial decode")
            try:
                d = decode_round_body(p.payload)
                print(f"  CurRound={d['CurRound']} NextState={d['NextState']} effects={len(d['effects'])} cooldowns={len(d['cooldowns'])} dead={d['pet_x_died_count']}")
                print(f"  Team0: input={d['teams'][0]['NextInputFlags']} trap={d['teams'][0]['NextTrapStatus']} time={d['teams'][0]['RoundTimeSecs']}")
                print(f"  Team1: input={d['teams'][1]['NextInputFlags']} trap={d['teams'][1]['NextTrapStatus']} time={d['teams'][1]['RoundTimeSecs']}")
                if d['cooldowns']:
                    for cd in d['cooldowns']:
                        print(f"    CD: ability={cd['AbilityID']} CDrem={cd['CooldownRemaining']} LDrem={cd['LockdownRemaining']} idx={cd['AbilityIndex']} pboid={cd['Pboid']}")
                for j, e in enumerate(d['effects']):
                    targets_str = ", ".join(
                        f"{t['type_name']}(rem={t['remaining']}{', params='+str(t['params']) if t['params'] else ''})"
                        for t in e['targets']
                    )
                    print(f"    [{j}] {e['effect_type_name']:22s} caster=PBOID{e['CasterPBOID']:<2d} ability_eff_id={e['AbilityEffectID']:<5d} flags={e['Flags']:#010x} aura_inst={e['SourceAuraInstanceID']:<3d} turn_inst={e['TurnInstanceID']:<3d} stack={e['StackDepth']} → [{targets_str}]")
                if d['dead_pet_pboids']:
                    print(f"  DEAD: {d['dead_pet_pboids']}")
                if d['bytes_consumed'] != d['bytes_total']:
                    print(f"  ⚠ MISALIGNMENT: consumed {d['bytes_consumed']}, payload {d['bytes_total']}")
            except Exception as ex:
                print(f"  DECODE_FAILED: {ex!r}")
        elif p.opcode_name == "SMSG_PET_BATTLE_FINAL_ROUND":
            print(f"\n=== Pkt #{p.number} FINAL_ROUND ({len(p.payload)} bytes) ===")
            r = Reader(p.payload)
            flags = r.u8()
            print(f"  Abandoned={(flags>>7)&1} PvpBattle={(flags>>6)&1} Win[0]={(flags>>5)&1} Win[1]={(flags>>4)&1}")
            npc_creature_ids = [r.i32(), r.i32()]
            pet_count = r.u32()
            print(f"  NpcCreatureIDs={npc_creature_ids} pets={pet_count}")
            # Pets are PackedGUID + 12 bytes + flags - skip detailed decode
        elif p.opcode_name == "SMSG_PET_BATTLE_FINISHED":
            print(f"\n=== Pkt #{p.number} FINISHED (no body) ===")


if __name__ == "__main__":
    main()
