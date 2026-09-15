# Covenant switching / reset (P5) — client 12.0.7.68275

Retail mechanism, read off the client data rather than assumed.

## Spell 338503 "Reset Covenant"

`SpellEffect` publishes seven effects for 338503:

| Index | Effect | Value | Meaning |
|---|---|---|---|
| 0 | 272 `SPELL_EFFECT_SET_COVENANT` | MiscValue **0** | leave the covenant |
| 1-4 | 139 `SPELL_EFFECT_QUEST_FAIL` | 56066 / 56069 / 56068 / 56067 | re-arm all four covenant-choice quests |
| 5 | 170 `UPDATE_ZONE_AURAS_AND_PHASES` | — | refresh phases |
| 6 | 167 `UPDATE_PLAYER_PHASE` | — | refresh phases |

No cost, no reagent, no cooldown row, no `SpellCategories` row. So the reset is simply
"covenant := 0 and the choice becomes answerable again", and every penalty a switch might have
carried lives outside the spell.

`Spell::EffectSetCovenant` previously rejected MiscValue 0 (`sCovenantStore.LookupEntry(0)` is null),
which made 338503 a silent no-op. It now accepts 0 as the reset.

## What a switch does

Strips (all reversible, all restored on return):

* the covenant `SkillLine` (2730/2731/2732/2733) — `ApplyCovenantSkillLines` already handles this
* the active soulbind (`PlayerData::SoulbindID` → 0), **after** recording it per covenant
* the conduit auras of that soulbind (`RemoveConduitSpells`)
* the soulbind trait auras of that soulbind (`RemoveSoulbindTraitSpells`)
* every `GarrTalentRank.PerkSpellID` granted by a tree belonging to the covenant being left —
  the class + signature abilities, the sanctum perks and the soulbind traits
  (`Garrison::RefreshCovenantTalentPerks`)

Preserves — nothing below is ever deleted, moved or reduced by a switch:

* renown, on the per-covenant currencies 1829/1830/1831/1832
* the granted-reward high-water mark in `character_covenant_renown`
* reservoir anima 1859-1862 and redeemed souls 1863-1866
* every row of `character_garrison_talents`, for all four covenants
* the sanctum garrison itself, with its companions, missions, shipments and trophies
* the conduit collection and every conduit socket
* each covenant's calling board

Re-applies on the covenant being joined:

* its `SkillLine`
* the perks of every rank it has already researched
* the soulbind it was last using, via `ActivateSoulbind`, which brings back that tree's conduits and traits
* the 1822 renown and 1813 anima display currencies, repointed at its own tracks
* its calling board
* its ability tree, **when this is a switch and not a first pledge** (see below)

## Sanctum-talent scoping — the P3.0 / D.12 decision

**Researched sanctum talents are per covenant and are kept across a switch. Only the perks they
grant follow the active covenant.**

This is derived, not chosen for convenience. Every covenant-scoped tree of `GarrTypeID 111` names its
owner in `GarrTalentTree.FeatureSubtypeIndex` (= `Covenant.db2` id), and the four covenants never
share a tree:

| Feature (`FeatureTypeIndex`) | Kyrian | Venthyr | Night Fae | Necrolord |
|---|---|---|---|---|
| 0 Abilities | 393 | 396 | 397 | 395 |
| 1 Anima Conductor | 312 | 314 | 311 | 313 |
| 2 Transport Network | 308 | 309 | 307 | 310 |
| 3 Command Table | 316 | 317 | 315 | 318 |
| 4 Reservoir | 327 | 326 | 328 | 329 |
| 5 Unique feature | 320 | 324 | 319 | 321 |
| 6 Soulbind | 357/360/365 | 304/368/392 | 275/276/334 | 325/330/349 |
| 7 Channel Anima | 345 | 348 | 346 | 347 |

`character_garrison_talents` is keyed by `GarrTalentID`, so a Kyrian Transport Network row and a Night
Fae one are already different rows. The storage is covenant-partitioned for free: there is nothing to
migrate, nothing to key differently and nothing to delete. A returning member finds its sanctum exactly
as it left it. The 24 `FeatureSubtypeIndex 0` trees of type 111 are not covenant-scoped and are never
touched.

The bug this closes is the other half: `ApplyTalentRankPerk` used to grant a perk regardless of which
covenant owned the tree, so a switcher kept the abilities and sanctum perks of the covenant it left —
including across logins, because `ApplyAllTalentPerks` only ever adds. `ApplyTalentRankPerk` now
refuses foreign-covenant trees and `RefreshCovenantTalentPerks` takes back what is already applied. It
runs on every login too, so characters that switched before this existed are repaired.

