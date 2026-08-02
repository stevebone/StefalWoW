-- Hunter order-hall: fix the "I need to fly to the Trueshot Lodge" taxi destination.
--
-- Aludane Whitecloud (96813, Krasus' Landing) gossip menu 18723 option 2 = "I need to fly to the Trueshot Lodge"
-- fires smart_scripts id 8 (SMART_ACTION_TELEPORT). Its target was 4486.21, 4842.20, 662.01 - which is NESINGWARY'S
-- RETREAT (areaId 7733, z~662), NOT the Trueshot Lodge. The actual lodge is areaId 7877 at ~4627, 5338, 849 (where
-- Emmarel Shadewarden 102578 greets arrivals; you then follow her down the path for the Unseen Path backstory + oath
-- at the Visage of Ohn'ahra). So the taxi dumped hunters ~500 yd and ~180 yd of elevation short, at Nesingwary - the
-- reported "as soon as I arrived to turn in The Unseen Path I got ported to Nesingwary". Retarget to the lodge greeter.
UPDATE `smart_scripts`
   SET `target_x` = 4627.37, `target_y` = 5337.93, `target_z` = 849.254, `target_o` = 0.119857
 WHERE `entryorguid` = 96813 AND `source_type` = 0 AND `id` = 8 AND `action_type` = 62;
