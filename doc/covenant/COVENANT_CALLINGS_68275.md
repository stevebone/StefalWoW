# Covenant Callings (P4) — 12.0.7.68275

The "callings" are a covenant's daily quest offer (Shadowlands covenant system). This phase
implements the availability round-trip: the client asks which callings are on offer, and the
server answers with real callings drawn from the covenant's bounty pool.

## Opcodes

| Opcode | Dir | Status | Notes |
|---|---|---|---|
| `CMSG_REQUEST_COVENANT_CALLINGS` (0x3A0269) | client→server | `STATUS_LOGGEDIN` | empty payload |
| `SMSG_COVENANT_CALLINGS_AVAILABILITY_RESPONSE` (0x600024) | server→client | `STATUS_NEVER` | server-emitted |

## Wire — SMSG_COVENANT_CALLINGS_AVAILABILITY_RESPONSE

Recovered from the 68275 client deserializer (0x60-group dispatch), a live sniff, and the
`C_CovenantCallings` Lua API (field-name oracle):

```
Bits<1>  CallingsUnlocked      // 0x80 in the capture -> true; == C_CovenantCallings.AreCallingsUnlocked()
FlushBits
uint32   Count                 // number of callings offered (client constant MaxCallings = 3)
Count x  uint32 BountyID        // Bounty.db2 record ids
```

Observed capture payload: `80 03000000 c3000000 ca000000 d2000000`
= unlocked=true, 3 callings, bounty ids {195, 202, 210}.

Each element is only a `Bounty.db2` id on the wire; the client resolves the rest (icon, quest,
`CallingState` = QuestOffer/QuestActive/QuestCompleted) itself from the bounty's quest status
(`CovenantCallingsUpdated` event delivers `callings` as a list of `BountyInfo`).

## Data model

```
Covenant.db2 (BountySetID) --> Bounty.db2 rows with matching BountySetID (relation field)
                               each Bounty -> QuestV2 (QuestID), FactionID, Icon, TurninPlayerConditionID
BountySet.db2 (LockedQuestID) gates whether callings are unlocked
```

Both DB2 metas already ship in `DB2Metadata.h` (`BountyMeta` FDID 1367866 layout 0xEF863306;
`BountySetMeta` FDID 1367865 layout 0x487670F7). This phase adds the structs, LoadInfo, hotfix
tables/statements and stores, plus a `bounty-by-BountySetID` index in `DB2Manager`.

## Handler behaviour

`WorldSession::HandleRequestCovenantCallings`:
1. Resolve the player's active covenant (`Player::GetActiveCovenant`) → `CovenantEntry::BountySetID`.
2. `CallingsUnlocked` = the set has no `LockedQuestID`, or the player has completed it
   (`Player::IsQuestRewarded`).
3. If unlocked, gather bounties for the set (`DB2Manager::GetBountiesForBountySet`) that have a
   real `QuestID`, and offer up to `MaxCallings` (3) of them via a **deterministic daily rotation**
   (`(GameTime / DAY + i) % poolSize`) so the offer is stable within a day and refreshes daily.

All wire values are real: bounty ids come straight from `Bounty.db2`, the unlock flag from the
covenant's `BountySet` locking quest. Nothing is fabricated. The specific daily selection is a
server gameplay decision (Blizzard's exact rotation is not wire-observable); every id offered is a
valid calling for that covenant.

## Not yet built (P4b)
Accepting/turning in callings runs through the normal quest system (quests carry the
`QuestTagType::CovenantCalling` tag, which TC already recognises), so no dedicated calling accept
opcode is required; the outstanding work is calling-completion reward/tracking polish.
