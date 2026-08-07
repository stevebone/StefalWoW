/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* ScriptData
Name: garrison_commandscript
%Complete: 100
Comment: GM/dev commands for the garrison (force site-level upgrade, bypassing the client blueprint/quest gate)
Category: commandscripts
EndScriptData */

#include "ScriptMgr.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "DB2Stores.h"
#include "DB2Structure.h"
#include "AbominationFactory.h"
#include "EmberCourt.h"
#include "Garrison.h"
#include "GarrisonMgr.h"
#include "Optional.h"
#include "PathOfAscension.h"
#include "QueensConservatory.h"
#include "Player.h"
#include "RBAC.h"
#include "SharedDefines.h"
#include "WorldSession.h"

#include <iterator>
#include <sstream>
#include <unordered_map>
#include <vector>

using namespace Trinity::ChatCommands;

// Per-class order-hall entry location for the .garrison enter dev command. Map + a teleport point near each hall's
// mission table. Coordinates verified from game_tele / hall-leader creature spawns; classes not yet filled decline
// gracefully. (Legion class halls are a mix of phased continent locations and separate instance maps.)
struct ClassHallLocation { uint32 Map; float X; float Y; float Z; char const* Name; };
static std::unordered_map<uint8 /*Classes*/, ClassHallLocation> const ClassHallLocations =
{
    { CLASS_WARRIOR,      { 1479,  1028.47f,  7225.17f,  100.18f, "Skyhold" } },
    { CLASS_PALADIN,      { 1220,  -817.16f,  4391.80f,  739.24f, "Sanctum of Light" } },
    { CLASS_HUNTER,       { 1220,  4632.03f,  5320.96f,  852.01f, "Trueshot Lodge" } },
    { CLASS_ROGUE,        { 1220,  -927.24f,  4501.06f,  700.75f, "Hall of Shadows" } },
    { CLASS_PRIEST,       { 1512,  1333.91f,  1335.63f,  177.22f, "Netherlight Temple" } },
    { CLASS_DEATH_KNIGHT, { 1220, -1502.45f,  1060.68f,  260.42f, "Acherus" } },
    { CLASS_SHAMAN,       {  730,   851.31f,  1067.76f,  -10.02f, "The Maelstrom" } },
    { CLASS_MAGE,         { 1513,  -844.00f,  4759.00f,  918.00f, "Hall of the Guardian" } },
    { CLASS_WARLOCK,      { 1107,  3094.91f,  1062.63f,  242.58f, "Dreadscar Rift" } },
    { CLASS_MONK,         { 1514,   885.31f,  3605.37f,  192.23f, "Temple of Five Dawns" } },
    { CLASS_DRUID,        { 1220,  4411.15f,  7164.21f,  350.00f, "The Dreamgrove" } },
    { CLASS_DEMON_HUNTER, { 1519,  1561.02f,  1399.94f,  237.11f, "The Fel Hammer" } },
};

// Dev commands for the WoD garrison. The retail upgrade path (Garrison Architect table) only unlocks the
// upgrade button once the client knows the "Garrison Blueprint: Level N" and the prerequisite quests are
// done; that client gate is not satisfied by completing the quests alone. This command drives the same
// server logic the architect handler uses (Garrison::Upgrade) directly, so a developer can level a garrison
// up on demand to test higher-level plots/buildings. Buildings are preserved by plot instance id.
class garrison_commandscript : public CommandScript
{
public:
    garrison_commandscript() : CommandScript("garrison_commandscript") { }

    std::span<ChatCommandBuilder const> GetCommands() const override
    {
        static ChatCommandTable conservatoryCommandTable =
        {
            { "status",   HandleConservatoryStatusCommand,   rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "plant",    HandleConservatoryPlantCommand,    rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "catalyst", HandleConservatoryCatalystCommand, rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "harvest",  HandleConservatoryHarvestCommand,  rbac::RBAC_PERM_COMMAND_GM, Console::No },
        };

        static ChatCommandTable abominationCommandTable =
        {
            { "status", HandleAbominationStatusCommand, rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "sync",   HandleAbominationSyncCommand,   rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "build",  HandleAbominationBuildCommand,  rbac::RBAC_PERM_COMMAND_GM, Console::No },
        };

        static ChatCommandTable ascensionCommandTable =
        {
            { "status",   HandleAscensionStatusCommand,   rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "capture",  HandleAscensionCaptureCommand,  rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "start",    HandleAscensionStartCommand,    rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "complete", HandleAscensionCompleteCommand, rbac::RBAC_PERM_COMMAND_GM, Console::No },
        };

        static ChatCommandTable emberCourtCommandTable =
        {
            { "status",   HandleEmberCourtStatusCommand,   rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "guests",   HandleEmberCourtGuestsCommand,   rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "invite",   HandleEmberCourtInviteCommand,   rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "uninvite", HandleEmberCourtUninviteCommand, rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "start",    HandleEmberCourtStartCommand,    rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "complete", HandleEmberCourtCompleteCommand, rbac::RBAC_PERM_COMMAND_GM, Console::No },
        };

        static ChatCommandTable garrisonCommandTable =
        {
            { "upgrade",   HandleGarrisonUpgradeCommand,   rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "create",    HandleGarrisonCreateCommand,    rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "champions", HandleGarrisonChampionsCommand, rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "enter",     HandleGarrisonEnterCommand,     rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "exit",      HandleGarrisonExitCommand,      rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "resettalents", HandleGarrisonResetTalentsCommand, rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "conservatory", conservatoryCommandTable },
            { "abomination",  abominationCommandTable },
            { "ascension",    ascensionCommandTable },
            { "embercourt",   emberCourtCommandTable },
        };

        static ChatCommandTable commandTable =
        {
            { "garrison", garrisonCommandTable },
        };
        return commandTable;
    }

