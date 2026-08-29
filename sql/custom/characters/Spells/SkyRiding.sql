-- ============================================================
-- Apply to 'characters' database
-- Clean up old data that prevents Skyriding from working
-- ============================================================

-- USE characters;

-- Remove flight auras incorrectly saved as learned spells
DELETE FROM character_spell WHERE spell IN (404464, 404468);

-- Remove saved aura data (these are reapplied on login)
DELETE FROM character_aura WHERE spell IN (404464, 404468, 372773);
DELETE FROM character_aura_effect WHERE spell IN (404464, 404468, 372773);