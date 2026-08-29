-- 2) Re-apply the full Midnight/Journeys state bundle
DELETE FROM `world_state`
WHERE `ID` IN (17742, 18233, 18448, 21379, 22847, 29511, 29512, 29609, 30924, 31021, 31023, 31024, 31025);

INSERT INTO `world_state` (`ID`, `DefaultValue`, `MapIDs`, `AreaIDs`, `ScriptName`, `Comment`) VALUES
(17742, 2, NULL, NULL, '', 'Season sub-index - sniff: 2'),
(18233, 1, NULL, NULL, '', 'Season lock state for Midnight-era client snapshots (66709).'),
(18448, 1, NULL, NULL, '', 'Expansion season index - sniff: 1 (Midnight).'),
(21379, 39, NULL, NULL, '', 'Traveler''s Log tab gate: AdventureJournal PC 104831 -> WSExpr 38537 -> WS[21379] > 1. Retail value: 39.'),
(22847, 1, NULL, NULL, '', 'Delves Season 1 Journey unlock gate: Covenant.db2 ID 36 RequiredPlayerConditionID=150464 -> ModifierTree 428532 -> PlayersRealmWorldState(22847)==1'),
(29511, 5, NULL, NULL, '', 'Current Mythic+ season index for Midnight S1 gates.'),
(29512, 4, NULL, NULL, '', 'Current DelvesSeasonID for Midnight S1 gates.'),
(29609, 4, NULL, NULL, '', 'Delves season display state for Midnight S1.'),
(30924, 5, NULL, NULL, '', 'Midnight Journeys gate phase switch; used by WSExpr 51500/51502.'),
(31021, 20, NULL, NULL, '', 'Renown max quantity worldstate: The Amani Tribe.'),
(31023, 20, NULL, NULL, '', 'Renown max quantity worldstate: The Hara''ti.'),
(31024, 20, NULL, NULL, '', 'Renown max quantity worldstate: Silvermoon Court.'),
(31025, 20, NULL, NULL, '', 'Renown max quantity worldstate: The Singularity.');
