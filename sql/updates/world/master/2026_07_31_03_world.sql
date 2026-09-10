--
-- Bind all 12 Legion class order hall unlock quests to the class-agnostic QuestScript 'quest_class_order_hall'.
-- Completing one of a class's unlock quests creates that player's faction class hall (Alliance 161 / Horde 163)
-- and recruits the class's champions (idempotent). Quest ids per class (from the class order-hall questlines;
-- champion GarrFollower ids live in the ClassOrderHalls table in garrison_generic.cpp):
--   Warrior 42598 | Paladin 39696 | Hunter 40954/40955 | Rogue 42139 | Priest 43270 | Death Knight 43264
--   Shaman 42383 | Mage 42663 | Warlock 40823/42608 | Monk 42187 | Druid 42583 | Demon Hunter 42671/42670
--
INSERT INTO `quest_template_addon` (`ID`, `ScriptName`) VALUES
 (42598, 'quest_class_order_hall'),
 (39696, 'quest_class_order_hall'),
 (40954, 'quest_class_order_hall'),
 (40955, 'quest_class_order_hall'),
 (42139, 'quest_class_order_hall'),
 (43270, 'quest_class_order_hall'),
 (43264, 'quest_class_order_hall'),
 (42383, 'quest_class_order_hall'),
 (42663, 'quest_class_order_hall'),
 (40823, 'quest_class_order_hall'),
 (42608, 'quest_class_order_hall'),
 (42187, 'quest_class_order_hall'),
 (42583, 'quest_class_order_hall'),
 (42671, 'quest_class_order_hall'),
 (42670, 'quest_class_order_hall')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);
