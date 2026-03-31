-- ============================================================
-- Shadow Enclave Delve - Database Setup
-- Map 2952, Scenario 3154, Difficulty 208
-- ============================================================

-- Instance template for Shadow Enclave Delve
INSERT INTO instance_template (map, parent, script)
VALUES (2952, 0, 'instance_shadow_enclave_delve')
ON DUPLICATE KEY UPDATE script='instance_shadow_enclave_delve';

-- Scenario mapping (map + difficulty → scenario ID)
DELETE FROM scenarios WHERE map = 2952 AND difficulty = 208;
INSERT INTO scenarios (map, difficulty, scenario_A, scenario_H) VALUES (2952, 208, 3154, 3154);

-- Lord Antenorian (Final boss - Step 4)
UPDATE creature_template SET ScriptName = 'npc_lord_antenorian' WHERE entry = 246717;

-- Darkcaller Lysaille (Ritual - Step 2)
UPDATE creature_template SET ScriptName = 'npc_darkcaller' WHERE entry = 250242;

-- Darkcaller Cimberon (Ritual - Step 2)
UPDATE creature_template SET ScriptName = 'npc_darkcaller' WHERE entry = 251899;

-- Darkcaller Thelamorn (Ritual - Step 2)
UPDATE creature_template SET ScriptName = 'npc_darkcaller' WHERE entry = 251898;

-- Void Focus (Step 1 - destroy to advance)
UPDATE creature_template SET ScriptName = 'npc_void_focus_se' WHERE entry = 250266;

-- Valeera Sanguinar (Companion NPC - trait config UI)
UPDATE creature_template SET ScriptName = 'npc_valeera_companion', npcflag = npcflag | 1 WHERE entry = 248567;

UPDATE creature_template_difficulty SET ContentTuningID = 4898 WHERE entry IN (
    246717,  -- Lord Antenorian
    250242,  -- Darkcaller Lysaille
    251899,  -- Darkcaller Cimberon
    251898,  -- Darkcaller Thelamorn
    250266,  -- Void Focus
    250275,  -- Antenorian's Devoted
    246352,  -- Bladesworn Cultist
    246348,  -- Shadowspawn
    246357,  -- Shadowburn Vortex
    257834   -- Eye of Antenorian
);

-- Gossip menu addon for Shadow Enclave — LfgDungeonsID drives the tier-selection UI
DELETE FROM gossip_menu_addon WHERE MenuID = 40277;
INSERT INTO gossip_menu_addon (MenuID, FriendshipFactionID, LfgDungeonsID) VALUES (40277, 0, 3069);

-- Lord Antenorian death yell
DELETE FROM creature_text WHERE CreatureID = 246717;
INSERT INTO creature_text (CreatureID, GroupID, ID, TEXT, TYPE, LANGUAGE, Probability, Emote, Duration, Sound, BroadcastTextId, TextRange, COMMENT) VALUES
(246717, 0, 0, 'The shadows... will consume... everything...', 14, 0, 100, 0, 0, 0, 0, 0, 'Lord Antenorian - Death Yell');