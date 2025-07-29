-- Quest: 29661 The Lesson of Dry Fur
-- NPC: 54993 Balance Pole
-- NPC: 57431 Balance Pole
-- NPC: 55083 Balance Pole
-- NPC: 56869 Bunny landing impact

DELETE FROM `areatrigger_scripts` WHERE `entry` IN (6986, 6987, 8628);
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES ('6986', 'at_singing_pools_transform');
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES ('6987', 'at_singing_pools_transform');
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES ('8628', 'at_singing_pools_training_bell');

UPDATE `creature_template` SET `ScriptName` = 'npc_balance_pole' WHERE `Entry` IN (54993, 57431, 55083);

DELETE FROM `vehicle_template` WHERE `creatureId` IN (54993, 57431, 55083, 56869);
INSERT INTO `vehicle_template` (`creatureId`, `despawnDelayMs`, `CustomFlags`) VALUES ('54993', '0', '0');
INSERT INTO `vehicle_template` (`creatureId`, `despawnDelayMs`, `CustomFlags`) VALUES ('57431', '0', '0');
INSERT INTO `vehicle_template` (`creatureId`, `despawnDelayMs`, `CustomFlags`) VALUES ('55083', '0', '0');
INSERT INTO `vehicle_template` (`creatureId`, `despawnDelayMs`, `CustomFlags`) VALUES ('56869', '0', '0');

UPDATE `creature_template` SET `unit_flags3` = '524288' WHERE (`entry` = '54993');
UPDATE `creature_template` SET `unit_flags3` = '524288' WHERE (`entry` = '55083');
UPDATE `creature_template` SET `unit_flags3` = '524288' WHERE (`entry` = '57431');


UPDATE `creature_template` SET `npcflag` = '50331648' WHERE (`entry` = '54993');
UPDATE `creature_template` SET `npcflag` = '50331648' WHERE (`entry` = '55083');
UPDATE `creature_template` SET `npcflag` = '50331648' WHERE (`entry` = '57431');

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN (55083, 57431,54993);
INSERT INTO `npc_spellclick_spells` VALUES
(54993, 102717, 1, 0),
(55083, 102717, 1, 0),
(57431, 102717, 1, 0);

DELETE FROM `conditions` WHERE `SourceEntry` = 107049;
insert  into `conditions`(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) values 
(13,1,107049,0,0,51,0,5,56869,0,0,0,0,'','Ride Vehicle target Balance Pole Landing Bunny'),
(17,0,107049,0,0,1,0,133381,0,0,1,30,0,'','Ride Vehicle when player has not aura'),
(17,0,107049,0,0,29,0,56869,8,0,0,30,0,'','Ride Vehicle when bunny within 8y'),
(13,1,107049,0,0,31,0,5,56869,0,0,0,0,'','Ride Vehicle target Balance Pole Landing Bunny');