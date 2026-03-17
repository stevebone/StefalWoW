/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * MULTI-STATION TRAVEL PLANNING SYSTEM - Implementation
 */

#include "TravelRouteManager.h"
#include "PortalDatabase.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "MotionMaster.h"
#include "Log.h"
#include "GameTime.h"
#include "SpellHistory.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "ObjectMgr.h"
#include "Transport.h"
#include "TransportMgr.h"
#include "Map.h"
#include "WorldSession.h"
#include "GameObject.h"
#include "GameObjectData.h"
#include "MovementPackets.h"
#include "../Interaction/FlightMasterManager.h"
#include "../Core/PlayerBotHelpers.h"
#include "../AI/BotAI.h"
#include <queue>
#include <algorithm>
#include <cmath>

namespace Playerbot
{

// Static member initialization
std::vector<TransportConnection> TravelRouteManager::s_transportConnections;
std::unordered_map<uint32, std::vector<size_t>> TravelRouteManager::s_connectionsByDepartureMap;
std::unordered_map<uint32, std::vector<size_t>> TravelRouteManager::s_connectionsByArrivalMap;
std::unordered_map<uint32, std::vector<std::pair<uint32, TransportConnection const*>>> TravelRouteManager::s_mapConnectivityGraph;
bool TravelRouteManager::s_connectionsInitialized = false;

// ============================================================================
// TRANSPORT CONNECTIONS DATABASE
// ============================================================================
// World of Warcraft 11.x transport connections (ships, zeppelins, portals)

namespace TransportDB
{
    // Map IDs - Classic
    constexpr uint32 MAP_EASTERN_KINGDOMS = 0;
    constexpr uint32 MAP_KALIMDOR = 1;

    // Map IDs - Expansions
    constexpr uint32 MAP_OUTLAND = 530;
    constexpr uint32 MAP_NORTHREND = 571;
    constexpr uint32 MAP_DEEPHOLM = 646;           // Cataclysm
    constexpr uint32 MAP_PANDARIA = 870;
    constexpr uint32 MAP_DRAENOR = 1116;
    constexpr uint32 MAP_BROKEN_ISLES = 1220;
    constexpr uint32 MAP_ARGUS = 1669;             // Legion 7.3
    constexpr uint32 MAP_ZANDALAR = 1642;
    constexpr uint32 MAP_KUL_TIRAS = 1643;
    constexpr uint32 MAP_DRAGON_ISLES = 2222;

    // Map IDs - Shadowlands
    constexpr uint32 MAP_SHADOWLANDS = 2222;       // Note: Shares ID with Dragon Isles in some contexts
    constexpr uint32 MAP_ORIBOS = 2364;            // Shadowlands hub city
    constexpr uint32 MAP_MALDRAXXUS = 2286;        // Shadowlands zone
    constexpr uint32 MAP_BASTION = 2287;           // Shadowlands zone
    constexpr uint32 MAP_ARDENWEALD = 2288;        // Shadowlands zone
    constexpr uint32 MAP_REVENDRETH = 2289;        // Shadowlands zone
    constexpr uint32 MAP_THE_MAW = 2290;           // Shadowlands endgame zone
    constexpr uint32 MAP_ZERETH_MORTIS = 2291;     // Shadowlands 9.2 zone

    // Map IDs - The War Within (TWW 11.x)
    constexpr uint32 MAP_KHAZ_ALGAR = 2552;        // Main TWW continent ID
    constexpr uint32 MAP_ISLE_OF_DORN = 2444;      // TWW surface zone (Isle of Dorn)
    constexpr uint32 MAP_RINGING_DEEPS = 2214;     // TWW underground zone 1
    constexpr uint32 MAP_HALLOWFALL = 2215;        // TWW underground zone 2
    constexpr uint32 MAP_AZJ_KAHET = 2255;         // TWW underground zone 3
    constexpr uint32 MAP_CITY_OF_THREADS = 2213;   // TWW Nerubian city
    constexpr uint32 MAP_DORNOGAL = 2339;          // TWW capital city instance
    constexpr uint32 MAP_SCENARIO_TWW = 2601;      // TWW scenario/instance map

    // Map IDs - Special locations
    constexpr uint32 MAP_CAVERNS_OF_TIME = 1;      // Tanaris (same as Kalimdor, different area)
    constexpr uint32 MAP_SILITHUS = 1;             // Same as Kalimdor
    constexpr uint32 MAP_EMERALD_DREAM = 2200;     // Dragonflight 10.2 zone

    // Transport wait times (in seconds)
    constexpr uint32 SHIP_WAIT_TIME = 180;      // 3 minutes average wait
    constexpr uint32 ZEPPELIN_WAIT_TIME = 180;  // 3 minutes average wait
    constexpr uint32 PORTAL_WAIT_TIME = 0;      // Instant

    // Transport travel times (in seconds)
    constexpr uint32 SHIP_TRAVEL_TIME = 60;     // 1 minute travel
    constexpr uint32 ZEPPELIN_TRAVEL_TIME = 60; // 1 minute travel
    constexpr uint32 PORTAL_TRAVEL_TIME = 5;    // Near instant

    /**
     * @brief Initialize all transport connections
     * This is the master database of transport routes in WoW
     */
    void InitializeConnections(std::vector<TransportConnection>& connections)
    {
        connections.clear();
        connections.reserve(64);
        uint32 nextId = 1;

        // ====================================================================
        // EASTERN KINGDOMS <-> KALIMDOR CONNECTIONS
        // ====================================================================

        // Booty Bay <-> Ratchet (Neutral ship)
        {
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::SHIP;
            conn.name = "Booty Bay to Ratchet Ship";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(-14281.0f, 556.0f, 8.9f);  // Booty Bay dock
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(-994.0f, -3827.0f, 5.5f);    // Ratchet dock
            conn.transportEntry = 20808;  // The Maiden's Fancy
            conn.waitTimeSeconds = SHIP_WAIT_TIME;
            conn.travelTimeSeconds = SHIP_TRAVEL_TIME;
            conn.allianceOnly = false;
            conn.hordeOnly = false;
            connections.push_back(conn);

            // Reverse connection
            conn.connectionId = nextId++;
            conn.name = "Ratchet to Booty Bay Ship";
            std::swap(conn.departureMapId, conn.arrivalMapId);
            std::swap(conn.departurePosition, conn.arrivalPosition);
            connections.push_back(conn);
        }

        // Menethil Harbor <-> Theramore (Alliance ship)
        {
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::SHIP;
            conn.name = "Menethil Harbor to Theramore Ship";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(-3670.0f, -609.0f, 5.4f);  // Menethil dock
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(-3838.0f, -4527.0f, 8.7f);   // Theramore dock
            conn.transportEntry = 176231; // The Lady Mehley
            conn.waitTimeSeconds = SHIP_WAIT_TIME;
            conn.travelTimeSeconds = SHIP_TRAVEL_TIME;
            conn.allianceOnly = true;
            conn.hordeOnly = false;
            connections.push_back(conn);

            conn.connectionId = nextId++;
            conn.name = "Theramore to Menethil Harbor Ship";
            std::swap(conn.departureMapId, conn.arrivalMapId);
            std::swap(conn.departurePosition, conn.arrivalPosition);
            connections.push_back(conn);
        }

        // Stormwind Harbor <-> Auberdine/Rut'theran (Alliance ship)
        {
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::SHIP;
            conn.name = "Stormwind to Rut'theran Ship";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(-8650.0f, 1345.0f, 5.2f);  // Stormwind Harbor
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(8181.0f, 1005.0f, 0.2f);     // Rut'theran Village
            conn.transportEntry = 181646; // Alliance ship
            conn.waitTimeSeconds = SHIP_WAIT_TIME;
            conn.travelTimeSeconds = SHIP_TRAVEL_TIME;
            conn.allianceOnly = true;
            conn.hordeOnly = false;
            connections.push_back(conn);

            conn.connectionId = nextId++;
            conn.name = "Rut'theran to Stormwind Ship";
            std::swap(conn.departureMapId, conn.arrivalMapId);
            std::swap(conn.departurePosition, conn.arrivalPosition);
            connections.push_back(conn);
        }

        // Undercity <-> Orgrimmar Zeppelin (Horde)
        {
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::ZEPPELIN;
            conn.name = "Undercity to Orgrimmar Zeppelin";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(2066.0f, 285.0f, 97.0f);   // Undercity zeppelin tower
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(1177.0f, -4291.0f, 21.3f);   // Orgrimmar zeppelin tower
            conn.transportEntry = 186238; // Horde zeppelin
            conn.waitTimeSeconds = ZEPPELIN_WAIT_TIME;
            conn.travelTimeSeconds = ZEPPELIN_TRAVEL_TIME;
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);

            conn.connectionId = nextId++;
            conn.name = "Orgrimmar to Undercity Zeppelin";
            std::swap(conn.departureMapId, conn.arrivalMapId);
            std::swap(conn.departurePosition, conn.arrivalPosition);
            connections.push_back(conn);
        }

        // Stranglethorn <-> Orgrimmar Zeppelin (Horde)
        {
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::ZEPPELIN;
            conn.name = "Grom'gol to Orgrimmar Zeppelin";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(-12415.0f, 208.0f, 31.5f); // Grom'gol zeppelin
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(1177.0f, -4291.0f, 21.3f);   // Orgrimmar
            conn.transportEntry = 186238;
            conn.waitTimeSeconds = ZEPPELIN_WAIT_TIME;
            conn.travelTimeSeconds = ZEPPELIN_TRAVEL_TIME;
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);

            conn.connectionId = nextId++;
            conn.name = "Orgrimmar to Grom'gol Zeppelin";
            std::swap(conn.departureMapId, conn.arrivalMapId);
            std::swap(conn.departurePosition, conn.arrivalPosition);
            connections.push_back(conn);
        }

        // Grom'gol <-> Undercity Zeppelin (Horde)
        {
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::ZEPPELIN;
            conn.name = "Grom'gol to Undercity Zeppelin";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(-12415.0f, 208.0f, 31.5f); // Grom'gol
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;  // Same map!
            conn.arrivalPosition.Relocate(2066.0f, 285.0f, 97.0f);     // Undercity
            conn.transportEntry = 186238;
            conn.waitTimeSeconds = ZEPPELIN_WAIT_TIME;
            conn.travelTimeSeconds = ZEPPELIN_TRAVEL_TIME;
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);

