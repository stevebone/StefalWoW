-- Quest: 26209 Murder Was The Case That They Gave Me
-- NPC: 42383 Transient
-- NPC: 42384 Homeless Stormwind Citizen
-- NPC: 42386 Homeless Stormwind Citizen (Female)
-- NPC: 42391 West Plains Drifter

UPDATE `creature_template` SET `ScriptName` = 'npc_westplains_drifter' WHERE `entry` = 42383;
UPDATE `creature_template` SET `ScriptName` = 'npc_westplains_drifter' WHERE `entry` = 42384;
UPDATE `creature_template` SET `ScriptName` = 'npc_westplains_drifter' WHERE `entry` = 42386;
UPDATE `creature_template` SET `ScriptName` = 'npc_westplains_drifter' WHERE `entry` = 42391;

UPDATE `creature_template` SET `npcflag` = '1' WHERE (`entry` = '42383');
UPDATE `creature_template` SET `npcflag` = '1' WHERE (`entry` = '42384');
UPDATE `creature_template` SET `npcflag` = '1' WHERE (`entry` = '42386');
UPDATE `creature_template` SET `npcflag` = '1' WHERE (`entry` = '42391');

INSERT IGNORE INTO `creature_template_gossip` (`CreatureID`, `MenuID`, `VerifiedBuild`) VALUES ('42386', '11635', '0');
INSERT IGNORE INTO `creature_template_gossip` (`CreatureID`, `MenuID`, `VerifiedBuild`) VALUES ('42391', '11635', '0');