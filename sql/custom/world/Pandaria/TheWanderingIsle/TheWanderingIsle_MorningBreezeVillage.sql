-- Quest: 29777 Tools of the enemy
-- Quest: 29778 Rewritten wisdoms
-- Quest: 29783 Stronger than stone
-- Quest: 29779 The direct solution
-- Quest: 29780 Do no evil
-- Quest: 29781 Monkey Advisory warning
-- Quest: 29784 Balanced perspective
-- Quest: 29785 Dafeng-the-spirit-of-air
-- Quest: 29786 Battle for the skies
-- Quest: 29787 Worthy Of Passing
-- Quest: 29789 Small but significant
-- Quest: 29788 Unwelcome nature
-- Quest: 29790 Passing Wisdom
-- Quest: 29791 The Suffering of Shen-zin Su
-- Quest: 29782 Stronger than bone

-- NPC: 57623 Shen Stonecarver
-- NPC: 55588 Elder Shaopai at Morning Breeze
-- NPC: 55583 Ji Firepaw at Morning Breeze
-- NPC: 55585 Jojo at Morning Breeze
-- NPC: 57670 Jojo spawn at Morning Breeze
-- NPC: 55601 Fe-feng Wiseman
-- NPC: 55632 Fe-Feng Ruffian
-- NPC: 57465 Fe-Feng Ruffian
-- NPC: 57466 Fe-Feng Firethief
-- NPC: 55633 Fe-Feng Firethief
-- NPC: 55634 Ruk-Ruk
-- NPC: 64322 Ruk-Ruk's rocket
-- NPC: 65558 Ji Firepaw (with player)
-- NPC: 65559 Vision of Zhao-Ren
-- NPC: 55650 Shang Xi's Hot Air Balloon
-- NPC: 55592 Dafeng Spirit of Air
-- NPC: 65560 Dafeng spawn ??
-- NPC: 64532 Dafeng spawn (quest accept)
-- NPC: 55595 Aysa with Dafeng
-- NPC: 64543 Aysa spawn  (quest accept)
-- NPC: 55786 Zhaoren
-- NPC: 64507 Firework Launcher
-- NPC: 64505 Ji at Zhaoren
-- NPC: 64506 Aysha at Zhaoren
-- NPC: 55874 Dead Zhaoren
-- NPC: 55586 Master Shang Xi - Zhaoren dead phase
-- NPC: 56159 Master Shang (spawned for quest 29787)
-- NPC: 55672 Master Shang for the forest quests
-- NPC: 56274 Guardian of the Elders
-- NPC: 56686 Another Shang in the forest (spawn)
-- NPC: 55640 Thornbranch Scamp
-- NPC: 57799 Spawned Thornbranch
-- NPC: 65545 Huojin Monk
-- NPC: 57690 Jade Tiger Pillar vehicle
-- NPC: 57691 Jade Tiger Pillar accessory

DELETE FROM `creature_queststarter` WHERE `quest` IN (29776, 29777,29778,29783,29779,29780,29781,29785,29786,29787,29788, 29789,29790,29791);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES 
('55588', '29777', '0'),
('55583', '29778', '0'),
('55585', '29783', '0'),
('55583', '29779', '0'),
('55583', '29780', '0'),
('55583', '29781', '0'),
('55595', '29785', '0'),
('55595', '29786', '0'),
('55586', '29787', '0'),
('55672', '29788', '0'),
('55672', '29789', '0'),
('55672', '29790', '0'),
('54786', '29776', '0'), -- Quest: 29776 morning-breeze-village
(56662, 29791, 0);