            conn.connectionId = nextId++;
            conn.name = "Undercity to Grom'gol Zeppelin";
            std::swap(conn.departurePosition, conn.arrivalPosition);
            connections.push_back(conn);
        }

        // ====================================================================
        // OUTLAND CONNECTIONS
        // ====================================================================

        // Dark Portal (Eastern Kingdoms <-> Outland)
        {
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Dark Portal (Azeroth to Outland)";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(-11903.0f, -3206.0f, -14.9f); // Blasted Lands
            conn.arrivalMapId = MAP_OUTLAND;
            conn.arrivalPosition.Relocate(-248.0f, 934.0f, 84.4f);        // Hellfire Peninsula
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.requiresLevel = true;
            conn.minLevel = 58;
            connections.push_back(conn);

            conn.connectionId = nextId++;
            conn.name = "Dark Portal (Outland to Azeroth)";
            std::swap(conn.departureMapId, conn.arrivalMapId);
            std::swap(conn.departurePosition, conn.arrivalPosition);
            connections.push_back(conn);
        }

        // Shattrath Portals to capital cities
        {
            // Shattrath -> Stormwind
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Shattrath to Stormwind Portal";
            conn.departureMapId = MAP_OUTLAND;
            conn.departurePosition.Relocate(-1889.0f, 5395.0f, -12.4f); // Shattrath
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;
            conn.arrivalPosition.Relocate(-8838.0f, 626.0f, 94.0f);     // Stormwind
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.allianceOnly = true;
            connections.push_back(conn);

            // Shattrath -> Orgrimmar
            conn.connectionId = nextId++;
            conn.name = "Shattrath to Orgrimmar Portal";
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(1676.0f, -4315.0f, 61.2f);    // Orgrimmar
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);
        }

        // ====================================================================
        // NORTHREND CONNECTIONS
        // ====================================================================

        // Stormwind -> Borean Tundra (Alliance ship)
        {
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::SHIP;
            conn.name = "Stormwind to Valiance Keep Ship";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(-8650.0f, 1345.0f, 5.2f);  // Stormwind Harbor
            conn.arrivalMapId = MAP_NORTHREND;
            conn.arrivalPosition.Relocate(2236.0f, 5140.0f, 5.3f);     // Valiance Keep
            conn.waitTimeSeconds = SHIP_WAIT_TIME;
            conn.travelTimeSeconds = SHIP_TRAVEL_TIME * 2; // Longer journey
            conn.allianceOnly = true;
            conn.requiresLevel = true;
            conn.minLevel = 68;
            connections.push_back(conn);

            conn.connectionId = nextId++;
            conn.name = "Valiance Keep to Stormwind Ship";
            std::swap(conn.departureMapId, conn.arrivalMapId);
            std::swap(conn.departurePosition, conn.arrivalPosition);
            connections.push_back(conn);
        }

        // Orgrimmar -> Borean Tundra (Horde zeppelin)
        {
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::ZEPPELIN;
            conn.name = "Orgrimmar to Warsong Hold Zeppelin";
            conn.departureMapId = MAP_KALIMDOR;
            conn.departurePosition.Relocate(1177.0f, -4291.0f, 21.3f); // Orgrimmar
            conn.arrivalMapId = MAP_NORTHREND;
            conn.arrivalPosition.Relocate(2836.0f, 6180.0f, 104.0f);   // Warsong Hold
            conn.waitTimeSeconds = ZEPPELIN_WAIT_TIME;
            conn.travelTimeSeconds = ZEPPELIN_TRAVEL_TIME * 2;
            conn.hordeOnly = true;
            conn.requiresLevel = true;
            conn.minLevel = 68;
            connections.push_back(conn);

            conn.connectionId = nextId++;
            conn.name = "Warsong Hold to Orgrimmar Zeppelin";
            std::swap(conn.departureMapId, conn.arrivalMapId);
            std::swap(conn.departurePosition, conn.arrivalPosition);
            connections.push_back(conn);
        }

        // Menethil -> Howling Fjord (Alliance ship)
        {
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::SHIP;
            conn.name = "Menethil to Valgarde Ship";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(-3670.0f, -609.0f, 5.4f);  // Menethil
            conn.arrivalMapId = MAP_NORTHREND;
            conn.arrivalPosition.Relocate(588.0f, -5095.0f, 1.6f);     // Valgarde
            conn.waitTimeSeconds = SHIP_WAIT_TIME;
            conn.travelTimeSeconds = SHIP_TRAVEL_TIME * 2;
            conn.allianceOnly = true;
            conn.requiresLevel = true;
            conn.minLevel = 68;
            connections.push_back(conn);

            conn.connectionId = nextId++;
            conn.name = "Valgarde to Menethil Ship";
            std::swap(conn.departureMapId, conn.arrivalMapId);
            std::swap(conn.departurePosition, conn.arrivalPosition);
            connections.push_back(conn);
        }

        // Undercity -> Howling Fjord (Horde zeppelin)
        {
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::ZEPPELIN;
            conn.name = "Undercity to Vengeance Landing Zeppelin";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(2066.0f, 285.0f, 97.0f);   // Undercity
            conn.arrivalMapId = MAP_NORTHREND;
            conn.arrivalPosition.Relocate(1974.0f, -6081.0f, 67.0f);   // Vengeance Landing
            conn.waitTimeSeconds = ZEPPELIN_WAIT_TIME;
            conn.travelTimeSeconds = ZEPPELIN_TRAVEL_TIME * 2;
            conn.hordeOnly = true;
            conn.requiresLevel = true;
            conn.minLevel = 68;
            connections.push_back(conn);

            conn.connectionId = nextId++;
            conn.name = "Vengeance Landing to Undercity Zeppelin";
            std::swap(conn.departureMapId, conn.arrivalMapId);
            std::swap(conn.departurePosition, conn.arrivalPosition);
            connections.push_back(conn);
        }

        // Dalaran portals (Northrend)
        {
            // Dalaran -> Stormwind
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Dalaran to Stormwind Portal";
            conn.departureMapId = MAP_NORTHREND;
            conn.departurePosition.Relocate(5719.0f, 719.0f, 641.7f);  // Dalaran
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;
            conn.arrivalPosition.Relocate(-8838.0f, 626.0f, 94.0f);    // Stormwind
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.allianceOnly = true;
            connections.push_back(conn);

            // Dalaran -> Orgrimmar
            conn.connectionId = nextId++;
            conn.name = "Dalaran to Orgrimmar Portal";
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(1676.0f, -4315.0f, 61.2f);   // Orgrimmar
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);
        }

        // ====================================================================
        // MODERN EXPANSION CONNECTIONS (Pandaria, Draenor, etc.)
        // ====================================================================

        // Stormwind/Orgrimmar -> Jade Forest (Pandaria)
        {
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Stormwind to Jade Forest Portal";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(-8838.0f, 626.0f, 94.0f);  // Stormwind
            conn.arrivalMapId = MAP_PANDARIA;
            conn.arrivalPosition.Relocate(942.0f, -569.0f, 184.0f);    // Jade Forest
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.allianceOnly = true;
            conn.requiresLevel = true;
            conn.minLevel = 85;
            connections.push_back(conn);

            conn.connectionId = nextId++;
            conn.name = "Orgrimmar to Jade Forest Portal";
            conn.departureMapId = MAP_KALIMDOR;
            conn.departurePosition.Relocate(1676.0f, -4315.0f, 61.2f); // Orgrimmar
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);
        }

        // Stormwind/Orgrimmar -> Draenor
        {
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Stormwind to Draenor Portal";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(-8838.0f, 626.0f, 94.0f);  // Stormwind
            conn.arrivalMapId = MAP_DRAENOR;
            conn.arrivalPosition.Relocate(2068.0f, 196.0f, 87.0f);     // Shadowmoon Valley
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.allianceOnly = true;
            conn.requiresLevel = true;
            conn.minLevel = 90;
            connections.push_back(conn);

            conn.connectionId = nextId++;
            conn.name = "Orgrimmar to Draenor Portal";
            conn.departureMapId = MAP_KALIMDOR;
            conn.departurePosition.Relocate(1676.0f, -4315.0f, 61.2f); // Orgrimmar
            conn.arrivalPosition.Relocate(5579.0f, 4571.0f, 133.0f);   // Frostfire Ridge
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);
        }

        // Capital city -> Broken Isles (Legion)
        {
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Stormwind to Dalaran (Broken Isles) Portal";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(-8838.0f, 626.0f, 94.0f);
            conn.arrivalMapId = MAP_BROKEN_ISLES;
            conn.arrivalPosition.Relocate(-853.0f, 4491.0f, 729.0f);   // Dalaran
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.requiresLevel = true;
            conn.minLevel = 98;
            connections.push_back(conn);
        }

        // Capital city -> Zandalar/Kul Tiras (BFA)
        {
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Stormwind to Boralus Portal";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(-8838.0f, 626.0f, 94.0f);
            conn.arrivalMapId = MAP_KUL_TIRAS;
            conn.arrivalPosition.Relocate(-1774.0f, -1580.0f, 0.3f);   // Boralus
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.allianceOnly = true;
            conn.requiresLevel = true;
            conn.minLevel = 110;
            connections.push_back(conn);

            conn.connectionId = nextId++;
            conn.name = "Orgrimmar to Dazar'alor Portal";
            conn.departureMapId = MAP_KALIMDOR;
            conn.departurePosition.Relocate(1676.0f, -4315.0f, 61.2f);
            conn.arrivalMapId = MAP_ZANDALAR;
            conn.arrivalPosition.Relocate(-1015.0f, 805.0f, 440.0f);   // Dazar'alor
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);
        }

        // Capital city -> Dragon Isles (Dragonflight)
        {
            // Stormwind -> Valdrakken (Alliance)
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Stormwind to Valdrakken Portal";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(-8838.0f, 626.0f, 94.0f);
            conn.arrivalMapId = MAP_DRAGON_ISLES;
            conn.arrivalPosition.Relocate(-2512.0f, -376.0f, 201.0f);  // Valdrakken
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.allianceOnly = true;
            conn.requiresLevel = true;
            conn.minLevel = 60;
            connections.push_back(conn);

            // Orgrimmar -> Valdrakken (Horde)
            conn.connectionId = nextId++;
            conn.name = "Orgrimmar to Valdrakken Portal";
            conn.departureMapId = MAP_KALIMDOR;
            conn.departurePosition.Relocate(1676.0f, -4315.0f, 61.2f);  // Orgrimmar
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);

            // Valdrakken -> Stormwind (Alliance) - REVERSE
            conn.connectionId = nextId++;
            conn.name = "Valdrakken to Stormwind Portal";
            conn.departureMapId = MAP_DRAGON_ISLES;
            conn.departurePosition.Relocate(-2512.0f, -376.0f, 201.0f);  // Valdrakken portal room
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;
            conn.arrivalPosition.Relocate(-8838.0f, 626.0f, 94.0f);      // Stormwind
            conn.allianceOnly = true;
            conn.hordeOnly = false;
            connections.push_back(conn);

            // Valdrakken -> Orgrimmar (Horde) - REVERSE
            conn.connectionId = nextId++;
            conn.name = "Valdrakken to Orgrimmar Portal";
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(1676.0f, -4315.0f, 61.2f);     // Orgrimmar
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);
        }

        // Capital city <-> Khaz Algar (The War Within)
        {
            // Stormwind -> Dornogal (Alliance)
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Stormwind to Dornogal Portal";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(-8838.0f, 626.0f, 94.0f);
            conn.arrivalMapId = MAP_KHAZ_ALGAR;
            conn.arrivalPosition.Relocate(1287.0f, -2252.0f, 176.0f);  // Dornogal
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.allianceOnly = true;
            conn.requiresLevel = true;
            conn.minLevel = 70;
            connections.push_back(conn);

            // Orgrimmar -> Dornogal (Horde)
            conn.connectionId = nextId++;
            conn.name = "Orgrimmar to Dornogal Portal";
            conn.departureMapId = MAP_KALIMDOR;
            conn.departurePosition.Relocate(1676.0f, -4315.0f, 61.2f);  // Orgrimmar
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);

            // Dornogal -> Stormwind (Alliance) - REVERSE
            conn.connectionId = nextId++;
            conn.name = "Dornogal to Stormwind Portal";
            conn.departureMapId = MAP_KHAZ_ALGAR;
            conn.departurePosition.Relocate(1287.0f, -2252.0f, 176.0f);  // Dornogal portal room
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;
            conn.arrivalPosition.Relocate(-8838.0f, 626.0f, 94.0f);      // Stormwind
            conn.allianceOnly = true;
            conn.hordeOnly = false;
            connections.push_back(conn);

            // Dornogal -> Orgrimmar (Horde) - REVERSE
            conn.connectionId = nextId++;
            conn.name = "Dornogal to Orgrimmar Portal";
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(1676.0f, -4315.0f, 61.2f);     // Orgrimmar
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);
        }

        // ====================================================================
        // REVERSE ROUTES FROM EXPANSION ZONES (Critical for returning home)
        // ====================================================================

        // Broken Isles (Legion Dalaran) -> Capitals
        {
            // Dalaran (Legion) -> Stormwind (Alliance)
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Dalaran (Legion) to Stormwind Portal";
            conn.departureMapId = MAP_BROKEN_ISLES;
            conn.departurePosition.Relocate(-853.0f, 4491.0f, 729.0f);  // Dalaran portal room
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;
            conn.arrivalPosition.Relocate(-8838.0f, 626.0f, 94.0f);     // Stormwind
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.allianceOnly = true;
            connections.push_back(conn);

            // Dalaran (Legion) -> Orgrimmar (Horde)
            conn.connectionId = nextId++;
            conn.name = "Dalaran (Legion) to Orgrimmar Portal";
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(1676.0f, -4315.0f, 61.2f);    // Orgrimmar
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);
        }

        // Kul Tiras / Zandalar (BFA) -> Capitals
        {
            // Boralus -> Stormwind (Alliance)
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Boralus to Stormwind Portal";
            conn.departureMapId = MAP_KUL_TIRAS;
            conn.departurePosition.Relocate(-1774.0f, -1580.0f, 0.3f);  // Boralus portal room
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;
            conn.arrivalPosition.Relocate(-8838.0f, 626.0f, 94.0f);     // Stormwind
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.allianceOnly = true;
            connections.push_back(conn);

            // Dazar'alor -> Orgrimmar (Horde)
            conn.connectionId = nextId++;
            conn.name = "Dazar'alor to Orgrimmar Portal";
            conn.departureMapId = MAP_ZANDALAR;
            conn.departurePosition.Relocate(-1015.0f, 805.0f, 440.0f);  // Dazar'alor portal room
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(1676.0f, -4315.0f, 61.2f);    // Orgrimmar
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);
        }

        // Pandaria (Shrine) -> Capitals
        {
            // Shrine of Seven Stars -> Stormwind (Alliance)
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Shrine of Seven Stars to Stormwind Portal";
            conn.departureMapId = MAP_PANDARIA;
            conn.departurePosition.Relocate(942.0f, 249.0f, 520.0f);    // Shrine portal room
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;
            conn.arrivalPosition.Relocate(-8838.0f, 626.0f, 94.0f);     // Stormwind
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.allianceOnly = true;
            connections.push_back(conn);

            // Shrine of Two Moons -> Orgrimmar (Horde)
            conn.connectionId = nextId++;
            conn.name = "Shrine of Two Moons to Orgrimmar Portal";
            conn.departurePosition.Relocate(1641.0f, 931.0f, 471.0f);   // Shrine portal room
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(1676.0f, -4315.0f, 61.2f);    // Orgrimmar
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);

            // Pandaria -> Jade Forest (reverse of capital->pandaria)
            conn.connectionId = nextId++;
            conn.name = "Jade Forest to Stormwind Portal";
            conn.departurePosition.Relocate(942.0f, -569.0f, 184.0f);   // Jade Forest
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;
            conn.arrivalPosition.Relocate(-8838.0f, 626.0f, 94.0f);
            conn.allianceOnly = true;
            conn.hordeOnly = false;
            connections.push_back(conn);
        }

        // Draenor (Garrison/Ashran) -> Capitals
        {
            // Alliance Garrison -> Stormwind
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Alliance Garrison to Stormwind Portal";
            conn.departureMapId = MAP_DRAENOR;
            conn.departurePosition.Relocate(2068.0f, 196.0f, 87.0f);    // Shadowmoon garrison
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;
            conn.arrivalPosition.Relocate(-8838.0f, 626.0f, 94.0f);     // Stormwind
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.allianceOnly = true;
            connections.push_back(conn);

            // Horde Garrison -> Orgrimmar
            conn.connectionId = nextId++;
            conn.name = "Horde Garrison to Orgrimmar Portal";
            conn.departurePosition.Relocate(5579.0f, 4571.0f, 133.0f);  // Frostfire garrison
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(1676.0f, -4315.0f, 61.2f);    // Orgrimmar
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);

            // Ashran -> Stormwind (Alliance)
            conn.connectionId = nextId++;
            conn.name = "Stormshield to Stormwind Portal";
            conn.departurePosition.Relocate(-4059.0f, -2271.0f, 51.0f); // Stormshield
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;
            conn.arrivalPosition.Relocate(-8838.0f, 626.0f, 94.0f);
            conn.allianceOnly = true;
            conn.hordeOnly = false;
            connections.push_back(conn);

            // Ashran -> Orgrimmar (Horde)
            conn.connectionId = nextId++;
            conn.name = "Warspear to Orgrimmar Portal";
            conn.departurePosition.Relocate(-3998.0f, -2525.0f, 72.0f); // Warspear
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(1676.0f, -4315.0f, 61.2f);
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);
        }

        // ====================================================================
        // THE WAR WITHIN INTERNAL CONNECTIONS
        // ====================================================================
        // TWW has multiple map IDs for different zones that need interconnection

        // Isle of Dorn (2444) <-> Khaz Algar Hub (2552) / Dornogal
        {
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Isle of Dorn to Dornogal Portal";
            conn.departureMapId = MAP_ISLE_OF_DORN;
            conn.departurePosition.Relocate(3675.0f, -1833.0f, 2.8f);   // Isle of Dorn
            conn.arrivalMapId = MAP_KHAZ_ALGAR;
            conn.arrivalPosition.Relocate(1287.0f, -2252.0f, 176.0f);   // Dornogal
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            connections.push_back(conn);

            // Reverse
            conn.connectionId = nextId++;
            conn.name = "Dornogal to Isle of Dorn Portal";
            std::swap(conn.departureMapId, conn.arrivalMapId);
            std::swap(conn.departurePosition, conn.arrivalPosition);
            connections.push_back(conn);
        }

        // Isle of Dorn (2444) -> Capital cities (for bots stuck on this specific map ID)
        {
            // Isle of Dorn -> Stormwind (Alliance)
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Isle of Dorn to Stormwind Portal";
            conn.departureMapId = MAP_ISLE_OF_DORN;
            conn.departurePosition.Relocate(3675.0f, -1833.0f, 2.8f);   // Isle of Dorn (via Dornogal)
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;
            conn.arrivalPosition.Relocate(-8838.0f, 626.0f, 94.0f);     // Stormwind
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.allianceOnly = true;
            connections.push_back(conn);

            // Isle of Dorn -> Orgrimmar (Horde)
            conn.connectionId = nextId++;
            conn.name = "Isle of Dorn to Orgrimmar Portal";
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(1676.0f, -4315.0f, 61.2f);    // Orgrimmar
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);
        }

        // Underground zones <-> Dornogal hub
        // These are elevator/portal connections within TWW
        {
            // Ringing Deeps <-> Dornogal
            // WoWHead Portal to Dornogal object #441968 - Pillarstone Spire elevator
            // Map coordinates: [53.4, 44.6] - requires in-game verification
            // Elevator unlocked via "Rust and Redemption" quest after Ringing Deeps campaign
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Ringing Deeps to Dornogal Elevator";
            conn.departureMapId = MAP_RINGING_DEEPS;
            conn.departurePosition.Relocate(1430.0f, -890.0f, -310.0f); // Pillarstone Spire area
            conn.arrivalMapId = MAP_KHAZ_ALGAR;
            conn.arrivalPosition.Relocate(1287.0f, -2252.0f, 176.0f);   // Dornogal
            conn.waitTimeSeconds = 30;  // Elevator wait time
            conn.travelTimeSeconds = 30;
            connections.push_back(conn);

            conn.connectionId = nextId++;
            conn.name = "Dornogal to Ringing Deeps Elevator";
            std::swap(conn.departureMapId, conn.arrivalMapId);
            std::swap(conn.departurePosition, conn.arrivalPosition);
            connections.push_back(conn);

            // Hallowfall <-> Dornogal
            // WoWHead Portal to Dornogal object #441968
            // Map coordinates: [28.3, 56.3] - requires in-game verification
            // Note: Hallowfall portal unlocks after zone quests
            conn.connectionId = nextId++;
            conn.name = "Hallowfall to Dornogal Portal";
            conn.departureMapId = MAP_HALLOWFALL;
            conn.departurePosition.Relocate(-870.0f, 255.0f, -480.0f);  // Portal location in Hallowfall
            conn.arrivalMapId = MAP_KHAZ_ALGAR;
            conn.arrivalPosition.Relocate(1287.0f, -2252.0f, 176.0f);
            connections.push_back(conn);

            conn.connectionId = nextId++;
            conn.name = "Dornogal to Hallowfall Portal";
            std::swap(conn.departureMapId, conn.arrivalMapId);
            std::swap(conn.departurePosition, conn.arrivalPosition);
            connections.push_back(conn);

            // Azj-Kahet <-> Dornogal
            // WoWHead Portal to Dornogal object #441968
            // Map coordinates: [54.9, 72.6] - Weaver's Lair to Fissure area of Dornogal
            // Unlocks after completing "Plans Within Plans" questline
            conn.connectionId = nextId++;
            conn.name = "Azj-Kahet to Dornogal Portal";
            conn.departureMapId = MAP_AZJ_KAHET;
            conn.departurePosition.Relocate(195.0f, -460.0f, -620.0f);  // Weaver's Lair portal
            conn.arrivalMapId = MAP_KHAZ_ALGAR;
            conn.arrivalPosition.Relocate(1287.0f, -2252.0f, 176.0f);
            connections.push_back(conn);

            conn.connectionId = nextId++;
            conn.name = "Dornogal to Azj-Kahet Portal";
            std::swap(conn.departureMapId, conn.arrivalMapId);
            std::swap(conn.departurePosition, conn.arrivalPosition);
            connections.push_back(conn);
        }

        // Dragon Isles internal connections (Valdrakken is the hub)
        // Emerald Dream (map 2200) was added in 10.2
        {
            // Valdrakken -> Emerald Dream
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Valdrakken to Emerald Dream Portal";
            conn.departureMapId = MAP_DRAGON_ISLES;
            conn.departurePosition.Relocate(-2512.0f, -376.0f, 201.0f); // Valdrakken
            conn.arrivalMapId = 2200;  // Emerald Dream
            conn.arrivalPosition.Relocate(4525.0f, -2265.0f, 34.0f);    // Central Encampment
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.requiresLevel = true;
            conn.minLevel = 70;
            connections.push_back(conn);

            // Emerald Dream -> Valdrakken
            conn.connectionId = nextId++;
            conn.name = "Emerald Dream to Valdrakken Portal";
            std::swap(conn.departureMapId, conn.arrivalMapId);
            std::swap(conn.departurePosition, conn.arrivalPosition);
            connections.push_back(conn);
        }

        // ====================================================================
        // CATACLYSM CONNECTIONS (Deepholm, Twilight Highlands, etc.)
        // ====================================================================
        {
            // Stormwind -> Deepholm (via portal in Earthen Ring enclave)
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Stormwind to Deepholm Portal";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(-8178.0f, 823.0f, 72.0f);   // Stormwind Earthen Ring
            conn.arrivalMapId = MAP_DEEPHOLM;
            conn.arrivalPosition.Relocate(980.0f, 523.0f, -44.0f);      // Temple of Earth
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.requiresLevel = true;
            conn.minLevel = 82;
            connections.push_back(conn);

            // Orgrimmar -> Deepholm
            conn.connectionId = nextId++;
            conn.name = "Orgrimmar to Deepholm Portal";
            conn.departureMapId = MAP_KALIMDOR;
            conn.departurePosition.Relocate(1778.0f, -4341.0f, -7.5f);  // Orgrimmar Earthen Ring
            connections.push_back(conn);

            // Deepholm -> Stormwind
            conn.connectionId = nextId++;
            conn.name = "Deepholm to Stormwind Portal";
            conn.departureMapId = MAP_DEEPHOLM;
            conn.departurePosition.Relocate(980.0f, 523.0f, -44.0f);
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;
            conn.arrivalPosition.Relocate(-8178.0f, 823.0f, 72.0f);
            conn.allianceOnly = true;
            connections.push_back(conn);

            // Deepholm -> Orgrimmar
            conn.connectionId = nextId++;
            conn.name = "Deepholm to Orgrimmar Portal";
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(1778.0f, -4341.0f, -7.5f);
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);
        }

        // ====================================================================
        // ARGUS CONNECTIONS (Legion 7.3)
        // ====================================================================
        {
            // Dalaran (Legion) -> Vindicaar (Argus)
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Dalaran to Argus Portal";
            conn.departureMapId = MAP_BROKEN_ISLES;
            conn.departurePosition.Relocate(-853.0f, 4491.0f, 729.0f);  // Dalaran
            conn.arrivalMapId = MAP_ARGUS;
            conn.arrivalPosition.Relocate(-3033.0f, 9023.0f, -168.0f);  // Vindicaar
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.requiresLevel = true;
            conn.minLevel = 110;
            connections.push_back(conn);

            // Argus -> Dalaran
            conn.connectionId = nextId++;
            conn.name = "Argus to Dalaran Portal";
            std::swap(conn.departureMapId, conn.arrivalMapId);
            std::swap(conn.departurePosition, conn.arrivalPosition);
            connections.push_back(conn);
        }

        // ====================================================================
        // SHADOWLANDS CONNECTIONS (Oribos hub)
        // ====================================================================
        {
            // Stormwind -> Oribos (Alliance)
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Stormwind to Oribos Portal";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(-8838.0f, 626.0f, 94.0f);   // Stormwind portal room
            conn.arrivalMapId = MAP_ORIBOS;
            conn.arrivalPosition.Relocate(-1758.0f, 1257.0f, 5453.0f);  // Oribos Ring of Transference
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.allianceOnly = true;
            conn.requiresLevel = true;
            conn.minLevel = 48;
            connections.push_back(conn);

            // Orgrimmar -> Oribos (Horde)
            conn.connectionId = nextId++;
            conn.name = "Orgrimmar to Oribos Portal";
            conn.departureMapId = MAP_KALIMDOR;
            conn.departurePosition.Relocate(1676.0f, -4315.0f, 61.2f);  // Orgrimmar portal room
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);

            // Oribos -> Stormwind (Alliance)
            conn.connectionId = nextId++;
            conn.name = "Oribos to Stormwind Portal";
            conn.departureMapId = MAP_ORIBOS;
            conn.departurePosition.Relocate(-1758.0f, 1257.0f, 5453.0f);
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;
            conn.arrivalPosition.Relocate(-8838.0f, 626.0f, 94.0f);
            conn.allianceOnly = true;
            conn.hordeOnly = false;
            connections.push_back(conn);

            // Oribos -> Orgrimmar (Horde)
            conn.connectionId = nextId++;
            conn.name = "Oribos to Orgrimmar Portal";
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(1676.0f, -4315.0f, 61.2f);
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);
        }

        // ====================================================================
        // CAVERNS OF TIME CONNECTIONS
        // ====================================================================
        {
            // Stormwind -> Caverns of Time (via portal room)
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Stormwind to Caverns of Time Portal";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(-8838.0f, 626.0f, 94.0f);
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(-8173.0f, -4746.0f, 33.8f);   // Caverns of Time entrance
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.allianceOnly = true;
            connections.push_back(conn);

            // Orgrimmar -> Caverns of Time
            conn.connectionId = nextId++;
            conn.name = "Orgrimmar to Caverns of Time Portal";
            conn.departureMapId = MAP_KALIMDOR;
            conn.departurePosition.Relocate(1676.0f, -4315.0f, 61.2f);
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);

            // Caverns of Time -> Stormwind (Alliance)
            conn.connectionId = nextId++;
            conn.name = "Caverns of Time to Stormwind Portal";
            conn.departurePosition.Relocate(-8173.0f, -4746.0f, 33.8f);
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;
            conn.arrivalPosition.Relocate(-8838.0f, 626.0f, 94.0f);
            conn.allianceOnly = true;
            conn.hordeOnly = false;
            connections.push_back(conn);

            // Caverns of Time -> Orgrimmar (Horde)
            conn.connectionId = nextId++;
            conn.name = "Caverns of Time to Orgrimmar Portal";
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(1676.0f, -4315.0f, 61.2f);
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);
        }

        // ====================================================================
        // VALDRAKKEN HUB CONNECTIONS (Portals within Valdrakken)
        // ====================================================================
        {
            // Valdrakken -> New Dalaran (Broken Isles)
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Valdrakken to New Dalaran Portal";
            conn.departureMapId = MAP_DRAGON_ISLES;
            conn.departurePosition.Relocate(-2512.0f, -376.0f, 201.0f);  // Valdrakken
            conn.arrivalMapId = MAP_BROKEN_ISLES;
            conn.arrivalPosition.Relocate(-853.0f, 4491.0f, 729.0f);     // Dalaran
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            connections.push_back(conn);

            // Valdrakken -> Jade Forest (Pandaria)
            conn.connectionId = nextId++;
            conn.name = "Valdrakken to Jade Forest Portal";
            conn.arrivalMapId = MAP_PANDARIA;
            conn.arrivalPosition.Relocate(942.0f, -569.0f, 184.0f);      // Jade Forest
            connections.push_back(conn);

            // Valdrakken -> Shadowmoon Valley (Draenor) - Alliance only
            conn.connectionId = nextId++;
            conn.name = "Valdrakken to Shadowmoon Valley Portal";
            conn.arrivalMapId = MAP_DRAENOR;
            conn.arrivalPosition.Relocate(2068.0f, 196.0f, 87.0f);       // Shadowmoon
            conn.allianceOnly = true;
            connections.push_back(conn);

            // Valdrakken -> Frostfire Ridge (Draenor) - Horde only
            conn.connectionId = nextId++;
            conn.name = "Valdrakken to Frostfire Ridge Portal";
            conn.arrivalPosition.Relocate(5579.0f, 4571.0f, 133.0f);     // Frostfire
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);
        }

        // ====================================================================
        // ADDITIONAL ALLIANCE CAPITAL CONNECTIONS
        // ====================================================================
        {
            // Stormwind <-> Ironforge (Deeprun Tram - instant)
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;  // Tram acts like portal
            conn.name = "Stormwind to Ironforge Tram";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(-8366.0f, 615.0f, 91.7f);   // Stormwind Tram entrance
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;
            conn.arrivalPosition.Relocate(-4841.0f, -1323.0f, 502.0f);  // Ironforge Tram exit
            conn.waitTimeSeconds = 30;
            conn.travelTimeSeconds = 60;
            conn.allianceOnly = true;
            connections.push_back(conn);

            // Reverse
            conn.connectionId = nextId++;
            conn.name = "Ironforge to Stormwind Tram";
            std::swap(conn.departurePosition, conn.arrivalPosition);
            connections.push_back(conn);

            // Boralus -> Ironforge Portal
            conn.connectionId = nextId++;
            conn.name = "Boralus to Ironforge Portal";
            conn.type = TransportType::PORTAL;
            conn.departureMapId = MAP_KUL_TIRAS;
            conn.departurePosition.Relocate(-1774.0f, -1580.0f, 0.3f);  // Boralus portal room
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;
            conn.arrivalPosition.Relocate(-4841.0f, -1323.0f, 502.0f);  // Ironforge
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.allianceOnly = true;
            connections.push_back(conn);

            // Boralus -> Exodar Portal
            conn.connectionId = nextId++;
            conn.name = "Boralus to Exodar Portal";
            conn.arrivalMapId = 530;  // Outland (Exodar is technically on same map system)
            conn.arrivalPosition.Relocate(-4014.0f, -11897.0f, -1.3f);   // Exodar
            connections.push_back(conn);
        }

        // ====================================================================
        // ADDITIONAL HORDE CAPITAL CONNECTIONS
        // ====================================================================
        {
            // Orgrimmar <-> Undercity Zeppelin is already defined above

            // Orgrimmar <-> Thunder Bluff (not currently defined - long flight)
            // This is typically handled by taxi/flight paths rather than portal

            // Dazar'alor -> Thunder Bluff Portal (BFA addition)
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Dazar'alor to Thunder Bluff Portal";
            conn.departureMapId = MAP_ZANDALAR;
            conn.departurePosition.Relocate(-1015.0f, 805.0f, 440.0f);  // Dazar'alor
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(-1274.0f, 124.0f, 131.3f);    // Thunder Bluff
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.hordeOnly = true;
            connections.push_back(conn);

            // Dazar'alor -> Silvermoon Portal
            conn.connectionId = nextId++;
            conn.name = "Dazar'alor to Silvermoon Portal";
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;
            conn.arrivalPosition.Relocate(9492.0f, -7281.0f, 14.3f);    // Silvermoon (translocation)
            connections.push_back(conn);
        }

        // ====================================================================
        // SILVERMOON <-> UNDERCITY (Orb of Translocation - Horde only)
        // ====================================================================
        {
            // Silvermoon -> Undercity (via Orb of Translocation)
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Silvermoon to Undercity (Translocation Orb)";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(9492.0f, -7281.0f, 14.3f);   // Silvermoon Sunfury Spire
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;
            conn.arrivalPosition.Relocate(1811.0f, 274.0f, 75.0f);       // Undercity (Ruins of Lordaeron)
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.hordeOnly = true;
            connections.push_back(conn);

            // Undercity -> Silvermoon
            conn.connectionId = nextId++;
            conn.name = "Undercity to Silvermoon (Translocation Orb)";
            std::swap(conn.departurePosition, conn.arrivalPosition);
            connections.push_back(conn);
        }

        // ====================================================================
        // EXODAR CONNECTIONS (Draenei starting area)
        // ====================================================================
        {
            // Exodar -> Stormwind (direct portal in Vault of Lights)
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Exodar to Stormwind Portal";
            conn.departureMapId = MAP_OUTLAND;  // Exodar shares Outland map ID
            conn.departurePosition.Relocate(-4014.0f, -11897.0f, -1.3f);  // Exodar
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;
            conn.arrivalPosition.Relocate(-8838.0f, 626.0f, 94.0f);       // Stormwind
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.allianceOnly = true;
            connections.push_back(conn);

            // Exodar -> Darnassus (portal in Vault of Lights)
            conn.connectionId = nextId++;
            conn.name = "Exodar to Darnassus Portal";
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(9949.0f, 2412.0f, 1327.0f);     // Darnassus
            connections.push_back(conn);

            // Exodar -> Hellfire Peninsula (for Outland access)
            conn.connectionId = nextId++;
            conn.name = "Exodar to Hellfire Peninsula Portal";
            conn.arrivalMapId = MAP_OUTLAND;
            conn.arrivalPosition.Relocate(-248.0f, 934.0f, 84.4f);        // Hellfire Peninsula
            conn.requiresLevel = true;
            conn.minLevel = 58;
            connections.push_back(conn);

            // Darnassus -> Exodar (portal in Temple of the Moon)
            conn.connectionId = nextId++;
            conn.name = "Darnassus to Exodar Portal";
            conn.departureMapId = MAP_KALIMDOR;
            conn.departurePosition.Relocate(9949.0f, 2412.0f, 1327.0f);   // Darnassus Temple of Moon
            conn.arrivalMapId = MAP_OUTLAND;  // Exodar shares Outland map ID
            conn.arrivalPosition.Relocate(-4014.0f, -11897.0f, -1.3f);    // Exodar
            conn.requiresLevel = false;
            conn.minLevel = 0;
            connections.push_back(conn);

            // Stormwind -> Exodar (portal in Stormwind portal room - added Cataclysm+)
            conn.connectionId = nextId++;
            conn.name = "Stormwind to Exodar Portal";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(-8838.0f, 626.0f, 94.0f);     // Stormwind portal room
            conn.arrivalMapId = MAP_OUTLAND;  // Exodar shares Outland map ID
            conn.arrivalPosition.Relocate(-4014.0f, -11897.0f, -1.3f);    // Exodar
            conn.allianceOnly = true;
            conn.requiresLevel = false;
            conn.minLevel = 0;
            connections.push_back(conn);

            // Ironforge -> Exodar (portal in Ironforge - added Cataclysm+)
            conn.connectionId = nextId++;
            conn.name = "Ironforge to Exodar Portal";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(-4613.0f, -915.0f, 501.0f);   // Ironforge (Mystic Ward)
            conn.arrivalMapId = MAP_OUTLAND;  // Exodar shares Outland map ID
            conn.arrivalPosition.Relocate(-4014.0f, -11897.0f, -1.3f);    // Exodar
            connections.push_back(conn);
        }

        // ====================================================================
        // DARNASSUS / RUT'THERAN CONNECTIONS
        // ====================================================================
        {
            // Darnassus <-> Rut'theran Village (teleporter)
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Darnassus to Rut'theran Portal";
            conn.departureMapId = MAP_KALIMDOR;
            conn.departurePosition.Relocate(9949.0f, 2412.0f, 1327.0f);   // Darnassus
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(8181.0f, 1005.0f, 0.2f);        // Rut'theran Village
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.allianceOnly = true;
            connections.push_back(conn);

            conn.connectionId = nextId++;
            conn.name = "Rut'theran to Darnassus Portal";
            std::swap(conn.departurePosition, conn.arrivalPosition);
            connections.push_back(conn);

            // Darnassus -> Blasted Lands (Dark Portal access for Night Elves)
            conn.connectionId = nextId++;
            conn.name = "Darnassus to Blasted Lands Portal";
            conn.departureMapId = MAP_KALIMDOR;
            conn.departurePosition.Relocate(9949.0f, 2412.0f, 1327.0f);   // Temple of the Moon
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;
            conn.arrivalPosition.Relocate(-11903.0f, -3206.0f, -14.9f);   // Blasted Lands (Dark Portal)
            conn.requiresLevel = true;
            conn.minLevel = 58;
            connections.push_back(conn);
        }

        // ====================================================================
        // AZSUNA PORTAL (Legion starter zone)
        // ====================================================================
        {
            // Stormwind -> Azsuna (Broken Isles)
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Stormwind to Azsuna Portal";
            conn.departureMapId = MAP_EASTERN_KINGDOMS;
            conn.departurePosition.Relocate(-8838.0f, 626.0f, 94.0f);     // Stormwind portal room
            conn.arrivalMapId = MAP_BROKEN_ISLES;
            conn.arrivalPosition.Relocate(-155.0f, 6673.0f, 0.5f);        // Azsuna (Crumbled Palace)
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.allianceOnly = true;
            conn.requiresLevel = true;
            conn.minLevel = 98;
            connections.push_back(conn);

            // Orgrimmar -> Azsuna
            conn.connectionId = nextId++;
            conn.name = "Orgrimmar to Azsuna Portal";
            conn.departureMapId = MAP_KALIMDOR;
            conn.departurePosition.Relocate(1676.0f, -4315.0f, 61.2f);    // Orgrimmar portal room
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);

            // Azsuna -> Stormwind
            conn.connectionId = nextId++;
            conn.name = "Azsuna to Stormwind Portal";
            conn.departureMapId = MAP_BROKEN_ISLES;
            conn.departurePosition.Relocate(-155.0f, 6673.0f, 0.5f);
            conn.arrivalMapId = MAP_EASTERN_KINGDOMS;
            conn.arrivalPosition.Relocate(-8838.0f, 626.0f, 94.0f);
            conn.allianceOnly = true;
            conn.hordeOnly = false;
            conn.requiresLevel = false;
            connections.push_back(conn);

            // Azsuna -> Orgrimmar
            conn.connectionId = nextId++;
            conn.name = "Azsuna to Orgrimmar Portal";
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(1676.0f, -4315.0f, 61.2f);
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);
        }

        // ====================================================================
        // SILITHUS PORTAL (BFA - Sword of Sargeras location)
        // ====================================================================
        {
            // Boralus -> Silithus (Alliance - via BFA portal hub)
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Boralus to Silithus Portal";
            conn.departureMapId = MAP_KUL_TIRAS;
            conn.departurePosition.Relocate(-1774.0f, -1580.0f, 0.3f);    // Boralus
            conn.arrivalMapId = MAP_KALIMDOR;
            conn.arrivalPosition.Relocate(-6948.0f, 1037.0f, 5.9f);       // Silithus (Magni's camp)
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.allianceOnly = true;
            conn.requiresLevel = true;
            conn.minLevel = 50;
            connections.push_back(conn);

            // Dazar'alor -> Silithus (Horde)
            conn.connectionId = nextId++;
            conn.name = "Dazar'alor to Silithus Portal";
            conn.departureMapId = MAP_ZANDALAR;
            conn.departurePosition.Relocate(-1015.0f, 805.0f, 440.0f);    // Dazar'alor
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);

            // Silithus -> Boralus
            conn.connectionId = nextId++;
            conn.name = "Silithus to Boralus Portal";
            conn.departureMapId = MAP_KALIMDOR;
            conn.departurePosition.Relocate(-6948.0f, 1037.0f, 5.9f);
            conn.arrivalMapId = MAP_KUL_TIRAS;
            conn.arrivalPosition.Relocate(-1774.0f, -1580.0f, 0.3f);
            conn.allianceOnly = true;
            conn.hordeOnly = false;
            connections.push_back(conn);

            // Silithus -> Dazar'alor
            conn.connectionId = nextId++;
            conn.name = "Silithus to Dazar'alor Portal";
            conn.arrivalMapId = MAP_ZANDALAR;
            conn.arrivalPosition.Relocate(-1015.0f, 805.0f, 440.0f);
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);
        }

        // ====================================================================
        // NAZJATAR PORTAL (BFA 8.2)
        // ====================================================================
        {
            // Boralus -> Nazjatar (Alliance)
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Boralus to Nazjatar Portal";
            conn.departureMapId = MAP_KUL_TIRAS;
            conn.departurePosition.Relocate(-1774.0f, -1580.0f, 0.3f);
            conn.arrivalMapId = 1355;  // Nazjatar map ID
            conn.arrivalPosition.Relocate(-925.0f, 698.0f, 0.8f);         // Mezzamere (Alliance hub)
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.allianceOnly = true;
            conn.requiresLevel = true;
            conn.minLevel = 50;
            connections.push_back(conn);

            // Dazar'alor -> Nazjatar (Horde)
            conn.connectionId = nextId++;
            conn.name = "Dazar'alor to Nazjatar Portal";
            conn.departureMapId = MAP_ZANDALAR;
            conn.departurePosition.Relocate(-1015.0f, 805.0f, 440.0f);
            conn.arrivalPosition.Relocate(-985.0f, 435.0f, 0.8f);         // Newhome (Horde hub)
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);

            // Nazjatar -> Boralus
            conn.connectionId = nextId++;
            conn.name = "Nazjatar to Boralus Portal";
            conn.departureMapId = 1355;
            conn.departurePosition.Relocate(-925.0f, 698.0f, 0.8f);
            conn.arrivalMapId = MAP_KUL_TIRAS;
            conn.arrivalPosition.Relocate(-1774.0f, -1580.0f, 0.3f);
            conn.allianceOnly = true;
            conn.hordeOnly = false;
            connections.push_back(conn);

            // Nazjatar -> Dazar'alor
            conn.connectionId = nextId++;
            conn.name = "Nazjatar to Dazar'alor Portal";
            conn.departurePosition.Relocate(-985.0f, 435.0f, 0.8f);
            conn.arrivalMapId = MAP_ZANDALAR;
            conn.arrivalPosition.Relocate(-1015.0f, 805.0f, 440.0f);
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);
        }

        // ====================================================================
        // MECHAGON PORTAL (BFA 8.2)
        // ====================================================================
        {
            // Boralus -> Mechagon (Alliance)
            TransportConnection conn;
            conn.connectionId = nextId++;
            conn.type = TransportType::PORTAL;
            conn.name = "Boralus to Mechagon Portal";
            conn.departureMapId = MAP_KUL_TIRAS;
            conn.departurePosition.Relocate(-1774.0f, -1580.0f, 0.3f);
            conn.arrivalMapId = 1462;  // Mechagon map ID
            conn.arrivalPosition.Relocate(617.0f, 1418.0f, 45.0f);        // Rustbolt
            conn.waitTimeSeconds = PORTAL_WAIT_TIME;
            conn.travelTimeSeconds = PORTAL_TRAVEL_TIME;
            conn.allianceOnly = true;
            conn.requiresLevel = true;
            conn.minLevel = 50;
            connections.push_back(conn);

            // Dazar'alor -> Mechagon (Horde)
            conn.connectionId = nextId++;
            conn.name = "Dazar'alor to Mechagon Portal";
            conn.departureMapId = MAP_ZANDALAR;
            conn.departurePosition.Relocate(-1015.0f, 805.0f, 440.0f);
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);

            // Mechagon -> Boralus
            conn.connectionId = nextId++;
            conn.name = "Mechagon to Boralus Portal";
            conn.departureMapId = 1462;
            conn.departurePosition.Relocate(617.0f, 1418.0f, 45.0f);
            conn.arrivalMapId = MAP_KUL_TIRAS;
            conn.arrivalPosition.Relocate(-1774.0f, -1580.0f, 0.3f);
            conn.allianceOnly = true;
            conn.hordeOnly = false;
            connections.push_back(conn);

            // Mechagon -> Dazar'alor
            conn.connectionId = nextId++;
            conn.name = "Mechagon to Dazar'alor Portal";
            conn.arrivalMapId = MAP_ZANDALAR;
            conn.arrivalPosition.Relocate(-1015.0f, 805.0f, 440.0f);
            conn.allianceOnly = false;
            conn.hordeOnly = true;
            connections.push_back(conn);
        }

        TC_LOG_INFO("module.playerbot.travel", "TravelRouteManager: Initialized {} transport connections",
                    connections.size());
    }
}

