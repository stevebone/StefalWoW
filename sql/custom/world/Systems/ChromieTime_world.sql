-- Chromie Time world data, re-authored (audit item R2, gaps C2/M1/M2/m1).
-- History: the original chromie world SQL was first committed as 2026_03_06_00_world.sql
-- (9f87195c11 + ae563c2ab3); that filename was clobbered by upstream 08a0280829 (warrior
-- "Brutal Finish") and the chromie content now ships as 2026_03_06_05_world.sql. That file
-- still carries pre-audit data errors, corrected here:
--  * terrain-swap conditions used Expansions-enum-style ConditionValue1 (1..6) which never
--    match CONDITION_CHROMIE_TIME's comparand ActivePlayerData::UiChromieTimeExpansionID —
--    that field holds UiChromieTimeExpansionInfo.ID (DB2 record ids, wago @12.0.7.68887:
--    Cata=5, TBC=6, WotLK=7, MoP=8, WoD=9, Legion=10, SL=14, BfA=15, DF=16)
--  * gossip menu 25426 used one custom option (GossipOptionID -250000, GossipNpcOptionID
--    NULL -> wrong opener packet SMSG_NPC_INTERACTION_OPEN_RESULT); retail uses
--    state-dependent options 51901/51902/51903 with GossipNpcOptionID=32282 opening the UI
--    via SMSG_GOSSIP_OPTION_NPC_INTERACTION (capture A recs 4406/4448/4675 @68275)
--  * no phase data: retail DF selection adds phases 16439+16440 (capture A rec 4621)

-- ============================================================================
-- Terrain swap conditions (CONDITION_CHROMIE_TIME=60, NegativeCondition=1:
-- "NOT in CT for that timeline"; rows in the same ElseGroup are ANDed).
-- SourceTypeOrReferenceId=25 = CONDITION_SOURCE_TYPE_TERRAIN_SWAP,
-- SourceEntry = terrain swap map ID.
-- ConditionValue1 = UiChromieTimeExpansionInfo.ID (DB2 record id).
-- ============================================================================

-- Blasted Lands WoD terrain (1190): not active for TBC/WotLK/Cata CT
-- (original data omitted MoP; omission preserved pending retail verification)
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=25 AND `SourceEntry`=1190;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(25,0,1190,0,0,60,0,6,0,0,1,0,0,'','Blasted Lands WoD terrain: not active in TBC CT'),
(25,0,1190,0,0,60,0,7,0,0,1,0,0,'','Blasted Lands WoD terrain: not active in WotLK CT'),
(25,0,1190,0,0,60,0,5,0,0,1,0,0,'','Blasted Lands WoD terrain: not active in Cata CT');

-- Silithus: The Wound (1817): not active for pre-BfA CT (TBC..Legion)
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=25 AND `SourceEntry`=1817;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(25,0,1817,0,0,60,0,6,0,0,1,0,0,'','Silithus Wound terrain: not active in TBC CT'),
(25,0,1817,0,0,60,0,7,0,0,1,0,0,'','Silithus Wound terrain: not active in WotLK CT'),
(25,0,1817,0,0,60,0,5,0,0,1,0,0,'','Silithus Wound terrain: not active in Cata CT'),
(25,0,1817,0,0,60,0,8,0,0,1,0,0,'','Silithus Wound terrain: not active in MoP CT'),
(25,0,1817,0,0,60,0,9,0,0,1,0,0,'','Silithus Wound terrain: not active in WoD CT'),
(25,0,1817,0,0,60,0,10,0,0,1,0,0,'','Silithus Wound terrain: not active in Legion CT');

-- Stormwind Gunship Pandaria Start (1066): not active for pre-MoP CT
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=25 AND `SourceEntry`=1066;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(25,0,1066,0,0,60,0,6,0,0,1,0,0,'','SW Gunship MoP terrain: not active in TBC CT'),
(25,0,1066,0,0,60,0,7,0,0,1,0,0,'','SW Gunship MoP terrain: not active in WotLK CT'),
(25,0,1066,0,0,60,0,5,0,0,1,0,0,'','SW Gunship MoP terrain: not active in Cata CT');

-- Orgrimmar Gunship Pandaria Start (1074): not active for pre-MoP CT
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=25 AND `SourceEntry`=1074;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(25,0,1074,0,0,60,0,6,0,0,1,0,0,'','Org Gunship MoP terrain: not active in TBC CT'),
(25,0,1074,0,0,60,0,7,0,0,1,0,0,'','Org Gunship MoP terrain: not active in WotLK CT'),
(25,0,1074,0,0,60,0,5,0,0,1,0,0,'','Org Gunship MoP terrain: not active in Cata CT');

-- Twilight Highlands Dragonmaw Port (736): not active for pre-Cata CT
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=25 AND `SourceEntry`=736;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(25,0,736,0,0,60,0,6,0,0,1,0,0,'','TH Dragonmaw terrain: not active in TBC CT'),
(25,0,736,0,0,60,0,7,0,0,1,0,0,'','TH Dragonmaw terrain: not active in WotLK CT');

-- Mount Hyjal default terrain (719): not active for pre-Cata CT
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=25 AND `SourceEntry`=719;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(25,0,719,0,0,60,0,6,0,0,1,0,0,'','Hyjal terrain: not active in TBC CT'),
(25,0,719,0,0,60,0,7,0,0,1,0,0,'','Hyjal terrain: not active in WotLK CT');

