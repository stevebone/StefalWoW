-- Darnassus Fixes

-- NPC: 6034 Lotherias
-- NPC: 4156 Astaia <Fishing Trainer>
-- NPC: 4222 Voloren <Fishing Supplies>

DELETE FROM `creature_equip_template` WHERE `CreatureID` = 6034 AND `ID` = 1 AND `ItemID1` = 5278;
INSERT INTO `creature_equip_template` (`CreatureID`, `ID`, `ItemID1`, `AppearanceModID1`, `ItemVisual1`,  `ItemID2`, `AppearanceModID2`, `ItemVisual2`,  `ItemID3`, `AppearanceModID3`, `ItemVisual3`, `VerifiedBuild`) VALUES 
(6034, 1, 5278, 0, 0, 0, 0, 0, 0, 0, 0, 67186);

-- Add swim flag
UPDATE creature_template_difficulty SET StaticFlags1 = StaticFlags1 | 0x10000000 WHERE Entry IN (13321);


-- Creature Fixes
UPDATE creature SET equipment_id = 1 WHERE id = 6034;

-- Creature Addons
DELETE FROM `creature_addon` WHERE `guid` IN (311636,311641);
-- Fishing State
INSERT INTO `creature_addon` (`guid`, `emote`) VALUES (311636, 379);
INSERT INTO `creature_addon` (`guid`, `emote`) VALUES (311641, 379);