// ============================================================================
// TRAVEL ROUTE MANAGER IMPLEMENTATION
// ============================================================================

TravelRouteManager::TravelRouteManager(Player* bot)
    : m_bot(bot)
    , m_lastRoutePlanTime(0)
    , m_lastStateUpdateTime(0)
    , m_transportStationaryStartTime(0)
{
    // Initialize static transport connections on first instance
    if (!s_connectionsInitialized)
    {
        InitializeTransportConnections();
        s_connectionsInitialized = true;
    }

    TC_LOG_DEBUG("module.playerbot.travel", "TravelRouteManager: Created for bot {}",
                 bot ? bot->GetName() : "NULL");
}

TravelRouteManager::~TravelRouteManager()
{
    if (m_activeRoute && m_activeRoute->IsActive())
    {
        TC_LOG_DEBUG("module.playerbot.travel", "TravelRouteManager: Destroying with active route");
    }
}

void TravelRouteManager::InitializeTransportConnections()
{
    // Step 1: Load hardcoded connections (ships, zeppelins, known portals)
    TransportDB::InitializeConnections(s_transportConnections);
    uint32 hardcodedCount = static_cast<uint32>(s_transportConnections.size());

    // Step 2: Supplement with database-driven portal connections from PortalDatabase
    // This ensures we pick up any portals that exist in the database but weren't hardcoded
    PortalDatabase& portalDb = PortalDatabase::Instance();
    if (portalDb.IsInitialized())
    {
        uint32 dbPortalCount = 0;
        uint32 nextId = static_cast<uint32>(s_transportConnections.size()) + 1000; // Offset to avoid ID conflicts

        // Get all portals from the database
        for (uint32 mapId = 0; mapId <= 2600; ++mapId) // Check all map IDs
        {
            auto portalsOnMap = portalDb.GetPortalsOnMap(mapId);
            for (PortalInfo const* portal : portalsOnMap)
            {
                if (!portal || portal->destinationMapId == 0)
                    continue;

                // Skip if same source and destination map (not cross-map)
                if (portal->sourceMapId == portal->destinationMapId)
                    continue;

                // Check if we already have this connection
                bool alreadyExists = false;
                for (auto const& existing : s_transportConnections)
                {
                    if (existing.departureMapId == portal->sourceMapId &&
                        existing.arrivalMapId == portal->destinationMapId)
                    {
                        // Check if positions are close (within 100 yards)
                        float dist = existing.departurePosition.GetExactDist(portal->sourcePosition);
                        if (dist < 100.0f)
                        {
                            alreadyExists = true;
                            break;
                        }
                    }
                }

                if (alreadyExists)
                    continue;

                // Create new connection from database portal
                TransportConnection conn;
                conn.connectionId = nextId++;
                conn.type = TransportType::PORTAL;
                conn.name = portal->name;
                conn.departureMapId = portal->sourceMapId;
                conn.departurePosition = portal->sourcePosition;
                conn.arrivalMapId = portal->destinationMapId;
                conn.arrivalPosition = portal->destinationPosition;
                conn.waitTimeSeconds = 0;
                conn.travelTimeSeconds = 5;
                conn.allianceOnly = (portal->faction == PortalFaction::ALLIANCE);
                conn.hordeOnly = (portal->faction == PortalFaction::HORDE);
                conn.requiresLevel = (portal->minLevel > 0);
                conn.minLevel = portal->minLevel;

                s_transportConnections.push_back(conn);
                ++dbPortalCount;
            }
        }

        TC_LOG_INFO("module.playerbot.travel",
            "TravelRouteManager: Added {} portal connections from database (total: {} hardcoded + {} from DB)",
            dbPortalCount, hardcodedCount, dbPortalCount);
    }
    else
    {
        TC_LOG_WARN("module.playerbot.travel",
            "TravelRouteManager: PortalDatabase not initialized - using only hardcoded connections");
    }

    // Step 3: Build lookup indices
    s_connectionsByDepartureMap.clear();
    s_connectionsByArrivalMap.clear();
    s_mapConnectivityGraph.clear();

    for (size_t i = 0; i < s_transportConnections.size(); ++i)
    {
        TransportConnection const& conn = s_transportConnections[i];
        s_connectionsByDepartureMap[conn.departureMapId].push_back(i);
        s_connectionsByArrivalMap[conn.arrivalMapId].push_back(i);

        // Build connectivity graph
        s_mapConnectivityGraph[conn.departureMapId].emplace_back(conn.arrivalMapId, &conn);
    }

    TC_LOG_INFO("module.playerbot.travel", "TravelRouteManager: Built connectivity graph for {} maps with {} total connections",
                s_mapConnectivityGraph.size(), s_transportConnections.size());
}