## Abilities on a switch

The class + signature abilities are handed out by the covenant campaign: quest reward spells
337187 (Kyrian) / 337059 (Venthyr) / 337191 (Night Fae) / 337190 (Necrolord) with 13
`SPELL_EFFECT_LEARN_GARR_TALENT` effects each, and the signature grants 328604 / 320846 / 336692 /
337388. (Resolved by `SpellEffect(279).EffectMiscValue → GarrTalent → GarrTalentTree.FeatureSubtypeIndex`;
note the class-grant order is covenant **1, 2, 4, 3**, not 1-4.)

A switcher never runs a second campaign, so a covenant joined by switching would otherwise have no
abilities at all. `Garrison::GrantCovenantAbilityTalents` seats the 14 talents of that covenant's
ability tree — which is exactly what those grant spells do, since all four ability trees are authored
with a single rank at cost 0 / gold 0 / duration 0 and no prerequisite, and the per-class filtering
happens through `GarrTalentRank.PerkPlayerConditionID`. No spell or ability id is hardcoded.

A **first** pledge is deliberately left alone: there the campaign is still ahead of the character and
grants them itself.

## The 9.1.5 rule

Switching is free once **any** covenant has reached maximum renown, and there is no other gate.

Maximum renown is read, not hardcoded: `CurrencyTypes` 1829-1832 (and the 1822 display mirror) all
publish `MaxQty 79` through the shared `MaxQtyWorldStateID 19735`, and renown level = quantity + 1, so
the cap is Renown 80 — which is also the highest level `RenownRewards.db2` defines for covenants 1-4.
`Player::GetMaxCovenantRenownLevel()` returns it from the currency row.

`Player::GetHighestCovenantRenownLevel()` takes the best of the four tracks, counting a covenant only
when it holds renown or is the active one (quantity 0 means Renown 1 for a member, and must not read
as Renown 1 for the three the character never touched).

Evaluated in two places, both from the same helper:

* `Player::CanChangeCovenant()` — the server-side refusal in `playerchoice_covenant_selection`
* `CONDITION_COVENANT` (62) with `ConditionValue2` = minimum renown level — the client-side visibility
  of the three foreign join responses on PlayerChoice 644

**Not implemented, on purpose:** the launch-era model (a re-join quest chain, a lockout timer and a
renown penalty). None of its numbers exist anywhere in the 12.0.7.68275 client data, so building it
would mean inventing them.

## `CONDITION_COVENANT` (62) gained a renown parameter

`ConditionValue2` is a minimum renown level and is optional; `0` keeps the original membership-only
meaning, and no existing row in `integ_world` uses it (16 type-62 rows, all with `ConditionValue2 = 0`).

* `ConditionValue1 = C, ConditionValue2 = N` → in covenant C **and** its renown is at least N
* `ConditionValue1 = 0, ConditionValue2 = N` → has reached renown N on **any** covenant; membership is
  not required, because renown is per covenant and outlives leaving one. This is what the free-switch
  rule needs.

## Per-covenant soulbind memory

`character_covenant` is single-valued by design (it holds the *active* pledge), so leaving a covenant
had nowhere to keep its soulbind choice. `character_covenant_soulbind (guid, covenantId, soulbindId)`
records it per covenant. A row is written for every covenant the character pledges to — even before it
picks a soulbind — so the table doubles as the "covenants ever joined" set that tells a switch apart
from a first pledge.

## Deliberately left alone

* **Companions of a former covenant.** `Garrison::IsFollowerCovenantAllowed` gates *acquiring* a
  follower, not keeping one. Companions earned under a previous covenant stay in the sanctum roster.
  Removing them would be destructive and hiding them has no data behind it.
* **The sanctum garrison.** A reset does not delete it; it holds every covenant's talents, companions
  and running missions.
* **Conduit Energy** (currency 1883) — deleted by Blizzard in 9.1.5, still not implemented.
* **The Oribos emissary gossip.** World data carries a real switch path — Lady Moonberry's menu 26132
  option 8 "I wish to rejoin the Night Fae" → menu 27404 with the confirmation box "This path will lead
  to you leaving your current covenant. Are you sure?", plus 26041 → 26356 (Venthyr) and 26133 → 26344
  (Necrolord) — but TDB coverage is partial (Kyrian's confirm menu has no `ActionMenuID` at all) and
  none of those options carries a script or a covenant binding. The switch is driven through the Oribos
  covenant map (PlayerChoice 644) instead; wiring the emissaries is world-data work, not engine work.
