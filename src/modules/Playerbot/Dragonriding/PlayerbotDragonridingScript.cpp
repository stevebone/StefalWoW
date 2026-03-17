/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/*
 * Dragonriding SpellScripts for Playerbot Module
 *
 * ENTERPRISE-GRADE RETAIL SPELL INTEGRATION
 *
 * This module uses REAL retail spell IDs (from wowhead.com WoW 12.0.7):
 *   369536 = Soar (Dracthyr racial - initiates dragonriding)
 *   372608 = Surge Forward (primary forward burst)
 *   372610 = Skyward Ascent (upward burst)
 *   361584 = Whirling Surge (spiral forward)
 *   403092 = Aerial Halt (brake/stop)
 *   383359 = Vigor (the resource/charges system)
 *   383366 = Thrill of the Skies (high-speed regen buff)
 *
 * Using retail spell IDs means the client already has:
 *   - Proper icons, names, tooltips in all languages
 *   - Spell effects, animations, sounds
 *   - Correct spell mechanics and categories
 *
 * We only need override_spell_data to map our override ID to these spells.
 */

#include "DragonridingDefines.h"
#include "DragonridingMgr.h"
#include "DB2Stores.h"
#include "DB2HotfixGenerator.h"
#include "UnitDefines.h"
#include "DB2Structure.h"
#include "Log.h"
#include "Map.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellHistory.h"
#include "SpellMgr.h"
#include "SpellPackets.h"
#include "SpellScript.h"
#include "WorldSession.h"

using namespace Playerbot::Dragonriding;

// ============================================================================
// DEBUG: SPELL REQUIREMENTS DUMPER
// ============================================================================
// Logs all DB2 conditions for a spell to help diagnose "You can't do that yet"

namespace
{

void DumpSpellRequirements(uint32 spellId, Player* player)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
    {
        TC_LOG_ERROR("playerbot.dragonriding", ">>> DEBUG: Spell {} NOT FOUND in SpellMgr", spellId);
        return;
    }

    TC_LOG_ERROR("playerbot.dragonriding", ">>> ========================================");
    TC_LOG_ERROR("playerbot.dragonriding", ">>> DEBUG SPELL REQUIREMENTS: {} (ID: {})", spellInfo->SpellName->Str[LOCALE_enUS], spellId);
    TC_LOG_ERROR("playerbot.dragonriding", ">>> ========================================");

    // Basic spell attributes
    TC_LOG_ERROR("playerbot.dragonriding", ">>> Attributes[0]: 0x{:08X}", spellInfo->Attributes);
    TC_LOG_ERROR("playerbot.dragonriding", ">>> AttributesEx[0]: 0x{:08X}", spellInfo->AttributesEx);
    TC_LOG_ERROR("playerbot.dragonriding", ">>> AttributesEx2: 0x{:08X}", spellInfo->AttributesEx2);
    TC_LOG_ERROR("playerbot.dragonriding", ">>> AttributesEx3: 0x{:08X}", spellInfo->AttributesEx3);
    TC_LOG_ERROR("playerbot.dragonriding", ">>> AttributesEx4: 0x{:08X}", spellInfo->AttributesEx4);
    TC_LOG_ERROR("playerbot.dragonriding", ">>> Stances: 0x{:016X}", spellInfo->Stances);
    TC_LOG_ERROR("playerbot.dragonriding", ">>> StancesNot: 0x{:016X}", spellInfo->StancesNot);

    // Aura restrictions (CasterAuraSpell, etc.)
    TC_LOG_ERROR("playerbot.dragonriding", ">>> CasterAuraSpell: {}", spellInfo->CasterAuraSpell);
    TC_LOG_ERROR("playerbot.dragonriding", ">>> TargetAuraSpell: {}", spellInfo->TargetAuraSpell);
    TC_LOG_ERROR("playerbot.dragonriding", ">>> ExcludeCasterAuraSpell: {}", spellInfo->ExcludeCasterAuraSpell);
    TC_LOG_ERROR("playerbot.dragonriding", ">>> ExcludeTargetAuraSpell: {}", spellInfo->ExcludeTargetAuraSpell);
    TC_LOG_ERROR("playerbot.dragonriding", ">>> CasterAuraState: {}", uint32(spellInfo->CasterAuraState));
    TC_LOG_ERROR("playerbot.dragonriding", ">>> TargetAuraState: {}", uint32(spellInfo->TargetAuraState));
    TC_LOG_ERROR("playerbot.dragonriding", ">>> CasterAuraType: {}", uint32(spellInfo->CasterAuraType));
    TC_LOG_ERROR("playerbot.dragonriding", ">>> TargetAuraType: {}", uint32(spellInfo->TargetAuraType));

    // Casting requirements
    TC_LOG_ERROR("playerbot.dragonriding", ">>> RequiredAreasID: {}", spellInfo->RequiredAreasID);
    TC_LOG_ERROR("playerbot.dragonriding", ">>> RequiresSpellFocus: {}", spellInfo->RequiresSpellFocus);

    // Player state checks
    if (player)
    {
        TC_LOG_ERROR("playerbot.dragonriding", ">>> --- PLAYER STATE ---");
        TC_LOG_ERROR("playerbot.dragonriding", ">>> Player IsMounted: {}", player->IsMounted() ? "YES" : "NO");
        TC_LOG_ERROR("playerbot.dragonriding", ">>> Player IsFlying: {}", player->IsFlying() ? "YES" : "NO");
        TC_LOG_ERROR("playerbot.dragonriding", ">>> Player FlightCapabilityID: {}", player->GetFlightCapabilityID());
        TC_LOG_ERROR("playerbot.dragonriding", ">>> Player HasSpell(DRAGONRIDING={}): {}",
            SPELL_DRAGONRIDING, player->HasSpell(SPELL_DRAGONRIDING) ? "YES" : "NO");
        TC_LOG_ERROR("playerbot.dragonriding", ">>> Player HasAura(SOAR={}): {}",
            SPELL_SOAR, player->HasAura(SPELL_SOAR) ? "YES" : "NO");
        TC_LOG_ERROR("playerbot.dragonriding", ">>> Player HasAura(FLIGHT_STYLE_SKYRIDING={}): {}",
            SPELL_FLIGHT_STYLE_SKYRIDING, player->HasAura(SPELL_FLIGHT_STYLE_SKYRIDING) ? "YES" : "NO");
        TC_LOG_ERROR("playerbot.dragonriding", ">>> Player HasAura(VIGOR={}): {}",
            SPELL_VIGOR, player->HasAura(SPELL_VIGOR) ? "YES" : "NO");
        TC_LOG_ERROR("playerbot.dragonriding", ">>> Player HasAura(DRAGONRIDER_ENERGY={}): {}",
            SPELL_DRAGONRIDER_ENERGY, player->HasAura(SPELL_DRAGONRIDER_ENERGY) ? "YES" : "NO");
        TC_LOG_ERROR("playerbot.dragonriding", ">>> Player POWER_ALTERNATE_POWER (vigor UI): {}/{}",
            player->GetPower(POWER_ALTERNATE_POWER), player->GetMaxPower(POWER_ALTERNATE_POWER));
        TC_LOG_ERROR("playerbot.dragonriding", ">>> Player HasUnitFlag(MOUNT): {}", player->HasUnitFlag(UNIT_FLAG_MOUNT) ? "YES" : "NO");
        TC_LOG_ERROR("playerbot.dragonriding", ">>> Player ShapeshiftForm: {}", uint32(player->GetShapeshiftForm()));

        // Check if player has the required auras
        if (spellInfo->CasterAuraSpell != 0)
        {
            TC_LOG_ERROR("playerbot.dragonriding", ">>> REQUIRED: CasterAuraSpell {} - Player has it: {}",
                spellInfo->CasterAuraSpell, player->HasAura(spellInfo->CasterAuraSpell) ? "YES" : "NO");
        }
        if (spellInfo->ExcludeCasterAuraSpell != 0)
        {
            TC_LOG_ERROR("playerbot.dragonriding", ">>> EXCLUDED: ExcludeCasterAuraSpell {} - Player has it: {}",
                spellInfo->ExcludeCasterAuraSpell, player->HasAura(spellInfo->ExcludeCasterAuraSpell) ? "YES" : "NO");
        }
    }

