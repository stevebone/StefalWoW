DELETE FROM `creature_template_gossip` WHERE `MenuID` >= 900000; 
INSERT INTO `creature_template_gossip` VALUES 
('198', '900000', '-1'),
('198', '900001', '-1'),
('198', '900002', '-1'),
('198', '900003', '-1'),
('198', '900004', '-1'),
('198', '900005', '-1'),
('141508', '900000', '-1'),
('141508', '900001', '-1'),
('141508', '900002', '-1'),
('141508', '900003', '-1'),
('141508', '900004', '-1'),
('141508', '900005', '-1'),
('375', '900000', '-1'),
('375', '900001', '-1'),
('375', '900002', '-1'),
('375', '900003', '-1'),
('375', '900004', '-1'),
('375', '900005', '-1');


UPDATE `creature_template` SET `ScriptName` = 'npc_followship_bots' WHERE `Entry` = 141508;
UPDATE `creature_template` SET `ScriptName` = 'npc_followship_bots' WHERE `Entry` = 375;
UPDATE `creature_template` SET `ScriptName` = 'npc_followship_bots' WHERE `Entry` = 198;