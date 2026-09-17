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
