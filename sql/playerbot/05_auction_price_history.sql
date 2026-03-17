-- Playerbot Auction Price History Tables
-- Optional: Persistent price tracking for market analysis

-- Price history table for trend analysis
CREATE TABLE IF NOT EXISTS `playerbot_auction_price_history` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `item_id` INT UNSIGNED NOT NULL,
    `price` BIGINT UNSIGNED NOT NULL,
    `timestamp` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_item_timestamp` (`item_id`, `timestamp`),
    KEY `idx_timestamp` (`timestamp`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Tracks historical auction prices for bot market analysis';

-- Bot auction statistics
CREATE TABLE IF NOT EXISTS `playerbot_auction_stats` (
    `bot_guid` BIGINT UNSIGNED NOT NULL,
    `total_auctions_created` INT UNSIGNED NOT NULL DEFAULT 0,
    `total_auctions_sold` INT UNSIGNED NOT NULL DEFAULT 0,
    `total_auctions_cancelled` INT UNSIGNED NOT NULL DEFAULT 0,
    `total_auctions_expired` INT UNSIGNED NOT NULL DEFAULT 0,
    `total_bids_placed` INT UNSIGNED NOT NULL DEFAULT 0,
    `total_commodities_bought` INT UNSIGNED NOT NULL DEFAULT 0,
    `total_gold_spent` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `total_gold_earned` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `net_profit` BIGINT NOT NULL DEFAULT 0,
    `success_rate` FLOAT NOT NULL DEFAULT 0.0,
    `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`bot_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Tracks auction house statistics per bot';

-- Active bot auctions for tracking
CREATE TABLE IF NOT EXISTS `playerbot_active_auctions` (
    `auction_id` INT UNSIGNED NOT NULL,
    `bot_guid` BIGINT UNSIGNED NOT NULL,
    `item_id` INT UNSIGNED NOT NULL,
    `item_count` INT UNSIGNED NOT NULL DEFAULT 1,
    `start_price` BIGINT UNSIGNED NOT NULL,
    `buyout_price` BIGINT UNSIGNED NOT NULL,
    `cost_basis` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `listed_time` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `expiry_time` TIMESTAMP NOT NULL,
    `is_commodity` TINYINT(1) NOT NULL DEFAULT 0,
    `strategy` TINYINT UNSIGNED NOT NULL DEFAULT 5,
    PRIMARY KEY (`auction_id`),
    KEY `idx_bot_guid` (`bot_guid`),
    KEY `idx_item_id` (`item_id`),
    KEY `idx_expiry` (`expiry_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Tracks active bot auctions for profit calculation';

-- Market condition cache
CREATE TABLE IF NOT EXISTS `playerbot_market_cache` (
    `item_id` INT UNSIGNED NOT NULL,
    `current_price` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `average_price_7d` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `median_price_7d` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `min_price_7d` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `max_price_7d` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `daily_volume` INT UNSIGNED NOT NULL DEFAULT 0,
    `active_listings` INT UNSIGNED NOT NULL DEFAULT 0,
    `price_trend` FLOAT NOT NULL DEFAULT 0.0,
    `market_condition` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`item_id`),
    KEY `idx_last_update` (`last_update`),
    KEY `idx_condition` (`market_condition`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Caches market analysis data for bot decision making';

-- Cleanup stored procedure for old price history
DELIMITER $$
CREATE PROCEDURE `sp_cleanup_auction_price_history`(IN days_to_keep INT)
BEGIN
    DELETE FROM `playerbot_auction_price_history`
    WHERE `timestamp` < DATE_SUB(NOW(), INTERVAL days_to_keep DAY);

    SELECT ROW_COUNT() AS rows_deleted;
END$$
DELIMITER ;

-- Event to auto-cleanup price history (runs daily)
CREATE EVENT IF NOT EXISTS `evt_cleanup_auction_price_history`
ON SCHEDULE EVERY 1 DAY
STARTS CURRENT_TIMESTAMP
DO CALL sp_cleanup_auction_price_history(7);

-- Indexes for performance
ALTER TABLE `playerbot_auction_price_history`
    ADD INDEX `idx_item_price` (`item_id`, `price`),
    ADD INDEX `idx_recent` (`timestamp` DESC);

-- Example queries for market analysis:

-- Get price trend for an item
/*
SELECT
    item_id,
    AVG(price) as avg_price,
    MIN(price) as min_price,
    MAX(price) as max_price,
    COUNT(*) as sample_count,
    STDDEV(price) as price_volatility
FROM playerbot_auction_price_history
WHERE item_id = 12345
  AND timestamp >= DATE_SUB(NOW(), INTERVAL 7 DAY)
GROUP BY item_id;
*/

-- Get top performing bot traders
/*
SELECT
    bot_guid,
    total_auctions_sold,
    total_gold_earned,
    net_profit,
    success_rate,
    (total_gold_earned - total_gold_spent) as total_profit
FROM playerbot_auction_stats
WHERE total_auctions_created > 10
ORDER BY net_profit DESC
LIMIT 10;
*/

-- Find profitable items (high volume, good margins)
/*
SELECT
    m.item_id,
    m.current_price,
    m.median_price_7d,
    m.daily_volume,
    m.price_trend,
    COUNT(DISTINCT h.timestamp) as price_samples
FROM playerbot_market_cache m
LEFT JOIN playerbot_auction_price_history h ON h.item_id = m.item_id
WHERE m.daily_volume > 5
  AND m.price_trend > 0
  AND m.current_price < m.median_price_7d * 0.9
GROUP BY m.item_id
ORDER BY m.daily_volume DESC, m.price_trend DESC
LIMIT 20;
*/
