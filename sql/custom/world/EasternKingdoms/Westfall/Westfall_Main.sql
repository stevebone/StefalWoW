-- Westfall Main changes

-- NPC: 95 Defias Smuggler
-- NPC: 98 Riverpaw Taskmaster
-- NPC: 114 Harvest Watchers
-- NPC: 115 Harvest Reaper
-- NPC: 117 Riverpaw Gnoll
-- NPC: 121 Defias Pathstalker
-- NPC: 122 Defias Highwayman
-- NPC: 123 Riverpaw Mongrel
-- NPC: 124 Riverpaw Brute
-- NPC: 126 Murloc Coastrunner
-- NPC: 127 Murloc Tidehunter
-- NPC: 154 Greater Fleshripper
-- NPC: 157 Goretusk
-- NPC: 171 Murloc Warrior
-- NPC: 199 Young Fleshripper
-- NPC: 233 Farmer Saldean
-- NPC: 235 Salma Saldean
-- NPC: 237 Farmer Furlbrow
-- NPC: 238 Verna Furlbrow
-- NPC: 391 Murloc Old Murk
-- NPC: 392 Captain Grayson
-- NPC: 449 Defias Knuckleduster
-- NPC: 452 Riverpaw Bandit
-- NPC: 453 Riverpaw Mystic
-- NPC: 454 Young Goretusk
-- NPC: 456 Murloc Minor Oracle
-- NPC: 458 Murloc Hunter
-- NPC: 462 Vultros
-- NPC: 467 Defias Traitor
-- NPC: 480 Rusty Harvest Golem
-- NPC: 481 Defias Footpad
-- NPC: 500 Riverpaw Scout
-- NPC: 501 Riverpaw Herbalist
-- NPC: 504 Defias Trapper
-- NPC: 513 Murloc Netter
-- NPC: 515 Murloc Raider
-- NPC: 517 Murloc Oracle
-- NPC: 519 Murloc Slark
-- NPC: 520 Murloc Brack
-- NPC: 550 Defias Messenger
-- NPC: 582 Old Blanchy
-- NPC: 589 Defias Pillager
-- NPC: 590 Defias Looter
-- NPC: 594 Defias Hencheman
-- NPC: 830 Sand Crawler
-- NPC: 831 Sea Crawler
-- NPC: 832 Unbound Cyclone
-- NPC: 834 Coyote
-- NPC: 1065 Riverpaw Shaman
-- NPC: 1109 Fleshripper
-- NPC: 1236 Kobold Digger
-- NPC: 1424 Master Digger
-- NPC: 6250 Crawler
-- NPC: 7024 Agent Kearnen
-- NPC: 17234 Trigger for Mortwake tower
-- NPC: 42308 Lieutenant Horatio
-- NPC: 42342 Energized Harvest Reaper
-- NPC: 42357 Hulking Goretusk
-- NPC: 42383 Transient
-- NPC: 42384 Homeless Stormwind Citizen
-- NPC: 42386 Homeless Stormwind Citizen (Female)
-- NPC: 42387 Thug
-- NPC: 42390 Small-Time Hustler
-- NPC: 42391 West Plains Drifter
-- NPC: 42405 Two-Shoed Lou
-- NPC: 42562 Thug Trigger
-- NPC: 42653 Riverpaw Jango
-- NPC: 42656 Mercenary
-- NPC: 42662 Shadowy Figure
-- NPC: 42669 Chasm Slime
-- NPC: 42677 Moonbrook Thug
-- NPC: 900000 copy of Farmer Furlbrow for phasing
-- NPC: 900001 copy of Verna Furlbrow
-- NPC: 900002 copy of Old Blanchy

-- Quest: 9 The killing fields
-- Quest: 22 Goretusk Liver Pie
-- Quest: 36 Westfall Stew
-- Quest: 64 The Forgotten Heirloom
-- Quest: 103 Keeper of the flame
-- Quest: 109 Report to Gryan Stoutmantle
-- Quest: 151 Poor Old Blanchy
-- Quest: 184 Furlbrows Deed
-- Quest: 26209 Murder Was The Case That They Gave Me
-- Quest: 26213 Hot On the Trail: The Riverpaw Clan
-- Quest: 26214 Hot On the Trail: Murlocs
-- Quest: 26215 Meet Two-Shoed Lou
-- Quest: 26232 Lou's Parting Thoughts
-- Quest: 26290 Secrets of the tower

-- Spell: 76633 Apply Quest Invis Zone 1 (Lou in Westfall)
-- Spell: 79229 Detect Quest Invis 1 (same effect as the above)
-- Spell: 79489 Detect Quest Invis 0 (for Laine and murder npcs)
-- Spell: 79341 Detect Quest Invis 2 (for Lou murder)

-- AT: 5989 Furlbrow's Pumpkin Farm (Lou)

-- Phases
DELETE FROM `phase_area` WHERE `PhaseId` IN (50004, 50005, 50006);
INSERT INTO `phase_area` VALUES
(40, 50004, 'Westfall - Furlbrows 1 - Defias Brotherhood'),
(40, 50005, 'Westfall - Furlbrows 2 - Return of the Defias (Vanessa)');
-- (40, 50006, 'xxx to be reused');

-- Phase Conditions
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26 AND `SourceGroup` IN (50004, 50005, 50006);
INSERT INTO `conditions` VALUES
-- Phase 50004 allows the OLD Furlbrows npcs to be again visible for the old quests
-- This includes ALL the OLD Defias mobs around westfall.
-- They will be gone after the quest The Defias Brotherhood is completed
-- At that point the new quest line can be started
('26', '50004', '0', '0', '0', '47', '0', '166', '64', '0', '', '1', '0', '0', '', 'Westfall - Add phase 50004 if 166 IS NOT rewarded'),
-- Phase 50005 is the phase for the new/post Cata storyline
-- It starts after the quest The Defias Brotherhood
('26', '50005', '40', '0', '0', '47', '0', '166', '64', '0', '', '0', '0', '0', '', 'Westfall - Add phase 50005 if 109 IS rewarded');

-- AT scripts
DELETE FROM `areatrigger_scripts` WHERE `entry` IN (5989);
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES 
(5989, 'at_westfall_furlsbrow_farm_5989');