    TC_LOG_ERROR("playerbot.dragonriding", ">>> ========================================");
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Get FlightCapabilityEntry for dragonriding physics
const FlightCapabilityEntry* GetDragonridingFlightCapability()
{
    // Query the FlightCapability DB2 store for a valid dragonriding entry
    // FlightCapability ID 1 is typically the standard dragonriding physics
    return sFlightCapabilityStore.LookupEntry(FLIGHT_CAPABILITY_SOAR);
}

// Check if a player can use Soar (Dracthyr Evoker only)
bool CanPlayerUseSoar(Player* player)
{
    if (!player)
        return false;

    return CanUseSoar(player->GetRace(), player->GetClass());
}

// Get account ID from player
uint32 GetAccountId(Player* player)
{
    if (!player || !player->GetSession())
        return 0;
    return player->GetSession()->GetAccountId();
}

} // anonymous namespace

// ============================================================================
// SPELL: SOAR (369536) - Dracthyr Evoker Racial
// Activates dragonriding physics mode
// ============================================================================

class spell_playerbot_soar : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        // DEBUG: Log what spells we're checking
        TC_LOG_ERROR("playerbot.dragonriding", ">>> spell_playerbot_soar::Validate() called");
        TC_LOG_ERROR("playerbot.dragonriding", ">>>   Checking SPELL_VIGOR ({}): {}",
            SPELL_VIGOR, sSpellMgr->GetSpellInfo(SPELL_VIGOR, DIFFICULTY_NONE) ? "EXISTS" : "MISSING");
        TC_LOG_ERROR("playerbot.dragonriding", ">>>   Checking SPELL_SURGE_FORWARD ({}): {}",
            SPELL_SURGE_FORWARD, sSpellMgr->GetSpellInfo(SPELL_SURGE_FORWARD, DIFFICULTY_NONE) ? "EXISTS" : "MISSING");
        TC_LOG_ERROR("playerbot.dragonriding", ">>>   Checking SPELL_SKYWARD_ASCENT ({}): {}",
            SPELL_SKYWARD_ASCENT, sSpellMgr->GetSpellInfo(SPELL_SKYWARD_ASCENT, DIFFICULTY_NONE) ? "EXISTS" : "MISSING");
        TC_LOG_ERROR("playerbot.dragonriding", ">>>   Checking SPELL_WHIRLING_SURGE ({}): {}",
            SPELL_WHIRLING_SURGE, sSpellMgr->GetSpellInfo(SPELL_WHIRLING_SURGE, DIFFICULTY_NONE) ? "EXISTS" : "MISSING");
        TC_LOG_ERROR("playerbot.dragonriding", ">>>   Checking SPELL_AERIAL_HALT ({}): {}",
            SPELL_AERIAL_HALT, sSpellMgr->GetSpellInfo(SPELL_AERIAL_HALT, DIFFICULTY_NONE) ? "EXISTS" : "MISSING");
        TC_LOG_ERROR("playerbot.dragonriding", ">>>   Checking SPELL_FLIGHT_STYLE_SKYRIDING ({}): {}",
            SPELL_FLIGHT_STYLE_SKYRIDING, sSpellMgr->GetSpellInfo(SPELL_FLIGHT_STYLE_SKYRIDING, DIFFICULTY_NONE) ? "EXISTS" : "MISSING");

        // Validate that required retail spells exist in SpellMgr
        // Using retail spell IDs - client has all data, we just verify server knows them
        TC_LOG_ERROR("playerbot.dragonriding", ">>>   Checking SPELL_DRAGONRIDER_ENERGY ({}): {}",
            SPELL_DRAGONRIDER_ENERGY, sSpellMgr->GetSpellInfo(SPELL_DRAGONRIDER_ENERGY, DIFFICULTY_NONE) ? "EXISTS" : "MISSING");

        bool result = ValidateSpellInfo({
            SPELL_VIGOR,                   // 383359 - Vigor/Skyriding charges (retail)
            SPELL_FLIGHT_STYLE_SKYRIDING,  // 404464 - "Skyriding enabled" - CRITICAL for abilities
            SPELL_DRAGONRIDER_ENERGY,      // 372773 - "Dragonrider Energy" - REQUIRED by Surge/Skyward!
            SPELL_SURGE_FORWARD,           // 372608 - Forward burst (retail)
            SPELL_SKYWARD_ASCENT,          // 372610 - Upward burst (retail)
            SPELL_WHIRLING_SURGE,          // 361584 - Spiral forward (retail)
            SPELL_AERIAL_HALT              // 403092 - Brake/stop (retail)
        });

        TC_LOG_ERROR("playerbot.dragonriding", ">>> spell_playerbot_soar::Validate() result: {}", result ? "SUCCESS" : "FAILED");
        return result;
    }

    SpellCastResult CheckCast()
    {
        Player* caster = GetCaster()->ToPlayer();
        if (!caster)
            return SPELL_FAILED_BAD_TARGETS;

        // Only Dracthyr Evokers can use Soar
        if (!CanPlayerUseSoar(caster))
            return SPELL_FAILED_INCORRECT_AREA; // Fallback error

        // Cannot use while already in dragonriding mode
        if (caster->GetFlightCapabilityID() != 0)
            return SPELL_FAILED_NOT_HERE;

        // Cannot use indoors
        if (!caster->IsOutdoors())
            return SPELL_FAILED_ONLY_OUTDOORS;

        return SPELL_CAST_OK;
    }

    void HandleOnCast()
    {
        Player* caster = GetCaster()->ToPlayer();
        if (!caster)
            return;

        TC_LOG_ERROR("playerbot.dragonriding", ">>> SOAR SPELL SCRIPT TRIGGERED for player {}", caster->GetName());

        uint32 accountId = GetAccountId(caster);
        if (accountId == 0)
        {
            TC_LOG_ERROR("playerbot.dragonriding", ">>> Account ID is 0, aborting Soar");
            return;
        }

        // Check if DragonridingMgr is initialized
        if (!sDragonridingMgr->IsInitialized())
        {
            TC_LOG_ERROR("playerbot.dragonriding", ">>> DragonridingMgr not initialized, Soar cast failed for player {}",
                caster->GetName());
            return;
        }

        // CRITICAL FIX #1: Ensure player has SPELL_DRAGONRIDING (376027) learned
        // This is the ACCOUNT-WIDE dragonriding unlock from quest completion
        // Dragonriding abilities check if this spell is known before they can be used
        // Without this, client blocks abilities with "You can't do that yet"
        if (!caster->HasSpell(SPELL_DRAGONRIDING))
        {
            caster->LearnSpell(SPELL_DRAGONRIDING, false);
            TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: Taught SPELL_DRAGONRIDING ({}) to player {} (account-wide unlock)",
                SPELL_DRAGONRIDING, caster->GetName());
        }
        else
        {
            TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: Player {} already has SPELL_DRAGONRIDING ({})",
                caster->GetName(), SPELL_DRAGONRIDING);
        }

        // CRITICAL FIX #2: Set the mounted state using DISPLAYID_HIDDEN_MOUNT
        // Dragonriding abilities check IsMounted() which requires UNIT_FLAG_MOUNT to be set
        // Dracthyr using Soar ARE the mount (self-mount), so we use hidden mount display
        // Without this, client blocks abilities with "You can't do that yet"
        caster->Mount(DISPLAYID_HIDDEN_MOUNT, 0, 0);
        TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: Set mounted state (DISPLAYID_HIDDEN_MOUNT={}) for player {}",
            DISPLAYID_HIDDEN_MOUNT, caster->GetName());

        // Activate dragonriding physics
        const FlightCapabilityEntry* flightCap = GetDragonridingFlightCapability();
        if (flightCap)
        {
            caster->SetFlightCapabilityID(flightCap->ID, true);
            TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: FlightCapability set to {} for player {}",
                flightCap->ID, caster->GetName());
        }
        else
        {
            // Fallback to ID 1 if lookup fails
            caster->SetFlightCapabilityID(FLIGHT_CAPABILITY_SOAR, true);
            TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: FlightCapability lookup failed, using default ID {} for player {}",
                FLIGHT_CAPABILITY_SOAR, caster->GetName());
        }

        // Grant vigor based on learned talents (progression-based)
        uint32 maxVigor = sDragonridingMgr->GetMaxVigor(accountId);

        // DEBUG: Check if retail vigor spell (383359) exists in SpellMgr
        SpellInfo const* vigorSpellInfo = sSpellMgr->GetSpellInfo(SPELL_VIGOR, DIFFICULTY_NONE);
        if (!vigorSpellInfo)
        {
            TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: CRITICAL - Retail Vigor spell {} NOT FOUND in SpellMgr! "
                "This spell should exist in client DB2 data.", SPELL_VIGOR);
        }
        else
        {
            TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: Retail Vigor spell {} found in SpellMgr (good!)",
                SPELL_VIGOR);
        }

        // CRITICAL: Remove Flight Style: Steady (404468) first - can only have ONE flight style active
        // Player may have Steady Flight enabled by default, which blocks Skyriding abilities
        if (caster->HasAura(SPELL_FLIGHT_STYLE_STEADY))
        {
            caster->RemoveAura(SPELL_FLIGHT_STYLE_STEADY);
            TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: Removed Flight Style: Steady ({}) to enable Skyriding",
                SPELL_FLIGHT_STYLE_STEADY);
        }

        // =========================================================================
        // CRITICAL FIX #3: Apply Dragonrider Energy (372773) FIRST
        // This aura is REQUIRED by Surge Forward and Skyward Ascent!
        // Both abilities check CasterAuraSpell = 372773 before allowing cast
        // Without this aura: "You can't do that yet" / "Das könnt ihr noch nicht"
        //
        // IMPORTANT: This aura's HandleEnableAltPower() sets POWER_ALTERNATE_POWER
        // to UnitPowerBar 631's MaxPower (base = 3). We MUST apply this aura BEFORE
        // setting our progression-based max vigor, otherwise the aura will override!
        // =========================================================================
        SpellCastResult energyResult = caster->CastSpell(caster, SPELL_DRAGONRIDER_ENERGY, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_FULL_MASK,
            .OriginalCaster = caster->GetGUID()
        });
        TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: CastSpell(SPELL_DRAGONRIDER_ENERGY={}) result: {} - THIS IS REQUIRED FOR ABILITIES!",
            SPELL_DRAGONRIDER_ENERGY, uint32(energyResult));

        // =========================================================================
        // Set vigor using POWER_ALTERNATE_POWER (power type 10) AFTER aura applies
        // The aura sets base max to 3, we override with progression-based max (3-6)
        // based on Dragon Glyphs collected and talents learned.
        // =========================================================================
        caster->SetMaxPower(POWER_ALTERNATE_POWER, maxVigor);
        caster->SetPower(POWER_ALTERNATE_POWER, maxVigor);
        TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: Set POWER_ALTERNATE_POWER to {}/{} for player {} (after aura, overriding base)",
            maxVigor, maxVigor, caster->GetName());

        if (caster->HasAura(SPELL_DRAGONRIDER_ENERGY))
        {
            TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: Dragonrider Energy aura CONFIRMED APPLIED - abilities should now work!");

            // DEBUG: Check what the Dragonrider Energy spell's effects are
            if (SpellInfo const* energyInfo = sSpellMgr->GetSpellInfo(SPELL_DRAGONRIDER_ENERGY, DIFFICULTY_NONE))
            {
                TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: Dragonrider Energy ({}) effects:", SPELL_DRAGONRIDER_ENERGY);
                for (SpellEffectInfo const& effectInfo : energyInfo->GetEffects())
                {
                    TC_LOG_ERROR("playerbot.dragonriding", ">>>   Effect {}: Type={}, AuraType={}, MiscValue={}, MiscValueB={}",
                        effectInfo.EffectIndex,
                        uint32(effectInfo.Effect),
                        uint32(effectInfo.ApplyAuraName),
                        effectInfo.MiscValue,
                        effectInfo.MiscValueB);
                }
            }

            // DEBUG: Check UnitPowerBar 631 (the Alt Power bar for dragonriding)
            if (UnitPowerBarEntry const* powerBar = sUnitPowerBarStore.LookupEntry(631))
            {
                TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: UnitPowerBar 631 EXISTS - Name: {}, MaxPower: {}",
                    powerBar->Name.Str[LOCALE_enUS], powerBar->MaxPower);
            }
            else
            {
                TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: WARNING - UnitPowerBar 631 NOT FOUND! Need to add hotfix data.");
            }

            // DEBUG: Check both POWER_ALTERNATE_POWER and POWER_ALTERNATE_MOUNT
            TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: POWER_ALTERNATE_POWER (10): {}/{}",
                caster->GetPower(POWER_ALTERNATE_POWER), caster->GetMaxPower(POWER_ALTERNATE_POWER));
            TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: POWER_ALTERNATE_MOUNT (25): {}/{}",
                caster->GetPower(POWER_ALTERNATE_MOUNT), caster->GetMaxPower(POWER_ALTERNATE_MOUNT));
        }
        else
        {
            TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: WARNING - Dragonrider Energy aura NOT APPLIED! Abilities will be blocked!");
        }

        // CRITICAL: Apply Flight Style: Skyriding (404464) to enable dragonriding abilities
        // This aura tells the client "Skyriding is currently enabled. Skyriding abilities will be active."
        // Without this, client blocks Surge Forward, Skyward Ascent, etc. with "You can't do that yet"
        SpellCastResult skyrideResult = caster->CastSpell(caster, SPELL_FLIGHT_STYLE_SKYRIDING, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_FULL_MASK,
            .OriginalCaster = caster->GetGUID()
        });
        TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: CastSpell(SPELL_FLIGHT_STYLE_SKYRIDING={}) result: {}",
            SPELL_FLIGHT_STYLE_SKYRIDING, uint32(skyrideResult));

        // Also apply retail vigor buff (383359) for visual tracking
        SpellCastResult castResult = caster->CastSpell(caster, SPELL_VIGOR, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_FULL_MASK,
            .OriginalCaster = caster->GetGUID()
        });

        TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: CastSpell(SPELL_VIGOR={}) result: {}",
            SPELL_VIGOR, uint32(castResult));

        // Set vigor to max stacks
        if (Aura* vigorAura = caster->GetAura(SPELL_VIGOR))
        {
            vigorAura->SetStackAmount(maxVigor);
            TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: Vigor aura granted {} stacks for player {}",
                maxVigor, caster->GetName());
        }
        else
        {
            TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: No vigor aura (may be handled by power type instead)");
        }

        // Swap action bar to dragonriding abilities using DIRECT approach
        // SetOverrideSpellsId updates the client update field directly
        // AddTemporarySpell makes the spells castable on the server
        // The override points to retail spell IDs: 372608, 372610, 361584, 403092
        TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: Setting OverrideSpellsId to {} for player {}",
            OVERRIDE_SPELL_DATA_SOAR, caster->GetName());

        caster->SetOverrideSpellsId(OVERRIDE_SPELL_DATA_SOAR);

        // Add temporary spells based on what the player has unlocked
        // Core abilities are always available:
        caster->AddTemporarySpell(SPELL_SURGE_FORWARD);
        caster->AddTemporarySpell(SPELL_SKYWARD_ASCENT);

        // CRITICAL: Send LearnedSpells packet to notify client about the new spells
        // Without this, AddTemporarySpell only updates server-side, client doesn't know
        WorldPackets::Spells::LearnedSpells learnedSpells;
        learnedSpells.SuppressMessaging = true; // Don't show "You have learned..." messages

        WorldPackets::Spells::LearnedSpellInfo& spell1 = learnedSpells.ClientLearnedSpellData.emplace_back();
        spell1.SpellID = SPELL_SURGE_FORWARD;

        WorldPackets::Spells::LearnedSpellInfo& spell2 = learnedSpells.ClientLearnedSpellData.emplace_back();
        spell2.SpellID = SPELL_SKYWARD_ASCENT;

        // Whirling Surge requires Airborne Tumbling talent
        if (sDragonridingMgr->HasWhirlingSurge(accountId))
        {
            caster->AddTemporarySpell(SPELL_WHIRLING_SURGE);
            WorldPackets::Spells::LearnedSpellInfo& spell3 = learnedSpells.ClientLearnedSpellData.emplace_back();
            spell3.SpellID = SPELL_WHIRLING_SURGE;
            TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: Player has Airborne Tumbling - added Whirling Surge");
        }

        // Aerial Halt requires At Home Aloft talent
        if (sDragonridingMgr->HasAerialHalt(accountId))
        {
            caster->AddTemporarySpell(SPELL_AERIAL_HALT);
            WorldPackets::Spells::LearnedSpellInfo& spell4 = learnedSpells.ClientLearnedSpellData.emplace_back();
            spell4.SpellID = SPELL_AERIAL_HALT;
            TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: Player has At Home Aloft - added Aerial Halt");
        }

        caster->SendDirectMessage(learnedSpells.Write());

        TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: Added temporary spells and sent LearnedSpells packet (core: {}, {})",
            SPELL_SURGE_FORWARD, SPELL_SKYWARD_ASCENT);

        // Check if the OverrideSpellData exists in the store (for debugging)
        // This should contain retail spell IDs: 372608, 372610, 361584, 403092
        // Log the table hash for troubleshooting hotfix_data entries
        TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: OverrideSpellData table hash: 0x{:X} (update SQL if mismatched!)",
            sOverrideSpellDataStore.GetTableHash());

        if (OverrideSpellDataEntry const* overrideSpells = sOverrideSpellDataStore.LookupEntry(OVERRIDE_SPELL_DATA_SOAR))
        {
            TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: OverrideSpellData {} FOUND! Retail spells: {}, {}, {}, {}",
                OVERRIDE_SPELL_DATA_SOAR,
                overrideSpells->Spells[0], overrideSpells->Spells[1],
                overrideSpells->Spells[2], overrideSpells->Spells[3]);
        }
        else
        {
            TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: OverrideSpellData {} NOT FOUND in DB2 store! "
                "1. Import sql/hotfixes/dragonriding_retail_spells.sql into hotfixes database "
                "2. RESTART worldserver for changes to take effect",
                OVERRIDE_SPELL_DATA_SOAR);
        }

        TC_LOG_ERROR("playerbot.dragonriding", ">>> Soar: COMPLETE - Player {} activated Soar with {} vigor, OverrideSpellsId={}",
            caster->GetName(), maxVigor, OVERRIDE_SPELL_DATA_SOAR);

        // DEBUG: Dump spell requirements for all dragonriding abilities
        // This will help diagnose why "You can't do that yet" appears
        TC_LOG_ERROR("playerbot.dragonriding", ">>> =============================================");
        TC_LOG_ERROR("playerbot.dragonriding", ">>> DEBUG: DUMPING ALL DRAGONRIDING SPELL REQUIREMENTS");
        TC_LOG_ERROR("playerbot.dragonriding", ">>> =============================================");
        DumpSpellRequirements(SPELL_SOAR, caster);
        DumpSpellRequirements(SPELL_DRAGONRIDING, caster);
        DumpSpellRequirements(SPELL_SURGE_FORWARD, caster);
        DumpSpellRequirements(SPELL_SKYWARD_ASCENT, caster);
        DumpSpellRequirements(SPELL_VIGOR, caster);
        DumpSpellRequirements(SPELL_FLIGHT_STYLE_SKYRIDING, caster);
        TC_LOG_ERROR("playerbot.dragonriding", ">>> =============================================");
        TC_LOG_ERROR("playerbot.dragonriding", ">>> DEBUG DUMP COMPLETE - Check above for missing requirements");
        TC_LOG_ERROR("playerbot.dragonriding", ">>> =============================================");
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_playerbot_soar::CheckCast);
        OnCast += SpellCastFn(spell_playerbot_soar::HandleOnCast);
    }
};

