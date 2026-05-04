#!/usr/bin/env python3
"""Build a per-ability summary of retail behavior from the sniff.

For each ability ID seen in CMSG_INPUT, list:
  - The effects retail emits in the matching SMSG_PET_BATTLE_ROUND_RESULT
  - Effect flags observed (super-effective, weak, crit, etc.)
  - Cooldown set on the ability after use
  - Damage values

Then output a comparison table that we can spot-check against our DB2 chain.
"""
from __future__ import annotations

import importlib.util
import sys
from collections import defaultdict
from pathlib import Path

# Reuse the decoder
sys.path.insert(0, str(Path(__file__).parent))
import petbattle_sniff_decode as decode  # noqa: E402

EXTRACT = Path(r"C:\sniff\pet_battle\dumps\pet_battle_packets_extracted.txt")


def main():
    text = EXTRACT.read_text(encoding="utf-8", errors="replace")
    pkts = decode.parse_packets(text)

    # Walk: every CMSG_INPUT followed by next ROUND_RESULT
    pairs = []
    pending_input = None
    for p in pkts:
        if p.opcode_name == "CMSG_PET_BATTLE_INPUT":
            pending_input = decode.decode_input(p.payload)
        elif p.opcode_name == "SMSG_PET_BATTLE_ROUND_RESULT" and pending_input is not None:
            try:
                body = decode.decode_round_body(p.payload)
                pairs.append((pending_input, body, p))
            except Exception as ex:
                print(f"# decode failed pkt {p.number}: {ex!r}", file=sys.stderr)
            pending_input = None

    # Per-ability rollup
    by_ability: dict[int, dict] = defaultdict(lambda: {
        "rounds": [], "effects": defaultdict(int), "effect_flags": set(),
        "cooldowns_set": set(),
    })

    for inp, body, p in pairs:
        ab = inp["AbilityID"]
        rec = by_ability[ab]
        rec["rounds"].append((inp["Round"], p.number, body))
        for eff in body["effects"]:
            tname = eff["effect_type_name"]
            rec["effects"][tname] += 1
            if eff["Flags"]:
                rec["effect_flags"].add(eff["Flags"])
        for cd in body["cooldowns"]:
            rec["cooldowns_set"].add((cd["AbilityID"], cd["CooldownRemaining"]))

    print("Ability behavior observed in sniff (12.0.1.66017)")
    print("=" * 70)
    for ab in sorted(by_ability):
        rec = by_ability[ab]
        print(f"\nAbility {ab} — used {len(rec['rounds'])}x")
        eff_summary = ", ".join(f"{n}×{c}" for n, c in sorted(rec["effects"].items(), key=lambda x: -x[1]))
        print(f"  Effects emitted (across all uses): {eff_summary}")
        if rec["effect_flags"]:
            print(f"  Effect flag values seen: {sorted(hex(f) for f in rec['effect_flags'])}")
        if rec["cooldowns_set"]:
            print(f"  Cooldowns set: {sorted(rec['cooldowns_set'])}")
        # Show effect chain for the first use
        first_round_num, first_pkt_num, first_body = rec["rounds"][0]
        print(f"  First-use chain (round {first_round_num}, pkt {first_pkt_num}):")
        for j, eff in enumerate(first_body["effects"]):
            t = eff["targets"][0] if eff["targets"] else {"type_name": "-", "remaining": 0, "params": []}
            params_str = f" params={t['params']}" if t.get("params") else ""
            print(f"    [{j}] {eff['effect_type_name']:22s} "
                  f"caster=PBOID{eff['CasterPBOID']:<2} "
                  f"eff_id={eff['AbilityEffectID']:<5} "
                  f"flags={eff['Flags']:#06x} "
                  f"turn_inst={eff['TurnInstanceID']} → "
                  f"{t['type_name']}(rem={t['remaining']}{params_str})")

    print("\n" + "=" * 70)
    print("All unique AbilityEffectIDs observed (with effect type)")
    print("=" * 70)
    eff_id_to_types: dict[int, set] = defaultdict(set)
    for inp, body, _ in pairs:
        for eff in body["effects"]:
            if eff["AbilityEffectID"] != 0:
                eff_id_to_types[eff["AbilityEffectID"]].add(eff["effect_type_name"])
    for eid in sorted(eff_id_to_types):
        types = ", ".join(sorted(eff_id_to_types[eid]))
        print(f"  AbilityEffectID {eid:5d} → {types}")

    print("\n" + "=" * 70)
    print("Effect-flag bit summary (these likely encode crit/weak/strong/miss)")
    print("=" * 70)
    flag_observations: dict[int, list[tuple[str, int]]] = defaultdict(list)
    for inp, body, _ in pairs:
        for eff in body["effects"]:
            if eff["Flags"]:
                flag_observations[eff["Flags"]].append((eff["effect_type_name"], eff["AbilityEffectID"]))
    for flags in sorted(flag_observations):
        examples = flag_observations[flags][:3]
        print(f"  flags={flags:#06x} (bits set: {bin(flags)[2:]}) — {len(flag_observations[flags])} occurrences, e.g. {examples}")


if __name__ == "__main__":
    main()
