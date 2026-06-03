-- Deadmines Dungeon
-- Modern version after Cataclysm

-- NPC: 47162 Glubtok
-- NPC: 48974 Glubtok Main Platter
-- NPC: 40859 Firewall

-- GO: 208002 Goblin Teleporter

-- Creature Difficulties
DELETE FROM `creature_template_difficulty` WHERE `entry` IN (47162);
INSERT INTO `creature_template_difficulty` VALUES
(47162, 1, 1, 1, 202, 0, 35, 1, 1, 16, 43989, 104, 128, 0, 47162, 0, 0, 13158, 13158, 524288, 0, 0, 0, 0, 0, 0, 0, 64978),
(47162, 2, 2, 2, 1199, 0, 50, 1, 1, 16, 43995, 104, 128, 0, 47162, 0, 0, 13158, 13158, 524288, 0, 0, 0, 0, 0, 0, 0, 64978),
(47162, 24, 2, 2, 1199, 3, 52, 1, 1, 125, 43996, 104, 128, 0, 47162, 0, 0, 13158, 13158, 524288, 0, 0, 0, 0, 0, 0, 0, 64978); -- Glubtok

-- Glubtok encounter
UPDATE `creature_template` SET `ScriptName` = 'boss_glubtok' WHERE `entry` = 47162;
UPDATE `creature_template` SET `ScriptName` = 'npc_glubtok_main_platter' WHERE `entry` = 48974;
UPDATE `creature_template` SET `ScriptName` = 'npc_glubtok_secondary_platter' WHERE `entry` IN (48975,48976,49039,49040,49041,49042);

UPDATE `creature_template` SET `VehicleId` = '1367' WHERE (`entry` = '48976');
UPDATE `creature_template` SET `VehicleId` = '1368' WHERE (`entry` = '49041');
UPDATE `creature_template` SET `VehicleId` = '1369' WHERE (`entry` = '49042');

DELETE FROM `creature` WHERE `id` = 48974 AND `map` = 36;

DELETE FROM `vehicle_template` WHERE `creatureId` IN (48974,48975,48976,49039,49040,49041,49042);
INSERT INTO `vehicle_template` (`creatureId`, `despawnDelayMs`, `Pitch`, `CustomFlags`) VALUES
(48974, 0, NULL, 0),
(48975, 0, NULL, 0),
(48976, 0, NULL, 0),
(49039, 0, NULL, 0),
(49040, 0, NULL, 0),
(49041, 0, NULL, 0),
(49042, 0, NULL, 0);

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN (48974,48975,48976,49039,49040,49041,49042);
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `cast_flags`, `user_type`) VALUES
(48974, 46598, 0, 0),
(48975, 46598, 0, 0),
(48976, 46598, 0, 0),
(49039, 46598, 0, 0),
(49040, 46598, 0, 0),
(49041, 46598, 0, 0),
(49042, 46598, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` IN (91397,87897,87900);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(91397, 'spell_glubtok_firewall_targetting'),
(87897, 'spell_glubtok_generic_proc'),
(87900, 'spell_glubtok_generic_proc');

DELETE FROM `creature_text` WHERE `CreatureID` IN (47162,48974);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `SoundPlayType`, `BroadcastTextId`, `TextRange`, `comment`) VALUES 
(48974, 0, 0, '|TInterface\Icons\spell_holy_innerfire.blp:20|t Glubtok creates a moving |cFFFF0000|Hspell:91398|h[Fire Wall]|h|r!', 16, 0, 0, 0, 0, 0, 0, 49155, 0, 'Glubtok Firewall'),
(47162, 0, 0, 'TOO...MUCH...POWER!!!', 14, 0, 100, 15, 0, 21145, 0, 47422, 0, 'VO_DM_GlubtokBoth_Death01'),
(47162, 1, 0, 'ARCANE POWER!!!', 14, 0, 100, 15, 0, 21146, 0, 47363, 0, 'VO_DM_GlubtokBoth_Spell03'),
(47162, 2, 0, 'Glubtok show you da power of de arcane!', 14, 0, 100, 0, 0, 21151, 0, 47256, 0, 'VO_DM_GlubtokHead1_Aggro01'),
(47162, 3, 0, 'Ha ha ha ha!', 14, 0, 100, 11, 0, 21152, 0, 40112, 0, 'VO_DM_GlubtokHead1_Kill01'),
(47162, 3, 1, '''Sploded dat one!', 14, 0, 0, 0, 0, 21155, 0, 47434, 0, 'VO_DM_GlubtokHead2_Kill01'),
(47162, 4, 0, 'Fists of flame!', 14, 0, 100, 0, 0, 21153, 0, 47239, 0, 'VO_DM_GlubtokHead1_Spell01'),
(47162, 5, 0, 'Glubtok ready?', 14, 0, 100, 1, 0, 21154, 0, 47361, 0, 'VO_DM_GlubtokHead1_Spell02'),
(47162, 6, 0, 'Fists of frost!', 14, 0, 100, 0, 0, 21156, 0, 47238, 0, 'VO_DM_GlubtokHead2_Spell01'),
(47162, 7, 0, 'Let''s do it!', 14, 0, 100, 15, 0, 21157, 0, 108750, 0, 'VO_DM_GlubtokHead2_Spell02');



-- GO: 208002 Goblin Teleporter
UPDATE `gameobject_template` SET `ScriptName` = 'go_goblin_teleporter' WHERE `entry` = 208002;

DELETE FROM `gossip_menu_option` WHERE `MenuID` = 12691;
INSERT INTO `gossip_menu_option` (`MenuID`, `GossipOptionID`, `OptionID`, `OptionText`, `OptionBroadcastTextID`, `VerifiedBuild`) VALUES
(12691, -3248896, 0, 'Press the button labeled \'Wood and Lumber.\'', 50716, 25996),
(12691, -3248897, 1, 'Press the button labeled \'Metal and Scraps.\'', 50717, 25937),
(12691, -3248898, 2, 'Press the button labeled \'Ship Parts.\'', 50718, 25996);