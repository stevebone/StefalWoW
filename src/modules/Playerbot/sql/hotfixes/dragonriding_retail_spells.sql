-- ============================================================================
-- PLAYERBOT DRAGONRIDING: RETAIL SPELL INTEGRATION
-- ============================================================================
-- INSTALLATION:
--   1. Connect to your hotfixes database (e.g., hotfixes_11)
--   2. Run this SQL script
--   3. Restart worldserver
--
-- IMPORTANT: If action bar doesn't change, check server logs for:
--   "Unknown DB2 store by hash" - means table hash may need updating
--   See TROUBLESHOOTING section at the end of this file.
-- ============================================================================
-- Enterprise-grade approach using REAL retail spell IDs
--
-- PHILOSOPHY:
-- The WoW client already has complete data for retail spells including:
--   - Icons, names, tooltips in all languages
--   - Spell effects, animations, sounds
--   - Proper spell mechanics and categories
--
-- We only need ONE thing: an override_spell_data entry that tells the client
-- which spells to show on the action bar when dragonriding is active.
--
-- RETAIL SPELL IDS (from wowhead.com - WoW 12.0):
--   372608 = Surge Forward (Flap forward, 6 charges, 15s recharge)
--   372610 = Skyward Ascent (Flap upward, 6 charges, 15s recharge)
--   361584 = Whirling Surge (Spiral forward, 30s cooldown)
--   403092 = Aerial Halt (Flap back, 10s cooldown)
--
-- These are the EXACT same spells players see on retail dragonriding.
-- ============================================================================

-- ============================================================================
-- STEP 1: CLEAN UP OLD CUSTOM SPELL ENTRIES (if any exist)
-- ============================================================================
-- Remove any previous custom spell attempts in the 900000+ range
-- These are no longer needed since we're using retail spells

DELETE FROM hotfix_data WHERE TableHash IN (
    -- Spell table hash
    0xC8D8FADF,
    -- SpellName table hash
    0xC9FC8E7D,
    -- SpellMisc table hash
    0x2C63C4B4,
    -- SpellEffect table hash
    0xC5C3F622,
    -- SpellAuraOptions table hash
    0x9A6BDB26,
    -- OverrideSpellData table hash (from server log: 0xCA75DF1C)
    0xCA75DF1C
) AND RecordId >= 900000;

-- ============================================================================
-- STEP 2: CREATE OVERRIDE SPELL DATA ENTRY
-- ============================================================================
-- This is the ONLY custom entry we need!
-- It maps our override ID (900001) to the retail dragonriding action bar spells.
--
-- Table: override_spell_data (hotfixes database)
-- Purpose: Defines action bar override spell sets
--
-- Column Layout:
--   ID           = Our custom override ID (referenced by SetOverrideSpellsId)
--   Spells[0-9]  = Up to 10 spell IDs to show on action bar
--   PlayerActionBarFileDataID = Optional custom action bar texture
--   Flags        = Override behavior flags
--
-- When player has this override active via SetOverrideSpellsId(900001):
--   Slot 1: Surge Forward (372608) - Primary mobility
--   Slot 2: Skyward Ascent (372610) - Gain altitude
--   Slot 3: Whirling Surge (361584) - Burst forward
--   Slot 4: Aerial Halt (403092) - Emergency brake
-- ============================================================================

-- First, ensure clean slate for our override ID
DELETE FROM override_spell_data WHERE ID = 900001;

-- Insert the override spell data entry
-- Note: Using retail spell IDs means client already has icons, tooltips, etc.
-- VerifiedBuild should match the client build version
INSERT INTO override_spell_data (
    ID,
    Spells1, Spells2, Spells3, Spells4, Spells5,
    Spells6, Spells7, Spells8, Spells9, Spells10,
    PlayerActionBarFileDataID,
    Flags,
    VerifiedBuild
) VALUES (
    900001,                 -- ID: Our custom override ID for Soar/Dragonriding
    372608,                 -- Slot 1: Surge Forward (retail ID)
    372610,                 -- Slot 2: Skyward Ascent (retail ID)
    361584,                 -- Slot 3: Whirling Surge (retail ID)
    403092,                 -- Slot 4: Aerial Halt (retail ID)
    0,                      -- Slot 5: Empty
    0,                      -- Slot 6: Empty
    0,                      -- Slot 7: Empty
    0,                      -- Slot 8: Empty
    0,                      -- Slot 9: Empty
    0,                      -- Slot 10: Empty
    0,                      -- PlayerActionBarFileDataID: Use default
    0,                      -- Flags: None
    64978                   -- VerifiedBuild: WoW 12.0 build number
);