// ============================================================================
// AURA: SOAR (369536) - Handles cleanup when Soar ends
// ============================================================================

class spell_playerbot_soar_aura : public AuraScript
{
    void HandleAfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        TC_LOG_ERROR("playerbot.dragonriding", ">>> SOAR AURA REMOVAL TRIGGERED");

        Player* target = GetTarget()->ToPlayer();
        if (!target)
        {
            TC_LOG_ERROR("playerbot.dragonriding", ">>> SOAR AURA REMOVAL: Target is not a player!");
            return;
        }

        TC_LOG_ERROR("playerbot.dragonriding", ">>> SOAR AURA REMOVAL: Cleaning up for player {}", target->GetName());

        // CRITICAL: Remove mounted state first (before disabling flight)
        // This clears UNIT_FLAG_MOUNT that was set when Soar started
        target->Dismount();
        TC_LOG_ERROR("playerbot.dragonriding", ">>> SOAR AURA REMOVAL: Called Dismount() to clear mounted state");

        // Disable dragonriding physics
        target->SetFlightCapabilityID(FLIGHT_CAPABILITY_NORMAL, true);
        TC_LOG_ERROR("playerbot.dragonriding", ">>> SOAR AURA REMOVAL: FlightCapability reset to normal");

        // Reset vigor power type (POWER_ALTERNATE_POWER, not POWER_ALTERNATE_MOUNT)
        target->SetMaxPower(POWER_ALTERNATE_POWER, 0);
        target->SetPower(POWER_ALTERNATE_POWER, 0);
        TC_LOG_ERROR("playerbot.dragonriding", ">>> SOAR AURA REMOVAL: POWER_ALTERNATE_POWER reset to 0");

