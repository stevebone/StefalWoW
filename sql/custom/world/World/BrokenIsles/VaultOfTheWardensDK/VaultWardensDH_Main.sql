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
UPDATE `creature_template` SET `npcflag`=0x01000000, `faction`=35, `unit_flags`=0x300, `unit_flags2`=0x800, `unit_flags3`=0x41000000 WHERE `entry`=103658; -- Kayn's Cell
UPDATE `creature_template` SET `npcflag`=0x01000000, `faction`=35, `unit_flags`=0x300, `unit_flags2`=0x800, `unit_flags3`=0x41000000 WHERE `entry`=103655; -- Altruis's Cell
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


UPDATE `quest_template` SET `VerifiedBuild`=70575 WHERE `ID` IN (44663, 44473, 44463, 44471, 32246, 56775, 46727, 27407, 27242, 27251, 27249, 27234, 27216, 27246, 27188, 27183, 27210, 26209, 43806, 42978, 44700, 82672, 39691, 39689, 40255, 40373, 39686, 40254, 39684, 39685, 39682, 40253, 65436, 38689, 38690, 39742, 38672);


UPDATE `quest_objectives` SET `ConditionalAmount`=251658240, `VerifiedBuild`=70575 WHERE `ID`=265754; -- 265754
UPDATE `quest_objectives` SET `ConditionalAmount`=-2147483629, `VerifiedBuild`=70575 WHERE `ID`=265755; -- 265755
UPDATE `quest_objectives` SET `ConditionalAmount`=190449689, `VerifiedBuild`=70575 WHERE `ID`=265756; -- 265756
UPDATE `quest_objectives` SET `ConditionalAmount`=137988105, `VerifiedBuild`=70575 WHERE `ID`=265757; -- 265757
UPDATE `quest_objectives` SET `VerifiedBuild`=70575 WHERE `ID` IN (267137, 286652, 286653, 280161, 280162, 280163, 280754, 280756, 257714, 280763, 265836, 286137, 280823, 265852, 286744, 278274, 278280, 266076, 280067, 280068, 280069, 280070, 280071, 337817, 285830, 266329, 266338, 279933, 280526);
UPDATE `quest_objectives` SET `ConditionalAmount`=29184, `VerifiedBuild`=70575 WHERE `ID`=280534; -- 280534
UPDATE `quest_objectives` SET `ConditionalAmount`=251658240, `VerifiedBuild`=70575 WHERE `ID`=286650; -- 286650
UPDATE `quest_objectives` SET `ConditionalAmount`=528486153, `VerifiedBuild`=70575 WHERE `ID`=280937; -- 280937
UPDATE `quest_objectives` SET `ConditionalAmount`=584, `VerifiedBuild`=70575 WHERE `ID`=286651; -- 286651
UPDATE `quest_objectives` SET `ConditionalAmount`=736104320, `VerifiedBuild`=70575 WHERE `ID`=280751; -- 280751
UPDATE `quest_objectives` SET `ConditionalAmount`=301998081, `VerifiedBuild`=70575 WHERE `ID`=286672; -- 286672
UPDATE `quest_objectives` SET `ConditionalAmount`=264, `VerifiedBuild`=70575 WHERE `ID`=279976; -- 279976
UPDATE `quest_objectives` SET `ConditionalAmount`=1634887020, `VerifiedBuild`=70575 WHERE `ID`=451774; -- 451774
UPDATE `quest_objectives` SET `ConditionalAmount`=6685195, `VerifiedBuild`=70575 WHERE `ID`=451775; -- 451775
UPDATE `quest_objectives` SET `ConditionalAmount`=1, `VerifiedBuild`=70575 WHERE `ID`=286136; -- 286136
UPDATE `quest_objectives` SET `ConditionalAmount`=-559076435, `VerifiedBuild`=70575 WHERE `ID`=286143; -- 286143
UPDATE `quest_objectives` SET `ConditionalAmount`=0, `VerifiedBuild`=70575 WHERE `ID`=391353; -- 391353
UPDATE `quest_objectives` SET `ConditionalAmount`=-1671575821, `VerifiedBuild`=70575 WHERE `ID`=278275; -- 278275
UPDATE `quest_objectives` SET `ConditionalAmount`=251658240, `VerifiedBuild`=70575 WHERE `ID`=278285; -- 278285
UPDATE `quest_objectives` SET `ConditionalAmount`=562197092, `VerifiedBuild`=70575 WHERE `ID`=286169; -- 286169
UPDATE `quest_objectives` SET `ConditionalAmount`=1953390952, `VerifiedBuild`=70575 WHERE `ID`=280065; -- 280065
UPDATE `quest_objectives` SET `ConditionalAmount`=251659528, `VerifiedBuild`=70575 WHERE `ID`=280066; -- 280066
UPDATE `quest_objectives` SET `ConditionalAmount`=2599, `VerifiedBuild`=70575 WHERE `ID`=279874; -- 279874
UPDATE `quest_objectives` SET `ConditionalAmount`=18, `VerifiedBuild`=70575 WHERE `ID`=280072; -- 280072
UPDATE `quest_objectives` SET `ConditionalAmount`=264, `VerifiedBuild`=70575 WHERE `ID`=279877; -- 279877
UPDATE `quest_objectives` SET `ConditionalAmount`=65536, `VerifiedBuild`=70575 WHERE `ID`=286773; -- 286773
UPDATE `quest_objectives` SET `ConditionalAmount`=8087, `VerifiedBuild`=70575 WHERE `ID`=279878; -- 279878
UPDATE `quest_objectives` SET `ConditionalAmount`=566, `VerifiedBuild`=70575 WHERE `ID`=337818; -- 337818
UPDATE `quest_objectives` SET `ConditionalAmount`=566, `VerifiedBuild`=70575 WHERE `ID`=266330; -- 266330
UPDATE `quest_objectives` SET `ConditionalAmount`=1, `VerifiedBuild`=70575 WHERE `ID`=279932; -- 279932


UPDATE `quest_visual_effect` SET `VerifiedBuild`=70575 WHERE (`Index`=0 AND `ID` IN (286744,286773,286651,286650,391353,337818,280756,280763,286169,280754,279878,279933,279932,279877,279874,280751,451775,278280,280161,286137,286136,278275,278274)) OR (`Index`=1 AND `ID` IN (286651,280751,278280)) OR (`Index`=2 AND `ID`=278280);


UPDATE `quest_reward_display_spell` SET `VerifiedBuild`=70575 WHERE (`QuestID`=32246 AND `Idx`=0);


UPDATE `creature_template` SET `VerifiedBuild`=70575 WHERE `entry` IN (5806, 100429, 100449, 100454, 61834, 100455, 100456, 61836, 100457, 61837, 1976, 100458, 61838, 61839, 102391, 100460, 61840, 61841, 102393, 113986, 100469, 100471, 100472, 68, 100501, 185467, 185468, 152643, 96645, 61896, 96659, 96665, 96666, 96672, 96675, 29093, 96680, 96681, 96682, 44582, 44583, 29142, 102543, 38821, 102554, 15659, 4075, 253181, 52358, 52359, 253183, 52361, 96783, 58167, 34997, 34998, 11827, 11828, 102585, 100665, 100667, 46602, 2198, 50480, 277, 279, 92980, 185669, 185670, 6089, 92984, 185672, 297, 6090, 92985, 92986, 92990, 141270, 100724, 179896, 143208, 114246, 50523, 50524, 50525, 197287, 338, 340, 100754, 2285, 48632, 44773, 44774, 189602, 189603, 6174, 2334, 201230, 416, 114358, 149121, 149124, 96997, 50669, 176192, 176203, 482, 483, 201312, 212899, 176220, 176226, 176229, 176230, 176233, 176235, 176237, 97070, 259276, 176244, 2455, 2456, 2457, 155011, 183978, 93223, 97090, 261231, 100973, 100974, 100975, 100977, 100978, 100979, 100980, 100981, 2504, 100993, 101004, 87501, 35365, 93296, 87503, 93307, 151247, 151249, 151251, 151255, 151256, 101055, 197609, 197610, 197611, 97225, 97228, 103033, 199602, 97265, 97267, 35477, 721, 97273, 162935, 97296, 103093, 209307, 97330, 97333, 97334, 242173, 85816, 85817, 85818, 85819, 257683, 2795, 12480, 12481, 14423, 112912, 101332, 14438, 14439, 56925, 8666, 230734, 8670, 6740, 147714, 99443, 230765, 147741, 140022, 101406, 259749, 8719, 99501, 101446, 165189, 97614, 97615, 97616, 207684, 97617, 97618, 97619, 97620, 97622, 97632, 97643, 101506, 97644, 101507, 4960, 4961, 4974, 140146, 4981, 111190, 101548, 51348, 99631, 99632, 62954, 95791, 101585, 242550, 256071, 59113, 59115, 242566, 72654, 43692, 113211, 259970, 256112, 99709, 99710, 72676, 51440, 101647, 103581, 103582, 103583, 103584, 209790, 103586, 103587, 103588, 99731, 99732, 97806, 1257, 155739, 49540, 1275, 252312, 1285, 1286, 43769, 1287, 1289, 1291, 1292, 1294, 1295, 1297, 1298, 1299, 1300, 1301, 1302, 1303, 1304, 1305, 1307, 1308, 1312, 28347, 1314, 1315, 1316, 103660, 1317, 1318, 1319, 1320, 28355, 1321, 1327, 1328, 5193, 1333, 43819, 43820, 1339, 43822, 47688, 1346, 1347, 175138, 1366, 1367, 148124, 165505, 113392, 256286, 1395, 1402, 113404, 1412, 11068, 1419, 49701, 1427, 97978, 1428, 1431, 1432, 1435, 1439, 1440, 11096, 204198, 130828, 82564, 49748, 1477, 1478, 99962, 99963, 101895, 99964, 99965, 99966, 99967, 99968, 99969, 101901, 101907, 53641, 198482, 5386, 111600, 198505, 198506, 198509, 18927, 100035, 100036, 100037, 24729, 100038, 100039, 100040, 100041, 100042, 100043, 100044, 198526, 198529, 198531, 198532, 198533, 100065, 100066, 3518, 198551, 7386, 49877, 55684, 244923, 198579, 49893, 198581, 5482, 5483, 32520, 175416, 198589, 175420, 5493, 5494, 5504, 17103, 5520, 15187, 206381, 3629, 5564, 5565, 84745, 84749, 84751, 51938, 1733, 42288, 30713, 1750, 1751, 1752, 44237, 1756, 44238, 44239, 44241, 44243, 30730, 46180, 109912, 42339, 23050, 53949, 50088, 40435, 100296, 100298, 100299, 52031, 173706, 9584, 42421, 206596, 100399);
UPDATE `creature_template` SET `VerifiedBuild`=70575 WHERE `entry`=461; -- Demisette Cloyce
UPDATE `creature_template` SET `VerifiedBuild`=70575 WHERE `entry`=5495; -- Ursula Deline
UPDATE `creature_template` SET `VerifiedBuild`=70575 WHERE `entry`=5496; -- Sandahl
UPDATE `creature_template` SET `VerifiedBuild`=70575 WHERE `entry`=5506; -- Maldryn


UPDATE `creature_template_model` SET `VerifiedBuild`=70575 WHERE (`Idx`=0 AND `CreatureID` IN (113986,5495,5520,461,5496,1435,1733,209307,103033,9584,1419,1307,256112,259276,148124,147714,155739,147741,113404,207684,206596,204198,206381,84745,85818,85819,82564,84751,85816,84749,85817,1305,198506,198505,198509,149121,1308,230765,149124,101004,109912,95791,113392,1304,197610,197609,130828,113211,114246,11827,11828,297,5504,197611,5506,256071,256286,230734,49893,261231,198482,198529,197287,259970,52031,198579,30713,30730,1314,201312,42288,49701,1300,1346,1347,198589,198581,185467,185468,185670,152643,185669,199602,5386,338,1316,183978,185672,93223,5493,11068,1317,1312,1315,1318,46180,5494,252312,6740,2456,277,43692,43822,2457,1301,2455,1431,1428,61896,53949,5193,43769,4974,43819,43820,242550,198531,162935,46602,51440,242566,1297,1298,1275,1257,1299,175416,175420,482,279,112912,48632,8670,40435,1432,257683,49877,1402,4981,8719,483,212899,15659,12481,6174,1294,1291,44774,29142,1295,12480,1285,1286,179896,1427,29093,198532,198551,198533,50669,49748,44773,141270,3518,52361,52359,52358,165189,1289,14439,1287,35477,1303,1302,2334,176233,176229,176230,253181,253183,61836,61834,14438,4961,416,44582,44583,1366,11096,176244,176226,61838,50524,56925,50480,14423,1367,1292,5565,5564,23050,1321,1320,1395,198526,61837,53641,242173,55684,28355,28347,8666,87501,7386,1319,2795,50525,50523,1339,1333,176235,201230,62954,100399,100754,5806,100977,100974,101055,100978,100993,100981,100979,100973,100975,100980,102554,1752,151249,151255,151251,44241,44243,1750,2504,1440,1751,44239,44238,1756,151247,151256,143208,102543,72654,24729,35365,34997,100429,93307,51938,34998,111600,102585,100454,100458,100460,15187,100455,100471,100456,100472,189603,100449,17103,1439,100457,2285,4960,721,93296,5483,50088,209790,165505,5482,32520,175138,189602,38821,111190,47688,259749,87503,176237,1976,4075,18927,176192,61840,61839,140022,140146,100501,97296,3629,6089,44237,340,155011,1477,1478,49540,1327,58167,176203,6090,176220,1328,51348,173706,1412,2198,42421,68,42339,61841,103660,97978,100040,100035,100042,96666,103584,103586,103588,100043,103582,100037,100039,100041,100036,103583,100044,103587,103581,100038,101506,101507,99967,101585,99966,99968,100065,100066,99965,97273,99969,99962,99963,101446,99964,72676,97267,97265,97644,100469,102391,103093,96680,99731,101548,99709,102393,99732,99710,96681,101406,96672,101332,100724,96682,97334,97333,100665,97090,100667,97330,114358,97225,97228,96645,97806,97643,96675,99443,101901,101895,101907,97070,96659,100298,97632,92990,92984,92985,100296,96665,100299,59115,96783,97614,97618,97616,97622,97619,101647,97615,97617,97620,96997,59113,92980,92986,99631,99632,99501,244923)) OR (`Idx`=1 AND `CreatureID` IN (148124,147714,155739,147741,230765,149124,109912,113392,197611,230734,152643,183978,185672,93223,242550,162935,242566,212899,52361,253181,253183,242173,62954,100754,100993,151249,1756,102543,72654,111600,189603,100449,721,50088,32520,38821,111190,1976,4075,18927,140022,140146,100501,49540,51348,42421,68,42339,101585,101446,100469,103093,99731,99709,99732,99710,101332,100724,101895,59115,59113)) OR (`Idx`=28 AND `CreatureID` IN (230765,230734)) OR (`Idx`=27 AND `CreatureID` IN (230765,230734)) OR (`Idx`=26 AND `CreatureID` IN (230765,230734)) OR (`Idx`=25 AND `CreatureID` IN (230765,230734)) OR (`Idx`=24 AND `CreatureID` IN (230765,230734)) OR (`Idx`=23 AND `CreatureID` IN (230765,230734)) OR (`Idx`=22 AND `CreatureID` IN (230765,230734)) OR (`Idx`=21 AND `CreatureID` IN (230765,230734)) OR (`Idx`=20 AND `CreatureID` IN (230765,230734)) OR (`Idx`=19 AND `CreatureID` IN (230765,230734)) OR (`Idx`=18 AND `CreatureID` IN (230765,230734)) OR (`Idx`=17 AND `CreatureID` IN (230765,230734,212899)) OR (`Idx`=16 AND `CreatureID` IN (230765,230734,212899)) OR (`Idx`=15 AND `CreatureID` IN (230765,230734,212899)) OR (`Idx`=14 AND `CreatureID` IN (230765,230734,212899)) OR (`Idx`=13 AND `CreatureID` IN (230765,230734,212899)) OR (`Idx`=12 AND `CreatureID` IN (230765,230734,212899)) OR (`Idx`=11 AND `CreatureID` IN (230765,230734,212899)) OR (`Idx`=10 AND `CreatureID` IN (230765,230734,212899)) OR (`Idx`=9 AND `CreatureID` IN (230765,230734,212899)) OR (`Idx`=8 AND `CreatureID` IN (230765,230734,212899)) OR (`Idx`=7 AND `CreatureID` IN (230765,230734,212899)) OR (`Idx`=6 AND `CreatureID` IN (230765,230734,212899)) OR (`Idx`=5 AND `CreatureID` IN (230765,230734,242550,212899)) OR (`Idx`=4 AND `CreatureID` IN (230765,230734,242550,242566,212899,1756)) OR (`Idx`=3 AND `CreatureID` IN (230765,230734,242550,242566,212899,253181,253183,151249,1756,68,42339)) OR (`Idx`=2 AND `CreatureID` IN (230765,149124,109912,230734,242550,242566,212899,253181,253183,62954,100993,151249,1756,102543,4075,49540,42421,68,42339));


UPDATE `creature_questitem` SET `VerifiedBuild`=70575 WHERE (`CreatureEntry`=130828 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=100455 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=97225 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=97228 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=96659 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=96997 AND `DifficultyID`=0 AND `Idx`=0);


UPDATE `creature_quest_currency` SET `VerifiedBuild`=70575 WHERE (`CurrencyId`=1155 AND `CreatureId` IN (130828,100455,96682,97225,97228,96659,96783,96997)) OR (`CurrencyId`=3252 AND `CreatureId` IN (130828,100455,96682,97225,97228,96659,96783,96997));

DELETE FROM `gameobject_template` WHERE `entry` IN (629272 /*Adoree's Stack of Books*/, 645097 /*Portal Mirror [DNT]*/);
INSERT INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `size`, `Data0`, `Data1`, `Data2`, `Data3`, `Data4`, `Data5`, `Data6`, `Data7`, `Data8`, `Data9`, `Data10`, `Data11`, `Data12`, `Data13`, `Data14`, `Data15`, `Data16`, `Data17`, `Data18`, `Data19`, `Data20`, `Data21`, `Data22`, `Data23`, `Data24`, `Data25`, `Data26`, `Data27`, `Data28`, `Data29`, `Data30`, `Data31`, `Data32`, `Data33`, `Data34`, `RequiredLevel`, `ContentTuningId`, `VerifiedBuild`) VALUES
(629272, 5, 85784, 'Adoree\'s Stack of Books', '', '', '', 0.5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 70575), -- Adoree's Stack of Books
(645097, 5, 61019, 'Portal Mirror [DNT]', '', '', '', 0.5, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 70575); -- Portal Mirror [DNT]

UPDATE `gameobject_template` SET `VerifiedBuild`=70575 WHERE `entry` IN (620465, 620464, 403736, 620458, 197074, 197069, 197068, 197067, 197065, 197064, 197063, 197062, 197058, 197046, 197045, 197044, 543407, 620479, 620477, 398823, 620475, 197043, 212212, 620473, 254090, 266152, 197007, 254097, 254096, 254091, 197042, 197041, 197040, 197039, 254098, 620467, 254121, 620455, 197038, 197037, 197036, 197035, 197034, 197033, 254120, 254119, 254095, 620472, 197015, 201999, 201998, 254104, 254103, 254102, 254101, 254100, 254099, 254094, 197087, 254089, 254088, 254106, 254105, 254092, 201992, 201997, 197086, 202003, 202002, 201996, 201993, 254093, 197010, 202011, 254115, 197088, 254116, 1561, 254118, 254117, 254114, 254113, 254112, 254108, 254107, 197093, 254110, 197014, 197096, 197079, 105174, 254111, 197084, 197085, 254109, 197083, 197081, 197080, 254123, 243324, 245923, 197092, 105175, 197089, 197082, 197090, 197091, 197017, 253169, 243305, 243297, 253168, 254080, 254068, 254081, 252281, 243294, 254085, 140911, 209297, 197097, 254122, 197020, 197009, 243300, 254086, 243301, 254082, 243299, 254084, 243295, 201983, 322639, 335930, 197013, 197026, 629082, 629076, 629080, 629081, 629073, 629072, 197030, 197029, 197028, 197027, 197025, 193981, 197031, 197018, 207661, 377200, 386352, 197008, 175758, 175729, 197024, 197023, 197011, 197032, 197022, 197012, 197145, 197143, 197142, 197099, 197098, 197021, 201987, 205553, 576393, 576392, 576395, 576396, 576463, 576391, 576394, 529989, 470037, 201989, 205106, 205107, 197125, 197136, 364429, 197184, 197176, 197183, 190040, 197182, 197181, 197177, 197178, 197179, 180523, 197186, 197180, 197119, 197185, 197150, 197149, 197147, 197148, 197174, 197146, 197120, 197175, 197126, 197135, 197140, 202260, 202258, 202256, 197123, 197117, 203978, 197118, 202257, 202253, 197173, 202259, 197172, 197116, 180472, 281339, 270011, 180427, 202005, 202022, 197134, 202012, 201981, 578628, 197138, 197121, 202010, 202004, 180471, 197124, 197122, 202036, 202006, 202033, 202035, 197193, 197192, 197191, 197190, 197189, 197188, 197187, 197139, 197133, 197132, 197131, 202034, 202032, 202031, 202030, 202029, 201553, 201552, 201551, 201546, 179706, 250672, 201556, 201547, 201545, 201516, 202026, 201560, 201557, 201548, 259114, 202028, 202027, 202025, 202024, 202023, 201558, 201554, 201550, 201549, 197137, 202009, 202008, 201559, 201555, 201520, 201518, 202007, 206623, 201519, 201515, 375073, 175738, 175727, 201644, 201643, 201642, 175761, 201645, 2657, 21582, 175752, 201647, 201646, 201648, 21581, 175746, 21583, 175735, 201649, 201641, 201657, 201655, 201654, 201656, 201653, 201652, 180433, 208219, 208218, 340002, 124374, 205190, 201664, 201665, 201662, 201661, 201663, 201660, 175750, 175745, 175753, 200315, 200314, 200313, 200312, 200310, 200308, 200307, 200306, 201822, 201658, 201659, 259880, 245640, 259879, 201671, 201669, 201651, 201843, 201670, 201667, 201666, 201650, 3799, 3800, 201668, 201688, 201690, 201685, 201677, 201689, 201678, 201672, 201682, 201692, 201687, 201675, 201694, 201693, 201691, 201686, 201679, 251045, 251222, 292763, 293701, 190536, 294556, 207941, 617539, 271817, 251289, 210181, 207416, 206110, 278802, 201680, 201683, 201681, 200309, 200311, 201684, 201676, 201415, 201673, 201420, 201674, 201419, 201416, 206560, 201513, 201502, 201514, 180426, 201501, 209039, 245649, 201445, 201453, 201446, 201447, 201452, 201449, 201448, 209040, 209043, 209041, 201451, 201450, 209101, 209045, 209042, 209692, 209044, 209047, 209046, 201522, 209054, 209049, 209053, 202758, 209052, 209051, 201499, 180431, 209048, 209050, 180408, 201509, 201506, 201508, 201503, 1684, 201497, 180410, 384744, 281118, 201500, 185454, 175739, 203801, 201510, 201507, 201505, 201498, 201543, 201544, 201530, 201523, 201529, 201531, 201521, 201504, 201532, 245687, 203762, 201536, 201492, 201542, 201539, 201511, 201491, 180428, 201533, 201538, 201537, 201490, 201534, 201541, 201540, 201489, 180411, 180429, 201493, 201535, 201528, 201494, 201526, 201495, 201527, 201524, 201496, 201488, 201525, 201512, 180425, 180406, 203800, 180415, 201422, 180407, 180405, 208200, 259008, 251344, 246746, 246747, 246749, 240647, 246752, 246750, 246748, 246751);
UPDATE `gameobject_template` SET `name`='[DNT] [AUTOGEN] Green Book of Runes - DO NOT USE', `VerifiedBuild`=70575 WHERE `entry`=175730; -- The World Tree and the Emerald Dream
UPDATE `gameobject_template` SET `RequiredLevel`=1, `VerifiedBuild`=70575 WHERE `entry`=180665; -- Draconic for Dummies
UPDATE `gameobject_template` SET `RequiredLevel`=50, `VerifiedBuild`=70575 WHERE `entry`=344604; -- Fearbreaker
UPDATE `gameobject_template` SET `RequiredLevel`=10, `ContentTuningId`=1159, `VerifiedBuild`=70575 WHERE `entry` IN (245514, 245515, 245516, 245518, 245517, 245519, 245520, 245521, 245523, 245522); -- Sentry
UPDATE `gameobject_template` SET `Data19`=2, `RequiredLevel`=8, `ContentTuningId`=699, `VerifiedBuild`=70575 WHERE `entry`=246562; -- Small Treasure Chest
UPDATE `gameobject_template` SET `RequiredLevel`=8, `ContentTuningId`=699, `VerifiedBuild`=70575 WHERE `entry`=244455; -- Pool of Judgment
UPDATE `gameobject_template` SET `Data19`=2, `RequiredLevel`=8, `ContentTuningId`=699, `VerifiedBuild`=70575 WHERE `entry`=246561; -- Small Treasure Chest
UPDATE `gameobject_template` SET `RequiredLevel`=8, `ContentTuningId`=699, `VerifiedBuild`=70575 WHERE `entry` IN (244449, 245349); -- Reflective Mirror
UPDATE `gameobject_template` SET `Data19`=2, `RequiredLevel`=8, `ContentTuningId`=699, `VerifiedBuild`=70575 WHERE `entry`=246559; -- Small Treasure Chest
UPDATE `gameobject_template` SET `RequiredLevel`=8, `ContentTuningId`=699, `VerifiedBuild`=70575 WHERE `entry`=247226; -- Collision Wall for Bridge
UPDATE `gameobject_template` SET `RequiredLevel`=5, `ContentTuningId`=436, `VerifiedBuild`=70575 WHERE `entry` IN (244409, 244412, 244408, 244411, 244410); -- Abcess
UPDATE `gameobject_template` SET `Data19`=2, `RequiredLevel`=8, `ContentTuningId`=699, `VerifiedBuild`=70575 WHERE `entry`=246560; -- Small Treasure Chest
UPDATE `gameobject_template` SET `Data19`=2, `RequiredLevel`=8, `ContentTuningId`=699, `VerifiedBuild`=70575 WHERE `entry`=246558; -- Small Treasure Chest
UPDATE `gameobject_template` SET `Data19`=2, `RequiredLevel`=8, `ContentTuningId`=699, `VerifiedBuild`=70575 WHERE `entry`=246557; -- Small Treasure Chest
UPDATE `gameobject_template` SET `RequiredLevel`=8, `ContentTuningId`=699, `VerifiedBuild`=70575 WHERE `entry`=244420; -- Demon Hunter Trap
UPDATE `gameobject_template` SET `RequiredLevel`=8, `ContentTuningId`=699, `VerifiedBuild`=70575 WHERE `entry` IN (253400, 245467); -- Vault of the Betrayer
UPDATE `gameobject_template` SET `Data19`=2, `RequiredLevel`=8, `ContentTuningId`=699, `VerifiedBuild`=70575 WHERE `entry`=246556; -- Small Treasure Chest
UPDATE `gameobject_template` SET `RequiredLevel`=8, `ContentTuningId`=699, `VerifiedBuild`=70575 WHERE `entry`=244417; -- Legion Gate
UPDATE `gameobject_template` SET `RequiredLevel`=8, `ContentTuningId`=699, `VerifiedBuild`=70575 WHERE `entry`=244416; -- Legion Brazier
UPDATE `gameobject_template` SET `Data19`=2, `RequiredLevel`=8, `ContentTuningId`=699, `VerifiedBuild`=70575 WHERE `entry`=246555; -- Small Treasure Chest
UPDATE `gameobject_template` SET `RequiredLevel`=8, `ContentTuningId`=699, `VerifiedBuild`=70575 WHERE `entry`=244925; -- Ward of the Hunters


