-- FSB Tables

DROP TABLE if exists followship_bot_owners;
CREATE TABLE followship_bot_owners (
    bot_id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    bot_guid BIGINT UNSIGNED NOT NULL, -- spawnId
    bot_entry INT UNSIGNED NOT NULL,
    player_guid BIGINT UNSIGNED NOT NULL,
    hire_expiry_time BIGINT UNSIGNED NOT NULL,

    UNIQUE KEY uq_bot_player (bot_guid, player_guid)
);