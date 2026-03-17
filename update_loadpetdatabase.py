import re

# Read the file
with open('C:/TrinityBots/TrinityCore/src/modules/Playerbot/Companion/BattlePetManager.cpp', 'r') as f:
    content = f.read()

# Define the old section to replace (from function start to before the fallback)
old_section = '''void BattlePetManager::LoadPetDatabase()
{
    // DESIGN NOTE: Complete battle pet database loading implementation
    // Current behavior: Loads all WoW battle pet species from WorldDatabase
    // - Queries creature_template with type 15 (CREATURE_TYPE_BATTLEPET)
    // - Cross-references battle_pet_species for metadata
    // - Includes fallback hardcoded pet data if database query returns empty
    // Full implementation includes:
    // - Database tables: creature_template, battle_pet_species, battle_pet_breed
    // - Stat calculations based on level/quality/breed modifiers
    // - Ability assignments from battle_pet_species_abilities
    // Reference: TrinityCore WorldDatabase battle pet tables

    // Query creature templates that are battle pets (type 15 = CREATURE_TYPE_BATTLEPET)
    // Note: We only query creature_template from world database
    // Battle pet species metadata should be stored in playerbot database if needed
    // TrinityCore 11.2: minlevel/maxlevel, HealthModifier, DamageModifier columns removed
    // Levels are now dynamically scaled and modifiers are handled elsewhere
    QueryResult result = WorldDatabase.Query(
        "SELECT entry, name, speed_walk "
        "FROM creature_template "
        "WHERE type = 15 "
        "ORDER BY entry"
    );

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 creatureEntry = fields[0].GetUInt32();
            std::string name = fields[1].GetString();
            float speedWalk = fields[2].GetFloat();
            // TrinityCore 11.2: These modifiers don't exist in creature_template anymore
            // Use default values - actual scaling handled by game systems
            float healthMod = 1.0f;
            float damageMod = 1.0f;
            // TrinityCore 11.2: Battle pets use level scaling, default to standard pet level range
            uint32 minLevel = 1;
            uint32 maxLevel = 25;
            // Query battle_pet_species from playerbot database for metadata
            uint32 speciesId = creatureEntry;
            uint32 petType = 0;
            uint32 flags = 0;

            // Try to get extended metadata from playerbot.battle_pet_species
            if (sPlayerbotDatabase->IsConnected())
            {
                QueryResult speciesResult = sPlayerbotDatabase->Query(
                    "SELECT speciesId, petType, flags FROM battle_pet_species WHERE creatureId = " + std::to_string(creatureEntry));
                if (speciesResult)
                {
                    Field* speciesFields = speciesResult->Fetch();
                    speciesId = speciesFields[0].GetUInt32();
                    petType = speciesFields[1].GetUInt8();
                    flags = speciesFields[2].GetUInt32();
                }
            }

            BattlePetInfo petInfo;
            petInfo.speciesId = speciesId;
            petInfo.name = name;

            // Map WoW pet type to PetFamily enum
            switch (petType)
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

            petInfo.level = minLevel > 0 ? minLevel : 1;
            petInfo.xp = 0;

            // Determine quality based on flags or level range
            if (flags & 0x10)  // Legendary flag
                petInfo.quality = PetQuality::LEGENDARY;
            else if (flags & 0x8)  // Epic flag
                petInfo.quality = PetQuality::EPIC;
            else if (flags & 0x4)  // Rare flag
                petInfo.quality = PetQuality::RARE;
            else if (maxLevel > 20)
                petInfo.quality = PetQuality::UNCOMMON;
            else
                petInfo.quality = PetQuality::COMMON;

            // Calculate stats based on level and modifiers
            // Base stats scale with level (WoW formula: base + level * 5)
            uint32 baseHealth = 100 + petInfo.level * 5;
            petInfo.maxHealth = static_cast<uint32>(baseHealth * healthMod);
            petInfo.health = petInfo.maxHealth;

            uint32 basePower = 10 + petInfo.level * 2;
            petInfo.power = static_cast<uint32>(basePower * damageMod);

            // Speed calculation (walk speed typically 2.5-3.5)
            petInfo.speed = static_cast<uint32>((speedWalk > 0 ? speedWalk : 2.8f) * 4);

            // Flags for special pets
            petInfo.isRare = (petInfo.quality >= PetQuality::RARE);
            petInfo.isTradeable = !(flags & 0x20);  // Non-tradeable flag
            petInfo.isFavorite = false;

            _petDatabase[speciesId] = petInfo;

        } while (result->NextRow());
    }'''

# Define the new section
new_section = '''void BattlePetManager::LoadPetDatabase()
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

# Replace the old section with new section
new_content = content.replace(old_section, new_section)

if new_content == content:
    print('ERROR: Pattern not found - no replacement made')
else:
    # Write the file
    with open('C:/TrinityBots/TrinityCore/src/modules/Playerbot/Companion/BattlePetManager.cpp', 'w') as f:
        f.write(new_content)
    print('SUCCESS: LoadPetDatabase updated to use DB2 store')
