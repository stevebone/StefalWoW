--
-- BfA Warfronts: CreatureXContribution.db2 rows that authorize the war-table recruiters to collect war-effort
-- donations for their own faction's bars.
--
-- Shipped client data (build 68275) only points the four warfront contributions at the invisible collector dummies:
--      CreatureXContribution 115: Contribution  11 <- creature 143707 (Warfront Horde Contribution Dummy)
--      CreatureXContribution 116: Contribution 116 <- creature 143709 (Warfront Alliance Contribution Dummy)
--      CreatureXContribution 117: Contribution 117 <- creature 143709
--      CreatureXContribution 118: Contribution 118 <- creature 143707
-- Those dummies use CreatureDisplayID 13069 (invisible) and UNIT_FLAG_UNINTERACTIBLE, so no player can ever
-- interact with them. The rows below mirror exactly the same pairs onto the VISIBLE war-table recruiters:
--      Ralston Karn 142721 (Alliance) -> Contribution 116 (Stromgarde, ManagedWorldState 113)
--                                     -> Contribution 117 (Darkshore,  ManagedWorldState 114)
--      Throk        138949 (Horde)    -> Contribution  11 (Stromgarde, ManagedWorldState 12)
--                                     -> Contribution 118 (Darkshore,  ManagedWorldState 115)
--
-- ContributionMgr::IsCollectorFor() checks exactly this table (plus the collector npcflag applied in the world
-- update), so these rows are what make the recruiter's "Donate supplies to the war effort" option legal.
--
-- VerifiedBuild = 0 marks the rows as custom hotfix content. DB2StorageBase::LoadFromDB loads both verified and
-- custom rows, so the SERVER sees them immediately. Advertising them to the CLIENT (which is what makes the native
-- ContributionCollectionFrame list these bars at the recruiter) additionally needs a matching `hotfix_data` row
-- carrying CreatureXContribution.db2's TableHash; that hash is only readable from the shipped .db2 header and was
-- not available offline, so the client-side push is deliberately NOT authored here. The server-side donate path in
-- the recruiter gossip does not depend on it.
--
DELETE FROM `creature_x_contribution` WHERE `ID` IN (5001, 5002, 5003, 5004);
INSERT INTO `creature_x_contribution` (`ID`, `ContributionID`, `CreatureID`, `VerifiedBuild`) VALUES
(5001, 116, 142721, 0),
(5002, 117, 142721, 0),
(5003,  11, 138949, 0),
(5004, 118, 138949, 0);
