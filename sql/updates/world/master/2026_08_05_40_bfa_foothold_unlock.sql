-- BfA War Campaign: unlock the Zandalar footholds (Alliance)
--
-- Root cause of "nothing happens when accepting Foothold: Zuldazar/Nazmir/Vol'dun":
-- the footholds (51570/51571/51572) are the "Choose a foothold" choices of the umbrella
-- quest 51569 "The Zandalar Campaign" (obj = kill-credit 146017), offered by Halford
-- Wyrmbane (135612). The client will not accept a foothold unless the player is ON 51569.
-- In retail 51569's prereq is a deep BfA intro chain (53074<-51715<-51714...) that is not
-- built in this world DB, so the player can never reach 51569 -> can never accept a foothold.
--
-- Fix 1: make 51569 reachable right after "The War Campaign" (52654), which the player DOES
--        complete. Halford then offers "The Zandalar Campaign" as the foothold-selection step.
UPDATE `quest_template_addon` SET `PrevQuestID`=52654 WHERE `ID`=51569;

-- Fix 2: accepting any foothold at the Zandalar Campaign table (144635) gives the accepting
--        player kill-credit 146017, completing 51569's "Choose a foothold" objective
--        (matches retail, where picking a foothold at the war table advances the umbrella quest).
DELETE FROM `smart_scripts` WHERE `entryorguid`=144635 AND `source_type`=0 AND `id` IN (1,2,3);
INSERT INTO `smart_scripts`
(`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`action_param7`,`action_param_string`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_param_string`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES
(144635,0,1,0,'',19,0,100,0,51570,0,0,0,0,'',33,146017,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Zandalar Campaign - On Quest 51570 (Foothold: Zuldazar) Accepted - Kill Credit 146017 (Choose a foothold)'),
(144635,0,2,0,'',19,0,100,0,51571,0,0,0,0,'',33,146017,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Zandalar Campaign - On Quest 51571 (Foothold: Nazmir) Accepted - Kill Credit 146017 (Choose a foothold)'),
(144635,0,3,0,'',19,0,100,0,51572,0,0,0,0,'',33,146017,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Zandalar Campaign - On Quest 51572 (Foothold: Vol\'dun) Accepted - Kill Credit 146017 (Choose a foothold)');
