-- Quest: 29663 The Lesson of the Balanced Rock
-- NPC: 55019 Tushui Monk
-- NPC: 65468 Tushui Monk

UPDATE `creature_template` SET `ScriptName` = 'npc_tushui_monk_on_pole' WHERE (`entry` = '55019');
UPDATE `creature_template` SET `ScriptName` = 'npc_tushui_monk_on_pole', `KillCredit1` = '55019' WHERE (`entry` = '65468');
