-- Custom Hotfixes
-- Id starts at 800001
-- Table Hashes need to be updated when client is updated

DELETE FROM `hotfix_data` WHERE `Id` BETWEEN 800001 AND 800016;
INSERT INTO `hotfix_data` (`Id`, `UniqueId`, `TableHash`, `RecordId`, `Status`, `VerifiedBuild`) VALUES
(800001, 800001, 4042326115, 50001, 1, -1), -- Phase Start zone 1
(800002, 800002, 4042326115, 50002, 1, -1), -- Phase Start zone 2
(800003, 800003, 4042326115, 50003, 1, -1), -- Phase Start zone 2
(800004, 800004, 4042326115, 50004, 1, -1), -- Phase Westfall 1
(800005, 800005, 4042326115, 50005, 1, -1), -- Phase Westfall 2
(800006, 800006, 4042326115, 50006, 1, -1), -- Phase Westfall 3
(800007, 800007, 4042326115, 50007, 1, -1), -- Phase Old Deadmines Dungeon
(800008, 800008, 441483745,  119,   1, -1), -- Area Trigger Deadmines Portal
(800009, 800009, 441483745,  6361,   1, -1), -- Area Trigger Deadmines Cannon door outside
(800010, 800010, 0xD2EE2CA7,  800001,   1, -1), -- Achievement 1 Deadmines
(800011, 800011, 0xEF23DC80,  800001,   1, -1), -- Achievement 1 Criteria 1 Deadmines
(800012, 800012, 0x4AD4429C,  800000,   1, -1), -- Achievement 1 Criteria 1 Criteria Tree 1 Deadmines
(800013, 800013, 0x4AD4429C,  800001,   1, -1), -- Achievement 1 Criteria 1 Criteria Tree 2 Deadmines
(800014, 800014, 0x50238EC2,  2874,   1, -1), -- Item: An unsent letter
(800015, 800015, 0x919BE54E,  2874,   1, -1), -- ItemSparse: An unsent letter
(800016, 800016, 0x919BE54E,  2874,   1, -1); -- ItemSparse: Sayges Fortune 24

DELETE FROM `phase` WHERE `Id` BETWEEN 50001 AND 50007;
INSERT INTO `phase` VALUES
(50001, 4, -1), -- Start zone 11,345,316,162
(50002, 4, -1), -- Start zone 2
(50003, 4, -1), -- Start zone 2
(50004, 20, -1), -- Westfall 1
(50005, 20, -1), -- Westfall 2
(50006, 20, -1), -- Westfall 3
(50007, 4, -1); -- Old Deadmines Dungeon

DELETE FROM `area_trigger` WHERE `ID` IN (119,6361);
INSERT INTO `area_trigger` (`PosX`, `PosY`, `PosZ`, `ID`, `ContinentID`, `PhaseUseFlags`, `PhaseID`, `PhaseGroupID`, `Radius`, `BoxLength`, `BoxWidth`, `BoxHeight`, `BoxYaw`, `ShapeType`, `ShapeID`, `AreaTriggerActionSetID`, `Flags`, `VerifiedBuild`) VALUES 
(-14.36279964447, -393.38000488281, 64.56050109863, 119, 36, 1, 0, 0, 6, 0, 0, 0, 0, 0, 0, 180, 0, 67823),
(-96.15100097656, -704.35400390625, 8.89498996735, 6361, 36, 1, 0, 0, 0, 35.56999969482, 43.72999954224, 12.57999992371, 0, 1, 0, 2162, 0, 67823);

DELETE FROM `achievement` WHERE `ID` IN (800001);
INSERT INTO `achievement` (`Description`, `Title`, `Reward`, `ID`, `InstanceID`, `Faction`, `Supercedes`, `Category`, `MinimumCriteria`, `Points`, `Flags`, `UiOrder`, `IconFileID`, `RewardItemID`, `CriteriaTree`, `SharesCriteria`, `CovenantID`, `HiddenBeforeDisplaySeason`, `LegacyAfterTimeEvent`, `VerifiedBuild`) VALUES
('Defeat Edwin VanCleef in the Classic version of Deadmines.', 'Classic Deadmines', '', 800001, -1, -1, 0, 15277, 1, 10, 0, 73, 236409, 0, 800000, 0, 0, 0, 0, 0);

DELETE FROM `criteria` WHERE `ID` IN (800001);
INSERT INTO `criteria` (`ID`, `Type`, `Asset`, `ModifierTreeId`, `StartEvent`, `StartAsset`, `StartTimer`, `FailEvent`, `FailAsset`, `Flags`, `EligibilityWorldStateID`, `EligibilityWorldStateValue`, `VerifiedBuild`) VALUES 
(800001, 0, 639, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0); -- Kill VanCleef