// ============================================================================
// ROUTE PLANNING
// ============================================================================

TravelRoute TravelRouteManager::PlanRoute(uint32 destinationMapId, Position const& destination)
{
    TravelRoute route;
    if (!m_bot)
    {
        route.overallState = TravelState::FAILED;
        return route;
    }

    uint32 currentMapId = m_bot->GetMapId();
    Position currentPos = m_bot->GetPosition();

    TC_LOG_DEBUG("module.playerbot.travel", "TravelRouteManager: Planning route for {} from MAP {} to MAP {} at ({:.1f}, {:.1f}, {:.1f})",
                 m_bot->GetName(), currentMapId, destinationMapId,
                 destination.GetPositionX(), destination.GetPositionY(), destination.GetPositionZ());

    // Initialize route
    route.routeId = static_cast<uint32>(GameTime::GetGameTimeMS());
    route.originMapId = currentMapId;
    route.originPosition = currentPos;
    route.destinationMapId = destinationMapId;
    route.destinationPosition = destination;
    route.currentLegIndex = 0;
    route.overallState = TravelState::IDLE;

    // Same map - just taxi or walk
    if (currentMapId == destinationMapId)
    {
        float distance = currentPos.GetExactDist(destination);

        if (distance < 50.0f)
        {
            // Just walk
            AddWalkingLeg(route, currentMapId, currentPos, destination);
        }
        else
        {
            // Try taxi, fallback to walk
            if (!AddTaxiLeg(route, currentMapId, currentPos, destination))
            {
                AddWalkingLeg(route, currentMapId, currentPos, destination);
            }
        }

        route.description = "Same-map travel";
        route.totalLegs = static_cast<uint32>(route.legs.size());
        m_stats.routesPlanned++;
        return route;
    }

    // Different map - need transport connections
    if (!BuildRoute(route, currentMapId, currentPos, destinationMapId, destination))
    {
        TC_LOG_WARN("module.playerbot.travel", "TravelRouteManager: Failed to build route from MAP {} to MAP {} for {}",
                    currentMapId, destinationMapId, m_bot->GetName());
        route.overallState = TravelState::FAILED;
        return route;
    }

    // Calculate totals
    route.totalLegs = static_cast<uint32>(route.legs.size());
    route.totalEstimatedTimeSeconds = 0;
    route.totalEstimatedCostCopper = 0;

    for (auto const& leg : route.legs)
    {
        route.totalEstimatedTimeSeconds += leg.estimatedTimeSeconds;
        route.totalEstimatedCostCopper += leg.estimatedCostCopper;
    }

    // Build description
    std::string desc = "";
    for (size_t i = 0; i < route.legs.size(); ++i)
    {
        if (i > 0) desc += " -> ";
        switch (route.legs[i].type)
        {
            case TransportType::TAXI_FLIGHT: desc += "Taxi"; break;
            case TransportType::SHIP: desc += "Ship"; break;
            case TransportType::ZEPPELIN: desc += "Zeppelin"; break;
            case TransportType::PORTAL: desc += "Portal"; break;
            case TransportType::BOAT: desc += "Boat"; break;
            case TransportType::HEARTHSTONE: desc += "Hearthstone"; break;
            case TransportType::WALK: desc += "Walk"; break;
            default: desc += "Unknown"; break;
        }
    }
    route.description = desc;

    TC_LOG_INFO("module.playerbot.travel", "TravelRouteManager: Planned {}-leg route for {}: {} ({}s, {}c)",
                route.totalLegs, m_bot->GetName(), route.description,
                route.totalEstimatedTimeSeconds, route.totalEstimatedCostCopper);

    m_stats.routesPlanned++;
    return route;
}

bool TravelRouteManager::BuildRoute(TravelRoute& route, uint32 fromMapId, Position const& fromPos,
                                     uint32 toMapId, Position const& toPos)
{
    // Use BFS to find shortest path through transport graph
    struct PathNode
    {
        uint32 mapId;
        TransportConnection const* connection;  // Connection used to reach this map
        size_t parentIndex;
    };

    std::queue<size_t> bfsQueue;
    std::vector<PathNode> visited;
    std::unordered_map<uint32, size_t> visitedMaps;

    // Start node
    visited.push_back({fromMapId, nullptr, SIZE_MAX});
    visitedMaps[fromMapId] = 0;
    bfsQueue.push(0);

    bool found = false;
    size_t destinationIndex = SIZE_MAX;

    while (!bfsQueue.empty() && !found)
    {
        size_t currentIndex = bfsQueue.front();
        bfsQueue.pop();

        uint32 currentMapId = visited[currentIndex].mapId;

        // Check all connections from current map
        auto it = s_mapConnectivityGraph.find(currentMapId);
        if (it == s_mapConnectivityGraph.end())
            continue;

        for (auto const& [neighborMapId, connection] : it->second)
        {
            // Skip if already visited
            if (visitedMaps.find(neighborMapId) != visitedMaps.end())
                continue;

            // Skip if bot can't use this connection
            if (!CanUseConnection(connection))
                continue;

            // Add to visited
            size_t newIndex = visited.size();
            visited.push_back({neighborMapId, connection, currentIndex});
            visitedMaps[neighborMapId] = newIndex;

            // Check if destination reached
            if (neighborMapId == toMapId)
            {
                found = true;
                destinationIndex = newIndex;
                break;
            }

            bfsQueue.push(newIndex);
        }
    }

    if (!found)
    {
        TC_LOG_DEBUG("module.playerbot.travel", "TravelRouteManager: No path found from MAP {} to MAP {}",
                     fromMapId, toMapId);
        return false;
    }

    // Reconstruct path
    std::vector<TransportConnection const*> pathConnections;
    size_t current = destinationIndex;
    while (visited[current].connection != nullptr)
    {
        pathConnections.push_back(visited[current].connection);
        current = visited[current].parentIndex;
    }

    // Reverse to get correct order
    std::reverse(pathConnections.begin(), pathConnections.end());

    // Build route legs
    Position legStartPos = fromPos;
    uint32 legStartMapId = fromMapId;

    for (TransportConnection const* conn : pathConnections)
    {
        // Walk to transport departure if needed
        if (legStartMapId == conn->departureMapId)
        {
            float distToTransport = legStartPos.GetExactDist(conn->departurePosition);
            if (distToTransport > 10.0f)
            {
                // Try taxi to transport, otherwise walk
                if (distToTransport > 100.0f)
                {
                    AddTaxiLeg(route, legStartMapId, legStartPos, conn->departurePosition);
                }
                else
                {
                    AddWalkingLeg(route, legStartMapId, legStartPos, conn->departurePosition);
                }
            }
        }

        // Add transport leg
        AddTransportLeg(route, conn);

        // Update position for next leg
        legStartPos = conn->arrivalPosition;
        legStartMapId = conn->arrivalMapId;
    }

    // Final leg - taxi/walk to destination
    float distToDestination = legStartPos.GetExactDist(toPos);
    if (distToDestination > 10.0f)
    {
        if (distToDestination > 100.0f)
        {
            if (!AddTaxiLeg(route, toMapId, legStartPos, toPos))
            {
                AddWalkingLeg(route, toMapId, legStartPos, toPos);
            }
        }
        else
        {
            AddWalkingLeg(route, toMapId, legStartPos, toPos);
        }
    }

    return true;
}

