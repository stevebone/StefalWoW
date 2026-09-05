-- Trading Post (Perks)

-- NPC: 185467 Wilder Seabraid (Stormwind)
-- NPC: 185468 Tawny Seabraid (Stormwind)
-- NPC: 219243 Teha (Dornogal)
-- NPC: 219244 Andee Seabraid (Dornogal)
-- NPC: 249042 Zalani
-- NPC: 185473 Zen'kala
-- NPC: 185472 Shiri

-- Quest: 66858 Tour the Trading Post

INSERT IGNORE INTO `creature_questender` VALUES
(185468, 66858, 0),
(219244, 66858, 0);

UPDATE `creature_template` SET `ScriptName` = '' WHERE `ScriptName` = 'npc_perks_program_vendor'; -- cleanup