UPDATE `gameobject_questitem` SET `VerifiedBuild`=70575 WHERE (`Idx`=0 AND `GameObjectEntry` IN (105174,105175,140911,344604,278802,203801,203762,203800));

DELETE FROM `playerchoice` WHERE `ChoiceId`=234;
INSERT INTO `playerchoice` (`ChoiceId`, `UiTextureKitId`, `SoundKitId`, `CloseSoundKitId`, `Duration`, `Question`, `PendingChoiceText`, `InfiniteRange`, `HideWarboardHeader`, `KeepOpenAfterChoice`, `ShowChoicesAsList`, `ShowChoicesAsGrid`, `HideAnswerArt`, `ShowChoicesAsColumns`, `RequiresSelection`, `VerifiedBuild`) VALUES
(234, 0, 0, 0, NULL, 'Which follower will you choose?', '', 0, 0, 0, 0, 0, 0, 0, 0, 70575);


DELETE FROM `playerchoice_response` WHERE (`ChoiceId`=234 AND `ResponseId` IN (487,486));
INSERT INTO `playerchoice_response` (`ChoiceId`, `ResponseId`, `Index`, `ChoiceArtFileId`, `Flags`, `WidgetSetId`, `UiTextureAtlasElementID`, `SoundKitId`, `GroupId`, `Header`, `Subheader`, `ButtonTooltip`, `Answer`, `Description`, `Confirmation`, `RewardQuestID`, `UiTextureKitID`, `VerifiedBuild`) VALUES
(234, 487, 1, 1274661, 0, 0, 0, 0, 0, '', '', '', 'Altruis the Sufferer', 'Altruis the Sufferer is a renegade who takes matters into his own hands.\n\nAlthough he and Illidan share the same goals, he often disagrees with Illidan\'s methods.\n\nBelieving Illidan no longer controls his demonic powers, Altruis wants new leadership for the Illidari.\n\n|cFF7a0000 This story choice will not affect your character\'s power.|r', '', 0, 0, 70575),
(234, 486, 0, 1274666, 0, 0, 0, 0, 0, '', '', '', 'Kayn Sunfury', 'Kayn Sunfury is a loyal soldier and one of Lord Illidan Stormrage\'s most trusted lieutenants.\n\nHe believes that cooperation and personal sacrifice are necessary to defeat the Legion.\n\nHonorable and demanding, he follows Illidan\'s ideals to get the job done no matter the cost.\n\n|cFF7a0000 This story choice will not affect your character\'s power.|r', '', 0, 0, 70575);


DELETE FROM `playerchoice_response_reward` WHERE (`ChoiceId`=234 AND `ResponseId` IN (487,486));
INSERT INTO `playerchoice_response_reward` (`ChoiceId`, `ResponseId`, `TitleId`, `PackageId`, `SkillLineId`, `SkillPointCount`, `ArenaPointCount`, `HonorPointCount`, `Money`, `Xp`, `VerifiedBuild`) VALUES
(234, 487, 0, 0, 0, 0, 0, 0, 27750, 2100, 70575),
(234, 486, 0, 0, 0, 0, 0, 0, 27750, 2100, 70575);


UPDATE `creature_template` SET `unit_flags3`=0x1000000 WHERE `entry`=101585; -- Legion Gateway
UPDATE `creature_template` SET `unit_flags`=0x0 WHERE `entry`=99967; -- Falara Nightsong
UPDATE `creature_template` SET `unit_flags`=0x0 WHERE `entry`=99968; -- Kor'vas Bloodthorn
UPDATE `creature_template` SET `unit_flags`=0x0 WHERE `entry`=99966; -- Asha Ravensong
UPDATE `creature_template` SET `unit_flags`=0x0 WHERE `entry`=100065; -- Cassiel Nightthorn
UPDATE `creature_template` SET `unit_flags`=0x0 WHERE `entry`=100066; -- Sevis Brightflame
UPDATE `creature_template` SET `unit_flags`=0x0 WHERE `entry`=99965; -- Marius Felbane
UPDATE `creature_template` SET `unit_flags`=0x0 WHERE `entry`=99969; -- Allari the Souleater
UPDATE `creature_template` SET `unit_flags3`=0x1000000 WHERE `entry`=101446; -- Fel Annihilation
UPDATE `creature_template` SET `unit_flags`=0x0 WHERE `entry` IN (99963, 96672); -- Cyana Nightglaive
UPDATE `creature_template` SET `unit_flags`=0x0 WHERE `entry`=99964; -- Jace Darkweaver
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=96783; -- Bastillax
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=72676; -- Spirit Healer
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry` IN (100469, 59115, 59113); -- Generic Bunny
UPDATE `creature_template` SET `unit_flags3`=0x40000001 WHERE `entry`=96680; -- Glazer
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=102391; -- Altruis the Sufferer
UPDATE `creature_template` SET `unit_flags2`=0x4000800, `unit_flags3`=0x41000001 WHERE `entry`=103093; -- Safety Net
UPDATE `creature_template` SET `npcflag`=16777216, `unit_flags3`=0x41000000 WHERE `entry` IN (99731, 99732, 99709); -- Countermeasures
UPDATE `creature_template` SET `unit_flags3`=0x1000000 WHERE `entry`=101548; -- Updraft
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=102393; -- Kayn Sunfury
UPDATE `creature_template` SET `unit_flags3`=0x1000000 WHERE `entry`=99710; -- Freeze Statue
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=96681; -- Ash'golm
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags3`=0x40080000 WHERE `entry`=101406; -- Fel Spirit
UPDATE `creature_template` SET `unit_flags3`=0x1000000 WHERE `entry`=101332; -- Burning Fel
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=97333; -- Magolith
UPDATE `creature_template` SET `unit_flags3`=0x14000000 WHERE `entry`=97330; -- Raxx
UPDATE `creature_template` SET `unit_flags3`=0x41000000 WHERE `entry`=114358; -- Legion Portal
UPDATE `creature_template` SET `unit_flags`=0x0 WHERE `entry`=96645; -- Vault Warden
UPDATE `creature_template` SET `BaseAttackTime`=2000, `unit_flags2`=0x800, `VehicleId`=4372 WHERE `entry`=99443; -- Vampiric Felbat
UPDATE `creature_template` SET `faction`=954, `speed_walk`=1.20000004768371582, `speed_run`=0.428571432828903198, `BaseAttackTime`=2000, `unit_flags3`=0x80000 WHERE `entry`=101901; -- Explosive Inquisitor Eye
UPDATE `creature_template` SET `faction`=954, `BaseAttackTime`=2000, `unit_flags`=0x2000000, `unit_flags3`=0x1080000 WHERE `entry`=101895; -- Eye Dest
UPDATE `creature_template` SET `faction`=954, `speed_walk`=1.20000004768371582, `speed_run`=0.428571432828903198, `BaseAttackTime`=2000, `unit_flags`=0x2000000, `unit_flags3`=0x40080001 WHERE `entry`=101907; -- Seeping Inquisitor Eye
UPDATE `creature_template` SET `unit_flags2`=0x800, `unit_flags3`=0x40000000 WHERE `entry`=96659; -- Vampiric Felbat
UPDATE `creature_template` SET `unit_flags`=0x40 WHERE `entry`=92984; -- Kayn Sunfury
UPDATE `creature_template` SET `unit_flags3`=0x200 WHERE `entry` IN (96665, 92980); -- Kayn Sunfury
UPDATE `creature_template` SET `BaseAttackTime`=1000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=97614; -- Allari the Souleater
UPDATE `creature_template` SET `BaseAttackTime`=1000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=97618; -- Falara Nightsong
UPDATE `creature_template` SET `BaseAttackTime`=1000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=97616; -- Cyana Nightglaive
UPDATE `creature_template` SET `BaseAttackTime`=1000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=97622; -- Kor'vas Bloodthorn
UPDATE `creature_template` SET `BaseAttackTime`=1000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=97619; -- Marius Felbane
UPDATE `creature_template` SET `BaseAttackTime`=1000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=97615; -- Jace Darkweaver
UPDATE `creature_template` SET `BaseAttackTime`=1000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=97617; -- Asha Ravensong
UPDATE `creature_template` SET `BaseAttackTime`=1000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=97620; -- Lyana Darksorrow
UPDATE `creature_template` SET `unit_flags`=0x0 WHERE `entry`=96997; -- Kethrazor
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=99451; -- Mirana Starlight
UPDATE `creature_template` SET `unit_flags3`=0x200 WHERE `entry`=92986; -- Altruis the Sufferer
UPDATE `creature_template` SET `npcflag`=0, `unit_flags`=0x300, `unit_flags3`=0x41000000 WHERE `entry`=99501; -- Legion Portal
UPDATE `creature_template` SET `unit_flags`=0x0 WHERE `entry`=92776; -- Fel Shocktrooper
UPDATE `creature_template` SET `faction`=3000, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=209307; -- Lorraine Darktide
UPDATE `creature_template` SET `unit_flags3`=0x1000000 WHERE `entry`=103033; -- Enchanted Broom
UPDATE `creature_template` SET `npcflag`=1, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=256112; -- Sothon Doomrender
UPDATE `creature_template` SET `npcflag`=129 WHERE `entry`=1307; -- Charys Yserian
UPDATE `creature_template` SET `faction`=1078, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=259276; -- Archmage Galdrin
UPDATE `creature_template` SET `unit_flags3`=0x1000001 WHERE `entry`=207684; -- Stagecoach
UPDATE `creature_template` SET `speed_walk`=1, `speed_run`=1.385714292526245117 WHERE `entry`=14505; -- Dreadsteed
UPDATE `creature_template` SET `npcflag`=129 WHERE `entry`=1305; -- Jarel Moor
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=109912; -- Captive Wyrmtongue
UPDATE `creature_template` SET `unit_flags3`=0x800001 WHERE `entry`=101004; -- Elerion Bladedancer
UPDATE `creature_template` SET `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=230765; -- Dracthyr Occultist
UPDATE `creature_template` SET `npcflag`=129 WHERE `entry`=1308; -- Owen Vaughn
UPDATE `creature_template` SET `faction`=12, `npcflag`=1, `BaseAttackTime`=2000 WHERE `entry`=197610; -- Knight-Captain Sanine
UPDATE `creature_template` SET `faction`=12, `npcflag`=1, `BaseAttackTime`=2000, `unit_flags`=0x100, `unit_flags2`=0x800 WHERE `entry`=197609; -- Otan Tennoh
UPDATE `creature_template` SET `unit_flags3`=0x800000 WHERE `entry`=113211; -- Officer Fairbanks
UPDATE `creature_template` SET `unit_flags3`=0x40000001 WHERE `entry`=197611; -- (Bunny) Sessile + Gigantic AOI
UPDATE `creature_template` SET `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=230734; -- Dracthyr Stalker
UPDATE `creature_template` SET `npcflag`=129, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=256071; -- Solelo
UPDATE `creature_template` SET `faction`=190, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x4000800, `unit_flags3`=0x41000000 WHERE `entry`=256286; -- Azerothian Athenaeum
UPDATE `creature_template` SET `faction`=2028, `npcflag`=128, `BaseAttackTime`=2000, `unit_flags`=0x200, `unit_flags2`=0x800 WHERE `entry`=261231; -- Tuuran
UPDATE `creature_template` SET `faction`=1078, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=259970; -- Carl
UPDATE `creature_template` SET `unit_flags3`=0x41000000 WHERE `entry`=152643; -- Despawn Bunny
UPDATE `creature_template` SET `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=185670; -- Pet
UPDATE `creature_template` SET `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=185669; -- Mount
UPDATE `creature_template` SET `BaseAttackTime`=2000, `unit_flags2`=0x800, `unit_flags3`=0x40000000 WHERE `entry`=185672; -- Mannequin
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=93223; -- Generic Stalker
UPDATE `creature_template` SET `npcflag`=65667 WHERE `entry`=6740; -- Innkeeper Allison
UPDATE `creature_template` SET `npcflag`=129 WHERE `entry`=277; -- Roberto Pupellyverbos
UPDATE `creature_template` SET `npcflag`=129 WHERE `entry`=1301; -- Julia Gallina
UPDATE `creature_template` SET `npcflag`=524289 WHERE `entry`=4974; -- Aldwin Laughlin
UPDATE `creature_template` SET `npcflag`=9007199255265409 WHERE `entry`=5193; -- Rebecca Laughlin
UPDATE `creature_template` SET `npcflag`=0 WHERE `entry`=43769; -- Bralla Cloudwing
UPDATE `creature_template` SET `npcflag`=129 WHERE `entry`=1275; -- Kyra Boucher
UPDATE `creature_template` SET `npcflag`=129 WHERE `entry`=1257; -- Keldric Boucher
UPDATE `creature_template` SET `npcflag`=67108864, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x4000800, `unit_flags3`=0x41088000 WHERE `entry`=257683; -- Void Mail
UPDATE `creature_template` SET `npcflag`=129 WHERE `entry`=4981; -- Ben Trias
UPDATE `creature_template` SET `npcflag`=129 WHERE `entry`=483; -- Elaine Trias
UPDATE `creature_template` SET `npcflag`=2, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=212899; -- Greyguard Elite
UPDATE `creature_template` SET `npcflag`=129 WHERE `entry`=1285; -- Thurman Mullby
UPDATE `creature_template` SET `npcflag`=36028797018964096 WHERE `entry`=179896; -- Finn
UPDATE `creature_template` SET `npcflag`=131 WHERE `entry`=3518; -- Thomas Miller
UPDATE `creature_template` SET `BaseAttackTime`=1889, `unit_flags3`=0x14000000 WHERE `entry`=165189; -- Generic Hunter Pet
UPDATE `creature_template` SET `npcflag`=4227 WHERE `entry`=1287; -- Marda Weller
UPDATE `creature_template` SET `npcflag`=1, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=253183; -- Chatty Contender
UPDATE `creature_template` SET `npcflag`=3, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=253181; -- Babbling Brawler
UPDATE `creature_template` SET `BaseAttackTime`=1740 WHERE `entry`=416; -- Imp
UPDATE `creature_template` SET `unit_flags3`=0x800000 WHERE `entry`=14423; -- Officer Jaxon
UPDATE `creature_template` SET `npcflag`=129 WHERE `entry`=28347; -- Miles Sidney
UPDATE `creature_template` SET `unit_flags3`=0x1 WHERE `entry`=50525; -- Chris Miller
UPDATE `creature_template` SET `faction`=3273, `npcflag`=3, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=201230; -- Mizzen
UPDATE `creature_template` SET `unit_flags`=0x0, `unit_flags3`=0x800000 WHERE `entry`=100754; -- Stormwind Royal Guard
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=102554; -- Muradin Bronzebeard
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=100981; -- Prophet Velen
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=100980; -- Tyrande Whisperwind
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=100979; -- Moira Thaurissan
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=100978; -- Gelbin Mekkatorque
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=100977; -- Malfurion Stormrage
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=100974; -- Aysa Cloudsinger
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=100973; -- Anduin Wrynn
UPDATE `creature_template` SET `faction`=1812 WHERE `entry`=5806; -- Treant Ally
UPDATE `creature_template` SET `unit_flags`=0x0 WHERE `entry`=100993; -- Felblade Assassin
UPDATE `creature_template` SET `unit_flags3`=0x1000001 WHERE `entry`=151256; -- Conjured Illusion
UPDATE `creature_template` SET `unit_flags3`=0x1 WHERE `entry`=102543; -- Felblade Assassin
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=72654; -- General Purpose Stalker (RKS)
UPDATE `creature_template` SET `unit_flags2`=0x800 WHERE `entry`=100429; -- Anduin Wrynn
UPDATE `creature_template` SET `unit_flags3`=0x800000 WHERE `entry` IN (111600, 100449); -- Stormwind Royal Guard
UPDATE `creature_template` SET `unit_flags3`=0x800000 WHERE `entry`=15187; -- Cenarion Emissary Jademoon
UPDATE `creature_template` SET `unit_flags2`=0x800 WHERE `entry`=100456; -- Falstad Wildhammer
UPDATE `creature_template` SET `unit_flags3`=0x41000000 WHERE `entry`=93296; -- Coffee
UPDATE `creature_template` SET `unit_flags3`=0x40000001 WHERE `entry`=38821; -- Dave's Industrial Light and Magic Bunny (Medium)(Sessile)
UPDATE `creature_template` SET `unit_flags3`=0x41000000 WHERE `entry`=111190; -- "The Imposter" Quest SI:7 Courtyard Reached Kill Credit & POI Marker
UPDATE `creature_template` SET `speed_run`=1.38888859748840332, `BaseAttackTime`=2000, `unit_flags`=0x2000300, `unit_flags2`=0x4000800 WHERE `entry`=259749; -- Mech Suit
UPDATE `creature_template` SET `unit_flags3`=0x41000000 WHERE `entry`=87503; -- Champion's Treadblade
UPDATE `creature_template` SET `unit_flags`=0x0 WHERE `entry`=18927; -- Human Commoner
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=155011; -- Daphne Funkybeats
UPDATE `creature_template` SET `npcflag`=65667 WHERE `entry`=44237; -- Maegan Tillman
UPDATE `creature_template` SET `unit_flags`=0x340 WHERE `entry`=140146; -- Stormwind Infantry Captain
UPDATE `creature_template` SET `unit_flags`=0x300 WHERE `entry`=140022; -- Stormwind Infantry
UPDATE `creature_template` SET `npcflag`=129 WHERE `entry`=1327; -- Reese Langston
UPDATE `creature_template` SET `unit_flags3`=0x800000 WHERE `entry`=51348; -- Stormwind Gryphon Rider
UPDATE `creature_template` SET `npcflag`=0 WHERE `entry`=42339; -- Canal Crab
UPDATE `creature_template` SET `speed_run`=1.428571462631225585 WHERE `entry`=68; -- Stormwind City Guard
UPDATE `creature_template` SET `faction`=35, `npcflag`=3, `speed_walk`=1, `speed_run`=1.142857193946838378, `unit_flags`=0x100 WHERE `entry`=97978; -- Archmage Khadgar

UPDATE `gossip_menu` SET `VerifiedBuild`=70575 WHERE (`MenuID`=20009 AND `TextID`=29733) OR (`MenuID`=19810 AND `TextID`=29373) OR (`MenuID`=19075 AND `TextID`=30591) OR (`MenuID`=18424 AND `TextID`=26564) OR (`MenuID`=19012 AND `TextID`=27763) OR (`MenuID`=18831 AND `TextID`=27274) OR (`MenuID`=18830 AND `TextID`=27429) OR (`MenuID`=18832 AND `TextID`=27430) OR (`MenuID`=18829 AND `TextID`=27428) OR (`MenuID`=19233 AND `TextID`=28240) OR (`MenuID`=18426 AND `TextID`=26566) OR (`MenuID`=19234 AND `TextID`=28239);
DELETE FROM `gossip_menu_option` WHERE (`MenuID`=35200 AND `OptionID`=0);
INSERT INTO `gossip_menu_option` (`MenuID`, `GossipOptionID`, `OptionID`, `OptionNpc`, `OptionText`, `OptionBroadcastTextID`, `Language`, `Flags`, `ActionMenuID`, `ActionPoiID`, `GossipNpcOptionID`, `BoxCoded`, `BoxMoney`, `BoxText`, `BoxBroadcastTextID`, `SpellID`, `OverrideIconID`, `VerifiedBuild`) VALUES
(35200, 122776, 0, 0, 'I have other things to attend to. <Find Chromie and choose a Timewalking Campaign.>', 0, 0, 0, 0, 0, NULL, 0, 0, '', 0, NULL, NULL, 70575);

UPDATE `gossip_menu_option` SET `GossipOptionID`=45043, `VerifiedBuild`=70575 WHERE (`MenuID`=19075 AND `OptionID`=1);
UPDATE `gossip_menu_option` SET `GossipOptionID`=44364, `OptionBroadcastTextID`=11820, `VerifiedBuild`=70575 WHERE (`MenuID`=18830 AND `OptionID`=0); -- OptionBroadcastTextID: 11820 - 17411 - 92547 - 122256 - 122264 - 122277 - 122283 - 122291 - 122296 - 122299 - 122301 - 122309
UPDATE `gossip_menu_option` SET `GossipOptionID`=44797, `ActionMenuID`=19233, `VerifiedBuild`=70575 WHERE (`MenuID`=18426 AND `OptionID`=0);
UPDATE `gossip_menu_option` SET `GossipOptionID`=44795, `ActionMenuID`=19234, `VerifiedBuild`=70575 WHERE (`MenuID`=18424 AND `OptionID`=0);
UPDATE `gossip_menu_option` SET `GossipOptionID`=44913, `VerifiedBuild`=70575 WHERE (`MenuID`=19012 AND `OptionID`=0);


