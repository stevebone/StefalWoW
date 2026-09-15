--
-- Legion Hunter Order Hall (Trueshot Lodge) unlock trigger. Bind the class-hall establishment quest "The Unseen
-- Path" (40954) and the champion-recruitment quest "Oath of Service" (40955) of the Hunter "Unseen Path" intro
-- chain to the QuestScript that creates the GarrType-3 order hall (GarrSite 161) and recruits its champions on
-- turn-in. Retail casts SPELL_EFFECT_CREATE_GARRISON / follower-grant spells at these points; we hook the quests'
-- REWARDED status so the trigger is independent of that (offline-unavailable) spell-effect layout.
--   40954 The Unseen Path -> create the hall + recruit Emmarel Shadewarden (leader).
--   40955 Oath of Service -> recruit the remaining starting champions + seed the mission board.
--
INSERT INTO `quest_template_addon` (`ID`, `ScriptName`) VALUES
 (40954, 'quest_hunter_order_hall'),
 (40955, 'quest_hunter_order_hall')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);