-- ============================================================================
-- Chromie (167032) gossip menu 25426 — retail option layout (capture A @68275):
--  pre-select  (rec 4406): 51901 OptionNpc=40 + 109278
--  post-select (rec 4675): 51902 OptionNpc=40 + 51903 OptionNpc=0 (deselect) + 109278
-- 51901/51902 carry GossipNpcOptionID=32282 -> retail opener packet
-- SMSG_GOSSIP_OPTION_NPC_INTERACTION (rec 4448; SMSG_NPC_INTERACTION_OPEN_RESULT
-- has 0 wire hits). OptionBroadcastTextID unmined (texts taken from sniff strings).
-- 109278's Timewalking-info submenu (ActionMenuID) is unmined - option is inert.
-- 51903's server action (SetChromieTime(0)) lands with the deselect script (audit R6).
-- ============================================================================
DELETE FROM `gossip_menu_option` WHERE `MenuID`=25426;
INSERT INTO `gossip_menu_option` (`MenuID`,`GossipOptionID`,`OptionID`,`OptionNpc`,`OptionText`,`OptionBroadcastTextID`,`Language`,`Flags`,`ActionMenuID`,`ActionPoiID`,`GossipNpcOptionID`,`BoxCoded`,`BoxMoney`,`BoxText`,`BoxBroadcastTextID`,`SpellID`,`OverrideIconID`,`VerifiedBuild`) VALUES
(25426,51901,0,40,'|cFF0000FF(Recommended)|r Select a timeline.',0,0,0,0,0,32282,0,0,NULL,0,NULL,NULL,68275),
(25426,51902,1,40,'Select a different timeline.',0,0,0,0,0,32282,0,0,NULL,0,NULL,NULL,68275),
(25426,51903,2,0,'I''d like to return to the present timeline, Chromie.',0,0,0,0,0,NULL,0,0,NULL,0,NULL,NULL,68275),
(25426,109278,3,0,'I have a question about Timewalking Campaigns.',0,0,0,0,0,NULL,0,0,NULL,0,NULL,NULL,68275);

-- State-dependent visibility (CONDITION_SOURCE_TYPE_GOSSIP_MENU_OPTION=15,
-- SourceGroup=MenuID, SourceEntry=OptionID/OrderIndex):
-- 51901 only while NOT in Chromie Time; 51902/51903 only while in Chromie Time.
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=15 AND `SourceGroup`=25426;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(15,25426,0,0,0,60,0,0,0,0,1,0,0,'','Chromie: Select a timeline - only while not in Chromie Time'),
(15,25426,1,0,0,60,0,0,0,0,0,0,0,'','Chromie: Select a different timeline - only while in Chromie Time'),
(15,25426,2,0,0,60,0,0,0,0,0,0,0,'','Chromie: Return to the present - only while in Chromie Time');

-- ============================================================================
-- Dragonflight Chromie Time phases 16439/16440 (capture A rec 4621: DF select
-- adds both, nothing removed). Scoped to the capital zones where the phase
-- change is sniff-verified (Stormwind City 1519 / Orgrimmar 1637); other zones'
-- phase coverage and other expansions' phase IDs are unmined (audit m1 -
-- partially deferred). Conditions: CONDITION_SOURCE_TYPE_PHASE=26,
-- SourceGroup=PhaseId, SourceEntry=0 (all areas of the phase), active only
-- while in DF Chromie Time (UiChromieTimeExpansionInfo.ID 16).
-- ============================================================================
DELETE FROM `phase_area` WHERE `PhaseId` IN (16439,16440);
INSERT INTO `phase_area` (`AreaId`,`PhaseId`,`Comment`) VALUES
(1519,16439,'Stormwind City - Dragonflight Chromie Time phase'),
(1519,16440,'Stormwind City - Dragonflight Chromie Time phase'),
(1637,16439,'Orgrimmar - Dragonflight Chromie Time phase'),
(1637,16440,'Orgrimmar - Dragonflight Chromie Time phase');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=26 AND `SourceGroup` IN (16439,16440);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(26,16439,0,0,0,60,0,16,0,0,0,0,0,'','Phase 16439 only in Dragonflight Chromie Time'),
(26,16440,0,0,0,60,0,16,0,0,0,0,0,'','Phase 16440 only in Dragonflight Chromie Time');


-- ============================================================================
-- Chromie NPC (167032) Orgrimmar spawn
-- Retail has Chromie in both Stormwind and Orgrimmar Embassy areas
-- Stormwind spawn already exists (guid 8000063, map 0, zone 1519)
-- ============================================================================
SET @CGUID := 900000;

DELETE FROM `creature` WHERE `guid`=@CGUID+1247 AND `id`=167032;
INSERT INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnDifficulties`,`phaseUseFlags`,`PhaseId`,`PhaseGroup`,`terrainSwapMap`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curHealthPct`,`MovementType`,`npcflag`,`unit_flags`,`unit_flags2`,`unit_flags3`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES
(@CGUID+1247,167032,1,1637,8618,'0',0,0,0,-1,0,0,1606.17,-4389.46,19.47,2.33,120,0,0,100,0,NULL,NULL,NULL,NULL,'',NULL,0);

-- Chromie Time deselect path (audit R6): bind the script that handles gossip
-- option 51903 ("I'd like to return to the present timeline, Chromie.",
-- capture A rec 4675) to Chromie 167032. The option row itself ships with
-- 2026_08_08_07_world.sql.
UPDATE `creature_template` SET `ScriptName`='npc_chromie_timewalking' WHERE `entry`=167032;