        // Remove Flight Style: Skyriding (404464) - disables dragonriding abilities
        target->RemoveAura(SPELL_FLIGHT_STYLE_SKYRIDING);
        TC_LOG_ERROR("playerbot.dragonriding", ">>> SOAR AURA REMOVAL: Removed Flight Style: Skyriding");

        // Remove Dragonrider Energy (372773) - required aura for abilities
        target->RemoveAura(SPELL_DRAGONRIDER_ENERGY);
        TC_LOG_ERROR("playerbot.dragonriding", ">>> SOAR AURA REMOVAL: Removed Dragonrider Energy ({})", SPELL_DRAGONRIDER_ENERGY);

        // Restore Flight Style: Steady (404468) - player returns to normal flight mode
        if (!target->HasAura(SPELL_FLIGHT_STYLE_STEADY))
        {
            target->CastSpell(target, SPELL_FLIGHT_STYLE_STEADY, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_FULL_MASK,
                .OriginalCaster = target->GetGUID()
            });
            TC_LOG_ERROR("playerbot.dragonriding", ">>> SOAR AURA REMOVAL: Restored Flight Style: Steady ({})",
                SPELL_FLIGHT_STYLE_STEADY);
        }

        // Remove retail vigor buff (383359)
        target->RemoveAura(SPELL_VIGOR);

        // Remove any active retail buff spells
        target->RemoveAura(SPELL_THRILL_OF_THE_SKIES);    // 383366 - high-speed regen
        target->RemoveAura(SPELL_GROUND_SKIMMING_BUFF);   // 900002 - custom ground skim tracking

        // Reset action bar override using DIRECT approach (matching HandleOnCast)
        TC_LOG_ERROR("playerbot.dragonriding", ">>> SOAR AURA REMOVAL: Resetting OverrideSpellsId to 0");
        target->SetOverrideSpellsId(0);

        // Remove all temporary spells we added in HandleOnCast
        target->RemoveTemporarySpell(SPELL_SURGE_FORWARD);
        target->RemoveTemporarySpell(SPELL_SKYWARD_ASCENT);
        target->RemoveTemporarySpell(SPELL_WHIRLING_SURGE);
        target->RemoveTemporarySpell(SPELL_AERIAL_HALT);

        // Send UnlearnedSpells packet to notify client
        WorldPackets::Spells::UnlearnedSpells unlearnedSpells;
        unlearnedSpells.SpellID.push_back(SPELL_SURGE_FORWARD);
        unlearnedSpells.SpellID.push_back(SPELL_SKYWARD_ASCENT);
        unlearnedSpells.SpellID.push_back(SPELL_WHIRLING_SURGE);
        unlearnedSpells.SpellID.push_back(SPELL_AERIAL_HALT);
        target->SendDirectMessage(unlearnedSpells.Write());

        TC_LOG_ERROR("playerbot.dragonriding", ">>> SOAR AURA REMOVAL: Removed temporary spells and sent UnlearnedSpells packet");

        TC_LOG_ERROR("playerbot.dragonriding", ">>> SOAR AURA REMOVAL COMPLETE for player {}", target->GetName());
    }

    void Register() override
    {
        // Hook into aura removal to clean up - use SPELL_AURA_ANY to match whatever aura type Soar uses
        AfterEffectRemove += AuraEffectRemoveFn(spell_playerbot_soar_aura::HandleAfterRemove, EFFECT_0, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
    }
};

