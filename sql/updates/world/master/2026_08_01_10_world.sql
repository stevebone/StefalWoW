--
-- Hunter Beast Mastery artifact "Stolen Thunder" (41574): wire the missing Grif Wildheart -> Shield's Rest flight.
-- The quest sends the Hunter to Warlord Volund's tomb on Shield's Rest (populated scenario sub-map 1495), reached in
-- retail via a scripted Grif/Huey flight whose kill-credit (creature 104993) completes objective 1. That flight is
-- absent from our world DB, stranding the quest in Dalaran. Bind Grif (the 41574 quest giver 104381 + the nearby
-- 106879) to a script that credits the flight objective and transports the player to the Shield's Rest landing while
-- the flight leg is still outstanding. Add the GOSSIP npcflag (|1) so clicking Grif opens the interaction (a
-- questgiver-only flag does nothing once the quest is in progress) which fires the script's OnGossipHello.
--
UPDATE `creature_template` SET `ScriptName`='npc_grif_wildheart_flight', `npcflag`=`npcflag`|1 WHERE `entry` IN (104381, 106879);
