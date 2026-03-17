-- ============================================================================
-- Playerbot Spell Fix: Hunter Binding Shot (spell 109248)
-- ============================================================================
--
-- Problem: TrinityCore's at_hun_binding_shot script has a null pointer bug
-- that causes crashes when units are removed during AreaTrigger iteration.
--
-- Solution: Change the script name in the database to use Playerbot's
-- null-safe version instead of TrinityCore's buggy version.
--
-- This SQL must be applied to the WORLD database.
-- ============================================================================

-- Update areatrigger_template to use Playerbot's fixed script
UPDATE `areatrigger_template`
SET `ScriptName` = 'at_hun_binding_shot_playerbot'
WHERE `ScriptName` = 'at_hun_binding_shot';

-- Verify the change
SELECT Id, ScriptName FROM `areatrigger_template` WHERE `ScriptName` LIKE '%binding_shot%';
