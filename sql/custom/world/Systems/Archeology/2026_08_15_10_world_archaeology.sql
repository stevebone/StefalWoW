--
-- Archaeology: enable the Mantid / Pandaren / Mogu branches (find objects + fragment loot).
-- Find GameObjects verified as chests with lock 1859 in gameobject_template:
--   211163 Pandaren Archaeology Find (data1=41486)
--   211174 Mogu Archaeology Find (data1=41505)
--   218950 Mantid Archaeology Find (data1=46918)
-- Fragment currencies from ResearchBranch.db2 CurrencyID (754/676/677).
--
-- PROVISIONAL-FROM-FORK (evry/master-track/archaeology b59c8db8ff): 5-9 fragments per find.
-- No provisional keystone rows are added for these three branches.
--
-- Ported from evry/master-track/archaeology b59c8db8ff.
--
DELETE FROM `archaeology_research_branch` WHERE `researchBranchId` IN (29,229,231);
INSERT INTO `archaeology_research_branch` (`researchBranchId`,`findGameObjectId`) VALUES
(29,218950),  -- Mantid
(229,211163), -- Pandaren
(231,211174); -- Mogu

UPDATE `gameobject_template`
SET `ScriptName` = 'go_archaeology_find',
    `data1` = CASE `entry`
      WHEN 211163 THEN 41486 -- Pandaren
      WHEN 211174 THEN 41505 -- Mogu
      WHEN 218950 THEN 46918 -- Mantid
    END
WHERE `entry` IN (211163,211174,218950);

DELETE FROM `gameobject_loot_template`
WHERE `Entry` IN (41486,41505,46918)
  AND `ItemType` = 2
  AND `Item` IN (676,677,754);
INSERT INTO `gameobject_loot_template`
(`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(41486,2,676,100,0,1,0,5,9,'Pandaren Archaeology Fragments'),
(41505,2,677,100,0,1,0,5,9,'Mogu Archaeology Fragments'),
(46918,2,754,100,0,1,0,5,9,'Mantid Archaeology Fragments');