bool TravelRouteManager::AddTaxiLeg(TravelRoute& route, uint32 mapId, Position const& from, Position const& to)
{
    // Find taxi nodes for start and end positions (faction-aware)
    uint32 startNode = FlightMasterManager::FindNearestTaxiNode(from, mapId, m_bot);
    uint32 endNode = FlightMasterManager::FindNearestTaxiNode(to, mapId, m_bot);

    // If we can't find valid taxi nodes, leg creation fails
    if (startNode == 0 || endNode == 0)
    {
        TC_LOG_DEBUG("module.playerbot.travel",
            "TravelRouteManager::AddTaxiLeg - Cannot find taxi nodes for map {}: start={}, end={}",
            mapId, startNode, endNode);
        return false;
    }

    // Same node - no taxi needed
    if (startNode == endNode)
    {
        TC_LOG_DEBUG("module.playerbot.travel",
            "TravelRouteManager::AddTaxiLeg - Start and end are same taxi node {}, skipping taxi leg",
            startNode);
        return false;
    }

    // CRITICAL: Validate that a flight path actually EXISTS between these nodes
    // This prevents creating invalid taxi legs for cross-continent routes where
    // no taxi connection exists (e.g., Ratchet to Eastern Kingdoms)
    if (!FlightMasterManager::HasValidFlightPath(startNode, endNode, m_bot))
    {
        TC_LOG_WARN("module.playerbot.travel",
            "TravelRouteManager::AddTaxiLeg - NO VALID FLIGHT PATH from node {} to node {} on map {} - "
            "leg creation REJECTED (likely cross-continent or disconnected nodes)",
            startNode, endNode, mapId);
        return false;
    }

    TravelLeg leg;
    leg.legIndex = static_cast<uint32>(route.legs.size());
    leg.type = TransportType::TAXI_FLIGHT;
    leg.description = "Flight path";
    leg.startMapId = mapId;
    leg.startPosition = from;
    leg.endMapId = mapId;
    leg.endPosition = to;
    leg.currentState = TravelState::IDLE;

    // Set the taxi node fields - CRITICAL for QuestStrategy to use FlyToTaxiNode
    leg.taxiStartNode = startNode;
    leg.taxiEndNode = endNode;

    float distance = from.GetExactDist(to);
    leg.estimatedTimeSeconds = static_cast<uint32>(distance / 50.0f); // ~50 yards/sec flight speed
    leg.estimatedCostCopper = static_cast<uint32>(distance * 0.1f);   // Rough cost estimate

    TC_LOG_DEBUG("module.playerbot.travel",
        "TravelRouteManager::AddTaxiLeg - Added taxi leg: node {} -> node {} on map {}",
        startNode, endNode, mapId);

    route.legs.push_back(leg);
    return true;
}

bool TravelRouteManager::AddTransportLeg(TravelRoute& route, TransportConnection const* connection)
{
    if (!connection)
        return false;

    TravelLeg leg;
    leg.legIndex = static_cast<uint32>(route.legs.size());
    leg.type = connection->type;
    leg.description = connection->name;
    leg.startMapId = connection->departureMapId;
    leg.startPosition = connection->departurePosition;
    leg.endMapId = connection->arrivalMapId;
    leg.endPosition = connection->arrivalPosition;
    leg.connection = connection;
    leg.currentState = TravelState::IDLE;
    leg.estimatedTimeSeconds = connection->waitTimeSeconds + connection->travelTimeSeconds;
    leg.estimatedCostCopper = connection->costCopper;

    route.legs.push_back(leg);
    return true;
}

void TravelRouteManager::AddWalkingLeg(TravelRoute& route, uint32 mapId, Position const& from, Position const& to)
{
    TravelLeg leg;
    leg.legIndex = static_cast<uint32>(route.legs.size());
    leg.type = TransportType::WALK;
    leg.description = "Walking";
    leg.startMapId = mapId;
    leg.startPosition = from;
    leg.endMapId = mapId;
    leg.endPosition = to;
    leg.currentState = TravelState::IDLE;

    float distance = from.GetExactDist(to);
    leg.estimatedTimeSeconds = static_cast<uint32>(distance / 7.0f); // ~7 yards/sec run speed
    leg.estimatedCostCopper = 0;

    route.legs.push_back(leg);
}

bool TravelRouteManager::AddHearthstoneLeg(TravelRoute& route)
{
    if (!m_bot)
        return false;

    WorldLocation const& homebind = m_bot->m_homebind;

    TravelLeg leg;
    leg.legIndex = static_cast<uint32>(route.legs.size());
    leg.type = TransportType::HEARTHSTONE;
    leg.description = "Hearthstone";
    leg.startMapId = m_bot->GetMapId();
    leg.startPosition = m_bot->GetPosition();
    leg.endMapId = homebind.GetMapId();
    leg.endPosition.Relocate(homebind.GetPositionX(), homebind.GetPositionY(), homebind.GetPositionZ());
    leg.currentState = TravelState::IDLE;
    leg.estimatedTimeSeconds = 10; // Cast time
    leg.estimatedCostCopper = 0;

    route.legs.push_back(leg);
    return true;
}

// ============================================================================
// ROUTE EXECUTION
// ============================================================================

bool TravelRouteManager::StartRoute(TravelRoute&& route)
{
    if (route.legs.empty())
    {
        TC_LOG_WARN("module.playerbot.travel", "TravelRouteManager: Cannot start empty route");
        return false;
    }

    // DIAGNOSTIC: Log incoming route details BEFORE moving
    TC_LOG_DEBUG("module.playerbot.travel",
        "🔍 StartRoute: Bot {} - incoming route has {} legs, overallState={}",
        m_bot ? m_bot->GetName() : "NULL",
        route.legs.size(),
        static_cast<int>(route.overallState));

    for (size_t i = 0; i < route.legs.size(); ++i)
    {
        auto const& leg = route.legs[i];
        TC_LOG_DEBUG("module.playerbot.travel",
            "🔍 StartRoute: Bot {} - leg {} type={} desc='{}' state={} start=({:.1f},{:.1f},{:.1f}) end=({:.1f},{:.1f},{:.1f})",
            m_bot ? m_bot->GetName() : "NULL",
            i,
            static_cast<int>(leg.type),
            leg.description,
            static_cast<int>(leg.currentState),
            leg.startPosition.GetPositionX(), leg.startPosition.GetPositionY(), leg.startPosition.GetPositionZ(),
            leg.endPosition.GetPositionX(), leg.endPosition.GetPositionY(), leg.endPosition.GetPositionZ());
    }

    m_activeRoute = std::make_unique<TravelRoute>(std::move(route));
    m_activeRoute->overallState = TravelState::WALKING_TO_TRANSPORT;
    m_activeRoute->routeStartTime = static_cast<uint32>(GameTime::GetGameTimeMS());
    m_activeRoute->currentLegIndex = 0;

    if (!m_activeRoute->legs.empty())
    {
        m_activeRoute->legs[0].currentState = TravelState::WALKING_TO_TRANSPORT;
        m_activeRoute->legs[0].stateStartTime = m_activeRoute->routeStartTime;
    }

    TC_LOG_INFO("module.playerbot.travel", "TravelRouteManager: Started route for {} - {} legs: {}",
                m_bot ? m_bot->GetName() : "NULL",
                m_activeRoute->totalLegs,
                m_activeRoute->description);

    return true;
}

bool TravelRouteManager::Update(uint32 diff)
{
    // DIAGNOSTIC: Log entry state
    TC_LOG_DEBUG("module.playerbot.travel",
        "🔍 Update ENTRY: Bot {} - activeRoute={}, IsActive={}, overallState={}, legs={}, currentLegIdx={}",
        m_bot ? m_bot->GetName() : "NULL",
        m_activeRoute ? "YES" : "NO",
        m_activeRoute ? (m_activeRoute->IsActive() ? "YES" : "NO") : "N/A",
        m_activeRoute ? static_cast<int>(m_activeRoute->overallState) : -1,
        m_activeRoute ? m_activeRoute->legs.size() : 0,
        m_activeRoute ? m_activeRoute->currentLegIndex : 0);

    if (!m_activeRoute || !m_activeRoute->IsActive())
    {
        TC_LOG_DEBUG("module.playerbot.travel",
            "🔍 Update returning FALSE (no active route): Bot {} - activeRoute={}, IsActive={}",
            m_bot ? m_bot->GetName() : "NULL",
            m_activeRoute ? "YES" : "NO",
            m_activeRoute ? (m_activeRoute->IsActive() ? "YES" : "NO") : "N/A");
        return false;
    }

    uint32 now = static_cast<uint32>(GameTime::GetGameTimeMS());
    if (now - m_lastStateUpdateTime < STATE_UPDATE_INTERVAL_MS)
        return true;

    m_lastStateUpdateTime = now;

    TravelLeg* currentLeg = m_activeRoute->GetCurrentLeg();
    if (!currentLeg)
    {
        // All legs completed
        m_activeRoute->overallState = TravelState::COMPLETED;
        m_stats.routesCompleted++;
        m_stats.totalTravelTimeMs += now - m_activeRoute->routeStartTime;

        TC_LOG_INFO("module.playerbot.travel",
            "TravelRouteManager: Route completed for {} (no more legs - legIdx {} >= size {})",
            m_bot ? m_bot->GetName() : "NULL",
            m_activeRoute->currentLegIndex, m_activeRoute->legs.size());

        if (m_activeRoute->onCompleted)
            m_activeRoute->onCompleted(*m_activeRoute);

        return false;
    }

    // DIAGNOSTIC: Log leg state BEFORE UpdateLegState
    TC_LOG_DEBUG("module.playerbot.travel",
        "🔍 Update BEFORE UpdateLegState: Bot {} - leg {} type={} state={}",
        m_bot ? m_bot->GetName() : "NULL",
        currentLeg->legIndex,
        static_cast<int>(currentLeg->type),
        static_cast<int>(currentLeg->currentState));

    // Update current leg state
    UpdateLegState(*currentLeg, diff);

    // DIAGNOSTIC: Log leg state AFTER UpdateLegState
    TC_LOG_DEBUG("module.playerbot.travel",
        "🔍 Update AFTER UpdateLegState: Bot {} - leg {} state={}",
        m_bot ? m_bot->GetName() : "NULL",
        currentLeg->legIndex,
        static_cast<int>(currentLeg->currentState));

    // Check if current leg completed
    if (currentLeg->currentState == TravelState::COMPLETED)
    {
        TC_LOG_DEBUG("module.playerbot.travel",
            "🔍 Update: Bot {} leg {} COMPLETED, advancing to next leg",
            m_bot ? m_bot->GetName() : "NULL", currentLeg->legIndex);
        m_stats.totalLegsCompleted++;
        AdvanceToNextLeg();

        // DIAGNOSTIC: Check if route completed after advancing
        if (m_activeRoute->overallState == TravelState::COMPLETED)
        {
            TC_LOG_DEBUG("module.playerbot.travel",
                "🔍 Update: Bot {} route COMPLETED after AdvanceToNextLeg (currentLegIdx={}, legs={})",
                m_bot ? m_bot->GetName() : "NULL",
                m_activeRoute->currentLegIndex, m_activeRoute->legs.size());
        }
    }
    else if (currentLeg->currentState == TravelState::FAILED)
    {
        m_activeRoute->overallState = TravelState::FAILED;
        m_stats.routesFailed++;

        TC_LOG_WARN("module.playerbot.travel", "TravelRouteManager: Route failed for {} at leg {}",
                    m_bot ? m_bot->GetName() : "NULL", currentLeg->legIndex);

        if (m_activeRoute->onFailed)
            m_activeRoute->onFailed(*m_activeRoute, "Leg failed");

        return false;
    }

    // DIAGNOSTIC: Final check - did route state change during this update?
    bool stillActive = m_activeRoute->IsActive();
    TC_LOG_DEBUG("module.playerbot.travel",
        "🔍 Update returning TRUE (still processing): Bot {} - overallState={}, stillActive={}",
        m_bot ? m_bot->GetName() : "NULL",
        static_cast<int>(m_activeRoute->overallState),
        stillActive ? "YES" : "NO");

    // NOTE: Always return true here. If route just completed via AdvanceToNextLeg,
    // the NEXT Update() call will detect IsActive()=false and return false properly.
    // This gives one more tick to process the state change.
    return true;
}

void TravelRouteManager::CancelRoute()
{
    if (m_activeRoute)
    {
        TC_LOG_INFO("module.playerbot.travel", "TravelRouteManager: Route cancelled for {}",
                    m_bot ? m_bot->GetName() : "NULL");
        m_activeRoute->overallState = TravelState::FAILED;
        m_activeRoute.reset();
    }
}

void TravelRouteManager::UpdateLegState(TravelLeg& leg, uint32 /*diff*/)
{
    if (!m_bot)
        return;

    switch (leg.type)
    {
        case TransportType::WALK:
        {
            // Check if we've arrived
            bool atDestination = IsNearPosition(leg.endPosition, 15.0f);
            if (atDestination)
            {
                TC_LOG_DEBUG("module.playerbot.travel",
                    "🔍 WALK leg {} COMPLETED: Bot {} at ({:.1f}, {:.1f}, {:.1f}) is within 15 yards of destination ({:.1f}, {:.1f}, {:.1f})",
                    leg.legIndex, m_bot->GetName(),
                    m_bot->GetPositionX(), m_bot->GetPositionY(), m_bot->GetPositionZ(),
                    leg.endPosition.GetPositionX(), leg.endPosition.GetPositionY(), leg.endPosition.GetPositionZ());
                leg.currentState = TravelState::COMPLETED;
            }
            else if (leg.currentState == TravelState::IDLE || leg.currentState == TravelState::WALKING_TO_TRANSPORT)
            {
                // Use validated pathfinding for transport walking
                if (BotAI* ai = GetBotAI(m_bot))
                {
                    if (!ai->MoveTo(leg.endPosition, true))
                    {
                        // Fallback to legacy if validation fails
                        m_bot->GetMotionMaster()->MovePoint(0, leg.endPosition);
                    }
                }
                else
                {
                    // Non-bot player - use standard movement
                    m_bot->GetMotionMaster()->MovePoint(0, leg.endPosition);
                }
                leg.currentState = TravelState::WALKING_TO_TRANSPORT;
            }
            break;
        }

        case TransportType::TAXI_FLIGHT:
        {
            // Use dedicated taxi flight handler with proper state machine
            HandleTaxiFlight(leg);
            break;
        }

        case TransportType::SHIP:
        case TransportType::ZEPPELIN:
        case TransportType::BOAT:
        {
            HandleOnTransport(leg);
            break;
        }

        case TransportType::PORTAL:
        {
            HandlePortal(leg);
            break;
        }

        case TransportType::HEARTHSTONE:
        {
            HandleHearthstone(leg);
            break;
        }

        default:
            leg.currentState = TravelState::FAILED;
            break;
    }
}

void TravelRouteManager::AdvanceToNextLeg()
{
    if (!m_activeRoute)
        return;

    uint32 prevLegIndex = m_activeRoute->currentLegIndex;
    m_activeRoute->currentLegIndex++;

    TC_LOG_DEBUG("module.playerbot.travel",
        "🔍 AdvanceToNextLeg: Bot {} - advanced from leg {} to leg {}, totalLegs={}",
        m_bot ? m_bot->GetName() : "NULL",
        prevLegIndex, m_activeRoute->currentLegIndex, m_activeRoute->legs.size());

    if (m_activeRoute->currentLegIndex >= m_activeRoute->legs.size())
    {
        TC_LOG_DEBUG("module.playerbot.travel",
            "🔍 AdvanceToNextLeg: Bot {} - NO MORE LEGS, marking route COMPLETED",
            m_bot ? m_bot->GetName() : "NULL");
        m_activeRoute->overallState = TravelState::COMPLETED;
        return;
    }

    TravelLeg& nextLeg = m_activeRoute->legs[m_activeRoute->currentLegIndex];
    nextLeg.currentState = TravelState::WALKING_TO_TRANSPORT;
    nextLeg.stateStartTime = static_cast<uint32>(GameTime::GetGameTimeMS());

    TC_LOG_DEBUG("module.playerbot.travel", "TravelRouteManager: Advanced to leg {} for {}",
                 m_activeRoute->currentLegIndex, m_bot ? m_bot->GetName() : "NULL");
}