// ============================================================================
// AURA: VIGOR (383359) - Tracks vigor stacks with dynamic maximum
// This uses the RETAIL vigor spell ID for proper client integration
// ============================================================================

class spell_playerbot_vigor_aura : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({
            SPELL_THRILL_OF_THE_SKIES,     // 383366 - retail high-speed buff
            SPELL_GROUND_SKIMMING_BUFF     // 900002 - custom ground skim tracking
        });
    }

    void HandleCalcMaxStackAmount(int32& maxStackAmount)
    {
        Unit* owner = GetUnitOwner();
        if (!owner)
            return;

        Player* player = owner->ToPlayer();
        if (!player)
            return;

        uint32 accountId = GetAccountId(player);
        if (accountId == 0)
            return;

        // Get max vigor from DragonridingMgr based on learned talents
        maxStackAmount = sDragonridingMgr->GetMaxVigor(accountId);
    }

    // Called when the aura is applied - set initial vigor stacks
    void HandleOnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        TC_LOG_ERROR("playerbot.dragonriding", ">>> VIGOR AURA APPLIED!");

        Unit* owner = GetUnitOwner();
        if (!owner)
            return;

        Player* player = owner->ToPlayer();
        if (!player)
            return;

        uint32 accountId = GetAccountId(player);
        if (accountId == 0)
            return;

        // Set initial vigor to max
        uint32 maxVigor = sDragonridingMgr->GetMaxVigor(accountId);
        if (Aura* aura = GetAura())
        {
            aura->SetStackAmount(maxVigor);
            TC_LOG_ERROR("playerbot.dragonriding", ">>> VIGOR AURA: Set {} stacks for player {}",
                maxVigor, player->GetName());
        }
    }

    void OnPeriodicTick(AuraEffect const* /*aurEff*/)
    {
        Unit* owner = GetUnitOwner();
        if (!owner)
            return;

        Player* player = owner->ToPlayer();
        if (!player)
            return;

        uint32 accountId = GetAccountId(player);
        if (accountId == 0)
            return;

        // Check if we should regenerate vigor
        Aura* vigorAura = GetAura();
        if (!vigorAura)
            return;

        uint32 currentStacks = vigorAura->GetStackAmount();
        uint32 maxStacks = sDragonridingMgr->GetMaxVigor(accountId);

        // Don't regenerate if at max
        if (currentStacks >= maxStacks)
            return;

        // Determine regeneration rate based on conditions
        uint32 regenMs = 0;
        bool isFlying = player->IsFlying();
        bool isAdvFlyingActive = player->GetFlightCapabilityID() != 0;

        if (!isAdvFlyingActive)
        {
            // Not in dragonriding mode - no regen
            return;
        }

        if (!isFlying)
        {
            // Grounded regeneration (fastest)
            regenMs = sDragonridingMgr->GetGroundedRegenMs(accountId);

            // Apply Thrill of the Skies buff visual if not present
            if (!player->HasAura(SPELL_THRILL_OF_THE_SKIES))
            {
                player->RemoveAura(SPELL_GROUND_SKIMMING_BUFF);
            }
        }
        else
        {
            // Flying - check speed and height conditions
            float speed = player->GetSpeed(MOVE_FLIGHT);
            float maxSpeed = player->GetSpeedRate(MOVE_FLIGHT);
            float speedPercent = (maxSpeed > 0) ? (speed / maxSpeed) : 0.0f;

            // Check for Thrill of the Skies (high speed) - uses retail spell 383366
            if (speedPercent >= sDragonridingMgr->GetThrillSpeedThreshold())
            {
                regenMs = sDragonridingMgr->GetFlyingRegenMs(accountId);

                if (!player->HasAura(SPELL_THRILL_OF_THE_SKIES))
                {
                    player->CastSpell(player, SPELL_THRILL_OF_THE_SKIES, true);
                    player->RemoveAura(SPELL_GROUND_SKIMMING_BUFF);
                }
            }
            // Check for Ground Skimming (low altitude) - custom tracking spell
            else if (sDragonridingMgr->HasGroundSkimming(accountId))
            {
                float groundZ = player->GetMap()->GetHeight(player->GetPhaseShift(),
                    player->GetPositionX(), player->GetPositionY(), player->GetPositionZ());
                float heightAboveGround = player->GetPositionZ() - groundZ;

                if (heightAboveGround <= sDragonridingMgr->GetGroundSkimHeight())
                {
                    regenMs = BASE_REGEN_GROUND_SKIM_MS;

                    if (!player->HasAura(SPELL_GROUND_SKIMMING_BUFF))
                    {
                        player->CastSpell(player, SPELL_GROUND_SKIMMING_BUFF, true);
                        player->RemoveAura(SPELL_THRILL_OF_THE_SKIES);
                    }
                }
                else
                {
                    // Remove both buffs if not meeting conditions
                    player->RemoveAura(SPELL_THRILL_OF_THE_SKIES);
                    player->RemoveAura(SPELL_GROUND_SKIMMING_BUFF);
                }
            }
            else
            {
                // Remove regen buffs if not meeting conditions
                player->RemoveAura(SPELL_THRILL_OF_THE_SKIES);
                player->RemoveAura(SPELL_GROUND_SKIMMING_BUFF);
            }
        }

        // Regeneration logic handled by periodic effect with correct timing
        // The actual stack increment happens based on accumulated time
    }

    void Register() override
    {
        // SPELL_AURA_DUMMY (4) is the actual aura type for Vigor spell 383359
        // Use AfterEffectApply to set initial vigor stacks when aura is applied
        AfterEffectApply += AuraEffectApplyFn(spell_playerbot_vigor_aura::HandleOnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);

        // Note: SPELL_AURA_DUMMY doesn't have periodic ticks, so vigor regeneration
        // must be handled separately (e.g., via WorldScript or player update hooks)
    }
};

