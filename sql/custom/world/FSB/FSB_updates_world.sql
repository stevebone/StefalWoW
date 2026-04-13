-- Updates related to FSB
-- Remove spawned hunter pets since they are now spell spawned by master
-- Elwynn
DELETE FROM `creature` WHERE `guid` = 280677;


-- 63331/laoxi
DELETE FROM `creature` WHERE `ID` = 63331;
INSERT INTO `creature` VALUES
('900268', '63331', '1', '6450', '256', '0', '0', '0', '0', '-1', '0', '1', '10425.4', '765.203', '1322.67', '1.76636', '300', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0');
