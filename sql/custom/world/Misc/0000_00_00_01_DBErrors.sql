-- Delete Totem models for non existing spells
DELETE FROM `spell_totem_model` WHERE `SpellID` IN (157153);

-- Delete Incorrect GO Loot
DELETE FROM `gameobject_loot_template` WHERE `entry` = 1731; -- copper vein has a different loot id, keeping that one.
DELETE FROM `gameobject_loot_template` WHERE `entry` = 1732; -- tin vein has a different loot id, keeping that one.
DELETE FROM `gameobject_loot_template` WHERE `entry` = 1733; -- silver vein has a different loot id, keeping that one.
DELETE FROM `gameobject_loot_template` WHERE `entry` = 1734; -- gold vein has a different loot id, keeping that one.