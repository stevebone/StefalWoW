--
-- Oribos, Ring of Transference - "Pathscribe Roh-Avonavi" transport gossip
-- ========================================================================
--
-- BUG
-- ---
-- Every "I am ready. Send me to <zone>." / "I need to get back to <zone>." gossip option on the five
-- Pathscribe Roh-Avonavi entries is inert: OptionNpc = 0, ActionMenuID = 0, ActionPoiID = 0, no
-- creature_template.ScriptName, no smart_scripts row. The client sends CMSG_GOSSIP_SELECT_OPTION and
-- the server has nothing bound to it, so the window just closes (or stays open) and nothing happens.
--
-- WHERE THESE NPCs ACTUALLY ARE
-- -----------------------------
-- All of them stand on the UPPER ring of Oribos (map 2222, z ~ 5451), AreaTable 13498 "Ring of
-- Transference" (child of 10565 "Oribos") - NOT in a covenant sanctum. Each one is the attendant of
-- one of the four zone gates:
--
--   creature 175133  (-1825.72, 1189.14, 5450.70)  next to GO 364419 "Gate to Bastion"      (-1834.21, 1169.30, 5454.70)
--   creature 175132  (-1759.98, 1220.90, 5451.01)  next to GO 364424 "Gate to Maldraxxus"   (-1753.99, 1202.41, 5450.53)
--   creature 175131  (-1930.70, 1274.95, 5451.00)  next to GO 364423 "Gate to Ardenweald"   (-1947.72, 1282.82, 5455.34)
--   creature 175134  ** NOT SPAWNED in this world DB **  (GO 364422 "Gate to Revendreth" IS spawned at -1834.27, 1396.27, 5455.15)
--   creature 162666  (-1907.14, 1210.30, 5451.59)  the Oribos flight master itself (TaxiNodes 2395 "Oribos" is at -1902.44, 1214.76, 5450.87)
--
-- WHAT THE OPTIONS ARE SUPPOSED TO DO - EVIDENCE
-- ---------------------------------------------
-- 1) They are quest objectives. quest_objectives (VerifiedBuild 66384) names them explicitly:
--       quest 60156 "The Path to Bastion"     obj 397716  Type 0  ObjectID 175133  "Activate the gateway to Bastion."
--       quest 60338 "Journey to Ardenweald"   obj 408765  Type 0  ObjectID 175131  "Speak to Roh-Avonavi"
--       quest 57386 "If You Want Peace..."    obj 396485  Type 0  ObjectID 175132  "Speak to Roh-Avonavi to Travel to Maldraxxus"
--       quest 57025 "A Plea to Revendreth"    obj 408766  Type 0  ObjectID 175134  "Speak to Roh-Avonavi to Travel to Revendreth"
--    Type 0 = QUEST_OBJECTIVE_MONSTER, so the option has to hand out kill credit for the NPC's own entry.
--
-- 2) The transport itself is a TAXI FLIGHT, not a teleport. Two independent pieces of client data say so:
--    a) quest 61475 "The Heart of the Forest" obj 409037 is Type 3 (QUEST_OBJECTIVE_TALKTO) on the Oribos
--       flight master 162666 and its authored description is literally "Flight taken to Ardenweald".
--    b) TaxiNodes.db2 / TaxiPath.db2 (12.0.7.68275) ship a purpose-built Oribos->zone path per gate, and
--       the source node of each one sits exactly on the corresponding attendant:
--
--       TaxiPath 7571  from node 2395 "Oribos"                        (-1902.44, 1214.76, 5450.87)
--                        to node 2397 "9.0, Zone, Bastion"            (-4251.68, -3886.17, 6564.23)   19 spline nodes
--       TaxiPath 7573  from node 2396 "TEMP, 9.0, Oribos, Ardenweald" (-1926.49, 1282.90, 5450.94)
--                        to node 2400 "TEMP, 9.0, Zone, Ardenweald"   (-5196.73,  -573.35, 5838.19)   16 spline nodes
--       TaxiPath 8325  from node 2394 "TEMP, 9.0, Oribos, Maldraxxus" (-1768.76, 1217.39, 5450.94)
--                        to node 2643 "Theater of Pain North, Maldraxxus" (3008.02, -2525.33, 3310.17) 26 spline nodes
--       TaxiPath 7574  from node 2395 "Oribos"
--                        to node 2399 "TEMP, 9.0, Zone, Revendreth"   (-3499.05,  5435.46, 4282.96)   18 spline nodes
--       TaxiPath 8318  from node 2395 "Oribos"
--                        to node 2564 "Theater of Pain, Maldraxxus"   ( 2580.47, -2520.76, 3307.52)   24 spline nodes
--
--    Every source node above carries MountCreatureID 164767 "Everwyrm" (display 94449), and every path
--    has Cost 0, so Player::ActivateTaxiPathTo() resolves a mount and charges nothing.
--
--    NO COORDINATE IS INVENTED HERE. SMART_ACTION_ACTIVATE_TAXI only takes a TaxiPath id; the whole
--    spline and the landing point come straight out of the client's own TaxiPath/TaxiPathNode data.
--
-- 3) The gate greetings confirm the direction of travel (broadcast_text via gossip_menu.TextID):
--       menu 26679 -> npc_text 42424 -> bcast 206704 "The ways are re-opening. Has the Purpose sent you to us for this I wonder?"
--       menu 26682 -> npc_text 42423 -> bcast 206705 "The way is open. You may now pass through to Maldraxxus."
--       menu 26678 -> npc_text 42425 -> bcast 206703 "The way is open. You may now pass through to Ardenweald."
--       menu 26683 -> npc_text 42401 -> bcast 206701 "The way is open. You may now pass through to Revendreth."
--       menu 26693 -> npc_text 42426 -> bcast 206878 "Welcome. Which path may I assist you with?"
--
-- NOT COVERED (nothing to source, reported instead of guessed)
-- -----------------------------------------------------------
--   * creature 175134 (Revendreth attendant) has no spawn in ANY world DB on this box, and no sniffed
--     position exists, so no creature row is added here. Its script is installed anyway so that the
--     option works the moment a sourced spawn is added.
--   * gossip_menu 26693 is missing its Ardenweald (OptionID 2) and Revendreth (OptionID 3) entries in
--     every world DB here. They are not invented.
--   * creature 180719 "Kyrian Courier" (Bastion, -1855.63 -2807.40 6744.79) has the same dead-option
--     problem on menu 27237 "Take me to Elysian Hold.". The obvious client path (TaxiPath 8592, 26 nodes,
--     starting on top of the courier and ending at TaxiNodes 2628 "[Hidden] 9.0 Bastion Ground Hub") has
--     FromTaxiNode = 0 / ToTaxiNode = 0, which Player::ActivateTaxiPathTo() rejects, so it is NOT wired.
--

