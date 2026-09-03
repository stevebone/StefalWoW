-- Stefal patch for Custom_Warlock_Spell_Fixes.sql
-- Apply AFTER the base file. Safe to re-run (DELETE + INSERT).
-- Requires worldserver rebuild for C++ fixes (SpellMgr, SpellAuraEffects, scripts).

-- =============================================================================
-- HOTFIXES
-- =============================================================================

-- Sanctified Plates (402964)
DELETE FROM hotfixes.spell_effect WHERE `ID` = 1052151;
INSERT INTO hotfixes.spell_effect (`ID`, `EffectAura`, `DifficultyID`, `EffectIndex`, `Effect`, `EffectAmplitude`, `EffectAttributes`, `EffectAuraPeriod`, `EffectBonusCoefficient`, `EffectChainAmplitude`, `EffectChainTargets`, `EffectItemType`, `EffectMechanic`, `EffectPointsPerResource`, `EffectPosFacing`, `EffectRealPointsPerLevel`, `EffectTriggerSpell`, `BonusCoefficientFromAP`, `PvpMultiplier`, `Coefficient`, `Variance`, `ResourceCoefficient`, `GroupSizeBasePointsCoefficient`, `EffectBasePoints`, `ScalingClass`, `TargetNodeGraph`, `EffectMiscValue1`, `EffectMiscValue2`, `EffectRadiusIndex1`, `EffectRadiusIndex2`, `EffectSpellClassMask1`, `EffectSpellClassMask2`, `EffectSpellClassMask3`, `EffectSpellClassMask4`, `ImplicitTarget1`, `ImplicitTarget2`, `SpellID`, `VerifiedBuild`) VALUES
(1052151, 142, 0, 2, 6, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 10, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 402964, 63906);

-- Demon Skin (219272)
DELETE FROM hotfixes.spell_effect WHERE `ID` = 1104809;
INSERT INTO hotfixes.spell_effect (`ID`, `EffectAura`, `DifficultyID`, `EffectIndex`, `Effect`, `EffectAmplitude`, `EffectAttributes`, `EffectAuraPeriod`, `EffectBonusCoefficient`, `EffectChainAmplitude`, `EffectChainTargets`, `EffectItemType`, `EffectMechanic`, `EffectPointsPerResource`, `EffectPosFacing`, `EffectRealPointsPerLevel`, `EffectTriggerSpell`, `BonusCoefficientFromAP`, `PvpMultiplier`, `Coefficient`, `Variance`, `ResourceCoefficient`, `GroupSizeBasePointsCoefficient`, `EffectBasePoints`, `ScalingClass`, `TargetNodeGraph`, `EffectMiscValue1`, `EffectMiscValue2`, `EffectRadiusIndex1`, `EffectRadiusIndex2`, `EffectSpellClassMask1`, `EffectSpellClassMask2`, `EffectSpellClassMask3`, `EffectSpellClassMask4`, `ImplicitTarget1`, `ImplicitTarget2`, `SpellID`, `VerifiedBuild`) VALUES
(1104809, 142, 0, 3, 6, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1.29999995232, 0, 0, 0, 1, 45, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 219272, 63906);

-- Enduring Torment (453314)
DELETE FROM hotfixes.spell_effect WHERE `ID` IN (1155059, 1155060);
INSERT INTO hotfixes.spell_effect (`ID`, `EffectAura`, `DifficultyID`, `EffectIndex`, `Effect`, `EffectAmplitude`, `EffectAttributes`, `EffectAuraPeriod`, `EffectBonusCoefficient`, `EffectChainAmplitude`, `EffectChainTargets`, `EffectItemType`, `EffectMechanic`, `EffectPointsPerResource`, `EffectPosFacing`, `EffectRealPointsPerLevel`, `EffectTriggerSpell`, `BonusCoefficientFromAP`, `PvpMultiplier`, `Coefficient`, `Variance`, `ResourceCoefficient`, `GroupSizeBasePointsCoefficient`, `EffectBasePoints`, `ScalingClass`, `TargetNodeGraph`, `EffectMiscValue1`, `EffectMiscValue2`, `EffectRadiusIndex1`, `EffectRadiusIndex2`, `EffectSpellClassMask1`, `EffectSpellClassMask2`, `EffectSpellClassMask3`, `EffectSpellClassMask4`, `ImplicitTarget1`, `ImplicitTarget2`, `SpellID`, `VerifiedBuild`) VALUES
(1155059, 133, 0, 2, 6, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 453314, 63906),
(1155060, 142, 0, 3, 6, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 20, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 453314, 63906);

