-- Northshire changes and updates

-- Phase activation
DELETE FROM `phase_area` WHERE `PhaseId` = 50001;
DELETE FROM `phase_area` WHERE `PhaseId` = 50002;
DELETE FROM `phase_area` WHERE `PhaseId` = 50003;
DELETE FROM `phase_area` WHERE `PhaseId` = 169 AND `AreaId` IN (9, 24, 34, 59);
INSERT INTO `phase_area` VALUES
(9, 169, 'Northshire Valley - Kobold Camp Cleanup'),
(24, 169, 'Northshire Valley - Kobold Camp Cleanup'),
(34, 169, 'Northshire Valley - Kobold Camp Cleanup'),
(59, 169, 'Northshire Valley - Garrick Padfoot');

INSERT INTO `phase_area` VALUES
(9, 50001, 'Northshire Valley - Kobold Camp Cleanup'),
(24, 50001, 'Northshire Valley - Kobold Camp Cleanup'),
(34, 50001, 'Northshire Valley - Kobold Camp Cleanup'),
(59, 50001, 'Northshire Valley - Kobold Camp Cleanup');

INSERT INTO `phase_area` VALUES
(59, 50002, 'Northshire Valley - Garrick Padfoot'),
(34, 50002, 'Northshire Valley - Garrick Padfoot'),
(9, 50002, 'Northshire Valley - Garrick Padfoot');

INSERT INTO `phase_area` VALUES
(9, 50003, 'Northshire Valley - Report to Goldshire'),
(24, 50003, 'Northshire Valley - Report to Goldshire'),
(34, 50003, 'Northshire Valley - Report to Goldshire'),
(59, 50003, 'Northshire Valley - Report to Goldshire');

-- Phase Conditions
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26 AND `SourceGroup` = 50001;
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26 AND `SourceGroup` = 50002;
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26 AND `SourceGroup` = 50003;
INSERT INTO `conditions` VALUES
('26', '50001', '9', '0', '0', '47', '0', '6', '64', '0', '', '1', '0', '0', '', 'Northshire - Add phase 50001 if 6 IS NOT rewarded'),
('26', '50001', '24', '0', '0', '47', '0', '6', '64', '0', '', '1', '0', '0', '', 'Northshire - Add phase 50001 if 6 IS NOT rewarded'),
('26', '50001', '34', '0', '0', '47', '0', '6', '64', '0', '', '1', '0', '0', '', 'Northshire - Add phase 50001 if 6 IS NOT rewarded'),
('26', '50001', '59', '0', '0', '47', '0', '6', '64', '0', '', '1', '0', '0', '', 'Northshire - Add phase 50001 if 6 IS NOT rewarded'),
('26', '50002', '9', '0', '0', '47', '0', '6', '64', '0', '', '0', '0', '0', '', 'Northshire - Add phase 50002 if 6 IS rewarded'),
('26', '50002', '34', '0', '0', '47', '0', '6', '64', '0', '', '0', '0', '0', '', 'Northshire - Add phase 50002 if 6 IS rewarded'),
('26', '50002', '59', '0', '0', '47', '0', '6', '64', '0', '', '0', '0', '0', '', 'Northshire - Add phase 50002 if 6 IS rewarded'),
('26', '50002', '9', '0', '0', '47', '0', '54', '64', '0', '', '1', '0', '0', '', 'Northshire - Add phase 50002 if 54 IS NOT rewarded'),
('26', '50002', '34', '0', '0', '47', '0', '54', '64', '0', '', '1', '0', '0', '', 'Northshire - Add phase 50002 if 54 IS NOT rewarded'),
('26', '50002', '59', '0', '0', '47', '0', '54', '64', '0', '', '1', '0', '0', '', 'Northshire - Add phase 50002 if 54 IS NOT rewarded'),
('26', '50003', '9', '0', '0', '47', '0', '54', '64', '0', '', '0', '0', '0', '', 'Northshire - Add phase 50003 if 54 IS rewarded'),
('26', '50003', '24', '0', '0', '47', '0', '54', '64', '0', '', '0', '0', '0', '', 'Northshire - Add phase 50003 if 54 IS rewarded'),
('26', '50003', '34', '0', '0', '47', '0', '54', '64', '0', '', '0', '0', '0', '', 'Northshire - Add phase 50003 if 54 IS rewarded'),
('26', '50003', '59', '0', '0', '47', '0', '54', '64', '0', '', '0', '0', '0', '', 'Northshire - Add phase 50003 if 54 IS rewarded');

