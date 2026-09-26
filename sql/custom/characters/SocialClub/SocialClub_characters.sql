-- ============================================================================
-- SocialClub system - character database tables
-- Ported from RolePlay_Master (Club finder parts 1+2, stream history, guild rename)
--
-- Replaces the previous simplified club finder implementation:
-- `club_finder_post` / `club_finder_applicant` (incompatible schema, dropped).
-- ============================================================================

-- Guild flag field (GUILD_FLAG_RENAME etc.)
ALTER TABLE `guild` ADD `flags` INT(11) NOT NULL DEFAULT 0 AFTER `leaderguid`;

-- ---------------------------------------------------------------------------
-- Club Finder (guild recruitment)
-- ---------------------------------------------------------------------------

DROP TABLE IF EXISTS `club_finder_applicant`;
DROP TABLE IF EXISTS `club_finder_post`;
DROP TABLE IF EXISTS `club_finder_application`;
DROP TABLE IF EXISTS `club_finder_posting`;

CREATE TABLE `club_finder_posting` (
    `postingId`           INT UNSIGNED    NOT NULL,
    `clubId`              BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `name`                VARCHAR(96)     NOT NULL DEFAULT '',
    `description`         TEXT            NULL,
    `recruitingSpecs`     BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `recruitmentFlags`    INT UNSIGNED    NOT NULL DEFAULT 0,
    `itemLevelRequirement` INT UNSIGNED   NOT NULL DEFAULT 0,
    `avatarId`            INT UNSIGNED    NOT NULL DEFAULT 0,
    `displayFlags`        INT UNSIGNED    NOT NULL DEFAULT 0,
    `type`                TINYINT UNSIGNED NOT NULL DEFAULT 1,
    `crossFaction`        TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `lastPosterGuid`      BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `lastUpdatedTime`     BIGINT          NOT NULL DEFAULT 0,
    PRIMARY KEY (`postingId`),
    UNIQUE KEY `idx_clubId` (`clubId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `club_finder_application` (
    `postingId`         INT UNSIGNED    NOT NULL,
    `playerGuid`        BIGINT UNSIGNED NOT NULL,
    `comment`           TEXT            NULL,
    `specs`             BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `status`            TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'PlayerClubRequestStatus: 1=Pending, 2=AutoApproved, 3=Declined, 4=Approved, 5=Joined, 6=JoinedAnother, 7=Canceled',
    `lastUpdatedTime`   BIGINT          NOT NULL DEFAULT 0,
    PRIMARY KEY (`postingId`, `playerGuid`),
    KEY `idx_playerGuid` (`playerGuid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ---------------------------------------------------------------------------
-- Guild rename history (paid rename + refund bookkeeping)
-- ---------------------------------------------------------------------------

DROP TABLE IF EXISTS `guild_rename`;
CREATE TABLE `guild_rename` (
  `guildid` bigint unsigned NOT NULL DEFAULT '0',
  `previousName` varchar(24) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `costPaid` bigint unsigned NOT NULL DEFAULT '0',
  `renameTime` bigint unsigned NOT NULL DEFAULT '0',
  `refunded` tinyint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guildid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Guild Rename History';

-- ---------------------------------------------------------------------------
-- Club stream history (guild chat scrollback, read markers, mentions)
-- ---------------------------------------------------------------------------

DROP TABLE IF EXISTS `club_message`;
DROP TABLE IF EXISTS `club_stream_view_marker`;
DROP TABLE IF EXISTS `club_mention_view_marker`;
DROP TABLE IF EXISTS `club_member_mention`;

CREATE TABLE `club_message` (
    `clubId`           BIGINT UNSIGNED NOT NULL,
    `streamId`         BIGINT UNSIGNED NOT NULL,
    `epoch`            BIGINT UNSIGNED NOT NULL COMMENT 'MessageId.epoch - microseconds since unix epoch',
    `position`         BIGINT UNSIGNED NOT NULL COMMENT 'MessageId.position - monotonic per (club, stream)',
    `authorAccountId`  INT UNSIGNED    NOT NULL DEFAULT 0,
    `authorGuid`       BIGINT UNSIGNED NOT NULL,
    `content`          TEXT            NOT NULL,
    `createdTime`      BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'unix seconds, for age based retention',
    PRIMARY KEY (`clubId`, `streamId`, `epoch`, `position`),
    KEY `idx_createdTime` (`createdTime`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `club_stream_view_marker` (
    `clubId`       BIGINT UNSIGNED NOT NULL,
    `streamId`     BIGINT UNSIGNED NOT NULL,
    `memberGuid`   BIGINT UNSIGNED NOT NULL,
    `lastViewTime` BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'microseconds since unix epoch',
    PRIMARY KEY (`clubId`, `streamId`, `memberGuid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `club_mention_view_marker` (
    `memberGuid`   BIGINT UNSIGNED NOT NULL,
    `lastViewTime` BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'microseconds since unix epoch',
    PRIMARY KEY (`memberGuid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `club_member_mention` (
    `clubId`          BIGINT UNSIGNED NOT NULL,
    `streamId`        BIGINT UNSIGNED NOT NULL,
    `memberGuid`      BIGINT UNSIGNED NOT NULL COMMENT 'the mentioned member',
    `epoch`           BIGINT UNSIGNED NOT NULL,
    `position`        BIGINT UNSIGNED NOT NULL,
    `authorGuid`      BIGINT UNSIGNED NOT NULL,
    `authorAccountId` INT UNSIGNED    NOT NULL DEFAULT 0,
    `createdTime`     BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`memberGuid`, `epoch`, `position`),
    KEY `idx_createdTime` (`createdTime`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
