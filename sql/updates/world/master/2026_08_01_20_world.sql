--
-- Titanstrike scenario (Warlord Volund's tomb, Shield's Rest / map 1495) - Phase 1: fix broken-import factions.
-- Every creature on map 1495 was imported with faction 35 (universal-friendly), so the tomb's enemies rendered
-- green and unattackable. Set the tomb's hostile enemies + bosses to faction 16 - the Stormheim vrykul enemy
-- faction used by the open-world Drekirjar/Tideskorn vrykul - so they are hostile to the player. Allies (Grif,
-- Prustaga, Orik), triggers, and unrelated content sharing this map are left untouched.
--
UPDATE `creature_template` SET `faction`=16 WHERE `entry` IN (
  94707,          -- Icebreaker Tombguard
  106302, 105967, -- Restless Tombguard
  96571,          -- Rattling Dead
  104740,         -- Disturbed Tracker
  106307,         -- Disturbed Worg
  94813, 105968,  -- Spectral Windshaper
  106347,         -- Spectral Champion
  96468,          -- Hruthnir
  105695,         -- Yrgrim the Truthseeker
  104956          -- Warlord Volund
);