DELETE FROM `criteria_tree` WHERE `ID` IN (800000,800001);
INSERT INTO `criteria_tree` (`ID`, `Description`, `Parent`, `Amount`, `Operator`, `CriteriaID`, `OrderIndex`, `Flags`, `VerifiedBuild`) VALUES 
(800000, 'Defeat Edwin VanCleef', 0, 		1, 8, 0, 0, 0, 0),
(800001, 'Defeat Edwin VanCleef', 800000, 	1, 0, 800001, 1, 0, 0);

DELETE FROM `item` WHERE `ID` IN (2874);
INSERT INTO `item` (`ID`, `ClassID`, `SubclassID`, `Material`, `InventoryType`, `SheatheType`, `SoundOverrideSubclassID`, `IconFileDataID`, `ItemGroupSoundsID`, `ContentTuningID`, `ModifiedCraftingReagentItemID`, `Unknown1200`, `CraftingQualityID`, `ItemSquishEraID`, `RecraftReagentCountPercentage`, `OrderSource`) VALUES 
('2874', '12', '0', '0', '0', '0', '-1', '133471', '20', '0', '0', '0', '0', '0', '0', '0');

DELETE FROM `item_sparse` WHERE `ID` IN (2874,19424);
INSERT INTO `item_sparse` (`ID`, `Description`, `Display`, `ExpansionID`, `DmgVariance`, `LimitCategory`, `DurationInInventory`, `QualityModifier`, `BagFamily`, `StartQuestID`, `LanguageID`, `ItemRange`, `StatPercentageOfSocket1`, `StatPercentageOfSocket2`, `StatPercentageOfSocket3`, `StatPercentageOfSocket4`, `StatPercentageOfSocket5`, `StatPercentageOfSocket6`, `StatPercentageOfSocket7`, `StatPercentageOfSocket8`, `StatPercentageOfSocket9`, `StatPercentageOfSocket10`, `StatPercentEditor1`, `StatPercentEditor2`, `StatPercentEditor3`, `StatPercentEditor4`, `StatPercentEditor5`, `StatPercentEditor6`, `StatPercentEditor7`, `StatPercentEditor8`, `StatPercentEditor9`, `StatPercentEditor10`, `StatModifierBonusStat1`, `StatModifierBonusStat2`, `StatModifierBonusStat3`, `StatModifierBonusStat4`, `StatModifierBonusStat5`, `StatModifierBonusStat6`, `StatModifierBonusStat7`, `StatModifierBonusStat8`, `StatModifierBonusStat9`, `StatModifierBonusStat10`, `Stackable`, `MaxCount`, `MinReputation`, `RequiredAbility`, `AllowableRace1`, `AllowableRace2`, `SellPrice`, `BuyPrice`, `VendorStackCount`, `PriceVariance`, `PriceRandomValue`, `Flags1`, `Flags2`, `Flags3`, `Flags4`, `Flags5`, `FactionRelated`, `ModifiedCraftingReagentItemID`, `ContentTuningID`, `PlayerLevelToItemLevelCurveID`, `ItemLevelOffsetCurveID`, `ItemLevelOffsetItemLevel`, `ItemSquishEraID`, `ItemNameDescriptionID`, `RequiredTransmogHoliday`, `RequiredHoliday`, `GemProperties`, `SocketMatchEnchantmentId`, `TotemCategoryID`, `InstanceBound`, `ZoneBound1`, `ZoneBound2`, `ItemSet`, `LockID`, `PageID`, `ItemDelay`, `MinFactionID`, `RequiredSkillRank`, `RequiredSkill`, `ItemLevel`, `AllowableClass`, `ArtifactID`, `SpellWeight`, `SpellWeightCategory`, `SocketType1`, `SocketType2`, `SocketType3`, `SheatheType`, `Material`, `PageMaterialID`, `Bonding`, `DamageDamageType`, `ContainerSlots`, `RequiredPVPMedal`, `RequiredPVPRank`, `RequiredLevel`, `InventoryType`, `OverallQualityID`, `VerifiedBuild`) VALUES 
('2874', 'A letter found on Edwin VanCleef''s person.', 'An Unsent Letter', '0', '0', '0', '0', '0', '0', '373', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '-1', '-1', '-1', '-1', '-1', '-1', '-1', '-1', '-1', '-1', '1', '0', '0', '0', '-1', '-1', '0', '0', '1', '1', '1', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '1', '-1', '0', '0', '0', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '1', '0', 1, 67186),
(19424,'Your fortune awaits you inside the Deadmines.','Sayge''s Fortune #24',0,0,0,0,0,0,7938,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,-1,-1,0,0,1,1,1.01559996605,0,8192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,-1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,1,0,1,67186);