DELETE FROM `npc_vendor` WHERE (`entry`=97806 AND `item`=123960 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=97806 AND `item`=123959 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=97806 AND `item`=112460 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=97806 AND `item`=112459 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=97806 AND `item`=112461 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=97806 AND `item`=112462 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=97806 AND `item`=129181 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=97806 AND `item`=112458 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=97806 AND `item`=133323 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=97806 AND `item`=133325 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=97806 AND `item`=133318 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=97806 AND `item`=133319 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=97806 AND `item`=133320 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=97806 AND `item`=133321 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=97806 AND `item`=133324 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=97806 AND `item`=133322 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=97806 AND `item`=132752 AND `ExtendedCost`=0 AND `type`=1);
INSERT INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `ExtendedCost`, `type`, `PlayerConditionID`, `IgnoreFiltering`, `VerifiedBuild`) VALUES
(97806, 25, 123960, 0, 0, 1, 0, 0, 70575), -- Charm of Demonic Fire
(97806, 24, 123959, 0, 0, 1, 0, 0, 70575), -- Demon Trophy
(97806, 23, 112460, 0, 0, 1, 0, 0, 70575), -- Illidari Band
(97806, 22, 112459, 0, 0, 1, 0, 0, 70575), -- Illidari Ring
(97806, 21, 112461, 0, 0, 1, 0, 0, 70575), -- Illidari Chain
(97806, 20, 112462, 0, 0, 1, 0, 0, 70575), -- Illidari Drape
(97806, 19, 129181, 0, 0, 1, 0, 0, 70575), -- Illidari Warglaive
(97806, 18, 112458, 0, 0, 1, 0, 0, 70575), -- Illidari Warglaive
(97806, 9, 133323, 0, 0, 1, 0, 0, 70575), -- Illidari Bracers
(97806, 8, 133325, 0, 0, 1, 0, 0, 70575), -- Illidari Belt
(97806, 7, 133318, 0, 0, 1, 0, 0, 70575), -- Illidari Shoulders
(97806, 6, 133319, 0, 0, 1, 0, 0, 70575), -- Illidari Leggings
(97806, 5, 133320, 0, 0, 1, 0, 0, 70575), -- Illidari Blindfold
(97806, 4, 133321, 0, 0, 1, 0, 0, 70575), -- Illidari Gloves
(97806, 3, 133324, 0, 0, 1, 0, 0, 70575), -- Illidari Boots
(97806, 2, 133322, 0, 0, 1, 0, 0, 70575), -- Illidari Robe
(97806, 1, 132752, 0, 0, 1, 0, 0, 70575); -- Illidari Rations


