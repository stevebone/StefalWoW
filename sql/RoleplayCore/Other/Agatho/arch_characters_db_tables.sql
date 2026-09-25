
CREATE TABLE IF NOT EXISTS `character_research_site` (
  `guid` bigint unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `researchSiteId` smallint unsigned NOT NULL DEFAULT '0',
  `progress` int unsigned NOT NULL DEFAULT '0',
  `findX` float NOT NULL DEFAULT '0' COMMENT 'Current hidden find world X',
  `findY` float NOT NULL DEFAULT '0' COMMENT 'Current hidden find world Y',
  PRIMARY KEY (`guid`,`researchSiteId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Archaeology active dig sites per character';

CREATE TABLE IF NOT EXISTS `character_research_project` (
  `guid` bigint unsigned NOT NULL COMMENT 'Character GUID',
  `projectId` int unsigned NOT NULL COMMENT 'ResearchProject.db2 ID (current project for its branch)',
  PRIMARY KEY (`guid`,`projectId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Archaeology active research projects per character';

CREATE TABLE IF NOT EXISTS `character_research_history` (
  `guid` bigint unsigned NOT NULL COMMENT 'Character GUID',
  `projectId` int unsigned NOT NULL COMMENT 'ResearchProject.db2 ID',
  `firstCompleted` bigint NOT NULL DEFAULT '0' COMMENT 'Unix time of the first completion',
  `completionCount` int unsigned NOT NULL DEFAULT '1' COMMENT 'Times this project has been solved',
  PRIMARY KEY (`guid`,`projectId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Archaeology completed research projects per character';
