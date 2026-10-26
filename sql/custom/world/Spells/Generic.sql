-- ============================================================
-- Generic spell scripts SQL for TrinityCore Master (Retail)
-- Apply to 'world' database unless noted otherwise
-- ============================================================

-- Register spell scripts
DELETE FROM `spell_script_names` WHERE `spell_id` = 151597;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(151597, 'spell_gen_no_npc_damage_below_override_45');
