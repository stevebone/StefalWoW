DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_kill_or_be_killed', 'spell_warr_kill_or_be_killed_target', 'spell_warr_kill_or_be_killed_warrior');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(1265361, 'spell_warr_kill_or_be_killed'),
(1265641, 'spell_warr_kill_or_be_killed_target'),
(1265600, 'spell_warr_kill_or_be_killed_warrior');

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_rampaging_ruin');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(184367, 'spell_warr_rampaging_ruin');

DELETE FROM `spell_proc` WHERE `SpellId` IN (386633,386634);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(386633,0x00,4,0x02000000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x4,0x0,0x0,0x0,0,0,0,1), -- Executioner's Precision
(386634,0x00,4,0x00000000,0x00000000,0x00040000,0x00000000,0x0,0x0,0x0,0x2,0x0,0x0,0x0,0,0,0,0); -- Executioner's Precision

DELETE FROM `spell_proc` WHERE `SpellId` IN (85739);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(85739,0x00,4,0x00000000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x1,0x0,0x18,0x0,0,0,0,0); -- Whirlwind

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_improved_whirlwind_cleave');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(1464, 'spell_warr_improved_whirlwind_cleave'),  -- Slam
(1715, 'spell_warr_improved_whirlwind_cleave'),  -- Hamstring
(23881, 'spell_warr_improved_whirlwind_cleave'),  -- Bloodthirst
(23922, 'spell_warr_improved_whirlwind_cleave'),  -- Shield Slam
(34428, 'spell_warr_improved_whirlwind_cleave'),  -- Victory Rush
(85384, 'spell_warr_improved_whirlwind_cleave'),  -- Raging Blow
(96103, 'spell_warr_improved_whirlwind_cleave'),  -- Raging Blow
(100130, 'spell_warr_improved_whirlwind_cleave'),  -- Furious Slash
(163558, 'spell_warr_improved_whirlwind_cleave'),  -- Execute Off-Hand
(184707, 'spell_warr_improved_whirlwind_cleave'),  -- Rampage
(184709, 'spell_warr_improved_whirlwind_cleave'),  -- Rampage
(201363, 'spell_warr_improved_whirlwind_cleave'),  -- Rampage
(201364, 'spell_warr_improved_whirlwind_cleave'),  -- Rampage
(202168, 'spell_warr_improved_whirlwind_cleave'),  -- Impending Victory
(218617, 'spell_warr_improved_whirlwind_cleave'),  -- Rampage
(260798, 'spell_warr_improved_whirlwind_cleave'),  -- Execute
(280772, 'spell_warr_improved_whirlwind_cleave'),  -- Siegebreaker
(280849, 'spell_warr_improved_whirlwind_cleave'),  -- Execute
(317483, 'spell_warr_improved_whirlwind_cleave'),  -- Condemn (Venthyr)
(317488, 'spell_warr_improved_whirlwind_cleave'),  -- Condemn (Venthyr)
(317489, 'spell_warr_improved_whirlwind_cleave'),  -- Condemn Off-Hand (Venthyr)
(335096, 'spell_warr_improved_whirlwind_cleave'),  -- Bloodbath
(335098, 'spell_warr_improved_whirlwind_cleave'),  -- Crushing Blow
(335100, 'spell_warr_improved_whirlwind_cleave'),  -- Crushing Blow
(394062, 'spell_warr_improved_whirlwind_cleave'),  -- Rend
(394063, 'spell_warr_improved_whirlwind_cleave'),  -- Rend
(396718, 'spell_warr_improved_whirlwind_cleave'),  -- Onslaught
(463815, 'spell_warr_improved_whirlwind_cleave'),  -- Arms Execute FX Test
(463816, 'spell_warr_improved_whirlwind_cleave'),  -- Fury Execute FX Test
(463817, 'spell_warr_improved_whirlwind_cleave'),  -- Fury Execute Off-Hand FX Test
(1269383, 'spell_warr_improved_whirlwind_cleave');  -- Heroic Strike

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_pri_searing_light');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(1280131, 'spell_pri_searing_light');

DELETE FROM `spell_proc` WHERE `SpellId` IN (1280131);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(1280131,0x00,6,0x00000000,0x00008000,0x00000000,0x00000000,0x0,0x0,0x1,0x2,0x0,0x0,0x0,0,0,0,0); -- Searing Light

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ("spell_pri_archangel");
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(472433, 'spell_pri_archangel');

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dru_galactic_guardian_moonfire','spell_dru_lunar_wrath');
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(164812,'spell_dru_galactic_guardian_moonfire'),
(1253600,'spell_dru_lunar_wrath');

DELETE FROM `spell_proc` WHERE `SpellId` IN (203964,1253600);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(203964,0x00,7,0x00000000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x1,0x2,0x0,0x0,0x0,0,0,0,0), -- Galactic Guardian
(1253600,0x00,7,0x00000000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x2,0x0,0x10,0x0,0,0,0,0); -- Lunar Wrath

DELETE FROM `spell_proc` WHERE `SpellId` IN (1253724);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(1253724,0x00,6,0x00000080,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x2,0x0,0x0,0x0,0,0,0,0); -- Greater Smite

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_brutal_finish');
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(227847,'spell_warr_brutal_finish'), -- Brutal Finish (Bladestorm)
(389774,'spell_warr_brutal_finish'), -- Brutal Finish (Bladestorm, Hurricane)
(446035,'spell_warr_brutal_finish'); -- Brutal Finish (Bladestorm, Unrelenting Onslaught)

DELETE FROM `spell_proc` WHERE `SpellId` IN (446918);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(446918,0x00,4,0x02000000,0x00000000,0x00000000,0x00800000,0x0,0x0,0x0,0x2,0x0,0x0,0x0,0,0,0,1); -- Brutal Finish

DELETE FROM `spell_proc` WHERE `SpellId` IN (85739);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(85739,0x00,4,0x00300002,0x00202700,0x00000120,0x00900000,0x0,0x0,0x0,0x4,0x0,0x10,0x0,0,0,0,0); -- Whirlwind

DELETE FROM `spell_proc` WHERE `SpellId` IN (435607,435615);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(435607,0x01,4,0x00000000,0x00000600,0x00000000,0x00000000,0x0,0x0,0x1,0x4,0x403,0x0,0x0,0,0,0,0), -- Thunder Blast
(435615,0x00,4,0x00000080,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x4,0x0,0x10,0x0,0,0,0,0); -- Thunder Blast

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_thunder_blast', 'spell_warr_thunder_clap_rend', 'spell_warr_thunder_clap');
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(6343, 'spell_warr_thunder_clap'),
(6343, 'spell_warr_thunder_clap_rend'),
(435222, 'spell_warr_thunder_clap'),
(435222, 'spell_warr_thunder_clap_rend'),
(435607,'spell_warr_thunder_blast');

DELETE FROM `spell_proc` WHERE `SpellId` IN (438590);

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_keep_your_feet_on_the_ground');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(438590, 'spell_warr_keep_your_feet_on_the_ground');