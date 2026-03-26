-- Sentinel Hill changes and updates

-- NPC: 234 Gryan Stoutmantle
-- NPC: 821 Captain Danuvin
-- NPC: 878 Scout Galiaan

-- Quest: 12 The People's militia
-- Quest: 13 The People's militia
-- Quest: 14 The People's militia
-- Quest: 102 Patrolling Westfall
-- Quest: 153 Red Leather Bandanas


-- Reactivating old/deprecated quests in Sentinel Hill
DELETE FROM `disables` WHERE (`sourceType` = '1') and `entry` IN (12, 13, 14, 102, 153);

-- Quest Templates
UPDATE `quest_template` SET `ContentTuningID` = '6', `Expansion` = 0 WHERE `ID` IN (12, 13, 14, 102, 153);

-- Creature Quest Relations
DELETE FROM `creature_queststarter` WHERE `quest` IN (12, 13, 14, 102, 153);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES 
('821', '102', '0'),
('878', '153', '0'),
('234', '12', '0'),
('234', '13', '0'),
('234', '14', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (12, 13, 14, 102, 153);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES 
('821', '102', '0'),
('878', '153', '0'),
('234', '12', '0'),
('234', '13', '0'),
('234', '14', '0');