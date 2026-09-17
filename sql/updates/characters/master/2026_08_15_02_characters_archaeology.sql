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
