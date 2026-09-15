-- BfA War Campaign "A Nation Divided" (47189): the Scouting Map (creature 139522) is already fully wired -
-- gossip menu 22725 (OptionNpc=31 AdventureMap opens the war map + Drustvar/Stormsong zone picks) and SmartAI
-- that gives the quest kill-credit on gossip-open + adds the zone war-campaign quests (47960/47961/47962). But the
-- wowc_world import dropped its npcflag, so it wasn't interactable ("scouting map not clickable"). Restore GOSSIP.
UPDATE `creature_template` SET `npcflag` = `npcflag` | 1 WHERE `entry` = 139522;
UPDATE `creature` SET `npcflag` = 1 WHERE `id` = 139522;
