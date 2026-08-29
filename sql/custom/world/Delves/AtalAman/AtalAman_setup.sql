-- ============================================================
-- Atal'Aman Delve - Database Setup
-- ============================================================

-- instance_template: Register Map 2962 with our InstanceScript
INSERT INTO instance_template (map, parent, script)
VALUES (2962, 0, 'instance_atal_aman_delve')
ON DUPLICATE KEY UPDATE script = 'instance_atal_aman_delve';

-- creature_template: Assign ScriptName to the Enter Delve NPC
UPDATE creature_template SET ScriptName = 'npc_enter_delve' WHERE entry = 212407;

-- gameobject_template: Assign ScriptName to the Leave Delve portal
UPDATE gameobject_template SET ScriptName = 'go_leave_delve' WHERE entry = 408227;

-- spell_script_names: Register the Delve entry teleport spell
INSERT INTO spell_script_names (spell_id, ScriptName)
VALUES (1260942, 'spell_delve_entry')
ON DUPLICATE KEY UPDATE ScriptName = 'spell_delve_entry';

-- gossip_menu_addon: LfgDungeonsID drives the tier-selection UI (from 12.0.1 sniff)
DELETE FROM gossip_menu_addon WHERE MenuID = 39751;
INSERT INTO gossip_menu_addon (MenuID, FriendshipFactionID, LfgDungeonsID) VALUES (39751, 0, 3025);

-- Shadow Enclave gossip_menu_addon
DELETE FROM gossip_menu_addon WHERE MenuID = 40277;
INSERT INTO gossip_menu_addon (MenuID, FriendshipFactionID, LfgDungeonsID) VALUES (40277, 0, 3069);

-- Delve Entry NPC Gossip
DELETE FROM `gossip_menu` WHERE `MenuID` IN (39751);
INSERT INTO `gossip_menu` (`MenuID`, `TextID`, `VerifiedBuild`) VALUES ('39751', '16777378', '66527');
DELETE FROM `creature_template_gossip` WHERE `CreatureID` IN (212407);
INSERT INTO `creature_template_gossip` (`CreatureID`, `MenuID`, `VerifiedBuild`) VALUES ('212407', '39751', '66527');

DELETE FROM `gossip_menu_option` WHERE `MenuID` IN (39751);
INSERT INTO `gossip_menu_option` VALUES
('39751', '134444', '0', '0', 'Tier 1', '0', '0', '0', '0', '0', NULL, '0', '0', NULL, '0', '1260938', NULL, '66527'),
('39751', '134443', '1', '0', 'Tier 2', '0', '0', '0', '0', '0', NULL, '0', '0', NULL, '0', '1260942', NULL, '66527'),
('39751', '134442', '2', '0', 'Tier 3', '0', '0', '0', '0', '0', NULL, '0', '0', NULL, '0', '1260946', NULL, '66527'),
('39751', '134441', '3', '0', 'Tier 4', '0', '0', '0', '0', '0', NULL, '0', '0', NULL, '0', '1260950', NULL, '66527'),
('39751', '134440', '4', '0', 'Tier 5', '0', '0', '0', '0', '0', NULL, '0', '0', NULL, '0', '1260954', NULL, '66527'),
('39751', '134439', '5', '0', 'Tier 6', '0', '0', '0', '0', '0', NULL, '0', '0', NULL, '0', '1260957', NULL, '66527'),
('39751', '134438', '6', '0', 'Tier 7', '0', '0', '0', '0', '0', NULL, '0', '0', NULL, '0', '1260960', NULL, '66527'),
('39751', '134437', '7', '0', 'Tier 8', '0', '0', '0', '0', '0', NULL, '0', '0', NULL, '0', '1260963', NULL, '66527'),
('39751', '134436', '8', '0', 'Tier 9', '0', '0', '0', '0', '0', NULL, '0', '0', NULL, '0', '1260967', NULL, '66527'),
('39751', '134435', '9', '0', 'Tier 10', '0', '0', '0', '0', '0', NULL, '0', '0', NULL, '0', '1260970', NULL, '66527'),
('39751', '134434', '10', '0', 'Tier 11', '0', '0', '0', '0', '0', NULL, '0', '0', NULL, '0', '1260973', NULL, '66527');

DELETE FROM `npc_text` WHERE `id` IN (16777378);
INSERT INTO `npc_text` VALUES
('16777378', '1', '0', '0', '0', '0', '0', '0', '0', '292789', '0', '0', '0', '0', '0', '0', '0', '66527');


-- ============================================================
-- Shared Delve Objects (all delves)
-- ============================================================

-- Delve Campfire (GO 618844) — SpellCaster, spell 1272119 (heal + well-fed buff)
UPDATE gameobject_template SET ScriptName = 'go_delve_campfire' WHERE entry = 618844;

-- Delvers' Supplies (NPC 207283) — vendor/repair/companion
UPDATE creature_template SET ScriptName = 'npc_delvers_supplies' WHERE entry = 207283;
