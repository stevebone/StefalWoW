-- DK Blood artifact "Maw of the Damned" (quest 40740 "The Dead and the Damned")
-- The quest shipped with no objectives and no encounter. Add a single kill-Gorelix objective and bind the
-- acquisition scripts (quest flight/summon + Gorelix boss AI with the verified reveal/loot scenes 1561/1532).

-- Bind the QuestScript that flies the player to the Broken Shore and summons Gorelix on accept.
UPDATE `quest_template_addon` SET `ScriptName` = 'quest_dk_the_dead_and_the_damned' WHERE `ID` = 40740;

-- Bind the Gorelix boss AI (creature already faction 16 / hostile).
UPDATE `creature_template` SET `ScriptName` = 'npc_dk_gorelix' WHERE `entry` = 101778;

-- Single kill objective: slay Gorelix the Fleshripper (so the quest enters INCOMPLETE on accept and drives the encounter).
DELETE FROM `quest_objectives` WHERE `QuestID` = 40740;
INSERT INTO `quest_objectives`
    (`ID`, `QuestID`, `Type`, `Order`, `StorageIndex`, `ObjectID`, `Amount`, `ConditionalAmount`, `Flags`, `Flags2`, `ProgressBarWeight`, `ParentObjectiveID`, `Visible`, `Description`, `VerifiedBuild`) VALUES
    (40740001, 40740, 0, 0, 0, 101778, 1, 0, 0, 0, 0, 0, 1, 'Slay Gorelix the Fleshripper', 0);
