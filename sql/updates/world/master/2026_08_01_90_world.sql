-- Fire Mage artifact "Felo'melorn" (quest 11997 "The Frozen Flame", Icecrown map 1480).
-- The quest exists in the DB with 2 objectives (99418 "Aethas's Portal" on accept + 100290 "Obtain Felo'melorn" on
-- Lyandra's death) but had NO quest-giver and no boss AI, so it was unobtainable. Bind the scripts and give it Meryl
-- Felstorm as giver+ender (mirroring Frost's "The Mage Hunter" 42479); Meryl (102700 Dalaran / 109222 Hall of the
-- Guardian) and Lyandra Sunstrider (99615, already hostile on map 1480) are already spawned - no new spawns needed.

-- Bind the QuestScript (portal to Icecrown on accept) and the Lyandra boss AI.
UPDATE `quest_template_addon` SET `ScriptName` = 'quest_the_frozen_flame' WHERE `ID` = 11997;
UPDATE `creature_template` SET `ScriptName` = 'npc_lyandra_sunstrider' WHERE `entry` = 99615;

-- Quest giver + ender = Meryl Felstorm (both spawns), exactly as Frost 42479 is set up.
DELETE FROM `creature_queststarter` WHERE `quest` = 11997;
INSERT INTO `creature_queststarter` (`id`,`quest`) VALUES (102700,11997),(109222,11997);
DELETE FROM `creature_questender` WHERE `quest` = 11997;
INSERT INTO `creature_questender` (`id`,`quest`) VALUES (102700,11997),(109222,11997);
