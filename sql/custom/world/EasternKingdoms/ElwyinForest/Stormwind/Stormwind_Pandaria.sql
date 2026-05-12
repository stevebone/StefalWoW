-- Phase: 1135 Cosmetic - Stormwind/Elwynn - Aysa
-- Phase: 1136 Cosmetic - Stormwind/Elwynn - Jojo
-- Phase: 1137 Cosmetic - Stormwind/Elwynn - Balloon
-- Phase: 1138 Cosmetic - Ling Completion - See Scroll
-- Phase: 1139 Cosmetic - Stormwind Keep - See Varian

-- Spells: 120344 Summon Aysa (Stormwind Gate)
-- Spells: 120345 Summon Jojo (Stormwind Gate)
-- Spells: 83773 Conversation Trigger 01 (Instant No Delay)
-- Spells: 88811 CSA AT Dummy Timer (30s)
-- Spells: 116601 Timer Aura (5s)

-- AT: 7995 Stormwind Bridge (middle)
-- AT: 5828 Stormwind Bridge (Marshall)
-- AT: 7996 Stormwind City (Trade District Center)
-- AT: 7990 Stormwind City (Trade District Moni npc)
-- AT: 7991 Stormwind City (Trade District > Canal)
-- AT: 7992 Stormwind City (Castle 1st stairs)
-- AT: 7993 Stormwind City (Canals Bridge)
-- AT: 7994 Stormwind City (Canals Brunn Goldenmug)

-- Quest: 30987 Joining the Alliance

-- NPC: 62419 Aysa Stormwind Monk Trainer
-- NPC: 60565 Shang Xi's Hot Air Balloon
-- NPC: 61792 Aysa spawned at Stormwind Gate
-- NPC: 61793 Jojo spawned at Stormwind Gate
-- NPC: 60566 Aysa Stormwind Gate
-- NPC: 60567 Jojo Stormwind Gate
-- NPC: 61834 Alyn Black
-- NPC: 61836 Moni Widdlesprock
-- NPC: 61837 Leria Nightwind
-- NPC: 61841 Brunn Goldenmug
-- NPC: 61839 Lucas Severing
-- NPC: 61840 Naanae

-- Phases
DELETE FROM `phase_area` WHERE `PhaseId` IN (1135,1136,1137);
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(1617, 1135, 'Stormwind - Valley of Heroes see Aysa'),
(7486, 1135, 'Elwynn Forest - Stormwind Gate see Aysa'),
(1617, 1136, 'Stormwind - Valley of Heroes see Jojo'),
(7486, 1136, 'Elwynn Forest - Stormwind Gate see Jojo'),
(1617, 1137, 'Stormwind - Valley of Heroes see Balloon'),
(7486, 1137, 'Elwynn Forest - Stormwind Gate see Balloon');

-- Phase Conditions
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26 AND `SourceGroup` IN (1135,1136,1137);
INSERT INTO `conditions` VALUES
-- Stormwind Gate
(26, 1135, 0, 0, 0, 47, 0, 31450, 74, 0, '', 0, 0, 0, '', 'Stormwind Gate - Phase 1135 active if 31450 taken, in progress, complete, rewarded'),
(26, 1136, 0, 0, 0, 47, 0, 31450, 74, 0, '', 0, 0, 0, '', 'Stormwind Gate - Phase 1136 active if 31450 taken, in progress, complete, rewarded'),
(26, 1137, 0, 0, 0, 47, 0, 31450, 74, 0, '', 0, 0, 0, '', 'Stormwind Gate - Phase 1135 active if 31450 taken, in progress, complete, rewarded'),
(26, 1135, 0, 0, 0, 47, 0, 30987, 1, 0, '', 0, 0, 0, '', 'Stormwind Gate - Phase 1135 active if 30987 NOT taken'),
(26, 1136, 0, 0, 0, 47, 0, 30987, 1, 0, '', 0, 0, 0, '', 'Stormwind Gate - Phase 1136 active if 30987 NOT taken'),
(26, 1137, 0, 0, 0, 47, 0, 30987, 64, 0, '', 1, 0, 0, '', 'Stormwind Gate - Phase 1137 active if 30987 NOT rewarded');

-- On quest accept cast Summon Aysa spell on player
UPDATE `quest_template_addon` SET `SourceSpellID` = 120344 WHERE `ID` = 30987;

-- Spell Conditions
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=13 AND `SourceEntry` IN (120344,120345);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(13, 1, 120344, 0, 0, 51, 0, 5, 60566, 0, 0, 0, 0, '', 'Summon Aysa at Stormwind Gate > Aysa'),
(13, 1, 120345, 0, 0, 51, 0, 5, 60567, 0, 0, 0, 0, '', 'Summon Jojo at Stormwind Gate > Jojo');

