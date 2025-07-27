-- Quest: 29422 Huo, the Spirit of Fire
-- NPC: 57779 Huo
-- NPC: 54787 Huo (spawned)
-- Spell: 102522 Fan The Flames

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('102522', 'spell_fan_the_flames_script');
DELETE FROM `creature` WHERE `id` = 54787;