void TravelRouteManager::HandleOnTransport(TravelLeg& leg)
{
    // ========================================================================
    // ENTERPRISE-GRADE TRANSPORT HANDLING
    // ========================================================================
    // Implements REAL ship/zeppelin boarding using TrinityCore's Transport API:
    // 1. Walk to departure dock
    // 2. Wait for transport to arrive and stop
    // 3. Board using Transport::AddPassenger()
    // 4. Ride transport (position updates automatically)
    // 5. Detect arrival at destination dock
    // 6. Disembark using Transport::RemovePassenger()
    // ========================================================================

    if (!m_bot || !leg.connection)
    {
        leg.currentState = TravelState::FAILED;
        return;
    }

    // Re-validate faction before any transport operation
    if (!CanUseConnection(leg.connection))
    {
        TC_LOG_WARN("module.playerbot.travel",
            "HandleOnTransport: Bot {} can no longer use connection {} (faction/level changed)",
            m_bot->GetName(), leg.connection->name);
        leg.currentState = TravelState::FAILED;
        return;
    }

    uint32 now = static_cast<uint32>(GameTime::GetGameTimeMS());

    switch (leg.currentState)
    {
        case TravelState::IDLE:
        case TravelState::WALKING_TO_TRANSPORT:
        {
            // Walk to departure dock position
            float distToDeparture = m_bot->GetDistance(leg.startPosition);

            if (distToDeparture > 15.0f)
            {
                // Not at dock yet - walk there
                if (leg.currentState != TravelState::WALKING_TO_TRANSPORT)
                {
                    TC_LOG_DEBUG("module.playerbot.travel",
                        "HandleOnTransport: Bot {} walking to {} departure at ({:.1f}, {:.1f}, {:.1f}) - distance {:.1f}",
                        m_bot->GetName(), leg.connection->name,
                        leg.startPosition.GetPositionX(), leg.startPosition.GetPositionY(), leg.startPosition.GetPositionZ(),
                        distToDeparture);
                }

                // Use validated pathfinding for walking to departure dock
                if (BotAI* ai = GetBotAI(m_bot))
                {
                    if (!ai->MoveTo(leg.startPosition, true))
                    {
                        // Fallback to legacy if validation fails
                        m_bot->GetMotionMaster()->MovePoint(0, leg.startPosition);
                    }
                }
                else
                {
                    // Non-bot player - use standard movement
                    m_bot->GetMotionMaster()->MovePoint(0, leg.startPosition);
                }
                leg.currentState = TravelState::WALKING_TO_TRANSPORT;
            }
            else
            {
                // Arrived at dock - transition to waiting for transport
                TC_LOG_DEBUG("module.playerbot.travel",
                    "HandleOnTransport: Bot {} arrived at {} departure dock, waiting for transport",
                    m_bot->GetName(), leg.connection->name);
                leg.currentState = TravelState::WAITING_FOR_TRANSPORT;
                leg.stateStartTime = now;
            }
            break;
        }

        case TravelState::WAITING_FOR_TRANSPORT:
        {
            // ========================================================================
            // TRANSPORT BOARDING LOGIC
            // ========================================================================
            // Ships/zeppelins don't use IsStopped() for natural dock pauses.
            // IsStopped() only works for player-requested stops.
            //
            // We use TWO methods to detect when transport is at dock:
            // 1. Distance-based: Transport within 50yd of dock position
            // 2. Movement-based: Transport hasn't moved for 2+ seconds (safety check)
            //
            // Bot boarding range: 30yd (based on observed minimum of ~18.7yd)
            // ========================================================================

            // Look for transport (150yd radius to track it even when sailing)
            ::Transport* transport = FindTransportAtPosition(leg.startPosition, leg.connection->transportEntry, 150.0f);

            if (transport)
            {
                // Get current transport position
                Position currentTransportPos = transport->GetPosition();

                // Calculate distances
                float transportToDockDist = transport->GetDistance(leg.startPosition);
                float botToTransportDist = m_bot->GetDistance(transport);

                // ====================================================================
                // MOVEMENT TRACKING: Check if transport has stopped moving
                // ====================================================================
                float distanceMoved = m_lastTransportPosition.GetExactDist(&currentTransportPos);
                bool transportHasMoved = distanceMoved > TRANSPORT_MOVEMENT_THRESHOLD;

                if (transportHasMoved)
                {
                    // Transport is moving - reset stationary timer
                    m_transportStationaryStartTime = now;
                    m_lastTransportPosition = currentTransportPos;
                }

                // Calculate how long transport has been stationary
                uint32 stationaryDuration = now - m_transportStationaryStartTime;
                bool transportIsStopped = (stationaryDuration >= TRANSPORT_STOPPED_DURATION_MS);

                TC_LOG_DEBUG("module.playerbot.travel",
                    "WAITING_FOR_TRANSPORT: Bot {} - transport {} at ({:.1f}, {:.1f}, {:.1f}), "
                    "dock={:.1f}yd, bot={:.1f}yd, moved={:.2f}yd, stationary={}ms, stopped={}",
                    m_bot->GetName(), transport->GetEntry(),
                    transport->GetPositionX(), transport->GetPositionY(), transport->GetPositionZ(),
                    transportToDockDist, botToTransportDist, distanceMoved, stationaryDuration,
                    transportIsStopped ? "YES" : "NO");

                // ====================================================================
                // BOARDING CONDITIONS
                // ====================================================================
                // Primary: Transport within 50yd of dock AND stopped for 2+ seconds
                // Fallback: Transport within 30yd of dock (very close = definitely at dock)
                constexpr float TRANSPORT_AT_DOCK_RANGE = 50.0f;
                constexpr float TRANSPORT_VERY_CLOSE_RANGE = 30.0f;
                constexpr float BOT_BOARDING_RANGE = 30.0f;  // Based on observed ~18.7yd minimum

                bool transportAtDock = (transportToDockDist <= TRANSPORT_AT_DOCK_RANGE && transportIsStopped) ||
                                       (transportToDockDist <= TRANSPORT_VERY_CLOSE_RANGE);

                if (transportAtDock)
                {
                    // Transport is at dock! Now check if bot is close enough to board
                    if (botToTransportDist <= BOT_BOARDING_RANGE)
                    {
                        // ================================================================
                        // VISUAL BOARDING: Walk bot onto the transport deck first
                        // ================================================================
                        // Instead of teleporting the bot, we:
                        // 1. Calculate a position ON the transport deck
                        // 2. Have the bot walk to that position
                        // 3. Once close enough, add as passenger
                        // ================================================================

                        // Calculate world position on transport deck
                        Position boardingOffset;
                        boardingOffset.Relocate(0.0f, 0.0f, 2.0f, m_bot->GetOrientation());
                        Position deckWorldPos = transport->GetPositionWithOffset(boardingOffset);

                        // Check if bot is already on/very close to the deck
                        float distToDeck = m_bot->GetExactDist(&deckWorldPos);

                        if (distToDeck < 5.0f)
                        {
                            // Bot is on the deck - now add as passenger!
                            TC_LOG_INFO("module.playerbot.travel",
                                "HandleOnTransport: Bot {} BOARDING {} (entry {}) - on deck at {:.1f}yd",
                                m_bot->GetName(), leg.connection->name, transport->GetEntry(), distToDeck);

                            // Add bot as passenger with the deck offset
                            transport->AddPassenger(m_bot, boardingOffset);

                            // Ensure bot position is synchronized with transport
                            if (Map* map = m_bot->GetMap())
                            {
                                map->PlayerRelocation(m_bot, deckWorldPos.GetPositionX(),
                                    deckWorldPos.GetPositionY(), deckWorldPos.GetPositionZ(),
                                    deckWorldPos.GetOrientation());
                                m_bot->SetFallInformation(0, deckWorldPos.GetPositionZ());

                                // Broadcast movement update
                                WorldPackets::Movement::MoveUpdate moveUpdate;
                                moveUpdate.Status = &m_bot->m_movementInfo;
                                m_bot->SendMessageToSet(moveUpdate.Write(), false);
                            }

                            TC_LOG_INFO("module.playerbot.travel",
                                "HandleOnTransport: Bot {} now passenger on transport at ({:.1f}, {:.1f}, {:.1f})",
                                m_bot->GetName(), deckWorldPos.GetPositionX(), deckWorldPos.GetPositionY(), deckWorldPos.GetPositionZ());

                            m_currentTransportGuid = transport->GetGUID();
                            m_transportStationaryStartTime = 0;

                            leg.currentState = TravelState::ON_TRANSPORT;
                            leg.stateStartTime = now;
                        }
                        else
                        {
                            // Bot needs to walk onto the deck
                            TC_LOG_DEBUG("module.playerbot.travel",
                                "HandleOnTransport: Bot {} walking onto {} deck ({:.1f}yd away)",
                                m_bot->GetName(), leg.connection->name, distToDeck);

                            // Use validated pathfinding for walking onto transport deck
                            if (BotAI* ai = GetBotAI(m_bot))
                            {
                                if (!ai->MoveTo(deckWorldPos, true))
                                {
                                    // Fallback to legacy if validation fails
                                    m_bot->GetMotionMaster()->MovePoint(0, deckWorldPos);
                                }
                            }
                            else
                            {
                                // Non-bot player - use standard movement
                                m_bot->GetMotionMaster()->MovePoint(0, deckWorldPos);
                            }
                        }
                    }
                    else
                    {
                        // Transport at dock but bot too far - walk towards the gangway/ramp
                        // Use a position between bot and transport deck as intermediate point
                        Position boardingOffset;
                        boardingOffset.Relocate(0.0f, 0.0f, 2.0f, m_bot->GetOrientation());
                        Position deckWorldPos = transport->GetPositionWithOffset(boardingOffset);

                        TC_LOG_DEBUG("module.playerbot.travel",
                            "HandleOnTransport: Bot {} - transport at dock, walking to gangway ({:.1f}yd to deck)",
                            m_bot->GetName(), botToTransportDist);

                        // Use validated pathfinding for walking to gangway
                        if (BotAI* ai = GetBotAI(m_bot))
                        {
                            if (!ai->MoveTo(deckWorldPos, true))
                            {
                                // Fallback to legacy if validation fails
                                m_bot->GetMotionMaster()->MovePoint(0, deckWorldPos);
                            }
                        }
                        else
                        {
                            // Non-bot player - use standard movement
                            m_bot->GetMotionMaster()->MovePoint(0, deckWorldPos);
                        }
                    }
                }
                else
                {
                    // Transport not at dock yet - still sailing
                    TC_LOG_DEBUG("module.playerbot.travel",
                        "HandleOnTransport: Bot {} - transport not at dock ({:.1f}yd, stopped={})",
                        m_bot->GetName(), transportToDockDist, transportIsStopped ? "YES" : "NO");
                }
            }
            else
            {
                // Transport not found - reset tracking
                m_transportStationaryStartTime = now;
                TC_LOG_DEBUG("module.playerbot.travel",
                    "WAITING_FOR_TRANSPORT: Bot {} - transport entry {} not found",
                    m_bot->GetName(), leg.connection->transportEntry);
            }

            // Check for timeout
            uint32 elapsed = now - leg.stateStartTime;
            uint32 maxWaitTime = (leg.connection->waitTimeSeconds + 180) * 1000; // Add 3 min buffer

            if (elapsed > maxWaitTime)
            {
                TC_LOG_WARN("module.playerbot.travel",
                    "HandleOnTransport: Bot {} timed out waiting for {} after {}s",
                    m_bot->GetName(), leg.connection->name, elapsed / 1000);
                leg.currentState = TravelState::FAILED;
            }
            else if (elapsed % 30000 < 500) // Log every 30 seconds
            {
                TC_LOG_DEBUG("module.playerbot.travel",
                    "HandleOnTransport: Bot {} waiting for {} - {}s elapsed",
                    m_bot->GetName(), leg.connection->name, elapsed / 1000);
            }
            break;
        }

        case TravelState::ON_TRANSPORT:
        {
            // Check if we're still on the transport
            ::Transport* transport = dynamic_cast<::Transport*>(m_bot->GetTransport());

            TC_LOG_DEBUG("module.playerbot.travel",
                "ON_TRANSPORT: Bot {} - GetTransport={}, botMap={}, destMap={}, botPos=({:.1f},{:.1f},{:.1f})",
                m_bot->GetName(),
                transport ? std::to_string(transport->GetEntry()).c_str() : "NULL",
                m_bot->GetMapId(), leg.endMapId,
                m_bot->GetPositionX(), m_bot->GetPositionY(), m_bot->GetPositionZ());

            if (!transport)
            {
                // Bot is not on a transport - check if arrived at destination map
                TC_LOG_DEBUG("module.playerbot.travel",
                    "ON_TRANSPORT: Bot {} not on transport - checking arrival (botMap={}, destMap={})",
                    m_bot->GetName(), m_bot->GetMapId(), leg.endMapId);

                // Check if we've arrived at destination MAP (not just position - ship might drop us anywhere)
                if (m_bot->GetMapId() == leg.endMapId)
                {
                    TC_LOG_INFO("module.playerbot.travel",
                        "HandleOnTransport: Bot {} arrived at destination MAP {} via {}",
                        m_bot->GetName(), leg.endMapId, leg.connection->name);
                    leg.currentState = TravelState::COMPLETED;
                }
                else
                {
                    // Check distance to destination as fallback
                    float distToDest = m_bot->GetDistance(leg.endPosition);
                    if (distToDest < 500.0f)
                    {
                        TC_LOG_INFO("module.playerbot.travel",
                            "HandleOnTransport: Bot {} near destination ({:.1f}yd) via {}",
                            m_bot->GetName(), distToDest, leg.connection->name);
                        leg.currentState = TravelState::COMPLETED;
                    }
                    else
                    {
                        TC_LOG_WARN("module.playerbot.travel",
                            "HandleOnTransport: Bot {} not at destination (map {} vs {}, dist={:.1f}yd) - FAILED",
                            m_bot->GetName(), m_bot->GetMapId(), leg.endMapId, distToDest);
                        leg.currentState = TravelState::FAILED;
                    }
                }
                m_currentTransportGuid.Clear();
                break;
            }

            // Still on transport - check if arrived at destination
            float distToArrival = transport->GetDistance(leg.endPosition);

            // ====================================================================
            // DISEMBARKATION: Track transport movement to detect when stopped
            // ====================================================================
            // Same logic as boarding - transport must be STOPPED for 2+ seconds
            // before we disembark, otherwise bot might fall into the ocean!
            // ====================================================================
            Position currentTransportPos = transport->GetPosition();
            float distanceMoved = m_lastTransportPosition.GetExactDist(&currentTransportPos);
            bool transportHasMoved = distanceMoved > TRANSPORT_MOVEMENT_THRESHOLD;

            if (transportHasMoved)
            {
                // Transport is still moving - reset stationary timer
                m_transportStationaryStartTime = now;
                m_lastTransportPosition = currentTransportPos;
            }

            uint32 stationaryDuration = now - m_transportStationaryStartTime;
            bool transportIsStopped = (stationaryDuration >= TRANSPORT_STOPPED_DURATION_MS);

            TC_LOG_DEBUG("module.playerbot.travel",
                "ON_TRANSPORT: Bot {} on transport {} - distToArrival={:.1f}yd, transportMap={}, destMap={}, stopped={}",
                m_bot->GetName(), transport->GetEntry(), distToArrival,
                transport->GetMapId(), leg.endMapId, transportIsStopped ? "YES" : "NO");

            // Disembark conditions:
            // 1. Transport is on destination map
            // 2. Transport is within 100yd of destination dock
            // 3. Transport has STOPPED for at least 2 seconds
            constexpr float DISEMBARK_DISTANCE = 100.0f;

            if (transport->GetMapId() == leg.endMapId && distToArrival < DISEMBARK_DISTANCE && transportIsStopped)
            {
                // Transport stopped at destination dock - DISEMBARK!
                TC_LOG_INFO("module.playerbot.travel",
                    "HandleOnTransport: Bot {} disembarking {} at destination (stopped for {}ms)",
                    m_bot->GetName(), leg.connection->name, stationaryDuration);

                // Remove from transport
                transport->RemovePassenger(m_bot);
                m_currentTransportGuid.Clear();

                // Use validated pathfinding for arrival position
                if (BotAI* ai = GetBotAI(m_bot))
                {
                    if (!ai->MoveTo(leg.endPosition, true))
                    {
                        // Fallback to legacy if validation fails
                        m_bot->GetMotionMaster()->MovePoint(0, leg.endPosition);
                    }
                }
                else
                {
                    // Non-bot player - use standard movement
                    m_bot->GetMotionMaster()->MovePoint(0, leg.endPosition);
                }
                leg.currentState = TravelState::ARRIVING;
                leg.stateStartTime = now;
            }
            else if (transport->GetMapId() == leg.endMapId && distToArrival < DISEMBARK_DISTANCE)
            {
                // Near destination but transport still moving - wait for it to stop
                TC_LOG_DEBUG("module.playerbot.travel",
                    "ON_TRANSPORT: Bot {} waiting for {} to stop at dock (stationary {}ms)",
                    m_bot->GetName(), leg.connection->name, stationaryDuration);
            }
            else
            {
                // Still traveling - check timeout
                uint32 elapsed = now - leg.stateStartTime;
                uint32 maxTravelTime = (leg.connection->travelTimeSeconds + 180) * 1000; // Add 3 min buffer

                if (elapsed > maxTravelTime)
                {
                    TC_LOG_WARN("module.playerbot.travel",
                        "HandleOnTransport: Bot {} travel timeout on {} after {}s - forcing teleport fallback",
                        m_bot->GetName(), leg.connection->name, elapsed / 1000);

                    // Emergency fallback: remove from transport and teleport
                    transport->RemovePassenger(m_bot);
                    m_currentTransportGuid.Clear();

                    m_bot->TeleportTo(leg.endMapId,
                                      leg.endPosition.GetPositionX(),
                                      leg.endPosition.GetPositionY(),
                                      leg.endPosition.GetPositionZ(),
                                      leg.endPosition.GetOrientation());
                    leg.currentState = TravelState::COMPLETED;
                }
            }
            break;
        }

        case TravelState::ARRIVING:
        {
            // Walking from transport to final position
            if (IsNearPosition(leg.endPosition, 10.0f))
            {
                TC_LOG_DEBUG("module.playerbot.travel",
                    "HandleOnTransport: Bot {} completed {} journey",
                    m_bot->GetName(), leg.connection->name);
                leg.currentState = TravelState::COMPLETED;
            }
            else
            {
                // Check timeout
                uint32 elapsed = now - leg.stateStartTime;
                if (elapsed > 60000) // 1 minute to walk off
                {
                    // Close enough - mark completed
                    leg.currentState = TravelState::COMPLETED;
                }
            }
            break;
        }

        default:
            TC_LOG_WARN("module.playerbot.travel",
                "HandleOnTransport: Bot {} in unexpected state {}",
                m_bot->GetName(), static_cast<int>(leg.currentState));
            break;
    }
}

