-- Sentinel Hill changes and updates

-- NPC: 234 Gryan Stoutmantle
-- NPC: 491 Quartermaster Lewis
-- NPC: 523 Thor
-- NPC: 821 Captain Danuvin
-- NPC: 878 Scout Galiaan
-- NPC: 42384 Homeless Stormwind Citizen
-- NPC: 42386 Homeless Stormwind Citizen (Female
-- NPC: 42575 Hope Saldean
-- NPC: 42635 Ripsnarl

-- GO: 204005 Heavy Wooden Stocks

-- Quest: 12 The People's militia
-- Quest: 13 The People's militia
-- Quest: 14 The People's militia
-- Quest: 102 Patrolling Westfall
-- Quest: 153 Red Leather Bandanas
-- Quest: 6181 A swift message
-- Quest: 6281 Continue to Stormwind
-- Quest: 26271 Feeding the hungry

-- Quest: 65 The Defias Brotherhood
-- Quest: 132 The Defias Brotherhood
-- Quest: 135 The Defias Brotherhood
-- Quest: 141 The Defias Brotherhood
-- Quest: 142 The Defias Brotherhood
-- Quest: 155 The Defias Brotherhood
-- Quest: 166 The Defias Brotherhood


-- Reactivating old/deprecated quests in Sentinel Hill
DELETE FROM `disables` WHERE (`sourceType` = '1') and `entry` IN (12, 13, 14, 102, 153, 65, 132, 135, 141, 142, 155, 166);
DELETE FROM `disables` WHERE (`sourceType` = '1') and `entry` IN (6181, 6281);

-- Quest Templates
UPDATE `quest_template` SET `ContentTuningID` = '6', `Expansion` = 0, `AllowableRaces` = 18446744073709551615 WHERE `ID` IN (12, 13, 14, 102, 153, 65, 132, 135, 141, 142, 155, 166, 6181, 6281);

-- Creature Quest Relations
DELETE FROM `creature_queststarter` WHERE `quest` IN (12, 13, 14, 102, 153, 65, 132, 135, 141, 142, 155, 166, 6181, 6281);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES 
('523', '6281', '0'),
('491', '6181', '0'),
('821', '102', '0'),
('878', '153', '0'),
('234', '12', '0'),
('234', '13', '0'),
('234', '14', '0'),
('234', '65', '0'),
('266', '132', '0'),
('234', '135', '0'),
('332', '141', '0'),
('234', '142', '0'),
('467', '155', '0'),
('234', '166', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (12, 13, 14, 102, 153, 65, 132, 135, 141, 142, 155, 166, 6181,6281);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES 
('1323', '6281', '0'),
('523', '6181', '0'),
('821', '102', '0'),
('878', '153', '0'),
('234', '12', '0'),
('234', '13', '0'),
('234', '14', '0'),
('266', '65', '0'),
('234', '132', '0'),
('332', '135', '0'),
('234', '141', '0'),
('234', '142', '0'),
('234', '155', '0'),
('234', '166', '0');

-- Quest: 26271 Feeding the hungry
UPDATE `creature_template` SET `ScriptName` = 'npc_custom_hungry_hobo' WHERE (`entry` = '42384');
UPDATE `creature_template` SET `ScriptName` = 'npc_custom_hungry_hobo' WHERE (`entry` = '42386');