--
-- Archaeology: provisional branch-keystone loot for Draenei / Orc / Vrykul.
-- Keystone identity from ResearchBranch.db2 ItemID (64394/64392/64395); quantity one.
--
-- PROVISIONAL-FROM-FORK (evry/master-track/archaeology eb4525d6bf): 7% matches the Cataclysm
-- provisional fuse and is NOT a retail-configured percentage. The fuse documented in
-- 2026_08_15_05_world.sql covers these rows too (same Comment suffix).
--
-- Ported from evry/master-track/archaeology eb4525d6bf.
--
DELETE FROM `gameobject_loot_template`
WHERE `Entry` IN (36019, 36037, 36054)
  AND `ItemType` = 0
  AND `Item` IN (64392, 64394, 64395);

INSERT INTO `gameobject_loot_template`
(`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(36019, 0, 64392, 7, 0, 1, 0, 1, 1, 'Orc Blood Text (provisional keystone chance)'),
(36037, 0, 64394, 7, 0, 1, 0, 1, 1, 'Draenei Tome (provisional keystone chance)'),
(36054, 0, 64395, 7, 0, 1, 0, 1, 1, 'Vrykul Rune Stick (provisional keystone chance)');