DELETE FROM `creature_template_addon` WHERE `entry` IN (209307 /*209307 (Lorraine Darktide) - Cosmetic - Tidesage Book*/, 113404 /*113404 (Illidari Darkdealer)*/, 256112 /*256112 (Sothon Doomrender)*/, 259276 /*259276 (Archmage Galdrin) - Cosmetic - Tidesage Book*/, 14505 /*14505 (Dreadsteed) - Ride Vehicle Hardcoded*/, 82564 /*82564 (Archmage Nakada)*/, 85819 /*85819 (Daniel) - Wizard-in-Training Hat*/, 85818 /*85818 (Jeremy) - Wizard-in-Training Hat*/, 85817 /*85817 (Naomi) - Wizard-in-Training Hat*/, 85816 /*85816 (Elizabeth) - Wizard-in-Training Hat*/, 198506 /*198506 (Krenzen)*/, 198505 /*198505 (Blue)*/, 84751 /*84751 (Linda)*/, 84749 /*84749 (Jenna) - Wizard-in-Training Hat*/, 84745 /*84745 (Sammy) - Wizard-in-Training Hat*/, 113392 /*113392 (Illidari Enforcer) - Invisibility and Stealth Detection*/, 101004 /*101004 (Elerion Bladedancer)*/, 95791 /*95791 (Falara Nightsong)*/, 130828 /*130828 (Gordon MacKellar)*/, 113211 /*113211 (Officer Fairbanks) - The Imposter: Stormwind City Guard Stealth Detection Circle, Invisibility and Stealth Detection*/, 197611 /*197611 ((Bunny) Sessile + Gigantic AOI)*/, 230734 /*230734 (Dracthyr Stalker)*/, 261231 /*261231 (Tuuran)*/, 259970 /*259970 (Carl) - Cosmetic - TRIBUTE - Chris Burnett - Hold Magnifying Glass [DNT]*/, 185670 /*185670 (Pet) - [DND] Pet Interact Check*/, 185672 /*185672 (Mannequin)*/, 93223 /*93223 (Generic Stalker) - Fel Fire*/, 112912 /*112912 (Martin Ocejo)*/, 257683 /*257683 (Void Mail)*/, 212899 /*212899 (Greyguard Elite)*/, 165189 /*165189 (Generic Hunter Pet) - Avoidance, Pet Damage, Pet Health*/, 176233 /*176233 (Mai Kota)*/, 176229 /*176229 (Ren Arsane)*/, 416 /*416 (Imp) - Soul Leech, Override Command Demon, Avoidance, Soul Link*/, 176244 /*176244 (Maggie Camp)*/, 176226 /*176226 (Rye Tele)*/, 7386 /*7386 (White Kitten) - Pet Sit or Sleep*/, 176235 /*176235 (Erika Saka)*/, 100754 /*100754 (Stormwind Royal Guard) - Permanent Feign Death*/, 100449 /*100449 (Stormwind Royal Guard) - Permanent Feign Death*/, 100980 /*100980 (Tyrande Whisperwind)*/, 100993 /*100993 (Felblade Assassin) - Permanent Feign Death*/, 143208 /*143208 (Kayla Mills) - Read Scroll*/, 151247 /*151247 (Shauna Strattonmeier) - Cosmetic - Teaching from a Book*/, 102543 /*102543 (Felblade Assassin) - Generic Magic Invisibility*/, 102585 /*102585 (Jace Darkweaver) - Glaive Anim Replacement*/, 100460 /*100460 (Prophet Velen)*/, 100458 /*100458 (Tyrande Whisperwind)*/, 100454 /*100454 (Muradin Bronzebeard)*/, 100471 /*100471 (Gelbin Mekkatorque)*/, 100456 /*100456 (Falstad Wildhammer)*/, 100455 /*100455 (Moira Thaurissan)*/, 100457 /*100457 (Malfurion Stormrage)*/, 97296 /*97296 (Archmage Khadgar) - Raven Form*/, 175138 /*175138 (Alleria Windrunner)*/, 111190 /*111190 ("The Imposter" Quest SI:7 Courtyard Reached Kill Credit & POI Marker)*/, 176237 /*176237 (Sojurn Saka)*/, 176192 /*176192 (Xavier Carpenter)*/, 140146 /*140146 (Stormwind Infantry Captain)*/, 140022 /*140022 (Stormwind Infantry)*/, 100501 /*100501 (Weary-Eyed Mourner) - Mourner Transform*/, 176220 /*176220 (Antoinette Jardin)*/, 176203 /*176203 (Tanaya Perch)*/, 173706 /*173706 (Mummiaabu) - Cosmetic - Hold Mug Kul Tiras Tankard*/, 97978 /*97978 (Archmage Khadgar)*/, 103581 /*103581 (Baric Stormrunner) - Glaive Anim Replacement*/, 103583 /*103583 (Cailyn Paledoom) - Glaive Anim Replacement*/, 103582 /*103582 (Belath Dawnblade) - Glaive Anim Replacement*/, 96666 /*96666 (Kayn Sunfury) - Glaive Anim Replacement*/, 103588 /*103588 (Zaria Shadowheart) - Glaive Anim Replacement*/, 103584 /*103584 (Izal Whitemoon) - Glaive Anim Replacement*/, 103587 /*103587 (Tirathon Saltheril) - Glaive Anim Replacement*/, 103586 /*103586 (Sirius Ebonwing) - Glaive Anim Replacement*/, 100044 /*100044 (Sevis Brightflame) - Glaive Anim Replacement*/, 100041 /*100041 (Lyana Darksorrow) - Glaive Anim Replacement*/, 100040 /*100040 (Jace Darkweaver) - Glaive Anim Replacement*/, 100043 /*100043 (Cassiel Nightthorn) - Glaive Anim Replacement*/, 100042 /*100042 (Marius Felbane) - Glaive Anim Replacement*/, 100037 /*100037 (Kor'vas Bloodthorn) - Glaive Anim Replacement*/, 100036 /*100036 (Falara Nightsong) - Glaive Anim Replacement*/, 100039 /*100039 (Cyana Nightglaive) - Glaive Anim Replacement*/, 100038 /*100038 (Asha Ravensong) - Glaive Anim Replacement*/, 100035 /*100035 (Allari the Souleater) - Glaive Anim Replacement*/, 101585 /*101585 (Legion Gateway)*/, 99967 /*99967 (Falara Nightsong) - Glaive Anim Replacement, Dual Wield*/, 99968 /*99968 (Kor'vas Bloodthorn) - Glaive Anim Replacement, Dual Wield*/, 99966 /*99966 (Asha Ravensong) - Glaive Anim Replacement, Dual Wield*/, 100065 /*100065 (Cassiel Nightthorn) - Glaive Anim Replacement, Dual Wield*/, 100066 /*100066 (Sevis Brightflame) - Glaive Anim Replacement, Dual Wield*/, 97273 /*97273 (Kayn Sunfury) - Glaive Anim Replacement, Dual Wield*/, 99965 /*99965 (Marius Felbane) - Glaive Anim Replacement, Dual Wield*/, 99969 /*99969 (Allari the Souleater) - Glaive Anim Replacement*/, 101446 /*101446 (Fel Annihilation)*/, 99963 /*99963 (Cyana Nightglaive) - Glaive Anim Replacement, Dual Wield*/, 99962 /*99962 (Lyana Darksorrow) - Glaive Anim Replacement, Dual Wield*/, 99964 /*99964 (Jace Darkweaver) - Glaive Anim Replacement, Dual Wield*/, 96783 /*96783 (Bastillax)*/, 72676 /*72676 (Spirit Healer) - Ghost, Shroud of Death*/, 97265 /*97265 (Kayn Sunfury) - Glaive Anim Replacement, Dual Wield*/, 97267 /*97267 (Altruis the Sufferer) - Glaive Anim Replacement, Dual Wield*/, 97644 /*97644 (Kor'vas Bloodthorn) - Glaive Anim Replacement*/, 97225 /*97225 (Wrathguard Legate) - Permanent Feign Death (Stun, Untrackable, Immune)*/, 92782 /*92782 (Savage Felstalker) - Permanent Feign Death (Stun, Untrackable, Immune)*/, 96680 /*96680 (Glazer) - Focusing*/, 102391 /*102391 (Altruis the Sufferer) - Glaive Anim Replacement, Cosmetic - Purple Cat Mark State (5.00)*/, 99731 /*99731 (Countermeasures) - Countermeasure*/, 102393 /*102393 (Kayn Sunfury) - Glaive Anim Replacement*/, 99732 /*99732 (Countermeasures) - Countermeasure*/, 99709 /*99709 (Countermeasures) - Countermeasure*/, 99710 /*99710 (Freeze Statue)*/, 96681 /*96681 (Ash'golm) - 99% Damage Reduction*/, 101406 /*101406 (Fel Spirit) - Fel Spirit*/, 96672 /*96672 (Cyana Nightglaive) - Glaive Anim Replacement, Illidari Area Trigger*/, 100724 /*100724 (Freed Illidari) - Dual Wield*/, 101332 /*101332 (Burning Fel)*/, 96682 /*96682 (Immolanth) - Zero Energy + Zero Regen, Dual Wield*/, 97334 /*97334 (Kurloth)*/, 100667 /*100667 (Jin Featherfall)*/, 114358 /*114358 (Legion Portal)*/, 96645 /*96645 (Vault Warden) - Permanent Feign Death (Stun, Untrackable, Immune)*/, 97806 /*97806 (Falara Nightsong) - Glaive Anim Replacement*/, 97643 /*97643 (Kor'vas Bloodthorn) - Glaive Anim Replacement*/, 96675 /*96675 (Allari the Souleater) - Glaive Anim Replacement*/, 99443 /*99443 (Vampiric Felbat)*/, 101901 /*101901 (Explosive Inquisitor Eye) - Explosive Shadows*/, 101895 /*101895 (Eye Dest)*/, 97070 /*97070 (Jace Darkweaver) - Glaive Anim Replacement*/, 96659 /*96659 (Vampiric Felbat)*/, 100298 /*100298 (Marius Felbane) - Glaive Anim Replacement, Dual Wield*/, 97632 /*97632 (Crusher)*/, 92990 /*92990 (Sledge)*/, 92985 /*92985 (Altruis the Sufferer) - Glaive Anim Replacement, Dual Wield*/, 92984 /*92984 (Kayn Sunfury) - Glaive Anim Replacement, Dual Wield*/, 100296 /*100296 (Cassiel Nightthorn) - Glaive Anim Replacement, Dual Wield*/, 96665 /*96665 (Kayn Sunfury) - Glaive Anim Replacement*/, 100299 /*100299 (Lyana Darksorrow) - Glaive Anim Replacement, Dual Wield*/, 59115 /*59115 (Generic Bunny) - Legion Shield*/, 97614 /*97614 (Allari the Souleater) - Glaive Anim Replacement*/, 97618 /*97618 (Falara Nightsong) - Glaive Anim Replacement*/, 97616 /*97616 (Cyana Nightglaive) - Glaive Anim Replacement*/, 97622 /*97622 (Kor'vas Bloodthorn) - Glaive Anim Replacement*/, 97619 /*97619 (Marius Felbane) - Glaive Anim Replacement*/, 97615 /*97615 (Jace Darkweaver) - Glaive Anim Replacement*/, 97617 /*97617 (Asha Ravensong) - Glaive Anim Replacement*/, 97620 /*97620 (Lyana Darksorrow) - Glaive Anim Replacement*/, 96997 /*96997 (Kethrazor)*/, 99451 /*99451 (Mirana Starlight) - Dual Wield, Bull-headed, Illidari Area Trigger*/, 92986 /*92986 (Altruis the Sufferer) - Glaive Anim Replacement*/, 92980 /*92980 (Kayn Sunfury) - Glaive Anim Replacement*/, 99631 /*99631 (Kayn Sunfury) - Glaive Anim Replacement*/, 99632 /*99632 (Altruis the Sufferer) - Glaive Anim Replacement*/, 99501 /*99501 (Legion Portal)*/);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(209307, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '274046'), -- 209307 (Lorraine Darktide) - Cosmetic - Tidesage Book
(113404, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 113404 (Illidari Darkdealer)
(256112, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 256112 (Sothon Doomrender)
(259276, 0, 0, 0, 0, 0, 0, 0, 0, 31362, 0, 0, 0, '274046'), -- 259276 (Archmage Galdrin) - Cosmetic - Tidesage Book
(14505, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '46598'), -- 14505 (Dreadsteed) - Ride Vehicle Hardcoded
(82564, 0, 0, 0, 0, 0, 1, 1, 0, 6908, 0, 0, 0, ''), -- 82564 (Archmage Nakada)
(85819, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '169394'), -- 85819 (Daniel) - Wizard-in-Training Hat
(85818, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '169394'), -- 85818 (Jeremy) - Wizard-in-Training Hat
(85817, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '169394'), -- 85817 (Naomi) - Wizard-in-Training Hat
(85816, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '169394'), -- 85816 (Elizabeth) - Wizard-in-Training Hat
(198506, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, ''), -- 198506 (Krenzen)
(198505, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, ''), -- 198505 (Blue)
(84751, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, ''), -- 84751 (Linda)
(84749, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '169394'), -- 84749 (Jenna) - Wizard-in-Training Hat
(84745, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '169394'), -- 84745 (Sammy) - Wizard-in-Training Hat
(113392, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '18950'), -- 113392 (Illidari Enforcer) - Invisibility and Stealth Detection
(101004, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 101004 (Elerion Bladedancer)
(95791, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 95791 (Falara Nightsong)
(130828, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, ''), -- 130828 (Gordon MacKellar)
(113211, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, '221508 18950'), -- 113211 (Officer Fairbanks) - The Imposter: Stormwind City Guard Stealth Detection Circle, Invisibility and Stealth Detection
(197611, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 4, ''), -- 197611 ((Bunny) Sessile + Gigantic AOI)
(230734, 0, 0, 8, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 230734 (Dracthyr Stalker)
(261231, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, ''), -- 261231 (Tuuran)
(259970, 0, 0, 5, 0, 0, 1, 0, 0, 36126, 0, 0, 0, '1282342'), -- 259970 (Carl) - Cosmetic - TRIBUTE - Chris Burnett - Hold Magnifying Glass [DNT]
(185670, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '1288600'), -- 185670 (Pet) - [DND] Pet Interact Check
(185672, 0, 0, 0, 0, 0, 1, 0, 0, 21382, 0, 0, 0, ''), -- 185672 (Mannequin)
(93223, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, '224469'), -- 93223 (Generic Stalker) - Fel Fire
(112912, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, ''), -- 112912 (Martin Ocejo)
(257683, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 257683 (Void Mail)
(212899, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 212899 (Greyguard Elite)
(165189, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '65220 8875 19581'), -- 165189 (Generic Hunter Pet) - Avoidance, Pet Damage, Pet Health
(176233, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, ''), -- 176233 (Mai Kota)
(176229, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, ''), -- 176229 (Ren Arsane)
(416, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '108370 119904 32233 108446'), -- 416 (Imp) - Soul Leech, Override Command Demon, Avoidance, Soul Link
(176244, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, ''), -- 176244 (Maggie Camp)
(176226, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, ''), -- 176226 (Rye Tele)
(7386, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '96573'), -- 7386 (White Kitten) - Pet Sit or Sleep
(176235, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, ''), -- 176235 (Erika Saka)
(100754, 0, 0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, '130966'), -- 100754 (Stormwind Royal Guard) - Permanent Feign Death
(100449, 0, 0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, '130966'), -- 100449 (Stormwind Royal Guard) - Permanent Feign Death
(100980, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, ''), -- 100980 (Tyrande Whisperwind)
(100993, 0, 0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, '130966'), -- 100993 (Felblade Assassin) - Permanent Feign Death
(143208, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '133464'), -- 143208 (Kayla Mills) - Read Scroll
(151247, 0, 0, 5, 0, 0, 1, 0, 0, 0, 0, 0, 0, '292924'), -- 151247 (Shauna Strattonmeier) - Cosmetic - Teaching from a Book
(102543, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '191143'), -- 102543 (Felblade Assassin) - Generic Magic Invisibility
(102585, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 102585 (Jace Darkweaver) - Glaive Anim Replacement
(100460, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, ''), -- 100460 (Prophet Velen)
(100458, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 100458 (Tyrande Whisperwind)
(100454, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 100454 (Muradin Bronzebeard)
(100471, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 100471 (Gelbin Mekkatorque)
(100456, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 100456 (Falstad Wildhammer)
(100455, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, ''), -- 100455 (Moira Thaurissan)
(100457, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 100457 (Malfurion Stormrage)
(97296, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, '187983'), -- 97296 (Archmage Khadgar) - Raven Form
(175138, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 3, ''), -- 175138 (Alleria Windrunner)
(111190, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 5, ''), -- 111190 ("The Imposter" Quest SI:7 Courtyard Reached Kill Credit & POI Marker)
(176237, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, ''), -- 176237 (Sojurn Saka)
(176192, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, ''), -- 176192 (Xavier Carpenter)
(140146, 0, 28912, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 140146 (Stormwind Infantry Captain)
(140022, 0, 0, 0, 0, 0, 1, 0, 0, 15998, 0, 0, 0, ''), -- 140022 (Stormwind Infantry)
(100501, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, '199386'), -- 100501 (Weary-Eyed Mourner) - Mourner Transform
(176220, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, ''), -- 176220 (Antoinette Jardin)
(176203, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, ''), -- 176203 (Tanaya Perch)
(173706, 0, 0, 0, 0, 0, 0, 1, 0, 14677, 0, 0, 0, '263410'), -- 173706 (Mummiaabu) - Cosmetic - Hold Mug Kul Tiras Tankard
(97978, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 97978 (Archmage Khadgar)
(103581, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '222589'), -- 103581 (Baric Stormrunner) - Glaive Anim Replacement
(103583, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 103583 (Cailyn Paledoom) - Glaive Anim Replacement
(103582, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 103582 (Belath Dawnblade) - Glaive Anim Replacement
(96666, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 96666 (Kayn Sunfury) - Glaive Anim Replacement
(103588, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 103588 (Zaria Shadowheart) - Glaive Anim Replacement
(103584, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 103584 (Izal Whitemoon) - Glaive Anim Replacement
(103587, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '222589'), -- 103587 (Tirathon Saltheril) - Glaive Anim Replacement
(103586, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 103586 (Sirius Ebonwing) - Glaive Anim Replacement
(100044, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 100044 (Sevis Brightflame) - Glaive Anim Replacement
(100041, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '222589'), -- 100041 (Lyana Darksorrow) - Glaive Anim Replacement
(100040, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 100040 (Jace Darkweaver) - Glaive Anim Replacement
(100043, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 100043 (Cassiel Nightthorn) - Glaive Anim Replacement
(100042, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 100042 (Marius Felbane) - Glaive Anim Replacement
(100037, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 100037 (Kor'vas Bloodthorn) - Glaive Anim Replacement
(100036, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 100036 (Falara Nightsong) - Glaive Anim Replacement
(100039, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 100039 (Cyana Nightglaive) - Glaive Anim Replacement
(100038, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '222589'), -- 100038 (Asha Ravensong) - Glaive Anim Replacement
(100035, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 100035 (Allari the Souleater) - Glaive Anim Replacement
(101585, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 101585 (Legion Gateway)
(99967, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589 42459'), -- 99967 (Falara Nightsong) - Glaive Anim Replacement, Dual Wield
(99968, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589 42459'), -- 99968 (Kor'vas Bloodthorn) - Glaive Anim Replacement, Dual Wield
(99966, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589 42459'), -- 99966 (Asha Ravensong) - Glaive Anim Replacement, Dual Wield
(100065, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589 42459'), -- 100065 (Cassiel Nightthorn) - Glaive Anim Replacement, Dual Wield
(100066, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589 42459'), -- 100066 (Sevis Brightflame) - Glaive Anim Replacement, Dual Wield
(97273, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589 42459'), -- 97273 (Kayn Sunfury) - Glaive Anim Replacement, Dual Wield
(99965, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589 42459'), -- 99965 (Marius Felbane) - Glaive Anim Replacement, Dual Wield
(99969, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 99969 (Allari the Souleater) - Glaive Anim Replacement
(101446, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 101446 (Fel Annihilation)
(99963, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589 42459'), -- 99963 (Cyana Nightglaive) - Glaive Anim Replacement, Dual Wield
(99962, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589 42459'), -- 99962 (Lyana Darksorrow) - Glaive Anim Replacement, Dual Wield
(99964, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589 42459'), -- 99964 (Jace Darkweaver) - Glaive Anim Replacement, Dual Wield
(96783, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 96783 (Bastillax)
(72676, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '9036 10848'), -- 72676 (Spirit Healer) - Ghost, Shroud of Death
(97265, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589 42459'), -- 97265 (Kayn Sunfury) - Glaive Anim Replacement, Dual Wield
(97267, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589 42459'), -- 97267 (Altruis the Sufferer) - Glaive Anim Replacement, Dual Wield
(97644, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 97644 (Kor'vas Bloodthorn) - Glaive Anim Replacement
-- 97225/92782/96645: 145363 is per-spawn, not per-template (sniffed corpse
-- spawns in Demon Ward/Warden's Court only; live spawns carry no aura).
-- The corpse guids already get it via creature_addon.
(97225, 0, 0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, ''), -- 97225 (Wrathguard Legate) - feign death moved to creature_addon per guid
(92782, 0, 0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, ''), -- 92782 (Savage Felstalker) - feign death moved to creature_addon per guid
(96680, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, '191915'), -- 96680 (Glazer) - Focusing
(102391, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589 202212'), -- 102391 (Altruis the Sufferer) - Glaive Anim Replacement, Cosmetic - Purple Cat Mark State (5.00)
(99731, 0, 0, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, '195189'), -- 99731 (Countermeasures) - Countermeasure
(102393, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 102393 (Kayn Sunfury) - Glaive Anim Replacement
(99732, 0, 0, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, '195189'), -- 99732 (Countermeasures) - Countermeasure
(99709, 0, 0, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, '195189'), -- 99709 (Countermeasures) - Countermeasure
(99710, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 99710 (Freeze Statue)
(96681, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '101386'), -- 96681 (Ash'golm) - 99% Damage Reduction
(101406, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '199831'), -- 101406 (Fel Spirit) - Fel Spirit
(96672, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589 195834'), -- 96672 (Cyana Nightglaive) - Glaive Anim Replacement, Illidari Area Trigger
(100724, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '42459'), -- 100724 (Freed Illidari) - Dual Wield
(101332, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 101332 (Burning Fel)
(96682, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, '72242 42459'), -- 96682 (Immolanth) - Zero Energy + Zero Regen, Dual Wield
(97334, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 97334 (Kurloth)
(100667, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, ''), -- 100667 (Jin Featherfall)
(114358, 0, 0, 0, 0, 0, 1, 0, 0, 3761, 0, 0, 0, ''), -- 114358 (Legion Portal)
(96645, 0, 0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, ''), -- 96645 (Vault Warden) - feign death moved to creature_addon per guid
(97806, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '222589'), -- 97806 (Falara Nightsong) - Glaive Anim Replacement
(97643, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 97643 (Kor'vas Bloodthorn) - Glaive Anim Replacement
(96675, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 96675 (Allari the Souleater) - Glaive Anim Replacement
(99443, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 99443 (Vampiric Felbat)
(101901, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '201028'), -- 101901 (Explosive Inquisitor Eye) - Explosive Shadows
(101895, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 101895 (Eye Dest)
(97070, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 97070 (Jace Darkweaver) - Glaive Anim Replacement
(96659, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 96659 (Vampiric Felbat)
(100298, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589 42459'), -- 100298 (Marius Felbane) - Glaive Anim Replacement, Dual Wield
(97632, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 97632 (Crusher)
(92990, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 92990 (Sledge)
(92985, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589 42459'), -- 92985 (Altruis the Sufferer) - Glaive Anim Replacement, Dual Wield
(92984, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589 42459'), -- 92984 (Kayn Sunfury) - Glaive Anim Replacement, Dual Wield
(100296, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589 42459'), -- 100296 (Cassiel Nightthorn) - Glaive Anim Replacement, Dual Wield
(96665, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 96665 (Kayn Sunfury) - Glaive Anim Replacement
(100299, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589 42459'), -- 100299 (Lyana Darksorrow) - Glaive Anim Replacement, Dual Wield
(59115, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, '223626'), -- 59115 (Generic Bunny) - Legion Shield
(97614, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 97614 (Allari the Souleater) - Glaive Anim Replacement
(97618, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 97618 (Falara Nightsong) - Glaive Anim Replacement
(97616, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 97616 (Cyana Nightglaive) - Glaive Anim Replacement
(97622, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 97622 (Kor'vas Bloodthorn) - Glaive Anim Replacement
(97619, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 97619 (Marius Felbane) - Glaive Anim Replacement
(97615, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 97615 (Jace Darkweaver) - Glaive Anim Replacement
(97617, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 97617 (Asha Ravensong) - Glaive Anim Replacement
(97620, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '222589'), -- 97620 (Lyana Darksorrow) - Glaive Anim Replacement
(96997, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 96997 (Kethrazor)
(99451, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '42459 200401 195834'), -- 99451 (Mirana Starlight) - Dual Wield, Bull-headed, Illidari Area Trigger
(92986, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '222589'), -- 92986 (Altruis the Sufferer) - Glaive Anim Replacement
(92980, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '222589'), -- 92980 (Kayn Sunfury) - Glaive Anim Replacement
(99631, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '222589'), -- 99631 (Kayn Sunfury) - Glaive Anim Replacement
(99632, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '222589'), -- 99632 (Altruis the Sufferer) - Glaive Anim Replacement
(99501, 0, 0, 0, 0, 0, 1, 0, 0, 3761, 0, 0, 0, ''); -- 99501 (Legion Portal)

UPDATE `creature_template_addon` SET `aiAnimKit`=1320 WHERE `entry`=114246; -- 114246 (Karl Wogksch)
UPDATE `creature_template_addon` SET `auras`='394615' WHERE `entry`=198482; -- 198482 (Krozhalan) - Cosmetic - Eat
UPDATE `creature_template_addon` SET `auras`='394615' WHERE `entry`=198529; -- 198529 (Kotheshal) - Cosmetic - Eat
UPDATE `creature_template_addon` SET `auras`='182641' WHERE `entry`=152643; -- 152643 (Despawn Bunny) - Game Object Despawn Periodic
UPDATE `creature_template_addon` SET `visibilityDistanceType`=3, `auras`='46598 96850 16245' WHERE `entry`=52361; -- 52361 (Toy Cart Bunny) - Ride Vehicle Hardcoded, Toy Cart Contents, Freeze Anim
UPDATE `creature_template_addon` SET `auras`='46598 96847' WHERE `entry`=52359; -- 52359 (Toy Cart) - Ride Vehicle Hardcoded, Blue Doll State
UPDATE `creature_template_addon` SET `aiAnimKit`=645 WHERE `entry`=52358; -- 52358 (Craggle Wobbletop)
UPDATE `creature_template_addon` SET `PvpFlags`=1 WHERE `entry`=176230; -- 176230 (Saka Kidd)
UPDATE `creature_template_addon` SET `auras`='86603' WHERE `entry`=61838; -- 61838 (Gavin Marlsbury) - Stealth
UPDATE `creature_template_addon` SET `auras`='221508 18950' WHERE `entry`=14423; -- 14423 (Officer Jaxon) - The Imposter: Stormwind City Guard Stealth Detection Circle, Invisibility and Stealth Detection
UPDATE `creature_template_addon` SET `PvpFlags`=0, `auras`='46598' WHERE `entry`=50525; -- 50525 (Chris Miller) - Ride Vehicle Hardcoded
UPDATE `creature_template_addon` SET `aiAnimKit`=1193 WHERE `entry`=50523; -- 50523 (Kyle Radue)
UPDATE `creature_template_addon` SET `PvpFlags`=0 WHERE `entry`=35365; -- 35365 (Behsten)
UPDATE `creature_template_addon` SET `visibilityDistanceType`=3 WHERE `entry`=50088; -- 50088 (Waters of Farseeing)
UPDATE `creature_template_addon` SET `auras`='' WHERE `entry`=38821; -- 38821 (Dave's Industrial Light and Magic Bunny (Medium)(Sessile))
UPDATE `creature_template_addon` SET `auras`='221508' WHERE `entry`=47688; -- 47688 (Officer Connelly) - The Imposter: Stormwind City Guard Stealth Detection Circle
UPDATE `creature_template_addon` SET `auras`='65525 33207 33208' WHERE `entry`=18927; -- 18927 (Human Commoner) - Gossip NPC Appearance - Hallow's End, Gossip NPC Periodic - Fidget, Gossip NPC Periodic - Talk
UPDATE `creature_template_addon` SET `auras`='79976' WHERE `entry`=61840; -- 61840 (Naanae) - Retribution Aura
UPDATE `creature_template_addon` SET `StandState`=3 WHERE `entry`=61841; -- 61841 (Brunn Goldenmug)


UPDATE `creature_template_difficulty` SET `HealthModifier`=1.049999952316284179 WHERE (`Entry`=279 AND `DifficultyID`=0); -- Morgan Pestle
UPDATE `creature_template_difficulty` SET `HealthModifier`=1.049999952316284179 WHERE (`Entry`=4961 AND `DifficultyID`=0); -- Dashel Stonefist


UPDATE `creature_template_difficulty` SET `VerifiedBuild`=70575 WHERE (`DifficultyID`=0 AND `Entry` IN (5496,5495,1435,461,5520,209307,1733,147714,9584,1419,155739,1307,148124,147741,204198,206381,206596,85819,85818,85817,85816,84749,84745,1305,1308,1304,130828,114246,11827,11828,297,5504,5506,49893,197287,1314,201312,1347,1346,42288,185670,185669,1300,5386,1316,338,1318,1317,1315,1312,5493,11068,46180,5494,252312,43692,1431,1428,6740,2457,277,2456,2455,43822,1301,53949,4974,5193,43769,43820,43819,242550,242566,51440,46602,1275,1257,1299,1298,1297,175420,482,49877,4981,1402,1432,8670,483,40435,15659,12481,48632,8719,1291,1286,1285,6174,1427,12480,179896,1295,1294,3518,1289,1287,18375,35477,18362,18360,18359,18357,1303,1302,18376,18406,4961,44583,44582,1366,11096,1367,23050,5565,5564,1292,1321,1320,1395,53641,55684,28347,8666,28355,1319,7386,2795,1339,1333,62954,1752,2504,143208,44243,44241,44239,44238,1440,151256,1756,1751,1750,34997,93307,24729,51938,35365,34998,100458,100472,189603,1439,2285,4960,721,5483,165505,209790,5482,32520,189602,1976,4075,44237,1478,3629,1477,100501,6089,340,1327,49540,6090,1328,173706,42339,2198,1412,68,101647));
UPDATE `creature_template_difficulty` SET `StaticFlags5`=0x1, `VerifiedBuild`=70575 WHERE (`Entry`=103033 AND `DifficultyID`=0); -- 103033 (Enchanted Broom) - CannotTurn - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=113404 AND `DifficultyID`=0); -- 113404 (Illidari Darkdealer) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=3313, `StaticFlags1`=0x10000000, `StaticFlags3`=0x40000, `VerifiedBuild`=70575 WHERE (`Entry`=256112 AND `DifficultyID`=0); -- 256112 (Sothon Doomrender) - CanSwim - AllowInteractionWhileInCombat
UPDATE `creature_template_difficulty` SET `ContentTuningID`=3321, `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=259276 AND `DifficultyID`=0); -- 259276 (Archmage Galdrin) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags5`=0x1, `VerifiedBuild`=70575 WHERE (`Entry`=207684 AND `DifficultyID`=0); -- 207684 (Stagecoach) - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `VerifiedBuild`=70575 WHERE (`Entry`=14505 AND `DifficultyID`=0); -- 14505 (Dreadsteed) - Sessile
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=82564 AND `DifficultyID`=0); -- 82564 (Archmage Nakada) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=149121 AND `DifficultyID`=0); -- 149121 (Summoned Guardian) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=198509 AND `DifficultyID`=0); -- 198509 (Berkeley) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=198506 AND `DifficultyID`=0); -- 198506 (Krenzen) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=198505 AND `DifficultyID`=0); -- 198505 (Blue) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=84751 AND `DifficultyID`=0); -- 84751 (Linda) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=149124 AND `DifficultyID`=0); -- 149124 (Arcane Golem) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=113392 AND `DifficultyID`=0); -- 113392 (Illidari Enforcer) - CanSwim - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `VerifiedBuild`=70575 WHERE (`Entry`=109912 AND `DifficultyID`=0); -- 109912 (Captive Wyrmtongue) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=101004 AND `DifficultyID`=0); -- 101004 (Elerion Bladedancer) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2991, `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=230765 AND `DifficultyID`=0); -- 230765 (Dracthyr Occultist) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=95791 AND `DifficultyID`=0); -- 95791 (Falara Nightsong) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=197610 AND `DifficultyID`=0); -- 197610 (Knight-Captain Sanine) - CanSwim - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=197609 AND `DifficultyID`=0); -- 197609 (Otan Tennoh) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=113211 AND `DifficultyID`=0); -- 113211 (Officer Fairbanks) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `StaticFlags4`=0x2000000, `StaticFlags5`=0x1, `VerifiedBuild`=70575 WHERE (`Entry`=197611 AND `DifficultyID`=0); -- 197611 ((Bunny) Sessile + Gigantic AOI) - Sessile - GiganticAoi - HideInCombatLog - UntargetableByClient
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2991, `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=230734 AND `DifficultyID`=0); -- 230734 (Dracthyr Stalker) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=868, `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=256071 AND `DifficultyID`=0); -- 256071 (Solelo) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2164, `StaticFlags1`=0x100, `StaticFlags3`=0x2000000, `StaticFlags5`=0x1, `VerifiedBuild`=70575 WHERE (`Entry`=256286 AND `DifficultyID`=0); -- 256286 (Azerothian Athenaeum) - Sessile - CannotTurn - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=198482 AND `DifficultyID`=0); -- 198482 (Krozhalan) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=864, `VerifiedBuild`=70575 WHERE (`Entry`=261231 AND `DifficultyID`=0); -- 261231 (Tuuran) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=198529 AND `DifficultyID`=0); -- 198529 (Kotheshal) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=198579 AND `DifficultyID`=0); -- 198579 (Valarian) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=3321, `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=259970 AND `DifficultyID`=0); -- 259970 (Carl) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=52031 AND `DifficultyID`=0); -- 52031 (Sarana Damir) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=30713 AND `DifficultyID`=0); -- 30713 (Catarina Stanford) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=30730 AND `DifficultyID`=0); -- 30730 (Stanly McCormick) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=185468 AND `DifficultyID`=0); -- 185468 (Tawny Seabraid) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=185467 AND `DifficultyID`=0); -- 185467 (Wilder Seabraid) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=198581 AND `DifficultyID`=0); -- 198581 (Sam Corff) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=49701 AND `DifficultyID`=0); -- 49701 (Jon Casper) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `VerifiedBuild`=70575 WHERE (`Entry`=152643 AND `DifficultyID`=0); -- 152643 (Despawn Bunny) - Sessile
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `VerifiedBuild`=70575 WHERE (`Entry`=199602 AND `DifficultyID`=0); -- 199602 ([DND] Fireworks) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=198589 AND `DifficultyID`=0); -- 198589 (Assistant Dilya) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `VerifiedBuild`=70575 WHERE (`Entry`=185672 AND `DifficultyID`=0); -- 185672 (Mannequin) - Sessile
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `VerifiedBuild`=70575 WHERE (`Entry`=183978 AND `DifficultyID`=0); -- 183978 (Important Camera Dog) - Sessile
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x80000100, `StaticFlags4`=0x2000000, `StaticFlags5`=0x1, `VerifiedBuild`=70575 WHERE (`Entry`=93223 AND `DifficultyID`=0); -- 93223 (Generic Stalker) - Sessile, LargeAoi - HideInCombatLog - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=61896 AND `DifficultyID`=0); -- 61896 (Josie) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=162935 AND `DifficultyID`=0); -- 162935 (Stormwind City Guard) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=198531 AND `DifficultyID`=0); -- 198531 (Zaralash) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=112912 AND `DifficultyID`=0); -- 112912 (Martin Ocejo) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=175416 AND `DifficultyID`=0); -- 175416 (Miggi) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x0, `VerifiedBuild`=70575 WHERE (`Entry`=279 AND `DifficultyID`=0); -- 279 (Morgan Pestle) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=482, `StaticFlags1`=0x10000100, `StaticFlags5`=0x2001, `VerifiedBuild`=70575 WHERE (`Entry`=257683 AND `DifficultyID`=0); -- 257683 (Void Mail) - Sessile, CanSwim - UntargetableByClient, SuppressHighlightWhenTargetedOrMousedOver
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=212899 AND `DifficultyID`=0); -- 212899 (Greyguard Elite) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=50669 AND `DifficultyID`=0); -- 50669 (Dawn Radue) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=29093 AND `DifficultyID`=0); -- 29093 (Ian Drake) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=29142 AND `DifficultyID`=0); -- 29142 (Jelinek Sharpshear) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=49748 AND `DifficultyID`=0); -- 49748 (Hero's Herald) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=198551 AND `DifficultyID`=0); -- 198551 (Thaza) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=44774 AND `DifficultyID`=0); -- 44774 (Genevieve) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=198533 AND `DifficultyID`=0); -- 198533 (Nazurathar) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=198532 AND `DifficultyID`=0); -- 198532 (Zhakloskar) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=141270 AND `DifficultyID`=0); -- 141270 (Pepper) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=44773 AND `DifficultyID`=0); -- 44773 (Anastasia) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x90040100, `VerifiedBuild`=70575 WHERE (`Entry`=52361 AND `DifficultyID`=0); -- 52361 (Toy Cart Bunny) - Sessile, Aquatic, CanSwim, LargeAoi
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `VerifiedBuild`=70575 WHERE (`Entry`=52359 AND `DifficultyID`=0); -- 52359 (Toy Cart) - Sessile
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=52358 AND `DifficultyID`=0); -- 52358 (Craggle Wobbletop) - Amphibious, CanSwim - CannotTurn
UPDATE `creature_template_difficulty` SET `ContentTuningID`=623, `VerifiedBuild`=70575 WHERE (`Entry`=253183 AND `DifficultyID`=0); -- 253183 (Chatty Contender) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=623, `VerifiedBuild`=70575 WHERE (`Entry`=253181 AND `DifficultyID`=0); -- 253181 (Babbling Brawler) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x0, `VerifiedBuild`=70575 WHERE (`Entry`=2334 AND `DifficultyID`=0); -- 2334 (Event Generator 001) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=176233 AND `DifficultyID`=0); -- 176233 (Mai Kota) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=176230 AND `DifficultyID`=0); -- 176230 (Saka Kidd) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=176229 AND `DifficultyID`=0); -- 176229 (Ren Arsane) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=14439 AND `DifficultyID`=0); -- 14439 (Officer Brady) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=61836 AND `DifficultyID`=0); -- 61836 (Moni Widdlesprock) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=61834 AND `DifficultyID`=0); -- 61834 (Alyn Black) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=14438 AND `DifficultyID`=0); -- 14438 (Officer Pomeroy) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=50524 AND `DifficultyID`=0); -- 50524 (Travis Nichols) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=176244 AND `DifficultyID`=0); -- 176244 (Maggie Camp) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=176226 AND `DifficultyID`=0); -- 176226 (Rye Tele) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=61838 AND `DifficultyID`=0); -- 61838 (Gavin Marlsbury) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=56925 AND `DifficultyID`=0); -- 56925 (Farrah Facet) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=50480 AND `DifficultyID`=0); -- 50480 (Isabel Jones) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=14423 AND `DifficultyID`=0); -- 14423 (Officer Jaxon) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=198526 AND `DifficultyID`=0); -- 198526 (Nirathan) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=61837 AND `DifficultyID`=0); -- 61837 (Leria Nightwind) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `VerifiedBuild`=70575 WHERE (`Entry`=242173 AND `DifficultyID`=0); -- 242173 (Generic Bunny) - Sessile - CannotTurn
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=87501 AND `DifficultyID`=0); -- 87501 (Paulie) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080100, `VerifiedBuild`=70575 WHERE (`Entry`=50525 AND `DifficultyID`=0); -- 50525 (Chris Miller) - Sessile, Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=50523 AND `DifficultyID`=0); -- 50523 (Kyle Radue) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=201230 AND `DifficultyID`=0); -- 201230 (Mizzen) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=176235 AND `DifficultyID`=0); -- 176235 (Erika Saka) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100399 AND `DifficultyID`=0); -- 100399 (Magus Solgaze) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100754 AND `DifficultyID`=0); -- 100754 (Stormwind Royal Guard) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `VerifiedBuild`=70575 WHERE (`Entry`=102554 AND `DifficultyID`=0); -- 102554 (Muradin Bronzebeard) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `VerifiedBuild`=70575 WHERE (`Entry`=100981 AND `DifficultyID`=0); -- 100981 (Prophet Velen) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `VerifiedBuild`=70575 WHERE (`Entry`=100980 AND `DifficultyID`=0); -- 100980 (Tyrande Whisperwind) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `VerifiedBuild`=70575 WHERE (`Entry`=100979 AND `DifficultyID`=0); -- 100979 (Moira Thaurissan) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `VerifiedBuild`=70575 WHERE (`Entry`=100978 AND `DifficultyID`=0); -- 100978 (Gelbin Mekkatorque) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `VerifiedBuild`=70575 WHERE (`Entry`=100977 AND `DifficultyID`=0); -- 100977 (Malfurion Stormrage) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100975 AND `DifficultyID`=0); -- 100975 (Falstad Wildhammer) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `VerifiedBuild`=70575 WHERE (`Entry`=100974 AND `DifficultyID`=0); -- 100974 (Aysa Cloudsinger) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `VerifiedBuild`=70575 WHERE (`Entry`=100973 AND `DifficultyID`=0); -- 100973 (Anduin Wrynn) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=356, `VerifiedBuild`=70575 WHERE (`Entry`=5806 AND `DifficultyID`=0); -- 5806 (Treant Ally) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=101055 AND `DifficultyID`=0); -- 101055 (Jace Darkweaver) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100993 AND `DifficultyID`=0); -- 100993 (Felblade Assassin) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=151255 AND `DifficultyID`=0); -- 151255 (James Meier) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=151251 AND `DifficultyID`=0); -- 151251 (Jordan Meier) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=151249 AND `DifficultyID`=0); -- 151249 (Attentive Child) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=151247 AND `DifficultyID`=0); -- 151247 (Shauna Strattonmeier) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=102543 AND `DifficultyID`=0); -- 102543 (Felblade Assassin) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `VerifiedBuild`=70575 WHERE (`Entry`=72654 AND `DifficultyID`=0); -- 72654 (General Purpose Stalker (RKS)) - Sessile
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags5`=0x100, `VerifiedBuild`=70575 WHERE (`Entry`=100429 AND `DifficultyID`=0); -- 100429 (Anduin Wrynn) - CanSwim - InteractWhileHostile
UPDATE `creature_template_difficulty` SET `ContentTuningID`=699, `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=102585 AND `DifficultyID`=0); -- 102585 (Jace Darkweaver) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=111600 AND `DifficultyID`=0); -- 111600 (Stormwind Royal Guard) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100460 AND `DifficultyID`=0); -- 100460 (Prophet Velen) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100454 AND `DifficultyID`=0); -- 100454 (Muradin Bronzebeard) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=15187 AND `DifficultyID`=0); -- 15187 (Cenarion Emissary Jademoon) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100471 AND `DifficultyID`=0); -- 100471 (Gelbin Mekkatorque) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags5`=0x100, `VerifiedBuild`=70575 WHERE (`Entry`=100456 AND `DifficultyID`=0); -- 100456 (Falstad Wildhammer) - CanSwim - InteractWhileHostile
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100455 AND `DifficultyID`=0); -- 100455 (Moira Thaurissan) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100449 AND `DifficultyID`=0); -- 100449 (Stormwind Royal Guard) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=17103 AND `DifficultyID`=0); -- 17103 (Emissary Taluun) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100457 AND `DifficultyID`=0); -- 100457 (Malfurion Stormrage) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `StaticFlags5`=0x1, `VerifiedBuild`=70575 WHERE (`Entry`=93296 AND `DifficultyID`=0); -- 93296 (Coffee) - Sessile - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0xA0000000, `VerifiedBuild`=70575 WHERE (`Entry`=50088 AND `DifficultyID`=0); -- 50088 (Waters of Farseeing) - Floating, LargeAoi
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x90000000, `VerifiedBuild`=70575 WHERE (`Entry`=175138 AND `DifficultyID`=0); -- 175138 (Alleria Windrunner) - CanSwim, LargeAoi - CannotTurn
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `VerifiedBuild`=70575 WHERE (`Entry`=38821 AND `DifficultyID`=0); -- 38821 (Dave's Industrial Light and Magic Bunny (Medium)(Sessile)) - Sessile - GiganticAoi
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `StaticFlags5`=0x1, `VerifiedBuild`=70575 WHERE (`Entry`=111190 AND `DifficultyID`=0); -- 111190 ("The Imposter" Quest SI:7 Courtyard Reached Kill Credit & POI Marker) - Sessile - InfiniteAoi - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=47688 AND `DifficultyID`=0); -- 47688 (Officer Connelly) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=3086, `StaticFlags1`=0x10000000, `StaticFlags5`=0x101, `VerifiedBuild`=70575 WHERE (`Entry`=259749 AND `DifficultyID`=0); -- 259749 (Mech Suit) - CanSwim - UntargetableByClient, InteractWhileHostile
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `StaticFlags5`=0x1, `VerifiedBuild`=70575 WHERE (`Entry`=87503 AND `DifficultyID`=0); -- 87503 (Champion's Treadblade) - Sessile - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=176237 AND `DifficultyID`=0); -- 176237 (Sojurn Saka) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=18927 AND `DifficultyID`=0); -- 18927 (Human Commoner) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=176192 AND `DifficultyID`=0); -- 176192 (Xavier Carpenter) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=61840 AND `DifficultyID`=0); -- 61840 (Naanae) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=61839 AND `DifficultyID`=0); -- 61839 (Lucas Severing) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `VerifiedBuild`=70575 WHERE (`Entry`=155011 AND `DifficultyID`=0); -- 155011 (Daphne Funkybeats) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=97296 AND `DifficultyID`=0); -- 97296 (Archmage Khadgar) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=140146 AND `DifficultyID`=0); -- 140146 (Stormwind Infantry Captain) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=140022 AND `DifficultyID`=0); -- 140022 (Stormwind Infantry) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=58167 AND `DifficultyID`=0); -- 58167 (Colton Smith) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=176220 AND `DifficultyID`=0); -- 176220 (Antoinette Jardin) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=176203 AND `DifficultyID`=0); -- 176203 (Tanaya Perch) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30080000, `VerifiedBuild`=70575 WHERE (`Entry`=51348 AND `DifficultyID`=0); -- 51348 (Stormwind Gryphon Rider) - Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=42421 AND `DifficultyID`=0); -- 42421 (Stormwind Fisherman) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10080000, `VerifiedBuild`=70575 WHERE (`Entry`=61841 AND `DifficultyID`=0); -- 61841 (Brunn Goldenmug) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=97978 AND `DifficultyID`=0); -- 97978 (Archmage Khadgar) - CanSwim - InfiniteAoi
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=103581 AND `DifficultyID`=0); -- 103581 (Baric Stormrunner) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=103583 AND `DifficultyID`=0); -- 103583 (Cailyn Paledoom) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=103582 AND `DifficultyID`=0); -- 103582 (Belath Dawnblade) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=96666 AND `DifficultyID`=0); -- 96666 (Kayn Sunfury) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=103588 AND `DifficultyID`=0); -- 103588 (Zaria Shadowheart) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=103584 AND `DifficultyID`=0); -- 103584 (Izal Whitemoon) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=103587 AND `DifficultyID`=0); -- 103587 (Tirathon Saltheril) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=103586 AND `DifficultyID`=0); -- 103586 (Sirius Ebonwing) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100044 AND `DifficultyID`=0); -- 100044 (Sevis Brightflame) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100041 AND `DifficultyID`=0); -- 100041 (Lyana Darksorrow) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100040 AND `DifficultyID`=0); -- 100040 (Jace Darkweaver) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100043 AND `DifficultyID`=0); -- 100043 (Cassiel Nightthorn) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100042 AND `DifficultyID`=0); -- 100042 (Marius Felbane) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100037 AND `DifficultyID`=0); -- 100037 (Kor'vas Bloodthorn) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100036 AND `DifficultyID`=0); -- 100036 (Falara Nightsong) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100039 AND `DifficultyID`=0); -- 100039 (Cyana Nightglaive) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100038 AND `DifficultyID`=0); -- 100038 (Asha Ravensong) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100035 AND `DifficultyID`=0); -- 100035 (Allari the Souleater) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=699, `StaticFlags1`=0x10000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=101507 AND `DifficultyID`=0); -- 101507 (Wrathguard Legate) - CanSwim - HideInCombatLog
UPDATE `creature_template_difficulty` SET `ContentTuningID`=699, `StaticFlags1`=0x10000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=101506 AND `DifficultyID`=0); -- 101506 (Fel Shocktrooper) - CanSwim - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000000, `VerifiedBuild`=70575 WHERE (`Entry`=101585 AND `DifficultyID`=0); -- 101585 (Legion Gateway) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=99967 AND `DifficultyID`=0); -- 99967 (Falara Nightsong) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=99968 AND `DifficultyID`=0); -- 99968 (Kor'vas Bloodthorn) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=99966 AND `DifficultyID`=0); -- 99966 (Asha Ravensong) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100065 AND `DifficultyID`=0); -- 100065 (Cassiel Nightthorn) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100066 AND `DifficultyID`=0); -- 100066 (Sevis Brightflame) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=97273 AND `DifficultyID`=0); -- 97273 (Kayn Sunfury) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=99965 AND `DifficultyID`=0); -- 99965 (Marius Felbane) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=99969 AND `DifficultyID`=0); -- 99969 (Allari the Souleater) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000000, `VerifiedBuild`=70575 WHERE (`Entry`=101446 AND `DifficultyID`=0); -- 101446 (Fel Annihilation) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=99963 AND `DifficultyID`=0); -- 99963 (Cyana Nightglaive) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=99962 AND `DifficultyID`=0); -- 99962 (Lyana Darksorrow) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=99964 AND `DifficultyID`=0); -- 99964 (Jace Darkweaver) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x90000100, `VerifiedBuild`=70575 WHERE (`Entry`=96783 AND `DifficultyID`=0); -- 96783 (Bastillax) - Sessile, CanSwim, LargeAoi
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x80100, `VerifiedBuild`=70575 WHERE (`Entry`=72676 AND `DifficultyID`=0); -- 72676 (Spirit Healer) - Sessile, Amphibious
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=97265 AND `DifficultyID`=0); -- 97265 (Kayn Sunfury) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=97267 AND `DifficultyID`=0); -- 97267 (Altruis the Sufferer) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=97644 AND `DifficultyID`=0); -- 97644 (Kor'vas Bloodthorn) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `VerifiedBuild`=70575 WHERE (`Entry`=100469 AND `DifficultyID`=0); -- 100469 (Generic Bunny) - Sessile
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `VerifiedBuild`=70575 WHERE (`Entry`=96680 AND `DifficultyID`=0); -- 96680 (Glazer) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `VerifiedBuild`=70575 WHERE (`Entry`=102391 AND `DifficultyID`=0); -- 102391 (Altruis the Sufferer) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `StaticFlags5`=0x1, `VerifiedBuild`=70575 WHERE (`Entry`=99731 AND `DifficultyID`=0); -- 99731 (Countermeasures) - Sessile, Floating - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `VerifiedBuild`=70575 WHERE (`Entry`=102393 AND `DifficultyID`=0); -- 102393 (Kayn Sunfury) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `StaticFlags5`=0x1, `VerifiedBuild`=70575 WHERE (`Entry`=99732 AND `DifficultyID`=0); -- 99732 (Countermeasures) - Sessile, Floating - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `StaticFlags5`=0x1, `VerifiedBuild`=70575 WHERE (`Entry`=99709 AND `DifficultyID`=0); -- 99709 (Countermeasures) - Sessile, Floating - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000000, `StaticFlags5`=0x1, `VerifiedBuild`=70575 WHERE (`Entry`=99710 AND `DifficultyID`=0); -- 99710 (Freeze Statue) - Floating - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `VerifiedBuild`=70575 WHERE (`Entry`=96681 AND `DifficultyID`=0); -- 96681 (Ash'golm) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=699, `StaticFlags1`=0x100, `StaticFlags5`=0x2000, `VerifiedBuild`=70575 WHERE (`Entry`=101406 AND `DifficultyID`=0); -- 101406 (Fel Spirit) - Sessile - SuppressHighlightWhenTargetedOrMousedOver
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=96672 AND `DifficultyID`=0); -- 96672 (Cyana Nightglaive) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100724 AND `DifficultyID`=0); -- 100724 (Freed Illidari) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000000, `VerifiedBuild`=70575 WHERE (`Entry`=101332 AND `DifficultyID`=0); -- 101332 (Burning Fel) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x90000000, `VerifiedBuild`=70575 WHERE (`Entry`=96682 AND `DifficultyID`=0); -- 96682 (Immolanth) - CanSwim, LargeAoi
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000000, `VerifiedBuild`=70575 WHERE (`Entry`=97334 AND `DifficultyID`=0); -- 97334 (Kurloth) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `VerifiedBuild`=70575 WHERE (`Entry`=97333 AND `DifficultyID`=0); -- 97333 (Magolith) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100665 AND `DifficultyID`=0); -- 100665 (Vela Steelsong) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=100667 AND `DifficultyID`=0); -- 100667 (Jin Featherfall) - CanSwim - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=97090 AND `DifficultyID`=0); -- 97090 (Malace Shade) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=97330 AND `DifficultyID`=0); -- 97330 (Raxx) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `VerifiedBuild`=70575 WHERE (`Entry`=114358 AND `DifficultyID`=0); -- 114358 (Legion Portal) - Sessile
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=97225 AND `DifficultyID`=0); -- 97225 (Wrathguard Legate) - CanSwim - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=97228 AND `DifficultyID`=0); -- 97228 (Abyssal Shard) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=97806 AND `DifficultyID`=0); -- 97806 (Falara Nightsong) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=97643 AND `DifficultyID`=0); -- 97643 (Kor'vas Bloodthorn) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags3`=0x40000, `VerifiedBuild`=70575 WHERE (`Entry`=96675 AND `DifficultyID`=0); -- 96675 (Allari the Souleater) - CanSwim - AllowInteractionWhileInCombat
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=96645 AND `DifficultyID`=0); -- 96645 (Vault Warden) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=699, `StaticFlags1`=0x30000000, `StaticFlags5`=0x100, `VerifiedBuild`=70575 WHERE (`Entry`=99443 AND `DifficultyID`=0); -- 99443 (Vampiric Felbat) - CanSwim, Floating - InteractWhileHostile
UPDATE `creature_template_difficulty` SET `ContentTuningID`=699, `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=101901 AND `DifficultyID`=0); -- 101901 (Explosive Inquisitor Eye) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=699, `StaticFlags1`=0x30000000, `VerifiedBuild`=70575 WHERE (`Entry`=101895 AND `DifficultyID`=0); -- 101895 (Eye Dest) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=699, `StaticFlags1`=0x10000100, `VerifiedBuild`=70575 WHERE (`Entry`=101907 AND `DifficultyID`=0); -- 101907 (Seeping Inquisitor Eye) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=97070 AND `DifficultyID`=0); -- 97070 (Jace Darkweaver) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `StaticFlags5`=0x100, `VerifiedBuild`=70575 WHERE (`Entry`=96659 AND `DifficultyID`=0); -- 96659 (Vampiric Felbat) - Sessile, CanSwim - InteractWhileHostile
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100298 AND `DifficultyID`=0); -- 100298 (Marius Felbane) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x90000000, `VerifiedBuild`=70575 WHERE (`Entry`=97632 AND `DifficultyID`=0); -- 97632 (Crusher) - CanSwim, LargeAoi
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x90000000, `VerifiedBuild`=70575 WHERE (`Entry`=92990 AND `DifficultyID`=0); -- 92990 (Sledge) - CanSwim, LargeAoi
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=92985 AND `DifficultyID`=0); -- 92985 (Altruis the Sufferer) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=92984 AND `DifficultyID`=0); -- 92984 (Kayn Sunfury) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100296 AND `DifficultyID`=0); -- 100296 (Cassiel Nightthorn) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=96665 AND `DifficultyID`=0); -- 96665 (Kayn Sunfury) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=100299 AND `DifficultyID`=0); -- 100299 (Lyana Darksorrow) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x80080100, `VerifiedBuild`=70575 WHERE (`Entry`=59115 AND `DifficultyID`=0); -- 59115 (Generic Bunny) - Sessile, Amphibious, LargeAoi
UPDATE `creature_template_difficulty` SET `ContentTuningID`=699, `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=97614 AND `DifficultyID`=0); -- 97614 (Allari the Souleater) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=699, `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=97618 AND `DifficultyID`=0); -- 97618 (Falara Nightsong) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=699, `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=97616 AND `DifficultyID`=0); -- 97616 (Cyana Nightglaive) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=699, `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=97622 AND `DifficultyID`=0); -- 97622 (Kor'vas Bloodthorn) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=699, `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=97619 AND `DifficultyID`=0); -- 97619 (Marius Felbane) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=699, `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=97615 AND `DifficultyID`=0); -- 97615 (Jace Darkweaver) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=699, `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=97617 AND `DifficultyID`=0); -- 97617 (Asha Ravensong) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=699, `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=97620 AND `DifficultyID`=0); -- 97620 (Lyana Darksorrow) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0xB0000000, `VerifiedBuild`=70575 WHERE (`Entry`=96997 AND `DifficultyID`=0); -- 96997 (Kethrazor) - CanSwim, Floating, LargeAoi
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x80080100, `VerifiedBuild`=70575 WHERE (`Entry`=59113 AND `DifficultyID`=0); -- 59113 (Generic Bunny) - Sessile, Amphibious, LargeAoi
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `VerifiedBuild`=70575 WHERE (`Entry`=99451 AND `DifficultyID`=0); -- 99451 (Mirana Starlight) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=92986 AND `DifficultyID`=0); -- 92986 (Altruis the Sufferer) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=92980 AND `DifficultyID`=0); -- 92980 (Kayn Sunfury) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=99631 AND `DifficultyID`=0); -- 99631 (Kayn Sunfury) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=70575 WHERE (`Entry`=99632 AND `DifficultyID`=0); -- 99632 (Altruis the Sufferer) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `VerifiedBuild`=70575 WHERE (`Entry`=99501 AND `DifficultyID`=0); -- 99501 (Legion Portal) - Sessile
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags4`=0x2000000, `VerifiedBuild`=70575 WHERE (`Entry`=92776 AND `DifficultyID`=0); -- 92776 (Fel Shocktrooper) - CanSwim - HideInCombatLog

DELETE FROM `creature_model_info` WHERE `DisplayID` IN (145535, 76644);
INSERT INTO `creature_model_info` (`DisplayID`, `BoundingRadius`, `CombatReach`, `DisplayID_Other_Gender`, `VerifiedBuild`) VALUES
(145535, 0.166422381997108459, 0.60000002384185791, 0, 70575),
(76644, 1.299999952316284179, 1.949999928474426269, 0, 70575);

UPDATE `creature_model_info` SET `VerifiedBuild`=70575 WHERE `DisplayID` IN (3286, 3291, 1765, 1469, 3316, 113856, 16910, 72819, 89772, 92464, 8809, 92463, 20985, 1480, 141439, 20988, 89777, 18930, 93505, 112136, 62185, 14554, 4449, 57081, 90104, 99416, 19619, 59145, 67053, 68845, 110154, 110153, 58478, 99415, 99198, 46209, 62530, 62529, 72815, 72820, 104841, 1494, 109621, 109614, 99194, 81436, 72724, 73152, 11747, 11746, 1756, 99506, 104835, 139467, 3302, 113617, 36656, 110111, 142813, 109492, 110173, 110231, 142123, 99381, 99423, 27261, 4626, 99383, 38804, 106965, 106966, 105429, 131409, 99422, 1502, 110234, 32928, 99379, 64062, 105546, 1432, 110242, 5081, 99193, 107560, 5080, 27823, 99186, 99185, 1486, 1477, 3285, 10591, 99187, 99496, 137392, 33468, 5545, 5552, 5444, 1437, 1433, 1436, 1450, 33520, 1443, 38576, 2974, 3133, 33507, 33519, 33518, 45304, 110175, 137099, 37372, 1444, 1431, 1447, 1427, 1446, 72447, 913, 99615, 3246, 5082, 36758, 140351, 5547, 1438, 5074, 7991, 5546, 31851, 89217, 36213, 7992, 1439, 1440, 1434, 4888, 36839, 5551, 99391, 25901, 5567, 33948, 36597, 110189, 34070, 110178, 110176, 1445, 1423, 1541, 87182, 80, 33303, 37850, 1429, 1448, 17703, 47158, 47160, 100170, 100168, 100167, 14493, 42274, 42273, 17717, 17697, 17696, 17694, 1441, 99499, 17718, 17759, 3238, 14492, 99497, 99498, 36815, 10625, 100184, 100165, 42276, 221, 39532, 37087, 15921, 14472, 3448, 3449, 1449, 1520, 1517, 1508, 110172, 42275, 5446, 38422, 39184, 60316, 25319, 7935, 25336, 1510, 9990, 1505, 36816, 36814, 2176, 1522, 1511, 26375, 45881, 100172, 26070, 9590, 338, 87011, 91081, 91080, 262, 257, 18931, 1573, 15594, 99947, 99949, 33748, 33747, 33746, 33745, 5553, 17188, 91079, 99950, 99951, 5077, 5565, 63968, 29514, 65199, 63521, 344, 31174, 29796, 29515, 17822, 62092, 62751, 15321, 41667, 7006, 32681, 32680, 34005, 58246, 102032, 57432, 34004, 57418, 1758, 58338, 16854, 60494, 35095, 37310, 5075, 2961, 328, 63522, 3288, 60530, 57175, 95808, 60045, 58694, 58690, 58502, 71286, 60551, 59973, 60041, 60940, 59930, 60525, 60468, 57260, 57439, 3281, 21342, 105170, 78345, 21072, 99500, 116916, 60207, 60005, 58366, 58325, 58363, 22370, 100174, 22371, 58682, 100159, 42278, 42277, 58467, 60402, 60528, 61057, 36767, 60601, 92198, 59189, 1141, 59935, 33743, 1519, 1696, 59266, 60032, 1509, 87371, 87372, 57329, 4996, 5728, 1516, 18055, 40027, 100003, 100004, 5000, 1521, 98571, 37311, 3167, 99401, 18054, 19670, 28507, 1525, 42279, 134, 99389, 68476, 65834, 66806, 5233, 64174, 67229, 46710, 51861, 38852, 67177, 63247, 64560, 67052, 62572, 63165, 66774, 66773, 64442, 63527, 73233, 20041, 62374, 64233, 63990, 65822, 68247, 64823, 65304, 15880, 1418, 64443, 63985, 68154);
UPDATE `creature_model_info` SET `BoundingRadius`=0.347222000360488891, `CombatReach`=1.5, `VerifiedBuild`=70575 WHERE `DisplayID`=139494;
UPDATE `creature_model_info` SET `BoundingRadius`=0.305999994277954101, `CombatReach`=1.5, `VerifiedBuild`=70575 WHERE `DisplayID` IN (60017, 60557);
UPDATE `creature_model_info` SET `BoundingRadius`=1.026380538940429687, `VerifiedBuild`=70575 WHERE `DisplayID`=32546;

DELETE FROM `creature_template_gossip` WHERE (`CreatureID`=256112 AND `MenuID`=41536) OR (`CreatureID`=113392 AND `MenuID`=19810) OR (`CreatureID`=97296 AND `MenuID`=35200) OR (`CreatureID`=97070 AND `MenuID`=18829);
INSERT INTO `creature_template_gossip` (`CreatureID`, `MenuID`, `VerifiedBuild`) VALUES
(256112, 41536, 70575), -- Sothon Doomrender
(113392, 19810, 70575), -- Illidari Enforcer
(97296, 35200, 70575), -- Archmage Khadgar
(97070, 18829, 70575); -- Jace Darkweaver

UPDATE `creature_template_gossip` SET `VerifiedBuild`=70575 WHERE (`MenuID`=20009 AND `CreatureID`=109912) OR (`MenuID`=19075 AND `CreatureID`=100429) OR (`MenuID`=18424 AND `CreatureID` IN (96666,96665,92980)) OR (`MenuID`=19012 AND `CreatureID`=97644) OR (`MenuID`=18831 AND `CreatureID`=96675) OR (`MenuID`=18830 AND `CreatureID`=97806) OR (`MenuID`=18832 AND `CreatureID`=97643) OR (`MenuID`=18426 AND `CreatureID`=92986);


DELETE FROM `spell_target_position` WHERE (`ID`=192757 AND `EffectIndex`=0 AND `OrderIndex`=0) OR (`ID`=197364 AND `EffectIndex`=0 AND `OrderIndex`=0);
INSERT INTO `spell_target_position` (`ID`, `EffectIndex`, `OrderIndex`, `MapID`, `PositionX`, `PositionY`, `PositionZ`, `VerifiedBuild`) VALUES
(192757, 0, 0, 0, -8544.259765625, 462.8900146484375, 104.470001220703125, 70575), -- Spell: 192757 (Teleport to Stormwind) Effect 0: 252 (SPELL_EFFECT_TELEPORT_UNITS)
(197364, 0, 0, 0, -9066.330078125, 433.170013427734375, 93.05999755859375, 70575); -- Spell: 197364 (Bind to Stormwind) Effect 0: 11 (SPELL_EFFECT_BIND)

DELETE FROM `spell_target_position` WHERE `ID` IN (199476,199481,199760);
INSERT INTO `spell_target_position` (`ID`, `EffectIndex`, `OrderIndex`, `MapID`, `PositionX`, `PositionY`, `PositionZ`, `VerifiedBuild`) VALUES
(199476, 0, 0, 1468, 4028.096435546875, -320.458343505859375, -281.00628662109375, 70575), -- Spell: 199476 (Shoulder Charge) Effect 0: 149 (SPELL_EFFECT_CHARGE_DEST)
(199476, 0, 1, 1468, 4072.4306640625, -276.611114501953125, -281.00628662109375, 70575), -- Spell: 199476 (Shoulder Charge) Effect 0: 149 (SPELL_EFFECT_CHARGE_DEST)
(199481, 0, 0, 1468, 4073.02001953125, -320.91839599609375, -281.00628662109375, 70575), -- Spell: 199481 (Shoulder Charge) Effect 0: 149 (SPELL_EFFECT_CHARGE_DEST)
(199481, 0, 1, 1468, 4028.2509765625, -274.97222900390625, -281.00628662109375, 70575), -- Spell: 199481 (Shoulder Charge) Effect 0: 149 (SPELL_EFFECT_CHARGE_DEST)
(199760, 0, 0, 1468, 4703.3662109375, -435.484375, 122.305572509765625, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 1, 1468, 4638.7109375, -414.9600830078125, 122.0277786254882812, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 2, 1468, 4634.87744140625, -436.057281494140625, 122.0277786254882812, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 3, 1468, 4683.38037109375, -414.118072509765625, 122.0277862548828125, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 4, 1468, 4630.501953125, -469.96527099609375, 122.0277633666992187, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 5, 1468, 4637.330078125, -417.8507080078125, 122.0277633666992187, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 6, 1468, 4687.77783203125, -414.46527099609375, 122.0277862548828125, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 7, 1468, 4698.54541015625, -434.192718505859375, 122.0277786254882812, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 8, 1468, 4649.66650390625, -485.45660400390625, 122.0277633666992187, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 9, 1468, 4679.80224609375, -489.98089599609375, 122.3055267333984375, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 10, 1468, 4685.91845703125, -417.46527099609375, 122.0278091430664062, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 11, 1468, 4685.91943359375, -485.208343505859375, 122.0277633666992187, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 12, 1468, 4695.59814453125, -431.92535400390625, 122.0277786254882812, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 13, 1468, 4646.150390625, -417.28125, 122.0277862548828125, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 14, 1468, 4702.56982421875, -425.691009521484375, 122.305999755859375, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 15, 1468, 4630.3193359375, -429.779510498046875, 122.0277786254882812, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 16, 1468, 4640.3505859375, -482.99652099609375, 122.0277633666992187, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 17, 1468, 4677.08056640625, -485.380218505859375, 122.0277633666992187, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 18, 1468, 4698.6728515625, -438.045135498046875, 122.0277786254882812, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 19, 1468, 4698.9453125, -424.560760498046875, 122.0277633666992187, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 20, 1468, 4677.09814453125, -416.8507080078125, 122.0278091430664062, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 21, 1468, 4641.5068359375, -488.130218505859375, 122.0277633666992187, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 22, 1468, 4647.587890625, -414.048614501953125, 122.0277786254882812, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 23, 1468, 4634.26416015625, -466.44964599609375, 122.0277786254882812, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 24, 1468, 4650.79345703125, -417.045135498046875, 122.0277862548828125, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 25, 1468, 4627.5927734375, -477.196197509765625, 122.0277481079101562, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 26, 1468, 4630.18408203125, -438.618072509765625, 122.0277786254882812, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 27, 1468, 4686.75537109375, -488.770843505859375, 122.0277633666992187, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 28, 1468, 4641.16064453125, -417.416656494140625, 122.0277786254882812, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 29, 1468, 4702.3974609375, -428.494781494140625, 122.0277786254882812, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 30, 1468, 4695.8994140625, -436.4375, 122.0277786254882812, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 31, 1468, 4678.861328125, -414.15972900390625, 122.0278091430664062, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 32, 1468, 4643.18310546875, -414.295135498046875, 122.0277786254882812, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 33, 1468, 4647.7978515625, -488.751739501953125, 122.0277481079101562, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 34, 1468, 4683.0869140625, -482.095489501953125, 122.0277633666992187, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 35, 1468, 4630.056640625, -434.765625, 122.0277786254882812, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 36, 1468, 4630.23876953125, -474.951385498046875, 122.0277633666992187, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 37, 1468, 4690.546875, -417.861114501953125, 122.0277786254882812, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 38, 1468, 4693.86474609375, -476.642364501953125, 122.0277786254882812, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 39, 1468, 4701.33349609375, -475.689239501953125, 122.0277786254882812, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 40, 1468, 4636.5556640625, -482.291656494140625, 122.0277633666992187, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 41, 1468, 4678.59033203125, -482.5538330078125, 122.0277633666992187, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 42, 1468, 4695.7041015625, -427.51910400390625, 122.0277633666992187, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 43, 1468, 4701.27587890625, -472.171875, 122.0277786254882812, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 44, 1468, 4680.923828125, -485.119781494140625, 122.0277633666992187, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 45, 1468, 4633.90869140625, -429.067718505859375, 122.0277786254882812, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 46, 1468, 4689.21875, -421.423614501953125, 122.0277481079101562, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 47, 1468, 4630.3662109375, -478.8038330078125, 122.0277633666992187, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 48, 1468, 4683.31787109375, -489.80035400390625, 122.3055419921875, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 49, 1468, 4627.1103515625, -432.498260498046875, 122.0277633666992187, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 50, 1468, 4680.95751953125, -416.930572509765625, 122.0278091430664062, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 51, 1468, 4627.21533203125, -428.092010498046875, 122.0277786254882812, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 52, 1468, 4696.7734375, -469.411468505859375, 122.0277862548828125, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 53, 1468, 4687.49658203125, -482.045135498046875, 122.0277786254882812, 70575), -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)
(199760, 0, 54, 1468, 4698.80810546875, -429.20660400390625, 122.0277786254882812, 70575); -- Spell: 199760 (Burning Fel) Effect 0: 32 (SPELL_EFFECT_TRIGGER_MISSILE)


DELETE FROM `scene_template` WHERE `SceneId` IN (1456, 1016);
INSERT INTO `scene_template` (`SceneId`, `Flags`, `ScriptPackageID`, `Encrypted`) VALUES
(1456, 16, 1735, 0),
(1016, 26, 1423, 0);


DELETE FROM `quest_offer_reward` WHERE `ID` IN (44473 /*A Weapon of the Alliance*/, 44463 /*Demons Among Them*/, 44471 /*Second Sight*/, 39691 /*The Call of War*/, 39689 /*Illidari, We Are Leaving*/, 40255 /*Between Us and Freedom*/, 40373 /*A New Direction*/, 39686 /*All The Way Up*/, 39684 /*Beam Me Up*/, 40254 /*Forged in Fire*/, 39685 /*Frozen in Time*/, 39682 /*Grand Theft Felbat*/, 40253 /*Stop Gul'dan!*/, 38689 /*Fel Infusion*/, 38690 /*Rise of the Illidari*/, 38672 /*Breaking Out*/);
INSERT INTO `quest_offer_reward` (`ID`, `Emote1`, `Emote2`, `Emote3`, `Emote4`, `EmoteDelay1`, `EmoteDelay2`, `EmoteDelay3`, `EmoteDelay4`, `RewardText`, `VerifiedBuild`) VALUES
(44473, 1, 1, 0, 0, 0, 0, 0, 0, 'I trust the young king is safe.$b$bWe have more work ahead of us, $c. Much more.', 70575), -- A Weapon of the Alliance
(44463, 0, 0, 0, 0, 0, 0, 0, 0, 'I must admit, you saved us from what might have been a terrible tragedy. You have my gratitude, $c.', 70575), -- Demons Among Them
(44471, 0, 0, 0, 0, 0, 0, 0, 0, 'Of course the Legion would try to infiltrate this funeral. Having all the leaders of the Alliance gathered in one place at one time... it is too tasty a target for the demons to resist!', 70575), -- Second Sight
(39691, 274, 0, 0, 0, 0, 0, 0, 0, 'Dark times have fallen upon Stormwind, $n.$b$bWith King Varian Wrynn slain on the Broken Shore, uncertainty and tension grip the Alliance leaders.$b$bThe timing of this tragedy is no accident. The Burning Legion seeks to sow disorder amongst its enemies, weakening its prey from within.$b$bWe must act.', 70575), -- The Call of War
(39689, 274, 0, 0, 0, 0, 0, 0, 0, 'If you are ready, I will take you to our destination.$B$BAre you prepared to leave?', 70575), -- Illidari, We Are Leaving
(40255, 274, 0, 0, 0, 0, 0, 0, 0, 'We escaped the Vault, but our work is far from over.$B$BThe Legion\'s assault on this world is just beginning.$B$BIf Azeroth is to survive, we may have to work with this... Archmage Khadgar.', 70575), -- Between Us and Freedom
(40373, 274, 0, 0, 0, 0, 0, 0, 0, 'I see. I support you no matter your choice. With this new beginning, we shall rebuild the Illidari.', 70575), -- A New Direction
(39686, 274, 0, 0, 0, 0, 0, 0, 0, 'This is serious. Their core ideals are in direct conflict.$B$BI fear they will take this duel to the death.', 70575), -- All The Way Up
(39684, 274, 0, 0, 0, 0, 0, 0, 0, 'I had no doubt you would know what to do.', 70575), -- Beam Me Up
(40254, 25, 0, 0, 0, 0, 0, 0, 0, 'Yes... just look at you... surging with new power and knowledge!', 70575), -- Forged in Fire
(39685, 274, 0, 0, 0, 0, 0, 0, 0, 'I knew you could do it, $n. We could not let that creature escape.', 70575), -- Frozen in Time
(39682, 274, 0, 0, 0, 0, 0, 0, 0, 'Kayn and Altruis said you would be coming. Just in time, $n.', 70575), -- Grand Theft Felbat
(40253, 274, 0, 0, 0, 0, 0, 0, 0, 'Excellent. You have absorbed the powers of these brutes. Look how strong you have grown!', 70575), -- Stop Gul'dan!
(38689, 273, 0, 0, 0, 0, 0, 0, 0, 'The color has returned to your face, $n.', 70575), -- Fel Infusion
(38690, 396, 0, 0, 0, 0, 0, 0, 0, 'We always knew the Illidari would be called upon to defend this world. It is why we sacrificed everything to become who... WHAT... we are.', 70575), -- Rise of the Illidari
(38672, 0, 0, 0, 0, 0, 0, 0, 0, '<Pull the lever.>', 70575); -- Breaking Out



UPDATE `quest_poi` SET `VerifiedBuild`=70575 WHERE (`QuestID`=44663 AND `BlobIndex`=1 AND `Idx1`=5) OR (`QuestID`=44663 AND `BlobIndex`=0 AND `Idx1`=4) OR (`QuestID`=44663 AND `BlobIndex`=0 AND `Idx1`=3) OR (`QuestID`=44663 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=44473 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=44473 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=44463 AND `BlobIndex`=0 AND `Idx1`=3) OR (`QuestID`=44463 AND `BlobIndex`=0 AND `Idx1`=2) OR (`QuestID`=44463 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=44463 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=44471 AND `BlobIndex`=0 AND `Idx1`=2) OR (`QuestID`=44471 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=44471 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=39691 AND `BlobIndex`=0 AND `Idx1`=2) OR (`QuestID`=39691 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=39691 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=39689 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=39689 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=40255 AND `BlobIndex`=0 AND `Idx1`=3) OR (`QuestID`=40255 AND `BlobIndex`=0 AND `Idx1`=2) OR (`QuestID`=40255 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=40255 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=40373 AND `BlobIndex`=0 AND `Idx1`=2) OR (`QuestID`=40373 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=40373 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=39686 AND `BlobIndex`=0 AND `Idx1`=3) OR (`QuestID`=39686 AND `BlobIndex`=1 AND `Idx1`=2) OR (`QuestID`=39686 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=39686 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=40254 AND `BlobIndex`=0 AND `Idx1`=2) OR (`QuestID`=40254 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=40254 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=39684 AND `BlobIndex`=0 AND `Idx1`=2) OR (`QuestID`=39684 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=39684 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=39685 AND `BlobIndex`=0 AND `Idx1`=4) OR (`QuestID`=39685 AND `BlobIndex`=0 AND `Idx1`=3) OR (`QuestID`=39685 AND `BlobIndex`=0 AND `Idx1`=2) OR (`QuestID`=39685 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=39685 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=39682 AND `BlobIndex`=0 AND `Idx1`=2) OR (`QuestID`=39682 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=39682 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=40253 AND `BlobIndex`=0 AND `Idx1`=3) OR (`QuestID`=40253 AND `BlobIndex`=1 AND `Idx1`=2) OR (`QuestID`=40253 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=40253 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=38689 AND `BlobIndex`=0 AND `Idx1`=2) OR (`QuestID`=38689 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=38689 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=38690 AND `BlobIndex`=0 AND `Idx1`=2) OR (`QuestID`=38690 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=38690 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=38672 AND `BlobIndex`=0 AND `Idx1`=4) OR (`QuestID`=38672 AND `BlobIndex`=0 AND `Idx1`=3) OR (`QuestID`=38672 AND `BlobIndex`=0 AND `Idx1`=2) OR (`QuestID`=38672 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=38672 AND `BlobIndex`=0 AND `Idx1`=0);
UPDATE `quest_poi` SET `PlayerConditionID`=23504, `VerifiedBuild`=70575 WHERE (`QuestID`=44663 AND `BlobIndex`=1 AND `Idx1`=2); -- In the Blink of an Eye
UPDATE `quest_poi` SET `PlayerConditionID`=23503, `VerifiedBuild`=70575 WHERE (`QuestID`=44663 AND `BlobIndex`=0 AND `Idx1`=1); -- In the Blink of an Eye

UPDATE `quest_poi_points` SET `VerifiedBuild`=70575 WHERE (`QuestID`=44663 AND `Idx1`=5 AND `Idx2`=0) OR (`QuestID`=44663 AND `Idx1`=4 AND `Idx2`=0) OR (`QuestID`=44663 AND `Idx1`=3 AND `Idx2`=0) OR (`QuestID`=44663 AND `Idx1`=2 AND `Idx2`=0) OR (`QuestID`=44663 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=44663 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=44473 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=44473 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=44463 AND `Idx1`=3 AND `Idx2`=0) OR (`QuestID`=44463 AND `Idx1`=2 AND `Idx2`=11) OR (`QuestID`=44463 AND `Idx1`=2 AND `Idx2`=10) OR (`QuestID`=44463 AND `Idx1`=2 AND `Idx2`=9) OR (`QuestID`=44463 AND `Idx1`=2 AND `Idx2`=8) OR (`QuestID`=44463 AND `Idx1`=2 AND `Idx2`=7) OR (`QuestID`=44463 AND `Idx1`=2 AND `Idx2`=6) OR (`QuestID`=44463 AND `Idx1`=2 AND `Idx2`=5) OR (`QuestID`=44463 AND `Idx1`=2 AND `Idx2`=4) OR (`QuestID`=44463 AND `Idx1`=2 AND `Idx2`=3) OR (`QuestID`=44463 AND `Idx1`=2 AND `Idx2`=2) OR (`QuestID`=44463 AND `Idx1`=2 AND `Idx2`=1) OR (`QuestID`=44463 AND `Idx1`=2 AND `Idx2`=0) OR (`QuestID`=44463 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=44463 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=44471 AND `Idx1`=2 AND `Idx2`=0) OR (`QuestID`=44471 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=44471 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=39691 AND `Idx1`=2 AND `Idx2`=0) OR (`QuestID`=39691 AND `Idx1`=1 AND `Idx2`=11) OR (`QuestID`=39691 AND `Idx1`=1 AND `Idx2`=10) OR (`QuestID`=39691 AND `Idx1`=1 AND `Idx2`=9) OR (`QuestID`=39691 AND `Idx1`=1 AND `Idx2`=8) OR (`QuestID`=39691 AND `Idx1`=1 AND `Idx2`=7) OR (`QuestID`=39691 AND `Idx1`=1 AND `Idx2`=6) OR (`QuestID`=39691 AND `Idx1`=1 AND `Idx2`=5) OR (`QuestID`=39691 AND `Idx1`=1 AND `Idx2`=4) OR (`QuestID`=39691 AND `Idx1`=1 AND `Idx2`=3) OR (`QuestID`=39691 AND `Idx1`=1 AND `Idx2`=2) OR (`QuestID`=39691 AND `Idx1`=1 AND `Idx2`=1) OR (`QuestID`=39691 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=39691 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=39689 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=39689 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=40255 AND `Idx1`=3 AND `Idx2`=0) OR (`QuestID`=40255 AND `Idx1`=2 AND `Idx2`=0) OR (`QuestID`=40255 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=40255 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=40373 AND `Idx1`=2 AND `Idx2`=0) OR (`QuestID`=40373 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=40373 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=39686 AND `Idx1`=3 AND `Idx2`=0) OR (`QuestID`=39686 AND `Idx1`=2 AND `Idx2`=0) OR (`QuestID`=39686 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=39686 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=40254 AND `Idx1`=2 AND `Idx2`=0) OR (`QuestID`=40254 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=40254 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=39684 AND `Idx1`=2 AND `Idx2`=0) OR (`QuestID`=39684 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=39684 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=39685 AND `Idx1`=4 AND `Idx2`=0) OR (`QuestID`=39685 AND `Idx1`=3 AND `Idx2`=0) OR (`QuestID`=39685 AND `Idx1`=2 AND `Idx2`=0) OR (`QuestID`=39685 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=39685 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=39682 AND `Idx1`=2 AND `Idx2`=0) OR (`QuestID`=39682 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=39682 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=40253 AND `Idx1`=3 AND `Idx2`=0) OR (`QuestID`=40253 AND `Idx1`=2 AND `Idx2`=0) OR (`QuestID`=40253 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=40253 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=38689 AND `Idx1`=2 AND `Idx2`=0) OR (`QuestID`=38689 AND `Idx1`=1 AND `Idx2`=11) OR (`QuestID`=38689 AND `Idx1`=1 AND `Idx2`=10) OR (`QuestID`=38689 AND `Idx1`=1 AND `Idx2`=9) OR (`QuestID`=38689 AND `Idx1`=1 AND `Idx2`=8) OR (`QuestID`=38689 AND `Idx1`=1 AND `Idx2`=7) OR (`QuestID`=38689 AND `Idx1`=1 AND `Idx2`=6) OR (`QuestID`=38689 AND `Idx1`=1 AND `Idx2`=5) OR (`QuestID`=38689 AND `Idx1`=1 AND `Idx2`=4) OR (`QuestID`=38689 AND `Idx1`=1 AND `Idx2`=3) OR (`QuestID`=38689 AND `Idx1`=1 AND `Idx2`=2) OR (`QuestID`=38689 AND `Idx1`=1 AND `Idx2`=1) OR (`QuestID`=38689 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=38689 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=38690 AND `Idx1`=2 AND `Idx2`=0) OR (`QuestID`=38690 AND `Idx1`=1 AND `Idx2`=11) OR (`QuestID`=38690 AND `Idx1`=1 AND `Idx2`=10) OR (`QuestID`=38690 AND `Idx1`=1 AND `Idx2`=9) OR (`QuestID`=38690 AND `Idx1`=1 AND `Idx2`=8) OR (`QuestID`=38690 AND `Idx1`=1 AND `Idx2`=7) OR (`QuestID`=38690 AND `Idx1`=1 AND `Idx2`=6) OR (`QuestID`=38690 AND `Idx1`=1 AND `Idx2`=5) OR (`QuestID`=38690 AND `Idx1`=1 AND `Idx2`=4) OR (`QuestID`=38690 AND `Idx1`=1 AND `Idx2`=3) OR (`QuestID`=38690 AND `Idx1`=1 AND `Idx2`=2) OR (`QuestID`=38690 AND `Idx1`=1 AND `Idx2`=1) OR (`QuestID`=38690 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=38690 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=38672 AND `Idx1`=4 AND `Idx2`=0) OR (`QuestID`=38672 AND `Idx1`=3 AND `Idx2`=0) OR (`QuestID`=38672 AND `Idx1`=2 AND `Idx2`=0) OR (`QuestID`=38672 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=38672 AND `Idx1`=0 AND `Idx2`=0);

DELETE FROM `quest_details` WHERE `ID` IN (44473 /*A Weapon of the Alliance*/, 44463 /*Demons Among Them*/, 44471 /*Second Sight*/, 39691 /*The Call of War*/, 39689 /*Illidari, We Are Leaving*/, 40255 /*Between Us and Freedom*/, 40373 /*A New Direction*/, 39686 /*All The Way Up*/, 40254 /*Forged in Fire*/, 39684 /*Beam Me Up*/, 39685 /*Frozen in Time*/, 39682 /*Grand Theft Felbat*/, 40253 /*Stop Gul'dan!*/, 38689 /*Fel Infusion*/, 38690 /*Rise of the Illidari*/, 38672 /*Breaking Out*/);
INSERT INTO `quest_details` (`ID`, `Emote1`, `Emote2`, `Emote3`, `Emote4`, `EmoteDelay1`, `EmoteDelay2`, `EmoteDelay3`, `EmoteDelay4`, `VerifiedBuild`) VALUES
(44473, 0, 0, 0, 0, 0, 0, 0, 0, 70575), -- A Weapon of the Alliance
(44463, 603, 0, 0, 0, 0, 0, 0, 0, 70575), -- Demons Among Them
(44471, 1, 0, 0, 0, 0, 0, 0, 0, 70575), -- Second Sight
(39691, 1, 1, 0, 0, 0, 1000, 0, 0, 70575), -- The Call of War
(39689, 6, 0, 0, 0, 0, 0, 0, 0, 70575), -- Illidari, We Are Leaving
(40255, 6, 0, 0, 0, 0, 0, 0, 0, 70575), -- Between Us and Freedom
(40373, 6, 0, 0, 0, 0, 0, 0, 0, 70575), -- A New Direction
(39686, 6, 0, 0, 0, 0, 0, 0, 0, 70575), -- All The Way Up
(40254, 6, 0, 0, 0, 0, 0, 0, 0, 70575), -- Forged in Fire
(39684, 6, 0, 0, 0, 0, 0, 0, 0, 70575), -- Beam Me Up
(39685, 6, 0, 0, 0, 0, 0, 0, 0, 70575), -- Frozen in Time
(39682, 6, 0, 0, 0, 0, 0, 0, 0, 70575), -- Grand Theft Felbat
(40253, 6, 0, 0, 0, 0, 0, 0, 0, 70575), -- Stop Gul'dan!
(38689, 274, 0, 0, 0, 0, 0, 0, 0, 70575), -- Fel Infusion
(38690, 1, 0, 0, 0, 0, 0, 0, 0, 70575), -- Rise of the Illidari
(38672, 0, 0, 0, 0, 0, 0, 0, 0, 70575); -- Breaking Out

UPDATE `quest_details` SET `VerifiedBuild`=70575 WHERE `ID`=44663;

DELETE FROM `ui_map_quest_line` WHERE (`UIMapId`=84 AND `QuestLineId` IN (6043,6023,6016,5930,5811,5770,150)) OR (`UIMapId`=679 AND `QuestLineId`=150) OR (`UIMapId`=678 AND `QuestLineId`=150) OR (`UIMapId`=677 AND `QuestLineId`=150);
INSERT INTO `ui_map_quest_line` (`UIMapId`, `QuestLineId`, `VerifiedBuild`) VALUES
(84, 6043, 70575),
(84, 6023, 70575),
(84, 6016, 70575),
(84, 5930, 70575),
(84, 5811, 70575),
(84, 5770, 70575),
(84, 150, 70575),
(679, 150, 70575),
(678, 150, 70575),
(677, 150, 70575);

UPDATE `ui_map_quest_line` SET `VerifiedBuild`=70575 WHERE (`UIMapId`=84 AND `QuestLineId` IN (6058,6024,6021,6009,6008,6007,6006,5740,5697,5691,5673,5638,5593,5519,5513,5511,5497,5482,5458,5399,5382,5377,5372,1289,1276,1267,1128,1108,1107,1030,1000,972,957,929,926,916,858,797,781,737,637,635,589,576,575,547,546,539,533,532,526,347,346,300,289,262,259,242,240,198,131,421));

DELETE FROM `creature_queststarter` WHERE (`id`=101004 AND `quest`=44663) OR (`id`=100973 AND `quest`=44473) OR (`id`=102585 AND `quest` IN (44463,44471)) OR (`id`=97296 AND `quest`=39691) OR (`id`=97978 AND `quest`=39689) OR (`id`=97644 AND `quest` IN (40255,40373)) OR (`id`=96675 AND `quest` IN (39686,40254)) OR (`id`=97643 AND `quest` IN (39684,39685)) OR (`id`=96665 AND `quest`=39682) OR (`id`=92718 AND `quest` IN (40253,38672)) OR (`id`=92986 AND `quest`=38689) OR (`id`=92980 AND `quest`=38690);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES
(101004, 44663, 70575), -- In the Blink of an Eye offered by Elerion Bladedancer
(100973, 44473, 70575), -- A Weapon of the Alliance offered by Anduin Wrynn
(102585, 44463, 70575), -- Demons Among Them offered by Jace Darkweaver
(102585, 44471, 70575), -- Second Sight offered by Jace Darkweaver
(97296, 39691, 70575), -- The Call of War offered by Archmage Khadgar
(97978, 39689, 70575), -- Illidari, We Are Leaving offered by Archmage Khadgar
(97644, 40255, 70575), -- Between Us and Freedom offered by Kor'vas Bloodthorn
(97644, 40373, 70575), -- A New Direction offered by Kor'vas Bloodthorn
(96675, 39686, 70575), -- All The Way Up offered by Allari the Souleater
(96675, 40254, 70575), -- Forged in Fire offered by Allari the Souleater
(97643, 39684, 70575), -- Beam Me Up offered by Kor'vas Bloodthorn
(97643, 39685, 70575), -- Frozen in Time offered by Kor'vas Bloodthorn
(96665, 39682, 70575), -- Grand Theft Felbat offered by Kayn Sunfury
(92718, 40253, 70575), -- Stop Gul'dan! offered by Maiev Shadowsong
(92986, 38689, 70575), -- Fel Infusion offered by Altruis the Sufferer
(92980, 38690, 70575), -- Rise of the Illidari offered by Kayn Sunfury
(92718, 38672, 70575); -- Breaking Out offered by Maiev Shadowsong

UPDATE `creature_questender` SET `VerifiedBuild`=70575 WHERE (`id`=101004 AND `quest`=44473) OR (`id`=100973 AND `quest`=44463) OR (`id`=102585 AND `quest` IN (44471,39691)) OR (`id`=97978 AND `quest`=39689) OR (`id`=96666 AND `quest`=40255) OR (`id`=97644 AND `quest` IN (40373,39686)) OR (`id`=96675 AND `quest` IN (39684,40254,39685)) OR (`id`=97643 AND `quest`=39682) OR (`id`=96665 AND `quest` IN (40253,38690)) OR (`id`=92986 AND `quest`=38689);
UPDATE `gameobject_questender` SET `VerifiedBuild`=70575 WHERE (`id`=244923 AND `quest`=38672);

DELETE FROM `gameobject_template_addon` WHERE `entry` IN (529989 /*Barbershop Pole [DNT]*/, 645097 /*Portal Mirror [DNT]*/, 259879 /*Collision Wall*/, 617539 /*Stormwind Portal*/, 246562 /*Small Treasure Chest*/, 246561 /*Small Treasure Chest*/, 244449 /*Reflective Mirror*/, 246559 /*Small Treasure Chest*/, 247226 /*Collision Wall for Bridge*/, 244409 /*Abcess*/, 246560 /*Small Treasure Chest*/, 246558 /*Small Treasure Chest*/, 245521 /*Sentry*/, 245523 /*Sentry*/, 245522 /*Sentry*/, 244412 /*Abcess*/, 244408 /*Abcess*/, 244411 /*Abcess*/, 244410 /*Abcess*/, 246557 /*Small Treasure Chest*/, 246746 /*Warden Owl*/, 244420 /*Demon Hunter Trap*/, 246747 /*Warden Owl*/, 246749 /*Warden Owl*/, 253400 /*Vault of the Betrayer*/, 246556 /*Small Treasure Chest*/, 245467 /*Vault of the Betrayer*/, 246748 /*Warden Owl*/, 246750 /*Warden Owl*/, 246752 /*Warden Owl*/, 244417 /*Legion Gate*/, 246751 /*Warden Owl*/, 246555 /*Small Treasure Chest*/, 244925 /*Ward of the Hunters*/);
INSERT INTO `gameobject_template_addon` (`entry`, `faction`, `flags`, `WorldEffectID`, `AIAnimKitID`) VALUES
(529989, 0, 0x2000, 0, 0), -- Barbershop Pole [DNT]
(645097, 0, 0x20, 0, 0), -- Portal Mirror [DNT]
(259879, 1375, 0x10, 0, 0), -- Collision Wall
(617539, 0, 0x20, 0, 0), -- Stormwind Portal
(246562, 0, 0x204000, 2437, 0), -- Small Treasure Chest
(246561, 0, 0x204000, 2437, 0), -- Small Treasure Chest
(244449, 0, 0x40000, 0, 0), -- Reflective Mirror
(246559, 0, 0x204000, 2437, 0), -- Small Treasure Chest
(247226, 114, 0x0, 0, 0), -- Collision Wall for Bridge
(244409, 0, 0x2000, 0, 0), -- Abcess
(246560, 0, 0x0, 2437, 0), -- Small Treasure Chest
(246558, 0, 0x204000, 2437, 0), -- Small Treasure Chest
(245521, 1375, 0x0, 0, 0), -- Sentry
(245523, 1375, 0x0, 0, 0), -- Sentry
(245522, 1375, 0x0, 0, 0), -- Sentry
(244412, 0, 0x2000, 0, 0), -- Abcess
(244408, 0, 0x2000, 0, 0), -- Abcess
(244411, 0, 0x2000, 0, 0), -- Abcess
(244410, 0, 0x2000, 0, 0), -- Abcess
(246557, 0, 0x204000, 2437, 0), -- Small Treasure Chest
(246746, 1375, 0x0, 0, 0), -- Warden Owl
(244420, 0, 0x0, 0, 1696), -- Demon Hunter Trap
(246747, 1375, 0x0, 0, 0), -- Warden Owl
(246749, 1375, 0x0, 0, 0), -- Warden Owl
(253400, 114, 0x0, 0, 11129), -- Vault of the Betrayer
(246556, 0, 0x204000, 2437, 0), -- Small Treasure Chest
(245467, 114, 0x0, 0, 0), -- Vault of the Betrayer
(246748, 1375, 0x0, 0, 0), -- Warden Owl
(246750, 1375, 0x0, 0, 0), -- Warden Owl
(246752, 1375, 0x0, 0, 0), -- Warden Owl
(244417, 0, 0x0, 0, 3761), -- Legion Gate
(246751, 1375, 0x0, 0, 0), -- Warden Owl
(246555, 0, 0x0, 2437, 0), -- Small Treasure Chest
(244925, 114, 0x0, 0, 0); -- Ward of the Hunters

UPDATE `gameobject_template_addon` SET `faction`=1733 WHERE `entry`=259114; -- [DNT] Command Table Collision Cylinder
UPDATE `gameobject_template_addon` SET `flags`=0x20800000 WHERE `entry`=180415; -- CandleBlack01

DELETE FROM `conversation_template` WHERE `Id` IN (3827, 3256, 3053, 1197, 538, 536, 1184, 833, 1183, 4086, 4085, 4084, 3443, 530, 529, 528, 527, 1181, 506);
INSERT INTO `conversation_template` (`Id`, `FirstLineID`, `TextureKitId`, `VerifiedBuild`) VALUES
(3827, 8609, 0, 70575),
(3256, 7927, 0, 70575),
(3053, 6493, 0, 70575),
(1197, 2845, 0, 70575),
(538, 1402, 0, 70575),
(536, 1396, 0, 70575),
(1184, 2804, 0, 70575),
(833, 2046, 0, 70575),
(1183, 2803, 0, 70575),
(4086, 9378, 0, 70575),
(4085, 9377, 0, 70575),
(4084, 9376, 0, 70575),
(3443, 7217, 0, 70575),
(530, 1381, 0, 70575),
(529, 1379, 0, 70575),
(528, 1377, 0, 70575),
(527, 1375, 0, 70575),
(1181, 2801, 0, 70575),
(506, 1348, 0, 70575);

DELETE FROM `conversation_line_template` WHERE `Id` IN (9376, 1380, 1379, 2047, 2046, 1819, 1382, 1381, 2803, 9378, 2801, 1349, 1348, 7219, 7218, 7217, 6856, 6855, 6854, 7927, 1376, 1375, 9377, 1404, 1402, 2804, 2845, 8610, 8609, 1399, 1398, 1397, 1396, 8103, 6497, 6494, 6493, 1378, 1377);
INSERT INTO `conversation_line_template` (`Id`, `UiCameraID`, `ActorIdx`, `Flags`, `ChatType`, `VerifiedBuild`) VALUES
(9376, 0, 0, 0, 0, 70575),
(1380, 0, 1, 0, 0, 70575),
(1379, 0, 0, 0, 0, 70575),
(2047, 0, 0, 0, 0, 70575),
(2046, 0, 0, 0, 0, 70575),
(1819, 0, 0, 0, 0, 70575),
(1382, 0, 1, 0, 0, 70575),
(1381, 0, 0, 0, 0, 70575),
(2803, 143, 0, 0, 0, 70575),
(9378, 0, 0, 0, 0, 70575),
(2801, 143, 0, 0, 0, 70575),
(1349, 0, 1, 0, 0, 70575),
(1348, 0, 0, 0, 0, 70575),
(7219, 0, 0, 0, 0, 70575),
(7218, 0, 1, 0, 0, 70575),
(7217, 0, 0, 0, 0, 70575),
(6856, 0, 3, 0, 0, 70575),
(6855, 0, 2, 0, 0, 70575),
(6854, 0, 1, 0, 0, 70575),
(7927, 0, 0, 0, 0, 70575),
(1376, 0, 1, 0, 0, 70575),
(1375, 0, 0, 0, 0, 70575),
(9377, 0, 0, 0, 0, 70575),
(1404, 0, 0, 0, 0, 70575),
(1402, 0, 0, 0, 0, 70575),
(2804, 143, 0, 0, 0, 70575),
(2845, 0, 0, 0, 0, 70575),
(8610, 82, 0, 0, 0, 70575),
(8609, 82, 0, 0, 0, 70575),
(1399, 0, 1, 0, 0, 70575),
(1398, 0, 2, 0, 0, 70575),
(1397, 0, 1, 0, 0, 70575),
(1396, 0, 0, 0, 0, 70575),
(8103, 0, 3, 0, 0, 70575),
(6497, 0, 2, 0, 0, 70575),
(6494, 0, 1, 0, 0, 70575),
(6493, 0, 0, 0, 0, 70575),
(1378, 0, 1, 0, 0, 70575),
(1377, 0, 0, 0, 0, 70575);


DELETE FROM `conversation_actors` WHERE (`Idx`=0 AND `ConversationId` IN (4084,529,833,530,1183,4086,1181,506,3443,3256,527,4085,538,1184,1197,3827,536,3053,528)) OR (`Idx`=1 AND `ConversationId` IN (529,530,506,3443,3256,527,536,3053,528)) OR (`Idx`=3 AND `ConversationId` IN (3256,3053)) OR (`Idx`=2 AND `ConversationId` IN (3256,536,3053));
INSERT INTO `conversation_actors` (`ConversationId`, `ConversationActorId`, `Idx`, `CreatureId`, `CreatureDisplayInfoId`, `NoActorObject`, `ActivePlayerObject`, `VerifiedBuild`) VALUES
(4084, 56412, 0, 0, 0, 0, 0, 70575), -- Full: 0x204258B78061F2C0001730000063829E Creature/0 R4246/S5936 Map: 1468 (Warden Prison DH Quests) Entry: 100299 (Lyana Darksorrow) Low: 6521502
(529, 0, 1, 0, 0, 0, 0, 70575), -- Full: 0x204258B7805ACE0000173000006386A8 Creature/0 R4246/S5936 Map: 1468 (Warden Prison DH Quests) Entry: 92984 (Kayn Sunfury) Low: 6522536
(529, 0, 0, 0, 0, 0, 0, 70575), -- Full: 0x204258B7805ACE4000173000006386A8 Creature/0 R4246/S5936 Map: 1468 (Warden Prison DH Quests) Entry: 92985 (Altruis the Sufferer) Low: 6522536
(833, 50505, 0, 0, 0, 0, 0, 70575), -- Full: 0x204258B7805F5AC0001730000063829E Creature/0 R4246/S5936 Map: 1468 (Warden Prison DH Quests) Entry: 97643 (Kor'vas Bloodthorn) Low: 6521502
(530, 0, 0, 0, 0, 0, 0, 70575), -- Full: 0x204258B7805ACE4000173000006386A8 Creature/0 R4246/S5936 Map: 1468 (Warden Prison DH Quests) Entry: 92985 (Altruis the Sufferer) Low: 6522536
(530, 0, 1, 0, 0, 0, 0, 70575), -- Full: 0x204258B7805ACE0000173000006386A8 Creature/0 R4246/S5936 Map: 1468 (Warden Prison DH Quests) Entry: 92984 (Kayn Sunfury) Low: 6522536
(1183, 49309, 0, 96783, 66806, 0, 0, 70575),
(4086, 56414, 0, 0, 0, 0, 0, 70575), -- Full: 0x204258B7805ECB80001730000063829E Creature/0 R4246/S5936 Map: 1468 (Warden Prison DH Quests) Entry: 97070 (Jace Darkweaver) Low: 6521502
(1181, 49309, 0, 96783, 66806, 0, 0, 70575),
(506, 49610, 1, 0, 0, 0, 0, 70575), -- Full: 0x204258B780614C00001730000063829E Creature/0 R4246/S5936 Map: 1468 (Warden Prison DH Quests) Entry: 99632 (Altruis the Sufferer) Low: 6521502
(506, 49611, 0, 0, 0, 0, 0, 70575), -- Full: 0x204258B780614BC0001730000063829E Creature/0 R4246/S5936 Map: 1468 (Warden Prison DH Quests) Entry: 99631 (Kayn Sunfury) Low: 6521502
(3443, 54992, 0, 0, 0, 0, 0, 70575), -- Full: 0x204258B7805E66400017300000638919 Creature/0 R4246/S5936 Map: 1468 (Warden Prison DH Quests) Entry: 96665 (Kayn Sunfury) Low: 6523161
(3443, 54993, 1, 0, 0, 0, 0, 70575), -- Full: 0x204258B7805ACE800017300000638919 Creature/0 R4246/S5936 Map: 1468 (Warden Prison DH Quests) Entry: 92986 (Altruis the Sufferer) Low: 6523161
(3256, 55139, 3, 0, 0, 0, 0, 70575), -- Full: 0x2042580000629C8000182A0000639BC1 Creature/0 R4246/S6186 Map: 0 (Eastern Kingdoms) Entry: 100978 (Gelbin Mekkatorque) Low: 6527937
(3256, 55138, 2, 0, 0, 0, 0, 70575), -- Full: 0x2042580000629D0000182A0000639BC1 Creature/0 R4246/S6186 Map: 0 (Eastern Kingdoms) Entry: 100980 (Tyrande Whisperwind) Low: 6527937
(3256, 55140, 1, 0, 0, 0, 0, 70575), -- Full: 0x2042580000629D4000182A0000639BC1 Creature/0 R4246/S6186 Map: 0 (Eastern Kingdoms) Entry: 100981 (Prophet Velen) Low: 6527937
(3256, 55182, 0, 0, 0, 0, 0, 70575), -- Full: 0x0
(527, 49445, 1, 0, 0, 0, 0, 70575), -- Full: 0x204258B7805E66400017300000E3829E Creature/0 R4246/S5936 Map: 1468 (Warden Prison DH Quests) Entry: 96665 (Kayn Sunfury) Low: 14910110
(527, 49446, 0, 0, 0, 0, 0, 70575), -- Full: 0x204258B7805A8B800017300000E3829E Creature/0 R4246/S5936 Map: 1468 (Warden Prison DH Quests) Entry: 92718 (Maiev Shadowsong) Low: 14910110
(4085, 56413, 0, 0, 0, 0, 0, 70575), -- Full: 0x204258B78061F280001730000063829E Creature/0 R4246/S5936 Map: 1468 (Warden Prison DH Quests) Entry: 100298 (Marius Felbane) Low: 6521502
(538, 49523, 0, 0, 0, 0, 0, 70575), -- Full: 0x204258B7805FAE80001730000063829E Creature/0 R4246/S5936 Map: 1468 (Warden Prison DH Quests) Entry: 97978 (Archmage Khadgar) Low: 6521502
(1184, 49309, 0, 96783, 66806, 0, 0, 70575),
(1197, 51327, 0, 0, 0, 0, 0, 70575), -- Full: 0x0
(3827, 55564, 0, 113986, 65834, 0, 0, 70575),
(536, 49485, 1, 0, 0, 0, 0, 70575), -- Full: 0x204258B7805EFC40001730000063829E Creature/0 R4246/S5936 Map: 1468 (Warden Prison DH Quests) Entry: 97265 (Kayn Sunfury) Low: 6521502
(536, 49484, 2, 0, 0, 0, 0, 70575), -- Full: 0x204258B7805EFCC00017300000E3829E Creature/0 R4246/S5936 Map: 1468 (Warden Prison DH Quests) Entry: 97267 (Altruis the Sufferer) Low: 14910110
(536, 49486, 0, 0, 0, 0, 0, 70575), -- Full: 0x204258B7805F5B00001730000063829E Creature/0 R4246/S5936 Map: 1468 (Warden Prison DH Quests) Entry: 97644 (Kor'vas Bloodthorn) Low: 6521502
(3053, 50385, 3, 0, 0, 0, 0, 70575), -- Full: 0x204258000062134000182A0000639BC1 Creature/0 R4246/S6186 Map: 0 (Eastern Kingdoms) Entry: 100429 (Anduin Wrynn) Low: 6527937
(3053, 50697, 2, 0, 0, 0, 0, 70575), -- Full: 0x0
(3053, 50402, 1, 0, 0, 0, 0, 70575), -- Full: 0x0
(3053, 50660, 0, 0, 0, 0, 0, 70575), -- Full: 0x2042580000621A4000182A0000639BC1 Creature/0 R4246/S6186 Map: 0 (Eastern Kingdoms) Entry: 100457 (Malfurion Stormrage) Low: 6527937
(528, 0, 1, 0, 0, 0, 0, 70575), -- Full: 0x204258B7805ACE0000173000006386A8 Creature/0 R4246/S5936 Map: 1468 (Warden Prison DH Quests) Entry: 92984 (Kayn Sunfury) Low: 6522536
(528, 0, 0, 0, 0, 0, 0, 70575); -- Full: 0x204258B7805ACE4000173000006386A8 Creature/0 R4246/S5936 Map: 1468 (Warden Prison DH Quests) Entry: 92985 (Altruis the Sufferer) Low: 6522536


DELETE FROM `areatrigger_create_properties` WHERE (`IsCustom`=0 AND `Id` IN (5572,5130,5269,7082,7083,4915,4919,5693,8318,5548));
INSERT INTO `areatrigger_create_properties` (`Id`, `IsCustom`, `AreaTriggerId`, `IsAreatriggerCustom`, `Flags`, `MoveCurveId`, `ScaleCurveId`, `MorphCurveId`, `FacingCurveId`, `AnimId`, `AnimKitId`, `DecalPropertiesId`, `SpellForVisuals`, `PositionalSoundKitId`, `TimeToTargetScale`, `Speed`, `Shape`, `ShapeData0`, `ShapeData1`, `ShapeData2`, `ShapeData3`, `ShapeData4`, `ShapeData5`, `ShapeData6`, `ShapeData7`, `Roll`, `Pitch`, `Yaw`, `TargetRoll`, `TargetPitch`, `TargetYaw`, `VerifiedBuild`) VALUES
(5572, 0, 10288, 0, 0, 0, 1669, 0, 0, -1, 0, 0, NULL, 0, 12000, 6.249996185302734375, 4, 0.5, 0.5, 1.5, 1.5, 0.300000011920928955, 0.300000011920928955, 0, 0, 0, 0, 0, NULL, NULL, NULL, 70575), -- Spell: 199761 (Burning Fel)
(5130, 0, 9856, 0, 0, 0, 0, 0, 0, -1, 0, 0, NULL, 0, 600000, 10.00007915496826171, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL, 70575), -- Spell: 202202 (Pulse)
(5269, 0, 9991, 0, 0, 0, 0, 0, 0, -1, 0, 0, NULL, 0, 150000, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL, 70575), -- Spell: 196503 (Lingering Gaze)
(7082, 0, 11551, 0, 0, 0, 0, 0, 0, -1, 0, 35, NULL, 0, 600000, 2.000015974044799804, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL, 70575), -- Spell: 213212 (Lava Wreath)
(7083, 0, 11552, 0, 4, 0, 0, 0, 0, -1, 0, 35, NULL, 0, 600000, 0.890484035015106201, 0, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL, 70575), -- Spell: 213214 (Fissure)
(4915, 0, 9701, 0, 4, 0, 0, 0, 0, -1, 0, 35, NULL, 0, 600000, 7.122277259826660156, 0, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL, 70575), -- Spell: 193238 (Fissure)
(4919, 0, 9709, 0, 0, 0, 0, 0, 0, -1, 0, 35, NULL, 0, 600000, 1.999995231628417968, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL, 70575), -- Spell: 192571 (Lava Wreath)
(5693, 0, 10400, 0, 0, 0, 0, 0, 0, -1, 0, 0, NULL, 0, 60000, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL, 70575), -- Spell: 201035 (Seeping Eye of Kethrazor)
(8318, 0, 12504, 0, 0, 0, 0, 0, 0, -1, 0, 0, NULL, 0, 15000, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL, 70575), -- Spell: 223473 (Shear)
(5548, 0, 10259, 0, 0, 0, 0, 0, 0, -1, 0, 0, NULL, 0, 0, 0, 4, 8, 8, 6, 6, 0.300000011920928955, 0.300000011920928955, 0, 0, 0, 0, 0, NULL, NULL, NULL, 70575); -- Spell: 199477 (Shoulder Charge)


DELETE FROM `areatrigger_create_properties_orbit` WHERE (`IsCustom`=0 AND `AreaTriggerCreatePropertiesId` IN (7082,4919));
INSERT INTO `areatrigger_create_properties_orbit` (`AreaTriggerCreatePropertiesId`, `IsCustom`, `ExtraTimeForBlending`, `CircleRadius`, `BlendFromRadius`, `InitialAngle`, `ZOffset`, `CounterClockwise`, `CanLoop`, `VerifiedBuild`) VALUES
(7082, 0, 0, 14.50399398803710937, 14.50399398803710937, 4.188794136047363281, 0, 1, 0, 70575), -- Spell: 213212 (Lava Wreath)
(4919, 0, 0, 14.64811515808105468, 14.64811515808105468, 2.09440779685974121, 0, 0, 0, 70575); -- Spell: 192571 (Lava Wreath)


DELETE FROM `areatrigger_create_properties_spline_point` WHERE (`AreaTriggerCreatePropertiesId`=5572 AND `IsCustom`=0 AND `Idx`=3) OR (`AreaTriggerCreatePropertiesId`=5572 AND `IsCustom`=0 AND `Idx`=2) OR (`AreaTriggerCreatePropertiesId`=5572 AND `IsCustom`=0 AND `Idx`=1) OR (`AreaTriggerCreatePropertiesId`=5572 AND `IsCustom`=0 AND `Idx`=0) OR (`AreaTriggerCreatePropertiesId`=5130 AND `IsCustom`=0 AND `Idx`=3) OR (`AreaTriggerCreatePropertiesId`=5130 AND `IsCustom`=0 AND `Idx`=2) OR (`AreaTriggerCreatePropertiesId`=5130 AND `IsCustom`=0 AND `Idx`=1) OR (`AreaTriggerCreatePropertiesId`=5130 AND `IsCustom`=0 AND `Idx`=0) OR (`AreaTriggerCreatePropertiesId`=4915 AND `IsCustom`=0 AND `Idx`=3) OR (`AreaTriggerCreatePropertiesId`=4915 AND `IsCustom`=0 AND `Idx`=2) OR (`AreaTriggerCreatePropertiesId`=4915 AND `IsCustom`=0 AND `Idx`=1) OR (`AreaTriggerCreatePropertiesId`=4915 AND `IsCustom`=0 AND `Idx`=0) OR (`AreaTriggerCreatePropertiesId`=7083 AND `IsCustom`=0 AND `Idx`=3) OR (`AreaTriggerCreatePropertiesId`=7083 AND `IsCustom`=0 AND `Idx`=2) OR (`AreaTriggerCreatePropertiesId`=7083 AND `IsCustom`=0 AND `Idx`=1) OR (`AreaTriggerCreatePropertiesId`=7083 AND `IsCustom`=0 AND `Idx`=0) OR (`AreaTriggerCreatePropertiesId`=7082 AND `IsCustom`=0 AND `Idx`=3) OR (`AreaTriggerCreatePropertiesId`=7082 AND `IsCustom`=0 AND `Idx`=2) OR (`AreaTriggerCreatePropertiesId`=7082 AND `IsCustom`=0 AND `Idx`=1) OR (`AreaTriggerCreatePropertiesId`=7082 AND `IsCustom`=0 AND `Idx`=0);
INSERT INTO `areatrigger_create_properties_spline_point` (`AreaTriggerCreatePropertiesId`, `IsCustom`, `Idx`, `X`, `Y`, `Z`, `VerifiedBuild`) VALUES
(5572, 0, 3, 2.80908203125, -69.630859375, 0, 70575), -- Spell: 199761 (Burning Fel)
(5572, 0, 2, 2.80908203125, -69.630859375, 0, 70575), -- Spell: 199761 (Burning Fel)
(5572, 0, 1, -0.21435546875, 5.308197021484375, 0, 70575), -- Spell: 199761 (Burning Fel)
(5572, 0, 0, -0.21435546875, 5.308197021484375, 0, 70575), -- Spell: 199761 (Burning Fel)
(5130, 0, 3, 48.47210311889648437, 23.95968055725097656, 0, 70575), -- Spell: 202202 (Pulse)
(5130, 0, 2, 48.47210311889648437, 23.95968055725097656, 0, 70575), -- Spell: 202202 (Pulse)
(5130, 0, 1, -0, -0, 0, 70575), -- Spell: 202202 (Pulse)
(5130, 0, 0, -0, -0, 0, 70575), -- Spell: 202202 (Pulse)
(4915, 0, 3, -6.30872249603271484, 3.66658949851989746, 0, 70575), -- Spell: 193238 (Fissure)
(4915, 0, 2, -6.30872249603271484, 3.66658949851989746, 0, 70575), -- Spell: 193238 (Fissure)
(4915, 0, 1, -5.49183368682861328, 4.015630245208740234, 0, 70575), -- Spell: 193238 (Fissure)
(4915, 0, 0, -5.49183368682861328, 4.015630245208740234, 0, 70575), -- Spell: 193238 (Fissure)
(7083, 0, 3, -50.8596076965332031, -28.6704578399658203, 0, 70575), -- Spell: 213214 (Fissure)
(7083, 0, 2, -50.8596076965332031, -28.6704578399658203, 0, 70575), -- Spell: 213214 (Fissure)
(7083, 0, 1, -44.3610343933105468, -31.550302505493164, 0, 70575), -- Spell: 213214 (Fissure)
(7083, 0, 0, -44.3610343933105468, -31.550302505493164, 0, 70575), -- Spell: 213214 (Fissure)
(7082, 0, 3, 15.00000476837158203, 0.000001204605837301, 0, 70575), -- Spell: 213212 (Lava Wreath)
(7082, 0, 2, 15.00000476837158203, 0.000001204605837301, 0, 70575), -- Spell: 213212 (Lava Wreath)
(7082, 0, 1, -0, -0, 0, 70575), -- Spell: 213212 (Lava Wreath)
(7082, 0, 0, -0, -0, 0, 70575); -- Spell: 213212 (Lava Wreath)

UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=10288 AND `IsCustom`=0);
UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=9856 AND `IsCustom`=0);
UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=9991 AND `IsCustom`=0);
UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=11551 AND `IsCustom`=0);
UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=11552 AND `IsCustom`=0);
UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=9701 AND `IsCustom`=0);
UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=9709 AND `IsCustom`=0);
UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=10400 AND `IsCustom`=0);
UPDATE `areatrigger_template` SET `VerifiedBuild`=70575 WHERE (`Id`=12504 AND `IsCustom`=0);

DELETE FROM `areatrigger_template` WHERE (`Id`=10259 AND `IsCustom`=0);
INSERT INTO `areatrigger_template` (`Id`, `IsCustom`, `VerifiedBuild`) VALUES
(10259, 0, 70575);

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN (103655,103658,99501,96659,114358,99709,99731,99732,42339,141270,198509,207684);
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `cast_flags`, `user_type`) VALUES
(103655, 184012, 1, 0),
(103658, 177803, 1, 0),
(99501, 202064, 1, 0),
(96659, 195537, 1, 0),
(114358, 202064, 1, 0),
(99709, 204588, 1, 0),
(99732, 204588, 1, 0),
(99731, 204588, 1, 0),
(42339, 78974, 1, 0),
(141270, 224326, 1, 0),
(198509, 224326, 1, 0),
(207684, 408907, 1, 0);

DELETE FROM `graveyard_zone` WHERE `GhostZone` = 7814;
INSERT INTO `graveyard_zone` (`ID`, `GhostZone`, `Comment`) VALUES
(8001, 7814, 'DH Vault of Wardens'),
(20046, 7814, 'Warden Prision DH Quest');


-- SAI
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` IN (92776);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (92776) AND source_type = 0;
INSERT INTO smart_scripts (entryorguid, source_type, id, link, Difficulties, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, event_param5, event_param_string, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, action_param7, action_param_string, target_type, target_param1, target_param2, target_param3, target_param4, target_param_string, target_x, target_y, target_z, target_o, comment) VALUES
(92776, 0, 1, 0, '', 4, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Fel Shocktrooper - On Aggro - Talk (No Repeat)'),
(92776, 0, 2, 0, '', 0, 0, 100, 0, 3000, 8000, 12000, 15000, 0, '', 11, 200930, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Fel Shocktrooper - In Combat - Cast Chaos Slash'),
(92776, 0, 3, 0, '', 0, 0, 100, 0, 3000, 9000, 15000, 25000, 0, '', 11, 200963, 32, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Fel Shocktrooper - In Combat - Cast Bladestorm (Aura Not Present)'),
(92776, 0, 5, 0, '', 11, 0, 50, 0, 1, 1468, 0, 0, 0, '', 49, 0, 0, 0, 0, 0, 0, 0, '', 25, 100, 0, 0, 0, '', 0, 0, 0, 0, 'Fel Shocktrooper - On Respawn - Attack Closest Enemy');

-- SAI
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` IN (92782);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (92782) AND source_type = 0;
INSERT INTO smart_scripts (entryorguid, source_type, id, link, Difficulties, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, event_param5, event_param_string, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, action_param7, action_param_string, target_type, target_param1, target_param2, target_param3, target_param4, target_param_string, target_x, target_y, target_z, target_o, comment) VALUES
(92782, 0, 1, 0, '', 0, 0, 100, 0, 0, 1500, 12000, 18000, 0, '', 11, 200992, 32, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Savage Felstalker - In Combat - Cast Savage Assault (Aura Not Present)'),
(92782, 0, 2, 0, '', 0, 0, 100, 0, 4000, 8000, 24000, 27000, 0, '', 11, 200498, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Savage Felstalker - In Combat - Cast Abyss Howl'),
(92782, 0, 4, 0, '', 11, 0, 30, 0, 1, 1468, 0, 0, 0, '', 49, 0, 0, 0, 0, 0, 0, 0, '', 25, 100, 0, 0, 0, '', 0, 0, 0, 0, 'Savage Felstalker - On Respawn - Attack Closest Enemy');

-- SAI
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` IN (96656);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (96656) AND source_type = 0;
INSERT INTO smart_scripts (entryorguid, source_type, id, link, Difficulties, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, event_param5, event_param_string, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, action_param7, action_param_string, target_type, target_param1, target_param2, target_param3, target_param4, target_param_string, target_x, target_y, target_z, target_o, comment) VALUES
(96656, 0, 1, 0, '', 0, 0, 100, 0, 0, 2500, 20000, 22000, 0, '', 11, 201122, 32, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Freed Illidari - In Combat - Cast Immolation Aura (Aura Not Present)'),
(96656, 0, 2, 0, '', 0, 0, 100, 0, 0, 2000, 5500, 6500, 0, '', 11, 197639, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Freed Illidari - In Combat - Cast Chaos Strike'),
(96656, 0, 3, 0, '', 0, 0, 100, 0, 0, 7000, 20000, 24000, 0, '', 11, 200879, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Freed Illidari - In Combat - Cast Fel Rush');

-- SAI - Drelanim Whisperwind (96847), warden holding the line vs the demon waves
-- in the Vault of Silence. Sniffed rotation (cap 4, ~25min of combat):
-- Rupture spam, Fan of Blades ~30-40s (1291385 is triggered by 195783 - no
-- separate cast), Knockdown Kick ~60-90s, Turn Kick ~90-120s.
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` IN (96847);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (96847) AND source_type = 0;
INSERT INTO smart_scripts (entryorguid, source_type, id, link, Difficulties, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, event_param5, event_param_string, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, action_param7, action_param_string, target_type, target_param1, target_param2, target_param3, target_param4, target_param_string, target_x, target_y, target_z, target_o, comment) VALUES
(96847, 0, 1, 0, '', 0, 0, 100, 0, 0, 2000, 4000, 7000, 0, '', 11, 15583, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Drelanim Whisperwind - In Combat - Cast Rupture'),
(96847, 0, 2, 0, '', 0, 0, 100, 0, 6000, 10000, 30000, 40000, 0, '', 11, 195783, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Drelanim Whisperwind - In Combat - Cast Fan of Blades'),
(96847, 0, 3, 0, '', 0, 0, 100, 0, 10000, 15000, 60000, 90000, 0, '', 11, 189467, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Drelanim Whisperwind - In Combat - Cast Knockdown Kick'),
(96847, 0, 4, 0, '', 0, 0, 100, 0, 15000, 25000, 90000, 120000, 0, '', 11, 189469, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Drelanim Whisperwind - In Combat - Cast Turn Kick');

-- Drelanim's "I will hold the line" whisper: retail self-casts the CSA aura
-- 138041 (CSA Area Trigger Dummy Timer Aura B, 15min duration) on the player
-- inside the Vault of Silence and Drelanim whispers ~150ms later. The aura is
-- the per-player cooldown - the sniffer's second whisper came ~16.8min after
-- the first once it expired. No server-side AT exists to hook, so a custom
-- serverside AT (sphere r=10 at the room center, near both sniffed trigger
-- points 4328.2,-536.0 / 4331.2,-508.4) drives it via
-- at_drelanim_hold_the_line, which applies the same aura as the gate.
DELETE FROM `areatrigger_template` WHERE `Id`=70003 AND `IsCustom`=1;
INSERT INTO `areatrigger_template` (`Id`, `IsCustom`, `Flags`, `ActionSetId`, `ActionSetFlags`, `VerifiedBuild`) VALUES
(70003, 1, 1, 0, 0, 70575); -- Flags=1 IsServerSide (invisible unless .debug areatriggers)

DELETE FROM `areatrigger_create_properties` WHERE `Id`=70003 AND `IsCustom`=1;
INSERT INTO `areatrigger_create_properties` (`Id`, `IsCustom`, `AreaTriggerId`, `IsAreatriggerCustom`, `Flags`, `MoveCurveId`, `ScaleCurveId`, `MorphCurveId`, `FacingCurveId`, `AnimId`, `AnimKitId`, `DecalPropertiesId`, `SpellForVisuals`, `TimeToTargetScale`, `Speed`, `SpeedIsTime`, `Shape`, `ShapeData0`, `ShapeData1`, `ShapeData2`, `ShapeData3`, `ShapeData4`, `ShapeData5`, `ShapeData6`, `ShapeData7`, `VerifiedBuild`) VALUES
(70003, 1, 70003, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 10, 0, 0, 0, 0, 0, 0, 70575); -- Shape 0 = Sphere, r=10

SET @ATSPAWNID := 100200;
DELETE FROM `areatrigger` WHERE `SpawnId`=@ATSPAWNID;
INSERT INTO `areatrigger` (`SpawnId`, `AreaTriggerCreatePropertiesId`, `IsCustom`, `MapId`, `SpawnDifficulties`, `PosX`, `PosY`, `PosZ`, `Orientation`, `PhaseUseFlags`, `PhaseId`, `PhaseGroup`, `ScriptName`, `Comment`, `VerifiedBuild`) VALUES
(@ATSPAWNID+0, 70003, 1, 1468, '0', 4328.0, -522.0, -287.2, 0, 0, 0, 0, 'at_drelanim_hold_the_line', 'Vault of Silence - Drelanim hold-the-line whisper zone', 70575);


-- SAI - Mirana Starlight (99451), warden in a permanent duel with Lekos and the
-- corridor demons (sniffed ~30min). Eviscerate every ~11-15s in melee; Throwing
-- Blade (191029) only while out of range after Lekos's knockback or his respawn -
-- its DUMMY effects hold the two missile variants (190898/191026) and
-- spell_vault_throwing_blade picks one per target, matching the sniffed pairs.
-- Fan of Blades rare (2 casts in 30min).
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` IN (99451);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (99451) AND source_type = 0;
INSERT INTO smart_scripts (entryorguid, source_type, id, link, Difficulties, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, event_param5, event_param_string, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, action_param7, action_param_string, target_type, target_param1, target_param2, target_param3, target_param4, target_param_string, target_x, target_y, target_z, target_o, comment) VALUES
(99451, 0, 1, 0, '', 0, 0, 100, 0, 2000, 4000, 11000, 15000, 0, '', 11, 191058, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mirana Starlight - In Combat - Cast Eviscerate'),
(99451, 0, 2, 0, '', 9, 0, 100, 0, 8, 30, 1500, 3000, 0, '', 11, 191029, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mirana Starlight - On Victim in 8-30y - Cast Throwing Blade'),
(99451, 0, 3, 0, '', 0, 0, 100, 0, 8000, 15000, 180000, 300000, 0, '', 11, 195783, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mirana Starlight - In Combat - Cast Fan of Blades');

DELETE FROM `spell_script_names` WHERE `spell_id`=191029;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(191029, 'spell_vault_throwing_blade'); -- Throwing Blade - dummy bp pick the blade-missile variant


-- Quest 38689 - Fel Infusion (Soul Fragment 89297 x100)
-- Sniffed chain: quest accept applies invisible aura 133508 on the player, dead
-- demons self-cast 133578 (soul-release visual 94045), and the aura then makes
-- the player cast 202154 Fill Fel which parents one 133511 kill credit per
-- absorbed soul (~8-13 bursts while packs die). quest_vault_wardens_fel_infusion
-- applies/removes the aura; spell_vault_fel_infusion runs the absorb sweep.
INSERT INTO `quest_template_addon` (`ID`, `ScriptName`) VALUES
(38689, 'quest_vault_wardens_fel_infusion')
ON DUPLICATE KEY UPDATE `ScriptName`='quest_vault_wardens_fel_infusion';

DELETE FROM `spell_script_names` WHERE `spell_id`=133508;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(133508, 'spell_vault_fel_infusion'); -- Fel Infusion aura - absorb sweep on heartbeat

-- Soul-release self-cast on death (sniffed on all three entries; the absorb
-- credits come from the player-side aura sweep, not the dying unit)
INSERT INTO smart_scripts (entryorguid, source_type, id, link, Difficulties, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, event_param5, event_param_string, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, action_param7, action_param_string, target_type, target_param1, target_param2, target_param3, target_param4, target_param_string, target_x, target_y, target_z, target_o, comment) VALUES
(92782, 0, 3, 0, '', 6, 0, 100, 0, 0, 0, 0, 0, 0, '', 11, 133578, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Savage Felstalker - On Just Died - Cast Fill Fel: Forcecast (soul release visual)'),
(92776, 0, 4, 0, '', 6, 0, 100, 0, 0, 0, 0, 0, 0, '', 11, 133578, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Fel Shocktrooper - On Just Died - Cast Fill Fel: Forcecast (soul release visual)');

-- Quest 38672 - Breaking Out
DELETE FROM `phase_name` WHERE `ID`=5121;
INSERT INTO `phase_name` (`ID`, `Name`) VALUES
(5121, 'Maiev Shadowsong Vault of Silence');

DELETE FROM `phase_area` WHERE `PhaseId`=5121;
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(7873, 5121, 'Maiev Shadowsong Vault of Silence');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=26 AND `SourceGroup`=5121;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(26, 5121, 7873, 0, 0, 14, 0, 38672, 0, 0, '', 0, 0, 0, '', 'Phase 5121 active while quest 38672 is not in the log (not taken or abandoned)');

DELETE FROM `phase_name` WHERE `ID`=5976;
INSERT INTO `phase_name` (`ID`, `Name`) VALUES
(5976, 'Ward of the Hunters Door - Vault of Silence');

DELETE FROM `phase_area` WHERE `PhaseId`=5976;
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(7873, 5976, 'Ward of the Hunters Door - Vault of Silence');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=26 AND `SourceGroup`=5976;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(26, 5976, 7873, 0, 0, 8, 0, 38672, 0, 0, '', 0, 0, 0, '', 'Phase 5976 active if Quest 38672 rewarded');

-- Ward of the Hunters static + door collision wall: sealed until 38672 is rewarded
-- (sniffed phase 5966 is dropped from the player shift at the reward transition)
DELETE FROM `phase_name` WHERE `ID`=5966;
INSERT INTO `phase_name` (`ID`, `Name`) VALUES
(5966, 'Ward of the Hunters - Sealed Door');

DELETE FROM `phase_area` WHERE `PhaseId`=5966 AND `AreaId`=7873;
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(7873, 5966, 'Ward of the Hunters - Sealed Door');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=26 AND `SourceGroup`=5966;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(26, 5966, 7873, 0, 0, 8, 0, 38672, 0, 0, '', 1, 0, 0, '', 'Phase 5966 active while Quest 38672 is not rewarded');

-- 92980/92986 hall questgiver statics (@CGUID+21/@CGUID+28): the sniffed phases
-- land when 38672 goes complete (the second freed objective) and persist through
-- the reward, so they gate on quest-state completed|rewarded. 5366 is the
-- tracked phase of SpawnTrackingId 1048180 (quest 38690 turn-in at Kayn's spot)
-- and 5367 of 1050760 (quest 38689 turn-in at Altruis's spot).
DELETE FROM `phase_name` WHERE `ID` IN (5366,5367);
INSERT INTO `phase_name` (`ID`, `Name`) VALUES
(5366, 'Kayn Sunfury - Hall Questgiver'),
(5367, 'Altruis the Sufferer - Hall Questgiver');

DELETE FROM `phase_area` WHERE `PhaseId` IN (5366,5367) AND `AreaId` IN (7872,7873);
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(7872, 5366, 'Kayn Sunfury - Hall Questgiver'),
(7873, 5366, 'Kayn Sunfury - Hall Questgiver'),
(7872, 5367, 'Altruis the Sufferer - Hall Questgiver'),
(7873, 5367, 'Altruis the Sufferer - Hall Questgiver');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=26 AND `SourceGroup` IN (5366,5367);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(26, 5366, 7872, 0, 0, 47, 0, 38672, 66, 0, '', 0, 0, 0, '', 'Phase 5366 active once Quest 38672 is completed or rewarded'),
(26, 5366, 7872, 0, 0, 47, 0, 38690, 106, 0, '', 1, 0, 0, '', 'Phase 5366 drops once Quest 38690 is in the log'),
(26, 5366, 7873, 0, 0, 47, 0, 38672, 66, 0, '', 0, 0, 0, '', 'Phase 5366 active once Quest 38672 is completed or rewarded'),
(26, 5366, 7873, 0, 0, 47, 0, 38690, 106, 0, '', 1, 0, 0, '', 'Phase 5366 drops once Quest 38690 is in the log'),
(26, 5367, 7872, 0, 0, 47, 0, 38672, 66, 0, '', 0, 0, 0, '', 'Phase 5367 active once Quest 38672 is completed or rewarded'),
(26, 5367, 7872, 0, 0, 47, 0, 38689, 106, 0, '', 1, 0, 0, '', 'Phase 5367 drops once Quest 38689 is in the log'),
(26, 5367, 7873, 0, 0, 47, 0, 38672, 66, 0, '', 0, 0, 0, '', 'Phase 5367 active once Quest 38672 is completed or rewarded'),
(26, 5367, 7873, 0, 0, 47, 0, 38689, 106, 0, '', 1, 0, 0, '', 'Phase 5367 drops once Quest 38689 is in the log');

-- Illidari advance: sniffed phase adds on the follow-up quest accepts
-- (5495 on 38690 accept, 5494 on 38689 accept)
DELETE FROM `phase_name` WHERE `ID` IN (5494,5495);
INSERT INTO `phase_name` (`ID`, `Name`) VALUES
(5494, 'Illidari Advance - Altruis (Fel Infusion)'),
(5495, 'Illidari Advance - Kayn (Rise of the Illidari)');

DELETE FROM `phase_area` WHERE `PhaseId` IN (5494,5495) AND `AreaId` IN (7872,7873);
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(7872, 5494, 'Illidari Advance - Altruis (Fel Infusion)'),
(7873, 5494, 'Illidari Advance - Altruis (Fel Infusion)'),
(7872, 5495, 'Illidari Advance - Kayn (Rise of the Illidari)'),
(7873, 5495, 'Illidari Advance - Kayn (Rise of the Illidari)');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=26 AND `SourceGroup` IN (5494,5495);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(26, 5494, 7872, 0, 0, 47, 0, 38689, 106, 0, '', 0, 0, 0, '', 'Phase 5494 active once Quest 38689 is in the log'),
(26, 5494, 7873, 0, 0, 47, 0, 38689, 106, 0, '', 0, 0, 0, '', 'Phase 5494 active once Quest 38689 is in the log'),
(26, 5495, 7872, 0, 0, 47, 0, 38690, 106, 0, '', 0, 0, 0, '', 'Phase 5495 active once Quest 38690 is in the log'),
(26, 5495, 7873, 0, 0, 47, 0, 38690, 106, 0, '', 0, 0, 0, '', 'Phase 5495 active once Quest 38690 is in the log');

UPDATE `creature_template` SET `ScriptName` = 'npc_maiev_shadowsong_vault_silence' WHERE `entry` = 92718;
UPDATE `creature_template` SET `ScriptName` = 'npc_ward_of_the_hunters_vault' WHERE `entry` = 100168;
UPDATE `creature_template` SET `ScriptName` = 'npc_vault_prison_cell' WHERE `entry` IN (103655, 103658);
UPDATE `creature_template` SET `ScriptName` = 'npc_vault_freed_illidari' WHERE `entry` IN (99631, 99632);
UPDATE `creature_template` SET `ScriptName` = 'npc_vault_postreward_illidari' WHERE `entry` IN (92980, 92986);
UPDATE `creature_template` SET `ScriptName` = 'npc_wrath_lord_lekos' WHERE `entry` = 97069;
UPDATE `creature_template` SET `ScriptName` = 'npc_legion_portal' WHERE `entry` = 99501;

-- Quest 38672 - Breaking Out: bind the C++ quest script (abandon cleanup + completion convo)
INSERT INTO `quest_template_addon` (`ID`, `ScriptName`) VALUES
(38672, 'quest_vault_wardens_breaking_out')
ON DUPLICATE KEY UPDATE `ScriptName`='quest_vault_wardens_breaking_out';

DELETE FROM `phase_name` WHERE `ID` IN (5124, 5125);
INSERT INTO `phase_name` (`ID`, `Name`) VALUES
(5124, 'Altruis Freed - Vault of Silence'),
(5125, 'Kayn Freed - Vault of Silence');

-- The in-scene phase swap is applied per-player by npc_vault_freed_illidari when the freed
-- NPC reaches the rally point (mirrors retail's delayed +5124/+5125 phase shift). The
-- phase_area conditions below only hold while the freed objective is complete and quest
-- 38672 is still in the quest log - retail strips both phases at quest reward.
DELETE FROM `phase_area` WHERE `PhaseId` IN (5124, 5125);
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(7873, 5124, 'Altruis Freed - Vault of Silence'),
(7873, 5125, 'Kayn Freed - Vault of Silence');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=26 AND `SourceGroup` IN (5124, 5125);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(26, 5124, 7873, 0, 0, 48, 0, 286143, 0, 1, '', 0, 0, 0, '', 'Phase 5124 while objective 286143 (Altruis End) complete and quest 38672 in log'),
(26, 5125, 7873, 0, 0, 48, 0, 280526, 0, 1, '', 0, 0, 0, '', 'Phase 5125 while objective 280526 (Kayn End) complete and quest 38672 in log');

-- Phases 5964/5965: the prison cells are present until their prisoner is freed.
-- Both conditions are negated and AND-ed: the phase drops the moment the freed
-- objective is credited, and stays off after the quest is rewarded (the freed
-- objective row alone would re-enable the phase once the quest leaves the log).
DELETE FROM `phase_name` WHERE `ID` IN (5964, 5965);
INSERT INTO `phase_name` (`ID`, `Name`) VALUES
(5964, 'Altruis Cell - Vault of Silence'),
(5965, 'Kayn Cell - Vault of Silence');

DELETE FROM `phase_area` WHERE `PhaseId` IN (5964, 5965);
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(7873, 5964, 'Altruis Cell - Vault of Silence'),
(7873, 5965, 'Kayn Cell - Vault of Silence');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=26 AND `SourceGroup` IN (5964, 5965);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(26, 5964, 7873, 0, 0, 48, 0, 278274, 0, 1, '', 1, 0, 0, '', 'Phase 5964 off once objective 278274 (Altruis freed) complete'),
(26, 5964, 7873, 0, 0, 8, 0, 38672, 0, 0, '', 1, 0, 0, '', 'Phase 5964 off once quest 38672 rewarded'),
(26, 5965, 7873, 0, 0, 48, 0, 278275, 0, 1, '', 1, 0, 0, '', 'Phase 5965 off once objective 278275 (Kayn freed) complete'),
(26, 5965, 7873, 0, 0, 8, 0, 38672, 0, 0, '', 1, 0, 0, '', 'Phase 5965 off once quest 38672 rewarded');

-- Spellclick gating: the cells are only clickable while Breaking Out is in progress
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=18 AND `SourceGroup` IN (103655,103658);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(18, 103655, 184012, 0, 0, 9, 0, 38672, 0, 0, '', 0, 0, 0, '', 'Altruis cell spellclick requires quest 38672 taken'),
(18, 103655, 184012, 0, 0, 8, 0, 38672, 0, 0, '', 1, 0, 0, '', 'Altruis cell spellclick blocked once quest 38672 rewarded'),
(18, 103658, 177803, 0, 0, 9, 0, 38672, 0, 0, '', 0, 0, 0, '', 'Kayn cell spellclick requires quest 38672 taken'),
(18, 103658, 177803, 0, 0, 8, 0, 38672, 0, 0, '', 1, 0, 0, '', 'Kayn cell spellclick blocked once quest 38672 rewarded'),
(18, 103655, 184012, 0, 0, 48, 0, 278274, 0, 0, '', 0, 0, 0, '', 'Altruis cell spellclick blocked once objective 278274 (Altruis freed) complete'),
(18, 103658, 177803, 0, 0, 48, 0, 278275, 0, 0, '', 0, 0, 0, '', 'Kayn cell spellclick blocked once objective 278275 (Kayn freed) complete');

-- Quest 38690 - Rise of the Illidari
-- Accept flow: npc_vault_postreward_illidari::OnQuestAccept summons the private
-- run-clone and refreshes phase_area conditions (static -5366, scene +5495).
-- The 8 interactable Warden Cells (244588) are spawn-tracked 1:1 to each
-- cell's named Illidari objective (280065-280072) in VaultWardensDK_Spawns.sql
-- (SpawnTrackingIds 1074361-1074368, states 0/1 animkit 947 sealed, state 2
-- animkit 9340 + worldeffects 5424 opened).
-- Use flow: go_vault_warden_cell::OnReportUse credits 96762 (generic counter
-- 278285) + the cell's named prisoner entry (marks the cell's tracking
-- complete -> per-player flip + GO_FLAG_IN_USE), then summons the prisoner as
-- a private player-owned clone (cell -> prisoner table in
-- Custom_VaultWardens_GameObjects.cpp).
UPDATE `gameobject_template` SET `ScriptName`='go_vault_warden_cell' WHERE `entry`=244588;
UPDATE `creature_template` SET `ScriptName`='npc_vault_freed_named_illidari' WHERE `entry` IN (97614, 97615, 97616, 97617, 97618, 97619, 97620, 97622);

-- Quest 40253 - Stop Gul'dan!
DELETE FROM `quest_template_addon` WHERE `ID`=40253;
INSERT INTO `quest_template_addon` (`ID`, `PrevQuestID`) VALUES
(40253, 38690);
