-- DH Campaign Vault of the Wardens

-- Sniff Data
DELETE FROM `areatrigger_template` WHERE (`Id`=10378 AND `IsCustom`=0);
INSERT INTO `areatrigger_template` (`Id`, `IsCustom`, `VerifiedBuild`) VALUES
(10378, 0, 70575);

UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=11266 AND `IsCustom`=0);
UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=10665 AND `IsCustom`=0);
UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=10693 AND `IsCustom`=0);
UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=10076 AND `IsCustom`=0);
UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=38618 AND `IsCustom`=0);
UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=10041 AND `IsCustom`=0);

DELETE FROM `areatrigger_create_properties` WHERE (`IsCustom`=0 AND `Id` IN (5363,5671));
INSERT INTO `areatrigger_create_properties` (`Id`, `IsCustom`, `AreaTriggerId`, `IsAreatriggerCustom`, `Flags`, `MoveCurveId`, `ScaleCurveId`, `MorphCurveId`, `FacingCurveId`, `AnimId`, `AnimKitId`, `DecalPropertiesId`, `SpellForVisuals`, `PositionalSoundKitId`, `TimeToTargetScale`, `Speed`, `Shape`, `ShapeData0`, `ShapeData1`, `ShapeData2`, `ShapeData3`, `ShapeData4`, `ShapeData5`, `ShapeData6`, `ShapeData7`, `Roll`, `Pitch`, `Yaw`, `TargetRoll`, `TargetPitch`, `TargetYaw`, `VerifiedBuild`) VALUES
(5363, 0, 10076, 0, 0, 0, 0, 0, 0, -1, 0, 0, NULL, 0, 15000, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL, 70575), -- Spell: 197608 (Demon's Blood)
(5671, 0, 10378, 0, 0, 0, 0, 0, 0, -1, 0, 0, NULL, 0, 15000, 0, 4, 5, 5, 4, 4, 0.300000011920928955, 0.300000011920928955, 0, 0, 0, 0, 0, NULL, NULL, NULL, 70575); -- Spell: 200786 (Bad Breath)

UPDATE `areatrigger_create_properties` SET `TimeToTargetScale`=25000, `VerifiedBuild`=70575 WHERE (`Id`=5977 AND `IsCustom`=0); -- Spell: 204255 (Soul Fragment)
UPDATE `areatrigger_create_properties` SET `VerifiedBuild`=70575 WHERE (`Id`=6710 AND `IsCustom`=0);
UPDATE `areatrigger_create_properties` SET `VerifiedBuild`=70575 WHERE (`Id`=6711 AND `IsCustom`=0);
UPDATE `areatrigger_create_properties` SET `TimeToTargetScale`=3000 WHERE (`Id`=5668 AND `IsCustom`=0); -- Spell: 200753 (Furious Flurry)
UPDATE `areatrigger_create_properties` SET `ShapeData0`=4, `ShapeData1`=4, `VerifiedBuild`=70575 WHERE (`Id`=36671 AND `IsCustom`=0); -- Spell: 1223412 (Soul Fragment)
UPDATE `areatrigger_create_properties` SET `VerifiedBuild`=70575 WHERE (`Id`=5319 AND `IsCustom`=0); -- Spell: 197245 (Gaseous Cloud)

UPDATE `creature_template` SET `unit_flags`=0x0 WHERE `entry`=92782; -- Savage Felstalker
UPDATE `creature_template` SET `npcflag`=281474976710659, `unit_flags3`=0x200 WHERE `entry`=92718; -- Maiev Shadowsong
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=96847; -- Drelanim Whisperwind
UPDATE `creature_template` SET `faction`=2838, `npcflag`=36028797018968192, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x4000800, `unit_flags3`=0x40000000 WHERE `entry`=149919; -- Worn Anvil
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=100168; -- Ward of the Hunters
UPDATE `creature_template` SET `npcflag`=0, `unit_flags3`=0x41000000 WHERE `entry`=103658; -- Kayn's Cell
UPDATE `creature_template` SET `unit_flags3`=0x41000000 WHERE `entry`=103655; -- Altruis's Cell
UPDATE `creature_template` SET `unit_flags`=0x0 WHERE `entry`=97069; -- Wrath-Lord Lekos


DELETE FROM `gameobject_template_addon` WHERE `entry` IN (247182 /*Warden's Door*/, 245434 /*Warden's Door*/, 245208 /*Warden's Door*/, 245207 /*Warden's Door*/, 245840 /*Warden's Door*/, 252405 /*Vault of the Wardens*/, 245206 /*Warden's Door*/, 245577 /*Frost Sentry*/, 245576 /*Frost Sentry*/, 243944 /*Warden's Door*/, 245575 /*Frost Sentry*/, 245757 /*Mirror Tile*/, 245756 /*Mirror Tile*/, 245759 /*Mirror Tile*/, 245758 /*Mirror Tile*/, 245753 /*Mirror Tile*/, 245752 /*Mirror Tile*/, 245755 /*Mirror Tile*/, 245754 /*Mirror Tile*/, 245749 /*Mirror Tile*/, 245748 /*Mirror Tile*/, 245751 /*Mirror Tile*/, 245750 /*Mirror Tile*/, 245745 /*Mirror Tile*/, 245744 /*Mirror Tile*/, 245747 /*Mirror Tile*/, 245746 /*Mirror Tile*/, 245741 /*Mirror Tile*/, 245740 /*Mirror Tile*/, 245743 /*Mirror Tile*/, 245611 /*Frost-Covered Bridge*/, 245742 /*Mirror Tile*/, 245737 /*Mirror Tile*/, 245739 /*Mirror Tile*/, 245738 /*Mirror Tile*/, 244353 /*Vault of Ice*/, 245913 /*Vault of Ice*/, 244352 /*Vault of Mirrors*/, 244355 /*Vault of Order*/, 245915 /*Vault of Law*/, 244354 /*Warden's Door*/, 245914 /*Vault of Mirrors*/, 244661 /*Warden's Door*/, 244660 /*Warden's Door*/, 244663 /*Warden's Door*/, 244662 /*Warden's Door*/, 244657 /*Warden's Door*/, 244656 /*Warden's Door*/, 244659 /*Warden's Door*/, 244658 /*Warden's Door*/, 244653 /*Warden's Door*/, 244404 /*Illidan's Rest*/, 244652 /*Warden's Door*/, 244655 /*Warden's Door*/, 244654 /*Warden's Door*/, 245761 /*Mirror Tile*/, 245760 /*Mirror Tile*/, 243945 /*Warden's Door*/, 243946 /*Warden's Door*/, 243943 /*Warden's Door*/, 247411 /*Collision Wall*/, 244927 /*Collision Wall*/, 244923 /*Lever*/, 244588 /*Warden Cell*/, 244596 /*Warden Cell*/, 245728 /*Sargerite Keystone*/, 244700 /*Small Treasure Chest*/, 244466 /*Fel Portal*/, 245112 /*Tome of Fel Secrets*/, 243968 /*Illidari Banner*/, 244701 /*Small Treasure Chest*/, 244699 /*Small Treasure Chest*/, 246296 /*Spider Web*/, 246353 /*Small Treasure Chest*/, 245996 /*Small Treasure Chest*/, 246012 /*Spire Stabilizer*/, 243967 /*Illidari Banner*/, 245995 /*Spire Stabilizer*/, 245345 /*Small Treasure Chest*/, 245169 /*7LG_Legion_Book01 [scale x2]*/, 244604 /*Spire Stabilizer*/, 244601 /*Spire Stabilizer*/);
INSERT INTO `gameobject_template_addon` (`entry`, `faction`, `flags`, `WorldEffectID`, `AIAnimKitID`) VALUES
(247182, 0, 0x20, 0, 0), -- Warden's Door
(245434, 0, 0x20, 0, 0), -- Warden's Door
(245208, 0, 0x20, 0, 0), -- Warden's Door
(245207, 0, 0x20, 0, 0), -- Warden's Door
(245840, 0, 0x20, 0, 0), -- Warden's Door
(252405, 0, 0x20, 0, 0), -- Vault of the Wardens
(245206, 0, 0x20, 0, 0), -- Warden's Door
(245577, 0, 0x20, 0, 0), -- Frost Sentry
(245576, 0, 0x20, 0, 0), -- Frost Sentry
(243944, 0, 0x20, 0, 0), -- Warden's Door
(245575, 0, 0x20, 0, 0), -- Frost Sentry
(245757, 1375, 0x20, 0, 0), -- Mirror Tile
(245756, 1375, 0x20, 0, 0), -- Mirror Tile
(245759, 1375, 0x20, 0, 0), -- Mirror Tile
(245758, 1375, 0x20, 0, 0), -- Mirror Tile
(245753, 1375, 0x20, 0, 0), -- Mirror Tile
(245752, 1375, 0x20, 0, 0), -- Mirror Tile
(245755, 1375, 0x20, 0, 0), -- Mirror Tile
(245754, 1375, 0x20, 0, 0), -- Mirror Tile
(245749, 1375, 0x20, 0, 0), -- Mirror Tile
(245748, 1375, 0x20, 0, 0), -- Mirror Tile
(245751, 1375, 0x20, 0, 0), -- Mirror Tile
(245750, 1375, 0x20, 0, 0), -- Mirror Tile
(245745, 1375, 0x20, 0, 0), -- Mirror Tile
(245744, 1375, 0x20, 0, 0), -- Mirror Tile
(245747, 1375, 0x20, 0, 0), -- Mirror Tile
(245746, 1375, 0x20, 0, 0), -- Mirror Tile
(245741, 1375, 0x20, 0, 0), -- Mirror Tile
(245740, 1375, 0x20, 0, 0), -- Mirror Tile
(245743, 1375, 0x20, 0, 0), -- Mirror Tile
(245611, 1375, 0x20, 0, 0), -- Frost-Covered Bridge
(245742, 1375, 0x20, 0, 0), -- Mirror Tile
(245737, 1375, 0x20, 0, 0), -- Mirror Tile
(245739, 1375, 0x20, 0, 0), -- Mirror Tile
(245738, 1375, 0x20, 0, 0), -- Mirror Tile
(244353, 114, 0x20, 0, 0), -- Vault of Ice
(245913, 0, 0x20, 0, 0), -- Vault of Ice
(244352, 114, 0x20, 0, 0), -- Vault of Mirrors
(244355, 114, 0x20, 0, 0), -- Vault of Order
(245915, 0, 0x20, 0, 0), -- Vault of Law
(244354, 114, 0x20, 0, 0), -- Warden's Door
(245914, 0, 0x20, 0, 0), -- Vault of Mirrors
(244661, 0, 0x20, 0, 0), -- Warden's Door
(244660, 0, 0x20, 0, 0), -- Warden's Door
(244663, 0, 0x20, 0, 0), -- Warden's Door
(244662, 0, 0x20, 0, 0), -- Warden's Door
(244657, 0, 0x20, 0, 0), -- Warden's Door
(244656, 0, 0x20, 0, 0), -- Warden's Door
(244659, 0, 0x20, 0, 0), -- Warden's Door
(244658, 0, 0x20, 0, 0), -- Warden's Door
(244653, 0, 0x20, 0, 0), -- Warden's Door
(244404, 114, 0x20, 0, 0), -- Illidan's Rest
(244652, 0, 0x20, 0, 0), -- Warden's Door
(244655, 0, 0x20, 0, 0), -- Warden's Door
(244654, 0, 0x20, 0, 0), -- Warden's Door
(245761, 1375, 0x20, 0, 0), -- Mirror Tile
(245760, 1375, 0x20, 0, 0), -- Mirror Tile
(243945, 0, 0x20, 0, 0), -- Warden's Door
(243946, 0, 0x20, 0, 0), -- Warden's Door
(243943, 0, 0x20, 0, 0), -- Warden's Door
(247411, 114, 0x0, 0, 0), -- Collision Wall
(244927, 114, 0x0, 0, 0), -- Collision Wall
(244923, 0, 0x4, 0, 7337), -- Lever
(244588, 0, 0x48020, 0, 0), -- Warden Cell
(244596, 0, 0x2000, 0, 0), -- Warden Cell
(245728, 0, 0x40000, 0, 0), -- Sargerite Keystone
(244700, 0, 0x0, 2437, 0), -- Small Treasure Chest
(244466, 0, 0x4, 0, 0), -- Fel Portal
(245112, 0, 0x40000, 0, 0), -- Tome of Fel Secrets
(243968, 0, 0x40000, 0, 0), -- Illidari Banner
(244701, 0, 0x0, 2437, 0), -- Small Treasure Chest
(244699, 0, 0x204000, 2437, 0), -- Small Treasure Chest
(246296, 114, 0x0, 0, 0), -- Spider Web
(246353, 0, 0x204000, 2437, 0), -- Small Treasure Chest
(245996, 0, 0x0, 2437, 0), -- Small Treasure Chest
(246012, 0, 0x1, 2100, 0), -- Spire Stabilizer
(243967, 0, 0x40000, 0, 0), -- Illidari Banner
(245995, 0, 0x1, 2100, 0), -- Spire Stabilizer
(245345, 0, 0x0, 2437, 0), -- Small Treasure Chest
(245169, 114, 0x0, 0, 0), -- 7LG_Legion_Book01 [scale x2]
(244604, 0, 0x20, 2100, 0), -- Spire Stabilizer
(244601, 0, 0x20, 2100, 0); -- Spire Stabilizer

UPDATE `spawn_tracking_template` SET `VerifiedBuild`=70575 WHERE `SpawnTrackingId` IN (1054898, 1102008, 1054886, 1054865, 1113926, 1113921, 1113928, 1113927, 1054859, 1026192, 1049866, 1049865, 1078117, 1046964, 1049873, 1031144, 1031228, 1031237, 1080667, 1080659);
UPDATE `spawn_tracking_quest_objective` SET `VerifiedBuild`=70575 WHERE (`QuestObjectiveId`=279980 AND `SpawnTrackingId`=1102008) OR (`QuestObjectiveId`=281206 AND `SpawnTrackingId` IN (1113926,1113921,1113928,1113927)) OR (`QuestObjectiveId`=278675 AND `SpawnTrackingId`=1054859) OR (`QuestObjectiveId`=279972 AND `SpawnTrackingId`=1054859) OR (`QuestObjectiveId`=279956 AND `SpawnTrackingId`=1049866) OR (`QuestObjectiveId`=280467 AND `SpawnTrackingId`=1049865) OR (`QuestObjectiveId`=280466 AND `SpawnTrackingId`=1078117) OR (`QuestObjectiveId`=280465 AND `SpawnTrackingId`=1026192) OR (`QuestObjectiveId`=280464 AND `SpawnTrackingId`=1046964) OR (`QuestObjectiveId`=280463 AND `SpawnTrackingId`=1049873) OR (`QuestObjectiveId`=280421 AND `SpawnTrackingId`=1080659);
