--
-- Hunter Beast Mastery artifact "Stolen Thunder" (41574): wire the missing Grif Wildheart -> Shield's Rest flight.
-- The quest sends the Hunter to Warlord Volund's tomb on Shield's Rest (populated scenario sub-map 1495), reached in
-- retail via a scripted Grif/Huey flight whose kill-credit (creature 104993) completes objective 1. That flight is
-- absent from our world DB, stranding the quest in Dalaran. Bind Grif (106879) to a script that credits the flight
-- objective and transports the player to the Shield's Rest landing while the flight leg is still outstanding.
--
UPDATE `creature_template` SET `ScriptName`='npc_grif_wildheart_flight' WHERE `entry`=106879;