-- Quest: 6 Bounty on Garrick Padfoot
-- Quest: 7 Kobold Camp Cleanup
-- Quest: 15 Investigate Echo Ridge
-- Quest: 18 Brotherhood of Thieves
-- Quest: 21 Skirmish at Echo Ridge
-- Quest: 33 Wolves Across the Border
-- Quest: 54 Report to Goldshire
-- Quest: 783 A Threat Within
-- Quest: 5261 Eagan Peltskinner

-- NPC: 6 Kobold Vermin
-- NPC: 38 Defias Thug
-- NPC: 62 Gug Fatcandle
-- NPC: 80 Kobold Laborer
-- NPC: 103 Garrick Padfoot
-- NPC: 196 Eagan Peltskinner
-- NPC: 197 Marshal McBride
-- NPC: 257 Kobold Worker
-- NPC: 299 Young Wolf
-- NPC: 823 Deputy Willem
-- NPC: 951 Brother Paxton
-- NPC: 9296 Milly
-- NPC: 11260 Northshire Peasant
-- NPC: 42937 Blackrock Invader
-- NPC: 42938 Kurtok The Slayer
-- NPC: 42940 Fire Trigger
-- NPC: 49871 Blackrock Worg
-- NPC: 49874 Blackrock Spy
-- NPC: 49869 Stormwind Infantry
-- NPC: 50039 Goblin Assassin
-- NPC: 50047 Injured Stormwind Infantry

-- Creature Templates
UPDATE `creature` SET `position_x` = '-8888.920', `position_y` = '-279.9920', `position_z` = '80.35540', `orientation` = '5.433910' WHERE (`guid` = '279826'); -- Northshire Peasant position
UPDATE `creature_template` SET `faction` = '12' WHERE (`entry` = '196'); -- Eagen faction fix
UPDATE `creature_template_difficulty` SET `DifficultyID` = 0 WHERE `DifficultyID` != 0 AND `Entry` IN (6, 38, 62, 80, 103, 257, 299);
UPDATE `creature_template_difficulty` SET `ContentTuningID` = 70 WHERE `ContentTuningID` != 70 AND `Entry` IN (6, 38, 62, 80, 103, 257, 299);
UPDATE `creature_loot_template` SET `Chance` = 1 WHERE `Chance` < 1 AND `Entry` IN (6, 38, 62, 80, 103, 257, 299, 50039, 49874, 49871, 42937, 42938);

-- Creature Spawns
UPDATE `creature` SET `PhaseId` = 50001 WHERE `Id` IN (62);
UPDATE `creature` SET `PhaseId` = 50002 WHERE `Id` IN (50039, 50047, 49874, 49871, 49869, 42937, 42938, 42940);
UPDATE `creature` SET `PhaseId` = '50002' WHERE (`guid` = '279904');
UPDATE `creature` SET `PhaseId` = '50002' WHERE (`guid` = '279895');
UPDATE `creature` SET `PhaseId` = '50002' WHERE (`guid` = '279975');

-- Reactivating old/deprecated quests in Northshire
DELETE FROM `disables` WHERE (`sourceType` = '1') and (`entry` = '6');
DELETE FROM `disables` WHERE (`sourceType` = '1') and (`entry` = '7');
DELETE FROM `disables` WHERE (`sourceType` = '1') and (`entry` = '15');
DELETE FROM `disables` WHERE (`sourceType` = '1') and (`entry` = '18');
DELETE FROM `disables` WHERE (`sourceType` = '1') and (`entry` = '21');
DELETE FROM `disables` WHERE (`sourceType` = '1') and (`entry` = '33');
DELETE FROM `disables` WHERE (`sourceType` = '1') and (`entry` = '54');
DELETE FROM `disables` WHERE (`sourceType` = '1') and (`entry` = '783');
DELETE FROM `disables` WHERE (`sourceType` = '1') and (`entry` = '5261');

-- remove deprecated title for old quests in Northshire
UPDATE `quest_template` SET `LogTitle` = 'Eagan Peltskinner' WHERE (`ID` = '5261');
UPDATE `quest_template` SET `LogTitle` = 'A Threat Within' WHERE (`ID` = '783');
UPDATE `quest_template` SET `LogTitle` = 'Wolves Across the Border' WHERE (`ID` = '33');
UPDATE `quest_template` SET `LogTitle` = 'Skirmish at Echo Ridge' WHERE (`ID` = '21');
UPDATE `quest_template` SET `LogTitle` = 'Investigate Echo Ridge' WHERE (`ID` = '15');
UPDATE `quest_template` SET `LogTitle` = 'Kobold Camp Cleanup' WHERE (`ID` = '7');

