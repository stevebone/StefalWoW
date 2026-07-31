--
-- Garrison exit AreaTriggers: let players leave the garrison instance back to Draenor (1116) from ANY
-- garrison level and BOTH factions.
--
-- Bug: leaving the garrison left the player stuck on the garrison child map. The garrison enter/leave is a
-- seamless map transfer (confirmed in the 12.0.x WoD garrison sniffs: the gate walk is SMSG_NEW_WORLD
-- reason=21 with no loading screen; only the Garrison Hearthstone is a full transfer). The mechanism is a
-- boundary AreaTrigger: on Draenor you enter the garrison-footprint polygon -> at_garrison_enter ->
-- Garrison::Enter(); on the garrison child map you exit that polygon at the gate -> at_garrison_exit ->
-- Garrison::Leave() (TELE_TO_SEAMLESS to the parent map). Garrison::Leave() itself is already correct.
--
-- Root cause: only the exit trigger for Alliance Garrison L1 (map 1158) was spawned, and no Horde trigger
-- at all. So an upgraded Alliance garrison (L2 = 1331, L3 = 1159) or any Horde garrison had no exit trigger,
-- OnUnitExit never fired, and the player was stranded on the instance.
--
-- Fix: reuse the existing garrison boundary polygon (areatrigger_create_properties Id 45, IsCustom=1) and
-- spawn the exit trigger on every garrison child map, plus a Horde enter trigger on Draenor. WoD garrisons
-- (Lunarfall / Frostwall) share the same layout, so the same polygon fits both factions.
--   Alliance (Lunarfall, Shadowmoon Valley) gate: 1900.9937, 221.3306, 76.9551 -- child maps 1158/1331/1159
--   Horde   (Frostwall, Frostfire Ridge)   gate: 5578.9,    4564.8,   136.9    -- child maps 1152/1330/1153
-- Horde gate position is the game_tele "FrostwallGarrisonFrostfireRidge" arrival point; the polygon centring
-- may want minor tuning against a live Horde garrison, but erring toward covering the whole footprint only
-- risks the trigger firing slightly early/late at the gate, never a stuck player.
--
DELETE FROM `areatrigger` WHERE `SpawnId` BETWEEN 9200006 AND 9200011 AND `IsCustom` = 1;
INSERT INTO `areatrigger`
    (`SpawnId`, `AreaTriggerCreatePropertiesId`, `IsCustom`, `MapId`, `SpawnDifficulties`, `PosX`, `PosY`, `PosZ`, `Orientation`, `PhaseUseFlags`, `PhaseId`, `PhaseGroup`, `ScriptName`, `Comment`, `VerifiedBuild`) VALUES
    -- Alliance Lunarfall: exit trigger on the upgraded-level maps (L1 map 1158 already spawned)
    (9200006, 45, 1, 1331, '1', 1900.9937, 221.3306,  76.9551, 0, 0, 0, 0, 'at_garrison_exit',  'Leave Garrison Alliance L2', 0),
    (9200007, 45, 1, 1159, '1', 1900.9937, 221.3306,  76.9551, 0, 0, 0, 0, 'at_garrison_exit',  'Leave Garrison Alliance L3', 0),
    -- Horde Frostwall: enter trigger on Draenor + exit trigger on each level map
    (9200008, 45, 1, 1116, '0', 5578.9000, 4564.8000, 136.9000, 0, 0, 0, 0, 'at_garrison_enter', 'Enter Garrison Horde',      0),
    (9200009, 45, 1, 1152, '1', 5578.9000, 4564.8000, 136.9000, 0, 0, 0, 0, 'at_garrison_exit',  'Leave Garrison Horde L1',   0),
    (9200010, 45, 1, 1330, '1', 5578.9000, 4564.8000, 136.9000, 0, 0, 0, 0, 'at_garrison_exit',  'Leave Garrison Horde L2',   0),
    (9200011, 45, 1, 1153, '1', 5578.9000, 4564.8000, 136.9000, 0, 0, 0, 0, 'at_garrison_exit',  'Leave Garrison Horde L3',   0);
