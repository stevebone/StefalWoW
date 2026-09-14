-- Death Knight Spell Fixes
-- Source: StefalWoW

-- Blood Draw (374598): 2 min internal cooldown enforced via ICD aura 374609.
-- 374606 (drain) gates on the ICD and applies ICD + 454871 (buff) synchronously.
-- 454871 rejects the raw talent trigger so it shares the ICD with the drain.
DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dk_blood_draw_drain', 'spell_dk_blood_draw_buff');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(374606, 'spell_dk_blood_draw_drain'),
(454871, 'spell_dk_blood_draw_buff');

-- =========================================================================
-- Vestigial Shell (454851 talent, 454863 buff) attached to Anti-Magic Shell (48707)
-- Script gates on the talent aura and casts the buff on AfterCast.
-- =========================================================================
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_dk_vestigial_shell';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(48707, 'spell_dk_vestigial_shell');

-- =========================================================================
-- March of Darkness (391546) - DK family mask3=0x08000000, damage proc
-- =========================================================================
DELETE FROM `spell_proc` WHERE `SpellId` IN (391546);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `SpellFamilyMask3`, `ProcFlags`, `ProcFlags2`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(391546, 0, 15, 0, 0, 0, 0x08000000, 0, 0, 0x4, 0x2, 0, 0, 0, 0, 0, 0, 0);
