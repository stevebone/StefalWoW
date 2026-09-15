# War-Effort Contribution System — Implementation Plan (12.0.7.68275)

Standalone `ContributionMgr`, faithful to the client DB2 model, on `feature/contribution`
(off base `560165c0a6`). No hard coupling to the major-factions branch — the only seam is
that a contribution's reward quest (via `ManagedWorldStateInput.QuestID`) may grant
reputation/renown, which flows through the normal reputation path.

Decision (user, 2026-07-07): build it standalone per the client's actual data model
(Contribution → ManagedWorldStateInput → ManagedWorldState), NOT folded into MajorFactionMgr.

---

## 1. What this system is

The "Contribution Collector" mechanic (Legionfall buildings, BfA 8.3 assault structures,
the generic "help build/advance the thing" world content). A player interacts with a
contribution-collector NPC/GO and turns in currency/items to advance a shared, realm-wide
**managed world state** (a progress bar that also depletes over time). Reaching a stage
flips world states that drive phasing, buffs, and reward quests.

Client API: `C_ContributionCollector` — `Contribute(contributionID)`, `Close()`,
`GetState`, `GetRequiredContributionCurrency/Item`, `GetContributionResult`,
`GetRewardQuestID`, `GetContributionCollectorsForMap`, `GetManagedContributionsForCreatureID`.

## 2. Opcodes / wire (RE'd 68275)

| Opcode | Value | Dir | Wire | Source |
|---|---|---|---|---|
| CMSG_CONTRIBUTION_CONTRIBUTE | 0x3B00FD | C→S | `{ ObjectGuid CollectorGUID (packed), uint32 ContributionID }` | ser `sub_7FF729154010` (BEBDED0=packed-guid @a1+32, CE60 u32 @a1+48) |
| CMSG_CONTRIBUTION_LAST_UPDATE_REQUEST | 0x3B00FE | C→S | `{ uint32 ContributionID, uint32 Field1 }` | ser `sub_7FF729154070` (two CE60 u32 from struct @0 / @4) |
| SMSG_CONTRIBUTION_LAST_UPDATE_RESPONSE | 0x4202C4 | S→C | **PENDING** — reader is a factory/thunk maze (`sub_7FF7290B1D30` factory sets vtable `off_7FF72C4AA498`; dispatch `case 0x4202C4` in `sub_7FF729103660`). Body read as bytes[rest]. Likely `{ uint32 ContributionID, uint32 LastUpdateServerTime }` but NOT byte-confirmed. Do NOT send a guessed layout. | dispatch trace |

The CONTRIBUTE action's effect is observable through the **world state** it advances
(SMSG_UPDATE_WORLD_STATE / init world states), not through a per-action SMSG, so the core
mechanic is verifiable without nailing LAST_UPDATE_RESPONSE.

## 3. Data model (DB2 — none currently in TC)

Layouts read from `WoWDBDefs` at the 68275 layout.

- **Contribution.db2** (layout `167173C3`): `ID, Description_lang, ManagedWorldStateInputID,
  Name_lang, OrderIndex, ContributionStyleContainer, UiTextureAtlasMemberID[4]`.
  **No FactionID** — confirms contribution is not structurally a major-faction sub-feature.
- **ManagedWorldStateInput.db2**: `ID, ManagedWorldStateID, QuestID, ValidInputConditionID`.
  The **QuestID is the cost/turn-in**: contributing = completing this quest's item/currency
  requirements; there is no separate cost column.
- **ManagedWorldState.db2**: `ID, CurrentStageWorldStateID, OccurrencesWorldStateID,
  ProgressWorldStateID, AccumulationAmountPerMinute, AccumulationStateTargetValue,
  DepletionAmountPerMinute, DepletionStateTargetValue, DownTimeSecs, UpTimeSecs`.
  A time-based accumulate/deplete progress driver bound to three WorldState IDs.
- **CreatureXContribution.db2**: `ID, ContributionID, CreatureID` — which collector NPCs
  offer which contributions.
- **ContributionStyleContainer.db2** / **ContributionStyle.db2**: cosmetic (atlas/appearance
  per state) — not needed for server logic.

## 4. Cost model (RE conclusion)

`GetRequiredContributionCurrency(contributionID)` / `GetRequiredContributionItem` are NOT
sourced from a cost column — the requirement is the **`ManagedWorldStateInput.QuestID`**
quest. Contribute validates + consumes that quest's requirements via TC's existing quest
turn-in path, then advances the ManagedWorldState progress world state. This is the seam
that makes the system buildable on existing TC infrastructure.

## 5. Phased build plan

- **P0 — data + packets.** Add DB2 stores (Contribution, ManagedWorldStateInput,
  ManagedWorldState, CreatureXContribution) via the db2_codegen pipeline
  (run_db2_codegen_68275 + apply). Packet layer: ContributionPackets.h/.cpp for the two
  CMSG readers. Route both to a ContributionMgr-backed handler.
- **P1 — ManagedWorldState runtime.** New `ManagedWorldStateMgr`: loads ManagedWorldState
  rows, holds per-state accumulated value, ticks accumulation/depletion per minute between
  Up/Down windows, and pushes ProgressWorldStateID / CurrentStageWorldStateID /
  OccurrencesWorldStateID to players via the world-state system. Persist accumulated values.
- **P2 — Contribute.** `ContributionMgr::Contribute(player, collectorGuid, contributionId)`:
  validate the collector offers the contribution (CreatureXContribution) and the player is
  interacting (PlayerInteractionType::ContributionCollector), resolve
  ManagedWorldStateInput→QuestID, verify + consume the quest requirements, add
  AccumulationStateTargetValue-scaled progress to the ManagedWorldState. Honest gap:
  per-contribute increment amount if not derivable → document, use the quest reward/1 unit.
- **P3 — stage rewards + LAST_UPDATE.** On stage threshold, reward the input QuestID's
  reward + apply ManagedWorldStateBuff spells + flip stage world state (phasing). Answer
  LAST_UPDATE_REQUEST once the SMSG layout is sniff/dispatch-confirmed (kept out of P0–P2
  to avoid guessing the wire).

## 6. Honest gaps requiring sniff / later RE
- SMSG_CONTRIBUTION_LAST_UPDATE_RESPONSE exact body (factory/thunk maze; needs a live
  12.0.7 sniff or a full dispatch decompile). Until then LAST_UPDATE_REQUEST is accepted
  but not answered (no guessed wire).
- Per-Contribute progress increment (how much one turn-in adds) — may be
  AccumulationStateTargetValue / a quest-defined amount; confirm against content or sniff.