    // .garrison upgrade   - advance the selected player's WoD garrison one site level (repeatable to max).
    static bool HandleGarrisonUpgradeCommand(ChatHandler* handler)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (handler->HasLowerSecurity(target, ObjectGuid::Empty))
            return false;

        Garrison* garrison = target->GetGarrison();
        if (!garrison)
        {
            handler->PSendSysMessage("{} has no WoD garrison.", target->GetName());
            handler->SetSentErrorMessage(true);
            return false;
        }

        GarrSiteLevelEntry const* before = garrison->GetSiteLevel();
        uint32 beforeLevel = before ? before->GarrLevel : 0;

        garrison->Upgrade();

        GarrSiteLevelEntry const* after = garrison->GetSiteLevel();
        uint32 afterLevel = after ? after->GarrLevel : 0;

        if (afterLevel <= beforeLevel)
        {
            handler->PSendSysMessage("Garrison is already at its maximum site level ({}).", beforeLevel);
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("Upgraded {}'s garrison: level {} -> {} (GarrSiteLevel {}). Buildings preserved.",
            target->GetName(), beforeLevel, afterLevel, after ? after->ID : 0);
        return true;
    }

    // .garrison create <siteId>   - create a garrison of the given GarrSite id for the selected player.
    //   Legion Hunter Order Hall (Trueshot Lodge) = 161; WoD Alliance = 2, WoD Horde = 71.
    static bool HandleGarrisonCreateCommand(ChatHandler* handler, uint32 garrSiteId)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }
        if (handler->HasLowerSecurity(target, ObjectGuid::Empty))
            return false;

        target->CreateGarrison(garrSiteId);

        // Resolve which garrison (by type) the new site produced, so we can confirm it and report the map.
        Garrison* created = nullptr;
        for (GarrisonType t : { GARRISON_TYPE_GARRISON, GARRISON_TYPE_CLASS_ORDER, GARRISON_TYPE_WAR_CAMPAIGN, GARRISON_TYPE_COVENANT })
            if (Garrison* g = target->GetGarrison(t))
                if (g->GetSiteLevel() && g->GetSiteLevel()->GarrSiteID == garrSiteId)
                    created = g;

        if (!created)
        {
            handler->PSendSysMessage("Failed to create a garrison for GarrSite {} (unknown/invalid site id).", garrSiteId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("Created garrison for {}: GarrSite {} -> type {}, map {}.",
            target->GetName(), garrSiteId, uint32(created->GetType()), created->GetSiteLevel()->MapID);
        return true;
    }

    // .garrison champions   - grant the Hunter Order Hall starting champions to the class-order garrison and seed
    // its mission board. Create the hall first with ".garrison create 161".
    static bool HandleGarrisonChampionsCommand(ChatHandler* handler)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }
        if (handler->HasLowerSecurity(target, ObjectGuid::Empty))
            return false;

        Garrison* garrison = target->GetGarrison(GARRISON_TYPE_CLASS_ORDER);
        if (!garrison)
        {
            handler->PSendSysMessage("{} has no Order Hall - create it first with '.garrison create 161'.", target->GetName());
            handler->SetSentErrorMessage(true);
            return false;
        }

        // Hunter "Unseen Path" champions (GarrFollower ids, GarrType 3). Emmarel Shadewarden (593) is the hall leader.
        static constexpr uint32 hunterChampions[] = { 593, 642, 742, 743, 744, 745, 746, 747, 748 };
        for (uint32 followerId : hunterChampions)
            garrison->AddFollower(followerId);

        garrison->GenerateAvailableMissions();

        handler->PSendSysMessage("Granted {} Hunter champions and seeded missions to {}'s Order Hall.",
            uint32(std::size(hunterChampions)), target->GetName());
        return true;
    }

    // .garrison enter   - teleport the selected player to THEIR class's order hall (data-driven per class). Retail
    // reaches the hall via the Dalaran Order Hall portal; this teleports straight there.
    static bool HandleGarrisonEnterCommand(ChatHandler* handler)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        auto itr = ClassHallLocations.find(target->GetClass());
        if (itr == ClassHallLocations.end())
        {
            handler->PSendSysMessage("No order-hall entry location is configured for {}'s class yet.", target->GetName());
            handler->SetSentErrorMessage(true);
            return false;
        }

        ClassHallLocation const& loc = itr->second;
        target->TeleportTo(loc.Map, loc.X, loc.Y, loc.Z, 0.0f);
        handler->PSendSysMessage("Teleported {} to {} (order hall).", target->GetName(), loc.Name);
        return true;
    }

    // .garrison exit   - return from the Order Hall to Dalaran (Broken Isles).
    // .garrison resettalents <garrTalentTreeID>  - wipe every talent the selected player has in the
    //   given GarrTalentTree and push the reset to the client. The tree must belong to a garrison
    //   type the player actually owns. There is no client request for this in 12.0.7, so a GM
    //   command is the only trigger.
    static bool HandleGarrisonResetTalentsCommand(ChatHandler* handler, uint32 garrTalentTreeID)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (handler->HasLowerSecurity(target, ObjectGuid::Empty))
            return false;

        GarrTalentTreeEntry const* treeEntry = sGarrTalentTreeStore.LookupEntry(garrTalentTreeID);
        if (!treeEntry)
        {
            handler->PSendSysMessage("GarrTalentTree {} does not exist.", garrTalentTreeID);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Garrison* garrison = target->GetGarrison(GarrisonType(treeEntry->GarrTypeID));
        if (!garrison)
        {
            handler->PSendSysMessage("{} has no garrison of type {} (required by GarrTalentTree {}).",
                target->GetName(), uint32(treeEntry->GarrTypeID), garrTalentTreeID);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (garrison->ResetTalentTree(garrTalentTreeID) != GARRISON_SUCCESS)
        {
            handler->PSendSysMessage("{} has no researched talents in GarrTalentTree {}.", target->GetName(), garrTalentTreeID);
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("Reset GarrTalentTree {} for {} (garrison type {}).",
            garrTalentTreeID, target->GetName(), uint32(treeEntry->GarrTypeID));
        return true;
    }

    static bool HandleGarrisonExitCommand(ChatHandler* handler)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        target->TeleportTo(1220, -849.908f, 4461.17f, 735.661f, 0.0f);
        handler->PSendSysMessage("Returned {} to Dalaran.", target->GetName());
        return true;
    }

    // ---------------------------------------------------------------------------------------------------
    // Queen's Conservatory (Night Fae unique sanctum feature). The 12.0.7 client has no CMSG for planting or
    // harvesting - C_ArdenwealdGardening exposes only GetGardenData/IsGardenAccessible and no mutators - and
    // neither the retail Wildseed of Regrowth (creature 165466), the Anima Catalyst Plot (creature 165480)
    // nor the reward chest "Queen's Conservatory Cache" (GameObject 350978) has a spawn in this world DB. So,
    // exactly like .garrison resettalents, a GM command is the only trigger the engine can be driven from.
    //
    // NB: GameObjects 353652/353653/353654 "Catalyst of Power/Renewal/Might" are NOT the Conservatory
    // catalysts - they are Revendreth vampire-bottle props (displayIds 64892-64894 ->
    // world/expansion08/doodads/vampire/9vm_vampire_bottle*.m2, AreaTable 10413 on map 2222; the
    // Conservatory is AreaTable 13367 on map 2363). The real catalysts are the items 176921 / 176922 /
    // 176832, which is what `.garrison conservatory catalyst` now takes; see QueensConservatory.h.
    // ---------------------------------------------------------------------------------------------------

    static QueensConservatory* GetConservatoryFor(ChatHandler* handler, Player* target)
    {
        Garrison* garrison = target->GetGarrison(GARRISON_TYPE_COVENANT);
        if (!garrison)
        {
            handler->PSendSysMessage("{} has no covenant sanctum (GarrType 111).", target->GetName());
            handler->SetSentErrorMessage(true);
            return nullptr;
        }

        return &garrison->GetConservatory();
    }

    static char const* ConservatoryErrorText(ConservatoryError error)
    {
        switch (error)
        {
            case CONSERVATORY_OK:                        return "ok";
            case CONSERVATORY_ERROR_NOT_NIGHT_FAE:       return "the character is not pledged to the Night Fae";
            case CONSERVATORY_ERROR_NOT_UNLOCKED:        return "GarrTalentTree 319 tier 1 (talent 1086 'First Planting') is not researched";
            case CONSERVATORY_ERROR_INVALID_PLOT:        return "no such wildseed plot at the current Conservatory tier";
            case CONSERVATORY_ERROR_PLOT_OCCUPIED:       return "that plot already holds a wildseed";
            case CONSERVATORY_ERROR_PLOT_EMPTY:          return "that plot is empty";
            case CONSERVATORY_ERROR_NOT_READY:           return "that wildseed has not finished maturing";
            case CONSERVATORY_ERROR_UNKNOWN_WILDSEED:    return "no `garrison_conservatory_wildseed` row with that entry";
            case CONSERVATORY_ERROR_NO_WILDSEED_DATA:    return "world table `garrison_conservatory_wildseed` is empty (or the row has maturationSeconds 0) - the maturation time and plant cost are not published by any 12.0.7 client data and must be authored";
            case CONSERVATORY_ERROR_TIER_TOO_LOW:        return "that wildseed needs more Conservatory tiers researched";
            case CONSERVATORY_ERROR_CANT_AFFORD:         return "the character cannot pay the wildseed's cost";
            case CONSERVATORY_ERROR_INVALID_CATALYST:    return "no `garrison_conservatory_catalyst` row with that item id (shipped set: 176921 Temporal Leaves, 176922 Wild Nightbloom, 176832 Wildseed Root Grain)";
            case CONSERVATORY_ERROR_CATALYST_SLOT_TAKEN: return "that pod's catalyst links are all used up (caps per pod are 1/2/2/2/3/4)";
            case CONSERVATORY_ERROR_NO_CATALYST_PLOTS:   return "catalyst plots need 2 researched tiers (talent 1087 'Initial Growth')";
            case CONSERVATORY_ERROR_NO_CATALYST_DATA:    return "world table `garrison_conservatory_catalyst` is empty - apply 2026_08_07_63_covenant_conservatory_catalysts.sql";
            case CONSERVATORY_ERROR_CATALYST_LIMIT:      return "that catalyst's own maxPerPlot is already reached on this pod";
            case CONSERVATORY_ERROR_NO_CATALYST_ITEM:    return "the character does not carry that catalyst item";
            case CONSERVATORY_ERROR_NO_YIELD_FOR_COMBINATION:
                                                         return "no `garrison_conservatory_yield` row for the resulting catalyst set - that combination's reward satchel does not exist in this build (there is no Artisan's Overflowing and no Spirit-Tender's Large/Stuffed/Overflowing Satchel), so the link is refused rather than left unharvestable";
            case CONSERVATORY_ERROR_NO_LOOT_TEMPLATE:    return "the loot table this harvest resolves to has no rows in `gameobject_loot_template`";
            default:                                     return "unknown error";
        }
    }

    // .garrison conservatory status  - plot count, per-plot state, and the three numbers the client's
    //   C_ArdenwealdGardening.GetGardenData() will report.
    static bool HandleConservatoryStatusCommand(ChatHandler* handler)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        QueensConservatory* conservatory = GetConservatoryFor(handler, target);
        if (!conservatory)
            return false;

        uint32 active = 0;
        uint32 ready = 0;
        int64 remaining = 0;
        conservatory->GetGardenData(active, ready, remaining);

        handler->PSendSysMessage("Queen's Conservatory for {}: accessible {}, {} researched tier(s) -> {} wildseed pod(s), catalyst plots {}.",
            target->GetName(), conservatory->IsAccessible() ? "yes" : "no", conservatory->GetResearchedTiers(),
            conservatory->GetPlotCount(), conservatory->HasCatalystPlots() ? "yes" : "no");
        handler->PSendSysMessage("GetGardenData(): active {}, ready {}, remainingSeconds {}.", active, ready, remaining);

        for (ConservatoryPlot const* plot : conservatory->GetPlots())
        {
            char const* state = plot->State == CONSERVATORY_PLOT_GROWING ? "growing"
                : (plot->State == CONSERVATORY_PLOT_READY ? "READY" : "empty");

            uint8 rootGrainCount = 0;
            uint8 nightbloomCount = 0;
            conservatory->GetCatalystCounts(*plot, rootGrainCount, nightbloomCount);

            handler->PSendSysMessage("  pod {}: {} (wildseed {}, matures at {}, {}/{} catalyst link(s): "
                "{} quality + {} quantity)",
                uint32(plot->PlotId), state, plot->WildseedEntry, int64(plot->MaturesAt), plot->CountCatalysts(),
                uint32(QueensConservatory::GetCatalystLinkCap(plot->PlotId)), uint32(rootGrainCount), uint32(nightbloomCount));

            // Show the loot table this pod would actually roll, so the command can never imply a payout that
            // HarvestWildseed would refuse.
            if (plot->IsOccupied())
            {
                uint32 lootId = 0;
                if (ConservatoryError error = conservatory->ResolveHarvestLootId(*plot, lootId))
                    handler->PSendSysMessage("    harvest would REFUSE: {}.", ConservatoryErrorText(error));
                else
                    handler->PSendSysMessage("    harvest rolls gameobject_loot_template {}.", lootId);
            }
        }

        if (sGarrisonMgr.GetConservatoryWildseeds().empty())
            handler->SendSysMessage("NOTE: `garrison_conservatory_wildseed` is empty, so planting is disabled. "
                "Maturation time and plant cost have no source in any 12.0.7 DB2 or in the world DB; they must be authored.");

        if (sGarrisonMgr.GetConservatoryCatalysts().empty())
            handler->SendSysMessage("NOTE: `garrison_conservatory_catalyst` is empty, so linking catalysts is "
                "disabled. Apply 2026_08_07_63_covenant_conservatory_catalysts.sql.");
        else
        {
            std::ostringstream known;
            for (auto const& [itemId, catalyst] : sGarrisonMgr.GetConservatoryCatalysts())
            {
                char const* effect = catalyst.EffectType == CONSERVATORY_CATALYST_EFFECT_TIME_DELTA ? "TIME_DELTA"
                    : (catalyst.EffectType == CONSERVATORY_CATALYST_EFFECT_YIELD_QUALITY ? "YIELD_QUALITY" : "YIELD_QUANTITY");
                known << ' ' << itemId << '(' << effect;
                if (catalyst.EffectType == CONSERVATORY_CATALYST_EFFECT_TIME_DELTA)
                    known << ' ' << catalyst.EffectValue << 's';
                known << ", max " << uint32(catalyst.MaxPerPlot) << ')';
            }
            handler->PSendSysMessage("Catalyst items:{}", known.str());
        }

        if (sGarrisonMgr.GetConservatoryYields().empty())
            handler->SendSysMessage("NOTE: `garrison_conservatory_yield` is empty, so a harvest falls back to the "
                "wildseed's reward chest (gameobject_loot_template 350978), which flattens every catalyst outcome "
                "into one table.");

        return true;
    }

    // .garrison conservatory plant <plotId> <wildseedEntry>
    static bool HandleConservatoryPlantCommand(ChatHandler* handler, uint8 plotId, uint32 wildseedEntry)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        QueensConservatory* conservatory = GetConservatoryFor(handler, target);
        if (!conservatory)
            return false;

        ConservatoryError result = conservatory->PlantWildseed(plotId, wildseedEntry);
        if (result != CONSERVATORY_OK)
        {
            handler->PSendSysMessage("Could not plant: {}.", ConservatoryErrorText(result));
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("Planted wildseed {} on plot {} for {}.", wildseedEntry, uint32(plotId), target->GetName());
        return true;
    }

    // .garrison conservatory catalyst <plotId> <catalystItemId>
    //   Links a catalyst ITEM to the wildseed growing in pod <plotId>: 176921 Temporal Leaves (-1 day),
    //   176922 Wild Nightbloom (bigger satchel), 176832 Wildseed Root Grain (better satchel). The item is
    //   consumed from the character's bags, so it has to be there. There is no slot argument any more - the
    //   next free link on the pod is used, and how many links a pod has is fixed by its position (1/2/2/2/3/4).
    static bool HandleConservatoryCatalystCommand(ChatHandler* handler, uint8 plotId, uint32 catalystItemId)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        QueensConservatory* conservatory = GetConservatoryFor(handler, target);
        if (!conservatory)
            return false;

        ConservatoryError result = conservatory->AttachCatalyst(plotId, catalystItemId);
        if (result != CONSERVATORY_OK)
        {
            handler->PSendSysMessage("Could not attach catalyst: {}.", ConservatoryErrorText(result));
            handler->SetSentErrorMessage(true);
            return false;
        }

        ConservatoryPlot const* plot = conservatory->GetPlot(plotId);
        handler->PSendSysMessage("Linked catalyst item {} to pod {} for {} - item consumed, {}/{} link(s) used, "
            "matures at {}.", catalystItemId, uint32(plotId), target->GetName(),
            plot ? plot->CountCatalysts() : 0, uint32(QueensConservatory::GetCatalystLinkCap(plotId)),
            plot ? int64(plot->MaturesAt) : 0);
        return true;
    }

    // .garrison conservatory harvest <plotId>
    static bool HandleConservatoryHarvestCommand(ChatHandler* handler, uint8 plotId)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        QueensConservatory* conservatory = GetConservatoryFor(handler, target);
        if (!conservatory)
            return false;

        // Resolve the loot table before harvesting so the confirmation can name the table the catalysts picked.
        uint32 lootId = 0;
        if (ConservatoryPlot const* plot = conservatory->GetPlot(plotId))
            conservatory->ResolveHarvestLootId(*plot, lootId);

        ConservatoryError result = conservatory->HarvestWildseed(plotId);
        if (result != CONSERVATORY_OK)
        {
            handler->PSendSysMessage("Could not harvest: {}.", ConservatoryErrorText(result));
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("Harvested pod {} for {} (rolled gameobject_loot_template {}, chosen by the "
            "pod's catalyst set).", uint32(plotId), target->GetName(), lootId);
        return true;
    }

    // Abomination Factory (Necrolord unique sanctum feature, GarrTalentTree 321). Like the Conservatory, the
    // 12.0.7 client has no opcode of its own for it - the whole thing rides on the generic garrison talent wire
    // plus SkillLine 2787 "Abominable Stitching" and its 66 SkillLineAbility recipes. These commands exist so the
    // engine can be driven and inspected before the Stitchyard NPCs (Rathan 167150, the 15 construct
    // questgivers) are spawned.
    static AbominationFactory* GetAbominationFactoryFor(ChatHandler* handler, Player* target)
    {
        Garrison* garrison = target->GetGarrison(GARRISON_TYPE_COVENANT);
        if (!garrison)
        {
            handler->PSendSysMessage("{} has no covenant sanctum (GarrType 111).", target->GetName());
            handler->SetSentErrorMessage(true);
            return nullptr;
        }

        return &garrison->GetAbominationFactory();
    }

    static char const* AbominationErrorText(AbominationFactoryError error)
    {
        switch (error)
        {
            case ABOMINATION_FACTORY_OK:                        return "ok";
            case ABOMINATION_FACTORY_ERROR_NOT_NECROLORD:       return "the character is not pledged to the Necrolords";
            case ABOMINATION_FACTORY_ERROR_NOT_UNLOCKED:        return "GarrTalentTree 321 tier 1 (talent 1096 'Build a Buddy') is not researched";
            case ABOMINATION_FACTORY_ERROR_UNKNOWN_RECIPE:      return "that spell is not a SkillLineAbility of SkillLine 2787 (Abominable Stitching)";
            case ABOMINATION_FACTORY_ERROR_NOT_A_CONSTRUCT:     return "that recipe is not a construct body (its spell has no SPELL_EFFECT_KILL_CREDIT)";
            case ABOMINATION_FACTORY_ERROR_NO_RECIPE_DATA:      return "world table `garrison_abomination_recipe` has no row for that recipe - no 12.0.7 client data says which Abominable Stitching rank unlocks it, so the mapping must be authored";
            case ABOMINATION_FACTORY_ERROR_RANK_TOO_LOW:        return "that recipe needs a higher Abominable Stitching rank than the character has";
            case ABOMINATION_FACTORY_ERROR_ALREADY_BUILT:       return "that construct is already in the stable";
            default:                                            return "unknown error";
        }
    }

    // .garrison abomination status
    static bool HandleAbominationStatusCommand(ChatHandler* handler)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        AbominationFactory* factory = GetAbominationFactoryFor(handler, target);
        if (!factory)
            return false;

        handler->PSendSysMessage("Abomination Factory for {}: accessible {}, Abominable Stitching rank {} (skill {} value {}).",
            target->GetName(), factory->IsAccessible() ? "yes" : "no", factory->GetRank(),
            uint32(SKILL_ABOMINABLE_STITCHING), target->GetPureSkillValue(SKILL_ABOMINABLE_STITCHING));

        std::vector<AbominationConstruct const*> constructs = factory->GetConstructs();
        handler->PSendSysMessage("  stable: {} construct(s).", uint32(constructs.size()));
        for (AbominationConstruct const* construct : constructs)
            handler->PSendSysMessage("    recipe {} built at {}.", construct->RecipeSpellId, int64(construct->BuiltTime));

        if (sGarrisonMgr.GetAbominationRecipes().empty())
            handler->SendSysMessage("  world table `garrison_abomination_recipe` is empty - no recipe is taught "
                "at any rank until the rank->recipe mapping is authored.");
        else
            handler->PSendSysMessage("  {} authored recipe unlock(s) loaded.", uint32(sGarrisonMgr.GetAbominationRecipes().size()));

        return true;
    }

    // .garrison abomination sync   - re-run the rank -> skill line -> recipe pass without waiting for the tick.
    static bool HandleAbominationSyncCommand(ChatHandler* handler)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        AbominationFactory* factory = GetAbominationFactoryFor(handler, target);
        if (!factory)
            return false;

        factory->RefreshSkillAndRecipes();
        handler->PSendSysMessage("Re-synced {}'s Abominable Stitching: rank {}, skill value {}.",
            target->GetName(), factory->GetRank(), target->GetPureSkillValue(SKILL_ABOMINABLE_STITCHING));
        return true;
    }

    // .garrison abomination build <recipeSpellId>   - records a construct without charging its reagents.
    static bool HandleAbominationBuildCommand(ChatHandler* handler, uint32 recipeSpellId)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        AbominationFactory* factory = GetAbominationFactoryFor(handler, target);
        if (!factory)
            return false;

        AbominationFactoryError result = factory->BuildConstruct(recipeSpellId);
        if (result != ABOMINATION_FACTORY_OK)
        {
            handler->PSendSysMessage("Could not build: {}.", AbominationErrorText(result));
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("Added construct {} to {}'s stable.", recipeSpellId, target->GetName());
        return true;
    }

    // Path of Ascension (Kyrian unique sanctum feature, GarrTalentTree 320). Like the Conservatory and the
    // Abomination Factory, the 12.0.7 client has no opcode of its own for it - the research half rides the
    // generic garrison-talent wire, and the activity is a SOLO scenario (1803) on map 2375 "Ascension
    // Coliseum" run at one of the four "Path of Ascension: ..." difficulties (168 Courage / 169 Loyalty /
    // 170 Wisdom / 171 Humility). These commands exist so the engine can be driven and inspected before the
    // Coliseum is spawned and the memory roster authored.
    static PathOfAscension* GetPathOfAscensionFor(ChatHandler* handler, Player* target)
    {
        Garrison* garrison = target->GetGarrison(GARRISON_TYPE_COVENANT);
        if (!garrison)
        {
            handler->PSendSysMessage("{} has no covenant sanctum (GarrType 111).", target->GetName());
            handler->SetSentErrorMessage(true);
            return nullptr;
        }

        return &garrison->GetPathOfAscension();
    }

    static char const* AscensionErrorText(PathOfAscensionError error)
    {
        switch (error)
        {
            case ASCENSION_OK:                          return "ok";
            case ASCENSION_ERROR_NOT_KYRIAN:            return "the character is not pledged to the Kyrian";
            case ASCENSION_ERROR_NOT_UNLOCKED:          return "GarrTalentTree 320 tier 1 (talent 1091 'First Steps') is not researched";
            case ASCENSION_ERROR_UNKNOWN_MEMORY:        return "world table `garrison_ascension_memory` has no row with that memoryId";
            case ASCENSION_ERROR_NO_MEMORY_DATA:        return "world table `garrison_ascension_memory` is empty - no 12.0.7 client row says which memories are the six the first tier captures and the four the second adds, so the roster must be authored";
            case ASCENSION_ERROR_MEMORY_TIER_TOO_LOW:   return "that memory needs more researched tiers of GarrTalentTree 320 than the character has";
            case ASCENSION_ERROR_MEMORY_CAPACITY:       return "the sanctum already holds as many memories as the researched tiers allow (six at one tier, ten from two)";
            case ASCENSION_ERROR_ALREADY_CAPTURED:      return "that memory is already captured";
            case ASCENSION_ERROR_NOT_CAPTURED:          return "that memory has not been captured";
            case ASCENSION_ERROR_INVALID_TRIAL:         return "trial must be 1 Courage, 2 Loyalty, 3 Wisdom or 4 Humility (Difficulty 168-171)";
            case ASCENSION_ERROR_TRIAL_LOCKED:          return "that trial is not open for that memory at the current research";
            case ASCENSION_ERROR_NO_ARENA_CONTENT:      return "the Ascension Coliseum (scenario 1803 on map 2375) is not authored in this world DB - no `scenarios` row and/or no spawns, so a trial cannot be entered and is refused rather than faked";
            default:                                    return "unknown error";
        }
    }

    // .garrison ascension status
    static bool HandleAscensionStatusCommand(ChatHandler* handler)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        PathOfAscension* ascension = GetPathOfAscensionFor(handler, target);
        if (!ascension)
            return false;

        handler->PSendSysMessage("Path of Ascension for {}: accessible {}, researched tiers {}/{}.",
            target->GetName(), ascension->IsAccessible() ? "yes" : "no",
            ascension->GetResearchedTiers(), uint32(ASCENSION_MAX_TIERS));
        handler->PSendSysMessage("  memories {}/{} held, highest trial offered {}, weekly quest slots {}, braziers lit {}/2.",
            uint32(ascension->GetMemories().size()), ascension->GetMemoryCapacity(),
            PathOfAscension::GetTrialName(ascension->GetMaxTrial()),
            ascension->GetWeeklyQuestSlots(), ascension->GetActiveBraziers());
        handler->PSendSysMessage("  total trial wins {} (what the 'Defeat N bosses in the Path of Ascension' achievements count).",
            ascension->GetTotalTrialWins());

        for (AscensionMemory const* memory : ascension->GetMemories())
        {
            AscensionMemoryTemplate const* memoryTemplate = sGarrisonMgr.GetAscensionMemory(memory->MemoryId);
            handler->PSendSysMessage("    memory {} (creature {}) captured at {}, highest trial won: {}.",
                memory->MemoryId, memoryTemplate ? memoryTemplate->CreatureId : 0, int64(memory->CapturedTime),
                PathOfAscension::GetTrialName(AscensionTrial(memory->HighestTrialWon)));
        }

        if (sGarrisonMgr.GetAscensionMemories().empty())
            handler->SendSysMessage("  world table `garrison_ascension_memory` is empty - no memory can be captured "
                "until the roster is authored.");
        else
            handler->PSendSysMessage("  {} authored memory row(s) loaded.", uint32(sGarrisonMgr.GetAscensionMemories().size()));

        handler->PSendSysMessage("  Ascension Coliseum (scenario {} on map {}): {}.",
            uint32(ASCENSION_SCENARIO_ID), uint32(ASCENSION_MAP_ID),
            sGarrisonMgr.IsAscensionArenaAuthored() ? "authored" : "NOT authored - trials are refused");
        return true;
    }

    // .garrison ascension capture <memoryId>
    static bool HandleAscensionCaptureCommand(ChatHandler* handler, uint32 memoryId)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        PathOfAscension* ascension = GetPathOfAscensionFor(handler, target);
        if (!ascension)
            return false;

        PathOfAscensionError result = ascension->CaptureMemory(memoryId);
        if (result != ASCENSION_OK)
        {
            handler->PSendSysMessage("Could not capture: {}.", AscensionErrorText(result));
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("Captured memory {} for {} ({} of {} held).", memoryId, target->GetName(),
            uint32(ascension->GetMemories().size()), ascension->GetMemoryCapacity());
        return true;
    }

    // .garrison ascension start <memoryId> <trial>   - runs every entry gate without changing any state.
    static bool HandleAscensionStartCommand(ChatHandler* handler, uint32 memoryId, uint8 trial)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        PathOfAscension* ascension = GetPathOfAscensionFor(handler, target);
        if (!ascension)
            return false;

        PathOfAscensionError result = ascension->StartTrial(memoryId, AscensionTrial(trial));
        if (result != ASCENSION_OK)
        {
            handler->PSendSysMessage("Cannot enter: {}.", AscensionErrorText(result));
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("{} may enter the {} against memory {} (scenario {} on map {}, Difficulty {}).",
            target->GetName(), PathOfAscension::GetTrialName(AscensionTrial(trial)), memoryId,
            uint32(ASCENSION_SCENARIO_ID), uint32(ASCENSION_MAP_ID),
            PathOfAscension::GetTrialDifficultyId(AscensionTrial(trial)));
        return true;
    }

    // .garrison ascension complete <memoryId> <trial>   - records a won trial without fighting it.
    static bool HandleAscensionCompleteCommand(ChatHandler* handler, uint32 memoryId, uint8 trial)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        PathOfAscension* ascension = GetPathOfAscensionFor(handler, target);
        if (!ascension)
            return false;

        PathOfAscensionError result = ascension->CompleteTrial(memoryId, AscensionTrial(trial));
        if (result != ASCENSION_OK)
        {
            handler->PSendSysMessage("Could not record the win: {}.", AscensionErrorText(result));
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("Recorded {} against memory {} for {} (kill credit awarded, total wins {}).",
            PathOfAscension::GetTrialName(AscensionTrial(trial)), memoryId, target->GetName(),
            ascension->GetTotalTrialWins());
        return true;
    }

    // The Ember Court (Venthyr unique sanctum feature, GarrTalentTree 324). As with the other three unique
    // features the 12.0.7 client has no opcode of its own for it: the upgrade tree rides the generic
    // garrison-talent wire, and the party is scenario 1791 in AreaTable 13329 "The Ember Court" (map 2222)
    // driven by UIWidget sets 459/461. These commands exist so the engine can be driven and inspected before
    // the venue is spawned and the per-guest preferences authored.
    static EmberCourt* GetEmberCourtFor(ChatHandler* handler, Player* target)
    {
        Garrison* garrison = target->GetGarrison(GARRISON_TYPE_COVENANT);
        if (!garrison)
        {
            handler->PSendSysMessage("{} has no covenant sanctum (GarrType 111).", target->GetName());
            handler->SetSentErrorMessage(true);
            return nullptr;
        }

        return &garrison->GetEmberCourt();
    }

    static char const* EmberCourtErrorText(EmberCourtError error)
    {
        switch (error)
        {
            case EMBER_COURT_OK:                            return "ok";
            case EMBER_COURT_ERROR_NOT_VENTHYR:             return "the character is not pledged to the Venthyr";
            case EMBER_COURT_ERROR_NOT_UNLOCKED:            return "GarrTalentTree 324 tier 0 (talent 1111 'A New Court') is not researched";
            case EMBER_COURT_ERROR_UNKNOWN_GUEST:           return "guest index must be 0-15 (CriteriaTree 87983 'Be Our Guest' has exactly sixteen children)";
            case EMBER_COURT_ERROR_GUEST_NOT_UNLOCKED:      return "that guest's 'RSVP: <Guest>' quest has not been completed";
            case EMBER_COURT_ERROR_GUEST_ALREADY_INVITED:   return "that guest is already on the guest list";
            case EMBER_COURT_ERROR_GUEST_NOT_INVITED:       return "that guest is not on the guest list";
            case EMBER_COURT_ERROR_GUEST_SLOTS_FULL:        return "the guest list is already as long as the researched talents allow (2 base, 3 with 'Court Influencer', 4 with 'Discerning Taste')";
            case EMBER_COURT_ERROR_NO_GUESTS_INVITED:       return "nobody is on the guest list";
            case EMBER_COURT_ERROR_INVALID_MOOD:            return "mood must be 0-5 (1 Miserable, 2 Uncomfortable, 3 Happy, 4 Very Happy, 5 Elated)";
            case EMBER_COURT_ERROR_NO_VENUE_CONTENT:        return "the Ember Court venue (scenario 1791 in area 13329 on map 2222) is not authored in this world DB - no `scenarios` row and/or no spawns, so a court cannot be held and is refused rather than faked";
            default:                                        return "unknown error";
        }
    }

    // .garrison embercourt status
    static bool HandleEmberCourtStatusCommand(ChatHandler* handler)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        EmberCourt* court = GetEmberCourtFor(handler, target);
        if (!court)
            return false;

        handler->PSendSysMessage("The Ember Court for {}: accessible {}, researched talents {}/{}.",
            target->GetName(), court->IsAccessible() ? "yes" : "no",
            court->GetResearchedTiers(), uint32(EMBER_COURT_MAX_TIERS));
        handler->PSendSysMessage("  guest slots {}, dredger butler {}, specialist staff {}/{}.",
            court->GetGuestSlots(), court->HasDredgerButler() ? "yes" : "no",
            court->GetStaffSlots(), uint32(EMBER_COURT_STAFF_SLOTS));
        handler->PSendSysMessage("  courts held {}, last court {}.",
            court->GetCourtsHeld(), int64(court->GetLastCourtTime()));

        std::vector<uint8> const invited = court->GetInvitedGuests();
        if (invited.empty())
            handler->SendSysMessage("  guest list: empty.");
        else
        {
            for (uint8 guestIndex : invited)
                if (EmberCourtGuest const* guest = EmberCourt::GetGuestInfo(guestIndex))
                    handler->PSendSysMessage("  guest list: [{}] {} (creature {}).", guestIndex, guest->Name, guest->CreatureId);
        }

        if (sGarrisonMgr.GetEmberCourtGuests().empty())
            handler->SendSysMessage("  world table `garrison_ember_court_guest` is empty - expected: the 12.0.7 "
                "build publishes no guest dislikes. Every guest's LIKES are client data and are loaded.");
        else
            handler->PSendSysMessage("  {} authored guest dislike row(s) loaded.", uint32(sGarrisonMgr.GetEmberCourtGuests().size()));

        handler->PSendSysMessage("  venue (scenario {} in area {} on map {}): {}.",
            uint32(EMBER_COURT_SCENARIO_ID), uint32(EMBER_COURT_AREA_ID), uint32(EMBER_COURT_MAP_ID),
            sGarrisonMgr.IsEmberCourtVenueAuthored() ? "authored" : "NOT authored - courts are refused");
        return true;
    }

    // .garrison embercourt guests   - the sixteen-guest roster with this character's standing.
    static bool HandleEmberCourtGuestsCommand(ChatHandler* handler)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        EmberCourt* court = GetEmberCourtFor(handler, target);
        if (!court)
            return false;

        for (EmberCourtGuest const& guest : EmberCourt::GetGuestRoster())
        {
            EmberCourtGuestState const* state = court->GetGuestState(guest.Index);

            // The guest's client-published "Likes:" list (ItemSparse item guest.MoodItemId).
            std::ostringstream likes;
            for (uint8 attribute = EMBER_COURT_ATTRIBUTE_CLEANLINESS; attribute <= EMBER_COURT_ATTRIBUTE_MAX; ++attribute)
            {
                EmberCourtAttributePole const pole = EmberCourtAttributePole(guest.LikedPoles[attribute]);
                if (pole == EMBER_COURT_POLE_NONE)
                    continue;

                if (likes.tellp() > 0)
                    likes << ", ";
                likes << EmberCourt::GetAttributePoleName(EmberCourtAttribute(attribute), pole);
            }

            handler->PSendSysMessage("  [{}] {} - RSVP quest {} {}, hosted {}x, best mood {}{}.",
                guest.Index, guest.Name, guest.RsvpQuestId,
                court->IsGuestUnlocked(guest.Index) ? "COMPLETED" : "not completed",
                state ? state->TimesHosted : 0,
                EmberCourt::GetMoodName(state ? EmberCourtMood(state->HighestMood) : EMBER_COURT_MOOD_NONE),
                court->IsGuestInvited(guest.Index) ? ", INVITED" : "");
            handler->PSendSysMessage("        likes: {} (item {}).", likes.str(), guest.MoodItemId);
        }
        return true;
    }

    // .garrison embercourt invite <guestIndex>
    static bool HandleEmberCourtInviteCommand(ChatHandler* handler, uint8 guestIndex)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        EmberCourt* court = GetEmberCourtFor(handler, target);
        if (!court)
            return false;

        EmberCourtError result = court->InviteGuest(guestIndex);
        if (result != EMBER_COURT_OK)
        {
            handler->PSendSysMessage("Could not invite: {}.", EmberCourtErrorText(result));
            handler->SetSentErrorMessage(true);
            return false;
        }

        EmberCourtGuest const* guest = EmberCourt::GetGuestInfo(guestIndex);
        handler->PSendSysMessage("Invited {} to {}'s Ember Court ({}/{} slots used).",
            guest ? guest->Name : "?", target->GetName(),
            uint32(court->GetInvitedGuests().size()), court->GetGuestSlots());
        return true;
    }

    // .garrison embercourt uninvite <guestIndex>
    static bool HandleEmberCourtUninviteCommand(ChatHandler* handler, uint8 guestIndex)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        EmberCourt* court = GetEmberCourtFor(handler, target);
        if (!court)
            return false;

        EmberCourtError result = court->UninviteGuest(guestIndex);
        if (result != EMBER_COURT_OK)
        {
            handler->PSendSysMessage("Could not uninvite: {}.", EmberCourtErrorText(result));
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("Removed guest {} from {}'s guest list.", guestIndex, target->GetName());
        return true;
    }

    // .garrison embercourt start   - checks every gate for actually holding the court. Changes no state.
    static bool HandleEmberCourtStartCommand(ChatHandler* handler)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        EmberCourt* court = GetEmberCourtFor(handler, target);
        if (!court)
            return false;

        EmberCourtError result = court->StartCourt();
        if (result != EMBER_COURT_OK)
        {
            handler->PSendSysMessage("Cannot hold the court: {}.", EmberCourtErrorText(result));
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("{} may hold the Ember Court (scenario {} in area {} on map {}) with {} guest(s).",
            target->GetName(), uint32(EMBER_COURT_SCENARIO_ID), uint32(EMBER_COURT_AREA_ID),
            uint32(EMBER_COURT_MAP_ID), uint32(court->GetInvitedGuests().size()));
        return true;
    }

    // .garrison embercourt complete [mood]   - records a court that happened, every invited guest at `mood`
    // on the (unpublished) mood scale. A GM tool, not a fallback: nothing else ever completes a court.
    static bool HandleEmberCourtCompleteCommand(ChatHandler* handler, Optional<uint8> mood)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        EmberCourt* court = GetEmberCourtFor(handler, target);
        if (!court)
            return false;

        std::unordered_map<uint8, uint8> moods;
        for (uint8 guestIndex : court->GetInvitedGuests())
            moods[guestIndex] = mood.value_or(0);

        EmberCourtError result = court->CompleteCourt(moods);
        if (result != EMBER_COURT_OK)
        {
            handler->PSendSysMessage("Could not record the court: {}.", EmberCourtErrorText(result));
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("Recorded an Ember Court for {} ({} held in total).",
            target->GetName(), court->GetCourtsHeld());
        return true;
    }
};

void AddSC_garrison_commandscript()
{
    new garrison_commandscript();
}
