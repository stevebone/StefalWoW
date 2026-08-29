-- Deadmines Dungeon
-- Old version before Cataclysm

-- NPC: 598 Defias Miner
-- NPC: 622 Goblin Engineer
-- NPC: 634 Defias Overseer
-- NPC: 636 Defias Blackguard
-- NPC: 639 Edwin VanCleef
-- NPC: 641 Defias Woodcarver
-- NPC: 642 Sneeds Shredder
-- NPC: 643 Sneed
-- NPC: 644 RhahkZor
-- NPC: 645 Cookie
-- NPC: 646 Mr Smite
-- NPC: 647 Captain Greenskin
-- NPC: 657 Defias Pirate
-- NPC: 1725 Defias Watchman
-- NPC: 1729 Defias Evoker
-- NPC: 1731 Goblin Craftsman
-- NPC: 1732 Defias Squallshaper
-- NPC: 1763 Gildnid
-- NPC: 2520 Remote Controlled Golem
-- NPC: 3450 Defias Companion
-- NPC: 3586 Miner Johnson
-- NPC: 3947 Goblin Shipbuilder
-- NPC: 4416 Defias Strip Miner
-- NPC: 4417 Defias Taskmaster
-- NPC: 4418 Defias Wizzard

-- Quest: 373 The Unsent Letter
-- Quest: 7938 Your fortune awaits you

-- AT: 3746 Deadmines Mysterious Chest
-- AT: 6361 Deadmines after cannon door

-- Deadmines Classic Phase
-- Phase 50007 applies Classic Deadmines mobs when instance data DeadminesVersion = 1 (Classic)

DELETE FROM `phase_area` WHERE `PhaseId` = 50007 AND `AreaId` = 1581;
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(1581, 50007, 'Deadmines - Classic Phase');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26 AND `SourceGroup` = 50007 AND `SourceEntry` = 1581;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `Comment`) VALUES
(26, 50007, 1581, 0, 0, 13, 0, 1000, 1, 0, '', 0, 'Deadmines - Apply phase 50007 if instance data DeadminesVersion = 1 (Classic)');

-- Reactivate OLD quests
DELETE FROM `disables` where `sourceType` = 1 AND `entry` IN (373,7938);
UPDATE `quest_template` SET `ContentTuningID` = 202, `StartItem` = 2874, `AllowableRaces` = 18446744073709551615, `Expansion` = 0 WHERE `ID` = 373;
UPDATE `quest_template` SET `ContentTuningID` = 202, `AllowableRaces` = 18446744073709551615, `Expansion` = 0 WHERE `ID` = 7938;


UPDATE `instance_template` SET `script` = 'custom_instance_deadmines' WHERE (`map` = '36');

UPDATE `creature_template` SET `ScriptName` = 'npc_defias_blackguard_old' WHERE `entry` = 636;
UPDATE `creature_template` SET `ScriptName` = 'boss_vancleef' WHERE `entry` = 639;
UPDATE `creature_template` SET `ScriptName` = 'boss_mr_smite' WHERE `entry` = 646;
UPDATE `creature_template` SET `ScriptName` = 'boss_rhahkzor' WHERE `entry` = 644;
UPDATE `creature_template` SET `ScriptName` = 'boss_sneed' WHERE `entry` = 643;
UPDATE `creature_template` SET `ScriptName` = 'boss_gilnid' WHERE `entry` = 1763;

UPDATE `creature_template_addon` SET `auras` = '6408' WHERE `entry` = 636; -- Defias Blackguard Faded aura
UPDATE `creature_template_addon` SET `emote` = 234 WHERE `entry` = 641; -- Goblin Woodcarver chop wood emote


DELETE FROM `areatrigger_scripts` WHERE `Entry` IN (3746,6361);
INSERT INTO `areatrigger_scripts` (`Entry`, `ScriptName`) VALUES
(3746, 'deadmines_mysterious_chest_at'),
(6361, 'deadmines_door_cannon_event_at');