-- Soul Link buff (108446)
DELETE FROM hotfixes.spell_effect WHERE `ID` = 119566;
INSERT INTO hotfixes.spell_effect (`ID`, `EffectAura`, `DifficultyID`, `EffectIndex`, `Effect`, `EffectAmplitude`, `EffectAttributes`, `EffectAuraPeriod`, `EffectBonusCoefficient`, `EffectChainAmplitude`, `EffectChainTargets`, `EffectItemType`, `EffectMechanic`, `EffectPointsPerResource`, `EffectPosFacing`, `EffectRealPointsPerLevel`, `EffectTriggerSpell`, `BonusCoefficientFromAP`, `PvpMultiplier`, `Coefficient`, `Variance`, `ResourceCoefficient`, `GroupSizeBasePointsCoefficient`, `EffectBasePoints`, `ScalingClass`, `TargetNodeGraph`, `EffectMiscValue1`, `EffectMiscValue2`, `EffectRadiusIndex1`, `EffectRadiusIndex2`, `EffectSpellClassMask1`, `EffectSpellClassMask2`, `EffectSpellClassMask3`, `EffectSpellClassMask4`, `ImplicitTarget1`, `ImplicitTarget2`, `SpellID`, `VerifiedBuild`) VALUES
(119566, 81, 0, 0, 6, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0.5, 0, 0, 0, 1, 20, 0, 0, 127, 0, 12, 12, 0, 0, 0, 0, 1, 0, 108446, 63906);

DELETE FROM hotfixes.hotfix_data WHERE `Id` IN (800017, 800018, 800019, 800020, 800021);
INSERT INTO hotfixes.hotfix_data (`Id`, `UniqueId`, `TableHash`, `RecordId`, `Status`, `VerifiedBuild`) VALUES
(800017, 800017, 4030871717, 1052151, 1, 63906),
(800018, 800018, 4030871717, 1104809, 1, 63906),
(800019, 800019, 4030871717, 119566, 1, 63906),
(800020, 800020, 4030871717, 1155059, 1, 63906),
(800021, 800021, 4030871717, 1155060, 1, 63906);

-- =============================================================================
-- WORLD
-- =============================================================================

DELETE FROM world.spell_script_names WHERE `spell_id` IN (108370, 219272, 108415, 234153, 387630, 6262, 104316, 364750);
DELETE FROM world.spell_script_names WHERE `ScriptName` IN (
    'spell_warl_soul_leech',
    'spell_warl_demon_skin',
    'spell_warl_soul_link_passive',
    'spell_warl_drain_life_soulburn',
    'spell_warl_soulburn_drain_life_absorb',
    'spell_warl_healthstone_soulburn',
    'spell_warl_drain_life',
    'spell_warl_healthstone_heal',
    'spell_warlock_call_dreadstalkers',
    'spell_warlock_call_dreadstalkers_summon'
);
INSERT INTO world.spell_script_names (`spell_id`, `ScriptName`) VALUES
(108370, 'spell_warl_soul_leech'),
(219272, 'spell_warl_demon_skin'),
(108415, 'spell_warl_soul_link_passive'),
(234153, 'spell_warl_drain_life_soulburn'),
(387630, 'spell_warl_soulburn_drain_life_absorb'),
(6262, 'spell_warl_healthstone_soulburn'),
(104316, 'spell_warlock_call_dreadstalkers'),
(364750, 'spell_warlock_call_dreadstalkers_summon');

DELETE FROM world.spell_proc WHERE `SpellId` IN (108370, 452999, 387630);
INSERT INTO world.spell_proc (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `SpellFamilyMask3`, `ProcFlags`, `ProcFlags2`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(108370, 0, 0, 0, 0, 0, 0, 0x00051000, 0, 1, 2, 0, 0, 0, 0, 100, 0, 0),
(452999, 0, 0, 0, 0, 0, 0, 0x00051000, 0, 1, 2, 0, 0, 0, 0, 100, 0, 0),
(387630, 0, 0, 0, 0, 0, 0, 0x00204000, 0, 2, 2, 0, 0, 0, 0, 100, 0, 0);

DELETE FROM world.serverside_spell_effect WHERE `SpellID` = 219272 AND `EffectIndex` = 3;
