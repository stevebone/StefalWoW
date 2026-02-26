--
-- Pet Battle Achievements: "Newbie" (reach level 3 with a battle pet)
-- Achievement 7433 exists in DB2 data but may be missing criteria entries.
-- This ensures the full chain is present: achievement -> criteria_tree -> criteria
--
-- Using REPLACE to safely upsert (won't duplicate if already loaded from DB2)
--

-- Achievement: Newbie (ID=7433)
-- Category 15117 = Battle > General
-- CriteriaTree 19024 = root criteria tree node for this achievement
REPLACE INTO `achievement` (`Description`, `Title`, `Reward`, `ID`, `InstanceID`, `Faction`, `Supercedes`, `Category`, `MinimumCriteria`, `Points`, `Flags`, `UiOrder`, `IconFileID`, `RewardItemID`, `CriteriaTree`, `SharesCriteria`, `CovenantID`, `HiddenBeforeDisplaySeason`, `LegacyAfterTimeEvent`, `VerifiedBuild`) VALUES
('Reach level 3 with a battle pet.', 'Newbie', '', 7433, -1, -1, 0, 15117, 0, 10, 0, 0, 655866, 0, 19024, 0, 0, 0, 0, 65727);

-- CriteriaTree: root node for Newbie (ID=19024)
-- Operator=0 (Complete), Amount=1, CriteriaID=21704
REPLACE INTO `criteria_tree` (`ID`, `Description`, `Parent`, `Amount`, `Operator`, `CriteriaID`, `OrderIndex`, `Flags`, `VerifiedBuild`) VALUES
(19024, 'Reach level 3 with a battle pet.', 0, 1, 0, 21704, 0, 0, 65727);

-- Criteria: BattlePetReachLevel (Type=160), Asset=3 (level 3)
REPLACE INTO `criteria` (`ID`, `Type`, `Asset`, `ModifierTreeId`, `StartEvent`, `StartAsset`, `StartTimer`, `FailEvent`, `FailAsset`, `Flags`, `EligibilityWorldStateID`, `EligibilityWorldStateValue`, `VerifiedBuild`) VALUES
(21704, 160, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 65727);

-- Register hotfix entries so the server sends these to the client
-- TableHash for Achievement = 3538824359 (from Achievement.db2 header)
-- TableHash for CriteriaTree = 1255424668 (from CriteriaTree.db2 header)
-- TableHash for Criteria = 4012104832 (from Criteria.db2 header)
-- Status: 3 = inserted record
REPLACE INTO `hotfix_data` (`Id`, `UniqueId`, `TableHash`, `RecordId`, `Status`, `VerifiedBuild`) VALUES
(900001, 900001, 3538824359, 7433, 3, 65727),
(900002, 900002, 1255424668, 19024, 3, 65727),
(900003, 900003, 4012104832, 21704, 3, 65727);