-- ============================================================================
-- STEP 3: REGISTER HOTFIX FOR OVERRIDE_SPELL_DATA
-- ============================================================================
-- The hotfix_data table tells the server to send this DB2 entry to the client.
-- Without this, the client won't know about our custom override entry.
--
-- Table hash for OverrideSpellData.db2: 0xCA75DF1C (from server log)
-- This hash was obtained from: sOverrideSpellDataStore.GetTableHash()
-- Status values: 0=NotSet, 1=Valid, 2=RecordRemoved, 3=Invalid, 4=NotPublic
-- ============================================================================

DELETE FROM hotfix_data WHERE TableHash = 0xCA75DF1C AND RecordId = 900001;

INSERT INTO hotfix_data (
    Id,
    UniqueId,
    TableHash,
    RecordId,
    Status,
    VerifiedBuild
) VALUES (
    900001,                 -- Id: Unique hotfix identifier
    900001,                 -- UniqueId: Same as Id for simplicity
    0xCA75DF1C,             -- TableHash: OverrideSpellData.db2 (from server log!)
    900001,                 -- RecordId: The override_spell_data.ID we created
    1,                      -- Status: 1 = Valid
    64978                   -- VerifiedBuild: WoW 12.0 build number
);

-- ============================================================================
-- VERIFICATION QUERIES
-- ============================================================================
-- Run these to verify the setup is correct:
--
-- Check override_spell_data entry:
--   SELECT * FROM override_spell_data WHERE ID = 900001;
--
-- Check hotfix registration:
--   SELECT * FROM hotfix_data WHERE RecordId = 900001;
--
-- Verify retail spells exist in client data (should return data):
--   SELECT * FROM spell WHERE ID IN (372608, 372610, 361584, 403092);
--
-- ============================================================================

-- ============================================================================
-- SUMMARY: RETAIL SPELL APPROACH
-- ============================================================================
--
-- BEFORE (Custom Spells - Complex):
--   - Created 4+ custom spell entries (900002-900005)
--   - Needed spell_name, spell_misc, spell_effect entries
--   - Required manual icon FileDataIDs
--   - Missing tooltips, localization, animations
--   - ~50+ database rows, error-prone
--
-- AFTER (Retail Spells - Simple):
--   - 1 override_spell_data entry (our custom ID → retail spell IDs)
--   - 1 hotfix_data entry (to send override to client)
--   - Client already has icons, tooltips, animations for retail spells
--   - 2 database rows total, bulletproof
--
-- The client does all the heavy lifting because it already knows about
-- spells 372608, 372610, 361584, 403092 from the retail game data.
-- ============================================================================

-- ============================================================================
-- TROUBLESHOOTING
-- ============================================================================
-- If action bar doesn't change when Soar is activated:
--
-- 1. Check server logs for this error:
--    "Table `hotfix_data` references unknown DB2 store by hash 0xXXXXXXXX"
--    The correct hash for OverrideSpellData.db2 is 0xCA75DF1C (from server log).
--
-- 2. To find the correct table hash, add this to PlayerbotDragonridingScript.cpp:
--    TC_LOG_INFO("playerbot", "OverrideSpellData hash: 0x{:X}",
--                sOverrideSpellDataStore.GetTableHash());
--    Then check the log output and update the hash in this file.
--
-- 3. Verify the override_spell_data entry exists:
--    SELECT * FROM override_spell_data WHERE ID = 900001;
--    Should show: 900001, 372608, 372610, 361584, 403092, ...
--
-- 4. Verify the hotfix_data entry exists with Status=1 (Valid):
--    SELECT * FROM hotfix_data WHERE RecordId = 900001;
--    Should show: Status = 1 (NOT 2, which means RecordRemoved!)
--
-- 5. After running this SQL, you MUST restart worldserver for changes
--    to take effect. The hotfix cache is loaded at startup.
--
-- Status values reference:
--   0 = NotSet
--   1 = Valid      (use this!)
--   2 = RecordRemoved
--   3 = Invalid
--   4 = NotPublic
-- ============================================================================
