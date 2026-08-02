-- Hunter order-hall: wire the Oath of Service (40955) to the Visage of Ohn'ahra spell-click.
--
-- The Visage of Ohn'ahra (102694, npcflag SPELLCLICK) casts spell 203240 when clicked (npc_spellclick_spells). Its
-- client-side effects do not grant the "Take the oath" credit (102794) server-side, so we attach a SpellScript that
-- grants the credit + stages Ohn'ahra's descent over the lodge. cast_flags is set to 3 (clicker is BOTH caster and
-- target) so the SpellScript's GetCaster() resolves to the swearing hunter.
UPDATE `npc_spellclick_spells` SET `cast_flags` = 3 WHERE `npc_entry` = 102694 AND `spell_id` = 203240;

DELETE FROM `spell_script_names` WHERE `spell_id` = 203240 AND `ScriptName` = 'spell_hunter_oath_of_service';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(203240, 'spell_hunter_oath_of_service');
