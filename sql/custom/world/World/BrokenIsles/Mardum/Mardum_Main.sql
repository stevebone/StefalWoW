-- Mardum (Demon Hunter) Fixes

-- NPC: 98484 Mo'arg Brute
-- NPC: 98483 Hellish Imp
-- NPC: 98482 Foul Felstalker
-- NPC: 98497 Imp Mother
-- NPC: 98486 Wrath Warrior

-- NPC: 97142 Fel Spreader

-- NPC: 93011 Kayn Sunfury <Illidari>
-- NPC: 98460 Kor'vas Bloodthorn <Illidari>

-- Quest: 39279 Assault On Mardum (Bonus Objectives)

-- Spell: 191827 Destroying Fel Spreader (spell click)
-- Spell: 199617 Assault on Mardum: Fel Spreader Fel Explosion

-- ========================= Fixes for the Bonus quest
DELETE FROM `conversation_template` WHERE `Id` = 581;
INSERT INTO `conversation_template` (`Id`, `FirstLineId`, `VerifiedBuild`) VALUES
(581, 1511, 69875);

DELETE FROM `conversation_line_template` WHERE `Id` = 1511;
INSERT INTO `conversation_line_template` (`Id`, `UiCameraID`, `VerifiedBuild`) VALUES
(1511, 119, 69875);

DELETE FROM `conversation_actors` WHERE `ConversationId` = 581;
INSERT  INTO `conversation_actors` (`ConversationId`, `ConversationActorId`, `Idx`, `CreatureId`, `CreatureDisplayInfoId`, `NoActorObject`, `ActivePlayerObject`, `VerifiedBuild`) VALUES
(581, 49947, 0, 93127, 61698, 0, 0, 69875);

-- Clicker casts 191827 (2s cast) on the spreader (cast_flags 1 = caster is clicker)
DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` = 97142;
INSERT INTO `npc_spellclick_spells` (`npc_entry`,`spell_id`,`cast_flags`,`user_type`) VALUES
(97142,191827,1,0);

-- Fel Spreader (97142) spellclick -> npc_fel_spreader script
UPDATE `creature_template` SET `ScriptName` = 'npc_fel_spreader', `npcflag` = `npcflag` | 0x01000000 WHERE `entry` = 97142;

-- Only clickable while Assault On Mardum (39279) is in the quest log
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 18 AND `SourceGroup` = 97142 AND `SourceEntry` = 191827;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(18,97142,191827,0,0,9,0,39279,0,0,0,0,0,'','Fel Spreader - spellclick requires quest 39279 Assault On Mardum in log');



-- ========================= Fixes for the Invasion Begins
-- Scene: 1116 The Invasion Begins (banner planted) -> Kayn Sunfury (98229) dialogue on complete
UPDATE `scene_template` SET `ScriptName` = 'scene_the_invasion_begins_banner_planted' WHERE `SceneId` = 1116;

-- Add missing gossip menus
DELETE FROM `creature_template_gossip` WHERE `CreatureID` IN (93011);
INSERT INTO `creature_template_gossip` (`CreatureID`, `MenuID`, `VerifiedBuild`) VALUES
(93011, 18761, 69587);

-- Creature Difficulties
-- Remove incorrect records
DELETE FROM `creature_template_difficulty` WHERE `DifficultyID` = 1 AND `Entry` IN (
	98486, 98482, 98484, 98483, 93115, 101288, 95226, 93105, 93112, 94651,
 96400, 94654, 95046, 102714, 93221, 93716, 99759, 96493, 96441, 96494,
 96473, 102726, 97034, 97706, 97059, 96277, 96402, 96280, 96278, 97014,
 103432, 96279, 97370, 102724, 97058, 97057, 100243, 100244, 93802, 98497, 98986
);

-- Floating flag
UPDATE `creature_template_difficulty` SET `StaticFlags1` = `StaticFlags1` | 0x20000000 WHERE `Entry` IN (94744);

-- SAI
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` IN (98484,98486,98497,98482);
DELETE FROM smart_scripts WHERE entryorguid IN (98484,98486,98497,98482) AND source_type = 0;
INSERT INTO smart_scripts (entryorguid, source_type, id, link, Difficulties, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, event_param5, event_param_string, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, action_param7, action_param_string, target_type, target_param1, target_param2, target_param3, target_param4, target_param_string, target_x, target_y, target_z, target_o, comment) VALUES
(98484, 0, 0, 0, '', 0, 0, 100, 0, 1000, 2000, 8000, 9000, 0, '', 11, 200425, 0, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mo''arg Brute - In Combat - Cast ''Brutal Slam'''),
(98484, 0, 1, 4, '', 0, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Mo Arg Brute - In Combat - Talk (No Repeat)'),

(98486, 0, 0, 4, '', 0, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Wrath Warrior - In Combat - Talk (No Repeat)'),

(98497, 0, 0, 0, '', 1, 0, 100, 1, 1000, 1000, 0, 0, 0, '', 11, 188485, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Time = 1 seconds (OOC) - Self: Cast spell  188485 on Self'),
(98497, 0, 1, 0, '', 0, 0, 100, 0, 2000, 3000, 15000, 18000, 0, '', 11, 200393, 2, 0, 0, 0, 0, 0, '', 2, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Imp Mother - In Combat - Cast ''Rain of Imp'''),
(98497, 0, 2, 4, '', 0, 0, 30, 1, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Imp Mother - In Combat - Talk (No Repeat)'),

(98482, 0, 0, 0, '', 0, 0, 100, 0, 5000, 8000, 12000, 15000, 0, '', 11, 200417, 0, 0, 0, 0, 0, 0, '', 1, 0, 0, 0, 0, '', 0, 0, 0, 0, 'Foul Felstalker - In Combat - Cast ''Foul Fel''');