::Transport* TravelRouteManager::FindTransportAtPosition(Position const& /*pos*/, uint32 transportEntry, float range) const
{
    if (!m_bot || !m_bot->GetMap())
    {
        TC_LOG_DEBUG("module.playerbot.travel",
            "FindTransportAtPosition: No bot or map - returning nullptr");
        return nullptr;
    }

    TC_LOG_DEBUG("module.playerbot.travel",
        "FindTransportAtPosition: Bot {} at ({:.1f}, {:.1f}, {:.1f}) searching for entry {} within {:.0f}yd",
        m_bot->GetName(),
        m_bot->GetPositionX(), m_bot->GetPositionY(), m_bot->GetPositionZ(),
        transportEntry, range);

    // ========================================================================
    // TRANSPORT LOOKUP - CORRECT METHOD
    // ========================================================================
    // Transports (ships, zeppelins) are NOT stored in the regular GameObject grid!
    // They are managed by TransportMgr and stored in Map::_transports.
    // FindNearestGameObject() will NOT find them!
    //
    // Correct approach: Use Player::m_visibleTransports which contains GUIDs
    // of all transports visible to the player, then look them up via Map::GetTransport()
    // ========================================================================

    Map* map = m_bot->GetMap();
    ::Transport* bestTransport = nullptr;
    float bestDistance = range;

    // Method 1: Search through visible transports (CORRECT for ships/zeppelins)
    TC_LOG_DEBUG("module.playerbot.travel",
        "FindTransportAtPosition: Bot {} has {} visible transports",
        m_bot->GetName(), m_bot->m_visibleTransports.size());

    for (ObjectGuid const& transportGuid : m_bot->m_visibleTransports)
    {
        ::Transport* transport = map->GetTransport(transportGuid);
        if (!transport)
        {
            TC_LOG_DEBUG("module.playerbot.travel",
                "FindTransportAtPosition: Transport GUID {} not found on map",
                transportGuid.ToString());
            continue;
        }

        TC_LOG_DEBUG("module.playerbot.travel",
            "FindTransportAtPosition: Checking transport entry {} '{}' at ({:.1f}, {:.1f}, {:.1f})",
            transport->GetEntry(),
            transport->GetName(),
            transport->GetPositionX(), transport->GetPositionY(), transport->GetPositionZ());

        // Check entry match if specified
        if (transportEntry != 0 && transport->GetEntry() != transportEntry)
        {
            TC_LOG_DEBUG("module.playerbot.travel",
                "FindTransportAtPosition: Entry mismatch - wanted {}, got {}",
                transportEntry, transport->GetEntry());
            continue;
        }

        // Check distance
        float dist = m_bot->GetDistance(transport);
        TC_LOG_DEBUG("module.playerbot.travel",
            "FindTransportAtPosition: Transport {} distance={:.1f}, range={:.1f}",
            transport->GetEntry(), dist, range);

        if (dist <= bestDistance)
        {
            bestDistance = dist;
            bestTransport = transport;
            TC_LOG_DEBUG("module.playerbot.travel",
                "FindTransportAtPosition: New best candidate - entry {} at {:.1f}yd",
                transport->GetEntry(), dist);
        }
    }

    if (bestTransport)
    {
        TC_LOG_DEBUG("module.playerbot.travel",
            "FindTransportAtPosition: FOUND transport entry {} '{}' at {:.1f}yd - IsStopped={}",
            bestTransport->GetEntry(), bestTransport->GetName(), bestDistance,
            bestTransport->IsStopped() ? "YES" : "NO");
        return bestTransport;
    }

    // Log all visible transport entries to help diagnose entry mismatches
    if (transportEntry != 0 && !m_bot->m_visibleTransports.empty())
    {
        std::string visibleEntries;
        for (ObjectGuid const& transportGuid : m_bot->m_visibleTransports)
        {
            if (::Transport* transport = map->GetTransport(transportGuid))
            {
                if (!visibleEntries.empty())
                    visibleEntries += ", ";
                visibleEntries += std::to_string(transport->GetEntry()) + " (" + transport->GetName() + ")";
            }
        }
        TC_LOG_INFO("module.playerbot.travel",
            "FindTransportAtPosition: Bot {} looking for entry {} but visible transports are: [{}]",
            m_bot->GetName(), transportEntry, visibleEntries);
    }

    // Method 2: Fallback - try FindNearestGameObject (rarely works for transports, but try anyway)
    // This might work for some static transport-like objects
    if (transportEntry != 0)
    {
        GameObject* go = m_bot->FindNearestGameObject(transportEntry, range, false);
        if (go)
        {
            TC_LOG_DEBUG("module.playerbot.travel",
                "FindTransportAtPosition: Fallback found GameObject entry {} type={}",
                go->GetEntry(), static_cast<int>(go->GetGoType()));

            if (::Transport* transport = go->ToTransport())
            {
                TC_LOG_DEBUG("module.playerbot.travel",
                    "FindTransportAtPosition: Fallback GameObject is a Transport");
                return transport;
            }
        }
    }

    TC_LOG_DEBUG("module.playerbot.travel",
        "FindTransportAtPosition: No transport found - visibleTransports={}, entry={}",
        m_bot->m_visibleTransports.size(), transportEntry);
    return nullptr;
}

void TravelRouteManager::HandlePortal(TravelLeg& leg)
{
    // ========================================================================
    // ENTERPRISE-GRADE PORTAL HANDLING WITH DATABASE LOOKUP
    // ========================================================================
    // Priority order for finding portal locations:
    // 1. PortalDatabase lookup (accurate spawn positions from DB)
    // 2. Dynamic runtime search (FindNearbyPortalObjects)
    // 3. Fallback to connection data (hardcoded coordinates)
    //
    // Steps:
    // 1. Walk to portal location (determined by priority above)
    // 2. Use portal spell OR interact with portal GameObject
    // 3. Validate arrival at destination
    // ========================================================================

    if (!m_bot)
    {
        leg.currentState = TravelState::FAILED;
        return;
    }

    // Get connection name for logging (may be nullptr)
    std::string portalName = leg.connection ? leg.connection->name : "Unknown Portal";

    uint32 now = static_cast<uint32>(GameTime::GetGameTimeMS());

    // ========================================================================
    // PORTAL POSITION RESOLUTION
    // ========================================================================
    // On first entry (IDLE state), resolve actual portal position using:
    // 1. PortalDatabase (most accurate)
    // 2. Dynamic search (fallback)
    // 3. Connection data (last resort)
    // ========================================================================

    if (leg.currentState == TravelState::IDLE)
    {
        Position resolvedPortalPos = leg.startPosition;  // Default to connection data
        Position resolvedDestPos = leg.endPosition;
        bool foundPortal = false;

        // === STEP 1: Try PortalDatabase ===
        PortalDatabase& portalDb = PortalDatabase::Instance();
        if (portalDb.IsInitialized())
        {
            // Look for best portal to our destination
            PortalInfo const* portalInfo = portalDb.GetBestPortalForDestination(
                m_bot, leg.endMapId, leg.endPosition);

            if (portalInfo && portalInfo->CanPlayerUse(m_bot))
            {
                resolvedPortalPos = portalInfo->sourcePosition;
                resolvedDestPos = portalInfo->destinationPosition;
                portalName = portalInfo->name;
                foundPortal = true;

                TC_LOG_DEBUG("module.playerbot.travel",
                    "HandlePortal: Bot {} found portal '{}' (entry {}) in database at ({:.1f}, {:.1f}, {:.1f})",
                    m_bot->GetName(), portalInfo->name, portalInfo->gameObjectEntry,
                    resolvedPortalPos.GetPositionX(), resolvedPortalPos.GetPositionY(), resolvedPortalPos.GetPositionZ());
            }
        }

        // === STEP 2: Dynamic search fallback ===
        if (!foundPortal)
        {
            // Search for portal GameObjects near connection's estimated position
            std::vector<GameObject*> nearbyPortals = portalDb.FindNearbyPortalObjects(m_bot, 200.0f);

            for (GameObject* go : nearbyPortals)
            {
                if (!go || !go->isSpawned())
                    continue;

                // Get the spell from this portal
                GameObjectTemplate const* goTemplate = go->GetGOInfo();
                if (!goTemplate)
                    continue;

                uint32 portalSpellId = 0;
                if (goTemplate->type == GAMEOBJECT_TYPE_SPELLCASTER)
                    portalSpellId = goTemplate->spellCaster.spell;
                else if (goTemplate->type == GAMEOBJECT_TYPE_GOOBER)
                    portalSpellId = goTemplate->goober.spell;

                if (portalSpellId == 0)
                    continue;

                // Check if this portal leads to our destination map
                auto destOpt = portalDb.GetPortalDestination(portalSpellId);
                if (destOpt && destOpt->GetMapId() == leg.endMapId)
                {
                    resolvedPortalPos.Relocate(go->GetPositionX(), go->GetPositionY(), go->GetPositionZ());
                    resolvedDestPos.Relocate(destOpt->GetPositionX(), destOpt->GetPositionY(), destOpt->GetPositionZ());
                    portalName = goTemplate->name;
                    foundPortal = true;

                    TC_LOG_DEBUG("module.playerbot.travel",
                        "HandlePortal: Bot {} found portal '{}' dynamically at ({:.1f}, {:.1f}, {:.1f})",
                        m_bot->GetName(), portalName,
                        resolvedPortalPos.GetPositionX(), resolvedPortalPos.GetPositionY(), resolvedPortalPos.GetPositionZ());
                    break;
                }
            }
        }

        // === STEP 3: Use connection data as last resort ===
        if (!foundPortal && leg.connection)
        {
            TC_LOG_DEBUG("module.playerbot.travel",
                "HandlePortal: Bot {} using fallback connection data for portal '{}' at ({:.1f}, {:.1f}, {:.1f})",
                m_bot->GetName(), leg.connection->name,
                leg.startPosition.GetPositionX(), leg.startPosition.GetPositionY(), leg.startPosition.GetPositionZ());
            // Keep default positions from leg
        }
        else if (!foundPortal)
        {
            TC_LOG_ERROR("module.playerbot.travel",
                "HandlePortal: Bot {} has no portal source - no database entry, no dynamic portal, no connection data",
                m_bot->GetName());
            leg.currentState = TravelState::FAILED;
            return;
        }

        // Update leg with resolved positions
        leg.startPosition = resolvedPortalPos;
        leg.endPosition = resolvedDestPos;
        leg.description = portalName;

        // Re-validate faction if connection exists
        if (leg.connection && !CanUseConnection(leg.connection))
        {
            TC_LOG_WARN("module.playerbot.travel",
                "HandlePortal: Bot {} can no longer use portal {} (faction/level changed)",
                m_bot->GetName(), portalName);
            leg.currentState = TravelState::FAILED;
            return;
        }
    }

    // ========================================================================
    // STATE MACHINE
    // ========================================================================

    switch (leg.currentState)
    {
        case TravelState::IDLE:
        case TravelState::WALKING_TO_TRANSPORT:
        {
            float distToPortal = m_bot->GetDistance(leg.startPosition);

            if (distToPortal > 10.0f)
            {
                // Walk to portal
                if (leg.currentState != TravelState::WALKING_TO_TRANSPORT)
                {
                    TC_LOG_DEBUG("module.playerbot.travel",
                        "HandlePortal: Bot {} walking to portal '{}' at ({:.1f}, {:.1f}, {:.1f}) - distance {:.1f}",
                        m_bot->GetName(), leg.description,
                        leg.startPosition.GetPositionX(), leg.startPosition.GetPositionY(), leg.startPosition.GetPositionZ(),
                        distToPortal);
                }

                // Use validated pathfinding for walking to portal
                if (BotAI* ai = GetBotAI(m_bot))
                {
                    if (!ai->MoveTo(leg.startPosition, true))
                    {
                        // Fallback to legacy if validation fails
                        m_bot->GetMotionMaster()->MovePoint(0, leg.startPosition);
                    }
                }
                else
                {
                    // Non-bot player - use standard movement
                    m_bot->GetMotionMaster()->MovePoint(0, leg.startPosition);
                }
                leg.currentState = TravelState::WALKING_TO_TRANSPORT;
                leg.stateStartTime = now;
            }
            else
            {
                // At portal - use it
                TC_LOG_INFO("module.playerbot.travel",
                    "HandlePortal: Bot {} using portal '{}' to MAP {}",
                    m_bot->GetName(), leg.description, leg.endMapId);

                // Validate destination is safe (not in void)
                if (leg.endPosition.GetPositionX() == 0.0f &&
                    leg.endPosition.GetPositionY() == 0.0f &&
                    leg.endPosition.GetPositionZ() == 0.0f)
                {
                    TC_LOG_ERROR("module.playerbot.travel",
                        "HandlePortal: Bot {} - portal '{}' has invalid destination (0,0,0)! Cannot teleport.",
                        m_bot->GetName(), leg.description);
                    leg.currentState = TravelState::FAILED;
                    return;
                }

                // Use portal (teleport to destination)
                if (m_bot->TeleportTo(leg.endMapId,
                                      leg.endPosition.GetPositionX(),
                                      leg.endPosition.GetPositionY(),
                                      leg.endPosition.GetPositionZ(),
                                      leg.endPosition.GetOrientation()))
                {
                    leg.currentState = TravelState::USING_PORTAL;
                    leg.stateStartTime = now;
                }
                else
                {
                    TC_LOG_ERROR("module.playerbot.travel",
                        "HandlePortal: Bot {} - TeleportTo failed for portal '{}'",
                        m_bot->GetName(), leg.description);
                    leg.currentState = TravelState::FAILED;
                }
            }
            break;
        }

        case TravelState::USING_PORTAL:
        {
            // Verify we arrived at destination
            if (m_bot->GetMapId() == leg.endMapId && IsNearPosition(leg.endPosition, 50.0f))
            {
                TC_LOG_DEBUG("module.playerbot.travel",
                    "HandlePortal: Bot {} successfully arrived via portal '{}'",
                    m_bot->GetName(), leg.description);
                leg.currentState = TravelState::COMPLETED;
            }
            else
            {
                // Check timeout
                uint32 elapsed = now - leg.stateStartTime;
                if (elapsed > 30000) // 30 second timeout for portal
                {
                    // May have arrived but at different position - check if on correct map
                    if (m_bot->GetMapId() == leg.endMapId)
                    {
                        TC_LOG_DEBUG("module.playerbot.travel",
                            "HandlePortal: Bot {} arrived on map {} (position differs from expected), marking complete",
                            m_bot->GetName(), leg.endMapId);
                        leg.currentState = TravelState::COMPLETED;
                    }
                    else
                    {
                        TC_LOG_WARN("module.playerbot.travel",
                            "HandlePortal: Bot {} timed out waiting for portal teleport to complete",
                            m_bot->GetName());
                        leg.currentState = TravelState::FAILED;
                    }
                }
            }
            break;
        }

        default:
            TC_LOG_WARN("module.playerbot.travel",
                "HandlePortal: Bot {} in unexpected state {}",
                m_bot->GetName(), static_cast<int>(leg.currentState));
            break;
    }
}

void TravelRouteManager::HandleHearthstone(TravelLeg& leg)
{
    static constexpr uint32 HEARTHSTONE_SPELL_ID = 8690;

    switch (leg.currentState)
    {
        case TravelState::IDLE:
        case TravelState::WALKING_TO_TRANSPORT:
        {
            // Check cooldown
            SpellHistory* spellHistory = m_bot->GetSpellHistory();
            if (spellHistory && spellHistory->HasCooldown(HEARTHSTONE_SPELL_ID))
            {
                leg.currentState = TravelState::FAILED;
                return;
            }

            // Cast hearthstone using Player::CastSpell which handles memory management
            // Note: Using CastSpell() instead of manual Spell allocation to avoid memory leaks
            // The spell system will handle the Spell object lifecycle internally
            SpellCastResult result = m_bot->CastSpell(m_bot, HEARTHSTONE_SPELL_ID, false);
            if (result != SPELL_CAST_OK)
            {
                TC_LOG_ERROR("module.playerbot.travel",
                    "TravelRouteManager::HandleHearthstone: Bot {} failed to cast Hearthstone (spell {}), result: {}",
                    m_bot->GetName(), HEARTHSTONE_SPELL_ID, static_cast<uint32>(result));
                leg.currentState = TravelState::FAILED;
                return;
            }

            leg.currentState = TravelState::CASTING_HEARTHSTONE;
            leg.stateStartTime = static_cast<uint32>(GameTime::GetGameTimeMS());
            break;
        }

        case TravelState::CASTING_HEARTHSTONE:
        {
            // Check if cast completed (bot is now at homebind)
            if (m_bot->GetMapId() == leg.endMapId && IsNearPosition(leg.endPosition, 50.0f))
            {
                leg.currentState = TravelState::COMPLETED;
            }
            else
            {
                // Check timeout
                uint32 elapsed = static_cast<uint32>(GameTime::GetGameTimeMS()) - leg.stateStartTime;
                if (elapsed > 15000) // 15 second timeout
                {
                    leg.currentState = TravelState::FAILED;
                }
            }
            break;
        }

        default:
            break;
    }
}

