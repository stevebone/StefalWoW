-- Beast Mastery Hunter artifact ("Stolen Thunder" 41574 -> Titanstrike) scenario wiring.
-- Drives TrinityCore InstanceScenario 1068 "Thunder of the Titans" (map 1495) and the follow-up
-- "The Creator's Workshop" (42158, map 1579). See zone_orderhall_hunter.cpp.

-- Scenario 1068 director: Prustaga (the tomb ally) reads the running scenario's current step and fires each step's
-- game event once a player reaches that step's landmark, advancing the scenario to completion.
UPDATE `creature_template` SET `ScriptName` = 'npc_prustaga_scenario_director' WHERE `entry` = 104949;

-- Scenario 1068 step 4 "Volund's Last Stand": Warlord Volund's death fires the "Defeat Warlord Volund" game event.
UPDATE `creature_template` SET `ScriptName` = 'npc_warlord_volund' WHERE `entry` = 104956;

-- Leg 2 "The Creator's Workshop" (42158): grant the "Mimiron Assisted" credit (106559) on accept.
UPDATE `quest_template_addon` SET `ScriptName` = 'quest_creators_workshop' WHERE `ID` = 42158;

-- Retire the stale Titan Chest binding: the chest is scenery (type-5); the scenario now handles Titanstrike via the
-- authored steps (defeat Volund + Relay Device pad), and the go_titanstrike script has been removed.
UPDATE `gameobject_template` SET `ScriptName` = '' WHERE `entry` = 249718 AND `ScriptName` = 'go_titanstrike';
