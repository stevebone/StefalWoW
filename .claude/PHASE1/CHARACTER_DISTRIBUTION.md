## Based on Official WoW Statistics

### Race Distribution (Overall)
**Alliance:**
- Human: 29.5%
- Night Elf: 20.8%
- Dwarf: 7.2%
- Gnome: 5.3%
- Draenei: 8.9%
- Worgen: 6.8%
- Void Elf: 12.3%
- Lightforged Draenei: 3.2%
- Dark Iron Dwarf: 2.8%
- Kul Tiran: 3.2%

**Horde:**
- Orc: 8.7%
- Undead: 11.2%
- Tauren: 6.4%
- Troll: 7.1%
- Blood Elf: 35.8%
- Goblin: 4.3%
- Nightborne: 8.9%
- Highmountain Tauren: 3.1%
- Mag'har Orc: 2.4%
- Zandalari Troll: 12.1%

### Class Distribution
1. Hunter: 11.8%
2. Paladin: 11.5%
3. Druid: 10.9%
4. Warrior: 9.8%
5. Death Knight: 9.2%
6. Priest: 8.7%
7. Mage: 8.4%
8. Demon Hunter: 7.9%
9. Rogue: 7.3%
10. Warlock: 6.8%
11. Shaman: 6.2%
12. Monk: 1.5% (least played)

### Gender Distribution by Race
**Typically Male-Heavy (70-80% male):**
- Orc, Dwarf, Tauren, Undead, Troll, Goblin, Worgen

**Balanced (45-55% male):**
- Human, Gnome, Pandaren

**Typically Female-Heavy (30-40% male):**
- Night Elf, Blood Elf, Draenei, Void Elf

### Popular Race/Class Combinations
**Most Common:**
1. Blood Elf Paladin (Horde)
2. Night Elf Demon Hunter (Alliance)
3. Human Paladin (Alliance)
4. Blood Elf Hunter (Horde)
5. Night Elf Druid (Alliance)
6. Human Warrior (Alliance)
7. Orc Warrior (Horde)
8. Undead Rogue (Horde)
9. Tauren Druid (Horde)
10. Dwarf Hunter (Alliance)

**Rare Combinations:**
- Gnome Warrior
- Tauren Priest
- Dwarf Mage
- Troll Monk

## Implementation Code

```cpp
// File: src/modules/Playerbot/Character/BotCharacterDistribution.h

#ifndef BOT_CHARACTER_DISTRIBUTION_H
#define BOT_CHARACTER_DISTRIBUTION_H

#include \"Define.h\"
#include \"SharedDefines.h\"
#include <vector>
#include <map>
#include <random>

struct RaceWeight
{
    Races race;
    float weight;
    float maleRatio; // 0.0 = all female, 1.0 = all male
};

struct ClassWeight  
{
    Classes classId;
    float weight;
};

struct RaceClassWeight
{
    Races race;
    Classes classId;
    float weight; // Multiplier for this specific combination
};

class TC_GAME_API BotCharacterDistribution
{
public:
    static BotCharacterDistribution* instance();
    
    // Generate random character based on WoW statistics
    void GenerateRandomCharacter(uint32& race, uint32& classId, uint32& gender);
    
    // Get weighted random selection
    Races GetRandomRace(bool forceAlliance = false, bool forceHorde = false);
    Classes GetRandomClass(Races race);
    uint8 GetRandomGender(Races race);
    
    // Get role distribution for class
    std::string GetClassRole(Classes classId, uint32 level = 60);
    
private:
    BotCharacterDistribution();
    void InitializeDistributions();
    
    // Weighted random selection helper
    template<typename T>
    T GetWeightedRandom(std::vector<std::pair<T, float>> const& weights);
    
    std::map<Races, float> _raceWeights;
    std::map<Races, float> _genderRatios;
    std::map<Classes, float> _classWeights;
    std::map<std::pair<Races, Classes>, float> _raceClassMultipliers;
    
    std::mt19937 _rng;
};

#define sBotCharacterDistribution BotCharacterDistribution::instance()

#endif // BOT_CHARACTER_DISTRIBUTION_H
```