-- --------------------------------------------------------------------------------------------------
-- The four gate attendants need SmartAI so that SmartAI::OnGossipSelect() is reached at all.
-- (162666 is already AIName = 'SmartAI'.)
-- --------------------------------------------------------------------------------------------------
UPDATE `creature_template` SET `AIName` = 'SmartAI'
 WHERE `entry` IN (175131, 175132, 175133, 175134)
   AND (`AIName` IS NULL OR `AIName` = '')
   AND (`ScriptName` IS NULL OR `ScriptName` = '');

DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryorguid` IN (162666, 175131, 175132, 175133, 175134);
INSERT INTO `smart_scripts`
(`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,
 `event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,
 `action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`action_param7`,`action_param_string`,
 `target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_param_string`,
 `target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES

-- ---------------- 175133 - Bastion gate attendant (menu 26679) ----------------
(175133,0,0,1,'',62,0,100,0,26679,0,0,0,0,'',72,0,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Pathscribe Roh-Avonavi (Bastion gate) - On Gossip Option 0 Selected - Close Gossip'),
(175133,0,1,2,'',61,0,100,0,0,0,0,0,0,'',33,175133,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Pathscribe Roh-Avonavi (Bastion gate) - On Link - Quest Credit "Activate the gateway to Bastion." (quest 60156 obj 397716)'),
(175133,0,2,0,'',61,0,100,0,0,0,0,0,0,'',52,7571,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Pathscribe Roh-Avonavi (Bastion gate) - On Link - Activate Taxi 7571 (Oribos 2395 -> "9.0, Zone, Bastion" 2397)'),
(175133,0,3,4,'',62,0,100,0,26679,1,0,0,0,'',72,0,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Pathscribe Roh-Avonavi (Bastion gate) - On Gossip Option 1 Selected - Close Gossip'),
(175133,0,4,0,'',61,0,100,0,0,0,0,0,0,'',52,7573,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Pathscribe Roh-Avonavi (Bastion gate) - On Link - Activate Taxi 7573 (Oribos 2396 -> "TEMP, 9.0, Zone, Ardenweald" 2400)'),

-- ---------------- 175131 - Ardenweald gate attendant (menu 26678) ----------------
(175131,0,0,1,'',62,0,100,0,26678,0,0,0,0,'',72,0,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Pathscribe Roh-Avonavi (Ardenweald gate) - On Gossip Option 0 Selected - Close Gossip'),
(175131,0,1,2,'',61,0,100,0,0,0,0,0,0,'',33,175131,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Pathscribe Roh-Avonavi (Ardenweald gate) - On Link - Quest Credit "Speak to Roh-Avonavi" (quest 60338 obj 408765)'),
(175131,0,2,0,'',61,0,100,0,0,0,0,0,0,'',52,7573,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Pathscribe Roh-Avonavi (Ardenweald gate) - On Link - Activate Taxi 7573 (Oribos 2396 -> "TEMP, 9.0, Zone, Ardenweald" 2400)'),

-- ---------------- 175132 - Maldraxxus gate attendant (menu 26682) ----------------
(175132,0,0,1,'',62,0,100,0,26682,0,0,0,0,'',72,0,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Pathscribe Roh-Avonavi (Maldraxxus gate) - On Gossip Option 0 Selected - Close Gossip'),
(175132,0,1,2,'',61,0,100,0,0,0,0,0,0,'',33,175132,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Pathscribe Roh-Avonavi (Maldraxxus gate) - On Link - Quest Credit "Speak to Roh-Avonavi to Travel to Maldraxxus" (quest 57386 obj 396485)'),
(175132,0,2,0,'',61,0,100,0,0,0,0,0,0,'',52,8325,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Pathscribe Roh-Avonavi (Maldraxxus gate) - On Link - Activate Taxi 8325 (Oribos 2394 -> "Theater of Pain North, Maldraxxus" 2643)'),

-- ---------------- 175134 - Revendreth gate attendant (menu 26683) - template only, NPC is not spawned ----------------
(175134,0,0,1,'',62,0,100,0,26683,0,0,0,0,'',72,0,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Pathscribe Roh-Avonavi (Revendreth gate) - On Gossip Option 0 Selected - Close Gossip'),
(175134,0,1,2,'',61,0,100,0,0,0,0,0,0,'',33,175134,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Pathscribe Roh-Avonavi (Revendreth gate) - On Link - Quest Credit "Speak to Roh-Avonavi to Travel to Revendreth" (quest 57025 obj 408766)'),
(175134,0,2,0,'',61,0,100,0,0,0,0,0,0,'',52,7574,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Pathscribe Roh-Avonavi (Revendreth gate) - On Link - Activate Taxi 7574 (Oribos 2395 -> "TEMP, 9.0, Zone, Revendreth" 2399)'),

-- ---------------- 162666 - Oribos flight master (menu 26693 "I need to get back to ...") ----------------
-- These are the return shortcuts. They are not quest objectives themselves, so they only fly.
(162666,0,0,1,'',62,0,100,0,26693,0,0,0,0,'',72,0,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Pathscribe Roh-Avonavi (flight master) - On Gossip Option 0 Selected - Close Gossip'),
(162666,0,1,0,'',61,0,100,0,0,0,0,0,0,'',52,7571,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Pathscribe Roh-Avonavi (flight master) - On Link - Activate Taxi 7571 (Oribos 2395 -> "9.0, Zone, Bastion" 2397)'),
(162666,0,2,3,'',62,0,100,0,26693,1,0,0,0,'',72,0,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Pathscribe Roh-Avonavi (flight master) - On Gossip Option 1 Selected - Close Gossip'),
(162666,0,3,0,'',61,0,100,0,0,0,0,0,0,'',52,8318,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Pathscribe Roh-Avonavi (flight master) - On Link - Activate Taxi 8318 (Oribos 2395 -> "Theater of Pain, Maldraxxus" 2564)'),
-- QUEST_OBJECTIVE_TALKTO credit. TC never calls Player::TalkedToCreature() from the gossip handler
-- (NPCHandler.cpp:187 is commented out), so without this the two TalkTo objectives on this entry can
-- never complete: quest 61475 "The Heart of the Forest" obj 409037 "Flight taken to Ardenweald" and
-- quest 60491 "Among the Kyrian" obj 411595.
(162666,0,4,0,'',64,0,100,0,0,0,0,0,0,'',153,0,0,0,0,0,0,0,'',7,0,0,0,0,'',0,0,0,0,'Pathscribe Roh-Avonavi (flight master) - On Gossip Hello - Credit Quest Objective Talk To');