void TravelRouteManager::HandleTaxiFlight(TravelLeg& leg)
{
    // ========================================================================
    // ENTERPRISE-GRADE TAXI FLIGHT HANDLING
    // ========================================================================
    // Implements proper taxi flight execution:
    // 1. Walk to nearest flight master
    // 2. Activate taxi flight via FlightMasterManager
    // 3. Monitor flight progress via IsInFlight()
    // 4. Detect arrival when flight completes
    // ========================================================================

    if (!m_bot)
    {
        leg.currentState = TravelState::FAILED;
        return;
    }

    uint32 now = static_cast<uint32>(GameTime::GetGameTimeMS());

    switch (leg.currentState)
    {
        case TravelState::IDLE:
        case TravelState::WALKING_TO_TRANSPORT:
        {
            // Check if already at destination
            if (IsNearPosition(leg.endPosition, 50.0f))
            {
                TC_LOG_DEBUG("module.playerbot.travel",
                    "HandleTaxiFlight: Bot {} already at destination, skipping taxi",
                    m_bot->GetName());
                leg.currentState = TravelState::COMPLETED;
                break;
            }

            // Check if already in flight (perhaps from a previous partial attempt)
            if (m_bot->IsInFlight())
            {
                TC_LOG_DEBUG("module.playerbot.travel",
                    "HandleTaxiFlight: Bot {} already in flight, monitoring...",
                    m_bot->GetName());
                leg.currentState = TravelState::TAXI_FLIGHT;
                leg.stateStartTime = now;
                break;
            }

            // Find nearest flight master
            auto flightMasterOpt = FlightMasterManager::FindNearestFlightMaster(m_bot);
            if (!flightMasterOpt.has_value())
            {
                TC_LOG_WARN("module.playerbot.travel",
                    "HandleTaxiFlight: No flight master found near bot {}",
                    m_bot->GetName());
                leg.currentState = TravelState::FAILED;
                break;
            }

            FlightMasterLocation const& fm = *flightMasterOpt;

            // If too far from flight master, walk to it
            if (fm.distanceFromPlayer > 15.0f)
            {
                TC_LOG_DEBUG("module.playerbot.travel",
                    "HandleTaxiFlight: Bot {} walking to flight master {} ({:.1f} yards away)",
                    m_bot->GetName(), fm.name, fm.distanceFromPlayer);

                // Use validated pathfinding for walking to flight master
                Position fmPos(fm.position.GetPositionX(), fm.position.GetPositionY(), fm.position.GetPositionZ(), 0.0f);
                if (BotAI* ai = GetBotAI(m_bot))
                {
                    if (!ai->MoveTo(fmPos, true))
                    {
                        // Fallback to legacy if validation fails
                        m_bot->GetMotionMaster()->MovePoint(0, fm.position.GetPositionX(), fm.position.GetPositionY(), fm.position.GetPositionZ());
                    }
                }
                else
                {
                    // Non-bot player - use standard movement
                    m_bot->GetMotionMaster()->MovePoint(0, fm.position.GetPositionX(), fm.position.GetPositionY(), fm.position.GetPositionZ());
                }
                leg.currentState = TravelState::WALKING_TO_TRANSPORT;
                leg.stateStartTime = now;
                break;
            }

            // At flight master - activate the taxi flight
            TC_LOG_INFO("module.playerbot.travel",
                "HandleTaxiFlight: Bot {} at flight master {}, activating taxi to node {}",
                m_bot->GetName(), fm.name, leg.taxiEndNode);

            FlightMasterManager flightMgr(m_bot);
            FlightResult result = flightMgr.FlyToTaxiNode(
                m_bot,
                leg.taxiEndNode,
                FlightPathStrategy::SHORTEST_DISTANCE);

            if (result == FlightResult::SUCCESS)
            {
                TC_LOG_INFO("module.playerbot.travel",
                    "HandleTaxiFlight: Bot {} taxi flight activated successfully",
                    m_bot->GetName());
                leg.currentState = TravelState::TAXI_FLIGHT;
                leg.stateStartTime = now;
            }
            else
            {
                TC_LOG_WARN("module.playerbot.travel",
                    "HandleTaxiFlight: Bot {} failed to activate taxi: {}",
                    m_bot->GetName(), FlightMasterManager::GetResultString(result));

                // If flight fails due to node not discovered, AUTO-DISCOVER it and retry
                if (result == FlightResult::NODE_UNKNOWN)
                {
                    TC_LOG_INFO("module.playerbot.travel",
                        "HandleTaxiFlight: Bot {} auto-discovering taxi node {} and retrying",
                        m_bot->GetName(), leg.taxiEndNode);

                    // Force-learn the destination taxi node
                    if (m_bot->m_taxi.SetTaximaskNode(leg.taxiEndNode))
                    {
                        TC_LOG_INFO("module.playerbot.travel",
                            "HandleTaxiFlight: Bot {} learned taxi node {}, will retry next tick",
                            m_bot->GetName(), leg.taxiEndNode);
                        // Stay in current state - retry will happen next tick
                    }
                    else
                    {
                        // Failed to learn taxi node - this shouldn't happen normally
                        // Failing the route is safer than walking to a potentially unreachable destination
                        TC_LOG_WARN("module.playerbot.travel",
                            "HandleTaxiFlight: Bot {} failed to learn taxi node {} - route FAILED",
                            m_bot->GetName(), leg.taxiEndNode);
                        leg.currentState = TravelState::FAILED;
                    }
                }
                else if (result == FlightResult::PATH_NOT_FOUND)
                {
                    // PATH_NOT_FOUND means no taxi route exists between these nodes
                    // This typically happens for cross-continent routes where flight paths don't connect
                    // Falling back to walking doesn't make sense here - FAIL the route
                    // The QuestStrategy travel failure cooldown system will handle retries
                    TC_LOG_WARN("module.playerbot.travel",
                        "HandleTaxiFlight: No flight path exists from node {} to node {} for bot {} - "
                        "route FAILED (cross-continent flight not possible via taxi)",
                        leg.taxiStartNode, leg.taxiEndNode, m_bot->GetName());
                    leg.currentState = TravelState::FAILED;
                }
                else
                {
                    leg.currentState = TravelState::FAILED;
                }
            }
            break;
        }

        case TravelState::TAXI_FLIGHT:
        {
            // Monitor flight progress
            if (!m_bot->IsInFlight())
            {
                // Flight ended - check if we arrived
                if (IsNearPosition(leg.endPosition, 100.0f))
                {
                    TC_LOG_INFO("module.playerbot.travel",
                        "HandleTaxiFlight: Bot {} arrived at destination via taxi",
                        m_bot->GetName());
                    leg.currentState = TravelState::COMPLETED;
                }
                else
                {
                    // Flight ended but not at destination - might have been interrupted
                    // or took a different route. Check if we made progress.
                    float distToEnd = m_bot->GetExactDist(&leg.endPosition);

                    TC_LOG_DEBUG("module.playerbot.travel",
                        "HandleTaxiFlight: Bot {} flight ended, {:.1f} yards from destination",
                        m_bot->GetName(), distToEnd);

                    if (distToEnd < 200.0f)
                    {
                        // Use validated pathfinding for walking after flight
                        if (BotAI* ai = GetBotAI(m_bot))
                        {
                            if (!ai->MoveTo(leg.endPosition, true))
                            {
                                // Fallback to legacy if validation fails
                                m_bot->GetMotionMaster()->MovePoint(0, leg.endPosition);
                            }
                        }
                        else
                        {
                            // Non-bot player - use standard movement
                            m_bot->GetMotionMaster()->MovePoint(0, leg.endPosition);
                        }
                        leg.currentState = TravelState::ARRIVING;
                        leg.stateStartTime = now;
                    }
                    else
                    {
                        // Need another taxi or we failed
                        TC_LOG_WARN("module.playerbot.travel",
                            "HandleTaxiFlight: Bot {} landed far from destination, retrying...",
                            m_bot->GetName());
                        leg.currentState = TravelState::IDLE;
                        leg.stateStartTime = now;
                    }
                }
            }
            else
            {
                // Still in flight - check for timeout
                uint32 elapsed = now - leg.stateStartTime;
                uint32 maxFlightTime = (leg.estimatedTimeSeconds + 300) * 1000; // Add 5 min buffer

                if (elapsed > maxFlightTime)
                {
                    TC_LOG_ERROR("module.playerbot.travel",
                        "HandleTaxiFlight: Bot {} flight timeout after {}s (expected {}s)",
                        m_bot->GetName(), elapsed / 1000, leg.estimatedTimeSeconds);
                    // Let flight system handle it - don't force interrupt
                }
                else if (elapsed % 60000 < 500) // Log every minute
                {
                    TC_LOG_DEBUG("module.playerbot.travel",
                        "HandleTaxiFlight: Bot {} in flight - {}s elapsed",
                        m_bot->GetName(), elapsed / 1000);
                }
            }
            break;
        }

        case TravelState::ARRIVING:
        {
            // Walking from landing spot to final destination
            if (IsNearPosition(leg.endPosition, 15.0f))
            {
                TC_LOG_DEBUG("module.playerbot.travel",
                    "HandleTaxiFlight: Bot {} arrived at final destination",
                    m_bot->GetName());
                leg.currentState = TravelState::COMPLETED;
            }
            else
            {
                // Check walking timeout
                uint32 elapsed = now - leg.stateStartTime;
                if (elapsed > 120000) // 2 minute walking timeout
                {
                    TC_LOG_WARN("module.playerbot.travel",
                        "HandleTaxiFlight: Bot {} walking timeout, marking as completed",
                        m_bot->GetName());
                    leg.currentState = TravelState::COMPLETED;
                }
            }
            break;
        }

        default:
            TC_LOG_WARN("module.playerbot.travel",
                "HandleTaxiFlight: Bot {} in unexpected state {}",
                m_bot->GetName(), static_cast<int>(leg.currentState));
            break;
    }
}

// ============================================================================
// HELPER METHODS
// ============================================================================

bool TravelRouteManager::CanReachMap(uint32 fromMapId, uint32 toMapId) const
{
    if (fromMapId == toMapId)
        return true;

    TC_LOG_DEBUG("module.playerbot.travel", "CanReachMap: Checking route from MAP {} to MAP {} for {}",
                 fromMapId, toMapId, m_bot ? m_bot->GetName() : "NULL");

    // BFS to check reachability
    std::queue<uint32> bfsQueue;
    std::unordered_set<uint32> visited;
    uint32 connectionsChecked = 0;
    uint32 connectionsRejected = 0;

    bfsQueue.push(fromMapId);
    visited.insert(fromMapId);

    while (!bfsQueue.empty())
    {
        uint32 currentMap = bfsQueue.front();
        bfsQueue.pop();

        auto it = s_mapConnectivityGraph.find(currentMap);
        if (it == s_mapConnectivityGraph.end())
        {
            TC_LOG_DEBUG("module.playerbot.travel", "CanReachMap: No connections from MAP {}", currentMap);
            continue;
        }

        for (auto const& [neighborMap, conn] : it->second)
        {
            connectionsChecked++;

            // CRITICAL FIX: Check if bot can use this connection BEFORE accepting it
            if (!CanUseConnection(conn))
            {
                connectionsRejected++;
                TC_LOG_DEBUG("module.playerbot.travel", "CanReachMap: Rejected '{}' (MAP {} -> MAP {}): bot cannot use (faction/level)",
                             conn->name, currentMap, neighborMap);
                continue;
            }

            if (neighborMap == toMapId)
            {
                TC_LOG_DEBUG("module.playerbot.travel", "CanReachMap: Found route via '{}' - {} connections checked, {} rejected",
                             conn->name, connectionsChecked, connectionsRejected);
                return true;
            }

            if (visited.find(neighborMap) == visited.end())
            {
                visited.insert(neighborMap);
                bfsQueue.push(neighborMap);
                TC_LOG_DEBUG("module.playerbot.travel", "CanReachMap: Added MAP {} via '{}'", neighborMap, conn->name);
            }
        }
    }

    TC_LOG_WARN("module.playerbot.travel", "CanReachMap: No route from MAP {} to MAP {} for {} - {} connections checked, {} rejected",
                fromMapId, toMapId, m_bot ? m_bot->GetName() : "NULL", connectionsChecked, connectionsRejected);
    return false;
}

uint32 TravelRouteManager::GetEstimatedTravelTime(uint32 fromMapId, Position const& /*fromPos*/,
                                                   uint32 toMapId, Position const& /*toPos*/) const
{
    if (fromMapId == toMapId)
        return 0;

    // Simplified estimate - count hops and multiply by average transport time
    // Full implementation would plan actual route

    if (!CanReachMap(fromMapId, toMapId))
        return UINT32_MAX;

    // Rough estimate: 5 minutes per map hop
    return 300;
}

std::vector<TransportConnection const*> TravelRouteManager::GetConnectionsFromMap(uint32 mapId) const
{
    std::vector<TransportConnection const*> result;

    auto it = s_connectionsByDepartureMap.find(mapId);
    if (it != s_connectionsByDepartureMap.end())
    {
        for (size_t idx : it->second)
        {
            if (idx < s_transportConnections.size())
                result.push_back(&s_transportConnections[idx]);
        }
    }

    return result;
}

std::vector<TransportConnection const*> TravelRouteManager::GetConnectionsToMap(uint32 mapId) const
{
    std::vector<TransportConnection const*> result;

    auto it = s_connectionsByArrivalMap.find(mapId);
    if (it != s_connectionsByArrivalMap.end())
    {
        for (size_t idx : it->second)
        {
            if (idx < s_transportConnections.size())
                result.push_back(&s_transportConnections[idx]);
        }
    }

    return result;
}

TransportConnection const* TravelRouteManager::FindDirectConnection(uint32 fromMapId, uint32 toMapId) const
{
    auto it = s_connectionsByDepartureMap.find(fromMapId);
    if (it == s_connectionsByDepartureMap.end())
        return nullptr;

    for (size_t idx : it->second)
    {
        if (idx < s_transportConnections.size() && s_transportConnections[idx].arrivalMapId == toMapId)
        {
            if (CanUseConnection(&s_transportConnections[idx]))
                return &s_transportConnections[idx];
        }
    }

    return nullptr;
}

bool TravelRouteManager::IsNearPosition(Position const& pos, float range) const
{
    if (!m_bot)
        return false;

    return m_bot->GetExactDist(pos) < range;
}

bool TravelRouteManager::IsOnTransport() const
{
    if (!m_bot)
        return false;

    return m_bot->GetTransport() != nullptr;
}

bool TravelRouteManager::CanUseConnection(TransportConnection const* connection) const
{
    if (!connection || !m_bot)
        return false;

    // Faction check
    bool isAlliance = (m_bot->GetTeam() == ALLIANCE);
    if (connection->allianceOnly && !isAlliance)
        return false;
    if (connection->hordeOnly && isAlliance)
        return false;

    // Level check
    if (connection->requiresLevel && m_bot->GetLevel() < connection->minLevel)
        return false;

    return true;
}

TravelState TravelRouteManager::GetCurrentState() const
{
    if (!m_activeRoute)
        return TravelState::IDLE;

    TravelLeg const* leg = m_activeRoute->GetCurrentLeg();
    if (leg)
        return leg->currentState;

    return m_activeRoute->overallState;
}

std::string TravelRouteManager::GetStatusString() const
{
    if (!m_activeRoute)
        return "No active route";

    if (!m_activeRoute->IsActive())
    {
        if (m_activeRoute->IsComplete())
            return "Route completed";
        if (m_activeRoute->IsFailed())
            return "Route failed";
        return "Route idle";
    }

    TravelLeg const* leg = m_activeRoute->GetCurrentLeg();
    if (!leg)
        return "No current leg";

    std::string status = "Leg " + std::to_string(leg->legIndex + 1) + "/" +
                         std::to_string(m_activeRoute->totalLegs) + ": ";

    switch (leg->currentState)
    {
        case TravelState::WALKING_TO_TRANSPORT: status += "Walking to transport"; break;
        case TravelState::WAITING_FOR_TRANSPORT: status += "Waiting for transport"; break;
        case TravelState::ON_TRANSPORT: status += "On transport"; break;
        case TravelState::TAXI_FLIGHT: status += "Flying"; break;
        case TravelState::CASTING_HEARTHSTONE: status += "Hearthing"; break;
        case TravelState::USING_PORTAL: status += "Using portal"; break;
        default: status += "Unknown state"; break;
    }

    return status;
}

} // namespace Playerbot
