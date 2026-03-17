-- ============================================================================
-- PLAYERBOT DRAGONRIDING: SPELL SCRIPT BINDINGS (WORLD DATABASE)
-- ============================================================================
-- Enterprise-grade spell script bindings using RETAIL spell IDs
--
-- This SQL binds our C++ spell script handlers to the retail spell IDs.
-- The spell_script_names table tells the server which C++ class to use
-- when processing each spell.
--
-- RETAIL SPELL IDS (from wowhead.com - WoW 12.0):
--   369536 = Soar (Dracthyr racial - initiates dragonriding)
--   372608 = Surge Forward (primary forward burst)
--   372610 = Skyward Ascent (upward burst)
--   361584 = Whirling Surge (spiral forward, talent-gated)
--   403092 = Aerial Halt (brake/stop, talent-gated)
--   383359 = Vigor (the resource/charges system)
--   383366 = Thrill of the Skies (high-speed regen buff)
--
-- Install into: world database
-- ============================================================================

-- ============================================================================
-- STEP 1: CLEAN UP OLD SCRIPT BINDINGS
-- ============================================================================
-- Remove all previous bindings for dragonriding spells (both old custom and retail)

DELETE FROM `spell_script_names` WHERE `spell_id` IN (
    -- Retail spell IDs
    369536,   -- Soar
    372608,   -- Surge Forward
    372610,   -- Skyward Ascent
    361584,   -- Whirling Surge
    403092,   -- Aerial Halt
    383359,   -- Vigor
    383366,   -- Thrill of the Skies
    376027,   -- Base Dragonriding unlock
    -- Old custom spell IDs (cleanup)
    900001, 900002, 900003, 900004, 900005,
    900006, 900007, 900008, 900009, 900010
);

-- ============================================================================
-- STEP 2: SOAR (369536) - Dracthyr Evoker Racial
-- ============================================================================
-- Main entry point for dragonriding. When cast:
--   1. Sets action bar override (SetOverrideSpellsId)
--   2. Enables dragonriding physics
--   3. Applies vigor buff for charge tracking
--   4. Adds temporary spells so abilities are castable

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(369536, 'spell_playerbot_soar'),
(369536, 'spell_playerbot_soar_aura');

-- ============================================================================
-- STEP 3: ACTION BAR ABILITIES (Retail IDs)
-- ============================================================================
-- These are the 4 abilities that appear on the dragonriding action bar.
-- Client already has complete data for these spells.

-- Surge Forward (372608) - Primary horizontal mobility
-- Consumes 1 vigor charge, provides forward velocity boost
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(372608, 'spell_playerbot_surge_forward');

-- Skyward Ascent (372610) - Vertical mobility
-- Consumes 1 vigor charge, provides upward velocity boost
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(372610, 'spell_playerbot_skyward_ascent');

-- Whirling Surge (361584) - Advanced mobility (Airborne Tumbling talent)
-- Consumes 1 vigor charge, spiral movement, 30s cooldown
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(361584, 'spell_playerbot_whirling_surge');

-- Aerial Halt (403092) - Emergency stop (At Home Aloft talent)
-- Does NOT consume vigor, 10s cooldown
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(403092, 'spell_playerbot_aerial_halt');

-- ============================================================================
-- STEP 4: SYSTEM SPELLS (Retail IDs)
-- ============================================================================

-- Vigor (383359) - The resource/charge system
-- Tracks charges, sets initial stacks on application
-- Note: Vigor regeneration handled separately (retail spell uses SPELL_AURA_DUMMY, no periodic)
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(383359, 'spell_playerbot_vigor_aura');

-- ============================================================================
-- VERIFICATION
-- ============================================================================
-- After applying, verify with:
--   SELECT * FROM spell_script_names
--   WHERE spell_id IN (369536, 372608, 372610, 361584, 403092, 383359);
--
-- Expected: 7 rows total:
--   369536 spell_playerbot_soar
--   369536 spell_playerbot_soar_aura
--   372608 spell_playerbot_surge_forward
--   372610 spell_playerbot_skyward_ascent
--   361584 spell_playerbot_whirling_surge
--   403092 spell_playerbot_aerial_halt
--   383359 spell_playerbot_vigor_aura
--
-- Note: Vigor regeneration is handled by WorldScript (DragonridingVigorRegeneration)
-- since retail Vigor (383359) uses SPELL_AURA_DUMMY which has no periodic ticks.
--
-- Restart worldserver for changes to take effect.
-- Check server log for: ">> Registered Playerbot Dragonriding SpellScripts"
-- ============================================================================
