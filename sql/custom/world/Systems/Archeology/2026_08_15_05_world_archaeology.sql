--
-- Archaeology: provisional branch-keystone loot on the Cataclysm six-branch find objects.
-- Keystone identity comes from ResearchBranch.db2 ItemID; quantity is one. Fossil (branch 3) has
-- ItemID 0, so it has no keystone row.
--
-- PROVISIONAL-FROM-FORK (evry/master-track/archaeology 24a970f7c7): the 7% drop chance is a
-- reference-backed guess, NOT a retail-configured percentage.
--
-- One-line fuse once an authoritative rate exists:
--   UPDATE `gameobject_loot_template`
--   SET `Chance` = <retail>
--   WHERE `Comment` LIKE '%provisional keystone%';
--
-- Ported from evry/master-track/archaeology 24a970f7c7.
--
DELETE FROM `gameobject_loot_template`
WHERE `Entry` IN (28434, 35546, 35827, 36055, 36056)
  AND `ItemType` = 0
  AND `Item` IN (52843, 63127, 64396, 64397, 63128);

INSERT INTO `gameobject_loot_template`
(`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(28434, 0, 52843, 7, 0, 1, 0, 1, 1, 'Dwarf Rune Stone (provisional keystone chance)'),
(35827, 0, 63127, 7, 0, 1, 0, 1, 1, 'Highborne Scroll (provisional keystone chance)'),
(36055, 0, 64396, 7, 0, 1, 0, 1, 1, 'Nerubian Obelisk (provisional keystone chance)'),
(36056, 0, 64397, 7, 0, 1, 0, 1, 1, 'Tol''vir Hieroglyphic (provisional keystone chance)'),
(35546, 0, 63128, 7, 0, 1, 0, 1, 1, 'Troll Tablet (provisional keystone chance)');
