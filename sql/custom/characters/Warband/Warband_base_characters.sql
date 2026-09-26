-- Warband groups moved from the characters database to the auth database
-- (account_warband_groups / account_warband_group_members). This script migrates
-- existing rows and drops the obsolete per-realm tables.
--
-- The old tables were keyed by battlenetAccountId; the auth tables are keyed by
-- the game account id, so the migration fans out one shared group set to every
-- game account of the battle.net account.
--
-- If your auth database is not named `auth`, edit the schema qualifier below.
-- On multi-realm installs run this once per characters schema.

-- Groups: duplicate the shared battle.net group set onto every game account.
INSERT IGNORE INTO `auth`.`account_warband_groups` (id, accountId, realmId, orderIndex, name, warbandSceneId, flags)
SELECT g.groupId, a.id, 1, g.orderIndex, g.name, g.warbandSceneId, g.flags
FROM `character_warband_groups` g
JOIN `auth`.`account` a ON a.battlenet_account = g.battlenetAccountId;

-- Members: each membership belongs to the game account that owns the character;
-- memberIndex and contentSetId have no counterpart in the auth schema.
INSERT IGNORE INTO `auth`.`account_warband_group_members` (accountId, realmId, groupId, characterGuid, placementId, type)
SELECT c.account, 1, m.groupId, m.guid, m.warbandScenePlacementId, m.memberType
FROM `character_warband_group_members` m
JOIN `characters` c ON c.guid = m.guid
JOIN `character_warband_groups` g ON g.groupId = m.groupId;

-- Drop child table first, then the parent
DROP TABLE IF EXISTS `character_warband_group_members`;
DROP TABLE IF EXISTS `character_warband_groups`;
