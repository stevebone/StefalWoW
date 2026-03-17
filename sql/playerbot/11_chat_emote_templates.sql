-- ============================================================================
-- Chat and Emote Template Tables for Playerbot
-- These tables store contextual chat messages and emotes that bots use
-- to appear more human-like.
-- ============================================================================

DROP TABLE IF EXISTS `playerbot_chat_templates`;
CREATE TABLE `playerbot_chat_templates` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `trigger_context` VARCHAR(50) NOT NULL COMMENT 'Trigger context: combat_start, death, greeting, etc.',
    `chat_type` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=SAY, 1=YELL, 2=PARTY, 3=RAID, 4=GUILD, 5=WHISPER, 6=EMOTE',
    `message_text` VARCHAR(255) NOT NULL COMMENT 'Message text with {variable} placeholders',
    `filter_class` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=any, 1-13=specific class',
    `filter_race` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=any, specific race ID',
    `filter_min_level` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=no minimum',
    `filter_max_level` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=no maximum',
    `weight` INT UNSIGNED NOT NULL DEFAULT 100 COMMENT 'Selection weight (higher=more likely)',
    `cooldown_ms` INT UNSIGNED NOT NULL DEFAULT 30000 COMMENT 'Per-bot cooldown in ms',
    `locale` VARCHAR(4) NOT NULL DEFAULT '' COMMENT 'Locale filter: empty=default, deDE, frFR, etc.',
    PRIMARY KEY (`id`),
    KEY `idx_context` (`trigger_context`),
    KEY `idx_class` (`filter_class`),
    KEY `idx_locale` (`locale`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Bot contextual chat message templates';

DROP TABLE IF EXISTS `playerbot_emote_templates`;
CREATE TABLE `playerbot_emote_templates` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `trigger_context` VARCHAR(50) NOT NULL COMMENT 'Trigger context: greeting, idle_emote, etc.',
    `emote_id` INT UNSIGNED NOT NULL COMMENT 'TrinityCore Emote enum value',
    `filter_class` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=any, 1-13=specific class',
    `filter_race` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=any, specific race ID',
    `weight` INT UNSIGNED NOT NULL DEFAULT 100 COMMENT 'Selection weight',
    `cooldown_ms` INT UNSIGNED NOT NULL DEFAULT 15000 COMMENT 'Per-bot cooldown in ms',
    PRIMARY KEY (`id`),
    KEY `idx_context` (`trigger_context`),
    KEY `idx_class` (`filter_class`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Bot contextual emote templates';

-- ============================================================================
-- Default Data: Combat Messages
-- ============================================================================

INSERT INTO `playerbot_chat_templates` (`trigger_context`, `chat_type`, `message_text`, `weight`, `cooldown_ms`) VALUES
-- Combat start
('combat_start', 0, 'Let''s do this!', 100, 120000),
('combat_start', 0, 'Engaging!', 80, 120000),
('combat_start', 2, 'Watch my back!', 60, 120000),
('combat_start', 0, 'Stay focused!', 70, 120000),
('combat_start', 0, 'Here we go!', 60, 120000),

-- Combat kill
('combat_kill', 0, 'Down!', 100, 60000),
('combat_kill', 0, 'One less to worry about.', 60, 60000),
('combat_kill', 0, 'Next!', 50, 60000),
('combat_kill', 0, 'That was close.', 40, 60000),

-- Combat death
('combat_death', 2, 'I need a rez!', 100, 30000),
('combat_death', 2, 'I''m down!', 80, 30000),
('combat_death', 2, 'Someone help!', 60, 30000),

-- Low health
('low_health', 2, 'I need healing!', 100, 15000),
('low_health', 1, 'Help!', 40, 30000),
('low_health', 2, 'Heals please!', 80, 15000),

-- Low mana / OOM
('low_mana', 2, 'Running low on mana.', 100, 30000),
('oom', 2, 'OOM!', 100, 20000),
('oom', 2, 'I''m out of mana!', 80, 20000),
('oom', 2, 'Need to drink!', 60, 20000);

-- ============================================================================
-- Default Data: Social Messages
-- ============================================================================

INSERT INTO `playerbot_chat_templates` (`trigger_context`, `chat_type`, `message_text`, `weight`, `cooldown_ms`) VALUES
-- Greeting
('greeting', 0, 'Hey {target}!', 100, 300000),
('greeting', 0, 'Hello!', 80, 300000),
('greeting', 0, 'Good to see you, {target}.', 60, 300000),
('greeting', 0, 'What''s up?', 50, 300000),

-- Farewell
('farewell', 0, 'See you around, {target}!', 100, 300000),
('farewell', 0, 'Take care!', 80, 300000),
('farewell', 0, 'Later!', 60, 300000),
('farewell', 0, 'Gotta go, catch you later!', 50, 300000),

-- Thank you
('thank_you', 0, 'Thanks, {target}!', 100, 60000),
('thank_you', 0, 'Appreciate it!', 80, 60000),
('thank_you', 0, 'Nice one, thanks!', 60, 60000),

-- Loot
('loot_epic', 2, 'Wow, epic drop!', 100, 30000),
('loot_rare', 0, 'Nice, a rare!', 80, 60000),

-- Ready check
('ready_check', 2, 'Ready!', 100, 10000),
('ready_check', 2, 'Good to go.', 80, 10000),
('ready_check', 2, 'All set!', 70, 10000),

-- Buff request
('buff_request', 2, 'Can I get a buff?', 100, 120000),
('buff_request', 2, 'Buffs would be nice.', 60, 120000),

-- Res request
('res_request', 2, 'Rez please!', 100, 30000),
('res_request', 2, 'Can someone rez me?', 80, 30000),

-- Group join/leave
('group_join', 2, 'Hey everyone!', 100, 300000),
('group_join', 2, 'Ready to go!', 80, 300000),
('group_leave', 2, 'Gotta go, thanks for the group!', 100, 300000),
('group_leave', 2, 'Later all, was fun!', 80, 300000);

-- ============================================================================
-- Default Data: Class-specific Messages
-- ============================================================================

INSERT INTO `playerbot_chat_templates` (`trigger_context`, `chat_type`, `message_text`, `filter_class`, `weight`, `cooldown_ms`) VALUES
-- Warrior (class 1)
('combat_start', 1, 'CHARGE!', 1, 50, 180000),
('low_health', 2, 'I can hold them!', 1, 40, 30000),

-- Paladin (class 2)
('combat_start', 0, 'The Light protects us!', 2, 50, 180000),
('low_health', 2, 'Light, give me strength!', 2, 40, 30000),

-- Hunter (class 3)
('combat_start', 0, 'My pet''s got this!', 3, 40, 180000),

-- Priest (class 5)
('oom', 2, 'I need to drink, hold on.', 5, 80, 20000),
('low_health', 2, 'Can''t heal myself and the group!', 5, 40, 30000),

-- Death Knight (class 6)
('combat_start', 0, 'Suffer well.', 6, 50, 180000),

-- Shaman (class 7)
('combat_start', 0, 'The elements guide us.', 7, 50, 180000),

-- Mage (class 8)
('oom', 2, 'Need a mana break!', 8, 80, 20000),

-- Druid (class 11)
('combat_start', 0, 'Nature''s wrath!', 11, 50, 180000);

-- ============================================================================
-- Default Data: Emote Templates
-- ============================================================================

INSERT INTO `playerbot_emote_templates` (`trigger_context`, `emote_id`, `weight`, `cooldown_ms`) VALUES
-- Greeting
('greeting', 3, 100, 60000),     -- WAVE
('greeting', 2, 60, 60000),      -- BOW

-- Farewell
('farewell', 3, 100, 60000),     -- WAVE
('farewell', 2, 40, 60000),      -- BOW

-- Thank you
('thank_you', 77, 100, 30000),   -- THANKS
('thank_you', 2, 60, 30000),     -- BOW

-- Combat start
('combat_start', 15, 60, 120000), -- ROAR
('combat_start', 71, 40, 120000), -- CHEER

-- Combat kill
('combat_kill', 71, 80, 60000),   -- CHEER
('combat_kill', 10, 40, 60000),   -- DANCE

-- Death
('combat_death', 18, 100, 30000), -- CRY

-- Idle
('idle_emote', 10, 60, 300000),   -- DANCE
('idle_emote', 8, 80, 300000),    -- SIT
('idle_emote', 11, 40, 300000),   -- LAUGH
('idle_emote', 3, 30, 300000),    -- WAVE

-- City
('city_emote', 10, 60, 180000),   -- DANCE
('city_emote', 8, 80, 180000),    -- SIT
('city_emote', 16, 40, 180000),   -- SLEEP
('city_emote', 11, 30, 180000);   -- LAUGH
