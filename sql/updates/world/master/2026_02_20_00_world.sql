--
-- Pet Battle NPC Team table
-- Defines pet teams for NPC pet battle trainers
--

DROP TABLE IF EXISTS `battle_pet_npc_team`;
CREATE TABLE `battle_pet_npc_team` (
    `npcEntry` INT UNSIGNED NOT NULL COMMENT 'Creature template entry',
    `slot` TINYINT UNSIGNED NOT NULL COMMENT 'Team slot (0-2)',
    `speciesId` INT UNSIGNED NOT NULL COMMENT 'BattlePetSpecies.db2 ID',
    `level` SMALLINT UNSIGNED NOT NULL DEFAULT 1,
    `breedId` SMALLINT UNSIGNED NOT NULL DEFAULT 3 COMMENT 'BattlePetBreedState breed ID',
    `quality` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=Poor, 1=Common, 2=Uncommon, 3=Rare',
    `ability1` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'BattlePetAbility.db2 ID for slot 0',
    `ability2` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'BattlePetAbility.db2 ID for slot 1',
    `ability3` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'BattlePetAbility.db2 ID for slot 2',
    PRIMARY KEY (`npcEntry`, `slot`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Pet Battle NPC Trainer Teams';
