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

#pragma once

#include "Define.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

namespace Playerbot
{

// Struktur für Rasse/Klasse Kombinationen
struct RaceClassCombination
{
    uint8 race;
    uint8 classId;
    float percentage;
    bool isPopular;
    ::std::string faction;
};

// Struktur für Geschlechterverteilung
struct GenderDistribution
{
    uint8 race;
    uint8 malePercentage;
    uint8 femalePercentage;
    ::std::string raceName;
};

// Struktur für Klassen-Popularität
struct ClassPopularity
{
    uint8 classId;
    ::std::string className;
    float overallPopularity;
    float pvePopularity;
    float pvpPopularity;
    float mythicPlusPopularity;
    float raidPopularity;
};

/**
 * Bot Character Distribution System
 *
 * Features:
 * - Database-driven realistic race/class distribution based on WoW 12.0 statistics
 * - Gender distribution with race-specific preferences
 * - Class popularity tracking (PvE, PvP, M+, Raid)
 * - Cumulative distribution for O(log n) random selection
 * - Hot-reload capability for dynamic updates
 */
class TC_GAME_API BotCharacterDistribution final
{
public:
    static BotCharacterDistribution* instance();

    // Initialisierung - lädt alle Daten aus der Datenbank
    bool LoadFromDatabase();
    void ReloadDistributions();

    // Getter für Verteilungsdaten
    ::std::pair<uint8, uint8> GetRandomRaceClassByDistribution();
    uint8 GetRandomGenderForRace(uint8 race);
    uint8 GetRandomGenderForRaceClass(uint8 race, uint8 classId);

    // Statistik-Abfragen
    float GetRaceClassPercentage(uint8 race, uint8 classId) const;
    float GetClassPopularity(uint8 classId) const;
    uint8 GetMalePercentageForRace(uint8 race) const;

    // Top-Kombinationen
    ::std::vector<RaceClassCombination> GetTopCombinations(uint32 limit = 25) const;
    ::std::vector<RaceClassCombination> GetPopularCombinations() const;

    // Statistiken
    uint32 GetTotalCombinations() const { return m_raceClassCombinations.size(); }
    uint32 GetPopularCombinationsCount() const;
    bool IsLoaded() const { return m_loaded; }

private:
    BotCharacterDistribution() = default;
    ~BotCharacterDistribution() = default;
    BotCharacterDistribution(BotCharacterDistribution const&) = delete;
    BotCharacterDistribution& operator=(BotCharacterDistribution const&) = delete;

    void LoadRaceClassDistribution();
    void LoadGenderDistribution();
    void LoadClassPopularity();
    void LoadRaceClassGenderOverrides();
    void BuildCumulativeDistribution();

private:
    // Haupt-Datenspeicher
    ::std::vector<RaceClassCombination> m_raceClassCombinations;
    ::std::unordered_map<uint8, GenderDistribution> m_genderDistributions;
    ::std::unordered_map<uint8, ClassPopularity> m_classPopularities;

    // Rasse/Klasse-spezifische Geschlechterverteilung (Overrides)
    ::std::unordered_map<uint32, uint8> m_raceClassGenderOverrides; // key: (race << 8) | class

    // Kumulative Verteilung für effiziente Zufallsauswahl
    ::std::vector<float> m_cumulativeDistribution;
    float m_totalPercentage = 0.0f;

    // Cache für häufige Abfragen
    mutable ::std::unordered_map<uint8, ::std::vector<RaceClassCombination>> m_raceCache;
    mutable ::std::unordered_map<uint8, ::std::vector<RaceClassCombination>> m_classCache;

    // Status
    bool m_loaded = false;
};

} // namespace Playerbot

#define sBotCharacterDistribution Playerbot::BotCharacterDistribution::instance()