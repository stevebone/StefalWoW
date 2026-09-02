-- Nature's Grace (450347) — replace native script with custom version
-- Midnight DB2 no longer has EFFECT_2 and the spell no longer stacks.
-- Native spell_dru_natures_grace in spell_druid.cpp becomes dead code.
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_dru_natures_grace';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(450347, 'spell_dru_natures_grace_custom');

-- Nature's Grace Eclipse (48517 Solar, 48518 Lunar) — replace native script with custom version
-- Native spell_dru_natures_grace_eclipse referenced EFFECT_2 of Nature's Grace which no longer exists.
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_dru_natures_grace_eclipse';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(48517, 'spell_dru_natures_grace_eclipse_custom'),
(48518, 'spell_dru_natures_grace_eclipse_custom');

-- Eclipse mechanic (79577 Dummy, 48517 Solar Aura, 48518 Lunar Aura, 329910 OOC)
-- Midnight DB2 no longer has SPELL_DRUID_ECLIPSE_DUMMY (79577), SPELL_DRUID_ECLIPSE_LUNAR_SPELL_CNT (326055),
-- or SPELL_DRUID_ECLIPSE_SOLAR_SPELL_CNT (326053). The Eclipse system is now purely DB2-driven.
-- Remove all script links — native spell_dru_eclipse_aura, spell_dru_eclipse_dummy, spell_dru_eclipse_ooc become dead code.
DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dru_eclipse_aura', 'spell_dru_eclipse_dummy', 'spell_dru_eclipse_ooc');