// ============================================================================
// SPELL: SURGE FORWARD (372608) - Speed boost, costs 1 vigor
// Uses RETAIL spell ID - client has all effects/animations
// ============================================================================

class spell_playerbot_surge_forward : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_VIGOR }); // 383359 - retail vigor
    }

    SpellCastResult CheckCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return SPELL_FAILED_BAD_TARGETS;

        // Must be in dragonriding mode
        if (caster->GetFlightCapabilityID() == 0)
            return SPELL_FAILED_NOT_READY;

        // Must have vigor (POWER_ALTERNATE_POWER - the UI power type)
        if (caster->GetPower(POWER_ALTERNATE_POWER) < 1)
            return SPELL_FAILED_NO_POWER;

        return SPELL_CAST_OK;
    }

    void HandleOnCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        // Consume 1 vigor charge (POWER_ALTERNATE_POWER)
        int32 currentVigor = caster->GetPower(POWER_ALTERNATE_POWER);
        if (currentVigor > 0)
        {
            caster->SetPower(POWER_ALTERNATE_POWER, currentVigor - 1);
        }

        if (Player* player = caster->ToPlayer())
        {
            TC_LOG_ERROR("playerbot.dragonriding", "Player {} used Surge Forward, vigor: {} -> {}",
                player->GetName(), currentVigor, currentVigor - 1);
        }
    }

    // =========================================================================
    // NOTE: Movement is handled by CLIENT when FlightCapabilityID is active!
    // =========================================================================
    // The retail dragonriding spells use SPELL_EFFECT_DUMMY, but the actual
    // movement physics are handled by the client's FlightCapability system.
    //
    // When FlightCapabilityID != 0, the client's dragonriding physics engine
    // processes these abilities and applies the appropriate momentum/velocity.
    //
    // Server-side knockback (KnockbackFrom) BREAKS this because it:
    //   1. Sends SMSG_MOVE_KNOCK_BACK which overrides client movement
    //   2. Puts the player in a "knocked" state, canceling flight
    //   3. Causes player to fall from sky
    //
    // The server only needs to:
    //   1. Validate the spell can be cast (CheckCast)
    //   2. Consume vigor (HandleOnCast)
    //   3. Let the client handle movement physics
    // =========================================================================

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_playerbot_surge_forward::CheckCast);
        OnCast += SpellCastFn(spell_playerbot_surge_forward::HandleOnCast);
    }
};

// ============================================================================
// SPELL: SKYWARD ASCENT (372610) - Vertical thrust, costs 1 vigor
// Uses RETAIL spell ID - client has all effects/animations
// ============================================================================

class spell_playerbot_skyward_ascent : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_VIGOR }); // 383359 - retail vigor
    }

    SpellCastResult CheckCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return SPELL_FAILED_BAD_TARGETS;

        // Must be in dragonriding mode
        if (caster->GetFlightCapabilityID() == 0)
            return SPELL_FAILED_NOT_READY;

        // Must have vigor (POWER_ALTERNATE_POWER - the UI power type)
        if (caster->GetPower(POWER_ALTERNATE_POWER) < 1)
            return SPELL_FAILED_NO_POWER;

        return SPELL_CAST_OK;
    }

    void HandleOnCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        // Consume 1 vigor charge (POWER_ALTERNATE_POWER)
        int32 currentVigor = caster->GetPower(POWER_ALTERNATE_POWER);
        if (currentVigor > 0)
        {
            caster->SetPower(POWER_ALTERNATE_POWER, currentVigor - 1);
        }

        if (Player* player = caster->ToPlayer())
        {
            TC_LOG_ERROR("playerbot.dragonriding", "Player {} used Skyward Ascent, vigor: {} -> {}",
                player->GetName(), currentVigor, currentVigor - 1);
        }
    }

    // =========================================================================
    // NOTE: Movement is handled by CLIENT when FlightCapabilityID is active!
    // See spell_playerbot_surge_forward for detailed explanation.
    // =========================================================================

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_playerbot_skyward_ascent::CheckCast);
        OnCast += SpellCastFn(spell_playerbot_skyward_ascent::HandleOnCast);
    }
};

// ============================================================================
// SPELL: WHIRLING SURGE (361584) - Barrel roll, requires Airborne Tumbling talent
// Uses RETAIL spell ID - client has all effects/animations
// ============================================================================

