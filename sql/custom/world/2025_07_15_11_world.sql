-- Quest: 26756 Nessa Shadowsong
-- Quest: 26757 The Bounty of Teldrassil
-- Quest: 26758 Flight to Auberdine
-- Quest: 26759 Return to Nessa
-- NPC: 4241 Mydrannul
-- NPC: 10118 Nessa Shadowsong
-- NPC: 3838 Vesprystus
-- NPC: 4200 Laird

DELETE FROM `disables` WHERE `entry` = 26756 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 26757 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 26758 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 26759 AND `sourceType` = 1; -- quest source

DELETE FROM `creature_questender` WHERE `quest` = 26756;
DELETE FROM `creature_questender` WHERE `quest` = 26757;
DELETE FROM `creature_questender` WHERE `quest` = 26758;
DELETE FROM `creature_questender` WHERE `quest` = 26759;
INSERT IGNORE INTO `creature_questender` VALUES
(10118, 26756, 0),
(3838, 26757, 0),
(4200, 26758, 0),
(10118, 26759, 0);

DELETE FROM `creature_queststarter` WHERE `quest` = 26756;
DELETE FROM `creature_queststarter` WHERE `quest` = 26757;
DELETE FROM `creature_queststarter` WHERE `quest` = 26758;
DELETE FROM `creature_queststarter` WHERE `quest` = 26759;
INSERT IGNORE INTO `creature_queststarter` VALUES
(4241, 26756, 0),
(10118, 26757, 0),
(3838, 26758, 0),
(4200, 26759, 0);

UPDATE `quest_template` SET `ContentTuningID` = '72', `RewardBonusMoney` = '30' WHERE (`ID` = '26756');
UPDATE `quest_template` SET `ContentTuningID` = '72', `RewardBonusMoney` = '30' WHERE (`ID` = '26757');
UPDATE `quest_template` SET `ContentTuningID` = '72', `RewardBonusMoney` = '60', `RewardFactionCapIn1` = '7', `RewardFactionCapIn2` = '7', `RewardFactionCapIn3` = '7', `RewardFactionCapIn4` = '7', `RewardFactionCapIn5` = '7' WHERE (`ID` = '26758');
UPDATE `quest_template` SET `ContentTuningID` = '72', `RewardBonusMoney` = '60', `RewardFactionCapIn1` = '7', `RewardFactionCapIn2` = '7', `RewardFactionCapIn3` = '7', `RewardFactionCapIn4` = '7', `RewardFactionCapIn5` = '7' WHERE (`ID` = '26759');

INSERT IGNORE INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`) VALUES ('26756', '0', '0', '0', '0', '26757', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0');
UPDATE `quest_template_addon` SET `PrevQuestID` = '26756', `NextQuestID` = '26758' WHERE (`ID` = '26757');
UPDATE `quest_template_addon` SET `PrevQuestID` = '26757', `NextQuestID` = '26759' WHERE (`ID` = '26758');
UPDATE `quest_template_addon` SET `PrevQuestID` = '26758' WHERE (`ID` = '26759');

-- Spawn for Laird
SET @CGUID := 900000;
DELETE FROM `creature` WHERE `id` = 4200;
INSERT INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES (@CGUID+2,4200,1,148,4659,'0',0,0,0,-1,0,1,7425.01,-246.827,7.6499,4.2985,300,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0);
