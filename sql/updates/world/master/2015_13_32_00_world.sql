--
-- Add npcTeamMemberID and creatureId columns to battle_pet_npc_team
-- npcTeamMemberID: maps to BattlePetNPCTeamMember.db2 for NPC pet name display
-- creatureId: optional override for creature model (0 = use species default)
--

-- Use conditional column add to be idempotent (safe to re-run)
SET @col_exists = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
    WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'battle_pet_npc_team' AND COLUMN_NAME = 'npcTeamMemberID');

SET @sql = IF(@col_exists = 0,
    'ALTER TABLE `battle_pet_npc_team`
        ADD COLUMN `npcTeamMemberID` INT NOT NULL DEFAULT 0 COMMENT ''BattlePetNPCTeamMember.db2 ID for pet name'' AFTER `ability3`,
        ADD COLUMN `creatureId` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT ''Creature template entry override for model (0=species default)'' AFTER `npcTeamMemberID`',
    'SELECT 1');

PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
