-- Quest: 5805 Welcome! (Goldshire)
-- Quest: 5842 Welcome! (Dolanaar)
-- Quest: 5841 Welcome! (Kharanos)
-- Quest: 9278 Welcome! (Azure Watch)
-- NPC: 16476 Jaeleil
-- NPC: 11940 Merissa Stilwell
-- NPC: 11941 Yori Crackhelm
-- NPC: 11942 Orenthil Whisperwind
-- Item: 14646 Goldshire Gift Voucher
-- Item: 14647 Kharanos Gift Voucher
-- Item: 14648 Dolanaar Gift Voucher
-- Item: 22888 Azure Watch Gift Voucher

DELETE FROM `playercreateinfo_item` WHERE `itemid` = 14648;
DELETE FROM `playercreateinfo_item` WHERE `itemid` = 14646;
DELETE FROM `playercreateinfo_item` WHERE `itemid` = 14647;
DELETE FROM `playercreateinfo_item` WHERE `itemid` = 22888;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES ('4', '0', '14648', '1');
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES ('1', '0', '14646', '1');
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES ('3', '0', '14647', '1');
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES ('7', '0', '14647', '1');
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES ('11', '0', '22888', '1');

DELETE FROM `disables` WHERE `entry` = 5841 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 5842 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 5805 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 9278 AND `sourceType` = 1; -- quest source

DELETE FROM `creature_questender` WHERE `quest` = 5841;
DELETE FROM `creature_questender` WHERE `quest` = 5842;
DELETE FROM `creature_questender` WHERE `quest` = 5805;
DELETE FROM `creature_questender` WHERE `quest` = 9278;
INSERT IGNORE INTO `creature_questender` VALUES
(16476, 9278, 0),
(11941, 5841, 0),
(11942, 5842, 0),
(11940, 5805, 0);

-- Quest: 5843 Welcome! (Razor Hill)
-- Quest: 5844 Welcome! (Bloodhoof Village)
-- Quest: 5847 Welcome! (Brill)
-- Quest: 8547 Welcome! (Falconwing Square)
-- NPC: 11943 Magga
-- NPC: 11944 Vorn Skyseer
-- NPC: 11945 Claire Willower
-- NPC: 15493 Marsilla Dawnstar
-- Item: 14649 Razor Hill Gift Voucher
-- Item: 14650 Bloodhoof Village Gift Voucher
-- Item: 14651 Brill Gift Voucher
-- Item: 20938 Falconwing Square Gift Voucher


DELETE FROM `playercreateinfo_item` WHERE `itemid` = 14649;
DELETE FROM `playercreateinfo_item` WHERE `itemid` = 14650;
DELETE FROM `playercreateinfo_item` WHERE `itemid` = 14651;
DELETE FROM `playercreateinfo_item` WHERE `itemid` = 20938;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES ('2', '0', '14649', '1');
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES ('8', '0', '14649', '1');
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES ('6', '0', '14650', '1');
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES ('5', '0', '14651', '1');
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES ('10', '0', '20938', '1');

DELETE FROM `disables` WHERE `entry` = 5843 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 5844 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 5847 AND `sourceType` = 1; -- quest source
DELETE FROM `disables` WHERE `entry` = 8547 AND `sourceType` = 1; -- quest source

DELETE FROM `creature_questender` WHERE `quest` = 5843;
DELETE FROM `creature_questender` WHERE `quest` = 5844;
DELETE FROM `creature_questender` WHERE `quest` = 5847;
DELETE FROM `creature_questender` WHERE `quest` = 8547;
INSERT IGNORE INTO `creature_questender` VALUES
(11943, 5843, 0),
(11944, 5844, 0),
(11945, 5847, 0),
(15493, 8547, 0);