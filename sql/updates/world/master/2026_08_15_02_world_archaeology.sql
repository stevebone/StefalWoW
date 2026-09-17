--
-- Archaeology: branch find GameObjects and fragment-currency loot (Cataclysm six-branch set).
-- The find GameObjects and their loot-template identities are retail client/world data; the
-- fragment currencies come from ResearchBranch.db2 CurrencyID.
--
-- PROVISIONAL-FROM-FORK (evry/master-track/archaeology cb60da9643): the 5-9 fragments per find
-- yield is a reference-backed policy, not a 12.0.7 retail sample.
--
-- Ported from evry/master-track/archaeology cb60da9643.
--
CREATE TABLE IF NOT EXISTS `archaeology_research_branch` (
  `researchBranchId` smallint unsigned NOT NULL COMMENT 'ResearchBranch.db2 ID',
  `findGameObjectId` int unsigned NOT NULL COMMENT 'Branch-specific archaeology find GameObject',
  PRIMARY KEY (`researchBranchId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Archaeology server-owned branch policy';

DELETE FROM `archaeology_research_branch` WHERE `researchBranchId` IN (1,3,4,5,7,8);
INSERT INTO `archaeology_research_branch` (`researchBranchId`,`findGameObjectId`) VALUES
(1,204282), -- Dwarf
(3,206836), -- Fossil
(4,203071), -- Night Elf
(5,203078), -- Nerubian
(7,207190), -- Tol'vir
(8,202655); -- Troll

UPDATE `gameobject_template`
SET `ScriptName` = 'go_archaeology_find',
    `data1` = CASE `entry`
      WHEN 202655 THEN 35546 -- Troll
      WHEN 203071 THEN 35827 -- Night Elf
      WHEN 203078 THEN 36055 -- Nerubian
      WHEN 204282 THEN 28434 -- Dwarf
      WHEN 206836 THEN 35733 -- Fossil
      WHEN 207190 THEN 36056 -- Tol'vir
    END
WHERE `entry` IN (202655,203071,203078,204282,206836,207190);

DELETE FROM `gameobject_loot_template`
WHERE `Entry` IN (28434,35546,35733,35827,36055,36056);
INSERT INTO `gameobject_loot_template`
(`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(28434,2,384,100,0,1,0,5,9,'Dwarf Archaeology Fragments'),
(35546,2,385,100,0,1,0,5,9,'Troll Archaeology Fragments'),
(35733,2,393,100,0,1,0,5,9,'Fossil Archaeology Fragments'),
(35827,2,394,100,0,1,0,5,9,'Night Elf Archaeology Fragments'),
(36055,2,400,100,0,1,0,5,9,'Nerubian Archaeology Fragments'),
(36056,2,401,100,0,1,0,5,9,'Tol''vir Archaeology Fragments');
