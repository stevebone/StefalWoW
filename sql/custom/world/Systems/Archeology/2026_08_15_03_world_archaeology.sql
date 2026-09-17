--
-- Archaeology: bind the Survey SpellScript to spell 80451.
-- RegisterSpellScript(spell_archaeology_survey) registers the class; TC attaches it to a spell via
-- spell_script_names.
--
-- Ported from evry/master-track/archaeology e621450284.
--
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_archaeology_survey';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(80451, 'spell_archaeology_survey');
