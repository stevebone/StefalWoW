-- Order Advancement: class-conditioned gossip options on shared advisor menu 19646.
-- Each class sees only its own option (CONDITION_CLASS=15, classmask), carrying that class's
-- GossipNpcOptionID so SMSG_GOSSIP_OPTION_NPC_INTERACTION resolves to the class's GarrTalentTree.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=19646;
DELETE FROM gossip_menu_option WHERE MenuID=19646;

-- OptionID, classmask, GossipNpcOptionID  (GossipOptionID -5029376 = proven-working value, one visible per player)
INSERT INTO gossip_menu_option
 (MenuID,GossipOptionID,OptionID,OptionNpc,OptionText,OptionBroadcastTextID,Language,Flags,ActionMenuID,ActionPoiID,GossipNpcOptionID,BoxCoded,BoxMoney,BoxText,BoxBroadcastTextID,SpellID,OverrideIconID,VerifiedBuild)
VALUES
 (19646,-5029376,0 ,32,'Show me the Order Advancement.',0,0,0,0,0,32286,0,0,NULL,0,NULL,NULL,0), -- Warrior
 (19646,-5029376,1 ,32,'Show me the Order Advancement.',0,0,0,0,0,32236,0,0,NULL,0,NULL,NULL,0), -- Paladin
 (19646,-5029376,2 ,32,'Show me the Order Advancement.',0,0,0,0,0,32330,0,0,NULL,0,NULL,NULL,0), -- Hunter
 (19646,-5029376,3 ,32,'Show me the Order Advancement.',0,0,0,0,0,30518,0,0,NULL,0,NULL,NULL,0), -- Rogue
 (19646,-5029376,4 ,32,'Show me the Order Advancement.',0,0,0,0,0,30609,0,0,NULL,0,NULL,NULL,0), -- Priest
 (19646,-5029376,5 ,32,'Show me the Order Advancement.',0,0,0,0,0,30519,0,0,NULL,0,NULL,NULL,0), -- Death Knight
 (19646,-5029376,6 ,32,'Show me the Order Advancement.',0,0,0,0,0,30488,0,0,NULL,0,NULL,NULL,0), -- Shaman
 (19646,-5029376,7 ,32,'Show me the Order Advancement.',0,0,0,0,0,30433,0,0,NULL,0,NULL,NULL,0), -- Mage
 (19646,-5029376,8 ,32,'Show me the Order Advancement.',0,0,0,0,0,30467,0,0,NULL,0,NULL,NULL,0), -- Warlock
 (19646,-5029376,9 ,32,'Show me the Order Advancement.',0,0,0,0,0,30489,0,0,NULL,0,NULL,NULL,0), -- Monk
 (19646,-5029376,10,32,'Show me the Order Advancement.',0,0,0,0,0,30379,0,0,NULL,0,NULL,NULL,0), -- Druid
 (19646,-5029376,11,32,'Show me the Order Advancement.',0,0,0,0,0,32302,0,0,NULL,0,NULL,NULL,0); -- Demon Hunter

INSERT INTO conditions
 (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,ConditionStringValue1,NegativeCondition,ErrorType,ErrorTextId,ScriptName,Comment)
VALUES
 (15,19646,0 ,0,0,15,0,1   ,0,0,'',0,0,0,'','Order Advancement - Warrior only'),
 (15,19646,1 ,0,0,15,0,2   ,0,0,'',0,0,0,'','Order Advancement - Paladin only'),
 (15,19646,2 ,0,0,15,0,4   ,0,0,'',0,0,0,'','Order Advancement - Hunter only'),
 (15,19646,3 ,0,0,15,0,8   ,0,0,'',0,0,0,'','Order Advancement - Rogue only'),
 (15,19646,4 ,0,0,15,0,16  ,0,0,'',0,0,0,'','Order Advancement - Priest only'),
 (15,19646,5 ,0,0,15,0,32  ,0,0,'',0,0,0,'','Order Advancement - Death Knight only'),
 (15,19646,6 ,0,0,15,0,64  ,0,0,'',0,0,0,'','Order Advancement - Shaman only'),
 (15,19646,7 ,0,0,15,0,128 ,0,0,'',0,0,0,'','Order Advancement - Mage only'),
 (15,19646,8 ,0,0,15,0,256 ,0,0,'',0,0,0,'','Order Advancement - Warlock only'),
 (15,19646,9 ,0,0,15,0,512 ,0,0,'',0,0,0,'','Order Advancement - Monk only'),
 (15,19646,10,0,0,15,0,1024,0,0,'',0,0,0,'','Order Advancement - Druid only'),
 (15,19646,11,0,0,15,0,2048,0,0,'',0,0,0,'','Order Advancement - Demon Hunter only');

-- point the other two talent advisors (Archivist Melinda / Zubashi) at the shared conditioned menu
UPDATE creature_template_gossip SET MenuID=19646 WHERE CreatureID IN (108018,97485) AND MenuID IN (19812,20002);

-- Spawn the Warrior order-hall (Skyhold, map 1479) talent advisor Einar the Runecaster (107994); was never spawned.
DELETE FROM creature WHERE id=107994;
INSERT INTO creature (guid,id,map,zoneId,areaId,spawnDifficulties,phaseUseFlags,PhaseId,PhaseGroup,terrainSwapMap,modelid,equipment_id,position_x,position_y,position_z,orientation,spawntimesecs,wander_distance,currentwaypoint,curHealthPct,MovementType,npcflag,unit_flags,unit_flags2,unit_flags3,ScriptName,StringId,VerifiedBuild)
VALUES (50052004,107994,1479,7813,7813,0,0,0,0,-1,0,0,1053.0,7228.0,100.46,3.0,3600,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0);
