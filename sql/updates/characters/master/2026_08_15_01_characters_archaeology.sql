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
