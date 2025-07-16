-- NPC: 11260 Northshire Peasant

DELETE FROM `creature_text` WHERE `CreatureID`=11260;
INSERT INTO creature_text (CreatureID, groupid, id, text, type, language, probability, emote, duration, sound, BroadcastTextId, TextRange, comment) VALUES
(11260, 0, 1, 'Aaah my foot!', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - reaction'),
(11260, 0, 2, 'Alright one more swing...', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - agreement'),
(11260, 0, 3, 'I guess I can do this!', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - reluctant acceptance'),
(11260, 0, 4, 'If you want', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - neutral consent'),
(11260, 0, 5, 'More work?', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - surprise'),
(11260, 0, 6, 'No one else available', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - complaint'),
(11260, 0, 7, 'No, not this again!', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - refusal'),
(11260, 0, 8, 'Off I go then', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - task accepted'),
(11260, 0, 9, 'Ready to work!', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - cheerful start'),
(11260, 0, 10, 'Right-o', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - quick agreement'),
(11260, 0, 11, 'What is it?', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - curious inquiry'),
(11260, 0, 12, 'What?', 12, 0, 100, 0, 0, 0, 0, 0, 'Northshire Peasant line - confused response');

DELETE FROM smart_scripts WHERE entryorguid = 11260 AND source_type = 0;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`action_param7`,`action_param_string`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_param_string`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES (11260,0,0,0,'',60,0,100,0,50000,150000,50000,150000,0,'',5,173,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Northshire Peasant - periodic working animation');
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`action_param7`,`action_param_string`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_param_string`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES (11260,0,1,0,'',60,0,100,0,50000,150000,50000,150000,0,'',1,0,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Northshire Peasant - periodic talk');
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`action_param7`,`action_param_string`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_param_string`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES (11260,0,2,0,'',60,0,100,0,1000,5000,50000,150000,0,'',5,12,0,0,0,0,0,0,NULL,1,0,0,0,0,NULL,0,0,0,0,'Northshire Peasant - periodic no animation');
