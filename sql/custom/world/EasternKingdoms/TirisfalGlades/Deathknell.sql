-- Tirisfal Glades Deathknell changes and updates

-- NPC: 1740 Deathguard Saltain
-- NPC: 41200 Deathknell Gate bunny
-- NPC: 49141 Darnell 0
-- NPC: 49337 Darnell 1
-- NPC: 49340 Scarlet Corpse

-- Quest: 24959 Fresh out of the grave
-- Quest: 26800 Recruitment

-- Creature Templates
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_custom_scarlet_corpse' WHERE (`entry` = '49340');
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_custom_darnell_0', `npcflag` = 0 WHERE (`entry` = '49141');
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_custom_darnell', `npcflag` = 0 WHERE (`entry` = '49337');
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_custom_deathguard_saltain' WHERE (`entry` = '1740');

UPDATE `creature_template` SET `IconName` = 'questinteract', `faction` = 7, `npcflag` = 0 WHERE `entry` = 49340;

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN (49340);
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `cast_flags`, `user_type`) VALUES
('49340', '91942', '1', '0');

DELETE FROM `vehicle_template` WHERE `creatureId` IN (49337);
INSERT INTO `vehicle_template` (`creatureId`, `despawnDelayMs`, `Pitch`, `CustomFlags`) VALUES
('49337', '5000', NULL, '0');

DELETE FROM creature_template_addon WHERE entry = 49340;
INSERT INTO creature_template_addon (entry, PathId, mount, MountCreatureID, StandState, AnimTier, VisFlags, SheathState, PvPFlags, emote, aiAnimKit, movementAnimKit, meleeAnimKit, visibilityDistanceType, auras) VALUES 
(49340, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '92230 29266');

DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryorguid` IN (1740, 49141, 49337, 49340);
