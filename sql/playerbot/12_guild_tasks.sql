-- ============================================================================
-- Guild Task Templates for Playerbot GuildTaskManager
-- ============================================================================

DROP TABLE IF EXISTS `playerbot_guild_task_templates`;
CREATE TABLE `playerbot_guild_task_templates` (
    `template_id`          MEDIUMINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `task_type`            TINYINT UNSIGNED NOT NULL COMMENT '0=KILL,1=GATHER,2=CRAFT,3=FISH,4=MINE,5=HERB,6=SKIN,7=DUNGEON,8=DELIVER,9=SCOUT',
    `difficulty`           TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '0=EASY,1=NORMAL,2=HARD,3=ELITE',
    `title_format`         VARCHAR(128) NOT NULL DEFAULT '',
    `description_format`   VARCHAR(512) NOT NULL DEFAULT '',
    `target_entry`         MEDIUMINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Creature/item/node entry (0=any)',
    `min_count`            SMALLINT UNSIGNED NOT NULL DEFAULT 1,
    `max_count`            SMALLINT UNSIGNED NOT NULL DEFAULT 10,
    `required_level`       TINYINT UNSIGNED NOT NULL DEFAULT 1,
    `required_skill`       SMALLINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Profession skill ID (0=none)',
    `required_skill_value` SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    `zone_id`              MEDIUMINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Target zone (0=any)',
    `base_gold_reward`     MEDIUMINT UNSIGNED NOT NULL DEFAULT 200 COMMENT 'Per-unit gold reward in copper',
    `base_rep_reward`      SMALLINT UNSIGNED NOT NULL DEFAULT 3 COMMENT 'Per-unit reputation reward',
    `duration_hours`       SMALLINT UNSIGNED NOT NULL DEFAULT 24 COMMENT 'Task lifetime in hours',
    `weight`               FLOAT NOT NULL DEFAULT 1.0 COMMENT 'Selection weight for random generation',
    PRIMARY KEY (`template_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Playerbot guild task generation templates';

-- ============================================================================
-- Default Task Templates
-- ============================================================================

-- KILL tasks (type=0) - any class
INSERT INTO `playerbot_guild_task_templates` (`task_type`, `difficulty`, `title_format`, `description_format`, `min_count`, `max_count`, `required_level`, `base_gold_reward`, `base_rep_reward`, `duration_hours`, `weight`) VALUES
(0, 0, 'Pest Control',           'Eliminate hostile creatures near guild territory.',             5, 15, 10, 200, 3, 24, 2.0),
(0, 1, 'Bounty Hunt',            'Track and eliminate dangerous creatures.',                    10, 30, 30, 500, 5, 24, 1.5),
(0, 2, 'Elite Extermination',    'Slay elite creatures threatening our lands.',                  3,  8, 60, 1000, 10, 48, 0.8);

-- GATHER tasks (type=1)
INSERT INTO `playerbot_guild_task_templates` (`task_type`, `difficulty`, `title_format`, `description_format`, `min_count`, `max_count`, `required_level`, `base_gold_reward`, `base_rep_reward`, `duration_hours`, `weight`) VALUES
(1, 0, 'Supply Run',             'Gather general supplies for the guild.',                       5, 20,  5, 150, 2, 24, 1.5);

-- FISH tasks (type=3) - requires fishing skill (356)
INSERT INTO `playerbot_guild_task_templates` (`task_type`, `difficulty`, `title_format`, `description_format`, `min_count`, `max_count`, `required_level`, `required_skill`, `required_skill_value`, `base_gold_reward`, `base_rep_reward`, `duration_hours`, `weight`) VALUES
(3, 0, 'Gone Fishing',           'Catch fish for the guild feast.',                              5, 15, 10, 356,   1, 200, 3, 24, 1.0),
(3, 1, 'Deep Sea Fishing',       'Catch rare fish from challenging waters.',                    10, 25, 40, 356, 200, 500, 5, 48, 0.7);

-- MINE tasks (type=4) - requires mining skill (186)
INSERT INTO `playerbot_guild_task_templates` (`task_type`, `difficulty`, `title_format`, `description_format`, `min_count`, `max_count`, `required_level`, `required_skill`, `required_skill_value`, `base_gold_reward`, `base_rep_reward`, `duration_hours`, `weight`) VALUES
(4, 0, 'Ore Collection',         'Mine ore for guild crafters.',                                 5, 15, 10, 186,   1, 200, 3, 24, 1.0),
(4, 1, 'Deep Mining Expedition',  'Mine rare minerals from deep veins.',                        10, 20, 40, 186, 200, 500, 5, 48, 0.7);

-- HERB tasks (type=5) - requires herbalism skill (182)
INSERT INTO `playerbot_guild_task_templates` (`task_type`, `difficulty`, `title_format`, `description_format`, `min_count`, `max_count`, `required_level`, `required_skill`, `required_skill_value`, `base_gold_reward`, `base_rep_reward`, `duration_hours`, `weight`) VALUES
(5, 0, 'Herb Gathering',         'Pick herbs for the guild alchemist.',                          5, 15, 10, 182,   1, 200, 3, 24, 1.0),
(5, 1, 'Rare Herb Expedition',   'Gather rare herbs from dangerous areas.',                     10, 20, 40, 182, 200, 500, 5, 48, 0.7);

-- SKIN tasks (type=6) - requires skinning skill (393)
INSERT INTO `playerbot_guild_task_templates` (`task_type`, `difficulty`, `title_format`, `description_format`, `min_count`, `max_count`, `required_level`, `required_skill`, `required_skill_value`, `base_gold_reward`, `base_rep_reward`, `duration_hours`, `weight`) VALUES
(6, 0, 'Leather Procurement',    'Skin creatures for guild leatherworkers.',                     5, 15, 10, 393,   1, 200, 3, 24, 0.8);

-- CRAFT tasks (type=2)
INSERT INTO `playerbot_guild_task_templates` (`task_type`, `difficulty`, `title_format`, `description_format`, `min_count`, `max_count`, `required_level`, `base_gold_reward`, `base_rep_reward`, `duration_hours`, `weight`) VALUES
(2, 1, 'Crafting Commission',    'Craft items for the guild.',                                   3,  8, 30, 500, 5, 48, 0.6);

-- DUNGEON tasks (type=7)
INSERT INTO `playerbot_guild_task_templates` (`task_type`, `difficulty`, `title_format`, `description_format`, `min_count`, `max_count`, `required_level`, `base_gold_reward`, `base_rep_reward`, `duration_hours`, `weight`) VALUES
(7, 2, 'Dungeon Expedition',     'Clear a dungeon for guild prestige.',                          1,  1, 15, 2000, 15, 72, 0.5),
(7, 3, 'Heroic Dungeon Challenge','Complete a heroic dungeon run.',                              1,  1, 70, 5000, 30, 72, 0.3);

-- DELIVER tasks (type=8)
INSERT INTO `playerbot_guild_task_templates` (`task_type`, `difficulty`, `title_format`, `description_format`, `min_count`, `max_count`, `required_level`, `base_gold_reward`, `base_rep_reward`, `duration_hours`, `weight`) VALUES
(8, 0, 'Guild Bank Deposit',     'Deposit gold into the guild bank.',                            1,  1, 10, 0, 5, 24, 1.2);

-- SCOUT tasks (type=9)
INSERT INTO `playerbot_guild_task_templates` (`task_type`, `difficulty`, `title_format`, `description_format`, `min_count`, `max_count`, `required_level`, `base_gold_reward`, `base_rep_reward`, `duration_hours`, `weight`) VALUES
(9, 0, 'Zone Patrol',            'Explore and patrol a zone.',                                   1,  1, 10, 300, 3, 24, 0.8),
(9, 1, 'Contested Territory Scout','Scout enemy-controlled zones.',                              1,  1, 40, 600, 5, 48, 0.5);
