--
-- WoD Shipyard unlock trigger. Bind the terminal "All Hands on Deck" quest of the "Garrison Campaign: War Council"
-- questline (Alliance 38259 / Horde 38574) to the QuestScript that builds the shipyard on turn-in. Retail casts the
-- reward spell (186007 / 185915) at this point; we hook the quest's REWARDED status so the trigger is independent of
-- that spell's effect layout. Garrison::CreateShipyard re-checks the Tier-3 prerequisite.
--
INSERT INTO `quest_template_addon` (`ID`, `ScriptName`) VALUES
 (38259, 'quest_garrison_shipyard_intro'),
 (38574, 'quest_garrison_shipyard_intro')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);