-- update the tuning ids for actual xp and AllowableRaces
UPDATE `quest_template` SET `ContentTuningID` = '1476', `Expansion` = '0', `AllowableRaces` = 18446744073709551615 WHERE `ID` IN (6,7,15,18,21,783,5261,33);
UPDATE `quest_template` SET `Expansion` = '0', `AllowableRaces` = 18446744073709551615 WHERE (`ID` = '54');

-- update quest starters and enders
DELETE FROM `creature_queststarter` WHERE `quest` IN (6,7,15,18,21,54,783,5261,33);
DELETE FROM `creature_questender` WHERE `quest` IN (6,7,15,18,21,54,783,5261,33);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('823', '6', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('197', '7', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('197', '15', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('823', '18', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('197', '21', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('197', '54', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('823', '783', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('196', '33', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('823', '5261', '0');

INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('823', '6', '0');
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('197', '7', '0');
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('197', '15', '0');
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('823', '18', '0');
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('197', '21', '0');
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('240', '54', '0');
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('197', '783', '0');
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('196', '33', '0');
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('196', '5261', '0');

UPDATE `quest_template_addon` SET `PrevQuestID` = '6' WHERE (`ID` = '28757');
UPDATE `quest_template_addon` SET `PrevQuestID` = '6' WHERE (`ID` = '28762');
UPDATE `quest_template_addon` SET `PrevQuestID` = '6' WHERE (`ID` = '28763');
UPDATE `quest_template_addon` SET `PrevQuestID` = '6' WHERE (`ID` = '28764');
UPDATE `quest_template_addon` SET `PrevQuestID` = '6' WHERE (`ID` = '28765');
UPDATE `quest_template_addon` SET `PrevQuestID` = '6' WHERE (`ID` = '28766');
UPDATE `quest_template_addon` SET `PrevQuestID` = '6' WHERE (`ID` = '28767');
UPDATE `quest_template_addon` SET `PrevQuestID` = '6' WHERE (`ID` = '31139');


SET @CGUID := 900000;
DELETE FROM `creature` WHERE `id` IN (6, 38, 62, 80, 103, 257, 299, 823, 951, 9296) AND areaid IN (9, 24, 34, 59) AND `PhaseId` = 50001;
DELETE FROM `creature` WHERE `areaid` IN (9, 24, 34, 59) AND `PhaseId` = 50003;
INSERT INTO `creature` VALUES
(@CGUID+1,'6','0','6170','9','0','0','50001','0','-1','0','1','-8772.91','-114.961','83.0156','0.0201845','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+2,'6','0','6170','9','0','0','50001','0','-1','0','1','-8766.76','-117.605','83.5213','2.56215','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+3,'6','0','6170','9','0','0','50001','0','-1','0','1','-8768.59','-112.086','83.943','4.60654','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+4,'6','0','6170','9','0','0','50001','0','-1','0','1','-8766.84','-174.794','83.5547','3.83346','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+5,'6','0','6170','9','0','0','50001','0','-1','0','1','-8770.11','-174.423','82.9691','0.709926','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+6,'6','0','6170','9','0','0','50001','0','-1','0','1','-8769.58','-170.885','82.9753','5.35241','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+7,'6','0','6170','9','0','0','50001','0','-1','0','1','-8755.6','-190.476','85.2868','6.16958','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+8,'6','0','6170','9','0','0','50001','0','-1','0','1','-8750.13','-192.06','85.5723','2.651','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+9,'6','0','6170','9','0','0','50001','0','-1','0','1','-8751.35','-188.766','85.2701','4.06001','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+10,'6','0','6170','9','0','0','50001','0','-1','0','1','-8784.73','-245.334','82.9203','3.58037','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+11,'6','0','6170','9','0','0','50001','0','-1','0','1','-8787.73','-251.903','82.5159','2.1046','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+12,'6','0','6170','9','0','0','50001','0','-1','0','1','-8790.94','-244.121','82.684','5.08754','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+13,'6','0','6170','9','0','0','50001','0','-1','0','1','-8747.01','-165.561','85.0714','1.62258','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+14,'6','0','6170','9','0','0','50001','0','-1','0','1','-8743.02','-141.468','84.3735','3.36811','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+15,'6','0','6170','34','0','0','50001','0','-1','0','1','-8725.14','-112.094','86.0005','0.929531','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+16,'6','0','6170','34','0','0','50001','0','-1','0','1','-8715.46','-99.1306','88.1277','0.929531','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+17,'6','0','6170','34','0','0','50001','0','-1','0','1','-8724.91','-59.7303','90.7296','1.26171','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+18,'6','0','6170','34','0','0','50001','0','-1','0','1','-8727.93','-52.6886','90.6415','2.00706','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+19,'6','0','6170','34','0','0','50001','0','-1','0','1','-8700.84','-68.0105','90.3078','4.80757','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+20,'6','0','6170','34','0','0','50001','0','-1','0','1','-8680.62','-86.4131','91.1438','3.40344','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+21,'38','0','6170','9','0','0','50001','0','-1','5035','1','-8974.72','-351.066','74.429','2.30426','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+22,'38','0','6170','9','0','0','50001','0','-1','5036','1','-8844.78','-457.473','66.5192','6.21862','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+23,'38','0','6170','9','0','0','50001','0','-1','5035','1','-9060.58','-460.358','72.624','0.116584','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+24,'38','0','6170','9','0','0','50001','0','-1','5036','1','-8883.47','-426.693','68.572','2.59015','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+25,'38','0','6170','9','0','0','50001','0','-1','5035','1','-8888.94','-445.828','68.0281','2.42398','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+26,'38','0','6170','9','0','0','50001','0','-1','5036','1','-8863.28','-366.451','72.2761','0.0792441','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+27,'38','0','6170','9','0','0','50001','0','-1','5035','1','-8806.15','-376.393','72.505','4.21191','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+28,'38','0','6170','9','0','0','50001','0','-1','5036','1','-8903.08','-345.584','70.7605','3.1074','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+29,'38','0','6170','9','0','0','50001','0','-1','5035','1','-8922.84','-386.492','71.0613','2.24698','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+30,'38','0','6170','9','0','0','50001','0','-1','5036','1','-8898.18','-387.975','69.7302','4.67337','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+31,'38','0','6170','9','0','0','50001','0','-1','5035','1','-8926.51','-353.925','72.8651','2.80156','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+32,'38','0','6170','9','0','0','50001','0','-1','5036','1','-8914.2','-434.974','73.0506','3.96664','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+33,'38','0','6170','9','0','0','50001','0','-1','5035','1','-8878.7','-435.696','68.2078','2.44625','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+34,'38','0','6170','9','0','0','50001','0','-1','5036','1','-8889.01','-365.047','72.7613','5.78922','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+35,'38','0','6170','9','0','0','50001','0','-1','5035','1','-9063.13','-446.592','72.4153','0.763675','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+36,'38','0','6170','9','0','0','50001','0','-1','5036','1','-9009.22','-315.4','74.3136','0.0150199','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+37,'38','0','6170','9','0','0','50001','0','-1','5035','1','-8998.88','-330.642','75.0093','2.95219','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+38,'38','0','6170','9','0','0','50001','0','-1','5036','1','-8688.01','-298.762','84.8322','5.87753','300','0','0','100','0',NULL,'32768',NULL,NULL,'',NULL,'0'),
(@CGUID+39,'38','0','6170','9','0','0','50001','0','-1','5035','1','-8688.45','-294.628','85.3663','6.2052','300','0','0','100','0',NULL,'32768',NULL,NULL,'',NULL,'0'),
(@CGUID+40,'38','0','6170','59','0','0','50001','0','-1','5035','1','-8959.06','-410.456','72.3455','3.22786','300','0','0','100','0',NULL,'32768',NULL,NULL,'',NULL,'0'),
(@CGUID+41,'38','0','6170','59','0','0','50001','0','-1','5036','1','-8995.15','-403.983','74.318','0.670801','300','0','0','100','0',NULL,'32768',NULL,NULL,'',NULL,'0'),
(@CGUID+42,'38','0','6170','59','0','0','50001','0','-1','5036','1','-9013.6','-378.635','74.5504','2.10196','300','0','0','100','0',NULL,'32768',NULL,NULL,'',NULL,'0'),
(@CGUID+43,'38','0','6170','59','0','0','50001','0','-1','5035','1','-9040.39','-279.971','74.1504','2.18166','300','0','0','100','0',NULL,'32768',NULL,NULL,'',NULL,'0'),
(@CGUID+44,'38','0','6170','59','0','0','50001','0','-1','5035','1','-9078.54','-255.862','73.7132','3.44119','300','0','0','100','0',NULL,'32768',NULL,NULL,'',NULL,'0'),
(@CGUID+45,'38','0','6170','59','0','0','50001','0','-1','5036','1','-9112.89','-272.327','73.6135','3.96637','300','0','0','100','0',NULL,'32768',NULL,NULL,'',NULL,'0'),
(@CGUID+46,'38','0','6170','59','0','0','50001','0','-1','5036','1','-9140.82','-291.148','74.0534','3.02493','300','0','0','100','0',NULL,'32768',NULL,NULL,'',NULL,'0'),
(@CGUID+47,'38','0','6170','59','0','0','50001','0','-1','5035','1','-9139.34','-287.449','73.974','2.88633','300','0','0','100','0',NULL,'32768',NULL,NULL,'',NULL,'0'),
(@CGUID+48,'38','0','6170','59','0','0','50001','0','-1','5035','1','-9147.46','-345.755','73.028','4.46671','300','0','0','100','0',NULL,'32768',NULL,NULL,'',NULL,'0'),
(@CGUID+49,'38','0','6170','59','0','0','50001','0','-1','5036','1','-9124.89','-365.744','73.3884','2.80778','300','0','0','100','0',NULL,'32768',NULL,NULL,'',NULL,'0'),
(@CGUID+50,'38','0','6170','59','0','0','50001','0','-1','0','1','-9051.6','-460.242','72.9275','2.86116','300','0','0','100','0',NULL,'32768',NULL,NULL,'',NULL,'0'),
(@CGUID+51,'62','0','6170','34','0','0','50001','0','-1','0','1','-8678.75','-118.523','91.1479','2.89725','1800','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+52,'80','0','6170','34','0','0','50001','0','-1','0','1','-8694.38','-123.922','88.2089','2.63244','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+53,'80','0','6170','34','0','0','50001','0','-1','0','1','-8692.81','-115.508','88.9363','0.893647','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+54,'80','0','6170','34','0','0','50001','0','-1','0','1','-8686.61','-107.737','89.1437','1.84774','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+55,'80','0','6170','34','0','0','50001','0','-1','0','1','-8685.52','-101.519','89.2122','4.54794','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+56,'80','0','6170','34','0','0','50001','0','-1','0','1','-8683.89','-104.029','89.1589','3.1837','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+57,'80','0','6170','34','0','0','50001','0','-1','0','1','-8688.05','-111.858','88.924','2.40789','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+58,'80','0','6170','34','0','0','50001','0','-1','0','1','-8620.08','-80.5622','105.812','3.27219','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+59,'80','0','6170','34','0','0','50001','0','-1','0','1','-8639.89','-83.1649','101.844','3.27219','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+60,'80','0','6170','34','0','0','50001','0','-1','0','1','-8688.75','-168.988','90.9543','5.53078','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+61,'80','0','6170','34','0','0','50001','0','-1','0','1','-8678.2','-147.122','108.625','3.22837','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+62,'80','0','6170','34','0','0','50001','0','-1','0','1','-8632.39','-153.335','135.958','3.61875','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+63,'80','0','6170','34','0','0','50001','0','-1','0','1','-8663.45','-203.555','94.7068','5.63304','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+64,'80','0','6170','34','0','0','50001','0','-1','0','1','-8657.28','-207.896','96.7457','1.18533','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+65,'80','0','6170','34','0','0','50001','0','-1','0','1','-8658.39','-194.563','95.1609','1.68485','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+66,'80','0','6170','34','0','0','50001','0','-1','0','1','-8669.15','-185.962','91.657','2.51659','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+67,'80','0','6170','34','0','0','50001','0','-1','0','1','-8683.66','-199.346','92.5849','3.47477','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+68,'80','0','6170','34','0','0','50001','0','-1','0','1','-8706.56','-182.936','101.224','3.34989','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+69,'80','0','6170','34','0','0','50001','0','-1','0','1','-8629.77','-203.595','120.123','3.25862','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+70,'80','0','6170','34','0','0','50001','0','-1','0','1','-8685.35','-127.696','90.4222','2.11706','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+71,'80','0','6170','34','0','0','50001','0','-1','0','1','-8675.98','-97.1291','90.4801','3.67492','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+72,'80','0','6170','34','0','0','50001','0','-1','0','1','-8611.39','-60.6814','120.339','4.11275','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+73,'103','0','6170','9','0','0','50001','0','-1','0','1','-9055.18','-460.977','72.7436','1.59673','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+74,'257','0','6170','9','0','0','50001','0','-1','0','1','-8771.37','-117.481','83.2099','0.639775','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+75,'257','0','6170','9','0','0','50001','0','-1','0','1','-8762.8','-112.655','84.1747','3.8462','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+76,'257','0','6170','9','0','0','50001','0','-1','0','1','-8768.24','-175.989','83.3087','1.68005','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+77,'257','0','6170','9','0','0','50001','0','-1','0','1','-8767.04','-172.239','83.3986','3.43058','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+78,'257','0','6170','9','0','0','50001','0','-1','0','1','-8753.38','-186.929','85.2601','5.33266','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+79,'257','0','6170','9','0','0','50001','0','-1','0','1','-8753.05','-194.038','85.7384','1.60045','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+80,'257','0','6170','9','0','0','50001','0','-1','0','1','-8788.5','-245.836','82.7575','4.23567','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+81,'257','0','6170','9','0','0','50001','0','-1','0','1','-8785.38','-248.287','82.555','2.8353','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+82,'257','0','6170','9','0','0','50001','0','-1','0','1','-8791.43','-251.761','82.5213','1.08151','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+83,'257','0','6170','34','0','0','50001','0','-1','0','1','-8714.66','-134.959','86.0984','2.63148','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+84,'257','0','6170','34','0','0','50001','0','-1','0','1','-8715.99','-92.6558','89.3405','1.09722','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+85,'257','0','6170','34','0','0','50001','0','-1','0','1','-8705.59','-80.2253','93.5113','3.82256','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+86,'257','0','6170','34','0','0','50001','0','-1','0','1','-8720.82','-59.9395','90.6001','2.3668','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+87,'257','0','6170','9','0','0','50001','0','-1','0','1','-8734.25','-54.4308','91.0934','2.60531','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+88,'257','0','6170','9','0','0','50001','0','-1','0','1','-8743.25','-23.7469','91.19','0.911044','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+89,'257','0','6170','9','0','0','50001','0','-1','0','1','-8739.06','-22.9274','91.6059','2.24308','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+90,'257','0','6170','9','0','0','50001','0','-1','0','1','-8737.36','-19.5545','91.9933','3.21462','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+91,'257','0','6170','9','0','0','50001','0','-1','0','1','-8741.88','-16.9521','91.7983','4.58749','300','0','0','100','0',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+92,'257','0','6170','9','0','0','50001','0','-1','0','1','-8755.51','-11.2939','93.5789','5.24561','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+93,'257','0','6170','9','0','0','50001','0','-1','0','1','-8762.95','13.533','94.1317','1.89642','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+94,'257','0','6170','9','0','0','50001','0','-1','0','1','-8766.4','-0.33427','98.8241','5.32307','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+95,'257','0','6170','34','0','0','50001','0','-1','0','1','-8675.08','-61.9423','93.363','3.7941','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+96,'257','0','6170','34','0','0','50001','0','-1','0','1','-8655.79','-87.5447','95.2798','0.33453','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+97,'299','0','6170','9','0','0','50001','0','-1','0','0','-8760.99','-298.553','77.9046','0.472621','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+98,'299','0','6170','9','0','0','50001','0','-1','0','0','-8741.29','-288.751','80.731','0.163335','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+99,'299','0','6170','9','0','0','50001','0','-1','0','0','-8748.76','-270.514','82.1178','2.01696','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+100,'299','0','6170','9','0','0','50001','0','-1','0','0','-8729.79','-245.738','86.2612','2.25836','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+101,'299','0','6170','34','0','0','50001','0','-1','0','0','-8717.26','-206.499','89.7589','0.799093','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+102,'299','0','6170','34','0','0','50001','0','-1','0','0','-8680.02','-187.715','90.9177','0.857583','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+103,'299','0','6170','34','0','0','50001','0','-1','0','0','-8718.69','-153.196','85.6934','2.96972','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+104,'299','0','6170','9','0','0','50001','0','-1','0','0','-8754.11','-50.9106','92.5537','2.61104','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+105,'299','0','6170','9','0','0','50001','0','-1','0','0','-8822.6','-23.8582','89.6757','2.35103','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+106,'299','0','6170','9','0','0','50001','0','-1','0','0','-8841.87','2.5885','91.2854','2.15311','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+107,'299','0','6170','9','0','0','50001','0','-1','0','0','-8810.01','4.23243','92.5251','0.449213','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+108,'299','0','6170','9','0','0','50001','0','-1','0','0','-8747.84','6.53722','93.6271','0.637706','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+109,'299','0','6170','9','0','0','50001','0','-1','0','0','-8729.99','-14.5749','93.4267','4.2996','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+110,'299','0','6170','34','0','0','50001','0','-1','0','0','-8701.58','-161.508','88.5857','5.74542','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+111,'299','0','6170','34','0','0','50001','0','-1','0','0','-8652.87','-236.364','102.017','5.80677','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+112,'299','0','6170','9','0','0','50001','0','-1','0','0','-8790.42','-287.825','77.5978','3.82719','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+113,'299','0','6170','9','0','0','50001','0','-1','0','0','-8820.54','-299.268','77.4839','2.93396','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+114,'299','0','6170','9','0','0','50001','0','-1','0','0','-8824.13','-7.96777','89.7637','2.65555','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+115,'299','0','6170','9','0','0','50001','0','-1','0','0','-8877.3','24.2208','96.3584','2.14343','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+116,'299','0','6170','9','0','0','50001','0','-1','0','0','-8912.79','-0.932722','101.855','4.90448','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+117,'299','0','6170','9','0','0','50001','0','-1','0','0','-8888.54','-31.537','88.9617','5.27581','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+118,'299','0','6170','9','0','0','50001','0','-1','0','0','-8744.92','-221.37','88.656','1.78414','300','10','0','100','1',NULL,NULL,NULL,NULL,'',NULL,'0'),
(@CGUID+119, '823', '0', '6170', '9', '0', '0', '50001', '0', '-1', '0', '1', '-8936.3', '-131.629', '82.2113', '5.90552', '300', '0', '0', '100', '0', NULL, '33280', '34816', NULL, '', NULL, '0'),
(@CGUID+120, '951', '0', '6170', '24', '0', '0', '50001', '0', '-1', '0', '1', '-8879.75', '-182.926', '81.9399', '3.71863', '300', '0', '0', '100', '0', NULL, '32768', NULL, NULL, '', NULL, '0'),
(@CGUID+121, '9296', '0', '6170', '9', '0', '0', '50001', '0', '-1', '0', '0', '-8852.242', '-214.983', '81.04672', '6.216281', '120', '0', '0', '100', '0', NULL, NULL, NULL, NULL, '', NULL, '0'),
(@CGUID+122, '9296', '0', '6170', '59', '0', '0', '50003', '0', '-1', '0', '0', '-9004.3', '-308.414', '72.7995', '3.579', '300', '0', '0', '100', '0', NULL, '33280', NULL, NULL, '', NULL, '0'),
(@CGUID+123, '823', '0', '6170', '34', '0', '0', '50003', '0', '-1', '0', '1', '-8681.73', '-117.491', '90.5722', '5.92986', '300', '0', '0', '100', '0', NULL, '33280', '34816', NULL, '', NULL, '0'),
(@CGUID+124, '951', '0', '6170', '24', '0', '0', '50003', '0', '-1', '0', '1', '-8879.75', '-182.926', '81.9399', '3.71863', '300', '0', '0', '100', '0', NULL, '32768', NULL, NULL, '', NULL, '0');

-- creature addons
DELETE FROM `creature_addon` WHERE `guid` IN (900038, 900039, @CGUID+119, @CGUID+123);
INSERT INTO `creature_addon` (`guid`, `StandState`) VALUES ('900038', '8');
INSERT INTO `creature_addon` (`guid`, `StandState`) VALUES ('900039', '8');
INSERT INTO `creature_addon` (`guid`, `auras`) VALUES (@CGUID+119, '');
INSERT INTO `creature_addon` (`guid`, `auras`) VALUES (@CGUID+123, '');

-- creature scripts
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_custom_stormwind_infantry' WHERE `Entry` = 49869; -- Stormwind Infantry
UPDATE `creature_template` SET `AIName` = '' WHERE `Entry` IN (823, 951, 9296, 49871);

UPDATE `creature` SET `ScriptName` = 'npc_custom_brother_paxton' WHERE `guid` = 279895;


-- remove smart scripts
DELETE FROM `smart_scripts` WHERE `entryorguid` = 49869; -- Stormwind Infantry
DELETE FROM `smart_scripts` WHERE  `entryorguid` IN (6, 38, 62, 80, 103, 257, 299, 823, 951, 9296, 50039, 50047, 49874, 49871, 49869, 42937, 42938, 42940) AND `id` IN (10000, 10001, 11000, 11001, 11002, 11003, 11004);
UPDATE `smart_scripts` SET `event_param3` = '600000', `event_param4` = '1200000' WHERE (`entryorguid` = '62') and (`source_type` = '0') and (`id` = '0') and (`link` = '0'); -- Gug Fatcandle increase delay for frost armor

-- creature text
UPDATE `creature_text` SET `Text` = 'Time to join your friends, kissin\' the dirt!', `Probability` = '100' WHERE (`CreatureID` = '50039') and (`GroupID` = '0') and (`ID` = '1');
DELETE FROM `creature_text` WHERE (`CreatureID` = '50039') and (`GroupID` = '1') and (`ID` = '0');
DELETE FROM `creature_text` WHERE (`CreatureID` = '42937') and (`GroupID` = '1') and (`ID` = '0');
DELETE FROM `creature_text` WHERE (`CreatureID` = '42937') and (`GroupID` = '2') and (`ID` = '0');
DELETE FROM `creature_text` WHERE (`CreatureID` = '42937') and (`GroupID` = '3') and (`ID` = '0');
UPDATE `creature_text` SET `Probability` = '100' WHERE (`CreatureID` = '42937') and (`GroupID` = '0') and (`ID` = '4');

-- Added an extra line for Garrick Padfoot
DELETE FROM `creature_text` WHERE `CreatureID` = 103 AND `GroupID` = 1;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `SoundPlayType`, `BroadcastTextId`, `TextRange`, `comment`) VALUES ('103', '1', '0', 'Help me out! Take this dog off me... NOW!', '12', '7', '100', '0', '0', '0', '0', '0', '0', 'Garrick Padfoot asking for help');

-- Extended scripts with the extra line of text and get 2 thugs to attack player
DELETE FROM `smart_scripts` WHERE `entryorguid` = 103;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`action_param7`,`action_param_string`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_param_string`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES (103,0,0,1,'',4,0,100,0,0,0,0,0,0,'',11,7164,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Garrick Padfoot - On Aggro - Cast \'Defensive Stance\' (No Repeat)');
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`action_param7`,`action_param_string`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_param_string`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES (103,0,1,2,'',61,0,100,0,0,0,0,0,0,'',1,0,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Garrick Padfoot - On Aggro - Say Line 0 (No Repeat)');
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`action_param7`,`action_param_string`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_param_string`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES (103,0,2,3,'',61,0,100,0,0,0,0,0,0,'',1,1,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Garrick talks on aggro and triggers Mob 2 and 3');
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`action_param7`,`action_param_string`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_param_string`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES (103,0,3,0,'',61,0,100,0,0,0,0,0,0,'',2,14,0,0,0,0,0,0,NULL,9,38,1,20,2,NULL,0,0,0,0,'Garrick Padfoot - calls for help');

-- Northshire Peasant text and scripts to change from work anim on/off
DELETE FROM `creature_text` WHERE `CreatureID`=11260;
INSERT INTO creature_text (CreatureID, groupid, id, text, type, language, probability, emote, duration, sound, BroadcastTextId, TextRange, comment) VALUES
(11260, 0, 1, 'Aaah my foot!', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - reaction'),
(11260, 0, 2, 'Alright one more swing...', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - agreement'),
(11260, 0, 3, 'I guess I can do this!', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - reluctant acceptance'),
(11260, 0, 4, 'If you want', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - neutral consent'),
(11260, 0, 5, 'More work?', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - surprise'),
(11260, 0, 6, 'No one else available', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - complaint'),
(11260, 0, 7, 'No, not this again!', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - refusal'),
(11260, 0, 8, 'Off I go then', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - task accepted'),
(11260, 0, 9, 'Ready to work!', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - cheerful start'),
(11260, 0, 10, 'Right-o', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - quick agreement'),
(11260, 0, 11, 'What is it?', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - curious inquiry'),
(11260, 0, 12, 'What?', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - confused response');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE (`entry` = '11260');
DELETE FROM smart_scripts WHERE entryorguid = 11260 AND source_type = 0;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`action_param7`,`action_param_string`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_param_string`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES (11260,0,0,0,'',60,0,100,0,50000,150000,50000,150000,0,'',5,173,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Northshire Peasant - periodic working animation');
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`action_param7`,`action_param_string`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_param_string`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES (11260,0,1,0,'',60,0,100,0,50000,150000,50000,150000,0,'',1,0,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Northshire Peasant - periodic talk');
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`action_param7`,`action_param_string`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_param_string`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES (11260,0,2,0,'',60,0,100,0,1000,5000,50000,150000,0,'',5,12,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Northshire Peasant - periodic no animation');