DELETE FROM `creature_template_difficulty` WHERE `entry` IN (598,622,634,636,639,641,642,643,644,645,646,647,657,
1725,1729,1731,1732,1763,2520,3450,3586,3947,4416,4417,4418);
INSERT INTO `creature_template_difficulty` VALUES
(598, 1, 0, 0, 202, 0, 1, 1, 1, 1.7, 409, 2097224, 0, 0, 598, 598, 0, 7, 54, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Defias Miner
(622, 1, 0, 0, 202, 0, 4, 1, 1, 1.7, 431, 2097224, 0, 0, 622, 622, 0, 87, 178, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Goblin Engineer
(634, 1, 0, 0, 202, 0, 6, 1, 1, 1.7, 439, 2097224, 0, 0, 634, 634, 0, 26, 158, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Defias Overseer
(636, 1, 1, 1, 202, 0, 3, 1, 1, 1.7, 440, 72, 0, 0, 636, 636, 0, 49, 121, 524288, 0, 0, 0, 0, 0, 0, 0, 45338), -- Defias Blackguard
(639, 1, 2, 2, 202, 0, 8, 1, 1, 1.7, 442, 0, 0, 0, 639, 639, 0, 51, 421, 268435456, 0, 0, 0, 0, 0, 0, 0, -1), -- Edwin VanCleef
(641, 1, 0, 0, 202, 0, 5, 1, 1, 1.7, 443, 2097224, 0, 0, 641, 641, 0, 32, 161, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Defias Woodcarver
(642, 1, 0, 0, 202, 0, 5, 1, 1, 1.7, 444, 2097256, 128, 0, 642, 0, 0, 159, 262, 0, 0, 0, 0, 0, 0, 0, 0, -1), -- Sneeds Shredder
(643, 1, 0, 0, 202, 0, 5, 1, 1, 7.5, 445, 2097256, 128, 0, 643, 643, 0, 137, 184, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Sneed
(644, 1, 0, 0, 202, 0, 5, 1, 1, 1.7, 446, 2097256, 128, 0, 644, 644, 0, 108, 298, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- RhahkZor
(645, 1, 2, 2, 202, 0, 6, 1, 1, 1.7, 447, 0, 0, 0, 645, 645, 0, 58, 212, 268959744, 0, 0, 0, 0, 0, 0, 0, 45338), -- Cookie
(646, 1, 2, 2, 202, 0, 8, 1, 1, 1.7, 448, 0, 0, 0, 646, 646, 0, 69, 267, 524288, 0, 0, 0, 0, 0, 0, 0, 45338), -- Mr Smite
(647, 1, 2, 2, 202, 0, 6, 1, 1, 1.7, 449, 0, 0, 0, 647, 647, 0, 76, 212, 524288, 0, 0, 0, 0, 0, 0, 0, 45338), -- Captain Greenskin
(657, 1, 1, 1, 202, 0, 3, 1, 1, 1.7, 457, 0, 0, 0, 657, 657, 0, 44, 196, 524288, 0, 0, 0, 0, 0, 0, 0, 45338), -- Defias Pirate
(1725, 1, 0, 0, 202, 0, 1, 1, 1, 1.7, 1416, 0, 0, 0, 1725, 1725, 0, 34, 103, 268959744, 0, 0, 0, 0, 0, 0, 0, -1), -- Defias Watchman
(1729, 1, 0, 0, 202, 0, 4.6, 0.7264, 1, 1.7, 1419, 2097224, 0, 0, 1729, 1729, 0, 23, 156, 268959744, 0, 0, 0, 0, 0, 0, 0, -1), -- Defias Evoker
(1731, 1, 0, 0, 202, 0, 5, 1, 1, 1.7, 1421, 2097224, 0, 0, 1731, 1731, 0, 41, 174, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Goblin Craftsman
(1732, 1, 1, 1, 202, 0, 3, 0.7871, 1, 1.7, 1422, 0, 0, 0, 1732, 1732, 0, 30, 156, 524288, 0, 0, 0, 0, 0, 0, 0, 45338), -- Defias Squallshaper
(1763, 1, 0, 0, 202, 0, 5, 1, 1, 1.7, 1452, 2097256, 128, 0, 1763, 1763, 0, 76, 217, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Gildnid
(2520, 1, 0, 0, 202, 0, 3, 1, 1, 1, 2117, 2097224, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), -- Remote Controlled Golem
(3450, 1, 0, 0, 202, 0, 0.5, 1, 1, 1, 2992, 2097224, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), -- Defias Companion
(3586, 1, 0, 0, 202, 0, 5, 1, 1, 1.7, 3124, 2097224, 0, 0, 3586, 0, 0, 128, 172, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Miner Johnson
(3947, 1, 1, 1, 202, 0, 3, 1, 1, 1.7, 3440, 0, 0, 0, 3947, 3947, 0, 89, 194, 524288, 0, 0, 0, 0, 0, 0, 0, 45338), -- Goblin Shipbuilder
(4416, 1, 0, 0, 202, 0, 4, 1, 1, 1.7, 3873, 2097224, 0, 0, 4416, 4416, 0, 8, 56, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Defias Strip Miner
(4417, 1, 0, 0, 202, 0, 6, 1, 1, 1.7, 3874, 2097224, 0, 0, 4417, 4417, 0, 66, 171, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Defias Taskmaster
(4418, 1, 0, 0, 202, 0, 5, 0.7871, 1, 1.7, 3875, 2097224, 0, 0, 4418, 4418, 0, 79, 169, 268959744, 0, 0, 0, 0, 0, 0, 0, -1); -- Defias Wizzard

-- Defias Cannon Event
UPDATE `gameobject_template` SET `ScriptName` = 'go_defias_cannon' WHERE (`entry` = 442661); -- Defias Cannon (new)
UPDATE `gameobject` SET `id` = 442661, `phaseUseFlags` = 1 WHERE (`guid` = 235341); -- Defias Cannon (new) replacing 16398 and making it visible for all phases

UPDATE `gameobject_template` SET `Data1` = 17155 WHERE `entry` = 17155; -- Defias Gunpowder Loot id
UPDATE `gameobject` SET `phaseUseFlags` = 1 WHERE `guid` = 400647; -- make the entrance portal visible for all phases
UPDATE `gameobject` SET `phaseUseFlags` = 1 WHERE `guid` = 235362; -- make iron clad door visible for all phases
UPDATE `gameobject` SET `phaseUseFlags` = 1 WHERE `guid` = 235298; -- make iron clad lever visible for all phases
UPDATE `gameobject` SET `phaseUseFlags` = 1 WHERE `guid` = 235363; -- make factory door visible for all phases
UPDATE `gameobject` SET `phaseUseFlags` = 1 WHERE `guid` = 235294; -- make factory door lever visible for all phases
UPDATE `gameobject` SET `phaseUseFlags` = 1 WHERE `guid` = 235334; -- make heavy door visible for all phases
UPDATE `gameobject` SET `phaseUseFlags` = 1 WHERE `guid` = 235342; -- make mast room door visible for all phases
UPDATE `gameobject` SET `phaseUseFlags` = 1 WHERE `guid` = 235300; -- make mast room door lever visible for all phases
UPDATE `gameobject` SET `phaseUseFlags` = 1 WHERE `guid` = 235345; -- make heavy door visible for all phases
UPDATE `gameobject` SET `phaseUseFlags` = 1 WHERE `guid` = 235340; -- make foundry door visible for all phases
DELETE FROM `gameobject` WHERE `guid` = 235346; -- delete duplicate foundry door
UPDATE `gameobject` SET `phaseUseFlags` = 1 WHERE `guid` = 235299; -- make foundry door lever visible for all phases

DELETE FROM `gameobject_loot_template` WHERE `Entry` = 17155;
INSERT INTO `gameobject_loot_template` (`Entry`, `ItemType`, `Item`, `Chance`, `LootMode`, `MinCount`, `MaxCount`, `Comment`) VALUES
(17155, 0, 221485, 100, 1, 1, 1, 'Deadmines - Defias Gunpowder'); -- Defias Gunpowder

-- Defias Gunpowder SAI
SET @ENTRY := 17155;
UPDATE `gameobject_template` SET `AIName`="SmartGameObjectAI" WHERE `entry`=@ENTRY;
DELETE FROM `smart_scripts` WHERE `entryorguid`=@ENTRY AND `source_type`=1;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES
(@ENTRY,1,0,0,64,0,100,0,1,0,0,0,12,4417,3,120000,1,0,0,8,0,0,0,-123.77,-613.586,14.126,6.035,"Defias Gunpowder - On Gossip Hello - Summon Creature 'Defias Taskmaster' (No Repeat)");

-- Creature Texts
DELETE FROM `creature_text` WHERE `CreatureID` = 646; -- Mr Smite
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES 
(646, 0, 0, 'You there! Check out that noise!', 14, 7, 100, 5775, 1148, 255, 'Mr Smite to Player'),
(646, 1, 0, 'We''re under attack! Avast, ye swabs! Repel the invaders!', 14, 7, 100, 5777, 1149, 255, 'Mr Smite to Player'),
(646, 2, 0, 'You landlubbers are tougher than I thought! I''ll have to improvise!', 14, 7, 100, 5778, 1344, 100, 'Mr Smite to Player'),
(646, 3, 0, 'D''ah! Now you''re making me angry!', 14, 7, 100, 5779, 1345, 100, 'Mr Smite to Player');

UPDATE `creature_text` SET `Language` = 7 WHERE `CreatureID` = 639 and `GroupID` IN (0, 1, 3, 4, 5); -- VanCleef text

