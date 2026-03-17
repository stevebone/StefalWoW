#ifndef PLAYERBOT_AUCTION_CONFIG_H
#define PLAYERBOT_AUCTION_CONFIG_H

/*
 * Configuration helper for Playerbot Auction System
 *
 * Add these lines to playerbots.conf:
 *
 * ###################################################################################################
 * # AUCTION HOUSE ECONOMY SYSTEM
 * ###################################################################################################
 *
 * # Enable bot participation in auction house
 * #     Default: 0 (disabled)
 * Playerbot.Auction.Enable = 0
 *
 * # Update interval for auction operations (milliseconds)
 * #     Default: 60000 (1 minute)
 * Playerbot.Auction.UpdateInterval = 60000
 *
 * # Maximum concurrent active auctions per bot
 * #     Default: 10
 * Playerbot.Auction.MaxActiveAuctions = 10
 *
 * # Minimum profit threshold for auction creation (copper)
 * #     Default: 10000 (1 gold)
 * Playerbot.Auction.MinProfit = 10000
 *
 * # Default auction strategy (0-5)
 * #     0 = CONSERVATIVE (1% undercut)
 * #     1 = AGGRESSIVE (5-10% undercut)
 * #     2 = PREMIUM (list at market average)
 * #     3 = QUICK_SALE (20% undercut)
 * #     4 = MARKET_MAKER (buy low, sell high)
 * #     5 = SMART_PRICING (AI-driven adaptive pricing)
 * #     Default: 5
 * Playerbot.Auction.DefaultStrategy = 5
 *
 * # Enable commodity trading (WoW 12.0 region-wide market)
 * #     Default: 1 (enabled)
 * Playerbot.Auction.CommodityEnabled = 1
 *
 * # Enable market maker behavior (buy underpriced, relist higher)
 * #     Default: 0 (disabled - can be disruptive to server economy)
 * Playerbot.Auction.MarketMakerEnabled = 0
 *
 * # Market scan interval (milliseconds)
 * #     How often to analyze auction house prices
 * #     Default: 300000 (5 minutes)
 * Playerbot.Auction.MarketScanInterval = 300000
 *
 * # Maximum risk score for flip opportunities (0-100)
 * #     Lower = safer, higher = more aggressive
 * #     Default: 50
 * Playerbot.Auction.MaxRiskScore = 50
 *
 * # Default undercut percentage for standard strategies
 * #     Default: 2.0 (2%)
 * Playerbot.Auction.UndercutPercentage = 2.0
 *
 * # Price history tracking duration (days)
 * #     Default: 7
 * Playerbot.Auction.PriceHistoryDays = 7
 *
 * ###################################################################################################
 * # AUCTION STRATEGY RECOMMENDATIONS
 * ###################################################################################################
 *
 * # CONSERVATIVE (0): Best for high-value items, stable profits
 * # AGGRESSIVE (1): Best for high-volume items, fast turnover
 * # PREMIUM (2): Best for rare items, patient sellers
 * # QUICK_SALE (3): Best for bag space management, immediate gold needs
 * # MARKET_MAKER (4): Best for experienced economy manipulation (use carefully)
 * # SMART_PRICING (5): Best for general use, adapts to market conditions
 *
 * ###################################################################################################
 * # PERFORMANCE CONSIDERATIONS
 * ###################################################################################################
 *
 * # - Each bot auction scan costs ~0.001% CPU
 * # - Market scans every 5 minutes minimize database load
 * # - Price history limited to 7 days for memory efficiency
 * # - Throttle system prevents auction house spam
 * # - Maximum 10 active auctions per bot prevents market flooding
 *
 * ###################################################################################################
 * # ECONOMY BALANCE
 * ###################################################################################################
 *
 * # Disable MarketMakerEnabled on live servers to prevent:
 * # - Market manipulation
 * # - Price inflation
 * # - Unfair competition with real players
 *
 * # Enable MarketMakerEnabled only on:
 * # - Single-player servers
 * # - Development/testing environments
 * # - Servers with economy reset policies
 *
 * ###################################################################################################
 */

#endif // PLAYERBOT_AUCTION_CONFIG_H
