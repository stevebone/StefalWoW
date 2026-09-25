-- Chromie Time: terrain swap conditions
-- ConditionType 60 = CONDITION_CHROMIE_TIME (value1: 0=any CT, N=specific expansion)
-- NegativeCondition=1 inverts: "NOT in CT for expansion N"
-- SourceTypeOrReferenceId=25 = CONDITION_SOURCE_TYPE_TERRAIN_SWAP
-- SourceEntry = terrain swap map ID
-- Multiple conditions with same ElseGroup are ANDed

-- Remove terrain swaps from defaults that need to be conditional
-- They will be re-added as conditional via the conditions table
-- Note: terrain_swap_defaults entries without conditions are always active
-- Entries WITH conditions are only active when ALL conditions are met

-- Blasted Lands WoD terrain (1190): should NOT be active for Classic/TBC/WotLK/Cata CT
-- (Blasted Lands was changed by WoD Iron Horde invasion — pre-WoD CT should see original)
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=25 AND `SourceEntry`=1190;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(25,0,1190,0,0,60,0,1,0,0,1,0,0,'','Blasted Lands WoD terrain: not active in TBC CT'),
(25,0,1190,0,0,60,0,2,0,0,1,0,0,'','Blasted Lands WoD terrain: not active in WotLK CT'),
(25,0,1190,0,0,60,0,3,0,0,1,0,0,'','Blasted Lands WoD terrain: not active in Cata CT');

-- Silithus: The Wound (1817): should NOT be active for pre-BfA CT (expansions 1-6)
-- (Silithus was changed by BfA sword of Sargeras event)
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=25 AND `SourceEntry`=1817;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(25,0,1817,0,0,60,0,1,0,0,1,0,0,'','Silithus Wound terrain: not active in TBC CT'),
(25,0,1817,0,0,60,0,2,0,0,1,0,0,'','Silithus Wound terrain: not active in WotLK CT'),
(25,0,1817,0,0,60,0,3,0,0,1,0,0,'','Silithus Wound terrain: not active in Cata CT'),
(25,0,1817,0,0,60,0,4,0,0,1,0,0,'','Silithus Wound terrain: not active in MoP CT'),
(25,0,1817,0,0,60,0,5,0,0,1,0,0,'','Silithus Wound terrain: not active in WoD CT'),
(25,0,1817,0,0,60,0,6,0,0,1,0,0,'','Silithus Wound terrain: not active in Legion CT');

-- Stormwind Gunship Pandaria Start (1066): should NOT be active for pre-MoP CT (expansions 1-3)
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=25 AND `SourceEntry`=1066;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(25,0,1066,0,0,60,0,1,0,0,1,0,0,'','SW Gunship MoP terrain: not active in TBC CT'),
(25,0,1066,0,0,60,0,2,0,0,1,0,0,'','SW Gunship MoP terrain: not active in WotLK CT'),
(25,0,1066,0,0,60,0,3,0,0,1,0,0,'','SW Gunship MoP terrain: not active in Cata CT');

-- Orgrimmar Gunship Pandaria Start (1074): should NOT be active for pre-MoP CT (expansions 1-3)
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=25 AND `SourceEntry`=1074;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(25,0,1074,0,0,60,0,1,0,0,1,0,0,'','Org Gunship MoP terrain: not active in TBC CT'),
(25,0,1074,0,0,60,0,2,0,0,1,0,0,'','Org Gunship MoP terrain: not active in WotLK CT'),
(25,0,1074,0,0,60,0,3,0,0,1,0,0,'','Org Gunship MoP terrain: not active in Cata CT');

-- Twilight Highlands Dragonmaw Port (736): should NOT be active for pre-Cata CT (expansions 1-2)
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=25 AND `SourceEntry`=736;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(25,0,736,0,0,60,0,1,0,0,1,0,0,'','TH Dragonmaw terrain: not active in TBC CT'),
(25,0,736,0,0,60,0,2,0,0,1,0,0,'','TH Dragonmaw terrain: not active in WotLK CT');

-- Mount Hyjal default terrain (719): should NOT be active for pre-Cata CT (expansions 1-2)
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=25 AND `SourceEntry`=719;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(25,0,719,0,0,60,0,1,0,0,1,0,0,'','Hyjal terrain: not active in TBC CT'),
(25,0,719,0,0,60,0,2,0,0,1,0,0,'','Hyjal terrain: not active in WotLK CT');

-- ============================================================================
-- Chromie NPC (167032) gossip option: open Chromie Time UI
-- OptionNpc=40 (ChromieTimeNpc) triggers NPCInteractionOpenResult with
-- PlayerInteractionType::ChromieTime, which opens the client's CT expansion picker
-- ============================================================================
DELETE FROM `gossip_menu_option` WHERE `MenuID`=25426;
INSERT INTO `gossip_menu_option` (`MenuID`,`GossipOptionID`,`OptionID`,`OptionNpc`,`OptionText`,`OptionBroadcastTextID`,`Language`,`Flags`,`ActionMenuID`,`ActionPoiID`,`GossipNpcOptionID`,`BoxCoded`,`BoxMoney`,`BoxText`,`BoxBroadcastTextID`,`SpellID`,`OverrideIconID`,`VerifiedBuild`) VALUES
(25426,-250000,0,40,'I want to select a different timeline.',0,0,0,0,0,NULL,0,0,NULL,0,NULL,NULL,0);