class spell_playerbot_whirling_surge : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_VIGOR }); // 383359 - retail vigor
    }

    SpellCastResult CheckCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return SPELL_FAILED_BAD_TARGETS;

        Player* player = caster->ToPlayer();
        if (!player)
            return SPELL_FAILED_BAD_TARGETS;

        uint32 accountId = GetAccountId(player);
        if (accountId == 0)
            return SPELL_FAILED_ERROR;

        // Must have Airborne Tumbling talent
        if (!sDragonridingMgr->HasWhirlingSurge(accountId))
            return SPELL_FAILED_NOT_KNOWN;

        // Must be in dragonriding mode
        if (caster->GetFlightCapabilityID() == 0)
            return SPELL_FAILED_NOT_READY;

        // Must have vigor (POWER_ALTERNATE_POWER - the UI power type)
        if (caster->GetPower(POWER_ALTERNATE_POWER) < 1)
            return SPELL_FAILED_NO_POWER;

        return SPELL_CAST_OK;
    }

    void HandleOnCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        // Consume 1 vigor charge (POWER_ALTERNATE_POWER)
        int32 currentVigor = caster->GetPower(POWER_ALTERNATE_POWER);
        if (currentVigor > 0)
        {
            caster->SetPower(POWER_ALTERNATE_POWER, currentVigor - 1);
        }

        if (Player* player = caster->ToPlayer())
        {
            TC_LOG_ERROR("playerbot.dragonriding", "Player {} used Whirling Surge, vigor: {} -> {}",
                player->GetName(), currentVigor, currentVigor - 1);
        }
    }

    // =========================================================================
    // NOTE: Movement is handled by CLIENT when FlightCapabilityID is active!
    // See spell_playerbot_surge_forward for detailed explanation.
    // =========================================================================

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_playerbot_whirling_surge::CheckCast);
        OnCast += SpellCastFn(spell_playerbot_whirling_surge::HandleOnCast);
    }
};

// ============================================================================
// SPELL: AERIAL HALT (403092) - Hover in place, requires At Home Aloft talent
// Uses RETAIL spell ID - does NOT consume vigor, just cooldown
// ============================================================================

class spell_playerbot_aerial_halt : public SpellScript
{
    SpellCastResult CheckCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return SPELL_FAILED_BAD_TARGETS;

        Player* player = caster->ToPlayer();
        if (!player)
            return SPELL_FAILED_BAD_TARGETS;

        uint32 accountId = GetAccountId(player);
        if (accountId == 0)
            return SPELL_FAILED_ERROR;

        // Must have At Home Aloft talent
        if (!sDragonridingMgr->HasAerialHalt(accountId))
            return SPELL_FAILED_NOT_KNOWN;

        // Must be in dragonriding mode
        if (caster->GetFlightCapabilityID() == 0)
            return SPELL_FAILED_NOT_READY;

        return SPELL_CAST_OK;
    }

    void HandleOnCast()
    {
        Player* player = GetCaster()->ToPlayer();
        if (!player)
            return;

        // Aerial Halt (403092) stops momentum and allows hovering
        // Note: Does NOT consume vigor, only has 10s cooldown

        TC_LOG_ERROR("playerbot.dragonriding", "Player {} used Aerial Halt", player->GetName());
    }

    // =========================================================================
    // NOTE: Movement is handled by CLIENT when FlightCapabilityID is active!
    // See spell_playerbot_surge_forward for detailed explanation.
    // =========================================================================

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_playerbot_aerial_halt::CheckCast);
        OnCast += SpellCastFn(spell_playerbot_aerial_halt::HandleOnCast);
    }
};

// ============================================================================
// VIGOR REGENERATION HANDLER (for retail SPELL_VIGOR 383359)
// Handles periodic vigor regeneration based on flight conditions
// ============================================================================

