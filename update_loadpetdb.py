import re

# Read the file
with open('C:/TrinityBots/TrinityCore/src/modules/Playerbot/Companion/BattlePetManager.cpp', 'r') as f:
    content = f.read()

# Find the LoadPetDatabase function and replace only the SQL section (from function start to before fallback)
# Use regex to find from "void BattlePetManager::LoadPetDatabase()" to "} while (result->NextRow());\n    }"

pattern = r'void BattlePetManager::LoadPetDatabase\(\)\s*\{[^}]*QueryResult result = WorldDatabase\.Query\([^)]+\);[^}]*if \(result\)[^}]*do[^}]*\{[^}]*\} while \(result->NextRow\(\)\);\s*\}'

replacement = '''void BattlePetManager::LoadPetDatabase()
{
    // DESIGN NOTE: Complete battle pet species loading implementation
    // PRIMARY: Load species from TrinityCore's DB2 store (sBattlePetSpeciesStore)
    // FALLBACK: Hardcoded pet data if DB2 store is empty
    //
    // BattlePetSpeciesEntry structure (from DB2Structure.h):
    // - ID: Unique species identifier
    // - Description/SourceText: LocalizedStrings
    // - CreatureID: Links to creature_template entry
    // - SummonSpellID: Spell to summon this pet
    // - IconFileDataID: Icon for UI display
    // - PetTypeEnum: Maps to PetFamily (0=Humanoid, 1=Dragonkin, etc.)
    // - Flags: Species flags (tradeable, capturable, etc.)
    // - SourceTypeEnum: How the pet is obtained
    // - CovenantID: Covenant restriction (if any)

    TC_LOG_INFO("playerbot.battlepet", "BattlePetManager: Loading species from TrinityCore DB2 store...");

    // PRIMARY: Load from TrinityCore's DB2 store
    uint32 db2SpeciesCount = 0;
    for (BattlePetSpeciesEntry const* speciesEntry : sBattlePetSpeciesStore)
    {
        if (!speciesEntry)
            continue;

        // Skip species without valid creature links
        if (speciesEntry->CreatureID <= 0)
            continue;

        BattlePetInfo petInfo;
        petInfo.speciesId = speciesEntry->ID;

        // Try to get name from creature_template
        if (CreatureTemplate const* creatureTemplate = sObjectMgr->GetCreatureTemplate(speciesEntry->CreatureID))
            petInfo.name = creatureTemplate->Name;
        else
            petInfo.name = speciesEntry->Description[LOCALE_enUS];

        // Map DB2 PetTypeEnum to our PetFamily enum
        // DB2 values: 0=Humanoid, 1=Dragonkin, 2=Flying, 3=Undead, 4=Critter,
        //             5=Magic, 6=Elemental, 7=Beast, 8=Aquatic, 9=Mechanical
        switch (speciesEntry->PetTypeEnum)
        {
            case 0: petInfo.family = PetFamily::HUMANOID; break;
            case 1: petInfo.family = PetFamily::DRAGONKIN; break;
            case 2: petInfo.family = PetFamily::FLYING; break;
            case 3: petInfo.family = PetFamily::UNDEAD; break;
            case 4: petInfo.family = PetFamily::CRITTER; break;
            case 5: petInfo.family = PetFamily::MAGIC; break;
            case 6: petInfo.family = PetFamily::ELEMENTAL; break;
            case 7: petInfo.family = PetFamily::BEAST; break;
            case 8: petInfo.family = PetFamily::AQUATIC; break;
            case 9: petInfo.family = PetFamily::MECHANICAL; break;
            default: petInfo.family = PetFamily::BEAST; break;
        }

        petInfo.level = 1;  // Default starting level
        petInfo.xp = 0;

        // Determine quality based on DB2 flags
        // Flag meanings: 0x100=Legendary, 0x80=Epic, 0x40=Rare, 0x20=Uncommon
        if (speciesEntry->Flags & 0x100)
            petInfo.quality = PetQuality::LEGENDARY;
        else if (speciesEntry->Flags & 0x80)
            petInfo.quality = PetQuality::EPIC;
        else if (speciesEntry->Flags & 0x40)
            petInfo.quality = PetQuality::RARE;
        else if (speciesEntry->Flags & 0x20)
            petInfo.quality = PetQuality::UNCOMMON;
        else
            petInfo.quality = PetQuality::COMMON;

        // Calculate base stats for level 1
        uint32 baseHealth = 100 + petInfo.level * 5;
        petInfo.maxHealth = baseHealth;
        petInfo.health = petInfo.maxHealth;
        petInfo.power = 10 + petInfo.level * 2;
        petInfo.speed = 10;

        // Flags from DB2
        petInfo.isRare = (petInfo.quality >= PetQuality::RARE);
        petInfo.isTradeable = !(speciesEntry->Flags & 0x1);  // Flag 0x1 = not tradeable
        petInfo.isFavorite = false;

        // Assign default abilities based on family (3 abilities per pet)
        uint32 familyBase = static_cast<uint32>(petInfo.family) * 100;
        petInfo.abilities = {familyBase + 1, familyBase + 2, familyBase + 3};

        _petDatabase[speciesEntry->ID] = petInfo;
        ++db2SpeciesCount;
    }

    TC_LOG_INFO("playerbot.battlepet", "BattlePetManager: Loaded {} species from DB2 store", db2SpeciesCount);'''

# Try line-based approach: find starting line and ending line
lines = content.split('\n')
start_line = -1
end_line = -1

# Find "void BattlePetManager::LoadPetDatabase()"
for i, line in enumerate(lines):
    if 'void BattlePetManager::LoadPetDatabase()' in line:
        start_line = i
        break

if start_line == -1:
    print("ERROR: Could not find LoadPetDatabase function start")
    exit(1)

# Find "} while (result->NextRow());" to end the SQL section
for i in range(start_line, len(lines)):
    if '} while (result->NextRow());' in lines[i]:
        # Find the closing brace after this
        for j in range(i+1, len(lines)):
            if lines[j].strip() == '}':
                end_line = j
                break
        break

if end_line == -1:
    print("ERROR: Could not find end of SQL section")
    exit(1)

print(f"Found LoadPetDatabase from line {start_line+1} to {end_line+1}")

# Build new content
new_content = '\n'.join(lines[:start_line]) + '\n' + replacement + '\n' + '\n'.join(lines[end_line+1:])

# Write the file
with open('C:/TrinityBots/TrinityCore/src/modules/Playerbot/Companion/BattlePetManager.cpp', 'w') as f:
    f.write(new_content)
print('SUCCESS: LoadPetDatabase updated to use DB2 store')
