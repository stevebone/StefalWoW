-- Tirisfal Glades Deathknell changes and updates

-- NPC: 41200 Deathknell Gate bunny
-- NPC: 49337 Darnell
-- NPC: 49340 Scarlet Corpse

-- Quest: 26800 Recruitment

-- Creature Templates
UPDATE `creature_template` SET `npcflag` = '1' WHERE (`entry` = '49340');
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_custom_scarlet_corpse' WHERE (`entry` = '49340');
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_custom_darnell' WHERE (`entry` = '49337');

