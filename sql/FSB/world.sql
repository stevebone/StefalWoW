DELETE FROM `gossip_menu` WHERE `MenuID` >= 900000;
INSERT INTO `gossip_menu` VALUES
('900000', '1', '-1'),
('900001', '1', '-1'),
('900002', '1', '-1'),
('900003', '1', '-1'),
('900004', '1', '-1'),
('900005', '1', '-1'),
('900006', '1', '-1');

DELETE FROM `creature_template_gossip` WHERE `MenuID` >= 900000; 
INSERT INTO `creature_template_gossip` VALUES 
('198', '900000', '-1'),
('198', '900001', '-1'),
('198', '900002', '-1'),
('198', '900003', '-1'),
('198', '900004', '-1'),
('198', '900005', '-1'),
('198', '900006', '-1'),
('375', '900000', '-1'),
('375', '900001', '-1'),
('375', '900002', '-1'),
('375', '900003', '-1'),
('375', '900004', '-1'),
('375', '900005', '-1'),
('459', '900000', '-1'),
('459', '900001', '-1'),
('459', '900002', '-1'),
('459', '900003', '-1'),
('459', '900004', '-1'),
('459', '900005', '-1'),
('911', '900000', '-1'),
('911', '900001', '-1'),
('911', '900002', '-1'),
('911', '900003', '-1'),
('911', '900004', '-1'),
('911', '900005', '-1'),
('925', '900000', '-1'),
('925', '900001', '-1'),
('925', '900002', '-1'),
('925', '900003', '-1'),
('925', '900004', '-1'),
('925', '900005', '-1'),
('3593', '900000', '-1'),
('3593', '900001', '-1'),
('3593', '900002', '-1'),
('3593', '900003', '-1'),
('3593', '900004', '-1'),
('3593', '900005', '-1'),
('3595', '900000', '-1'),
('3595', '900001', '-1'),
('3595', '900002', '-1'),
('3595', '900003', '-1'),
('3595', '900004', '-1'),
('3595', '900005', '-1'),
('16500', '900000', '-1'),
('16500', '900001', '-1'),
('16500', '900002', '-1'),
('16500', '900003', '-1'),
('16500', '900004', '-1'),
('16500', '900005', '-1'),
('16500', '900006', '-1'),
('16501', '900000', '-1'),
('16501', '900001', '-1'),
('16501', '900002', '-1'),
('16501', '900003', '-1'),
('16501', '900004', '-1'),
('16501', '900005', '-1'),
('16502', '900000', '-1'),
('16502', '900001', '-1'),
('16502', '900002', '-1'),
('16502', '900003', '-1'),
('16502', '900004', '-1'),
('16502', '900005', '-1'),
('16503', '900000', '-1'),
('16503', '900001', '-1'),
('16503', '900002', '-1'),
('16503', '900003', '-1'),
('16503', '900004', '-1'),
('16503', '900005', '-1'),
('43006', '900000', '-1'),
('43006', '900001', '-1'),
('43006', '900002', '-1'),
('43006', '900003', '-1'),
('43006', '900004', '-1'),
('43006', '900005', '-1'),
('43006', '900006', '-1'),
('46405', '900000', '-1'),
('46405', '900001', '-1'),
('46405', '900002', '-1'),
('46405', '900003', '-1'),
('46405', '900004', '-1'),
('46405', '900005', '-1'),
('50595', '900000', '-1'),
('50595', '900001', '-1'),
('50595', '900002', '-1'),
('50595', '900003', '-1'),
('50595', '900004', '-1'),
('50595', '900005', '-1'),
('141508', '900000', '-1'),
('141508', '900001', '-1'),
('141508', '900002', '-1'),
('141508', '900003', '-1'),
('141508', '900004', '-1'),
('141508', '900005', '-1');

-- Northshire trainers
UPDATE `creature_template` SET `ScriptName` = 'npc_followship_bots' WHERE `Entry` = 198; -- Mage
UPDATE `creature_template` SET `ScriptName` = 'npc_followship_bots' WHERE `Entry` = 375; -- Priest
UPDATE `creature_template` SET `ScriptName` = 'npc_followship_bots' WHERE `Entry` = 459; -- Drusilla la Salle (Warlock)
UPDATE `creature_template` SET `ScriptName` = 'npc_followship_bots' WHERE `Entry` = 911; -- Warrior
UPDATE `creature_template` SET `ScriptName` = 'npc_followship_bots' WHERE `Entry` = 925; -- Brother Sammuel (Paladin)

UPDATE `creature_template` SET `ScriptName` = 'npc_followship_bots' WHERE `Entry` = 141508; -- Stormwind Priest

UPDATE `creature_template` SET `ScriptName` = 'npc_followship_bots' WHERE `Entry` = 50595; -- Stormwind Defender (Warrior)
UPDATE `creature_template` SET `ScriptName` = 'npc_followship_bots' WHERE `Entry` = 46405; -- Stockade Guard (Warrior)

-- Ammen Vale trainers
UPDATE `creature_template` SET `ScriptName` = 'npc_followship_bots' WHERE `Entry` = 16500; -- Valaatu (Mage)
UPDATE `creature_template` SET `ScriptName` = 'npc_followship_bots' WHERE `Entry` = 16501; -- Aurelon (Paladin)
UPDATE `creature_template` SET `ScriptName` = 'npc_followship_bots' WHERE `Entry` = 16502; -- Zalduun (Priest)
UPDATE `creature_template` SET `ScriptName` = 'npc_followship_bots' WHERE `Entry` = 16503; -- Kore (Warrior)

-- Shadowglen trainers
UPDATE `creature_template` SET `ScriptName` = 'npc_followship_bots' WHERE `Entry` = 3593; -- Alyissia (Warrior)
UPDATE `creature_template` SET `ScriptName` = 'npc_followship_bots' WHERE `Entry` = 3595; -- Shanda (Priest)
UPDATE `creature_template` SET `ScriptName` = 'npc_followship_bots' WHERE `Entry` = 43006; -- Rhyanda (Mage)