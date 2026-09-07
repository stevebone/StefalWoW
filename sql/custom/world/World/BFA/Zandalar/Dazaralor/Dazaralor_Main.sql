-- Dazar'Alor Main Fixes

-- NPC: 133407 Ambassador Blackguard
-- NPC: 150953 Rastari Portal Keeper

-- Quest: 55137 For the Horde

-- =====================================================================
-- Fixes for Quest: 55137 For the Horde
-- Dazar'alor - Rastari Portal Keeper (150953) + Portal to Orgrimmar (78687)
-- Keeper channels the portal; clicking it teleports to the Gates of
-- Orgrimmar and credits quest 55137 objective 388745 (via go script).
-- Map 1642 (Zandalar) / zone 8499 (Zuldazar) / area 8670 (Dazar'alor)
-- =====================================================================

SET @OGUID := 70004525; -- next free block after Zandalar_GoSpawns (@OGUID 70000000 + 4524)
SET @PORTAL_GO := 78687; -- MUST match quest 55137 objective ObjectID (type 14)
SET @TELE_SPELL := 429913; -- Portal: Orgrimmar (SPELL_EFFECT_TELEPORT_UNITS)
SET @KEEPER_GUID := 70032357; -- existing in-world spawn of Rastari Portal Keeper
SET @KEEPER_AURA := 287470; -- Zandalari Arcanital channel aura (from 148265)
SET @ENDER_NPC := 133407; -- Ambassador Blackguard (quest 55137 turn-in)

-- 1) Reuse the existing keeper spawn: passive, non-attackable, channels

UPDATE creature
SET map=1642, zoneId=8499, areaId=8670,
position_x=-1084.229980, position_y=798.379028, position_z=497.076996,
orientation=1.86750, spawntimesecs=120, wander_distance=0, MovementType=0,
npcflag=0, unit_flags=768, unit_flags2=2048
WHERE guid=@KEEPER_GUID;

UPDATE creature_template SET faction=2976, unit_flags=768, unit_flags2=2048
WHERE entry=150953;

DELETE FROM creature_addon WHERE guid=@KEEPER_GUID;
INSERT INTO creature_addon
(guid, PathId, mount, StandState, AnimTier, VisFlags, SheathState, PvpFlags, emote, aiAnimKit, movementAnimKit, meleeAnimKit, visibilityDistanceType, auras) VALUES
(@KEEPER_GUID, 0, 0, 0, 0, 0, 1, 0, 469, 0, 0, 0, 0, @KEEPER_AURA); -- emote 469 = channel pose; aura = hand/feet glow

-- 2) Portal gameobject template: GOOBER (type 10) so a use hook fires,
-- plus the ScriptName that does teleport + objective credit.
-- Partial-column INSERT (schema has 49 columns; unset Data* default 0).
-- Data1 = questID (55137), Data10 = spell (teleport fallback)

DELETE FROM gameobject_template WHERE entry=@PORTAL_GO;
INSERT INTO gameobject_template
(entry, type, displayId, name, IconName, size, Data1, Data10, ScriptName, VerifiedBuild) VALUES
(@PORTAL_GO, 10, 12807, 'Portal to Orgrimmar', 'interact', 1, 55137, @TELE_SPELL, 'go_dazaralor_orgrimmar_portal', 0);

DELETE FROM gameobject_template_addon WHERE entry=@PORTAL_GO;
INSERT INTO gameobject_template_addon
(entry, faction, flags, WorldEffectID, AIAnimKitID) VALUES
(@PORTAL_GO, 0, 0, 0, 3503);

-- 3) Teleport destination: Gates of Orgrimmar (map 1)

DELETE FROM spell_target_position WHERE ID=@TELE_SPELL AND EffectIndex=0;
INSERT INTO spell_target_position
(ID, EffectIndex, MapID, PositionX, PositionY, PositionZ, Orientation, VerifiedBuild) VALUES
(@TELE_SPELL, 0, 1, 1407.266235, -4368.846191, 25.452627, 0.112644, 0);

-- 4) Spawn the portal IN FRONT of the keeper (faces back toward him)

