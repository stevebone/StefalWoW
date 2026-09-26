-- Warband groups moved from the characters database to the auth database
-- (account_warband_groups / account_warband_group_members). This script migrates
-- existing rows and drops the obsolete per-realm tables.
--
-- The old tables were keyed by battlenetAccountId; the auth tables are keyed by
-- the game account id, so the migration fans out one shared group set to every
-- game account of the battle.net account.

-- When starting the client for the first time only the initial default / group will be present
-- The rest need to be made via the UI

-- Drop child table first, then the parent
DROP TABLE IF EXISTS `character_warband_group_members`;
DROP TABLE IF EXISTS `character_warband_groups`;
