

-- Instance Templates
INSERT INTO instance_template (map, parent, script) VALUES 
(2962, 0, 'instance_atal_aman_delve'),
(2952, 0, 'instance_shadow_enclave_delve')
ON DUPLICATE KEY UPDATE script=VALUES(script);

-- Scenarios (map + difficulty → scenario ID)
DELETE FROM scenarios WHERE (map = 2962 AND difficulty = 208) OR (map = 2952 AND difficulty = 208);
INSERT INTO scenarios (map, difficulty, scenario_A, scenario_H) VALUES 
(2962, 208, 3147, 3147),
(2952, 208, 3154, 3154);

-- Creature Templates (ScriptNames, Flags, VehicleIds, etc.)
-- Delve Handler
UPDATE creature_template SET ScriptName = 'npc_enter_delve', npcflag = npcflag | 1 WHERE entry = 212407;
-- Perks Vendor
UPDATE creature_template SET ScriptName = 'npc_perks_program_vendor', npcflag = npcflag | 1 WHERE entry = 249042;
-- Atal'Aman Delve
UPDATE creature_template SET ScriptName = 'npc_fleek' WHERE entry = 252930;
UPDATE creature_template SET ScriptName = 'npc_hexbound_ritualist' WHERE entry = 253321;
UPDATE creature_template SET ScriptName = 'npc_consumption_of_nalorakk' WHERE entry = 247106;
UPDATE creature_template SET ScriptName = 'npc_spiritflayer_jinma' WHERE entry = 258372;
UPDATE creature_template SET ScriptName = 'npc_leave_o_bot', npcflag = 16777216, VehicleId = 8163 WHERE entry = 205496;
-- Shadow Enclave Delve
UPDATE creature_template SET ScriptName = 'npc_lord_antenorian' WHERE entry = 246717;
UPDATE creature_template SET ScriptName = 'npc_darkcaller' WHERE entry IN (250242, 251899, 251898);
UPDATE creature_template SET ScriptName = 'npc_void_focus_se' WHERE entry = 250266;
UPDATE creature_template SET ScriptName = 'npc_valeera_companion', npcflag = npcflag | 1 WHERE entry = 248567;

-- Content Tuning (Delve scaling)
UPDATE creature_template_difficulty SET ContentTuningID = 4898 WHERE entry IN (
    258372, 252930, 253321, 247106, 252948, -- Atal'Aman
    246717, 250242, 251899, 251898, 250266, 250275, 246352, 246348, 246357, 257834 -- Shadow Enclave
);

-- GameObject Templates
-- Collector's Cache
UPDATE gameobject_template SET ScriptName = 'go_perks_collectors_cache' WHERE entry = 565101;
-- Leave Delve portal
UPDATE gameobject_template SET ScriptName = 'go_leave_delve' WHERE entry = 408227;
-- Spiritpaw Cage (Atal'Aman)
UPDATE gameobject_template SET `type` = 10, `IconName` = 'questinteract', `castBarCaption` = 'Unlocking', `Data0` = 4210, `Data3` = 2147483647, `Data10` = 1257535, `Data22` = 24815, `Data27` = 1, `Data30` = 1, ScriptName = 'go_spiritpaw_cage' WHERE entry = 579108;

-- Creature Text
DELETE FROM creature_text WHERE CreatureID IN (258372, 247106, 252948, 246831, 246717);
INSERT INTO creature_text (CreatureID, GroupID, ID, TEXT, TYPE, LANGUAGE, Probability, Emote, Duration, Sound, BroadcastTextId, TextRange, COMMENT) VALUES
(258372, 0, 0, 'Show your face, $p. I make you pay for meddling!', 14, 0, 100, 0, 0, 0, 0, 0, 'Spiritflayer Jin''Ma - Spawn Yell'),
(247106, 0, 0, 'You disrupt de ritual, now you be de new sacrifice!', 14, 0, 100, 0, 0, 0, 0, 0, 'Consumption of Nalorakk - Aggro Yell'),
(252948, 0, 0, 'Me....run!', 12, 0, 100, 0, 0, 221108, 302833, 0, 'Spiritpaw Cubs - Freed 1'),
(252948, 0, 1, 'Rrrr... free!', 12, 0, 100, 0, 0, 221109, 302834, 0, 'Spiritpaw Cubs - Freed 2'),
(252948, 0, 2, 'Friend...yes!', 12, 0, 100, 0, 0, 221107, 302832, 0, 'Spiritpaw Cubs - Freed 3'),
(246831, 0, 0, 'De ritual must be completed!', 14, 0, 100, 0, 0, 0, 0, 0, 'Necrohex Soulbinder - Ritual Interrupted'),
(246717, 0, 0, 'The shadows... will consume... everything...', 14, 0, 100, 0, 0, 0, 0, 0, 'Lord Antenorian - Death Yell');

-- Gossip Menu Addon (LfgDungeonsID drives UI)
DELETE FROM gossip_menu_addon WHERE MenuID IN (39751, 40277);
INSERT INTO gossip_menu_addon (MenuID, FriendshipFactionID, LfgDungeonsID) VALUES 
(39751, 0, 3025), -- Atal'Aman
(40277, 0, 3069); -- Shadow Enclave

-- NPC SpellClick Spells
DELETE FROM npc_spellclick_spells WHERE npc_entry = 205496;
INSERT INTO npc_spellclick_spells (npc_entry, spell_id, cast_flags, user_type) VALUES (205496, 411497, 0, 0);

-- Creature Template Addon (Visuals/Anim)
DELETE FROM creature_template_addon WHERE entry = 205496;
INSERT INTO creature_template_addon (entry, PathId, mount, StandState, AnimTier, VisFlags, SheathState, PvpFlags, emote, aiAnimKit, movementAnimKit, meleeAnimKit, visibilityDistanceType, auras) VALUES
(205496, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, '');

-- GameObject Template Addon
DELETE FROM gameobject_template_addon WHERE entry = 579108;
INSERT INTO gameobject_template_addon (entry, faction, flags, WorldEffectID, AIAnimKitID) VALUES (579108, 0, 0x40000, 0, 0);

-- Spell Script Names
INSERT INTO spell_script_names (spell_id, ScriptName) VALUES (1260942, 'spell_delve_entry')
ON DUPLICATE KEY UPDATE ScriptName=VALUES(ScriptName);







