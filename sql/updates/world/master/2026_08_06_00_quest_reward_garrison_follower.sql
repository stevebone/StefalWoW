-- Quest -> Garrison/War-Campaign champion (GarrFollower) reward mapping
--
-- Retail grants several BfA War Campaign champions when a "Champion:"/recruitment quest is turned in.
-- TrinityCore's quest_template has no follower-reward field, and although these quests carry the
-- follower-granting spell as RewardSpell (SPELL_EFFECT_ADD_GARRISON_FOLLOWER), Spell::EffectAddGarrisonFollower
-- historically routed to the WoD garrison (type 2) and failed for GarrType 9 followers. This table is the
-- authoritative, faction-agnostic mechanism: Player::RewardQuest looks the quest up here and calls
-- Garrison::AddFollower on the correct GarrType garrison (creating the war-campaign garrison if missing).
--
-- GarrType: 9 = War Campaign (matches Garrison.h GARRISON_TYPE_WAR_CAMPAIGN).
-- GarrFollowerID values are from GarrFollower.db2 (build 12.0.7.68275), GarrTypeID=9. Each row here is
-- justified by the quest's own RewardSpell in quest_template resolving (via SpellEffect.db2 Effect=220,
-- EffectMiscValue0) to the named champion follower record.

DROP TABLE IF EXISTS `quest_reward_garrison_follower`;
CREATE TABLE `quest_reward_garrison_follower` (
    `QuestID` INT UNSIGNED NOT NULL COMMENT 'quest_template.ID whose turn-in grants the champion',
    `GarrFollowerID` INT UNSIGNED NOT NULL COMMENT 'GarrFollower.db2 id to grant',
    `GarrType` TINYINT UNSIGNED NOT NULL DEFAULT 9 COMMENT 'GarrisonType (9 = War Campaign)',
    PRIMARY KEY (`QuestID`, `GarrFollowerID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='Quest turn-in -> GarrFollower champion grants';

DELETE FROM `quest_reward_garrison_follower` WHERE `QuestID` IN (51714,51753,51770,51975,51987,52003,52008,52013,52861,53098,56378,56379);
INSERT INTO `quest_reward_garrison_follower` (`QuestID`, `GarrFollowerID`, `GarrType`) VALUES
-- Falstad Wildhammer (A) / Arcanist Valtrois (H) -- shared follower record 1065
(51714, 1065, 9),  -- "Mission from the King"      RewardSpell 273814 (Follower: Falstad Wildhammer)
(51770, 1065, 9),  -- "Mission from the Warchief"  RewardSpell 273816 (Follower: Arcanist Valtrois)
-- Rexxar (H) / John J. Keeshan (A) -- shared follower record 1069
(51753, 1069, 9),  -- "Champion: Rexxar"            RewardSpell 273025 (Follower: Rexxar)
(52013, 1069, 9),  -- "Champion: John J. Keeshan"   RewardSpell 273973 (Follower: John J. Keeshan)
-- Shadow Hunter Ty'jin (H) / Magister Umbric (A) -- shared follower record 1072
(51975, 1072, 9),  -- "Champion: Shadow Hunter Ty'jin" RewardSpell 273931 (Follower: Shadow Hunter Ty'jin)
(52008, 1072, 9),  -- "Champion: Magister Umbric"      RewardSpell 273972 (Follower: Magister Umbric)
-- Hobart Grapplehammer (H) / Kelsey Steelspark (A) -- shared follower record 1068
(51987, 1068, 9),  -- "Champion: Hobart Grapplehammer" RewardSpell 273971 (Follower: Hobart Grapplehammer)
(52003, 1068, 9),  -- "Champion: Kelsey Steelspark"    RewardSpell 274013 (Follower: Kelsey Steelspark)
-- Lilian Voss (H) / Shandris Feathermoon (A) -- shared follower record 1062
(52861, 1062, 9),  -- "Champion: Lilian Voss"          RewardSpell 274015 (Follower: Lilian Voss)
(53098, 1062, 9),  -- "Champion: Shandris Feathermoon" RewardSpell 278996 (Follower: Shandris Feathermoon)
-- Grand Admiral Jes-Tereth (A) / Dread-Admiral Tattersail (H) -- shared follower record 1182 (8.2 Nazjatar)
(56378, 1182, 9),  -- "The Missing Crew"               RewardSpell 301140 (Follower: Captain)
(56379, 1182, 9);  -- "The Missing Crew"               RewardSpell 301140 (Follower: Captain)
