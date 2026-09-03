--
-- Generalize the class order hall unlock trigger. The order-hall QuestScript was generalized from a
-- Hunter-specific script (quest_hunter_order_hall) to a class-agnostic, faction-aware one (quest_class_order_hall)
-- that reads a per-class data table (class -> establish quest + champion quest + champion GarrFollower ids) and
-- creates the shared faction class-hall site (Alliance 161 / Horde 163). Rebind Hunter's establish/champion quests
-- ("The Unseen Path" 40954, "Oath of Service" 40955) to the new script. As each remaining class is added to the
-- table, bind its two quests to 'quest_class_order_hall' here as well.
--
UPDATE `quest_template_addon` SET `ScriptName` = 'quest_class_order_hall'
 WHERE `ID` IN (40954, 40955) AND `ScriptName` = 'quest_hunter_order_hall';
