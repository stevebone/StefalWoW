-- BfA War Campaign: complete the "Zuldazar Foothold" (51308) sail-to-Zuldazar step
--
-- The player's real quest is 51308 "Zuldazar Foothold" (from Halford 135612), NOT 51570.
-- Objective StorageIndex 0 = "Speak with Jes-Tereth to travel to Zuldazar" -> kill-credit 137403
-- ("Speak with Captain Kill Credit"). Selecting "Set sail for Zuldazar" at Jes-Tereth (135681)
-- must give credit 137403 AND teleport to Zuldazar. The player clicks the quest-marked option
-- ("(Quest) Set sail for Zuldazar" = menu 22508 OptionID 6) while on the quest, or the plain
-- transport line (OptionID 9) otherwise -- so both give the credit + teleport to Fort Victory.
DELETE FROM `smart_scripts` WHERE `entryorguid`=135681 AND `source_type`=0 AND `id` IN (5,6,7);
INSERT INTO `smart_scripts`
(`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`action_param7`,`action_param_string`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_param_string`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES
(135681,0,5,0,'',62,0,100,0,22508,9,0,0,0,'',33,137403,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Jes-Tereth - Gossip Set sail for Zuldazar (plain opt9) - Kill Credit 137403 (51308 obj Speak with Jes-Tereth)'),
(135681,0,6,0,'',62,0,100,0,22508,6,0,0,0,'',62,1642,0,0,0,0,0,0,'',7,0,0,0,0,'',2098.6,191.8,4.9,3.6,'Jes-Tereth - Gossip (Quest) Set sail for Zuldazar (opt6) - Teleport player to Fort Victory'),
(135681,0,7,0,'',62,0,100,0,22508,6,0,0,0,'',33,137403,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Jes-Tereth - Gossip (Quest) Set sail for Zuldazar (opt6) - Kill Credit 137403 (51308 obj Speak with Jes-Tereth)');
