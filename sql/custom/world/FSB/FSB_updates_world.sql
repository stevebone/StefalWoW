-- Trainers Stormwind
-- 914/ander-germaine
UPDATE `creature_template_difficulty` SET `ContentTuningID` = '864' WHERE (`Entry` = '914') and (`DifficultyID` = '0');
DELETE FROM `creature` WHERE `ID` = 914;
INSERT INTO `creature` VALUES
('900267', '914', '0', '1519', '1519', '0', '0', '0', '0', '-1', '0', '1', '-8795.63', '356.435', '107.27', '6.18479', '300', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0');

-- 63331/laoxi
DELETE FROM `creature` WHERE `ID` = 63331;
INSERT INTO `creature` VALUES
('900268', '63331', '1', '6450', '256', '0', '0', '0', '0', '-1', '0', '1', '10425.4', '765.203', '1322.67', '1.76636', '300', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0');