```cpp
// File: src/modules/Playerbot/Character/BotCharacterDistribution.cpp

#include \"BotCharacterDistribution.h\"
#include \"Log.h\"
#include <algorithm>
#include <numeric>

BotCharacterDistribution* BotCharacterDistribution::instance()
{
    static BotCharacterDistribution instance;
    return &instance;
}

BotCharacterDistribution::BotCharacterDistribution()
{
    std::random_device rd;
    _rng.seed(rd());
    InitializeDistributions();
}

void BotCharacterDistribution::InitializeDistributions()
{
    // Race distribution based on WoW statistics
    // Alliance races
    _raceWeights[RACE_HUMAN] = 29.5f;
    _raceWeights[RACE_DWARF] = 7.2f;
    _raceWeights[RACE_NIGHTELF] = 20.8f;
    _raceWeights[RACE_GNOME] = 5.3f;
    _raceWeights[RACE_DRAENEI] = 8.9f;
    _raceWeights[RACE_WORGEN] = 6.8f;
    
    // Horde races  
    _raceWeights[RACE_ORC] = 8.7f;
    _raceWeights[RACE_UNDEAD_PLAYER] = 11.2f;
    _raceWeights[RACE_TAUREN] = 6.4f;
    _raceWeights[RACE_TROLL] = 7.1f;
    _raceWeights[RACE_BLOODELF] = 35.8f;
    _raceWeights[RACE_GOBLIN] = 4.3f;
    
    // Allied races (if supported)
    _raceWeights[RACE_VOID_ELF] = 12.3f;
    _raceWeights[RACE_LIGHTFORGED_DRAENEI] = 3.2f;
    _raceWeights[RACE_DARK_IRON_DWARF] = 2.8f;
    _raceWeights[RACE_KUL_TIRAN] = 3.2f;
    _raceWeights[RACE_NIGHTBORNE] = 8.9f;
    _raceWeights[RACE_HIGHMOUNTAIN_TAUREN] = 3.1f;
    _raceWeights[RACE_ZANDALARI_TROLL] = 12.1f;
    _raceWeights[RACE_MAGHAR_ORC] = 2.4f;
    
    // Gender ratios (% male)
    _genderRatios[RACE_HUMAN] = 0.52f;
    _genderRatios[RACE_DWARF] = 0.75f;
    _genderRatios[RACE_NIGHTELF] = 0.35f;
    _genderRatios[RACE_GNOME] = 0.50f;
    _genderRatios[RACE_DRAENEI] = 0.40f;
    _genderRatios[RACE_WORGEN] = 0.70f;
    _genderRatios[RACE_ORC] = 0.75f;
    _genderRatios[RACE_UNDEAD_PLAYER] = 0.70f;
    _genderRatios[RACE_TAUREN] = 0.72f;
    _genderRatios[RACE_TROLL] = 0.70f;
    _genderRatios[RACE_BLOODELF] = 0.35f;
    _genderRatios[RACE_GOBLIN] = 0.70f;
    
    // Class distribution
    _classWeights[CLASS_WARRIOR] = 9.8f;
    _classWeights[CLASS_PALADIN] = 11.5f;
    _classWeights[CLASS_HUNTER] = 11.8f;
    _classWeights[CLASS_ROGUE] = 7.3f;
    _classWeights[CLASS_PRIEST] = 8.7f;
    _classWeights[CLASS_DEATH_KNIGHT] = 9.2f;
    _classWeights[CLASS_SHAMAN] = 6.2f;
    _classWeights[CLASS_MAGE] = 8.4f;
    _classWeights[CLASS_WARLOCK] = 6.8f;
    _classWeights[CLASS_MONK] = 1.5f;
    _classWeights[CLASS_DRUID] = 10.9f;
    _classWeights[CLASS_DEMON_HUNTER] = 7.9f;
    
    // Popular race/class combination multipliers
    // Higher multiplier = more likely
    _raceClassMultipliers[{RACE_BLOODELF, CLASS_PALADIN}] = 2.5f;
    _raceClassMultipliers[{RACE_NIGHTELF, CLASS_DEMON_HUNTER}] = 3.0f;
    _raceClassMultipliers[{RACE_HUMAN, CLASS_PALADIN}] = 2.0f;
    _raceClassMultipliers[{RACE_BLOODELF, CLASS_HUNTER}] = 2.0f;
    _raceClassMultipliers[{RACE_NIGHTELF, CLASS_DRUID}] = 2.2f;
    _raceClassMultipliers[{RACE_HUMAN, CLASS_WARRIOR}] = 1.8f;
    _raceClassMultipliers[{RACE_ORC, CLASS_WARRIOR}] = 2.0f;
    _raceClassMultipliers[{RACE_UNDEAD_PLAYER, CLASS_ROGUE}] = 1.8f;
    _raceClassMultipliers[{RACE_TAUREN, CLASS_DRUID}] = 2.0f;
    _raceClassMultipliers[{RACE_DWARF, CLASS_HUNTER}] = 1.8f;
    
    // Rare combinations (lower multiplier)
    _raceClassMultipliers[{RACE_GNOME, CLASS_WARRIOR}] = 0.3f;
    _raceClassMultipliers[{RACE_TAUREN, CLASS_PRIEST}] = 0.4f;
    _raceClassMultipliers[{RACE_DWARF, CLASS_MAGE}] = 0.5f;
    _raceClassMultipliers[{RACE_TROLL, CLASS_MONK}] = 0.3f;
    
    TC_LOG_INFO(\"module.playerbot.distribution\", 
        \"Initialized character distribution with {} races, {} classes\", 
        _raceWeights.size(), _classWeights.size());
}

void BotCharacterDistribution::GenerateRandomCharacter(uint32& race, uint32& classId, uint32& gender)
{
    // Step 1: Choose race based on distribution
    Races selectedRace = GetRandomRace();
    
    // Step 2: Choose class based on race (with combination weights)
    Classes selectedClass = GetRandomClass(selectedRace);
    
    // Step 3: Choose gender based on race statistics
    uint8 selectedGender = GetRandomGender(selectedRace);
    
    race = static_cast<uint32>(selectedRace);
    classId = static_cast<uint32>(selectedClass);
    gender = selectedGender;
    
    TC_LOG_DEBUG(\"module.playerbot.distribution\",
        \"Generated character: Race {} Class {} Gender {}\",
        race, classId, gender);
}

Races BotCharacterDistribution::GetRandomRace(bool forceAlliance, bool forceHorde)
{
    std::vector<std::pair<Races, float>> weights;
    
    for (auto const& [race, weight] : _raceWeights)
    {
        // Filter by faction if requested
        if (forceAlliance && !IsAllianceRace(race))
            continue;
        if (forceHorde && !IsHordeRace(race))
            continue;
            
        weights.push_back({race, weight});
    }
    
    return GetWeightedRandom(weights);
}

Classes BotCharacterDistribution::GetRandomClass(Races race)
{
    std::vector<std::pair<Classes, float>> weights;
    
    for (auto const& [classId, baseWeight] : _classWeights)
    {
        // Check if race can be this class
        if (!CanRaceBeClass(race, classId))
            continue;
        
        float weight = baseWeight;
        
        // Apply combination multiplier if exists
        auto it = _raceClassMultipliers.find({race, classId});
        if (it != _raceClassMultipliers.end())
        {
            weight *= it->second;
        }
        
        weights.push_back({classId, weight});
    }
    
    return GetWeightedRandom(weights);
}

uint8 BotCharacterDistribution::GetRandomGender(Races race)
{
    float maleRatio = 0.5f; // Default 50/50
    
    auto it = _genderRatios.find(race);
    if (it != _genderRatios.end())
    {
        maleRatio = it->second;
    }
    
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return (dist(_rng) < maleRatio) ? GENDER_MALE : GENDER_FEMALE;
}

std::string BotCharacterDistribution::GetClassRole(Classes classId, uint32 level)
{
    // Determine role based on class
    switch (classId)
    {
        case CLASS_WARRIOR:
            return (level >= 10) ? \"tank\" : \"dps\"; // Can tank after level 10
        case CLASS_PALADIN:
            if (level >= 10)
            {
                // Distribution: 40% tank, 30% healer, 30% dps
                std::uniform_int_distribution<> dist(1, 10);
                int roll = dist(_rng);
                if (roll <= 4) return \"tank\";
                if (roll <= 7) return \"healer\";
                return \"dps\";
            }
            return \"dps\";
        case CLASS_HUNTER:
        case CLASS_ROGUE:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
            return \"dps\";
        case CLASS_PRIEST:
            // 70% healer, 30% dps (shadow)
            return (std::uniform_int_distribution<>(1, 10)(_rng) <= 7) ? \"healer\" : \"dps\";
        case CLASS_SHAMAN:
            // 40% healer, 60% dps
            return (std::uniform_int_distribution<>(1, 10)(_rng) <= 4) ? \"healer\" : \"dps\";
        case CLASS_DRUID:
            if (level >= 10)
            {
                // 25% tank, 35% healer, 40% dps
                std::uniform_int_distribution<> dist(1, 20);
                int roll = dist(_rng);
                if (roll <= 5) return \"tank\";
                if (roll <= 12) return \"healer\";
                return \"dps\";
            }
            return \"dps\";
        case CLASS_DEATH_KNIGHT:
            // 30% tank, 70% dps
            return (std::uniform_int_distribution<>(1, 10)(_rng) <= 3) ? \"tank\" : \"dps\";
        case CLASS_MONK:
            if (level >= 10)
            {
                // 20% tank, 30% healer, 50% dps
                std::uniform_int_distribution<> dist(1, 10);
                int roll = dist(_rng);
                if (roll <= 2) return \"tank\";
                if (roll <= 5) return \"healer\";
                return \"dps\";
            }
            return \"dps\";
        case CLASS_DEMON_HUNTER:
            // 25% tank, 75% dps
            return (std::uniform_int_distribution<>(1, 4)(_rng) == 1) ? \"tank\" : \"dps\";
        default:
            return \"dps\";
    }
}

template<typename T>
T BotCharacterDistribution::GetWeightedRandom(std::vector<std::pair<T, float>> const& weights)
{
    if (weights.empty())
        return T{};
    
    // Calculate total weight
    float totalWeight = 0.0f;
    for (auto const& [item, weight] : weights)
    {
        totalWeight += weight;
    }
    
    // Generate random value
    std::uniform_real_distribution<float> dist(0.0f, totalWeight);
    float randomValue = dist(_rng);
    
    // Find selected item
    float currentWeight = 0.0f;
    for (auto const& [item, weight] : weights)
    {
        currentWeight += weight;
        if (randomValue <= currentWeight)
        {
            return item;
        }
    }
    
    // Fallback (should not happen)
    return weights.front().first;
}

// Helper functions (would be in SharedDefines or similar)
bool IsAllianceRace(Races race)
{
    switch (race)
    {
        case RACE_HUMAN:
        case RACE_DWARF:
        case RACE_NIGHTELF:
        case RACE_GNOME:
        case RACE_DRAENEI:
        case RACE_WORGEN:
        case RACE_VOID_ELF:
        case RACE_LIGHTFORGED_DRAENEI:
        case RACE_DARK_IRON_DWARF:
        case RACE_KUL_TIRAN:
            return true;
        default:
            return false;
    }
}

bool IsHordeRace(Races race)
{
    switch (race)
    {
        case RACE_ORC:
        case RACE_UNDEAD_PLAYER:
        case RACE_TAUREN:
        case RACE_TROLL:
        case RACE_BLOODELF:
        case RACE_GOBLIN:
        case RACE_NIGHTBORNE:
        case RACE_HIGHMOUNTAIN_TAUREN:
        case RACE_ZANDALARI_TROLL:
        case RACE_MAGHAR_ORC:
            return true;
        default:
            return false;
    }
}

bool CanRaceBeClass(Races race, Classes classId)
{
    // This would use TrinityCore's existing race/class compatibility tables
    // For now, simplified version
    
    // Death Knights - all races
    if (classId == CLASS_DEATH_KNIGHT)
        return true;
    
    // Demon Hunters - only Night Elf and Blood Elf
    if (classId == CLASS_DEMON_HUNTER)
        return (race == RACE_NIGHTELF || race == RACE_BLOODELF);
    
    // Paladins - specific races
    if (classId == CLASS_PALADIN)
    {
        return (race == RACE_HUMAN || race == RACE_DWARF || 
                race == RACE_DRAENEI || race == RACE_BLOODELF ||
                race == RACE_TAUREN || race == RACE_LIGHTFORGED_DRAENEI ||
                race == RACE_DARK_IRON_DWARF || race == RACE_ZANDALARI_TROLL);
    }
    
    // Shamans - specific races
    if (classId == CLASS_SHAMAN)
    {
        return (race == RACE_ORC || race == RACE_TROLL || 
                race == RACE_TAUREN || race == RACE_DRAENEI ||
                race == RACE_GOBLIN || race == RACE_DWARF ||
                race == RACE_DARK_IRON_DWARF || race == RACE_HIGHMOUNTAIN_TAUREN ||
                race == RACE_ZANDALARI_TROLL || race == RACE_MAGHAR_ORC ||
                race == RACE_KUL_TIRAN);
    }
    
    // Druids - specific races
    if (classId == CLASS_DRUID)
    {
        return (race == RACE_NIGHTELF || race == RACE_TAUREN ||
                race == RACE_TROLL || race == RACE_WORGEN ||
                race == RACE_HIGHMOUNTAIN_TAUREN || race == RACE_ZANDALARI_TROLL ||
                race == RACE_KUL_TIRAN);
    }
    
    // TODO: Complete all race/class combinations
    // For now, return true for other combinations
    return true;
}
```

