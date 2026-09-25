-- ----------------------------
-- QUEST - 37444
-- ----------------------------
REPLACE INTO `item_script_names` VALUES (22962, 'item_inoculating_crystal');
UPDATE creature_template  SET AIName = '', ScriptName = 'npc_mutated_owlkin' WHERE entry = 16537;