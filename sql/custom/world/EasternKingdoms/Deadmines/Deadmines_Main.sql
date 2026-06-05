-- Deadmines Dungeon
-- Modern version after Cataclysm

-- NPC: 47162 Glubtok
-- NPC: 48974 Glubtok Main Platter
-- NPC: 48230 Ogre Henchman
-- NPC: 48266 Defias Cannon
-- NPC: 50595 Stormwind Defender
-- NPC: 46890 Shattered Hand Assassin
-- NPC: 46889 Kagtha
-- NPC: 46902 Miss Mayhem
-- NPC: 48284 Mining Powder

-- GO: 208002 Goblin Teleporter

-- Creature Template Updates
UPDATE `creature_template` SET `unit_flags` = 0 WHERE `entry` = 48284; -- remove uninterract flag for mining powder so it can be attacked

-- Creature Difficulties
-- Stormwind Defender and Shattered Hand Assassin have diff id 0 added for Followship Bots since they can be taken outside the dungeon
DELETE FROM `creature_template_difficulty` WHERE `entry` IN (47162,48229,48230,50595,46890,46903,46902,46889,46906,48262,48284);
INSERT INTO `creature_template_difficulty` VALUES
(48284, 1, 	-1, -1, 202, 1, 1, 1, 1, 1, 41616, 0, 0, 0, 0, 0, 0, 0, 0, 536871168, 0, 0, 0, 0, 0, 0, 0, -1),
(48284, 2, 	0, 0, 1199, 1, 1, 1, 1, 1, 41617, 0, 0, 0, 0, 0, 0, 0, 0, 536871168, 0, 0, 0, 0, 0, 0, 0, -1),
(48284, 24, 0, 0, 2872, 3, 1, 1, 1, 1, 41617, 0, 0, 0, 0, 0, 0, 0, 0, 536871168, 0, 0, 0, 0, 0, 0, 0, -1), -- Mining Powder
(46906, 1, 0, 0, 202, 0, 1, 1, 1, 1, 44620, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(46906, 2, 0, 0, 1199, 0, 1, 1, 1, 1, 44620, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(46906, 24, 0, 0, 2872, 0, 1, 1, 1, 1, 44620, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Slinky Sharpshiv
(46889, 1, 0, 0, 202, 0, 1, 1, 1, 1, 44655, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(46889, 2, 0, 0, 1199, 0, 1, 1, 1, 1, 44655, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(46889, 24, 0, 0, 2872, 0, 1, 1, 1, 1, 44655, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Kagtha
(46902, 1, 0, 0, 202, 0, 1, 1, 1, 1, 44629, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(46902, 2, 0, 0, 1199, 0, 1, 1, 1, 1, 44629, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(46902, 24, 0, 0, 2872, 0, 1, 1, 1, 1, 44629, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Miss Mayhem
(46903, 1, 0, 0, 202, 0, 1, 1, 1, 1, 44628, 32768, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(46903, 2, 0, 0, 1199, 0, 1, 1, 1, 1, 44628, 32768, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(46903, 24, 0, 0, 2872, 0, 1, 1, 1, 1, 44628, 32768, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), -- Mayhem Prototype
(46890, 0, 0, 0, 202, 0, 1, 1, 1, 1, 44653, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(46890, 1, 0, 0, 202, 0, 1, 1, 1, 1, 44653, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(46890, 2, 0, 0, 1199, 0, 1, 1, 1, 1, 44653, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1),
(46890, 24, 0, 0, 2872, 0, 1, 1, 1, 1, 44653, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Shattered Hand Assassin
(50595, 0, 0, 0, 202, 0, 1, 1, 1, 1, 37807, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45338),
(50595, 1, 0, 0, 202, 0, 1, 1, 1, 1, 37807, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45338),
(50595, 2, 0, 0, 1199, 0, 1, 1, 1, 1, 37807, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45338),
(50595, 24, 0, 0, 2872, 0, 1, 1, 1, 1, 37807, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45745), -- Alliance Defender
(48229, 1, -1, -1, 202, 0, 1, 1, 1, 2, 41764, 0, 0, 0, 48229, 48229, 0, 7684, 7684, 524288, 0, 0, 0, 0, 0, 0, 0, 45338),
(48229, 2, 0, 0, 1199, 0, 2, 1, 1, 7, 41765, 0, 0, 0, 48229, 48229, 0, 7684, 7684, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(48229, 24, 0, 0, 2872, 3, 2, 1, 1, 7, 41765, 0, 0, 0, 48229, 48229, 0, 7684, 7684, 524288, 0, 0, 0, 0, 0, 0, 0, 64978), -- Kobold Digger
(48230, 1, 0, 0, 202, 0, 7, 1, 1, 15, 41758, 0, 0, 0, 48230, 48230, 0, 7084, 7084, 524288, 0, 0, 0, 0, 0, 0, 0, 45338),
(48230, 2, 1, 1, 1199, 0, 7, 1, 1, 35, 41759, 0, 0, 0, 48230, 48230, 0, 7084, 7084, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(48230, 24, 1, 1, 2872, 3, 12, 1, 1, 35, 41759, 0, 0, 0, 48230, 48230, 0, 7084, 7084, 524288, 0, 0, 0, 0, 0, 0, 0, 45745), -- Ogre Henchman
(48262, 1, 0, 0, 202, 	0, 1, 1, 1, 1, 41681, 0, 0, 0, 0, 0, 0, 4819, 4819, 524288, 0, 0, 0, 0, 0, 0, 0, 45338),
(48262, 2, 1, 1, 1199, 	0, 7, 1, 1, 15, 41682, 0, 0, 0, 0, 0, 0, 4819, 4819, 524288, 0, 0, 0, 0, 0, 0, 0, 45745),
(48262, 24, 1, 1, 2872, 3, 15, 1, 1, 35, 41682, 0, 0, 0, 0, 0, 0, 4819, 4819, 524288, 0, 0, 0, 0, 0, 0, 0, -1), -- Ogre Bodyguard
(47162, 1, 1, 1, 202, 0, 35, 1, 1, 16, 43989, 104, 128, 0, 47162, 0, 0, 13158, 13158, 524288, 0, 0, 0, 0, 0, 0, 0, 64978),
(47162, 2, 2, 2, 1199, 0, 50, 1, 1, 16, 43995, 104, 128, 0, 47162, 0, 0, 13158, 13158, 524288, 0, 0, 0, 0, 0, 0, 0, 64978),
(47162, 24, 2, 2, 2872, 3, 52, 1, 1, 125, 43996, 104, 128, 0, 47162, 0, 0, 13158, 13158, 524288, 0, 0, 0, 0, 0, 0, 0, 64978); -- Glubtok

UPDATE `creature_template_addon` SET `PvPFlags` = 16, `emote` = 648 WHERE `entry` = 48229; -- Kobolds mining emote

-- Spawn Updates
UPDATE `creature` SET `ZoneId` = 1581, `AreaId` = 1581, `spawnDifficulties` = '1,2,24' WHERE `map` = 36 AND `PhaseId` IN (0, 169);
-- Schorch Mark bunny should have its model changed in the script to 36147
UPDATE `creature` SET `modelid` = 0 WHERE `guid` = 375860; 

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` IN (48229);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (48229) and `source_type` = 0;
INSERT INTO `smart_scripts` VALUES
(48229, 0, 0, 0, '', 0, 0, 100, 0, 5000, 5000, 9000, 12000, 0, '', 11, 89663, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Kobold Digger - Cast Candle Blast'),
(48229, 0, 1, 0, '', 4, 0, 30, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Kobold Digger - On Aggro Say 0');

-- Scripts
UPDATE `creature_template` SET `ScriptName` = 'npc_ogre_henchman' WHERE `entry` = 48230;
UPDATE `creature_template` SET `ScriptName` = 'npc_ogre_bodyguard' WHERE `entry` = 48262;
UPDATE `creature_template` SET `ScriptName` = 'npc_mining_powder' WHERE `entry` = 48284;

-- Spell Conditions
-- Delete conditions for OgrishMotivationNormal (89652)
DELETE FROM conditions WHERE SourceTypeOrReferenceId = 13 AND SourceEntry = 89652;

-- Delete conditions for OgrishMotivationHeroic (92747)
DELETE FROM conditions WHERE SourceTypeOrReferenceId = 13 AND SourceEntry = 92747;

-- For OgrishMotivationNormal (89652)
-- Condition 1: Target must be TYPEID_UNIT (creature only, not player)
INSERT INTO conditions (SourceTypeOrReferenceId, SourceGroup, SourceEntry, SourceId, ElseGroup, ConditionTypeOrReference, ConditionTarget, ConditionValue1, ConditionValue2, ConditionValue3, NegativeCondition, ErrorType, ErrorTextId, ScriptName, COMMENT)
VALUES (13, 63, 89652, 0, 0, 51, 0, 5, 0, 0, 0, 0, 0, '', 'OgrishMotivationNormal - Target must be TYPEID_UNIT (creature only, not player)');

-- Condition 2: Target must NOT be hostile (exclude HATED and HOSTILE)
INSERT INTO conditions (SourceTypeOrReferenceId, SourceGroup, SourceEntry, SourceId, ElseGroup, ConditionTypeOrReference, ConditionTarget, ConditionValue1, ConditionValue2, ConditionValue3, NegativeCondition, ErrorType, ErrorTextId, ScriptName, COMMENT)
VALUES (13, 63, 89652, 0, 0, 34, 0, 0, 3, 0, 1, 0, 0, '', 'OgrishMotivationNormal - Target must NOT be hostile (exclude HATED/HOSTILE)');

-- For OgrishMotivationHeroic (92747)
-- Condition 1: Target must be TYPEID_UNIT (creature only, not player)
INSERT INTO conditions (SourceTypeOrReferenceId, SourceGroup, SourceEntry, SourceId, ElseGroup, ConditionTypeOrReference, ConditionTarget, ConditionValue1, ConditionValue2, ConditionValue3, NegativeCondition, ErrorType, ErrorTextId, ScriptName, COMMENT)
VALUES (13, 63, 92747, 0, 0, 51, 0, 5, 0, 0, 0, 0, 0, '', 'OgrishMotivationHeroic - Target must be TYPEID_UNIT (creature only, not player)');

-- Condition 2: Target must NOT be hostile (exclude HATED and HOSTILE)
INSERT INTO conditions (SourceTypeOrReferenceId, SourceGroup, SourceEntry, SourceId, ElseGroup, ConditionTypeOrReference, ConditionTarget, ConditionValue1, ConditionValue2, ConditionValue3, NegativeCondition, ErrorType, ErrorTextId, ScriptName, COMMENT)
VALUES (13, 63, 92747, 0, 0, 34, 0, 0, 3, 0, 1, 0, 0, '', 'OgrishMotivationHeroic - Target must NOT be hostile (exclude HATED/HOSTILE)');

-- Glubtok encounter
UPDATE `creature_template` SET `ScriptName` = 'boss_glubtok' WHERE `entry` = 47162;
UPDATE `creature_template` SET `ScriptName` = 'npc_glubtok_main_platter' WHERE `entry` = 48974;
UPDATE `creature_template` SET `ScriptName` = 'npc_glubtok_secondary_platter' WHERE `entry` IN (48975,48976,49039,49040,49041,49042);

UPDATE `creature_template` SET `VehicleId` = '1367' WHERE (`entry` = '48976');
UPDATE `creature_template` SET `VehicleId` = '1368' WHERE (`entry` = '49041');
UPDATE `creature_template` SET `VehicleId` = '1369' WHERE (`entry` = '49042');

DELETE FROM `creature` WHERE `id` = 48974 AND `map` = 36;

DELETE FROM `vehicle_template` WHERE `creatureId` IN (48974,48975,48976,49039,49040,49041,49042);
INSERT INTO `vehicle_template` (`creatureId`, `despawnDelayMs`, `Pitch`, `CustomFlags`) VALUES
(48974, 0, NULL, 0),
(48975, 0, NULL, 0),
(48976, 0, NULL, 0),
(49039, 0, NULL, 0),
(49040, 0, NULL, 0),
(49041, 0, NULL, 0),
(49042, 0, NULL, 0);

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN (48974,48975,48976,49039,49040,49041,49042,48266);
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `cast_flags`, `user_type`) VALUES
(48266, 46598, 1, 0),
(48974, 46598, 0, 0),
(48975, 46598, 0, 0),
(48976, 46598, 0, 0),
(49039, 46598, 0, 0),
(49040, 46598, 0, 0),
(49041, 46598, 0, 0),
(49042, 46598, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` IN (91397,87897,87900,89769);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(89769, 'spell_mining_powder_explode'),
(91397, 'spell_glubtok_firewall_targetting'),
(87897, 'spell_glubtok_generic_proc'),
(87900, 'spell_glubtok_generic_proc');

DELETE FROM `creature_template_spell` WHERE `CreatureID` IN (48266);
INSERT INTO `creature_template_spell` (`CreatureID`, `Index`, `Spell`, `VerifiedBuild`) VALUES
(48266, 1, 91788, 39653); -- Defias Cannon Cannonball


DELETE FROM `creature_text` WHERE `CreatureID` IN (47162,48974,48230,48229,48262);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `SoundPlayType`, `BroadcastTextId`, `TextRange`, `comment`) VALUES 
-- Ogre Bodyguard
(48262, 0, 0, 'Me smash! You die!', 12, 0, 100, 0, 0, 0, 0, 1926, 0, 'Ogre Bodyguard - Aggro'),
(48262, 0, 1, 'Raaar!!! Me smash $r!', 12, 0, 100, 0, 0, 0, 0, 1927, 0, 'Ogre Bodyguard - Aggro'),
(48262, 0, 2, 'I''ll crush you!', 12, 0, 100, 0, 0, 0, 0, 1925, 0, 'Ogre Bodyguard - Aggro'),
(48262, 1, 0, '%s goes into a frenzy!', 16, 0, 100, 0, 0, 0, 0, 0, 0, 'Ogre Bodyguard - combat Frenzy'),
(48262, 2, 0, 'Me da best bodyguard!', 12, 0, 100, 0, 0, 0, 0, 48523, 0, 'Ogre Bodyguard - Conversation 1'),
(48262, 3, 0, 'No, me!', 12, 0, 100, 0, 0, 0, 0, 48524, 0, 'Ogre Bodyguard - Conversation 2'),
(48262, 4, 0, 'Me got a big board for hit things!', 12, 0, 100, 0, 0, 0, 0, 48525, 0, 'Ogre Bodyguard - Conversation 3'),
(48262, 5, 0, 'Mine gots a nail in it!', 12, 0, 100, 0, 0, 0, 0, 48526, 0, 'Ogre Bodyguard - Conversation 4'),
(48262, 6, 0, 'Mine gots a bigger nail!', 12, 0, 100, 0, 0, 0, 0, 48527, 0, 'Ogre Bodyguard - Conversation 5'),
(48262, 7, 0, 'Mine gots a RUSTY nail!', 12, 0, 100, 0, 0, 0, 0, 48528, 0, 'Ogre Bodyguard - Conversation 6'),
(48262, 8, 0, 'Dats dangerous.', 12, 0, 100, 0, 0, 0, 0, 48529, 0, 'Ogre Bodyguard - Conversation 7'),
(48262, 9, 0, 'Only for da bad guys.', 12, 0, 100, 0, 0, 0, 0, 48530, 0, 'Ogre Bodyguard - Conversation 8'),
(48262, 10, 0, 'But I''m da best at watchin'' da door.', 12, 0, 100, 0, 0, 0, 0, 48531, 0, 'Ogre Bodyguard - Conversation 9'),
(48262, 11, 0, 'No I am!', 12, 0, 100, 0, 0, 0, 0, 48532, 0, 'Ogre Bodyguard - Conversation 10'),
-- Kobold Digger
(48229, 0, 0, 'You no take candle!', 12, 0, 100, 0, 0, 0, 0, 1868, 0, 'Kobold Digger - Aggro'),
-- Ogre Henchman
(48230, 0, 0, 'You hit rock or me hit you!', 12, 0, 100, 0, 0, 0, 0, 48443, 0, 'Ogre Henchman - OOC'),
(48230, 0, 1, 'You want me hit rock with you head?', 12, 0, 100, 0, 0, 0, 0, 48444, 0, 'Ogre Henchman - OOC'),
(48230, 0, 2, 'More smash rock! Rar!', 12, 0, 100, 0, 0, 0, 0, 48445, 0, 'Ogre Henchman - OOC'),
(48230, 0, 3, 'Slow kobold look tasty!', 12, 0, 100, 0, 0, 0, 0, 48447, 0, 'Ogre Henchman - OOC'),
(48230, 0, 4, 'Me has whole box of candles!  Maybe give you some if hit rock good!', 12, 0, 100, 0, 0, 0, 0, 48448, 0, 'Ogre Henchman - OOC'),
(48230, 0, 5, 'You want get punched?  You make rock die now!', 12, 0, 100, 0, 0, 0, 0, 48445, 0, 'Ogre Henchman - OOC'),
(48230, 0, 6, 'Me rip slow kobold to pieces!', 12, 0, 100, 0, 0, 0, 0, 48450, 0, 'Ogre Henchman - OOC'),
(48230, 0, 7, 'You too slow!  Want me get Glubtok?', 12, 0, 100, 0, 0, 0, 0, 48451, 0, 'Ogre Henchman - OOC'),
(48230, 1, 0, 'Dis look warm!  Me getting sleepy...', 12, 0, 100, 0, 0, 0, 0, 48452, 0, 'Ogre Henchman - OOC'),
(48230, 0, 8, 'Look at all dem boxes.  What in dis box?', 12, 0, 100, 0, 0, 0, 0, 48455, 0, 'Ogre Henchman - OOC'),
(48230, 2, 0, 'Why stinky candle talk so much?', 12, 0, 100, 0, 0, 0, 0, 48461, 0, 'Ogre Henchman - OOC'),
(48230, 3, 0, 'Me smash! You die!', 12, 0, 100, 0, 0, 0, 0, 1926, 0, 'Ogre Henchman - Aggro'),
(48230, 3, 1, 'Raaar!!! Me smash $r!', 12, 0, 100, 0, 0, 0, 0, 1927, 0, 'Ogre Henchman - Aggro'),
(48230, 3, 2, 'I''ll crush you!', 12, 0, 100, 0, 0, 0, 0, 1925, 0, 'Ogre Henchman - Aggro'),
-- Glubtok Encounter
(48974, 0, 0, '|TInterface\Icons\spell_holy_innerfire.blp:20|t Glubtok creates a moving |cFFFF0000|Hspell:91398|h[Fire Wall]|h|r!', 16, 0, 0, 0, 0, 0, 0, 49155, 0, 'Glubtok Firewall'),
(47162, 0, 0, 'TOO...MUCH...POWER!!!', 14, 0, 100, 15, 0, 21145, 0, 47422, 0, 'VO_DM_GlubtokBoth_Death01'),
(47162, 1, 0, 'ARCANE POWER!!!', 14, 0, 100, 15, 0, 21146, 0, 47363, 0, 'VO_DM_GlubtokBoth_Spell03'),
(47162, 2, 0, 'Glubtok show you da power of de arcane!', 14, 0, 100, 0, 0, 21151, 0, 47256, 0, 'VO_DM_GlubtokHead1_Aggro01'),
(47162, 3, 0, 'Ha ha ha ha!', 14, 0, 100, 11, 0, 21152, 0, 40112, 0, 'VO_DM_GlubtokHead1_Kill01'),
(47162, 3, 1, '''Sploded dat one!', 14, 0, 0, 0, 0, 21155, 0, 47434, 0, 'VO_DM_GlubtokHead2_Kill01'),
(47162, 4, 0, 'Fists of flame!', 14, 0, 100, 0, 0, 21153, 0, 47239, 0, 'VO_DM_GlubtokHead1_Spell01'),
(47162, 5, 0, 'Glubtok ready?', 14, 0, 100, 1, 0, 21154, 0, 47361, 0, 'VO_DM_GlubtokHead1_Spell02'),
(47162, 6, 0, 'Fists of frost!', 14, 0, 100, 0, 0, 21156, 0, 47238, 0, 'VO_DM_GlubtokHead2_Spell01'),
(47162, 7, 0, 'Let''s do it!', 14, 0, 100, 15, 0, 21157, 0, 108750, 0, 'VO_DM_GlubtokHead2_Spell02');



-- GO: 208002 Goblin Teleporter
UPDATE `gameobject_template` SET `ScriptName` = 'go_goblin_teleporter' WHERE `entry` = 208002;

DELETE FROM `gossip_menu_option` WHERE `MenuID` = 12691;
INSERT INTO `gossip_menu_option` (`MenuID`, `GossipOptionID`, `OptionID`, `OptionText`, `OptionBroadcastTextID`, `VerifiedBuild`) VALUES
(12691, -3248896, 0, 'Press the button labeled \'Wood and Lumber.\'', 50716, 25996),
(12691, -3248897, 1, 'Press the button labeled \'Metal and Scraps.\'', 50717, 25937),
(12691, -3248898, 2, 'Press the button labeled \'Ship Parts.\'', 50718, 25996);

-- Deadmines Loot Tables
DELETE FROM `creature_loot_template` WHERE `Entry` IN (47162); -- Glubtok loot normal and heroic
INSERT INTO `creature_loot_template` (`Entry`, `ItemType`, `Item`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
('47162', '0', '5444', '37', '0', '1', '0', '1', '1', 'Glubtok - Miner Cape'),
('47162', '0', '2169', '32', '0', '1', '0', '1', '1', 'Glubtok - Buzzer Blade'),
('47162', '0', '5195', '32', '0', '1', '0', '1', '1', 'Glubtok - Gold-Flecked Gloves'),
('47162', '0', '65163', '21', '0', '2', '0', '1', '1', 'Glubtok - Heroic - Buzzer Blade'),
('47162', '0', '63468', '20', '0', '2', '0', '1', '1', 'Glubtok - Heroic - Defias Brotherhood Vest'),
('47162', '0', '63470', '20', '0', '2', '0', '1', '1', 'Glubtok - Heroic - Missing Diplomat Pauldrons'),
('47162', '0', '63471', '20', '0', '2', '0', '1', '1', 'Glubtok - Heroic - Vest of the curious visitor'),
('47162', '0', '63467', '20', '0', '2', '0', '1', '1', 'Glubtok - Heroic - Shadow of the past'),
('47162', '0', '157628', '19', '0', '2', '0', '1', '1', 'Glubtok - Heroic - Gold-Flecked Gloves');
