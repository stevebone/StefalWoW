-- Trading Post (Perks)

-- NPC: 185467 Wilder Seabraid (Stormwind)
-- NPC: 185468 Tawny Seabraid (Stormwind)
-- NPC: 219243 Teha (Dornogal)
-- NPC: 219244 Andee Seabraid (Dornogal)
-- NPC: 249042 Zalani

-- Quest: 66858 Tour the Trading Post

INSERT IGNORE INTO `creature_questender` VALUES
(185468, 66858, 0),
(219244, 66858, 0);

UPDATE `creature_template` SET `ScriptName` = 'npc_perks_program_vendor' WHERE `entry` IN (185467, 185468, 249042, 219244, 219243);

-- New Silvemoon
DELETE FROM creature_template WHERE entry=249042;
INSERT INTO creature_template (entry, KillCredit1, KillCredit2, name, femaleName, subname, TitleAlt, IconName, RequiredExpansion, VignetteID, faction, npcflag, speed_walk, speed_run, scale, Classification, dmgschool, BaseAttackTime, RangeAttackTime, BaseVariance, RangeVariance, unit_class, unit_flags, unit_flags2, unit_flags3, family, trainer_class, type, VehicleId, AIName, MovementType, ExperienceModifier, RacialLeader, movementId, WidgetSetID, WidgetSetUnitConditionID, RegenHealth, CreatureImmunitiesId, flags_extra, ScriptName, StringId, VerifiedBuild) VALUES 
(249042, 0, 0, 'Zalani', NULL, 'Zen'shiri Trading Post', NULL, NULL, 0, 0, 35, 2251799813685251, 1, 1.14286, 1, 0, 0, 2000, 0, 1, 1, 1, 768, 2048, 0, 0, 0, 7, 0, '', 0, 1, 0, 0, 0, 0, 1, 0, 0, 'npc_perks_program_vendor', NULL, 66562);

