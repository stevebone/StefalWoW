--
-- Titanstrike scenario - move the completion trigger from Warlord Volund's death to CLAIMING Titanstrike (clicking
-- the Titan Chest 249718), which is the retail beat: reach Titanstrike -> claim it -> Prustaga betrays + snatches it
-- -> Mimiron teleports you to the Creator's Workshop. Volund reverts to a plain hostile boss (no script). The chest's
-- go_titanstrike GameObjectAI completes objective 2 + fires the betrayal emote + the workshop transfer.
--
UPDATE `creature_template` SET `ScriptName`='' WHERE `entry`=104956;                 -- Warlord Volund: plain boss now
UPDATE `gameobject_template` SET `ScriptName`='go_titanstrike' WHERE `entry`=249718;  -- Titan Chest = the trigger
