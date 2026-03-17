-- ============================================================================
-- Enchant/Gem Template Cleanup
-- These tables are no longer needed — enchant and gem data is now loaded
-- directly from DB2 client data stores (SpellItemEnchantment, GemProperties)
-- at server startup. See EnchantGemDatabase::LoadEnchantsFromDB2().
-- ============================================================================

DROP TABLE IF EXISTS `playerbot_enchant_recommendations`;
DROP TABLE IF EXISTS `playerbot_gem_recommendations`;
