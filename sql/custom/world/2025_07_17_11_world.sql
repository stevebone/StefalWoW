-- NPC: 42260 Stormwind Charger
-- NPC: 240 Marshal Dughan

UPDATE `creature_template` SET `ScriptName` = 'npc_marshal_dughan' WHERE `Entry` = 240;
DELETE FROM `creature_template` WHERE `Entry` = 42260;
INSERT INTO `creature_template` VALUES
('42260', '0', '0', 'Stormwind Charger', '', '', NULL, 'vehichleCursor', '0', '0', '7', '0', '1', '1.14286', '1', '0', '0', '2000', '2000', '1', '1', '1', '33554432', '2048', '0', '0', '0', '1', '240', '', '2', '1', '0', '0', '0', '0', '1', '0', '0', 'npc_stormwind_charger', NULL, '61609');

DELETE FROM `creature_template_addon` WHERE `Entry` = 42260;
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`) VALUES ('42260', '1', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0');

DELETE FROM npc_spellclick_spells WHERE npc_entry=42260;
INSERT INTO npc_spellclick_spells (npc_entry, spell_id, cast_flags, user_type)
VALUES (42260, 78749, 0, 0);

DELETE FROM waypoint_path WHERE `PathId` = 1;
insert into waypoint_path values (1, 1, 0, null, 'Stormwind Charger - Guard Thomas');

DELETE FROM waypoint_path_node WHERE `PathId` = 1;
insert into waypoint_path_node values
(1, 1, -9465.519, 74.006, 56.778, 4.590, 0),
(1, 2, -9492.755, 59.428, 55.888, 4.292, 0),
(1, 3, -9541.305, -41.207, 56.514, 4.337, 0),
(1, 4, -9552.302, -133.080, 57.411, 4.039, 0),
(1, 5, -9576.928, -165.167, 57.497, 4.3899, 0),
(1, 6, -9619.863, -285.924, 57.699, 4.729, 0),
(1, 7, -9621.291, -397.493, 57.847, 5.117, 0),
(1, 8, -9587.075, -479.866, 57.803, 4.344, 0),
(1, 9, -9615.864, -554.769, 54.466, 4.547, 0),
(1, 10, -9630.808, -644.583, 50.445, 4.376, 0),
(1, 11, -9658.471, -723.813, 44.490, 4.844, 0),
(1, 12, -9648.625, -797.688, 43.584, 5.443, 0),
(1, 13, -9582.567, -871.272, 43.757, 4.660, 0),
(1, 14, -9586.051, -930.074, 43.371, 4.109, 0),
(1, 15, -9622.826, -983.529, 43.597, 4.821, 0),
(1, 16, -9618.161, -1027.437, 40.138, 5.608, 0);