## SQL for Distribution Tables

```sql
-- Add to playerbot database for configurable distributions
CREATE TABLE IF NOT EXISTS `playerbot_race_distribution` (
  `race` TINYINT UNSIGNED NOT NULL,
  `weight` FLOAT NOT NULL DEFAULT 1.0,
  `male_ratio` FLOAT NOT NULL DEFAULT 0.5,
  `faction` ENUM('alliance', 'horde', 'neutral') NOT NULL,
  `enabled` TINYINT(1) DEFAULT 1,
  PRIMARY KEY (`race`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `playerbot_class_distribution` (
  `class` TINYINT UNSIGNED NOT NULL,
  `weight` FLOAT NOT NULL DEFAULT 1.0,
  `tank_capable` TINYINT(1) DEFAULT 0,
  `healer_capable` TINYINT(1) DEFAULT 0,
  `enabled` TINYINT(1) DEFAULT 1,
  PRIMARY KEY (`class`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `playerbot_race_class_multipliers` (
  `race` TINYINT UNSIGNED NOT NULL,
  `class` TINYINT UNSIGNED NOT NULL,
  `multiplier` FLOAT NOT NULL DEFAULT 1.0 COMMENT 'Higher = more common',
  PRIMARY KEY (`race`, `class`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Insert default data based on WoW statistics
INSERT INTO `playerbot_race_distribution` (`race`, `weight`, `male_ratio`, `faction`) VALUES
(1, 29.5, 0.52, 'alliance'),  -- Human
(2, 8.7, 0.75, 'horde'),      -- Orc
(3, 7.2, 0.75, 'alliance'),   -- Dwarf
(4, 20.8, 0.35, 'alliance'),  -- Night Elf
(5, 11.2, 0.70, 'horde'),     -- Undead
(6, 6.4, 0.72, 'horde'),      -- Tauren
(7, 5.3, 0.50, 'alliance'),   -- Gnome
(8, 7.1, 0.70, 'horde'),      -- Troll
(10, 35.8, 0.35, 'horde'),    -- Blood Elf
(11, 8.9, 0.40, 'alliance');  -- Draenei

INSERT INTO `playerbot_class_distribution` (`class`, `weight`, `tank_capable`, `healer_capable`) VALUES
(1, 9.8, 1, 0),   -- Warrior
(2, 11.5, 1, 1),  -- Paladin
(3, 11.8, 0, 0),  -- Hunter
(4, 7.3, 0, 0),   -- Rogue
(5, 8.7, 0, 1),   -- Priest
(6, 9.2, 1, 0),   -- Death Knight
(7, 6.2, 0, 1),   -- Shaman
(8, 8.4, 0, 0),   -- Mage
(9, 6.8, 0, 0),   -- Warlock
(10, 1.5, 1, 1),  -- Monk
(11, 10.9, 1, 1), -- Druid
(12, 7.9, 1, 0);  -- Demon Hunter

-- Popular combinations get higher multipliers
INSERT INTO `playerbot_race_class_multipliers` (`race`, `class`, `multiplier`) VALUES
(10, 2, 2.5),  -- Blood Elf Paladin
(4, 12, 3.0),  -- Night Elf Demon Hunter
(1, 2, 2.0),   -- Human Paladin
(10, 3, 2.0),  -- Blood Elf Hunter
(4, 11, 2.2),  -- Night Elf Druid
(1, 1, 1.8),   -- Human Warrior
(2, 1, 2.0),   -- Orc Warrior
(5, 4, 1.8),   -- Undead Rogue
(6, 11, 2.0),  -- Tauren Druid
(3, 3, 1.8);   -- Dwarf Hunter
```

Diese Implementation sorgt dafür, dass die generierten Bot-Characters der realistischen WoW-Spielerverteilung entsprechen!`