DELETE FROM `hotfix_blob` WHERE `VerifiedBuild`>0 AND `TableHash` IN (0x00CB59F4, 0xD9F1B527);

DELETE FROM `campaign` WHERE `VerifiedBuild`>0 AND `ID` IN (284, 270);
INSERT INTO `campaign` (`ID`, `Title`, `Description`, `UiTextureKitID`, `RewardQuestID`, `Prerequisite`, `Stalled`, `Completed`, `OnlyStallIf`, `UiQuestDetailsThemeID`, `Flags`, `DisplayPriority`, `SortAsNormalQuest`, `UseMinimalHeader`, `VerifiedBuild`) VALUES
(284, 'The War of Light and Shadow', '', 5768, 0, 135490, 34450, 143073, 0, 74, 0, 1202, 0, 0, 65893),
(270, 'Midnight', '', 5767, 0, 147611, 147611, 138637, 0, 73, 0, 1201, 0, 0, 65893);