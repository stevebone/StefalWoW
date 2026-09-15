-- Druid order-hall: stop the Dreamgrove guardian from ejecting druids from their own hall.
--
-- Identical bug to the hunter Eagle Sentinel (2026_08_02_70). spell_area 203810 ("Drowsy", Dreamgrove area 7846)
-- auto-applies to everyone and, via a 6s periodic trigger, casts 203822 - a teleport-with-loading-screen (Effect 252,
-- TARGET_DEST_DB) to map 1220 @ 3689,7096,25, just outside the grove. Retail only ejects NON-druids; TC has no class
-- check. spell_druid_dreamgrove_eject skips the ejection teleport for DRUID players. (Verified via SpellEffect.db2.)
DELETE FROM `spell_script_names` WHERE `spell_id` = 203822 AND `ScriptName` = 'spell_druid_dreamgrove_eject';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(203822, 'spell_druid_dreamgrove_eject');
