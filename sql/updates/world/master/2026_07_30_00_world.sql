-- Garrison work-order crates (GAMEOBJECT_TYPE_GARRISON_SHIPMENT / type 45) must carry the base flag
-- GO_FLAG_IGNORE_CURRENT_STATE_FOR_USE_SPELL_EXCEPT_UNLOCKED (0x40000 / 262144) to be clickable on the
-- live client. Verified against a WoD-garrison sniff on a modern client: retail spawns these crates with
-- GAMEOBJECT_FIELD_FLAGS = 0x40000 (and DynamicFlags 0x8000, which we already emit). Many crate rows in
-- our world DB already have this flag; a subset (incl. 236685 "Leatherworking Work Order") were missing it,
-- so those crates never became interactable. This aligns every WoD work-order crate with its siblings.

-- Add an addon row for any work-order crate that lacks one.
INSERT INTO `gameobject_template_addon` (`entry`, `flags`)
SELECT gt.`entry`, 262144
FROM `gameobject_template` gt
LEFT JOIN `gameobject_template_addon` gta ON gta.`entry` = gt.`entry`
WHERE gt.`type` = 45 AND gt.`name` LIKE '%Work Order%' AND gta.`entry` IS NULL;

-- Set the flag on existing addon rows that are missing it (idempotent bitwise OR).
UPDATE `gameobject_template_addon` gta
JOIN `gameobject_template` gt ON gt.`entry` = gta.`entry`
SET gta.`flags` = gta.`flags` | 262144
WHERE gt.`type` = 45 AND gt.`name` LIKE '%Work Order%' AND (gta.`flags` & 262144) = 0;
