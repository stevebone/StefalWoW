-- Covenant switching / reset (spell 338503 "Reset Covenant").
--
-- A switch must never cost a character anything belonging to a covenant it may return to. Renown (1829-1832),
-- reservoir anima (1859-1862), researched sanctum talents (character_garrison_talents - every covenant-scoped
-- GarrTalentTree names its owner in FeatureSubtypeIndex, so the four covenants already own disjoint rows),
-- companions (character_garrison_followers), conduits, conduit sockets, the granted-renown high-water mark and
-- the per-covenant calling boards are all already stored per covenant and are left untouched by a switch.
--
-- The one piece of covenant-scoped state that had nowhere to live is WHICH SOULBIND a covenant was using:
-- character_covenant is single-valued by design (it holds the ACTIVE covenant/soulbind), so leaving a covenant
-- would have thrown its soulbind choice away. This table remembers it per covenant, and because a row is written
-- for every covenant the character pledges to - even before it picks a soulbind - it doubles as the "covenants
-- ever joined" set that tells a switch apart from a first pledge.
--
-- Idempotent: safe to re-run.

CREATE TABLE IF NOT EXISTS `character_covenant_soulbind` (
  `guid` bigint unsigned NOT NULL DEFAULT '0',
  `covenantId` int unsigned NOT NULL DEFAULT '0',
  `soulbindId` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`covenantId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Soulbind last active per covenant (also the set of covenants ever joined)';

-- Seed the characters that already belong to a covenant, so their current pledge is recognised as "joined" and
-- their current soulbind survives their first switch. INSERT IGNORE keeps a re-run from overwriting anything the
-- server has since recorded.
INSERT IGNORE INTO `character_covenant_soulbind` (`guid`, `covenantId`, `soulbindId`)
  SELECT `guid`, `covenantId`, `soulbindId` FROM `character_covenant` WHERE `covenantId` <> 0;
