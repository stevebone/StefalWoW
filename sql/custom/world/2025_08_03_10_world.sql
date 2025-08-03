-- NPC: 54130 Amberleaf Scamp
-- NPC: 54131 Fe-Feng Hozen
-- NPC: 57164 Fe-Feng Leaper


-- Remove SmartAI from 54130 since there is scripting in place
UPDATE `creature_template` SET `AIName` = '' WHERE `Entry` = 54130;
-- Added smart scripts missing for 54131 and 57164
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (54131,57164);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('54131', '0', '0', '0', '11', '0', '100', '0', '0', '0', '0', '0', '0', '', '8', '1', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Hozen - On Respawn - Set React State'),
('54131', '0', '1', '0', '4', '0', '100', '0', '0', '0', '0', '0', '0', '', '41', '0', '0', '0', '0', '0', '0', '11', '56739', '1', '0', '0', '0', '0', '0', 'Fe-Feng Hozen - On Aggro - Despawn Vehicle'),
('54131', '0', '2', '0', '7', '0', '100', '0', '0', '0', '0', '0', '0', '', '41', '2000', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Hozen - On Evade - Despawn'),
('54131', '0', '3', '0', '0', '0', '100', '0', '2000', '8000', '4000', '8000', '0', '', '11', '121484', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Hozen - IC - Cast Ook\'em'),
('54131', '0', '4', '0', '0', '0', '50', '0', '5000', '10000', '5000', '10000', '0', '', '11', '115006', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Hozen - IC - Cast Hozen Rage'),
('57164', '0', '0', '0', '4', '0', '100', '0', '0', '0', '0', '0', '0', '', '11', '107526', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Leaper - On Aggro - Cast Reckless Leap');
