-- ============================================================
-- Atal'Aman Delve - Database Setup
-- ============================================================

-- instance_template: Register Map 2962 with our InstanceScript
INSERT INTO instance_template (map, parent, script, allowMount)
VALUES (2962, 0, 'instance_atal_aman_delve', 0)
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

-- ============================================================
-- Shared Delve Objects (all delves)
-- ============================================================

-- Delve Campfire (GO 618844) — SpellCaster, spell 1272119 (heal + well-fed buff)
UPDATE gameobject_template SET ScriptName = 'go_delve_campfire' WHERE entry = 618844;

-- Delvers' Supplies (NPC 207283) — vendor/repair/companion
UPDATE creature_template SET ScriptName = 'npc_delvers_supplies' WHERE entry = 207283;