-- Update teleport spell location for Stormwind Gate
DELETE FROM `spell_target_position` WHERE `ID` IN (116957);
INSERT INTO `spell_target_position` VALUES
('116957', '0', 0, '0', '-9063.70', '434.73', '93.055', '0.6744', 0),
('116957', '1', 0, '0', '-9063.70', '434.73', '93.055', '0.6744', 0);

-- AT scripts
DELETE FROM `areatrigger_scripts` WHERE `entry` IN (7990,7993,7994);
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES 
(7990, 'at_stormwind_trade_district_7990'),
(7993, 'at_stormwind_canals_7993'),
(7994, 'at_stormwind_canals_7994');

UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_moni_widdlesprock' WHERE `entry` = 61836;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_alyn_black' WHERE `entry` = 61834;

-- Creature Templates
DELETE FROM `creature_template_difficulty` WHERE `entry` IN (60567,60566,60565,61792,61793);
INSERT INTO `creature_template_difficulty` VALUES
(61792, 0, 0, 0, 864, 11, 1, 1, 1, 1, 19012, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(61793, 0, 0, 0, 864, 11, 1, 1, 1, 1, 19010, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60565, 0, 0, 0, 864, 11, 30, 1, 1, 0.3, 21146, 1073741824, 0, 0, 0, 0, 0, 0, 0, 805306368, 0, 262144, 0, 0, 0, 0, 0, 56647),
(60566, 0, 0, 0, 864, 11, 1, 1, 1, 1, 21145, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(60567, 0, 0, 0, 864, 11, 1, 1, 1, 1, 21141, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647);

DELETE FROM `creature_template_addon` WHERE `entry` IN (60567,60566,60565,61792,61793);
INSERT INTO `creature_template_addon` VALUES
(61792, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Aysa Spawned at Stormwind Gate
(61793, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '115672'), -- Jojo Spawned at Stormwind Gate
(60565, 0, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- Shang Xi's Hot Air Balloon
(60566, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Aysa
(60567, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '115672'); -- Jojo

-- SmartAI
 -- Aysa Cloudsinger smart ai
SET @ENTRY := 61792;
UPDATE `creature_template` SET `AIName` = 'SmartAI', `ScriptName` = '' WHERE `entry` = @ENTRY;
DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryOrGuid` = @ENTRY;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `action_param7`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`, `Difficulties`) VALUES
(@ENTRY, 0, 0, 1, 11, 0, 100, 0, 0, 0, 0, 0, 0, 29, 0, 0, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'On respawn - Self: Follow Owner by distance 0, angle 0', ''),
(@ENTRY, 0, 1, 2, 61, 0, 100, 0, 0, 0, 0, 0, 0, 44, 1135, 0, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'On respawn - Owner/Summoner: Remove phase id Cosmetic - Stormwind/Elwynn - Aysa (1135)', ''),
(@ENTRY, 0, 2, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'On respawn - Self: Talk 0 to Owner/Summoner', ''),
(@ENTRY, 0, 3, 4, 38, 0, 100, 0, 1, 1, 0, 0, 0, 29, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On Data Set - Self: Stop Follow', ''),
(@ENTRY, 0, 4, 5, 61, 0, 100, 0, 0, 0, 0, 0, 0, 11, 88811, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On Data Set - Self: Cast 88811 on Self', ''),
(@ENTRY, 0, 5, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 69, 1, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, -8957.971, 517.9630, 96.3556, 0.7743, 'On Data Set - Move to POS', ''),
(@ENTRY, 0, 6, 0, 34, 0, 100, 0, 8, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On Movement Inform - Self: Talk 1', ''),
(@ENTRY, 0, 7, 0, 52, 0, 100, 0, 1, @ENTRY, 0, 0, 0, 45, 1, 2, 0, 0, 0, 0, 0, 9, 466, 0, 50, 0, 0, 0, 0, 'On Text Over 1 - Set Data', ''),
(@ENTRY, 0, 8, 0, 90, 0, 100, 0, 88811, 0, 0, 0, 0, 29, 0, 0, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'On Aura Remove - Self: Follow Owner', ''),
(@ENTRY, 0, 9, 10, 38, 0, 100, 0, 1, 3, 0, 0, 0, 29, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On Data Set - Self: Stop Follow', ''),
(@ENTRY, 0, 10, 11, 61, 0, 100, 0, 0, 0, 0, 0, 0, 1, 2, 0, 1, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'On Data Set - Self: Talk 2 to Owner/Summoner', ''),
(@ENTRY, 0, 11, 12, 61, 0, 100, 0, 0, 0, 0, 0, 0, 11, 116601, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On Data Set - Self: Cast 116601 on Self', ''),
(@ENTRY, 0, 12, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 69, 2, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, -8826.584, 627.582, 94.3396, 0.7692, 'On Data Set - Move to POS', ''),
(@ENTRY, 0, 13, 14, 90, 0, 100, 0, 116601, 0, 0, 0, 0, 1, 3, 0, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'On Aura Remove - Self: Follow Owner', ''),
(@ENTRY, 0, 14, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 29, 0, 0, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'On Aura Remove - Self: Follow Owner', '');

 -- Jojo smart ai
SET @ENTRY := 61793;
UPDATE `creature_template` SET `AIName` = 'SmartAI', `ScriptName` = '' WHERE `entry` = @ENTRY;
DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryOrGuid` = @ENTRY;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `action_param7`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`, `Difficulties`) VALUES
(@ENTRY, 0, 0, 1, 11, 0, 100, 0, 0, 0, 0, 0, 0, 29, 5, 0, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'On respawn - Self: Follow Owner by distance 0, angle 0', ''),
(@ENTRY, 0, 1, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 44, 1136, 0, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'On respawn - Owner/Summoner: Remove phase id Cosmetic - Stormwind/Elwynn - Jojo (1136)', '');

 -- Stormwind Bridge area trigger 7995 smart ai
SET @ENTRY := 7995;
DELETE FROM `areatrigger_scripts` WHERE `entry` = @ENTRY;
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES (@ENTRY, 'SmartTrigger');
DELETE FROM `smart_scripts` WHERE `source_type` = 2 AND `entryOrGuid` = @ENTRY;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `action_param7`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`, `Difficulties`) VALUES
(@ENTRY, 2, 0, 0, 46, 0, 100, 0, 0, 0, 0, 0, 0, 45, 1, 1, 0, 0, 0, 0, 0, 10, 313908, 68, 0, 0, 0, 0, 0, 'On trigger - Triggering player: Set Data on Creature Stormwind City Guard (68) with guid 313908 (fetching)', '');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 22 AND `SourceEntry` = 7995 AND `SourceId` = 2;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `Comment`) VALUES 
(22, 1, 7995, 2, 0, 47, 0, 30987, 10, 0, '', 0, 'Action invoker has quest Joining the Alliance (30987) active');

 -- Stormwind City Guard with guid 313908 smart ai
SET @ENTRY := -313908;
UPDATE `creature_template` SET `AIName` = 'SmartAI', `ScriptName` = '' WHERE `entry` = 68;
DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryOrGuid` = @ENTRY;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `action_param7`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`, `Difficulties`) VALUES
(@ENTRY, 0, 0, 0, 38, 0, 100, 0, 1, 1, 0, 0, 0, 1, 6, 1000, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On Data Set - Self: Talk 6 to invoker', '');

-- pre existing SAI for Stormwind City Guard 68
INSERT INTO `smart_scripts` VALUES
(@ENTRY, '0', '1', '0', '', '0', '0', '100', '0', '0', '0', '2000', '4000', '0', '', '11', '6660', '64', '0', '0', '0', '0', '0', NULL, '2', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Stormwind City Guard - In Combat - Cast \'Shoot\''),
(@ENTRY, '0', '2', '0', '', '9', '0', '100', '0', '0', '5', '11000', '14000', '0', '', '11', '12169', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Stormwind City Guard - Within 0-5 Range - Cast \'Shield Block\''),
(@ENTRY, '0', '3', '0', '', '0', '0', '100', '0', '5000', '9000', '8000', '13000', '0', '', '11', '12170', '0', '0', '0', '0', '0', '0', NULL, '2', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Stormwind City Guard - In Combat - Cast \'Revenge\''),
(@ENTRY, '0', '4', '0', '', '22', '0', '100', '0', '101', '5000', '5000', '0', '0', '', '80', '6800', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Stormwind City Guard - On Received Emote \'Wave\' - Run Script'),
(@ENTRY, '0', '5', '0', '', '22', '0', '100', '0', '78', '5000', '5000', '0', '0', '', '80', '6801', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Stormwind City Guard - On Received Emote \'Salute\' - Run Script'),
(@ENTRY, '0', '6', '0', '', '22', '0', '100', '0', '58', '5000', '5000', '0', '0', '', '80', '6802', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Stormwind City Guard - On Received Emote \'Kiss\' - Run Script'),
(@ENTRY, '0', '7', '0', '', '22', '0', '100', '0', '84', '5000', '5000', '0', '0', '', '80', '6803', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Stormwind City Guard - On Received Emote \'Shy\' - Run Script'),
(@ENTRY, '0', '8', '0', '', '22', '0', '100', '0', '77', '5000', '5000', '0', '0', '', '80', '6804', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Stormwind City Guard - On Received Emote \'Rude\' - Run Script'),
(@ENTRY, '0', '9', '0', '', '22', '0', '100', '0', '22', '5000', '5000', '0', '0', '', '80', '6804', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Stormwind City Guard - On Received Emote \'Chicken\' - Run Script'),
(@ENTRY, '0', '10', '0', '', '22', '0', '100', '0', '17', '5000', '5000', '0', '0', '', '80', '6802', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Stormwind City Guard - On Received Emote \'Bow\' - Run Script'),
(@ENTRY, '0', '11', '0', '', '8', '0', '100', '0', '58533', '0', '1000', '1000', '0', '', '87', '329600', '329601', '329602', '329603', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Stormwind City Guard - On spell hit - Run Random Script');

 -- Stormwind Bridge Marshall area trigger 5828 smart ai
SET @ENTRY := 5828;
DELETE FROM `areatrigger_scripts` WHERE `entry` = @ENTRY;
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES (@ENTRY, 'SmartTrigger');
DELETE FROM `smart_scripts` WHERE `source_type` = 2 AND `entryOrGuid` = @ENTRY;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `action_param7`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`, `Difficulties`) VALUES
(@ENTRY, 2, 0, 1, 46, 0, 100, 0, 0, 0, 0, 0, 0, 45, 1, 1, 0, 0, 0, 0, 0, 9, 61792, 0, 50, 0, 0, 0, 0, 'On trigger - Creature Aysa Cloudsinger (61792) in 0 - 50 yards: Set creature data #1 to 1', ''),
(@ENTRY, 2, 1, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 45, 1, 1, 0, 0, 0, 0, 0, 9, 466, 0, 50, 0, 0, 0, 0, 'On trigger - General Marcus Jonathan (66) in 0 - 50 yards: Set creature data #1 to 1', '');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 22 AND `SourceEntry` = 5828 AND `SourceId` = 2;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `Comment`) VALUES 
(22, 1, 5828, 2, 0, 47, 0, 30987, 10, 0, '', 0, 'Action invoker has quest Joining the Alliance (30987) active'),
(22, 2, 5828, 2, 0, 47, 0, 30987, 10, 0, '', 0, 'Action invoker has quest Joining the Alliance (30987) active');

 -- General Marcus Jonathan smart ai
SET @ENTRY := 466;
UPDATE `creature_template` SET `AIName` = 'SmartAI', `ScriptName` = '' WHERE `entry` = 466;
DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryOrGuid` = @ENTRY AND `id` IN (2,3,4,5,6);
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `action_param7`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`, `Difficulties`) VALUES
(@ENTRY, 0, 2, 0, 38, 0, 100, 0, 1, 1, 0, 0, 0, 1, 1, 2000, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On Data Set - Self: Talk 1 to self', ''),
(@ENTRY, 0, 3, 4, 38, 0, 100, 0, 1, 2, 0, 0, 0, 1, 2, 2000, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On Data Set - Self: Talk 2 to self', ''),
(@ENTRY, 0, 4, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 11, 116601, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On Data Set - Self: Cast 116601 on Self', ''),
(@ENTRY, 0, 5, 0, 90, 0, 100, 0, 116601, 0, 0, 0, 0, 1, 3, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On Aura Remove - Self: Talk 3 to self', ''),
(@ENTRY, 0, 6, 0, 52, 0, 100, 0, 3, @ENTRY, 0, 0, 0, 1, 4, 2000, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On Text Over 3 - Self: Talk 4 to self', '');

 -- Stormwind Trade Distric center area trigger 7996 smart ai
SET @ENTRY := 7996;
DELETE FROM `areatrigger_scripts` WHERE `entry` = @ENTRY;
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES (@ENTRY, 'SmartTrigger');
DELETE FROM `smart_scripts` WHERE `source_type` = 2 AND `entryOrGuid` = @ENTRY;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `action_param7`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`, `Difficulties`) VALUES
(@ENTRY, 2, 0, 0, 46, 0, 100, 0, 0, 0, 0, 0, 0, 45, 1, 3, 0, 0, 0, 0, 0, 9, 61792, 0, 50, 0, 0, 0, 0, 'On trigger - Creature Aysa Cloudsinger (61792) in 0 - 50 yards: Set creature data #1 to 1', '');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 22 AND `SourceEntry` = 7996 AND `SourceId` = 2;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `Comment`) VALUES 
(22, 1, 7996, 2, 0, 47, 0, 30987, 10, 0, '', 0, 'Action invoker has quest Joining the Alliance (30987) active');

-- Creature Text
DELETE FROM creature_text WHERE `CreatureID` IN (61792,61836,61834,61837,61841);
DELETE FROM creature_text WHERE `CreatureID` = 68 AND `GroupID` = 6;
DELETE FROM creature_text WHERE `CreatureID` = 466 AND `GroupID` IN (1,2,3,4);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `SoundPlayType`, `BroadcastTextId`, `TextRange`, `comment`) VALUES 
-- Aysa spawned at Stormwind Gate
(61792, 0, 0, 'Let''s get going, $p. If we are going to be of any use out here, we are going to need allies.', 12, 0, 100, 0, 0, 27389, 0, 61421, 0, 'Aysa Spawned at Stormwind Gate'),
(61792, 1, 0, 'We''re here to join the Alliance. We seek audience with your Emperor.', 12, 0, 100, 2, 5000, 0, 0, 61417, 0, 'Aysa Spawned at Stormwind Gate'),
(61792, 2, 0, 'Look at this!', 14, 0, 100, 5, 5000, 0, 0, 61422, 0, 'Aysa Spawned at Stormwind Gate'),
(61792, 3, 0, 'Well, that''s good news. Once we join the Alliance, we certainly won''t run out of things to do.', 12, 0, 100, 1, 5000, 0, 0, 61423, 0, 'Aysa Spawned at Stormwind Gate'),
-- Stormwind Guard at Stormwind Bridge
(68, 6, 0, 'Never thought I''d see one of your kind walk through here.', 12, 0, 100, 66, 0, 0, 0, 61418, 0, 'Stormwind Guard at Stormwind Bridge'),
-- General Marcus Jonathan
(466, 1, 0, 'Pandaren? What business do you have in Stormwind?', 12, 0, 100, 1, 0, 0, 0, 61397, 0, 'General Marcus Jonathan at Stormwind Bridge'),
(466, 2, 0, 'Emperor? You''ll find KING Anduin inside Stormwind Keep.', 12, 0, 100, 1, 0, 0, 0, 61398, 0, 'General Marcus Jonathan at Stormwind Bridge'),
(466, 3, 0, '<%s eyes Jojo''s pillar.>', 16, 0, 100, 1, 3000, 0, 0, 61399, 0, 'General Marcus Jonathan at Stormwind Bridge'),
(466, 4, 0, 'Behave yourselves inside Stormwind, pandaren. You''re in Alliance territory now.', 12, 0, 100, 397, 0, 0, 0, 61400, 0, 'General Marcus Jonathan at Stormwind Bridge'),
-- Moni
(61836, 0, 0, 'Teacher... look!', 12, 0, 100, 25, 5000, 0, 0, 61404, 0, 'Moni Widdlesprock at Stormwind Trade District'),
(61836, 1, 0, 'Pandaren? Here, in Stormwind?', 12, 0, 100, 6, 5000, 0, 0, 61406, 0, 'Moni Widdlesprock at Stormwind Trade District'),
-- Alyn Black
(61834, 0, 0, 'Yes. Those are pandaren, Moni.', 12, 0, 100, 1, 5000, 0, 0, 61405, 0, 'Alyn Black at Stormwind Trade District'),
-- Leria Nightwind
(61837, 0, 0, 'Welcome to Stormwind!', 12, 0, 100, 3, 0, 0, 0, 61410, 0, 'Leria Nightwind at Stormwind Canals Bridge'),
-- Brunn Goldenmug
(61841, 0, 0, 'Pandaren? I mussht be seein'' thingssh...', 12, 0, 100, 0, 0, 0, 0, 61411, 0, 'Brunn Goldenmug at Stormwind Canals');


-- Spawns
SET @CGUID := 900810;

DELETE FROM `creature` WHERE `id` IN (60567,60566,60565);
INSERT INTO `creature` VALUES
(@CGUID+0, '60565', '0', '12', '7486', '0', '0', '1137', '0', '-1', '0', '0', '-9084.53', '427.842', '92.5858', '0.521299', '300', '0', '0', '100', '0', NULL, NULL, NULL, '1048576', '', NULL, '0'),
(@CGUID+1, '60566', '0', '1519', '1617', '0', '0', '1135', '0', '-1', '0', '0', '-9061.02', '433.229', '93.0557', '0.705292', '300', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0'),
(@CGUID+2, '60567', '0', '1519', '1617', '0', '0', '1136', '0', '-1', '0', '0', '-9065.02', '437.349', '93.0558', '0.550979', '300', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0');
