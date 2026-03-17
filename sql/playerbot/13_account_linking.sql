-- ============================================================================
-- Account Linking System for Playerbot
-- Links human player accounts with bot accounts for permission-based access
-- ============================================================================

DROP TABLE IF EXISTS `playerbot_account_links`;
CREATE TABLE `playerbot_account_links` (
    `link_id`            MEDIUMINT UNSIGNED NOT NULL,
    `owner_account_id`   INT UNSIGNED NOT NULL COMMENT 'Human player account ID',
    `bot_account_id`     INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Bot account ID (0 = guid-based link)',
    `bot_guid`           BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Specific bot character GUID (0 = all on account)',
    `permissions`        SMALLINT UNSIGNED NOT NULL DEFAULT 0x0067 COMMENT 'Bitmask: 1=view_inv,2=trade,4=control,8=guild,16=friends,32=summon,64=dismiss',
    `created_time`       INT UNSIGNED NOT NULL DEFAULT 0,
    `last_access_time`   INT UNSIGNED NOT NULL DEFAULT 0,
    `active`             TINYINT(1) NOT NULL DEFAULT 1,
    PRIMARY KEY (`link_id`),
    KEY `idx_owner` (`owner_account_id`),
    KEY `idx_bot_account` (`bot_account_id`),
    KEY `idx_bot_guid` (`bot_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Playerbot account-to-account links with permissions';
