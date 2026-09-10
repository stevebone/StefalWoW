--
-- Abomination Factory (Necrolord unique covenant sanctum feature, GarrTalentTree 321).
--
-- One row per construct body the character has stitched. `recipeSpellId` is the "Construct Body: X" recipe
-- (a SkillLineAbility.Spell of SkillLine 2787 "Abominable Stitching") that produced it - the 15 of them are
-- identified by the server from client data as the stitching recipes whose spell carries
-- SPELL_EFFECT_KILL_CREDIT (creature credit 167076 / 167581), so nothing here hardcodes an id list.
--
-- The client's own roster of the same 15 is CriteriaTree 88195 "Abominable Stitching - Build Abom":
--   Chordy 325284, Roseboil 325451, Marz 325452, Flytrap 325453, Atticus 325454, Miru 325458, Neena 326379,
--   Gas Bag 326380, Professor 326406, Toothpick 326407, Mama Tomalin 326408, Iron Phillip 338037,
--   Guillotine 338039, Sabrina 338040, Naxx 338043.
--
-- Only the covenant sanctum (GarrType 111) has one, so the table is keyed by guid alone and is written and
-- purged from Garrison::SaveToDB / Garrison::DeleteFromDB behind a GARRISON_TYPE_COVENANT check.
--
-- Idempotent.
--
CREATE TABLE IF NOT EXISTS `character_garrison_abomination_factory` (
  `guid`          BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Character GUID',
  `recipeSpellId` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'SkillLineAbility.Spell of the construct-body recipe',
  `builtTime`     BIGINT NOT NULL DEFAULT 0 COMMENT 'unix time the construct was stitched',
  PRIMARY KEY (`guid`, `recipeSpellId`),
  KEY `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Abomination Factory construct stable';