DELETE FROM gameobject WHERE guid=@OGUID+0;
INSERT INTO gameobject
(guid, id, map, zoneId, areaId, spawnDifficulties, phaseUseFlags, PhaseId, PhaseGroup, terrainSwapMap, position_x, position_y, position_z, orientation, rotation0, rotation1, rotation2, rotation3, spawntimesecs, animprogress, state, ScriptName, StringId, VerifiedBuild) VALUES
(@OGUID+0, @PORTAL_GO, 1642, 8499, 8670, '0', 0, 0, 0, -1, -1085.107, 801.248, 497.076996, 5.00909, 0, 0, 0.594806, -0.803869, 120, 255, 1, '', NULL, 0);

DELETE FROM gameobject_addon WHERE guid=@OGUID+0;
INSERT INTO gameobject_addon
(guid, parent_rotation0, parent_rotation1, parent_rotation2, parent_rotation3, WorldEffectID, AIAnimKitID) VALUES
(@OGUID+0, 0, 0, 0, 1, 0, 3503);

-- 5) Portal visible only to Horde

DELETE FROM conditions WHERE SourceTypeOrReferenceId=32 AND SourceGroup=8 AND SourceEntry=@PORTAL_GO;
INSERT INTO conditions
(SourceTypeOrReferenceId, SourceGroup, SourceEntry, SourceId, ElseGroup, ConditionTypeOrReference, ConditionTarget, ConditionValue1, ConditionValue2, ConditionValue3, ConditionStringValue1, NegativeCondition, ErrorType, ErrorTextId, ScriptName, Comment) VALUES
(32, 8, @PORTAL_GO, 0, 0, 6, 0, 67, 0, 0, '', 0, 0, 0, '', 'Portal to Orgrimmar visible only for Horde');

-- 6) Missing quest-ender so turn-in works at Ambassador Blackguard

DELETE FROM creature_questender WHERE id=@ENDER_NPC AND quest=55137;
INSERT INTO creature_questender (id, quest, VerifiedBuild) VALUES
(@ENDER_NPC, 55137, 0);

-- ---------------------------------------------------------------------  
-- 7) Gold minimap POI dot for the objective at the portal (Dazar'alor)  
--    Requires a non-zero SpawnTrackingID on the objective blob, wired  
--    through the spawn_tracking* tables to the portal GO spawn (@OGUID+0).  
--    SpawnType 1 = gameobject. Map 1642. UiMapID 1165 (Dazar'alor city).  
-- ---------------------------------------------------------------------  
-- 7) Gold minimap POI dots for quest 55137 (matches retail sniff)  
DELETE FROM `quest_poi_points` WHERE `QuestID`=55137;  
DELETE FROM `quest_poi`        WHERE `QuestID`=55137;  
  
INSERT INTO `quest_poi`  
    (`QuestID`, `BlobIndex`, `Idx1`, `ObjectiveIndex`, `QuestObjectiveID`, `QuestObjectID`, `MapID`, `UiMapID`, `Priority`, `Flags`, `WorldEffectID`, `PlayerConditionID`, `NavigationPlayerConditionID`, `SpawnTrackingID`, `AlwaysAllowMergingBlobs`, `VerifiedBuild`) VALUES  
(55137, 0, 0, -1, 0,      0, 1,    85,   0, 0, 0, 0, 0, 0, 0, 0), -- play area (Orgrimmar) [442759]  
(55137, 0, 1, 0,  388745, 0, 1642, 1165, 0, 0, 0, 0, 0, 0, 0, 0), -- OBJECTIVE gold dot: Portal to Orgrimmar Taken [442760]  
(55137, 0, 2, 32, 0,      0, 1642, 1165, 0, 2, 0, 0, 0, 0, 0, 0), -- turn-in (Dazar'alor map) [497168]  
(55137, 0, 3, 32, 0,      0, 1642, 1181, 0, 2, 0, 0, 0, 0, 0, 0); -- turn-in [497169]  
  
INSERT INTO `quest_poi_points`  
    (`QuestID`, `Idx1`, `Idx2`, `X`, `Y`, `Z`, `VerifiedBuild`) VALUES  
(55137, 0, 0, 1564, -4191, 53,  0), -- play area point (Orgrimmar, near Blackguard)  
(55137, 1, 0, -1085, 801, 497,  0), -- objective point (the portal in Dazar'alor)  
(55137, 2, 0, -1085, 801, 497,  0), -- turn-in point (Dazar'alor map)  
(55137, 3, 0, 1564, -4191, 53,  0); -- turn-in point (Orgrimmar)