DELETE FROM `quest_template_addon` WHERE `ID` IN (29776,29782);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29776', '0', '0', '104396', '29775', '29778', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''), -- Quest: 29776 morning-breeze-village
('29782', '0', '0', '0', '29778', '29784', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '1', '0', '');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29777,29778,29783);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29777', '0', '0', '0', '29776', '0', '-29778', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29778', '0', '0', '0', '29776', '0', '-29778', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29783', '0', '0', '0', '29776', '0', '-29778', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29779,29780,29781);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29779', '0', '0', '0', '29778', '29784', '-29779', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29780', '0', '0', '0', '29778', '29784', '-29779', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29781', '0', '0', '0', '29778', '29784', '-29779', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29784,29785);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29784', '0', '0', '116992', '29780', '29785', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29785', '0', '0', '0', '29784', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', 'quest_29785_dafeng_the_spririt_of_air');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29786);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29786', '0', '0', '0', '29785', '29787', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29787);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
-- remove spell since it blocks SAI
-- remove script since no longer needed
-- ('29787', '0', '0', '105333', '29786', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', 'quest_29787_worthy_of_passing');
('29787', '0', '0', '0', '29786', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29788, 29789, 29790, 29791);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29788', '0', '0', '0', '29787', '29790', '-29788', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29789', '0', '0', '0', '29787', '29790', '-29788', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29790', '0', '0', '106623', '29788', '29791', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29791', '0', '0', '0', '29790', '29792', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

UPDATE `creature_template` SET `ScriptName` = 'npc_lorewalker_ruolin' WHERE `Entry` = 64876; -- Lorewalker event
UPDATE `creature_template` SET `ScriptName` = 'npc_ji_morning_breeze_docks' WHERE `Entry` = 55583;
UPDATE `creature_template` SET `faction` = 16 WHERE (`entry` = 57465); -- was 14 incorrect faction
UPDATE `creature_template` SET `VehicleId` = 1950 WHERE `entry` = 57690;

DELETE FROM `gossip_menu` WHERE `MenuID` IN (13204);
INSERT INTO `gossip_menu` (`MenuID`, `TextID`, `VerifiedBuild`) VALUES 
(13204, 18608, 61967); -- Ji at Morning Breeze Village dock

DELETE FROM `gossip_menu_option` WHERE `MenuID` = 13204 AND `GossipOptionID` = 40096;
INSERT INTO `gossip_menu_option` (`MenuID`, `GossipOptionID`, `OptionID`, `OptionNpc`, `OptionText`, `OptionBroadcastTextID`, `Language`, `Flags`, `ActionMenuID`, `ActionPoiID`, `GossipNpcOptionID`, `BoxCoded`, `BoxMoney`, `BoxText`, `BoxBroadcastTextID`, `SpellID`, `OverrideIconID`, `VerifiedBuild`) VALUES 
(13204, 40096, 0, 0, 'Maybe you should talk to her.', 62476, 0, 0, 14268, 0, NULL, 0, 0, '', 0, NULL, NULL, 61967);

DELETE FROM `vehicle_template_accessory` WHERE `entry` IN (57464,57690);
INSERT INTO `vehicle_template_accessory` (`entry`, `accessory_entry`, `seat_id`, `minion`, `description`, `summontype`, `summontimer`) VALUES
(57464, 57465, 0, 0, 'Fe-Feng Ruffian', 7, 0),
(57690, 57691, 0, 1, 'Tiger Pillar', 8, 0);

DELETE FROM `creature_template_addon` WHERE `entry` IN (55585,57419,65545, 65560, 65559, 65550, 55650, 55601,55632,57464,57465,65558,57466,55633,55634,57692,
55744,55595,55592,64543,64532,64507,64505,64506,55874,55786,55586,56274,56159,55672,55640,57799,56686);
INSERT INTO `creature_template_addon` 
(`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('55585', '0', '0', '0', '1', '0', '1', '0', '0', '461', '0', '0', '0', '0', '84886'),
-- Fe-Feng Firethief
(57466, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(55633, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '127932'),

(57692, 0, 0, 0, 0, 0, 0, 1, 0, 0, 2935, 0, 0, 0, ''), -- Jojo spawn pillar quest
(55634, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Ruk-Ruk
(65558, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '99203 105305'), -- Ji/Monk guardian

-- Fe-Feng Ruffian
(55632, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(57464, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- vehicle bunny hanging monkey
(57465, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- hanging npc

(55601, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Fe-Feng Wiseman
(55650, 0, 0, 0, 0, 3, 1, 1, 0, 0, 0, 0, 0, 3, '82358'), -- Balloon at Morning Breeze dock
(65550, 0, 0, 0, 0, 0, 0, 1, 0, 426, 0, 0, 0, 0, ''), -- Tushui Monk
(65559, 0, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Vision of Zhao-Ren
(65560, 6556000, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Vision of Dafeng
(57419, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(65545, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),

-- Wood of Staves
(55672, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '108900 126160'), -- Master Shang in the forest
(56159, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '105329'), -- Master Shang towards the forest
(56686, 0, 0, 0, 8, 0, 0, 1, 0, 0, 0, 0, 0, 0, '126160'), -- Master Shang spawned in the forest
(56274, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Guardian of the Elders
(55640, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Thornbranch Scamp
(57799, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Spwaned Thornbranch Scamp

-- Zhao-Ren Event
(55586, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '108900 126160'), -- Master Shang when Zhao-Ren is dead
(55786, 0, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- Zhao-Ren
(55874, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '29266'), -- NPC: 55874 Dead Zhaoren
(64505, 0, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- NPC: 64505 Ji at Zhaoren
(64506, 0, 0, 0, 8, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- NPC: 64506 Aysha at Zhaoren
(64507, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '81312'), -- NPC: 64507 Firework Launcher
(64532, 0, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Dafeng spawned at Dafeng
(64543, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Aysa spawned at Dafeng
(55744, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '104734'), -- Aysa outside chamber
(55595, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, ''), -- Aysa in chamber
(55592, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 3, '80797'); -- Dafeng in chamber

DELETE FROM `creature_template_difficulty` WHERE `entry` IN (55585,57419,65545,55583, 65560, 65559, 65550, 55650,55632,57464,57465,55601,65558,
57466,55633,55634,57692,55744,55595,55592,64543,64532,64507,64505,64506,55874,55786,55586,55672,55640,57799,56686,56159,56274);
INSERT INTO `creature_template_difficulty` VALUES
(56686, 0, 0, 0, 80, 4, 1, 1, 1, 1, 27511, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(57799, 0, 0, 0, 80, 4, 0.1, 1, 1, 0.2, 26130, 256, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(55640, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 29400, 256, 0, 0, 55640, 0, 0, 9, 9, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(55672, 0, 0, 0, 80, 4, 1, 1, 1, 1, 29340, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(56159, 0, 0, 0, 80, 4, 1, 1, 1, 1, 28549, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(56274, 0, 0, 0, 80, 4, 3, 1, 1, 0.2, 28250, 2147549697, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(55586, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 29498, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(55786, 0, 0, 0, 80, 4, 12, 1, 1, 0.2, 29125, 2147483661, 0, 0, 0, 0, 0, 0, 0, 805306368, 0, 0, 0, 0, 0, 0, 0, 56647),
(55874, 0, 0, 0, 80, 4, 5, 1, 1, 0.2, 28953, 5, 0, 0, 0, 0, 0, 0, 0, 268435712, 0, 0, 0, 0, 0, 0, 0, 56647),
(64505, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 57869, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 262144, 0, 0, 0, 0, 0, 56647),
(64506, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 57870, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 262144, 0, 0, 0, 0, 0, 56647),
(64507, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 57871, 1610612752, 6, 0, 0, 0, 0, 0, 0, 805306624, 0, 0, 0, 0, 0, 0, 0, 56647),
(64532, 0, 0, 0, 80, 4, 1, 1, 1, 1, 57918, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(64543, 0, 0, 0, 80, 4, 1, 1, 1, 1, 57929, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(55592, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 29485, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(55595, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 29479, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(55744, 0, 0, 0, 80, 4, 1, 1, 1, 1, 29184, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(57692, 0, 0, 0, 80, 4, 1, 1, 1, 1, 26287, 2097160, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(55634, 0, 0, 0, 80, 4, 4.5, 1, 1, 0.2, 29411, 2147483720, 0, 0, 55634, 0, 0, 5, 5, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(65558, 0, 0, 0, 80, 4, 3, 1, 1, 1, 59279, 4096, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 262144, 0, 0, 0, 0, 0, 56647),
(55601, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 29467, 0, 0, 0, 55601, 55601, 0, 5, 5, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(55632, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 29415, 0, 0, 0, 55632, 55632, 0, 7, 7, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(57464, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 26403, 0, 0, 0, 0, 0, 0, 0, 0, 536871168, 0, 0, 0, 0, 0, 0, 0, 56647),
(57465, 0, 0, 0, 80, 4, 1, 1, 1, 1, 26401, 0, 0, 0, 57465, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(57466, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 26399, 0, 0, 0, 57466, 0, 0, 7, 7, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(55633, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 29413, 0, 0, 0, 55633, 0, 0, 7, 7, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(55650, 0, 0, 0, 80, 4, 30, 1, 1, 0.2, 29383, 1073741824, 0, 0, 0, 0, 0, 0, 0, 805306368, 0, 262144, 0, 0, 0, 0, 0, 56647),
(65550, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 59264, 0, 0, 0, 0, 0, 0, 0, 0, 805306624, 0, 0, 0, 0, 0, 0, 0, 56647),
(65559, 0, 0, 0, 80, 4, 8, 1, 1, 0.2, 59280, 2147483661, 0, 0, 0, 0, 0, 0, 0, 805306368, 0, 0, 0, 0, 0, 0, 0, 56647),
(65560, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 59281, 0, 0, 0, 0, 0, 0, 0, 0, 805306368, 0, 0, 0, 0, 0, 0, 0, 56647),
(55583, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 29504, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(65545, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 59259, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(55585, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 29500, 2097160, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647),
(57419, 0, 0, 0, 80, 4, 1, 1, 1, 0.2, 26449, 0, 0, 0, 0, 0, 0, 0, 0, 268435456, 0, 0, 0, 0, 0, 0, 0, 56647);

UPDATE `creature` SET `PhaseId` = '1518' WHERE (`guid` = '451479'); -- Master Shang at Morning Breeze
UPDATE `creature` SET `PhaseId` = '1519' WHERE (`guid` = '451465'); -- Elder Shaopai at Morning Breeze
UPDATE `creature` SET `PhaseId` = '1523', `MovementType` = 2 WHERE (`guid` = '451588'); -- Vision of Dafeng
UPDATE `creature` SET `PhaseId` = '1523' WHERE (`guid` = '451589'); -- Vision of Zhao-Ren

UPDATE `creature` SET `id` = 57464 WHERE `guid` IN (451551,451558); -- these should be hanging monkey bunnies
DELETE FROM `creature` WHERE `guid` IN (451514,451517,451519,451524,451553); -- hanging monkeys are spawned by bunnies

-- Morning Breeze Villager with waypoints
UPDATE `creature` SET `MovementType` = 2 WHERE `guid` IN (451438, 451439, 451472);
-- Fe-Feng Wiseman with waypoints
UPDATE `creature` SET `MovementType` = 2 WHERE `guid` IN (451428,451509,451508,451484,451483,451432,451505,451495,451597);
-- Fe-Feng Ruffian with waypoints
UPDATE `creature` SET `MovementType` = 2 WHERE `guid` IN (451577,451562);
-- Fe-Feng Firethief with waypoints
UPDATE `creature` SET `MovementType` = 2 WHERE `guid` IN (451563,451583,451532,451587,451516);
-- Thornbranch Scamp with waypoints
UPDATE `creature` SET `MovementType` = 2 WHERE `guid` IN (451708,451584,451601,451603,451607,451606,451672,451714,451626,451629,
451487,451636,451655,451706,451653,451705,451684,451683,451697,451710,451709,451716,451661,451658,451660,451664,451674,451665,451676,451694);

UPDATE `creature` SET `id` = 55633 WHERE `guid` = 451583; -- incorrect Firethief id spawn

DELETE FROM `creature_formations` WHERE `leaderGUID` IN (451588);
INSERT INTO `creature_formations` (`leaderGUID`, `memberGUID`, `dist`, `angle`, `groupAI`, `point_1`, `point_2`) VALUES
(451588, 451588, 0, 0, 512, 0, 0),
(451588, 451589, 10, 0, 512, 0, 0);

DELETE FROM `creature_addon` WHERE `guid` IN (451461,451457,451466,451456,451467,451463,451458,451468,451459,451441,451436,451446,451448,451438,451472,451473,
451439,451428,451509,451508,451484,451483,451432,451505,451495,451597,451510,451443,451462,451577,451562,451563,451583,451532,451587,451516,451554,451556,451557,
451550,451560,451578,451610,451477,451621,451708,451584,451601,451603,451607,451606,451672,451714,451626,451629,451487,451636,451655,451706,451653,451705,451684,
451683,451697,451710,451709,451716,451661,451658,451660,451664,451674,451665,451676,451694,451625,451486,451649,451707,451680,451691,451703,451662,451666,451670,
451611,451600,451604,451687,451711,451663);
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
-- Fe-Feng Firethief
(451563, 5563300, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '127932'),
(451583, 5563301, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '127932'),
(451532, 5563302, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '127932'),
(451587, 5563303, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '127932'),
(451516, 5746600, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), 
(451554, 0, 0, 0, 0, 0, 0, 1, 0, 421, 0, 0, 0, 0, '127932'),
(451556, 0, 0, 0, 0, 0, 0, 1, 0, 421, 0, 0, 0, 0, ''),
(451557, 0, 0, 0, 0, 0, 0, 1, 0, 421, 0, 0, 0, 0, ''),
(451550, 0, 0, 0, 0, 0, 0, 1, 0, 421, 0, 0, 0, 0, ''),
(451560, 0, 0, 0, 0, 0, 0, 1, 0, 421, 0, 0, 0, 0, ''),
(451578, 0, 0, 0, 0, 0, 0, 1, 0, 421, 0, 0, 0, 0, ''),
-- Fe-Feng Ruffian
(451462, 0, 0, 0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, '119073'),
(451577, 5563200, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451562, 5563201, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
-- Fe-Feng Wiseman
(451443, 0, 0, 0, 0, 0, 0, 1, 0, 421, 0, 0, 0, 0, ''),
(451510, 0, 0, 0, 0, 0, 0, 1, 0, 421, 0, 0, 0, 0, ''),
(451428, 5560100, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451509, 5560101, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451508, 5560102, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451484, 5560103, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451483, 5560104, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451432, 5560105, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451505, 5560106, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451495, 5560107, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451597, 5560108, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
-- Chamber of Whispers
(451610, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '82343'), -- Aysa inside the chamber
(451477, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '78718 121801 81312'), -- Aysa at docks
(451621, 0, 0, 0, 8, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Aysa at Zhao-Ren

(451473, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '85096'), -- Ji at the Morning Breeze Village dock
-- Jojo's crowd
(451461, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '80852'), -- morning breeze jojo power quest
(451457, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '80852'),
(451466, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '80852'),
(451456, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '80852'),
(451467, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '80852'),
(451463, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '80852'),
(451458, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '80852'),
-- Huojin Monk
(451468, 0, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''),
(451459, 0, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''),
-- Morning Breeze Villager
(451441, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451436, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451446, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451448, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451438, 5741900, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451439, 5741901, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),
(451472, 5741902, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''),

-- Thornbranch Scamp
(451625, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123906'),
(451486, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123906'),
(451649, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123906'),
(451707, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123906'),
(451680, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123906'),
(451691, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123906'),
(451703, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123906'),
(451662, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123906'),
(451666, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123906'),
(451670, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123906'),
(451611, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123906'),

(451600, 0, 0, 0, 0, 0, 0, 1, 0, 133, 0, 0, 0, 0, ''),
(451604, 0, 0, 0, 0, 0, 0, 1, 0, 133, 0, 0, 0, 0, ''),
(451687, 0, 0, 0, 0, 0, 0, 1, 0, 133, 0, 0, 0, 0, ''),
(451711, 0, 0, 0, 0, 0, 0, 1, 0, 133, 0, 0, 0, 0, ''),
(451663, 0, 0, 0, 0, 0, 0, 1, 0, 133, 0, 0, 0, 0, ''),

(451708, 5564000, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451584, 5564001, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451601, 5564002, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451603, 5564003, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451607, 5564004, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451606, 5564005, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451672, 5564006, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451714, 5564007, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451626, 5564008, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451629, 5564009, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451487, 5564010, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451636, 5564011, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451655, 5564012, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451706, 5564013, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451653, 5564014, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451705, 5564015, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451684, 5564016, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451683, 5564017, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451697, 5564018, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451710, 5564019, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451709, 5564020, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451716, 5564021, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451661, 5564022, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451658, 5564023, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451660, 5564024, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451664, 5564025, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451674, 5564026, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451665, 5564027, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451676, 5564028, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  ''),
(451694, 5564029, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  '');

-- Scripts for Jojo
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (57670,65467,56394,56393);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (57670,5767000,65467,56394,56393,6546700,5639400,5639300) AND `source_type` IN (0,9);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('57670', '0', '0', '0', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '5767000', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Just Spawned - Run Script'),
('5767000', '9', '0', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '1', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Talk'),
('5767000', '9', '1', '0', '0', '0', '100', '0', '2500', '2500', '1000', '1000', '0', '', '128', '1', '1', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Set AI AnimKit'),
('5767000', '9', '2', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '69', '1', '0', '0', '0', '0', '0', '8', '0', '0', '0', '1077.31', '4179.94', '205.774', '0', 'Jojo Ironbrow - Move to Pos'),
('5767000', '9', '3', '0', '0', '0', '100', '0', '3000', '3000', '1000', '1000', '0', '', '11', '129294', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Cast Jojo Headbash, Blocks Cast'),
('5767000', '9', '4', '0', '0', '0', '100', '0', '2500', '2500', '1000', '1000', '0', '', '1', '1', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Talk'),
('5767000', '9', '5', '0', '0', '0', '100', '0', '100', '100', '0', '0', '0', '', '28', '108846', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Remove Auras'),
('5767000', '9', '6', '0', '0', '0', '100', '0', '100', '100', '0', '0', '0', '', '28', '108846', '0', '0', '0', '0', '0', '18', '50', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Remove Player Auras'),
('5767000', '9', '7', '0', '0', '0', '100', '0', '100', '100', '0', '0', '0', '', '28', '108846', '0', '0', '0', '0', '0', '11', '0', '10', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Remove Creatures Auras'),
('5767000', '9', '8', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '45', '1', '1', '0', '0', '0', '0', '11', '0', '10', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Set Data'),
('65467', '0', '0', '0', '38', '0', '100', '0', '1', '1', '0', '0', '0', '', '80', '6546700', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Mesmerized Onlooker - On Data Set - Run Script'),
('56394', '0', '0', '0', '38', '0', '100', '0', '1', '1', '0', '0', '0', '', '80', '5639400', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Mesmerized Child - On Data Set - Run Script'),
('56393', '0', '0', '0', '38', '0', '100', '0', '1', '1', '0', '0', '0', '', '80', '5639300', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Mesmerized Onlooker - On Data Set - Run Script'),
('5639300', '9', '0', '0', '0', '0', '100', '0', '2000', '2000', '1000', '1000', '0', '', '5', '4', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Excited Onlooker - Play Emote'),
('5639300', '9', '1', '0', '0', '0', '100', '0', '3500', '3500', '1000', '1000', '0', '', '5', '21', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Excited Onlooker - Play Emote'),
('5639400', '9', '0', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '5', '18', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Mesmerized Child - Play Emote'),
('6546700', '9', '0', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '5', '4', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Excited Onlooker - Play Emote'),
('6546700', '9', '1', '0', '0', '0', '100', '0', '4000', '4000', '1000', '1000', '0', '', '5', '21', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Excited Onlooker - Play Emote'),
('6546700', '9', '2', '0', '0', '0', '100', '0', '5000', '5000', '1000', '1000', '0', '', '5', '4', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Excited Onlooker - Play Emote');


UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (55601,57465,55632,65558,57466,55633,57690,57692);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55632,5563200, 57465,55601, 5560100, 5560101, 5560102, -451429, -451502, -451484, -451503, -451507, 
-451595, -451496, -451594, -451513, -451435, -451592, -451452, -451449, -451504, -451497, -451437, -451569, -451520, -451526, -451525, -451533, -451537, 
-451543, -451541, -451547,-451529,65558,6555800,57466,55633,-451518,-451570,-451564,-451565,-451572,-451542,-451539,-451532,-451563,-451549,-451552,
57690,57692,5769200) AND `source_type` IN (0,9);
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
-- Jojo Ironbrow
(57692, 0, 0, 0, 54, 0, 100, 0, 0, 0, 0, 0, 0, 80, 5769200, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Jojo Ironbrow - Just Spawned - Run Script'),
(5769200, 9, 0, 0, 0, 0, 100, 0, 1500, 1500, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Jojo Ironbrow - Talk'),
(5769200, 9, 1, 0, 0, 0, 100, 0, 2000, 2000, 0, 0, 0, 128, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Jojo Ironbrow - Set AI AnimKit'),
(5769200, 9, 2, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 69, 1, 0, 0, 0, 0, 0, 8, 0, 0, 0, 1077.47, 4180.03, 205.7929, 0, 'Jojo Ironbrow - Move to Pos'),
(5769200, 9, 3, 0, 0, 0, 100, 0, 5000, 5000, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Jojo Ironbrow - Talk'),
(5769200, 9, 4, 0, 0, 0, 100, 0, 6000, 6000, 0, 0, 0, 11, 129297, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Jojo Ironbrow - Cast Jojo Headbash, Pillar Cast'),
(5769200, 9, 5, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 128, 1078, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Jojo Ironbrow - Set AI AnimKit'),
(5769200, 9, 6, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 45, 2, 2, 0, 0, 0, 0, 11, 0, 10, 0, 0, 0, 0, 0, 'Jojo Ironbrow - Set Data'),
(5769200, 9, 7, 0, 0, 0, 100, 0, 2000, 2000, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Jojo Ironbrow - Talk'),
(5769200, 9, 8, 0, 0, 0, 100, 0, 6000, 6000, 0, 0, 0, 1, 3, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Jojo Ironbrow - Talk'),
(5769200, 9, 9, 0, 0, 0, 100, 0, 2000, 2000, 0, 0, 0, 136, 1, 0, 7, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Jojo Ironbrow - Set Movement Speed'),
(5769200, 9, 10, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 69, 2, 0, 0, 0, 0, 0, 8, 0, 0, 0, 1078.09, 4177.64, 205.7422, 0, 'Jojo Ironbrow - Move to Pos'),
(57692, 0, 1, 2, 34, 0, 100, 0, 8, 2, 0, 0, 0, 128, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Jojo Ironbrow - Movement Inform - Set AI AnimKit'),
(57692, 0, 2, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 90, 3, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Jojo Ironbrow - Movement Inform - Set Bytes1'),
-- Tiger Pillar Stand
(57690, 0, 0, 0, 54, 0, 100, 0, 0, 0, 0, 0, 0, 66, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 5.550147, 'Tiger Pillar Stand - Just Spawned - Set Orientation'),
-- Fe-Feng Firethief
(57466, 0, 0, 0, 0, 0, 100, 0, 4000, 8000, 15000, 20000, 0, 11, 109098, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update IC - Cast Set Ablaze'),
(-451518, 0, 0, 0, 1, 0, 100, 0, 0, 3000, 1000, 4000, 0, 5, 35, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update OOC - Play Emote'),
(-451518, 0, 1, 0, 0, 0, 100, 0, 4000, 8000, 15000, 20000, 0, 11, 109098, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update IC - Cast Set Ablaze'),
(-451570, 0, 0, 0, 1, 0, 100, 0, 0, 3000, 1000, 4000, 0, 5, 35, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update OOC - Play Emote'),
(-451570, 0, 1, 0, 0, 0, 100, 0, 4000, 8000, 15000, 20000, 0, 11, 109098, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update IC - Cast Set Ablaze'),
(-451564, 0, 0, 0, 1, 0, 100, 0, 0, 5000, 4000, 13000, 0, 10, 1, 6, 25, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update OOC - Play Random Emote'),
(-451564, 0, 1, 0, 0, 0, 100, 0, 4000, 8000, 15000, 20000, 0, 11, 109098, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update IC - Cast Set Ablaze'),
(-451565, 0, 0, 0, 1, 0, 100, 0, 0, 5000, 4000, 13000, 0, 10, 1, 6, 25, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update OOC - Play Random Emote'),
(-451565, 0, 1, 0, 0, 0, 100, 0, 4000, 8000, 15000, 20000, 0, 11, 109098, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update IC - Cast Set Ablaze'),
(-451572, 0, 0, 0, 1, 0, 100, 0, 0, 3000, 1000, 4000, 0, 5, 35, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update OOC - Play Emote'),
(-451572, 0, 1, 0, 0, 0, 100, 0, 4000, 8000, 15000, 20000, 0, 11, 109098, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update IC - Cast Set Ablaze'),
(-451542, 0, 0, 0, 1, 0, 100, 0, 0, 5000, 4000, 13000, 0, 10, 1, 6, 25, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update OOC - Play Random Emote'),
(-451542, 0, 1, 0, 0, 0, 100, 0, 4000, 8000, 15000, 20000, 0, 11, 109098, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update IC - Cast Set Ablaze'),
-- Fe-Feng Firethief 55633
(55633, 0, 0, 0, 0, 0, 100, 0, 4000, 8000, 8000, 12000, 0, 11, 127940, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update IC - Cast Shoot'),
(55633, 0, 1, 0, 0, 0, 100, 0, 10000, 15000, 10000, 15000, 0, 11, 109104, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update IC - Cast Shoot ALL the fireworks!'),
(-451539, 0, 0, 0, 1, 0, 100, 0, 0, 5000, 4000, 13000, 0, 10, 1, 6, 25, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update OOC - Play Random Emote'),
(-451539, 0, 1, 0, 0, 0, 100, 0, 4000, 8000, 8000, 12000, 0, 11, 127940, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update IC - Cast Shoot'),
(-451539, 0, 2, 0, 0, 0, 100, 0, 10000, 15000, 10000, 15000, 0, 11, 109104, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update IC - Cast Shoot ALL the fireworks!'),
(-451532, 0, 0, 0, 1, 0, 100, 0, 0, 2500, 2500, 3500, 0, 5, 35, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update OOC - Play Emote'),
(-451532, 0, 1, 0, 0, 0, 100, 0, 4000, 8000, 8000, 12000, 0, 11, 127940, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update IC - Cast Shoot'),
(-451532, 0, 2, 0, 0, 0, 100, 0, 10000, 15000, 10000, 15000, 0, 11, 109104, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update IC - Cast Shoot ALL the fireworks!'),
(-451563, 0, 0, 0, 1, 0, 100, 0, 0, 2500, 2500, 3500, 0, 5, 35, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update OOC - Play Emote'),
(-451563, 0, 1, 0, 0, 0, 100, 0, 4000, 8000, 8000, 12000, 0, 11, 127940, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update IC - Cast Shoot'),
(-451563, 0, 2, 0, 0, 0, 100, 0, 10000, 15000, 10000, 15000, 0, 11, 109104, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update IC - Cast Shoot ALL the fireworks!'),
(-451549, 0, 0, 0, 1, 0, 100, 0, 0, 5000, 4000, 13000, 0, 10, 1, 6, 25, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update OOC - Play Random Emote'),
(-451549, 0, 1, 0, 0, 0, 100, 0, 4000, 8000, 8000, 12000, 0, 11, 127940, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update IC - Cast Shoot'),
(-451549, 0, 2, 0, 0, 0, 100, 0, 10000, 15000, 10000, 15000, 0, 11, 109104, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update IC - Cast Shoot ALL the fireworks!'),
(-451552, 0, 0, 0, 1, 0, 100, 0, 0, 5000, 4000, 13000, 0, 10, 1, 6, 25, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update OOC - Play Random Emote'),
(-451552, 0, 1, 0, 0, 0, 100, 0, 4000, 8000, 8000, 12000, 0, 11, 127940, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update IC - Cast Shoot'),
(-451552, 0, 2, 0, 0, 0, 100, 0, 10000, 15000, 10000, 15000, 0, 11, 109104, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Firethief - Update IC - Cast Shoot ALL the fireworks!'),
-- Huojin Monk / Ji Firepaw
(65558, 0, 0, 0, 54, 0, 100, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - Just Spawned - Talk'),
(65558, 0, 1, 0, 4, 0, 100, 0, 0, 0, 0, 0, 0, 11, 117312, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - On Aggro - Cast Combat Roll'),
(65558, 0, 2, 0, 0, 0, 100, 0, 3000, 5000, 6000, 6000, 0, 11, 128631, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - Update IC - Cast Blackout Kick'),
(65558, 0, 3, 0, 0, 0, 100, 0, 3000, 5000, 3000, 5000, 0, 11, 128630, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - Update IC - Cast Jab'),
(65558, 0, 4, 0, 0, 0, 100, 0, 5000, 8000, 10000, 15000, 0, 11, 128635, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - Update IC - Cast Fists of Fury'),
(65558, 0, 5, 0, 0, 0, 100, 0, 5000, 8000, 10000, 15000, 0, 11, 128632, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - Update IC - Cast Spinning Crane Kick'),
(65558, 0, 6, 0, 0, 0, 100, 0, 10000, 10000, 10000, 10000, 0, 11, 128643, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - Update IC - Cast Healing Sphere'),
(65558, 0, 7, 0, 6, 0, 100, 0, 0, 0, 0, 0, 0, 86, 105306, 2, 23, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - On Death - Cross Cast Summon Ji Yuan'),
(65558, 0, 8, 0, 5, 0, 40, 0, 5000, 5000, 0, 55632, 0, 1, 1, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - On Kill - Talk'),
(65558, 0, 9, 0, 5, 0, 40, 0, 5000, 5000, 0, 57466, 0, 1, 1, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - On Kill - Talk'),
(65558, 0, 10, 11, 8, 0, 100, 0, 116992, 0, 0, 0, 0, 29, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - On Spellhit - Stop Follow'),
(65558, 0, 11, 12, 61, 0, 100, 0, 0, 0, 0, 0, 0, 1, 6, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - On Spellhit - Talk'),
(65558, 0, 12, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 80, 6555800, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - On Spellhit - Run Script'),
(6555800, 9, 0, 0, 0, 0, 100, 0, 3000, 3000, 0, 0, 0, 59, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - Set Run'),
(6555800, 9, 1, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - Set React State'),
(6555800, 9, 2, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 69, 1, 0, 0, 0, 0, 0, 8, 0, 0, 0, 1163.807, 4334.277, 210.9638, 0, 'Ji Firepaw - Move to Pos'),
(65558, 0, 13, 0, 34, 0, 100, 0, 8, 1, 0, 0, 0, 41, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - On Movement Inform - Despawn'),
-- Fe-Feng Ruffian
(55632, 0, 0, 0, 0, 0, 100, 0, 4000, 8000, 8000, 12000, 0, 80, 5563200, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update IC - Run Script'),
(5563200, 9, 0, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 11, 128414, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Cast Display of Fury'),
(-451569, 0, 0, 0, 1, 0, 100, 0, 0, 5000, 5000, 8000, 0, 5, 407, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update OOC - Play Emote'),
(-451569, 0, 1, 0, 0, 0, 100, 0, 4000, 8000, 8000, 12000, 0, 80, 5563200, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update IC - Run Script'),
(-451520, 0, 0, 0, 1, 0, 100, 0, 0, 5000, 5000, 8000, 0, 5, 407, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update OOC - Play Emote'),
(-451520, 0, 1, 0, 0, 0, 100, 0, 4000, 8000, 8000, 12000, 0, 80, 5563200, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update IC - Run Script'),
(-451526, 0, 0, 0, 1, 0, 100, 0, 0, 5000, 4000, 13000, 0, 10, 1, 6, 25, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update OOC - Play Random Emote'),
(-451526, 0, 1, 0, 0, 0, 100, 0, 4000, 8000, 8000, 12000, 0, 80, 5563200, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update IC - Run Script'),
(-451525, 0, 0, 0, 1, 0, 100, 0, 0, 5000, 4000, 13000, 0, 10, 1, 6, 25, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update OOC - Play Random Emote'),
(-451525, 0, 1, 0, 0, 0, 100, 0, 4000, 8000, 8000, 12000, 0, 80, 5563200, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update IC - Run Script'),
(-451533, 0, 0, 0, 1, 0, 100, 0, 0, 5000, 4000, 13000, 0, 10, 1, 6, 25, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update OOC - Play Random Emote'),
(-451533, 0, 1, 0, 0, 0, 100, 0, 4000, 8000, 8000, 12000, 0, 80, 5563200, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update IC - Run Script'),
(-451537, 0, 0, 0, 1, 0, 100, 0, 0, 5000, 4000, 13000, 0, 10, 1, 6, 25, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update OOC - Play Random Emote'),
(-451537, 0, 1, 0, 0, 0, 100, 0, 4000, 8000, 8000, 12000, 0, 80, 5563200, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update IC - Run Script'),
(-451547, 0, 0, 0, 1, 0, 100, 0, 0, 5000, 4000, 13000, 0, 10, 1, 6, 25, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update OOC - Play Random Emote'),
(-451547, 0, 1, 0, 0, 0, 100, 0, 4000, 8000, 8000, 12000, 0, 80, 5563200, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update IC - Run Script'),
(-451543, 0, 0, 0, 1, 0, 100, 0, 0, 5000, 4000, 13000, 0, 10, 1, 6, 25, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update OOC - Play Random Emote'),
(-451543, 0, 1, 0, 0, 0, 100, 0, 4000, 8000, 8000, 12000, 0, 80, 5563200, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update IC - Run Script'),
(-451541, 0, 0, 0, 1, 0, 100, 0, 0, 5000, 4000, 13000, 0, 10, 1, 6, 25, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update OOC - Play Random Emote'),
(-451541, 0, 1, 0, 0, 0, 100, 0, 4000, 8000, 8000, 12000, 0, 80, 5563200, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update IC - Run Script'),
(-451529, 0, 0, 0, 1, 0, 100, 0, 0, 5000, 4000, 13000, 0, 10, 1, 6, 25, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update OOC - Play Random Emote'),
(-451529, 0, 1, 0, 0, 0, 100, 0, 4000, 8000, 8000, 12000, 0, 80, 5563200, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update IC - Run Script'),
-- Fe-Feng Ruffian (hanging)
(57465, 0, 0, 0, 11, 0, 100, 0, 0, 0, 0, 0, 0, 8, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - On Respawn - Set React State'),
(57465, 0, 1, 0, 4, 0, 100, 0, 0, 0, 0, 0, 0, 41, 0, 0, 0, 0, 0, 0, 11, 57464, 1, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - On Aggro - Despawn Vehicle'),
(57465, 0, 2, 0, 0, 0, 100, 0, 4000, 8000, 8000, 12000, 0, 80, 5563200, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - Update IC - Run Script'),
(57465, 0, 3, 0, 7, 0, 100, 0, 0, 0, 0, 0, 0, 41, 2000, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Ruffian - On Evade - Despawn'),
-- Fe-Feng Wiseman
(-451429, 0, 0, 0, 1, 0, 100, 0, 0, 0, 23000, 23000, 0, 80, 5560100, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update OOC - Run Script'),
(5560100, 9, 0, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 97, 15, 15, 0, 0, 0, 0, 1, 0, 0, 0, 840.253, 4096.59, 209.464, 0, 'Fe-Feng Wiseman - Jump to Pos'),
(5560100, 9, 1, 0, 0, 0, 100, 0, 6000, 6000, 0, 0, 0, 97, 15, 15, 0, 0, 0, 0, 1, 0, 0, 0, 854.115, 4084.21, 205.568, 0, 'Fe-Feng Wiseman - Jump to Pos'),
(5560100, 9, 2, 0, 0, 0, 100, 0, 6000, 6000, 0, 0, 0, 97, 15, 15, 0, 0, 0, 0, 1, 0, 0, 0, 841.644, 4096.09, 209.709, 0, 'Fe-Feng Wiseman - Jump to Pos'),
(5560100, 9, 3, 0, 0, 0, 100, 0, 6000, 6000, 0, 0, 0, 97, 15, 15, 0, 0, 0, 0, 1, 0, 0, 0, 823.063, 4101.08, 212.425, 0, 'Fe-Feng Wiseman - Jump to Pos'),
(-451429, 0, 1, 0, 0, 0, 100, 0, 3000, 6000, 18000, 22000, 0, 11, 128751, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Paint it Red!'),
(-451429, 0, 2, 0, 0, 0, 100, 0, 2000, 4000, 8000, 12000, 0, 11, 128413, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Hozen Shock'),
(-451502, 0, 0, 0, 1, 0, 100, 0, 0, 0, 22000, 22000, 0, 80, 5560101, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update OOC - Run Script'),
(5560101, 9, 0, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 97, 15, 15, 0, 0, 0, 0, 1, 0, 0, 0, 870.788, 4128.51, 204.465, 0, 'Fe-Feng Wiseman - Jump to Pos'),
(5560101, 9, 1, 0, 0, 0, 100, 0, 5000, 5000, 0, 0, 0, 97, 15, 15, 0, 0, 0, 0, 1, 0, 0, 0, 885.913, 4136.3, 209.251, 0, 'Fe-Feng Wiseman - Jump to Pos'),
(5560101, 9, 2, 0, 0, 0, 100, 0, 6000, 6000, 0, 0, 0, 97, 15, 15, 0, 0, 0, 0, 1, 0, 0, 0, 871.609, 4129.59, 204.369, 0, 'Fe-Feng Wiseman - Jump to Pos'),
(5560101, 9, 3, 0, 0, 0, 100, 0, 5000, 5000, 0, 0, 0, 97, 15, 15, 0, 0, 0, 0, 1, 0, 0, 0, 856.233, 4122.32, 198.307, 0, 'Fe-Feng Wiseman - Jump to Pos'),
(-451502, 0, 1, 0, 0, 0, 100, 0, 3000, 6000, 18000, 22000, 0, 11, 128751, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Paint it Red!'),
(-451502, 0, 2, 0, 0, 0, 100, 0, 2000, 4000, 8000, 12000, 0, 11, 128413, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Hozen Shock'),
(-451484, 0, 0, 0, 1, 0, 100, 0, 0, 0, 15000, 15000, 0, 80, 5560102, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update OOC - Run Script'),
(5560102, 9, 0, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 97, 15, 15, 0, 0, 0, 0, 1, 0, 0, 0, 885.802, 4214.34, 204.627, 0, 'Fe-Feng Wiseman - Jump to Pos'),
(5560102, 9, 1, 0, 0, 0, 100, 0, 3500, 3500, 0, 0, 0, 97, 15, 15, 0, 0, 0, 0, 1, 0, 0, 0, 881.219, 4222.63, 213.961, 0, 'Fe-Feng Wiseman - Jump to Pos'),
(5560102, 9, 2, 0, 0, 0, 100, 0, 5000, 5000, 0, 0, 0, 97, 15, 15, 0, 0, 0, 0, 1, 0, 0, 0, 887.045, 4215.36, 204.606, 0, 'Fe-Feng Wiseman - Jump to Pos'),
(5560102, 9, 3, 0, 0, 0, 100, 0, 3500, 3500, 0, 0, 0, 97, 15, 15, 0, 0, 0, 0, 1, 0, 0, 0, 877.462, 4209.25, 200.89, 0, 'Fe-Feng Wiseman - Jump to Pos'),
(-451484, 0, 1, 0, 0, 0, 100, 0, 3000, 6000, 18000, 22000, 0, 11, 128751, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Paint it Red!'),
(-451484, 0, 2, 0, 0, 0, 100, 0, 2000, 4000, 8000, 12000, 0, 11, 128413, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Hozen Shock'),
(-451503, 0, 0, 0, 1, 0, 100, 0, 0, 3000, 6000, 9000, 0, 11, 128753, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update OOC - Cast Paint it Red!'),
(-451503, 0, 1, 0, 0, 0, 100, 0, 3000, 6000, 18000, 22000, 0, 11, 128751, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Paint it Red!'),
(-451503, 0, 2, 0, 0, 0, 100, 0, 2000, 4000, 8000, 12000, 0, 11, 128413, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Hozen Shock'),
(-451507, 0, 0, 0, 1, 0, 100, 0, 0, 3000, 6000, 9000, 0, 11, 128753, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update OOC - Cast Paint it Red!'),
(-451507, 0, 1, 0, 0, 0, 100, 0, 3000, 6000, 18000, 22000, 0, 11, 128751, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Paint it Red!'),
(-451507, 0, 2, 0, 0, 0, 100, 0, 2000, 4000, 8000, 12000, 0, 11, 128413, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Hozen Shock'),
(-451595, 0, 0, 0, 1, 0, 100, 0, 0, 3000, 6000, 9000, 0, 11, 128753, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update OOC - Cast Paint it Red!'),
(-451595, 0, 1, 0, 0, 0, 100, 0, 3000, 6000, 18000, 22000, 0, 11, 128751, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Paint it Red!'),
(-451595, 0, 2, 0, 0, 0, 100, 0, 2000, 4000, 8000, 12000, 0, 11, 128413, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Hozen Shock'),
(-451496, 0, 0, 0, 1, 0, 100, 0, 0, 2000, 3000, 8000, 0, 10, 1, 6, 25, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update OOC - Play Random Emote'),
(-451496, 0, 1, 0, 0, 0, 100, 0, 3000, 6000, 18000, 22000, 0, 11, 128751, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Paint it Red!'),
(-451496, 0, 2, 0, 0, 0, 100, 0, 2000, 4000, 8000, 12000, 0, 11, 128413, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Hozen Shock'),
(-451594, 0, 0, 0, 1, 0, 100, 0, 0, 3000, 6000, 9000, 0, 11, 128753, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update OOC - Cast Paint it Red!'),
(-451594, 0, 1, 0, 0, 0, 100, 0, 3000, 6000, 18000, 22000, 0, 11, 128751, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Paint it Red!'),
(-451594, 0, 2, 0, 0, 0, 100, 0, 2000, 4000, 8000, 12000, 0, 11, 128413, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Hozen Shock'),
(-451513, 0, 0, 0, 1, 0, 100, 0, 0, 3000, 6000, 9000, 0, 11, 128753, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update OOC - Cast Paint it Red!'),
(-451513, 0, 1, 0, 0, 0, 100, 0, 3000, 6000, 18000, 22000, 0, 11, 128751, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Paint it Red!'),
(-451513, 0, 2, 0, 0, 0, 100, 0, 2000, 4000, 8000, 12000, 0, 11, 128413, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Hozen Shock'),
(-451435, 0, 0, 0, 1, 0, 100, 0, 0, 2000, 3000, 8000, 0, 10, 1, 6, 25, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update OOC - Play Random Emote'),
(-451435, 0, 1, 0, 0, 0, 100, 0, 3000, 6000, 18000, 22000, 0, 11, 128751, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Paint it Red!'),
(-451435, 0, 2, 0, 0, 0, 100, 0, 2000, 4000, 8000, 12000, 0, 11, 128413, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Hozen Shock'),
(-451592, 0, 0, 0, 1, 0, 100, 0, 2000, 4000, 3000, 8000, 0, 10, 1, 6, 25, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update OOC - Play Random Emote'),
(-451592, 0, 1, 0, 0, 0, 100, 0, 3000, 6000, 18000, 22000, 0, 11, 128751, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Paint it Red!'),
(-451592, 0, 2, 0, 0, 0, 100, 0, 2000, 4000, 8000, 12000, 0, 11, 128413, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Hozen Shock'),
(-451452, 0, 0, 0, 1, 0, 100, 0, 2000, 4000, 3000, 8000, 0, 10, 1, 6, 25, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update OOC - Play Random Emote'),
(-451452, 0, 1, 0, 0, 0, 100, 0, 3000, 6000, 18000, 22000, 0, 11, 128751, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Paint it Red!'),
(-451452, 0, 2, 0, 0, 0, 100, 0, 2000, 4000, 8000, 12000, 0, 11, 128413, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Hozen Shock'),
(-451449, 0, 0, 0, 1, 0, 100, 0, 0, 2000, 3000, 8000, 0, 10, 1, 6, 25, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update OOC - Play Random Emote'),
(-451449, 0, 1, 0, 0, 0, 100, 0, 3000, 6000, 18000, 22000, 0, 11, 128751, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Paint it Red!'),
(-451449, 0, 2, 0, 0, 0, 100, 0, 2000, 4000, 8000, 12000, 0, 11, 128413, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Hozen Shock'),
(-451504, 0, 0, 0, 1, 0, 100, 0, 0, 3000, 6000, 9000, 0, 11, 128753, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update OOC - Cast Paint it Red!'),
(-451504, 0, 1, 0, 0, 0, 100, 0, 3000, 6000, 18000, 22000, 0, 11, 128751, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Paint it Red!'),
(-451504, 0, 2, 0, 0, 0, 100, 0, 2000, 4000, 8000, 12000, 0, 11, 128413, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Hozen Shock'),
(-451497, 0, 0, 0, 1, 0, 100, 0, 2000, 4000, 3000, 8000, 0, 10, 1, 6, 25, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update OOC - Play Random Emote'),
(-451497, 0, 1, 0, 0, 0, 100, 0, 3000, 6000, 18000, 22000, 0, 11, 128751, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Paint it Red!'),
(-451497, 0, 2, 0, 0, 0, 100, 0, 2000, 4000, 8000, 12000, 0, 11, 128413, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Hozen Shock'),
(-451437, 0, 0, 0, 1, 0, 100, 0, 0, 3000, 6000, 9000, 0, 11, 128753, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update OOC - Cast Paint it Red!'),
(-451437, 0, 1, 0, 0, 0, 100, 0, 3000, 6000, 18000, 22000, 0, 11, 128751, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Paint it Red!'),
(-451437, 0, 2, 0, 0, 0, 100, 0, 2000, 4000, 8000, 12000, 0, 11, 128413, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Hozen Shock'),
(55601, 0, 0, 0, 0, 0, 100, 0, 3000, 6000, 18000, 22000, 0, 11, 128751, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Paint it Red!'),
(55601, 0, 1, 0, 0, 0, 100, 0, 2000, 4000, 8000, 12000, 0, 11, 128413, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Fe-Feng Wiseman - Update IC - Cast Hozen Shock');

DELETE FROM `npc_vendor` WHERE `entry` IN (57623);
INSERT INTO `npc_vendor` (entry, slot, item, maxcount, incrtime, ExtendedCost, type, BonusListIDs, PlayerConditionID, IgnoreFiltering, VerifiedBuild) VALUES
('57623', '1', '2880', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('57623', '2', '2901', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('57623', '3', '3466', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('57623', '4', '3857', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('57623', '5', '5956', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('57623', '6', '18567', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('57623', '7', '160298', '0', '0', '0', '1', NULL, '0', '0', '63906'),
('57623', '8', '180733', '0', '0', '0', '1', NULL, '0', '0', '63906');

UPDATE `creature_template` SET `ScriptName` = 'npc_ruk_ruk' WHERE `Entry` = 55634;
UPDATE `creature_template` SET `ScriptName` = 'npc_ruk_ruk_rocket' WHERE `Entry` = 64322;

UPDATE `creature` SET `PhaseId` = 524 WHERE `guid` IN (451612,451613,451615,451614,451616,451617,451618,451619,451620);
UPDATE `creature` SET `PhaseId` = 536 WHERE `guid` IN (451621, 451622, 451623);
UPDATE `creature` SET `PhaseId` = 1526 WHERE `guid` IN (451624); -- Master Shang at Zhaoren battle (quest starter)
UPDATE `creature` SET `PhaseId` = 1836 WHERE `guid` IN (451498,451501); -- Chamber of Whispers winds ID 55665
UPDATE `creature` SET `PhaseId` = 1527 WHERE `guid` = 451650;
UPDATE `creature` SET `PhaseId` = 1430 WHERE `guid` = 451478; -- Zhao-Ren above Chamber of Whispers
UPDATE `creature` SET `PhaseId` = 1429, `StringId` = 'npc_zhaoren_flyby' WHERE `guid` = 451413; -- Zhao-Ren Fly-by
UPDATE `creature` SET `PhaseGroup` = 638, `PhaseId` = 0 WHERE `id` = 55601;

UPDATE `gameobject` SET `PhaseId` = 1836 WHERE `guid` IN (300567, 300568); -- Chamber of whispers winds
UPDATE `gameobject` SET `PhaseId` = 1714 WHERE `id` IN (209673);

UPDATE `creature_template` SET `ScriptName` = 'npc_zhaoren' WHERE `Entry` = 55786;

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (55592,55744,55595,64543,64532,55665);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55592,55744,5574400,55595,5559500,64543,6454300,64532,6453200,5566500,5566501,-451498,-451501) AND `source_type` IN (0,9);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (7037,7041,7042) AND `source_type` IN (2);
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
-- Trigger 7041 Chamber of Winds entrance
-- The spell summon makes the npc not targetable by other npcs. We spawn the npc directly
-- (7041, 2, 0, 0, 46, 0, 100, 0, 7041, 0, 0, 0, 0, 86, 104593, 2, 10, 451498, 55665, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'On Trigger - Cross Cast Forcecast Summon Aysa, Wind Chamber'),
(7041, 2, 0, 0, 46, 0, 100, 0, 0, 0, 0, 0, 0, 12, 55744, 8, 0, 0, 2, 0, 8, 0, 0, 0, 666.514, 4218.27, 200.851, 0, 'On Trigger - Summon Aysa, Wind Chamber'),
-- Trigger 7042 Chamber of Winds middle
(7042, 2, 0, 0, 46, 0, 100, 0, 0, 0, 0, 0, 0, 86, 104614, 2, 10, 451501, 55665, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'On Trigger - Cross Cast Forcecast Trigger Aysa Wind Chamber Middle'),
-- Trigger 7037 Chamber of Winds rear
(7037, 2, 0, 0, 46, 0, 100, 0, 0, 0, 0, 0, 0, 86, 104616, 2, 10, 451599, 55592, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'On Trigger - Cross Cast Forcecast Trigger Aysa Wind Chamber Rear'),

-- Frightened Winds
(-451498, 0, 0, 0, 11, 0, 100, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Frightened Winds - On Respawn - Set React State'),
(-451498, 0, 1, 2, 60, 0, 100, 0, 11000, 11000, 22000, 22000, 0, 28, 104333, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Frightened Winds - Update - Remove Aura'),
(-451498, 0, 2, 3, 61, 0, 100, 0, 0, 0, 0, 0, 0, 11, 105678, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Frightened Winds - Update - Cast Frightened Winds Offtime Aura'),
(-451498, 0, 3, 4, 61, 0, 100, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 20, 209685, 1, 0, 0, 0, 0, 0, 'Frightened Winds - Update - Activate GO'),
(-451498, 0, 4, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 80, 5566500, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Frightened Winds - Update - Run Script'),
(5566500, 9, 0, 0, 0, 0, 100, 0, 1000, 1000, 0, 0, 0, 45, 1, 1, 0, 0, 0, 0, 11, 55744, 40, 0, 0, 0, 0, 0, 'Frightened Winds - Set Data'),
(-451498, 0, 5, 6, 60, 0, 100, 0, 22000, 22000, 22000, 22000, 0, 28, 105678, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Frightened Winds - Update - Remove Aura'),
(-451498, 0, 6, 7, 61, 0, 100, 0, 0, 0, 0, 0, 0, 11, 104333, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Frightened Winds - Update - Cast Frightened Winds Aura'),
(-451498, 0, 7, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 20, 209685, 1, 0, 0, 0, 0, 0, 'Frightened Winds - Update - Reset GO'),
(-451501, 0, 0, 0, 11, 0, 100, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Frightened Winds - On Respawn - Set React State'),
(-451501, 0, 1, 2, 60, 0, 100, 0, 11000, 11000, 22000, 22000, 0, 28, 104333, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Frightened Winds - Update - Remove Aura'),
(-451501, 0, 2, 3, 61, 0, 100, 0, 0, 0, 0, 0, 0, 11, 105678, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Frightened Winds - Update - Cast Frightened Winds Offtime Aura'),
(-451501, 0, 3, 4, 61, 0, 100, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 20, 209685, 1, 0, 0, 0, 0, 0, 'Frightened Winds - Update - Activate GO'),
(-451501, 0, 4, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 80, 5566501, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Frightened Winds - Update - Run Script'),
(5566501, 9, 0, 0, 0, 0, 100, 0, 1000, 1000, 0, 0, 0, 45, 2, 2, 0, 0, 0, 0, 11, 55744, 50, 0, 0, 0, 0, 0, 'Frightened Winds - Set Data'),
(-451501, 0, 5, 6, 60, 0, 100, 0, 22000, 22000, 22000, 22000, 0, 28, 105678, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Frightened Winds - Update - Remove Aura'),
(-451501, 0, 6, 7, 61, 0, 100, 0, 0, 0, 0, 0, 0, 11, 104333, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Frightened Winds - Update - Cast Frightened Winds Aura'),
(-451501, 0, 7, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 20, 209685, 1, 0, 0, 0, 0, 0, 'Frightened Winds - Update - Reset GO'),

-- Dafeng
(55592, 0, 0, 0, 31, 0, 100, 0, 104616, 0, 0, 0, 0, 33, 55666, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Dafeng - On Spellhit Target - Quest Credit'),
-- Aysa Cloudsinger
(55744, 0, 0, 1, 54, 0, 100, 0, 0, 0, 0, 0, 0, 69, 1, 0, 0, 0, 0, 0, 8, 0, 0, 0, 653.1528, 4224.64, 202.9091, 0, 'Aysa Cloudsinger - Just Spawned - Move to Pos'),
(55744, 0, 1, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 22, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - Just Spawned - Set Event Phase 1'),
(55744, 0, 2, 0, 34, 1, 100, 0, 8, 1, 0, 0, 0, 1, 0, 2000, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - Movement Inform - Talk'),
(55744, 0, 3, 0, 52, 1, 100, 0, 0, 55744, 0, 0, 0, 1, 1, 4000, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Text Over - Talk'),
(55744, 0, 4, 0, 52, 1, 100, 0, 1, 55744, 0, 0, 0, 69, 2, 0, 0, 0, 0, 0, 8, 0, 0, 0, 647.493, 4224.63, 202.9091, 0, 'Aysa Cloudsinger - On Text Over - Move to Pos'),
(55744, 0, 5, 6, 34, 1, 100, 0, 8, 2, 0, 0, 0, 80, 5574400, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - Movement Inform - Run Script'),
(5574400, 9, 0, 0, 0, 0, 100, 0, 250, 250, 0, 0, 0, 66, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 2.426008, 'Aysa Cloudsinger - Movement Inform - Set Orientation'),
(55744, 0, 6, 0, 61, 1, 100, 0, 0, 0, 0, 0, 0, 22, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - Movement Inform - Set Event Phase 2'),
(55744, 0, 7, 8, 38, 2, 100, 0, 1, 1, 0, 0, 0, 53, 1, 5574400, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Data Set - Start WP'),
(55744, 0, 8, 0, 61, 2, 100, 0, 0, 0, 0, 0, 0, 22, 3, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Data Set - Set Event Phase 3'),
(55744, 0, 9, 10, 8, 4, 100, 0, 104612, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Spellhit - Talk'),
(55744, 0, 10, 0, 61, 4, 100, 0, 0, 0, 0, 0, 0, 22, 4, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Data Set - Set Event Phase 4'),
(55744, 0, 11, 12, 38, 8, 100, 0, 2, 2, 0, 0, 0, 53, 1, 5574401, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Data Set - Start WP'),
(55744, 0, 12, 0, 61, 8, 100, 0, 0, 0, 0, 0, 0, 22, 5, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Data Set - Set Event Phase 5'),
(55744, 0, 13, 14, 58, 16, 100, 0, 5, 5574401, 0, 0, 0, 11, 104748, 2, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On WP Ended - Cast Forcecast to Master for Aysa See Invis'),
(55744, 0, 14, 15, 61, 16, 100, 0, 0, 0, 0, 0, 0, 28, 104571, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On WP Ended - Remove Aura'),
(55744, 0, 15, 0, 61, 16, 100, 0, 0, 0, 0, 0, 0, 41, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On WP Ended - Despawn'),
-- Aysa Cloudsinger
(55595, 0, 0, 0, 8, 0, 100, 0, 104615, 0, 10000, 10000, 0, 80, 5559500, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Spellhit - Run Script'),
(5559500, 9, 0, 0, 0, 0, 100, 0, 2000, 2000, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - Talk'),
(55595, 0, 1, 2, 19, 0, 100, 0, 29786, 0, 0, 0, 0, 85, 126059, 2, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Accepted Quest - Invoker Cast Summon Aysa'),
(55595, 0, 2, 3, 61, 0, 100, 0, 0, 0, 0, 0, 0, 44, 1430, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Accepted Quest - Remove Phase'),
(55595, 0, 3, 4, 61, 0, 100, 0, 0, 0, 0, 0, 0, 28, 105307, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Accepted Quest - Player Remove Aura'),
(55595, 0, 4, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 28, 105308, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Accepted Quest - Player Remove Aura'),
-- Aysa Cloudsinger
(64543, 0, 0, 0, 54, 0, 100, 0, 0, 0, 0, 0, 0, 80, 6454300, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - Just Spawned - Run Script'),
(6454300, 9, 0, 0, 0, 0, 100, 0, 1000, 1000, 0, 0, 0, 136, 1, 12, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - Set Movement Speed'),
(6454300, 9, 1, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 53, 1, 6454300, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - Start WP'),
(64543, 0, 1, 0, 58, 0, 100, 0, 12, 6454300, 0, 0, 0, 97, 15, 20, 0, 0, 0, 0, 1, 0, 0, 0, 675.4445, 4204.716, 196.5799, 0, 'Aysa Cloudsinger - On WP Ended - Jump to Pos'),
(64543, 0, 2, 3, 34, 0, 100, 0, 16, 1004, 0, 0, 0, 28, 126059, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - Movement Inform - Remove Aura'),
(64543, 0, 3, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 41, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - Movement Inform - Despawn'),
-- Dafeng
(64532, 0, 1, 0, 54, 0, 100, 0, 0, 0, 0, 0, 0, 80, 6453200, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Dafeng - Just Spawned - Run Script'),
(6453200, 9, 0, 0, 0, 0, 100, 0, 5000, 5000, 0, 0, 0, 53, 1, 6453200, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Dafeng - Start WP'),
(64532, 0, 2, 0, 38, 0, 100, 0, 5, 5, 0, 0, 0, 69, 1, 0, 0, 0, 0, 0, 8, 0, 0, 0, 709.238, 4177.15, 198.7956, 0, 'Dafeng - On Data Set - Move to Pos');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (64505,64506,64507);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (64505,6450500,64506,64507) AND `source_type` IN (0,9);
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
-- Firework Launcher
(64507, 0, 0, 1, 8, 0, 100, 0, 125961, 0, 0, 0, 0, 11, 125970, 2, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Firework Launcher - On Spellhit - Cast AICast Overpacked Firework'),
(64507, 0, 1, 2, 61, 0, 100, 0, 0, 0, 0, 0, 0, 128, 2538, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Firework Launcher - On Spellhit - Play Oneshot AnimKit'),
(64507, 0, 2, 3, 61, 0, 100, 0, 0, 0, 0, 0, 0, 11, 125964, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Firework Launcher - On Spellhit - Cast Firework Launcher Inactive'),
(64507, 0, 3, 4, 61, 0, 100, 0, 0, 0, 0, 0, 0, 11, 104080, 0, 0, 0, 0, 0, 10, 451613, 64505, 0, 0, 0, 0, 0, 'Firework Launcher - On Spellhit - Set Data'),
(64507, 0, 4, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 22, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Firework Launcher - On Spellhit - Set Event Phase 1'),
(64507, 0, 5, 0, 60, 1, 100, 0, 5000, 5000, 5000, 5000, 0, 11, 104080, 0, 0, 0, 0, 0, 10, 451613, 64505, 0, 0, 0, 0, 0, 'Firework Launcher - Update - Cast Firework Launcher Inactive'),
(64507, 0, 6, 0, 38, 0, 100, 0, 1, 1, 0, 0, 0, 22, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Firework Launcher - On Data Set - Reset Event Phase'),
-- Ji Firepaw
(64505, 0, 0, 1, 25, 0, 100, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - On Reset - Set React State'),
(64505, 0, 1, 2, 61, 0, 100, 0, 0, 0, 0, 0, 0, 59, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - On Reset - Set Run'),
(64505, 0, 2, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 22, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - On Respawn - Set Event Phase 1'),
(64505, 0, 3, 4, 8, 1, 100, 0, 104080, 0, 0, 0, 0, 69, 1, 0, 0, 1, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - On Spellhit - Move to Pos'),
(64505, 0, 4, 0, 61, 1, 100, 0, 0, 0, 0, 0, 0, 22, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - On Spellhit - Set Event Phase 2'),
(64505, 0, 5, 0, 34, 2, 100, 0, 8, 1, 0, 0, 0, 80, 6450500, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - Movement Inform - Run Script'),
(6450500, 9, 0, 0, 0, 0, 100, 0, 200, 200, 0, 0, 0, 66, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - Set Orientation'),
(6450500, 9, 1, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 5, 381, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - Play Emote'),
(6450500, 9, 2, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 28, 125964, 0, 0, 0, 0, 0, 11, 64507, 3, 0, 0, 0, 0, 0, 'Ji Firepaw - Remove Aura'),
(6450500, 9, 3, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 45, 1, 1, 0, 0, 0, 0, 11, 64507, 3, 0, 0, 0, 0, 0, 'Ji Firepaw - Set Data'),
(6450500, 9, 4, 0, 0, 0, 100, 0, 1000, 1000, 0, 0, 0, 22, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - Set Event Phase 1'),
(64505, 0, 6, 7, 38, 0, 100, 0, 2, 2, 0, 0, 0, 8, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - On Data Set - Set React State'),
(64505, 0, 7, 8, 61, 0, 100, 0, 0, 0, 0, 0, 0, 49, 0, 0, 0, 0, 0, 0, 10, 451612, 55786, 0, 0, 0, 0, 0, 'Ji Firepaw - On Data Set - Start Attack'),
(64505, 0, 8, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 22, 3, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - On Data Set - Set Event Phase 3'),
(64505, 0, 9, 0, 9, 4, 100, 0, 10, 40, 0, 10000, 0, 11, 117312, 1, 0, 0, 0, 0, 10, 451612, 55786, 0, 0, 0, 0, 0, 'Ji Firepaw - On Range - Cast Combat Roll'),
(64505, 0, 10, 0, 0, 4, 100, 0, 3000, 5000, 3000, 5000, 0, 11, 128630, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - Update IC - Cast Jab'),
(64505, 0, 11, 0, 0, 4, 100, 0, 5000, 8000, 8000, 12000, 0, 11, 128631, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - Update IC - Cast Blackout Kick'),
(64505, 0, 12, 13, 38, 4, 100, 0, 4, 4, 0, 0, 0, 20, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - On Data Set - Stop Attack'),
(64505, 0, 13, 14, 61, 4, 100, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - On Data Set - Set React State'),
(64505, 0, 14, 15, 61, 4, 100, 0, 0, 0, 0, 0, 0, 69, 2, 0, 0, 1, 0, 0, 19, 64507, 40, 0, 0, 0, 0, 0, 'Ji Firepaw - On Data Set - Move to Pos'),
(64505, 0, 15, 0, 61, 4, 100, 0, 0, 0, 0, 0, 0, 22, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - On Data Set - Set Event Phase 1'),
(64505, 0, 16, 0, 38, 0, 100, 0, 5, 5, 0, 0, 0, 69, 3, 0, 0, 0, 0, 0, 8, 0, 0, 0, 714.385, 4163.74, 195.8898, 0, 'Ji Firepaw - On Data Set - Move to Pos'),
(64505, 0, 17, 18, 34, 0, 100, 0, 8, 3, 0, 0, 0, 66, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 6.015011, 'Ji Firepaw - Movement Inform - Set Orientation'),
(64505, 0, 18, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 22, 4, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - Movement Inform - Set Event Phase 4'),
(64505, 0, 19, 0, 38, 8, 100, 0, 6, 6, 0, 0, 0, 24, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Ji Firepaw - On Data Set - Evade'),
-- Aysa Cloudsinger
(64506, 0, 0, 1, 11, 0, 100, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Respawn - Set React State'),
(64506, 0, 1, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 22, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Respawn - Set Event Phase 1'),
(64506, 0, 2, 0, 38, 0, 100, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Data Set - Talk'),
(64506, 0, 3, 4, 38, 0, 100, 0, 2, 2, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Data Set - Talk'),
(64506, 0, 4, 5, 61, 0, 100, 0, 0, 0, 0, 0, 0, 91, 8, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Data Set - Remove Bytes 1'),
(64506, 0, 5, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 49, 0, 0, 0, 0, 0, 0, 10, 451612, 55786, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Data Set - Start Attack'),
(64506, 0, 6, 0, 38, 0, 100, 0, 3, 3, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Data Set - Talk'),
(64506, 0, 7, 8, 38, 0, 100, 0, 4, 4, 0, 0, 0, 20, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Data Set - Stop Attack'),
(64506, 0, 8, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 22, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Data Set - Set Event Phase 2'),
(64506, 0, 9, 0, 60, 2, 100, 0, 4000, 8000, 8000, 12000, 0, 11, 117312, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - Update - Cast Combat Roll'),
(64506, 0, 10, 0, 38, 0, 100, 0, 5, 5, 0, 0, 0, 24, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Aysa Cloudsinger - On Data Set - Evade');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (64530,56159,55586,56274,55640);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (64530,6453000,56159,5615900,55586,56274,55640) AND `source_type` IN (0,9);
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(55586, 0, 0, 0, 19, 0, 100, 0, 29787, 0, 0, 0, 0, 12, 56159, 8, 0, 0, 2, 0, 8, 0, 0, 0, 711.335, 4178.049, 197.8459, 0, 'On Quest Accept - Summon Master Shang - Quest 29787'),
-- Master Shang Xi spawned after Zhao battle ends
(64530, 0, 0, 1, 54, 0, 100, 0, 0, 0, 0, 0, 0, 59, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Just Spawned - Set Run'),
(64530, 0, 1, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 69, 1, 0, 0, 0, 0, 0, 8, 0, 0, 0, 711.335, 4178.05, 197.7852, 0, 'Master Shang - Just Spawned - Move to Pos'),
(64530, 0, 2, 0, 34, 0, 100, 0, 8, 1, 0, 0, 0, 80, 6453000, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Movement Inform - Run Script'),
(6453000, 9, 0, 0, 0, 0, 100, 0, 250, 250, 0, 0, 0, 66, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 5.410521, 'Master Shang - Set Orientation'),
(6453000, 9, 1, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 28, 126040, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Remove Aura'),
(6453000, 9, 2, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 44, 536, 1, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Add Phase'),
(6453000, 9, 3, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 44, 1526, 1, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Add Phase'),
(6453000, 9, 4, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 44, 524, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Remove Phase'),
(6453000, 9, 5, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 41, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Despawn'),

-- Master Shang Xi - spawned for escort quest
(56159, 0, 0, 0, 54, 0, 100, 0, 0, 0, 0, 0, 0, 80, 5615900, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Just Spawned - Run Script'),
(5615900, 9, 0, 0, 0, 0, 100, 0, 2000, 2000, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Talk'),
(5615900, 9, 1, 0, 0, 0, 100, 0, 3000, 3000, 0, 0, 0, 53, 1, 5615900, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Start WP'),
(5615900, 9, 2, 0, 0, 0, 100, 0, 2000, 2000, 0, 0, 0, 44, 169, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Add Phase'),
(56159, 0, 1, 2, 40, 0, 100, 0, 7, 5615900, 0, 0, 0, 59, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - On WP Reached - Set Walk'),
(56159, 0, 2, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - On WP Reached - Talk'),
(56159, 0, 3, 0, 40, 0, 100, 0, 8, 5615900, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - On WP Reached - Talk'),
(56159, 0, 4, 0, 40, 0, 100, 0, 11, 5615900, 0, 0, 0, 1, 3, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - On WP Reached - Talk'),
(56159, 0, 5, 0, 58, 0, 100, 0, 12, 5615900, 0, 0, 0, 66, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 5.480334, 'Master Shang - On WP Ended - Set Orientation'),
(56159, 0, 6, 7, 38, 0, 100, 0, 1, 1, 0, 0, 0, 55, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - On Data Set - Stop WP'),
(56159, 0, 7, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 1, 4, 2000, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - On Data Set - Talk'),
(56159, 0, 8, 0, 52, 0, 100, 0, 4, 56159, 0, 0, 0, 53, 1, 5615901, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - On Text Over - Start WP'),
(56159, 0, 9, 10, 58, 0, 100, 0, 6, 5615901, 0, 0, 0, 11, 109144, 2, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - On WP Ended - Cast Force Trigger Master Shang Xi Final Escort Say'),
(56159, 0, 10, 11, 61, 0, 100, 0, 0, 0, 0, 0, 0, 66, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 4.537856, 'Master Shang - On WP Ended - Set Orientation'),
(56159, 0, 11, 12, 61, 0, 100, 0, 0, 0, 0, 0, 0, 44, 1527, 1, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - On WP Ended - Add Phase'),
(56159, 0, 12, 13, 61, 0, 100, 0, 0, 0, 0, 0, 0, 28, 105333, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - On WP Ended - Remove Aura'),
(56159, 0, 13, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 41, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - On WP Ended - Despawn'),

-- Guardian of the Elders
(56274, 0, 0, 0, 0, 0, 100, 0, 8000, 12000, 8000, 12000, 0, 11, 125218, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Guardian of the Elders - Update IC - Cast Pounce'),
(56274, 0, 1, 0, 6, 0, 100, 0, 0, 0, 0, 0, 0, 45, 1, 1, 0, 0, 0, 0, 11, 56159, 50, 0, 0, 0, 0, 0, 'Guardian of the Elders - On Death - Set Data'),

-- Thornbranch Scamp
(55640, 0, 0, 0, 0, 0, 100, 0, 3000, 6000, 15000, 20000, 0, 11, 109126, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Thornbranch Scamp - Update IC - Cast Mirror Images');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (56686,57874);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (56686,5668600,5668601,5668602,5668603,57874);
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
-- Master Shang Xi
(56686, 0, 0, 0, 54, 0, 100, 0, 0, 0, 0, 0, 0, 80, 5668600, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Just Spawned - Run Script'),
(5668600, 9, 0, 0, 0, 0, 100, 0, 1000, 1000, 0, 0, 0, 4, 33097, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Play Sound'),
(5668600, 9, 1, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Talk'),
(5668600, 9, 2, 0, 0, 0, 100, 0, 6500, 6500, 0, 0, 0, 5, 396, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Play Emote'),
(5668600, 9, 3, 0, 0, 0, 100, 0, 5500, 5500, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Talk'),
(5668600, 9, 4, 0, 0, 0, 100, 0, 6500, 6500, 0, 0, 0, 136, 0, 1, 5, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Set Movement Speed'),
(5668600, 9, 5, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 53, 0, 5668600, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Start WP'),
(56686, 0, 1, 2, 40, 0, 100, 0, 0, 5668600, 0, 0, 0, 54, 10000, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - WP Reached - Pause WP'),
(56686, 0, 2, 3, 61, 0, 100, 0, 0, 0, 0, 0, 0, 66, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0.6990358, 'Master Shang - WP Reached - Set Orientation'),
(56686, 0, 3, 4, 61, 0, 100, 0, 0, 0, 0, 0, 0, 5, 396, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - WP Reached - Play Emote'),
(56686, 0, 4, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 80, 5668601, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - WP Reached - Run Script'),
(5668601, 9, 0, 0, 0, 0, 100, 0, 6000, 6000, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Talk'),
(56686, 0, 5, 6, 40, 0, 100, 0, 4, 5668600, 0, 0, 0, 54, 15000, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - WP Reached - Pause WP'),
(56686, 0, 6, 7, 61, 0, 100, 0, 0, 0, 0, 0, 0, 66, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 4.816799, 'Master Shang - WP Reached - Set Orientation'),
(56686, 0, 7, 8, 61, 0, 100, 0, 0, 0, 0, 0, 0, 4, 33098, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - WP Reached - Play Sound'),
(56686, 0, 8, 9, 61, 0, 100, 0, 0, 0, 0, 0, 0, 11, 56913, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - WP Reached - Cast CSA Dummy Effect Self'),
(56686, 0, 9, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 1, 3, 3500, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - WP Reached - Talk'),
(56686, 0, 10, 11, 52, 0, 100, 0, 3, 56686, 0, 0, 0, 28, 126160, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - On Text Over - Remove Aura'),
(56686, 0, 11, 12, 61, 0, 100, 0, 0, 0, 0, 0, 0, 11, 128850, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - On Text Over - Cast Forcecast Summon Walking Stick, Blossoming'),
(56686, 0, 12, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 80, 5668602, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - On Text Over - Run Script'),
(5668602, 9, 0, 0, 0, 0, 100, 0, 4000, 4000, 0, 0, 0, 5, 25, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Play Emote'),
(5668602, 9, 1, 0, 0, 0, 100, 0, 2500, 2500, 0, 0, 0, 5, 25, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Play Emote'),
(56686, 0, 13, 14, 58, 0, 100, 0, 5, 5668600, 0, 0, 0, 66, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 4.292658, 'Master Shang - WP Ended - Set Orientation'),
(56686, 0, 14, 15, 61, 0, 100, 0, 0, 0, 0, 0, 0, 1, 4, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - WP Ended - Talk'),
(56686, 0, 15, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 80, 5668603, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - WP Ended - Run Script'),
(5668603, 9, 0, 0, 0, 0, 100, 0, 8000, 8000, 0, 0, 0, 5, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Play Emote'),
(5668603, 9, 1, 0, 0, 0, 100, 0, 7000, 7000, 0, 0, 0, 1, 5, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Talk'),
(5668603, 9, 2, 0, 0, 0, 100, 0, 5500, 5500, 0, 0, 0, 90, 8, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Set Bytes1'),
(5668603, 9, 3, 0, 0, 0, 100, 0, 2500, 2500, 0, 0, 0, 11, 128851, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Cast Master Shang Spirit Transform'),
(5668603, 9, 4, 0, 0, 0, 100, 0, 1500, 1500, 0, 0, 0, 11, 109336, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Cast Trigger Walking Stick Blossom'),
(5668603, 9, 5, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 11, 106625, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Cast Planting Stave Credit'),
(5668603, 9, 6, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 44, 1885, 1, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Add Phase'),
(5668603, 9, 7, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 41, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Master Shang - Despawn'),
-- Walking Stick (temp hackfix - spawns shouldn't be visible to other units, TODO)
(57874, 0, 0, 0, 54, 0, 100, 0, 0, 0, 0, 0, 0, 18, 520, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Walking Stick - Just Spawned - Add Unit Flags');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 13 AND `SourceEntry` IN (104855,108845,108846,108857,116992,105333,106623,109335,109336);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `ConditionTypeOrReference`, `ConditionValue1`, `ConditionValue2`, `Comment`) VALUES 
(13, 1, 116992, 51, 5, 65558, 'Trigger Ji Guardian Exit target Ji Firepaw'),
(13, 1, 116992, 33, 1, 3, 'Trigger Ji Guardian Exit target Ji Firepaw owned by summoner'),
(13, 1, 104855, 51, 5, 55786, 'Overpacked Firework target Zhao-Ren'),
(13, 1, 108845, 51, 5, 55585, 'Summon Jojo Ironbrow target Jojo Ironbrow'),
(13, 1, 108846, 51, 5, 57668, 'Jojo Headbash, Stack of Blocks Impact target Stack of Blocks'),
(13, 1, 108857, 51, 5, 55585, 'Summon Jojo Ironbrow target Jojo Ironbrow'),
(13, 1, 105333, 51, 5, 55586, 'Summon Master Shang Xi target Master Shang'),
(13, 1, 106623, 51, 5, 55672, 'Summon Master Shang Xi target Master Shang'),
(13, 1, 109335, 51, 8, 209981, 'Summon Walking Stick target Sacred Dirt'),
(13, 1, 109336, 51, 5, 57874, 'Trigger Walking Stick Blossom target Walking Stick');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=13 AND `SourceEntry` IN (104612,104615,128801,126043,126059);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(13, 1, 104612, 0, 0, 51, 0, 5, 55744, 0, 0, 0, 0, '', 'Trigger Aysa Wind Chamber Middle target Aysa Cloudsinger'),
(13, 1, 104615, 0, 0, 51, 0, 5, 55595, 0, 0, 0, 0, '', 'Trigger Aysa Wind Chamber Rear target Aysa Cloudsinger'),
(13, 1, 128801, 0, 0, 51, 0, 5, 55744, 0, 0, 0, 0, '', 'Trigger Aysa Wind Chamber Rear, Summon target Aysa Cloudsinger'),
(13, 1, 126043, 0, 0, 51, 0, 5, 55592, 0, 0, 0, 0, '', 'Summon Dafeng target Dafeng'),
(13, 1, 126059, 0, 0, 51, 0, 5, 55595, 0, 0, 0, 0, '', 'Summon Aysa target Aysa Cloudsinger');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=22 AND `SourceEntry` IN (7041,7037);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(22, 1, 7041, 2, 0, 9, 0, 29785, 0, 0, 0, 0, 0, '', 'SAI only when quest taken'),
(22, 1, 7041, 2, 0, 1, 0, 104571, 1, 0, 1, 0, 0, '', 'SAI only when player has not aura'),
(22, 1, 7037, 2, 0, 9, 0, 29785, 0, 0, 0, 0, 0, '', 'SAI only when quest taken'),
(22, 1, 7037, 2, 0, 1, 0, 104615, 1, 0, 1, 0, 0, '', 'SAI only when player has not aura');


UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (57799);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (57799);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('57799', '0', '0', '0', '63', '0', '100', '0', '0', '0', '0', '0', '0', '', '20', '0', '0', '0', '0', '0', '0', '21', '30', '0', '0', '0', '0', '0', '0', 'Thornbranch Scamp - On Respawn - Attack nearest player');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (55672);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55672);
INSERT INTO `smart_scripts` VALUES
('55672', '0', '0', '0', '', '19', '0', '100', '0', '29790', '0', '0', '0', '0', '', '41', '1000', '30000', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Master Shang - On Accepted Quest - Force Despawn'),
('55672', '0', '1', '0', '', '20', '0', '100', '0', '29787', '0', '0', '0', '0', '', '1', '0', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Master Shang - On Reward Quest - Master Shang Xi Talk');

-- Delete existing reference loot
DELETE FROM creature_loot_template 
WHERE Entry IN (55601,57465,55632,57466,55633,55634) 
AND ItemType = 1;

-- NPC: 55601 Fe-Feng Wiseman (Humanoid)
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(55601, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(55601, 1, 4, 100, 0, 1, 1, 1, 'Humanoids');

-- NPC: 57465 Fe-Feng Ruffian (Humanoid)
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(57465, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(57465, 1, 4, 100, 0, 1, 1, 1, 'Humanoids');

-- NPC: 55632 Fe-Feng Ruffian (Humanoid)
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(55632, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(55632, 1, 4, 100, 0, 1, 1, 1, 'Humanoids');

-- NPC: 57466 Fe-Feng Firethief (Humanoid)
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(57466, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(57466, 1, 4, 100, 0, 1, 1, 1, 'Humanoids');

-- NPC: 55633 Fe-Feng Firethief (Humanoid)
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(55633, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(55633, 1, 4, 100, 0, 1, 1, 1, 'Humanoids');

-- NPC: 55634 Ruk-ruk (Humanoid)
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(55634, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(55634, 1, 4, 100, 0, 1, 1, 1, 'Humanoids');

-- NPC: 55640 Thornbranch Scamp (Humanoid)
INSERT INTO creature_loot_template (Entry, ItemType, Item, Chance, QuestRequired, LootMode, MinCount, MaxCount, Comment) VALUES
(55634, 1, 1, 100, 0, 1, 1, 1, 'General loot'),
(55634, 1, 4, 100, 0, 1, 1, 1, 'Humanoids');


UPDATE `gameobject_template` SET `Data6` = 120000, `Data23` = 1 WHERE `entry` IN (209656, 209660, 209661, 209663);

UPDATE `gameobject_template_addon` SET `flags` = 0 WHERE `entry` IN (209660, 209661); -- was 36
UPDATE `gameobject_template_addon` SET `flags` = 32 WHERE `entry` IN (209685); -- was 36

DELETE FROM `gameobject`
WHERE `guid` IN (
    300646, 300791, 300643, 300789, 300540, 300642, 300648, 300539,
    300644, 300787, 300788, 300759, 300751, 300753, 300466, 300560,
    300793, 300488, 300650, 300792, 300490, 300562, 300790, 300489,
    300561, 300649
); -- duplicate spawns or maybe for another phase????

-- Balloon Event

-- NPC: 55918 Balloon - stationary
-- NPC: 55649 Balloon - spawned by 105002 Summon hot air Balloon
-- NPC: 56660 Ji on the balloon
-- NPC: 56661 Aysa - spawned by 106636
-- NPC: 56662 Aysa - stationary
-- NPC: 56663 Ji - stationary
-- NPC: 56679 Balloon arrival controller
-- NPC: 40789 Generic controller
-- NPC: 65105 Shu stationary
-- NPC: 65102 Dafeng stationary
-- NPC: 65104 Wugou stationary
-- NPC: 65107 Huo stationary
-- NPC: 56012 Elder Shaopai

DELETE FROM `creature` WHERE `guid` IN (451635, 451634); -- Ji and huo are spawned by the vehicle
UPDATE `creature` SET `PhaseId` = 1885 WHERE `guid` IN (451656,451644,451654,451638,451715,451652);

UPDATE `creature_template` SET `VehicleId` = '1820', `ScriptName` = 'npc_shang_xi_hot_air_balloon' WHERE (`entry` = '55649');
UPDATE `creature_template` SET `VehicleId` = '1887', `ScriptName` = 'npc_shang_xi_hot_air_balloon' WHERE (`entry` = '55918');

DELETE FROM `creature_template_addon` WHERE `entry` IN (55649, 55918, 56661, 56662,56663, 56676);
INSERT INTO `creature_template_addon` VALUES
('55649', '0', '0', '0', '0', '3', '0', '1', '0', '0', '0', '0', '0', '0', ''), -- Spawned Balloon
('55918', '0', '0', '0', '0', '3', '0', '1', '0', '0', '0', '0', '0', '3', ''), -- Balloon
('56661', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '63313'),
('56662', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', ''),
('56663', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '46598 49414'),
('56676', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '4', ''); -- Shen-zin Su



DELETE FROM `vehicle_template` WHERE `creatureId` IN (55649, 55918);
INSERT INTO `vehicle_template` (`creatureId`, `despawnDelayMs`) VALUES ('55649', '10000');
INSERT INTO `vehicle_template` (`creatureId`, `despawnDelayMs`) VALUES ('55918', '0');

DELETE FROM `vehicle_template_accessory` WHERE `entry` IN (55918,55649);
INSERT INTO `vehicle_template_accessory` (`entry`, `accessory_entry`, `seat_id`, `minion`, `description`, `summontype`, `summontimer`, `RideSpellID`) VALUES 
('55649', '56661', '1', '1', 'Aysa on the balloon', '8', '0', '46598'),
('55649', '56660', '2', '1', 'Ji on the balloon', '8', '0', '46598'),
('55649', '65107', '3', '1', 'Huo on the balloon', '8', '0', '46598'),
('55649', '65102', '4', '1', 'Dafeng on the balloon', '8', '0', '46598'),
('55918', '56663', '1', '1', 'Ji on the balloon', '8', '0', '46598'),
('55918', '65107', '3', '1', 'Huo on the balloon', '8', '0', '46598');



DELETE FROM `creature_template_difficulty` WHERE `entry` IN (55649, 55918);
INSERT INTO `creature_template_difficulty` VALUES
('55649', '0', '0', '0', '80', '4', '30', '1', '1', '1', '29385', '1073741824', '0', '0', '0', '0', '0', '0', '0', '805306368', '0', '262144', '0', '0', '0', '0', '0', '56647'),
('55918', '0', '0', '0', '80', '4', '30', '1', '1', '1', '28878', '1610612736', '6', '0', '0', '0', '0', '0', '0', '805306624', '0', '33816576', '0', '0', '0', '0', '0', '56647');

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN (55649, 55918);
INSERT INTO `npc_spellclick_spells` (npc_entry, spell_id, cast_flags, user_type) VALUES
('55649', '46598', '0', '0'),
('55918', '46598', '0', '0'),
('55918', '56685', '1', '0');
