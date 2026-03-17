/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BGScriptInit.h"
#include "BGScriptRegistry.h"
#include "Log.h"

// Include ALL BG script headers for explicit registration
// This replaces the broken ForceInclude/REGISTER_BG_SCRIPT static-init pattern
// which was dead-stripped by the MSVC linker for static libraries.
#include "CTF/WarsongGulchScript.h"
#include "CTF/TwinPeaksScript.h"
#include "Domination/ArathiBasinScript.h"
#include "Domination/EyeOfTheStormScript.h"
#include "Domination/BattleForGilneasScript.h"
#include "Domination/TempleOfKotmoguScript.h"
#include "Domination/DeepwindGorgeScript.h"
#include "Domination/SeethingShoreScript.h"
#include "Siege/AlteracValleyScript.h"
#include "Siege/IsleOfConquestScript.h"
#include "Siege/StrandOfTheAncientsScript.h"
#include "ResourceRace/SilvershardMinesScript.h"
#include "Epic/AshranScript.h"

namespace Playerbot::Coordination::Battleground
{

void InitializeBGScripts()
{
    TC_LOG_INFO("playerbots.bg.script", "InitializeBGScripts: Registering BG scripts...");

    auto& registry = BGScriptRegistry::Instance();

    // ========================================================================
    // EXPLICIT REGISTRATION - bypasses MSVC static library linker dead-stripping
    //
    // The REGISTER_BG_SCRIPT macro (in each .cpp file) uses static global
    // constructors for auto-registration. MSVC's linker discards object files
    // from static libraries when no external symbol is referenced. The previous
    // ForceInclude/volatile-pointer pattern did not reliably prevent this.
    //
    // This explicit registration is the PRIMARY registration mechanism.
    // The REGISTER_BG_SCRIPT macros in .cpp files serve as documentation only.
    // ========================================================================

    // CTF Battlegrounds
    registry.RegisterScript(489, []() -> std::unique_ptr<IBGScript> {
        return std::make_unique<WarsongGulchScript>();
    }, "WarsongGulchScript");

    registry.RegisterScript(2106, []() -> std::unique_ptr<IBGScript> {
        return std::make_unique<WarsongGulchScript>();
    }, "WarsongGulchScript (Remake)");

    registry.RegisterScript(726, []() -> std::unique_ptr<IBGScript> {
        return std::make_unique<TwinPeaksScript>();
    }, "TwinPeaksScript");

    // Domination Battlegrounds
    registry.RegisterScript(529, []() -> std::unique_ptr<IBGScript> {
        return std::make_unique<ArathiBasinScript>();
    }, "ArathiBasinScript");

    registry.RegisterScript(566, []() -> std::unique_ptr<IBGScript> {
        return std::make_unique<EyeOfTheStormScript>();
    }, "EyeOfTheStormScript");

    registry.RegisterScript(761, []() -> std::unique_ptr<IBGScript> {
        return std::make_unique<BattleForGilneasScript>();
    }, "BattleForGilneasScript");

    registry.RegisterScript(998, []() -> std::unique_ptr<IBGScript> {
        return std::make_unique<TempleOfKotmoguScript>();
    }, "TempleOfKotmoguScript");

    registry.RegisterScript(1105, []() -> std::unique_ptr<IBGScript> {
        return std::make_unique<DeepwindGorgeScript>();
    }, "DeepwindGorgeScript");

    registry.RegisterScript(1803, []() -> std::unique_ptr<IBGScript> {
        return std::make_unique<SeethingShoreScript>();
    }, "SeethingShoreScript");

    // Siege Battlegrounds
    registry.RegisterScript(30, []() -> std::unique_ptr<IBGScript> {
        return std::make_unique<AlteracValleyScript>();
    }, "AlteracValleyScript");

    registry.RegisterScript(628, []() -> std::unique_ptr<IBGScript> {
        return std::make_unique<IsleOfConquestScript>();
    }, "IsleOfConquestScript");

    registry.RegisterScript(607, []() -> std::unique_ptr<IBGScript> {
        return std::make_unique<StrandOfTheAncientsScript>();
    }, "StrandOfTheAncientsScript");

    // Resource Race Battlegrounds
    registry.RegisterScript(727, []() -> std::unique_ptr<IBGScript> {
        return std::make_unique<SilvershardMinesScript>();
    }, "SilvershardMinesScript");

    // Epic Battlegrounds
    registry.RegisterScript(1191, []() -> std::unique_ptr<IBGScript> {
        return std::make_unique<AshranScript>();
    }, "AshranScript");

    // Log registered scripts
    TC_LOG_INFO("playerbots.bg.script",
        "InitializeBGScripts: {} BG scripts registered", registry.GetScriptCount());

    registry.LogRegisteredScripts();
}

} // namespace Playerbot::Coordination::Battleground
