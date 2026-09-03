--
-- Titanstrike scenario (Warlord Volund's tomb, map 1495) - Phase 2: make the tomb traversable.
-- The tomb doors (GAMEOBJECT_TYPE_DOOR, type 0) all spawn closed (state 1), and the retail "Trial of the
-- Truthseeker" that would open them room-by-room cannot be reconstructed here - the trial champions (Asgrim the
-- Dreadkiller, Hakkap One-leg) are not spawned and no scenario framework exists for this map (incomplete import).
-- Open the tomb doors (state 0) so the player can advance through the tomb and fight the now-hostile enemies to
-- reach Warlord Volund and claim Titanstrike. Map 1495 is only entered by players on the Titanstrike quest.
--
UPDATE `gameobject` g JOIN `gameobject_template` gt ON gt.`entry`=g.`id`
SET g.`state`=0
WHERE g.`map`=1495 AND gt.`type`=0;

-- Phase 3: defeating Warlord Volund completes "Stolen Thunder" (objective 2, Titanstrike criteria) for the killer.
UPDATE `creature_template` SET `ScriptName`='npc_warlord_volund' WHERE `entry`=104956;