class spell_playerbot_vigor_regen : public AuraScript
{
private:
    uint32 _accumulatedTimeMs = 0;

public:
    void HandlePeriodicTick(AuraEffect const* /*aurEff*/)
    {
        Unit* owner = GetUnitOwner();
        if (!owner)
            return;

        Player* player = owner->ToPlayer();
        if (!player)
            return;

        uint32 accountId = GetAccountId(player);
        if (accountId == 0)
            return;

        // Get vigor aura (retail spell 383359)
        Aura* vigorAura = player->GetAura(SPELL_VIGOR);
        if (!vigorAura)
            return;

        uint32 currentStacks = vigorAura->GetStackAmount();
        uint32 maxStacks = sDragonridingMgr->GetMaxVigor(accountId);

        // Don't regenerate if at max
        if (currentStacks >= maxStacks)
        {
            _accumulatedTimeMs = 0;
            return;
        }

        // Determine current regeneration rate
        uint32 regenMs = 0;
        bool isFlying = player->IsFlying();
        bool isAdvFlyingActive = player->GetFlightCapabilityID() != 0;

        if (!isAdvFlyingActive)
        {
            // Not in dragonriding mode
            _accumulatedTimeMs = 0;
            return;
        }

        if (!isFlying)
        {
            // Grounded regeneration
            regenMs = sDragonridingMgr->GetGroundedRegenMs(accountId);
        }
        else
        {
            // Check Thrill of the Skies condition (high speed)
            if (player->HasAura(SPELL_THRILL_OF_THE_SKIES))
            {
                regenMs = sDragonridingMgr->GetFlyingRegenMs(accountId);
            }
            // Check Ground Skimming condition (low altitude)
            else if (player->HasAura(SPELL_GROUND_SKIMMING_BUFF) && sDragonridingMgr->HasGroundSkimming(accountId))
            {
                regenMs = BASE_REGEN_GROUND_SKIM_MS;
            }
            else
            {
                // No regeneration while flying without conditions met
                _accumulatedTimeMs = 0;
                return;
            }
        }

        // Accumulate time (tick happens every VIGOR_UPDATE_INTERVAL_MS)
        _accumulatedTimeMs += VIGOR_UPDATE_INTERVAL_MS;

        // Check if we've accumulated enough time for a vigor point
        if (_accumulatedTimeMs >= regenMs)
        {
            vigorAura->ModStackAmount(1);
            _accumulatedTimeMs -= regenMs;

            TC_LOG_DEBUG("playerbot.dragonriding", "Player {} regenerated 1 vigor (now: {}/{}, rate: {}ms)",
                player->GetName(), vigorAura->GetStackAmount(), maxStacks, regenMs);
        }
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_playerbot_vigor_regen::HandlePeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

// ============================================================================
// SCRIPT REGISTRATION
// ============================================================================

void AddSC_playerbot_dragonriding()
{
    // Soar ability (369536) - Dracthyr racial
    RegisterSpellScript(spell_playerbot_soar);
    RegisterSpellScript(spell_playerbot_soar_aura);

    // Vigor system (383359) - Retail vigor spell
    RegisterSpellScript(spell_playerbot_vigor_aura);
    // NOTE: spell_playerbot_vigor_regen disabled because retail Vigor (383359) uses
    // SPELL_AURA_DUMMY which doesn't have periodic ticks. Vigor regeneration will be
    // implemented via WorldScript or player update hooks in a future update.
    // RegisterSpellScript(spell_playerbot_vigor_regen);

    // Boost abilities using RETAIL spell IDs
    // 372608 = Surge Forward, 372610 = Skyward Ascent
    // 361584 = Whirling Surge, 403092 = Aerial Halt
    RegisterSpellScript(spell_playerbot_surge_forward);
    RegisterSpellScript(spell_playerbot_skyward_ascent);
    RegisterSpellScript(spell_playerbot_whirling_surge);
    RegisterSpellScript(spell_playerbot_aerial_halt);

    TC_LOG_ERROR("server.loading", ">>> PLAYERBOT DRAGONRIDING: Registered with RETAIL spell IDs:");
    TC_LOG_ERROR("server.loading", ">>>   Soar: {}, Vigor: {}", SPELL_SOAR, SPELL_VIGOR);
    TC_LOG_ERROR("server.loading", ">>>   Surge Forward: {}, Skyward Ascent: {}", SPELL_SURGE_FORWARD, SPELL_SKYWARD_ASCENT);
    TC_LOG_ERROR("server.loading", ">>>   Whirling Surge: {}, Aerial Halt: {}", SPELL_WHIRLING_SURGE, SPELL_AERIAL_HALT);

    // DEBUG: Check if required spells exist in SpellMgr
    TC_LOG_ERROR("server.loading", ">>> PLAYERBOT DRAGONRIDING: Checking if spells exist in SpellMgr...");
    TC_LOG_ERROR("server.loading", ">>>   Soar ({}): {}",
        SPELL_SOAR, sSpellMgr->GetSpellInfo(SPELL_SOAR, DIFFICULTY_NONE) ? "EXISTS" : "MISSING");
    TC_LOG_ERROR("server.loading", ">>>   Vigor ({}): {}",
        SPELL_VIGOR, sSpellMgr->GetSpellInfo(SPELL_VIGOR, DIFFICULTY_NONE) ? "EXISTS" : "MISSING");
    TC_LOG_ERROR("server.loading", ">>>   Surge Forward ({}): {}",
        SPELL_SURGE_FORWARD, sSpellMgr->GetSpellInfo(SPELL_SURGE_FORWARD, DIFFICULTY_NONE) ? "EXISTS" : "MISSING");
    TC_LOG_ERROR("server.loading", ">>>   Skyward Ascent ({}): {}",
        SPELL_SKYWARD_ASCENT, sSpellMgr->GetSpellInfo(SPELL_SKYWARD_ASCENT, DIFFICULTY_NONE) ? "EXISTS" : "MISSING");
    TC_LOG_ERROR("server.loading", ">>>   Whirling Surge ({}): {}",
        SPELL_WHIRLING_SURGE, sSpellMgr->GetSpellInfo(SPELL_WHIRLING_SURGE, DIFFICULTY_NONE) ? "EXISTS" : "MISSING");
    TC_LOG_ERROR("server.loading", ">>>   Aerial Halt ({}): {}",
        SPELL_AERIAL_HALT, sSpellMgr->GetSpellInfo(SPELL_AERIAL_HALT, DIFFICULTY_NONE) ? "EXISTS" : "MISSING");
    TC_LOG_ERROR("server.loading", ">>>   Flight Style: Skyriding ({}): {}",
        SPELL_FLIGHT_STYLE_SKYRIDING, sSpellMgr->GetSpellInfo(SPELL_FLIGHT_STYLE_SKYRIDING, DIFFICULTY_NONE) ? "EXISTS" : "MISSING");
    TC_LOG_ERROR("server.loading", ">>>   Dragonrider Energy ({}): {} <-- CRITICAL: Required by Surge Forward & Skyward Ascent!",
        SPELL_DRAGONRIDER_ENERGY, sSpellMgr->GetSpellInfo(SPELL_DRAGONRIDER_ENERGY, DIFFICULTY_NONE) ? "EXISTS" : "MISSING");

    // DEBUG: Log the actual effect types of Vigor spell (383359) to find correct aura type
    if (SpellInfo const* vigorInfo = sSpellMgr->GetSpellInfo(SPELL_VIGOR, DIFFICULTY_NONE))
    {
        TC_LOG_ERROR("server.loading", ">>> PLAYERBOT DRAGONRIDING: Vigor spell {} effects:", SPELL_VIGOR);
        for (SpellEffectInfo const& effectInfo : vigorInfo->GetEffects())
        {
            TC_LOG_ERROR("server.loading", ">>>   Effect {}: Type={}, AuraType={}, Amplitude={}",
                effectInfo.EffectIndex,
                uint32(effectInfo.Effect),
                uint32(effectInfo.ApplyAuraName),
                effectInfo.ApplyAuraPeriod);
        }
    }

    // DEBUG: Log the actual effect types of Soar spell (369536)
    if (SpellInfo const* soarInfo = sSpellMgr->GetSpellInfo(SPELL_SOAR, DIFFICULTY_NONE))
    {
        TC_LOG_ERROR("server.loading", ">>> PLAYERBOT DRAGONRIDING: Soar spell {} effects:", SPELL_SOAR);
        for (SpellEffectInfo const& effectInfo : soarInfo->GetEffects())
        {
            TC_LOG_ERROR("server.loading", ">>>   Effect {}: Type={}, AuraType={}, Amplitude={}",
                effectInfo.EffectIndex,
                uint32(effectInfo.Effect),
                uint32(effectInfo.ApplyAuraName),
                effectInfo.ApplyAuraPeriod);
        }
    }

    // Check if OverrideSpellData is loaded (should contain retail spell IDs)
    if (OverrideSpellDataEntry const* overrideSpells = sOverrideSpellDataStore.LookupEntry(OVERRIDE_SPELL_DATA_SOAR))
    {
        TC_LOG_ERROR("server.loading", ">>> PLAYERBOT DRAGONRIDING: OverrideSpellData {} LOADED! Retail spells: {}, {}, {}, {}",
            OVERRIDE_SPELL_DATA_SOAR,
            overrideSpells->Spells[0], overrideSpells->Spells[1],
            overrideSpells->Spells[2], overrideSpells->Spells[3]);
    }
    else
    {
        TC_LOG_ERROR("server.loading", ">>> PLAYERBOT DRAGONRIDING: OverrideSpellData {} NOT IN DB2 STORE! "
            "Import sql/hotfixes/dragonriding_retail_spells.sql into hotfixes database and restart!",
            OVERRIDE_SPELL_DATA_SOAR);
    }

    // Get the correct UnitPowerBar table hash dynamically
    uint32 unitPowerBarHash = sUnitPowerBarStore.GetTableHash();
    TC_LOG_ERROR("server.loading", ">>> PLAYERBOT DRAGONRIDING: UnitPowerBar table hash: 0x{:X}", unitPowerBarHash);

    // Check if UnitPowerBar 631 (Vigor) is loaded correctly
    if (UnitPowerBarEntry const* vigorBar = sUnitPowerBarStore.LookupEntry(631))
    {
        TC_LOG_ERROR("server.loading", ">>> PLAYERBOT DRAGONRIDING: UnitPowerBar 631 (Vigor) LOADED!");
        TC_LOG_ERROR("server.loading", ">>>   Name: {}, MinPower: {}, MaxPower: {}, StartPower: {}",
            vigorBar->Name.Str[LOCALE_enUS], vigorBar->MinPower, vigorBar->MaxPower, vigorBar->StartPower);
        TC_LOG_ERROR("server.loading", ">>>   BarType: {}, Flags: {}", vigorBar->BarType, vigorBar->Flags);

        // Dynamically register the hotfix using the correct table hash
        // This replaces the need for SQL hotfix_data with hardcoded hash
        DB2HotfixGeneratorBase::AddClientHotfix(unitPowerBarHash, 631);
        TC_LOG_ERROR("server.loading", ">>> PLAYERBOT DRAGONRIDING: Registered hotfix for UnitPowerBar 631 with hash 0x{:X}", unitPowerBarHash);
    }
    else
    {
        TC_LOG_ERROR("server.loading", ">>> PLAYERBOT DRAGONRIDING: WARNING - UnitPowerBar 631 (Vigor) NOT FOUND!");
        TC_LOG_ERROR("server.loading", ">>>   Import sql/hotfixes/dragonriding_vigor_powerbar.sql into hotfixes database!");
        TC_LOG_ERROR("server.loading", ">>>   The vigor UI will NOT display without this entry!");
    }
}
