-- BfA War Campaign: fix Grand Admiral Jes-Tereth (135681) foothold transport
--
-- Symptom: selecting "Set sail for Zuldazar/Nazmir/Vol'dun" (or "Take us back to Boralus")
--          in her gossip does nothing.
-- Root cause: her transport is driven by SmartAI SMART_EVENT_GOSSIP_SELECT (62) -> TELEPORT (62),
--          but the existing 4 rows listen on gossip menu 900037 (a PHANTOM menu that does not
--          exist and that she is not assigned). Her real foothold menu is 22508 with transport
--          options 7=Vol'dun, 8=Nazmir, 9=Zuldazar, 10=Boralus. The old rows also carried
--          placeholder ("ExilReach"/"Ankunft") coordinates. So no handler ever matched the
--          option the player selected.
--
-- Fix: replace the phantom rows with correct GOSSIP_SELECT(menu 22508, option) -> TELEPORT rows,
--      teleporting the invoking player to the verified Alliance foothold camps on map 1642
--      (Zandalar) / Boralus (map 1643). Landing coords taken from the live populated camps:
--        Zuldazar  = Fort Victory        (Priestess Islara cluster)
--        Nazmir    = Alliance Nazmir camp (Caregiver Mila cluster)
--        Vol'dun   = Alliance Vol'dun camp (Halford/Thaelin cluster)
--        Boralus   = the war-campaign ship deck (Jes-Tereth's own spawn)

DELETE FROM `smart_scripts` WHERE `entryorguid`=135681 AND `source_type`=0 AND `id` IN (1,2,3,4);
INSERT INTO `smart_scripts`
(`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`action_param7`,`action_param_string`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_param_string`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES
(135681,0,1,0,'',62,0,100,0,22508,9,0,0,0,'',62,1642,0,0,0,0,0,0,'',7,0,0,0,0,'',2098.6,191.8,4.9,3.6,'Jes-Tereth - Gossip select Set sail for Zuldazar - Teleport player to Fort Victory (Zuldazar foothold)'),
(135681,0,2,0,'',62,0,100,0,22508,8,0,0,0,'',62,1642,0,0,0,0,0,0,'',7,0,0,0,0,'',2738.0,4177.0,10.0,0.5,'Jes-Tereth - Gossip select Set sail for Nazmir - Teleport player to Nazmir foothold camp'),
(135681,0,3,0,'',62,0,100,0,22508,7,0,0,0,'',62,1642,0,0,0,0,0,0,'',7,0,0,0,0,'',-2610.0,2270.0,14.0,1.5,'Jes-Tereth - Gossip select Set sail for Voldun - Teleport player to Voldun foothold camp'),
(135681,0,4,0,'',62,0,100,0,22508,10,0,0,0,'',62,1643,0,0,0,0,0,0,'',7,0,0,0,0,'',1014.0,-485.5,20.0,3.0,'Jes-Tereth - Gossip select Take us back to Boralus - Teleport player to Boralus war-campaign ship');
