-- Delete Totem models for non existing spells
DELETE FROM `spell_totem_model` WHERE `SpellID` IN (157153);

-- Delete Incorrect GO Loot
DELETE FROM `gameobject_loot_template` WHERE `entry` = 1731; -- copper vein has a different loot id, keeping that one.
DELETE FROM `gameobject_loot_template` WHERE `entry` = 1732; -- tin vein has a different loot id, keeping that one.
DELETE FROM `gameobject_loot_template` WHERE `entry` = 1733; -- silver vein has a different loot id, keeping that one.
DELETE FROM `gameobject_loot_template` WHERE `entry` = 1734; -- gold vein has a different loot id, keeping that one.

-- Remove not allowed unit_flags
UPDATE creature_template SET unit_flags3 = unit_flags3 & ~0x00000001;
UPDATE creature_template SET unit_flags2 = unit_flags2 & ~0x02000000;

