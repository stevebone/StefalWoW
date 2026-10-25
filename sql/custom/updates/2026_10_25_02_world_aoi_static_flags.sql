-- Migrate UNIT_FLAG2_{LARGE,GIGANTIC,INFINITE}_AOI -> creature static flags.
-- The unit_flags2 bits are disallowed for DB and stripped at load; the static
-- flag columns are the retail data location. ORDER MATTERS: the clear runs last.

-- (diagnostic) affected templates
SELECT `entry`, `unit_flags2` FROM `creature_template`
WHERE `unit_flags2` & 0x40600000;

-- LARGE_AOI -> StaticFlags1 (all difficulty rows of flagged entries)
UPDATE `creature_template_difficulty` ctd
JOIN `creature_template` ct ON ct.`entry` = ctd.`Entry`
SET ctd.`StaticFlags1` = ctd.`StaticFlags1` | 0x80000000
WHERE ct.`unit_flags2` & 0x00200000;

-- GIGANTIC_AOI -> StaticFlags3
UPDATE `creature_template_difficulty` ctd
JOIN `creature_template` ct ON ct.`entry` = ctd.`Entry`
SET ctd.`StaticFlags3` = ctd.`StaticFlags3` | 0x00000400
WHERE ct.`unit_flags2` & 0x00400000;

-- INFINITE_AOI -> StaticFlags3
UPDATE `creature_template_difficulty` ctd
JOIN `creature_template` ct ON ct.`entry` = ctd.`Entry`
SET ctd.`StaticFlags3` = ctd.`StaticFlags3` | 0x00000800
WHERE ct.`unit_flags2` & 0x40000000;

-- Entries missing a DifficultyID=0 row: single combined INSERT IGNORE so
-- multi-flag entries (e.g. LARGE+INFINITE) get every bit in one row.
-- (Entry, DifficultyID) PK skips entries that already have a 0 row; entries
-- with only non-zero-difficulty rows still get their base row created.
INSERT IGNORE INTO `creature_template_difficulty` (`Entry`, `DifficultyID`, `StaticFlags1`, `StaticFlags3`)
SELECT ct.`entry`, 0,
       IF(ct.`unit_flags2` & 0x00200000, CAST(0x80000000 AS UNSIGNED), 0),
       IF(ct.`unit_flags2` & 0x00400000, CAST(0x00000400 AS UNSIGNED), 0) | IF(ct.`unit_flags2` & 0x40000000, CAST(0x00000800 AS UNSIGNED), 0)
FROM `creature_template` ct
WHERE ct.`unit_flags2` & 0x40600000;

-- Spawn-level: the only flagged spawn row (guid 900815, entry 29611) is already
-- covered by the template migration - its template carries LARGE_AOI, so the
-- clear below is all it needs (no creature_static_flags_override required).

-- Remove the bits (must run last - every statement above reads unit_flags2)
UPDATE `creature_template` SET `unit_flags2` = `unit_flags2` & ~0x40600000 WHERE `unit_flags2` & 0x40600000;
UPDATE `creature`         SET `unit_flags2` = `unit_flags2` & ~0x40600000 WHERE `unit_flags2` & 0x40600000;
