-- Priest Holy artifact "T'uure, Beacon of the Naaru" (quest 42074 "Return of the Light", Niskara map 1489).
-- The quest exists with 3 objectives (106076 portal / 106033 stage / 106031 Holy-claim) and a spawned giver (Jace
-- Darkweaver 106011) + ender (Prophet Velen 101313), but had no boss/encounter, so the T'uure acquisition was only a
-- class-hall talk stand-in (41957). Bind the acquisition scripts. No new spawns: Lady Calindris is TRANSIENTLY summoned
-- on quest accept (map 1489 is densely shared with the Demon Hunter Niskara scenario, so a permanent spawn would
-- contaminate it).

-- Bind the QuestScript (portal to Niskara + summon Calindris on accept) and the Lady Calindris boss AI.
UPDATE `quest_template_addon` SET `ScriptName` = 'quest_return_of_the_light' WHERE `ID` = 42074;
UPDATE `creature_template` SET `ScriptName` = 'npc_lady_calindris' WHERE `entry` = 106318;
