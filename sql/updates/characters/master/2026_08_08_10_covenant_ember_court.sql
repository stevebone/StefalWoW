--
-- The Ember Court (Venthyr unique covenant sanctum feature, GarrTalentTree 324).
--
-- One row per guest this character has any standing with - hosted at least once, or currently on the guest
-- list for the next court.
--
-- `guestIndex` is 0-15 and is the client's OWN guest ordering: the sixteen children of CriteriaTree 87983,
-- which is Achievement 14723 "Be Our Guest" ("Host the following guests at your Ember Court"), in their
-- OrderIndex order. That roster is derived twice over - the identical sixteen names also appear as the
-- sixteen "RSVP: <Guest>" quests already present in the world DB, each started and ended by that guest's own
-- creature - so it lives in C++ (EmberCourt.cpp) rather than in a world table:
--     0 Baroness Vashj        4 Alexandros Mograine    8 Choofa                 12 Kleia and Pelagos
--     1 Lady Moonberry        5 Hunt-Captain Korayn     9 Cryptkeeper Kassir     13 Plague Deviser Marileth
--     2 Mikanikos             6 Polemarch Adrestes     10 Droman Aliothe         14 Sika
--     3 The Countess          7 Rendle and Cudgelface  11 Grandmaster Vole       15 Stonehead
-- A stored row whose guestIndex is outside 0-15 is dropped with an error at load (EmberCourt::LoadFromDB).
--
-- `invited` is the pending guest list for the NEXT court. It is capped at the slots the researched talents of
-- GarrTalentTree 324 grant - 2 at the bottom of the ladder, 3 with talent 1114 "Court Influencer" ("allowing
-- you to invite a THIRD guest"), 4 with talent 1112 "Discerning Taste" ("a FOURTH guest") - and the 60s
-- Garrison::Update tick trims it back down if a talent reset or a covenant switch shrinks the allowance, or
-- if a guest's "RSVP: <Guest>" quest no longer stands.
--
-- `highestMood` is a high-water mark on the mood scale the 12.0.7.68275 client does NOT publish. Achievement
-- 14724 "People Pleaser" names exactly one rung of that scale, "Elated", and nothing anywhere gives the rungs
-- below it or the thresholds between them ("Elated" is not even a GlobalStrings row). The value is therefore
-- only ever WRITTEN by a real court completion reporting it; the server never computes one. The rung that
-- counts as Elated for each guest is authored per guest in the world table `garrison_ember_court_guest`.
--
-- `courtsHeld` and `lastCourtTime` are court-level counters denormalised onto every guest row so the feature
-- keeps to a single table. They are identical across a character's rows. A court can only be completed with
-- at least one guest on the list, so a character with courtsHeld > 0 always has at least one row to carry
-- them. NOTE: no minimum interval between courts is enforced anywhere - the build publishes none (spell
-- 336617 is named "Ember Court Timer" but no interval this server could apply is published), and one is not
-- invented. lastCourtTime is recorded and exposed so whoever authors the venue can gate on it.
--
-- Only the covenant sanctum (GarrType 111) has one, so the table is keyed by guid alone and is written and
-- purged from Garrison::SaveToDB / Garrison::DeleteFromDB behind a GARRISON_TYPE_COVENANT check - the same
-- shape as `character_garrison_conservatory`, `character_garrison_abomination_factory` and
-- `character_garrison_path_of_ascension`.
--
-- Hosting history is deliberately NOT deleted when the covenant changes or a talent is reset: the character
-- keeps what it earned and simply loses access until it is Venthyr with the tree researched again. Only the
-- pending `invited` flags are trimmed.
--
-- Idempotent.
--
CREATE TABLE IF NOT EXISTS `character_garrison_ember_court` (
  `guid`           BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Character GUID',
  `guestIndex`     TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0-15, CriteriaTree 87983 child OrderIndex',
  `timesHosted`    INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'courts this guest has attended',
  `highestMood`    TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'high-water mark on the unpublished mood scale; 0 = never hosted',
  `lastHostedTime` BIGINT NOT NULL DEFAULT 0 COMMENT 'unix time this guest last attended',
  `invited`        TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'on the guest list for the next court',
  `courtsHeld`     INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'court-level counter, identical across this guid''s rows',
  `lastCourtTime`  BIGINT NOT NULL DEFAULT 0 COMMENT 'court-level counter, identical across this guid''s rows',
  PRIMARY KEY (`guid`, `guestIndex`),
  KEY `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Ember Court guest standing and pending guest list';
