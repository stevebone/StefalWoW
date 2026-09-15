-- Hunter order-hall: stop the Eagle Sentinel from ejecting hunters from their own hall.
--
-- spell_area 208643 ("Eagle Sentinel", Trueshot Lodge area 7877) auto-applies to everyone who enters and ejects them
-- via 208649 (teleport to Nesingwary's Retreat, map 1220 @ 4496,4850,662). Retail only ejects NON-hunters; TC never
-- implemented that class check, so it teleported hunters straight back out every time they approached the hall.
-- spell_hunter_eagle_sentinel_eject skips the ejection teleport for HUNTER players.
DELETE FROM `spell_script_names` WHERE `spell_id` = 208649 AND `ScriptName` = 'spell_hunter_eagle_sentinel_eject';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(208649, 'spell_hunter_eagle_sentinel_eject');
