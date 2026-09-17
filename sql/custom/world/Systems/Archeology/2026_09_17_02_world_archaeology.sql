--
-- Archaeology (world): find-object wiring for the Draenor / Broken Isles /
-- Kul Tiras / Zandalar branches. Companion to
-- arch_world_dig_sites_draenor_legion_bfa.sql: that file loads the dig sites
-- and their polygons, this one enables the branches (archaeology_research_
-- branch), binds the retail find GameObjects to the loot/find script and
-- fills their loot tables - together the new sites become surveyable.
--
--   * branches 315 Arakkoa / 350 Draenor Clans / 382 Ogre / 404 Highborne /
--     406 Highmountain Tauren / 408 Demonic / 423 Zandalari / 424 Drust.
--   * the retail find objects already carry the right data0 (lock 1859) and
--     data1 (loot id), so only the ScriptName binding is added. One template
--     fix: the Highborne find (268453) is type 50 in current retail data and
--     is aligned to chest (3) so the loader's chest validation accepts it.
--   * loot follows the consolidated file's provisional policy: fragments
--     (ItemType 2, ResearchBranch.CurrencyID, 100%, 5-9 per find) +
--     keystones (ItemType 0, ResearchBranch.ItemID, 7%, quantity 1).
--   * the second retail Highmountain Tauren find (268450, loot 71851) is
--     left unwired; 246804 already covers branch 406.
--   * project solve spells need no update: section 6 of the consolidated
--     file already binds every new-branch project spell.
--
-- Safe to re-run: every section clears only the rows it re-inserts.
--

UPDATE `gameobject_template`
SET `ScriptName` = 'go_archaeology_find'
WHERE `entry` IN (226521,234105,234106,246804,246812,278476,278477);

UPDATE `gameobject_template`
SET `type` = 3, `ScriptName` = 'go_archaeology_find'
WHERE `entry` = 268453;

DELETE FROM `archaeology_research_branch` WHERE `researchBranchId` IN (315,350,382,404,406,408,423,424);
INSERT INTO `archaeology_research_branch` (`researchBranchId`,`findGameObjectId`) VALUES
(315,234105), -- Arakkoa
(350,226521), -- Draenor Clans
(382,234106), -- Ogre
(404,268453), -- Highborne (template type aligned 50 -> 3 above)
(406,246804), -- Highmountain Tauren
(408,246812), -- Demonic
(423,278476), -- Zandalari
(424,278477); -- Drust

DELETE FROM `gameobject_loot_template` WHERE `Entry` IN (51118,55087,55089,64394,64397,71840,77641,77643);
INSERT INTO `gameobject_loot_template` (`Entry`,`ItemType`,`Item`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`) VALUES
(51118,2,821,100,0,1,0,5,9,'Draenor Clans Archaeology Fragments'),
(55087,2,829,100,0,1,0,5,9,'Arakkoa Archaeology Fragments'),
(55089,2,828,100,0,1,0,5,9,'Ogre Archaeology Fragments'),
(71840,2,1172,100,0,1,0,5,9,'Highborne Archaeology Fragments'),
(64394,2,1173,100,0,1,0,5,9,'Highmountain Tauren Archaeology Fragments'),
(64397,2,1174,100,0,1,0,5,9,'Demonic Archaeology Fragments'),
(77641,2,1534,100,0,1,0,5,9,'Zandalari Archaeology Fragments'),
(77643,2,1535,100,0,1,0,5,9,'Drust Archaeology Fragments'),
(51118,0,108439,7,0,1,0,1,1,'Draenor Clans keystone item (provisional keystone chance)'),
(55087,0,109585,7,0,1,0,1,1,'Arakkoa keystone item (provisional keystone chance)'),
(55089,0,109584,7,0,1,0,1,1,'Ogre keystone item (provisional keystone chance)'),
(71840,0,130903,7,0,1,0,1,1,'Highborne keystone item (provisional keystone chance)'),
(64394,0,130904,7,0,1,0,1,1,'Highmountain Tauren keystone item (provisional keystone chance)'),
(64397,0,130905,7,0,1,0,1,1,'Demonic keystone item (provisional keystone chance)'),
(77641,0,154989,7,0,1,0,1,1,'Zandalari keystone item (provisional keystone chance)'),
(77643,0,154990,7,0,1,0,1,1,'Drust keystone item (provisional keystone chance)');
