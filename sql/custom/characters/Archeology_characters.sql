--
-- Archaeology: per-character active dig sites.
-- Persists the ActivePlayer ResearchSites / ResearchSiteProgress update fields across relog/restart,
-- plus the site's current hidden find position so a relog cannot relocate an in-progress find.
-- Ported from evry/master-track/archaeology 94eadb810a (findX/findY added by cb60da9643).
--
CREATE TABLE IF NOT EXISTS `character_research_site` (
  `guid` bigint unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `researchSiteId` smallint unsigned NOT NULL DEFAULT '0',
  `progress` int unsigned NOT NULL DEFAULT '0',
  `findX` float NOT NULL DEFAULT '0' COMMENT 'Current hidden find world X',
  `findY` float NOT NULL DEFAULT '0' COMMENT 'Current hidden find world Y',
  PRIMARY KEY (`guid`,`researchSiteId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Archaeology active dig sites per character';

--
-- Archaeology: persist active research projects.
-- One row per active research branch = the branch's current in-progress project (ResearchProject.db2
-- ID). The branch is derivable from the project, so only the project id is stored. Mirrors
-- `character_research_site`. Restored into ActivePlayerData.Research on login.
-- Ported from evry/master-track/archaeology e621450284.
--
CREATE TABLE IF NOT EXISTS `character_research_project` (
  `guid` bigint unsigned NOT NULL COMMENT 'Character GUID',
  `projectId` int unsigned NOT NULL COMMENT 'ResearchProject.db2 ID (current project for its branch)',
  PRIMARY KEY (`guid`,`projectId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Archaeology active research projects per character';

--
-- Archaeology: persist completed research projects.
-- One row per solved project = ActivePlayerData.ResearchHistory.CompletedProjects. Used to bias new
-- project rolls away from repeats and to show completion counts. Restored on login.
-- Ported from evry/master-track/archaeology 0932638917.
--
CREATE TABLE IF NOT EXISTS `character_research_history` (
  `guid` bigint unsigned NOT NULL COMMENT 'Character GUID',
  `projectId` int unsigned NOT NULL COMMENT 'ResearchProject.db2 ID',
  `firstCompleted` bigint NOT NULL DEFAULT '0' COMMENT 'Unix time of the first completion',
  `completionCount` int unsigned NOT NULL DEFAULT '1' COMMENT 'Times this project has been solved',
  PRIMARY KEY (`guid`,`projectId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Archaeology completed research projects per character';
