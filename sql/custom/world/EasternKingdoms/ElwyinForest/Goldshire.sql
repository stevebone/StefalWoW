-- Goldshire changes and updates

-- NPC: 30 Forest Spider
-- NPC: 40 Kobold Miner
-- NPC: 94 Cutpurse
-- NPC: 240 Marshal Dughan
-- NPC: 247 Billy Maclure
-- NPC: 250 Pa Maclure
-- NPC: 253 William Pestle
-- NPC: 261 Guard Thomas
-- NPC: 279 Morgan Pestle
-- NPC: 295 Innkeeper Farley
-- NPC: 383 Jason Mathers
-- NPC: 475 Kobold Tunneler
-- NPC: 525 Mangy Wolf
-- NPC: 795 Mark
-- NPC: 796 Joshua
-- NPC: 797 Bo
-- NPC: 804 Dana
-- NPC: 805 Cameron
-- NPC: 806 John
-- NPC: 807 Lisa
-- NPC: 810 Aaron
-- NPC: 811 Jose
-- NPC: 6121 Remen Marcot
-- NPC: 6122 Gakin (Stormwind)
-- NPC: 6774 Falkhaan
-- NPC: 7381 Cat
-- NPC: 7382 Cat
-- NPC: 7384 Cat
-- NPC: 7385 Cat
-- NPC: 11940 Merissa Stilwell
-- NPC: 63014 Marcus Jensen

-- Quest: 61 Shipment to Stormwind
-- Quest: 1685 Gakin's Summons

-- Reactivating old/deprecated quests in Goldshire
DELETE FROM disables where sourceType = 1 AND entry = 61;
DELETE FROM disables where sourceType = 1 AND entry = 1685;

-- Quest Starters / Enders
DELETE FROM `creature_queststarter` WHERE `quest` IN (61, 1685);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('253', '61', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('6121', '1685', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (61, 1685);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('279', '61', '0');
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('6122', '1685', '0');

-- Quest Templates
UPDATE `quest_template` SET `ContentTuningID` = '73', `QuestSortID` = '12', `RewardFactionID1` = '72', `RewardFactionValue1` = '1', `RewardFactionFlags` = '1', `AllowableRaces` = '18446744073709551615', `Expansion` = '0' WHERE (`ID` = '1685');
UPDATE `quest_template_addon` SET `AllowableClasses` = '0' WHERE (`ID` = '1685');

-- Creature Templates
UPDATE `creature_template` SET `faction` = '12', `npcflag` = '0', `AIName` = '' WHERE (`entry` = '383'); -- Jason Mathers updates
UPDATE `creature_template` SET `faction` = '12' WHERE `entry` IN (247, 250, 795, 796, 797, 804, 805, 806, 807, 810, 811); -- incorrect faction
UPDATE `creature_template` SET `npcflag` = '179' WHERE (`entry` = '63014'); -- Marcus Jensen npc flag
UPDATE `creature_template` SET `AIName` = '' WHERE `entry` IN (6121); -- remove AI Name for creatures without scripts


-- Creature Texts
DELETE FROM `creature_text` WHERE `CreatureID` IN (94);
INSERT INTO `creature_text` VALUES
('94', '0', '0', 'You\'re in the wrong place at the wrong time, $g pal:missy;!', '12', '0', '100', '0', '0', '0', '0', '42883', '0', 'combat Say'),
('94', '0', '1', 'Give me all your gold!', '12', '0', '100', '0', '0', '0', '0', '0', '0', 'combat Say'),
('94', '0', '2', 'No one here gets out alive!', '12', '0', '100', '0', '0', '0', '0', '0', '0', 'combat Say');

-- Scripts Cleanup
DELETE FROM `smart_scripts` WHERE  `entryorguid` IN (30, 94, 383, 525, 795, 796, 797, 804, 805, 806, 807, 810, 811, 7381, 7382, 7384, 7385, 11940, 63014) AND `id` IN (10000, 10001, 11000, 11001, 11002, 11003, 11004);
DELETE FROM `smart_scripts` WHERE  `entryorguid` IN (40, 247, 250, 253, 261, 279, 295, 475, 6121, 6774) AND `id` IN (10000, 10001, 11000, 11001, 11002, 11003, 11004);

-- Trainers
INSERT IGNORE INTO `trainer_spell` VALUES
('580', '125610', '1000', '0', '0', '0', '0', '0', '1', '41079'); -- Battle Pet Training for Marcus Jensen