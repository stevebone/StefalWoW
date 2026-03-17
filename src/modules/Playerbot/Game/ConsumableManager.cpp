/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * CONSUMABLE MANAGER Implementation
 *
 * Manages pre-combat buffing (flasks, food, augment runes) and in-combat
 * consumable usage (health potions, mana potions, DPS potions, healthstones)
 * with context-aware selection based on spec, role, and content type.
 */

#include "ConsumableManager.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "Item.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"
#include "Map.h"
#include "Log.h"
#include "Timer.h"
#include "Group.h"
#include "Battleground.h"

namespace Playerbot
{

// ============================================================================
// CONSUMABLE DATABASES
// Sorted by priority (highest first) - best consumables checked first
// ============================================================================

// Flask/Phial database - covers TWW, Dragonflight, and legacy expansions
const std::vector<ConsumableInfo>& ConsumableManager::GetFlaskDatabase()
{
    static const std::vector<ConsumableInfo> database = {
        // ==========================================
        // The War Within (TWW) Phials - Top priority
        // ==========================================
        // Tank phials
        { 212283, ConsumableCategory::PHIAL, ConsumableRole::TANK,    0, 100, "Phial of Truesight" },
        { 212274, ConsumableCategory::PHIAL, ConsumableRole::TANK,    0, 99,  "Phial of Enhanced Ambush" },

        // Healer phials
        { 212281, ConsumableCategory::PHIAL, ConsumableRole::HEALER,  0, 100, "Phial of Concentrated Ingenuity" },

        // DPS phials (Melee + Ranged)
        { 212278, ConsumableCategory::PHIAL, ConsumableRole::ANY,     0, 100, "Phial of Bountiful Seasons" },
        { 212277, ConsumableCategory::PHIAL, ConsumableRole::ANY,     0, 99,  "Phial of Tempered Swiftness" },

        // ==========================================
        // Dragonflight Phials - Second tier
        // ==========================================
        { 191318, ConsumableCategory::PHIAL, ConsumableRole::ANY,     0, 80, "Phial of Tepid Versatility" },
        { 191319, ConsumableCategory::PHIAL, ConsumableRole::ANY,     0, 79, "Phial of Static Empowerment" },
        { 191332, ConsumableCategory::PHIAL, ConsumableRole::ANY,     0, 78, "Phial of Glacial Fury" },
        { 191341, ConsumableCategory::PHIAL, ConsumableRole::TANK,    0, 80, "Phial of the Eye in the Storm" },

        // ==========================================
        // Legacy Flasks - Lowest priority
        // ==========================================
        // Shadowlands
        { 171276, ConsumableCategory::FLASK, ConsumableRole::ANY,     0, 60, "Spectral Flask of Power" },
        { 171278, ConsumableCategory::FLASK, ConsumableRole::ANY,     0, 59, "Spectral Flask of Stamina" },

        // BfA
        { 168651, ConsumableCategory::FLASK, ConsumableRole::ANY,     0, 50, "Greater Flask of the Currents" },
        { 168652, ConsumableCategory::FLASK, ConsumableRole::ANY,     0, 49, "Greater Flask of Endless Fathoms" },
        { 168653, ConsumableCategory::FLASK, ConsumableRole::ANY,     0, 48, "Greater Flask of the Vast Horizon" },
        { 168654, ConsumableCategory::FLASK, ConsumableRole::ANY,     0, 47, "Greater Flask of the Undertow" },

        // Legion
        { 127847, ConsumableCategory::FLASK, ConsumableRole::ANY,     0, 40, "Flask of the Seventh Demon" },
        { 127848, ConsumableCategory::FLASK, ConsumableRole::ANY,     0, 39, "Flask of the Whispered Pact" },
        { 127849, ConsumableCategory::FLASK, ConsumableRole::ANY,     0, 38, "Flask of Ten Thousand Scars" },
        { 127850, ConsumableCategory::FLASK, ConsumableRole::ANY,     0, 37, "Flask of the Countless Armies" },

        // WoD
        { 109152, ConsumableCategory::FLASK, ConsumableRole::ANY,     0, 30, "Draenic Intellect Flask" },
        { 109153, ConsumableCategory::FLASK, ConsumableRole::ANY,     0, 29, "Draenic Strength Flask" },
        { 109155, ConsumableCategory::FLASK, ConsumableRole::ANY,     0, 28, "Draenic Stamina Flask" },
        { 109148, ConsumableCategory::FLASK, ConsumableRole::ANY,     0, 27, "Draenic Agility Flask" },

        // Classic/TBC/Wrath
        { 46376, ConsumableCategory::FLASK,  ConsumableRole::ANY,     0, 20, "Flask of the North" },
        { 46379, ConsumableCategory::FLASK,  ConsumableRole::ANY,     0, 19, "Flask of Endless Rage" },
        { 46377, ConsumableCategory::FLASK,  ConsumableRole::ANY,     0, 18, "Flask of the Frost Wyrm" },
        { 46378, ConsumableCategory::FLASK,  ConsumableRole::ANY,     0, 17, "Flask of Stoneblood" },
        { 22861, ConsumableCategory::FLASK,  ConsumableRole::ANY,     0, 10, "Flask of Supreme Power" },
        { 22851, ConsumableCategory::FLASK,  ConsumableRole::ANY,     0, 9,  "Flask of Fortification" },
    };
    return database;
}

// Food database - covers TWW, Dragonflight, and legacy
const std::vector<ConsumableInfo>& ConsumableManager::GetFoodDatabase()
{
    static const std::vector<ConsumableInfo> database = {
        // ==========================================
        // The War Within (TWW) Food - Top priority
        // ==========================================
        { 222720, ConsumableCategory::FOOD, ConsumableRole::ANY,      0, 100, "Feast of the Divine Day" },
        { 222721, ConsumableCategory::FOOD, ConsumableRole::ANY,      0, 99,  "Feast of the Midnight Masquerade" },
        { 222728, ConsumableCategory::FOOD, ConsumableRole::ANY,      0, 98,  "Sizzling Honey Roast" },
        { 222730, ConsumableCategory::FOOD, ConsumableRole::ANY,      0, 97,  "Tender Twilight Jerky" },
        { 222729, ConsumableCategory::FOOD, ConsumableRole::ANY,      0, 96,  "Fiery Fish Sticks" },

        // ==========================================
        // Dragonflight Food
        // ==========================================
        { 197794, ConsumableCategory::FOOD, ConsumableRole::ANY,      0, 80, "Grand Banquet of the Kalu'ak" },
        { 197795, ConsumableCategory::FOOD, ConsumableRole::ANY,      0, 79, "Hoard of Draconic Delicacies" },
        { 197784, ConsumableCategory::FOOD, ConsumableRole::ANY,      0, 78, "Fated Fortune Cookie" },
        { 197786, ConsumableCategory::FOOD, ConsumableRole::ANY,      0, 77, "Aromatic Seafood Platter" },

        // ==========================================
        // Legacy Food
        // ==========================================
        // Shadowlands
        { 172043, ConsumableCategory::FOOD, ConsumableRole::ANY,      0, 60, "Feast of Gluttonous Hedonism" },
        { 172041, ConsumableCategory::FOOD, ConsumableRole::ANY,      0, 59, "Iridescent Ravioli with Apple Sauce" },
        { 172045, ConsumableCategory::FOOD, ConsumableRole::ANY,      0, 58, "Tenebrous Crown Roast Aspic" },
        { 172049, ConsumableCategory::FOOD, ConsumableRole::ANY,      0, 57, "Steak a la Mode" },

        // BfA
        { 166240, ConsumableCategory::FOOD, ConsumableRole::ANY,      0, 50, "Sanguinated Feast" },
        { 168315, ConsumableCategory::FOOD, ConsumableRole::ANY,      0, 49, "Famine Evaluator And Snack Table" },
        { 154885, ConsumableCategory::FOOD, ConsumableRole::ANY,      0, 48, "Mon'Dazi" },

        // Generic conjured food (mage)
        { 113509, ConsumableCategory::FOOD, ConsumableRole::ANY,      0, 5,  "Conjured Mana Buns" },
        { 80618,  ConsumableCategory::FOOD, ConsumableRole::ANY,      0, 4,  "Conjured Mana Pudding" },
        { 80610,  ConsumableCategory::FOOD, ConsumableRole::ANY,      0, 3,  "Conjured Mana Fritter" },
    };
    return database;
}

// Augment Rune database
const std::vector<ConsumableInfo>& ConsumableManager::GetAugmentRuneDatabase()
{
    static const std::vector<ConsumableInfo> database = {
        // TWW
        { 224572, ConsumableCategory::AUGMENT_RUNE, ConsumableRole::ANY, 0, 100, "Crystallized Augment Rune" },
        // Dragonflight
        { 201325, ConsumableCategory::AUGMENT_RUNE, ConsumableRole::ANY, 0, 80,  "Draconic Augment Rune" },
        // Shadowlands
        { 181468, ConsumableCategory::AUGMENT_RUNE, ConsumableRole::ANY, 0, 60,  "Veiled Augment Rune" },
        // BfA
        { 160053, ConsumableCategory::AUGMENT_RUNE, ConsumableRole::ANY, 0, 50,  "Battle-Scarred Augment Rune" },
        // Legion
        { 153023, ConsumableCategory::AUGMENT_RUNE, ConsumableRole::ANY, 0, 40,  "Lightforged Augment Rune" },
        // WoD
        { 128482, ConsumableCategory::AUGMENT_RUNE, ConsumableRole::ANY, 0, 30,  "Empowered Augment Rune" },
    };
    return database;
}

// Health Potion database
const std::vector<ConsumableInfo>& ConsumableManager::GetHealthPotionDatabase()
{
    static const std::vector<ConsumableInfo> database = {
        // TWW
        { 211880, ConsumableCategory::HEALTH_POTION, ConsumableRole::ANY, 0, 100, "Algari Healing Potion" },
        // Dragonflight
        { 191380, ConsumableCategory::HEALTH_POTION, ConsumableRole::ANY, 0, 90,  "Potion of Withering Dreams" },
        { 191381, ConsumableCategory::HEALTH_POTION, ConsumableRole::ANY, 0, 89,  "Dreamwalker's Healing Potion" },
        { 191383, ConsumableCategory::HEALTH_POTION, ConsumableRole::ANY, 0, 88,  "Potion of Withering Vitality" },
        // Shadowlands
        { 171267, ConsumableCategory::HEALTH_POTION, ConsumableRole::ANY, 0, 70,  "Spiritual Healing Potion" },
        { 171270, ConsumableCategory::HEALTH_POTION, ConsumableRole::ANY, 0, 69,  "Potion of Spectral Healing" },
        { 171272, ConsumableCategory::HEALTH_POTION, ConsumableRole::ANY, 0, 68,  "Potion of Sacrificial Anima" },
        // BfA
        { 169451, ConsumableCategory::HEALTH_POTION, ConsumableRole::ANY, 0, 60,  "Abyssal Healing Potion" },
        { 152494, ConsumableCategory::HEALTH_POTION, ConsumableRole::ANY, 0, 59,  "Coastal Healing Potion" },
        // Legion
        { 127834, ConsumableCategory::HEALTH_POTION, ConsumableRole::ANY, 0, 50,  "Ancient Healing Potion" },
        { 127835, ConsumableCategory::HEALTH_POTION, ConsumableRole::ANY, 0, 49,  "Healing Potion (Legion)" },
        // WoD
        { 118910, ConsumableCategory::HEALTH_POTION, ConsumableRole::ANY, 0, 40,  "Draenic Rejuvenation Potion" },
        // Classic/TBC/Wrath
        { 33447,  ConsumableCategory::HEALTH_POTION, ConsumableRole::ANY, 0, 30,  "Runic Healing Potion" },
        { 22829,  ConsumableCategory::HEALTH_POTION, ConsumableRole::ANY, 0, 20,  "Super Healing Potion" },
        { 13446,  ConsumableCategory::HEALTH_POTION, ConsumableRole::ANY, 0, 15,  "Major Healing Potion" },
        { 3928,   ConsumableCategory::HEALTH_POTION, ConsumableRole::ANY, 0, 10,  "Superior Healing Potion" },
        { 1710,   ConsumableCategory::HEALTH_POTION, ConsumableRole::ANY, 0, 5,   "Greater Healing Potion" },
        { 929,    ConsumableCategory::HEALTH_POTION, ConsumableRole::ANY, 0, 3,   "Healing Potion" },
        { 118,    ConsumableCategory::HEALTH_POTION, ConsumableRole::ANY, 0, 1,   "Minor Healing Potion" },
    };
    return database;
}

// Mana Potion database
const std::vector<ConsumableInfo>& ConsumableManager::GetManaPotionDatabase()
{
    static const std::vector<ConsumableInfo> database = {
        // TWW
        { 211882, ConsumableCategory::MANA_POTION, ConsumableRole::ANY, 0, 100, "Algari Mana Potion" },
        // Dragonflight
        { 191386, ConsumableCategory::MANA_POTION, ConsumableRole::ANY, 0, 90,  "Potion of Frozen Focus" },
        { 191387, ConsumableCategory::MANA_POTION, ConsumableRole::ANY, 0, 89,  "Aerated Mana Potion" },
        // Shadowlands
        { 171268, ConsumableCategory::MANA_POTION, ConsumableRole::ANY, 0, 70,  "Spiritual Mana Potion" },
        { 171269, ConsumableCategory::MANA_POTION, ConsumableRole::ANY, 0, 69,  "Potion of Spectral Intellect" },
        // BfA
        { 152495, ConsumableCategory::MANA_POTION, ConsumableRole::ANY, 0, 60,  "Coastal Mana Potion" },
        // Legion
        { 127835, ConsumableCategory::MANA_POTION, ConsumableRole::ANY, 0, 50,  "Ancient Mana Potion" },
        // WoD
        { 109222, ConsumableCategory::MANA_POTION, ConsumableRole::ANY, 0, 40,  "Draenic Mana Potion" },
        // Classic/TBC/Wrath
        { 33448,  ConsumableCategory::MANA_POTION, ConsumableRole::ANY, 0, 30,  "Runic Mana Potion" },
        { 22832,  ConsumableCategory::MANA_POTION, ConsumableRole::ANY, 0, 20,  "Super Mana Potion" },
        { 13444,  ConsumableCategory::MANA_POTION, ConsumableRole::ANY, 0, 15,  "Major Mana Potion" },
        { 6149,   ConsumableCategory::MANA_POTION, ConsumableRole::ANY, 0, 10,  "Greater Mana Potion" },
        { 3827,   ConsumableCategory::MANA_POTION, ConsumableRole::ANY, 0, 5,   "Mana Potion" },
        { 2455,   ConsumableCategory::MANA_POTION, ConsumableRole::ANY, 0, 3,   "Minor Mana Potion" },
    };
    return database;
}

// DPS Combat Potion database
const std::vector<ConsumableInfo>& ConsumableManager::GetDPSPotionDatabase()
{
    static const std::vector<ConsumableInfo> database = {
        // TWW DPS potions
        { 212265, ConsumableCategory::DPS_POTION, ConsumableRole::MELEE_DPS,  0, 100, "Tempered Potion" },
        { 212259, ConsumableCategory::DPS_POTION, ConsumableRole::CASTER_DPS, 0, 100, "Potion of Unwavering Focus" },

        // Dragonflight
        { 191389, ConsumableCategory::DPS_POTION, ConsumableRole::ANY,        0, 80,  "Elemental Potion of Ultimate Power" },
        { 191388, ConsumableCategory::DPS_POTION, ConsumableRole::ANY,        0, 79,  "Elemental Potion of Power" },

        // Shadowlands
        { 171275, ConsumableCategory::DPS_POTION, ConsumableRole::ANY,        0, 60,  "Potion of Spectral Agility" },
        { 171273, ConsumableCategory::DPS_POTION, ConsumableRole::ANY,        0, 59,  "Potion of Spectral Strength" },
        { 171274, ConsumableCategory::DPS_POTION, ConsumableRole::ANY,        0, 58,  "Potion of Spectral Intellect" },

        // BfA
        { 168529, ConsumableCategory::DPS_POTION, ConsumableRole::ANY,        0, 50,  "Potion of Unbridled Fury" },
        { 169299, ConsumableCategory::DPS_POTION, ConsumableRole::ANY,        0, 49,  "Potion of Focused Resolve" },

        // Legion
        { 127844, ConsumableCategory::DPS_POTION, ConsumableRole::ANY,        0, 40,  "Potion of Prolonged Power" },
        { 127843, ConsumableCategory::DPS_POTION, ConsumableRole::ANY,        0, 39,  "Potion of Deadly Grace" },
    };
    return database;
}

// Healthstone database
const std::vector<ConsumableInfo>& ConsumableManager::GetHealthstoneDatabase()
{
    static const std::vector<ConsumableInfo> database = {
        { 224464, ConsumableCategory::HEALTHSTONE, ConsumableRole::ANY, 0, 100, "Healthstone (TWW)" },
        { 207030, ConsumableCategory::HEALTHSTONE, ConsumableRole::ANY, 0, 90,  "Healthstone (Dragonflight)" },
        { 177278, ConsumableCategory::HEALTHSTONE, ConsumableRole::ANY, 0, 80,  "Healthstone (Shadowlands)" },
        { 156438, ConsumableCategory::HEALTHSTONE, ConsumableRole::ANY, 0, 70,  "Healthstone (BfA)" },
        { 152303, ConsumableCategory::HEALTHSTONE, ConsumableRole::ANY, 0, 60,  "Healthstone (Legion)" },
        { 5512,   ConsumableCategory::HEALTHSTONE, ConsumableRole::ANY, 0, 50,  "Healthstone (generic)" },
        { 36889,  ConsumableCategory::HEALTHSTONE, ConsumableRole::ANY, 0, 20,  "Fel Healthstone" },
        { 36892,  ConsumableCategory::HEALTHSTONE, ConsumableRole::ANY, 0, 19,  "Demonic Healthstone" },
    };
    return database;
}

// ============================================================================
// CONSTRUCTOR
// ============================================================================

ConsumableManager::ConsumableManager(Player* bot, BotAI* ai)
    : _bot(bot)
    , _ai(ai)
    , _contentType(ContentType::OPEN_WORLD)
    , _role(ConsumableRole::ANY)
{
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void ConsumableManager::Initialize()
{
    if (_initialized)
        return;

    if (!_bot || !_bot->IsInWorld())
        return;

    _contentType = DetermineContentType();
    _role = DetermineConsumableRole();
    ScanInventory();
    UpdateBuffState();
    _initialized = true;

    TC_LOG_DEBUG("module.playerbot", "ConsumableManager initialized for bot {} (role={}, content={})",
        _bot->GetName(), static_cast<uint8>(_role), static_cast<uint8>(_contentType));
}

void ConsumableManager::Update(uint32 diff)
{
    if (!_initialized)
    {
        Initialize();
        if (!_initialized)
            return;
    }

    if (!_bot || !_bot->IsInWorld() || !_bot->IsAlive())
        return;

    // Periodically re-scan inventory (30 second interval)
    _inventoryScanTimer += diff;
    if (_inventoryScanTimer >= INVENTORY_SCAN_INTERVAL)
    {
        _inventoryScanTimer = 0;
        ScanInventory();
        _contentType = DetermineContentType();
        _role = DetermineConsumableRole();
    }

    if (_bot->IsInCombat())
    {
        // In combat: check for emergency consumable usage (500ms throttle)
        _inCombatUpdateTimer += diff;
        if (_inCombatUpdateTimer >= IN_COMBAT_UPDATE_INTERVAL)
        {
            _inCombatUpdateTimer = 0;
            UpdateBuffState();

            // Emergency health consumable at low HP
            float healthPct = _bot->GetHealthPct();
            if (healthPct <= HEALTHSTONE_THRESHOLD)
            {
                UseEmergencyHealthConsumable();
            }

            // Emergency mana consumable for healers/casters at low mana
            if (_role == ConsumableRole::HEALER || _role == ConsumableRole::CASTER_DPS)
            {
                float manaPct = 0.0f;
                uint32 maxPower = _bot->GetMaxPower(POWER_MANA);
                if (maxPower > 0)
                    manaPct = (static_cast<float>(_bot->GetPower(POWER_MANA)) / maxPower) * 100.0f;

                if (manaPct <= MANA_POTION_THRESHOLD)
                {
                    UseEmergencyManaConsumable();
                }
            }
        }
    }
    else
    {
        // Out of combat: check for pre-combat buffs (5s throttle)
        _outOfCombatUpdateTimer += diff;
        if (_outOfCombatUpdateTimer >= OUT_OF_COMBAT_UPDATE_INTERVAL)
        {
            _outOfCombatUpdateTimer = 0;
            UpdateBuffState();

            // Don't try to buff if currently eating/drinking
            if (!IsEatingOrDrinking())
            {
                if (ShouldUseConsumablesForContent() && IsMissingPreCombatBuffs())
                {
                    ApplyPreCombatBuffs();
                }
            }
        }
    }
}

// ============================================================================
// PRE-COMBAT BUFFING
// ============================================================================

bool ConsumableManager::ApplyPreCombatBuffs()
{
    if (!_bot || !_bot->IsAlive() || _bot->IsInCombat())
        return false;

    // Don't buff while moving
    if (_bot->isMoving())
        return false;

    // Priority 1: Flask/Phial (most important long-duration buff)
    if (!_state.hasFlaskOrPhial)
    {
        uint32 flaskId = FindBestConsumable(ConsumableCategory::PHIAL, _role);
        if (flaskId == 0)
            flaskId = FindBestConsumable(ConsumableCategory::FLASK, _role);
        if (flaskId == 0)
        {
            // Try with ANY role if no role-specific flask found
            flaskId = FindBestConsumable(ConsumableCategory::PHIAL, ConsumableRole::ANY);
            if (flaskId == 0)
                flaskId = FindBestConsumable(ConsumableCategory::FLASK, ConsumableRole::ANY);
        }

        if (flaskId != 0 && TryUseItem(flaskId))
        {
            TC_LOG_DEBUG("module.playerbot", "Bot {} used flask/phial (item {})",
                _bot->GetName(), flaskId);
            return true;  // One item per tick to respect GCD
        }
    }

    // Priority 2: Food buff (requires 10s channel, so start early)
    if (!_state.hasFoodBuff)
    {
        uint32 foodId = FindBestConsumable(ConsumableCategory::FOOD, _role);
        if (foodId == 0)
            foodId = FindBestConsumable(ConsumableCategory::FOOD, ConsumableRole::ANY);

        if (foodId != 0 && TryUseItem(foodId))
        {
            _state.lastFoodBuffTime = GameTime::GetGameTimeMS();
            TC_LOG_DEBUG("module.playerbot", "Bot {} started eating food (item {})",
                _bot->GetName(), foodId);
            return true;
        }
    }

    // Priority 3: Augment Rune (raid content primarily)
    if (!_state.hasAugmentRune && _contentType == ContentType::RAID)
    {
        uint32 runeId = FindBestConsumable(ConsumableCategory::AUGMENT_RUNE);
        if (runeId != 0 && TryUseItem(runeId))
        {
            TC_LOG_DEBUG("module.playerbot", "Bot {} used augment rune (item {})",
                _bot->GetName(), runeId);
            return true;
        }
    }

    return false;
}

bool ConsumableManager::IsMissingPreCombatBuffs() const
{
    ContentType content = _contentType;

    // Open world - minimal requirements
    if (content == ContentType::OPEN_WORLD)
        return false;  // No consumables needed for open world

    // PvP - no pre-combat buffs needed (potions during combat only)
    if (content == ContentType::PVP)
        return false;

    // Dungeon/Raid/Delve - check for missing buffs
    if (!_state.hasFlaskOrPhial)
        return true;

    if (!_state.hasFoodBuff)
        return true;

    // Augment rune only expected in raids
    if (content == ContentType::RAID && !_state.hasAugmentRune)
        return true;

    return false;
}

// ============================================================================
// COMBAT CONSUMABLE USAGE
// ============================================================================

bool ConsumableManager::UseEmergencyHealthConsumable()
{
    if (!_bot || !_bot->IsAlive())
        return false;

    float healthPct = _bot->GetHealthPct();
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Try healthstone first (separate cooldown from potions, so higher priority)
    if (healthPct <= HEALTHSTONE_THRESHOLD && !IsHealthstoneOnCooldown())
    {
        uint32 healthstoneId = FindBestConsumable(ConsumableCategory::HEALTHSTONE);
        if (healthstoneId != 0 && TryUseItem(healthstoneId))
        {
            _state.lastHealthstoneUseTime = currentTime;
            _state.healthstoneOnCooldown = true;
            TC_LOG_DEBUG("module.playerbot", "Bot {} used Healthstone (item {}) at {:.1f}% HP",
                _bot->GetName(), healthstoneId, healthPct);
            return true;
        }
    }

    // Try health potion (shared combat potion cooldown)
    if (healthPct <= HEALTH_POTION_THRESHOLD && !IsPotionOnCooldown())
    {
        uint32 potionId = FindBestConsumable(ConsumableCategory::HEALTH_POTION);
        if (potionId != 0 && TryUseItem(potionId))
        {
            _state.lastPotionUseTime = currentTime;
            _state.potionOnCooldown = true;
            TC_LOG_DEBUG("module.playerbot", "Bot {} used Health Potion (item {}) at {:.1f}% HP",
                _bot->GetName(), potionId, healthPct);
            return true;
        }
    }

    return false;
}

bool ConsumableManager::UseEmergencyManaConsumable()
{
    if (!_bot || !_bot->IsAlive())
        return false;

    // Only for mana users
    uint32 maxMana = _bot->GetMaxPower(POWER_MANA);
    if (maxMana == 0)
        return false;

    float manaPct = (static_cast<float>(_bot->GetPower(POWER_MANA)) / maxMana) * 100.0f;

    if (manaPct > MANA_POTION_THRESHOLD || IsPotionOnCooldown())
        return false;

    uint32 potionId = FindBestConsumable(ConsumableCategory::MANA_POTION);
    if (potionId != 0 && TryUseItem(potionId))
    {
        _state.lastPotionUseTime = GameTime::GetGameTimeMS();
        _state.potionOnCooldown = true;
        TC_LOG_DEBUG("module.playerbot", "Bot {} used Mana Potion (item {}) at {:.1f}% mana",
            _bot->GetName(), potionId, manaPct);
        return true;
    }

    return false;
}

bool ConsumableManager::UseCombatPotion()
{
    if (!_bot || !_bot->IsAlive() || !_bot->IsInCombat())
        return false;

    // Only use combat potions in meaningful content
    if (_contentType == ContentType::OPEN_WORLD)
        return false;

    if (IsPotionOnCooldown())
        return false;

    // Find appropriate DPS/healing potion based on role
    uint32 potionId = 0;
    if (_role == ConsumableRole::HEALER)
    {
        // Healers use mana potions for burst healing phases
        potionId = FindBestConsumable(ConsumableCategory::MANA_POTION);
    }
    else
    {
        // DPS/Tanks use DPS combat potions
        potionId = FindBestConsumable(ConsumableCategory::DPS_POTION, _role);
        if (potionId == 0)
            potionId = FindBestConsumable(ConsumableCategory::DPS_POTION, ConsumableRole::ANY);
    }

    if (potionId != 0 && TryUseItem(potionId))
    {
        _state.lastPotionUseTime = GameTime::GetGameTimeMS();
        _state.potionOnCooldown = true;
        TC_LOG_DEBUG("module.playerbot", "Bot {} used combat potion (item {}) during burst",
            _bot->GetName(), potionId);
        return true;
    }

    return false;
}

// ============================================================================
// STATE QUERIES
// ============================================================================

bool ConsumableManager::IsPotionOnCooldown() const
{
    if (!_state.potionOnCooldown)
        return false;

    uint32 elapsed = GameTime::GetGameTimeMS() - _state.lastPotionUseTime;
    return elapsed < POTION_COOLDOWN_MS;
}

bool ConsumableManager::IsHealthstoneOnCooldown() const
{
    if (!_state.healthstoneOnCooldown)
        return false;

    uint32 elapsed = GameTime::GetGameTimeMS() - _state.lastHealthstoneUseTime;
    return elapsed < HEALTHSTONE_COOLDOWN_MS;
}

ContentType ConsumableManager::GetCurrentContentType() const
{
    return _contentType;
}

ConsumableRole ConsumableManager::GetConsumableRole() const
{
    return _role;
}

void ConsumableManager::RefreshInventoryCache()
{
    ScanInventory();
}

// ============================================================================
// INTERNAL METHODS
// ============================================================================

void ConsumableManager::ScanInventory()
{
    if (!_bot)
        return;

    _availableConsumables.clear();

    // Helper lambda to scan a database and add available items
    auto scanDatabase = [this](const std::vector<ConsumableInfo>& database, ConsumableCategory category)
    {
        for (const auto& info : database)
        {
            if (_bot->HasItemCount(info.itemId, 1))
            {
                _availableConsumables[category].push_back(info);
            }
        }
    };

    // Scan all databases
    scanDatabase(GetFlaskDatabase(), ConsumableCategory::FLASK);
    scanDatabase(GetFlaskDatabase(), ConsumableCategory::PHIAL);  // Phials are in flask database
    scanDatabase(GetFoodDatabase(), ConsumableCategory::FOOD);
    scanDatabase(GetAugmentRuneDatabase(), ConsumableCategory::AUGMENT_RUNE);
    scanDatabase(GetHealthPotionDatabase(), ConsumableCategory::HEALTH_POTION);
    scanDatabase(GetManaPotionDatabase(), ConsumableCategory::MANA_POTION);
    scanDatabase(GetDPSPotionDatabase(), ConsumableCategory::DPS_POTION);
    scanDatabase(GetHealthstoneDatabase(), ConsumableCategory::HEALTHSTONE);

    _lastInventoryScan = GameTime::GetGameTimeMS();

    // Log summary
    uint32 totalItems = 0;
    for (const auto& [cat, items] : _availableConsumables)
        totalItems += static_cast<uint32>(items.size());

    TC_LOG_TRACE("module.playerbot", "Bot {} consumable scan: {} items across {} categories",
        _bot->GetName(), totalItems, _availableConsumables.size());
}

void ConsumableManager::UpdateBuffState()
{
    if (!_bot)
        return;

    // Check for flask/phial auras
    // We check broadly - any aura from a flask/phial item means we're buffed
    // This works because HasAura checks by spell ID, and item usage applies specific auras
    _state.hasFlaskOrPhial = false;

    // Check if bot has any flask-type aura effect
    // Flask auras have SPELL_AURA_MOD_STAT or SPELL_AURA_MOD_RATING with long duration
    // We check for common flask aura spell IDs and item enchant aura patterns
    {
        // Check all auras for flask/phial buff patterns
        // Flasks typically have duration > 30 minutes and boost primary stats
        auto const& auras = _bot->GetAppliedAuras();
        for (auto const& [spellId, auraApp] : auras)
        {
            if (!auraApp || !auraApp->GetBase())
                continue;

            Aura* aura = auraApp->GetBase();

            // Flask/Phial check: duration >= 30 minutes, triggered by item
            if (aura->GetMaxDuration() >= 30 * 60 * 1000) // 30 minutes in ms
            {
                SpellInfo const* spellInfo = aura->GetSpellInfo();
                if (spellInfo)
                {
                    // Check for stat-boosting effects typical of flasks
                    for (uint8 i = 0; i < spellInfo->GetEffects().size(); ++i)
                    {
                        SpellEffectInfo const& effect = spellInfo->GetEffects()[i];
                        if (effect.ApplyAuraName == SPELL_AURA_MOD_STAT ||
                            effect.ApplyAuraName == SPELL_AURA_MOD_RATING ||
                            effect.ApplyAuraName == SPELL_AURA_MOD_INCREASE_HEALTH_2 ||
                            effect.ApplyAuraName == SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE)
                        {
                            _state.hasFlaskOrPhial = true;
                            break;
                        }
                    }
                }
            }
            if (_state.hasFlaskOrPhial) break;
        }
    }

    // Check for food buff (Well Fed)
    // Well Fed auras typically have SPELL_AURA_MOD_STAT and duration around 60 minutes
    // but are NOT flasks (shorter or different effect pattern)
    _state.hasFoodBuff = false;
    {
        // Common Well Fed aura spell IDs across expansions
        // These are the buff auras applied by food items
        static const uint32 WELL_FED_AURAS[] = {
            // Generic Well Fed
            19705,  // Well Fed (generic buff)
            // TWW Well Fed variants
            462210, 462213, 462214, 462216,
            // Dragonflight Well Fed variants
            382427, 382428, 382429, 382430, 382431,
            // Shadowlands
            327701, 327704, 327705, 327706, 327707,
            // BfA
            257410, 257413, 257415, 257418,
        };

        for (uint32 auraId : WELL_FED_AURAS)
        {
            if (_bot->HasAura(auraId))
            {
                _state.hasFoodBuff = true;
                break;
            }
        }

        // Fallback: check for any aura with "Well Fed" pattern
        // Duration 30-60 minutes with stat boost
        if (!_state.hasFoodBuff)
        {
            auto const& auras = _bot->GetAppliedAuras();
            for (auto const& [spellId, auraApp] : auras)
            {
                if (!auraApp || !auraApp->GetBase())
                    continue;

                Aura* aura = auraApp->GetBase();
                int32 duration = aura->GetMaxDuration();

                // Food buffs: 15-60 minute duration, shorter than flasks
                if (duration >= 15 * 60 * 1000 && duration <= 60 * 60 * 1000)
                {
                    // Already counted as flask if >= 30 min, so skip those
                    if (_state.hasFlaskOrPhial && duration >= 30 * 60 * 1000)
                        continue;

                    SpellInfo const* spellInfo = aura->GetSpellInfo();
                    if (spellInfo)
                    {
                        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                        {
                            SpellEffectInfo const& effect = spellInfo->GetEffect(SpellEffIndex(i));
                            if (effect.ApplyAuraName == SPELL_AURA_MOD_STAT ||
                                effect.ApplyAuraName == SPELL_AURA_MOD_RATING)
                            {
                                _state.hasFoodBuff = true;
                                break;
                            }
                        }
                    }
                }
                if (_state.hasFoodBuff) break;
            }
        }
    }

    // Check for augment rune (typically a primary stat buff with specific aura IDs)
    _state.hasAugmentRune = false;
    {
        static const uint32 AUGMENT_RUNE_AURAS[] = {
            395665, // Crystallized Augment Rune (TWW)
            393438, // Draconic Augment Rune (Dragonflight)
            347901, // Veiled Augment Rune (SL)
            270058, // Battle-Scarred Augment Rune (BfA)
            224001, // Lightforged Augment Rune (Legion)
            175457, // Empowered Augment Rune (WoD)
        };

        for (uint32 auraId : AUGMENT_RUNE_AURAS)
        {
            if (_bot->HasAura(auraId))
            {
                _state.hasAugmentRune = true;
                break;
            }
        }
    }

    // Update potion/healthstone cooldown state from elapsed time
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (_state.potionOnCooldown)
    {
        uint32 elapsed = currentTime - _state.lastPotionUseTime;
        if (elapsed >= POTION_COOLDOWN_MS)
            _state.potionOnCooldown = false;
    }
    if (_state.healthstoneOnCooldown)
    {
        uint32 elapsed = currentTime - _state.lastHealthstoneUseTime;
        if (elapsed >= HEALTHSTONE_COOLDOWN_MS)
            _state.healthstoneOnCooldown = false;
    }
}

ContentType ConsumableManager::DetermineContentType() const
{
    if (!_bot)
        return ContentType::OPEN_WORLD;

    // Check for battleground
    if (_bot->InBattleground())
        return ContentType::PVP;

    // Check for arena
    if (_bot->InArena())
        return ContentType::PVP;

    Map* map = _bot->GetMap();
    if (!map)
        return ContentType::OPEN_WORLD;

    // Check for raid
    if (map->IsRaid())
        return ContentType::RAID;

    // Check for dungeon
    if (map->IsDungeon())
        return ContentType::DUNGEON;

    // Check for group content (could be a delve or outdoor group content)
    if (_bot->GetGroup() && _bot->GetGroup()->GetMembersCount() >= 3)
        return ContentType::DELVE;

    return ContentType::OPEN_WORLD;
}

ConsumableRole ConsumableManager::DetermineConsumableRole() const
{
    if (!_bot)
        return ConsumableRole::ANY;

    uint8 classId = _bot->GetClass();
    // Use TalentSpecialization if available, otherwise infer from class
    // For now, infer from class + role assignment

    // Check if bot has a group role
    Group* group = _bot->GetGroup();
    if (group)
    {
        uint8 role = group->GetLfgRoles(_bot->GetGUID());
        if (role & (1 << 0)) // ROLE_TANK = bit 0 in some implementations
        {
            // Check standard LFG role flags
            // PLAYER_ROLE_TANK = 0x02, PLAYER_ROLE_HEALER = 0x04, PLAYER_ROLE_DAMAGE = 0x08
            if (role & 0x02)
                return ConsumableRole::TANK;
            if (role & 0x04)
                return ConsumableRole::HEALER;
        }
    }

    // Infer from class
    switch (classId)
    {
        // Pure healers are detected by spec, but as fallback:
        case CLASS_WARRIOR:
            return ConsumableRole::MELEE_DPS;
        case CLASS_PALADIN:
            return ConsumableRole::MELEE_DPS; // Could be tank/healer
        case CLASS_HUNTER:
            return ConsumableRole::RANGED_DPS;
        case CLASS_ROGUE:
            return ConsumableRole::MELEE_DPS;
        case CLASS_PRIEST:
            return ConsumableRole::CASTER_DPS; // Could be healer
        case CLASS_DEATH_KNIGHT:
            return ConsumableRole::MELEE_DPS;
        case CLASS_SHAMAN:
            return ConsumableRole::CASTER_DPS; // Could be melee/healer
        case CLASS_MAGE:
            return ConsumableRole::CASTER_DPS;
        case CLASS_WARLOCK:
            return ConsumableRole::CASTER_DPS;
        case CLASS_MONK:
            return ConsumableRole::MELEE_DPS; // Could be tank/healer
        case CLASS_DRUID:
            return ConsumableRole::MELEE_DPS; // Could be caster/tank/healer
        case CLASS_DEMON_HUNTER:
            return ConsumableRole::MELEE_DPS;
        case CLASS_EVOKER:
            return ConsumableRole::CASTER_DPS; // Could be healer
        default:
            return ConsumableRole::ANY;
    }
}

bool ConsumableManager::TryUseItem(uint32 itemId)
{
    if (!_bot)
        return false;

    Item* item = _bot->GetItemByEntry(itemId);
    if (!item)
        return false;

    // Build spell cast targets (self-target for consumables)
    SpellCastTargets targets;
    targets.SetUnitTarget(_bot);

    // WoW 12.0: CastItemUseSpell signature uses std::array<int32, 3>
    std::array<int32, 3> misc = { 0, 0, 0 };
    _bot->CastItemUseSpell(item, targets, ObjectGuid::Empty, misc);

    return true;
}

uint32 ConsumableManager::FindBestConsumable(ConsumableCategory category, ConsumableRole role) const
{
    // For phials, they're stored in the FLASK category since they share the same slot
    ConsumableCategory searchCategory = category;
    if (category == ConsumableCategory::PHIAL)
        searchCategory = ConsumableCategory::FLASK;

    auto it = _availableConsumables.find(searchCategory);
    if (it == _availableConsumables.end())
        return 0;

    const auto& items = it->second;
    uint32 bestItemId = 0;
    uint32 bestPriority = 0;

    for (const auto& info : items)
    {
        // Category filter: if searching for PHIAL specifically, only match PHIAL entries
        if (category == ConsumableCategory::PHIAL && info.category != ConsumableCategory::PHIAL)
            continue;
        if (category == ConsumableCategory::FLASK && info.category != ConsumableCategory::FLASK)
            continue;

        // Role filter
        if (role != ConsumableRole::ANY && info.role != ConsumableRole::ANY && info.role != role)
            continue;

        // Verify item is still in inventory (may have been used since scan)
        if (!_bot->HasItemCount(info.itemId, 1))
            continue;

        // Pick highest priority
        if (info.priority > bestPriority)
        {
            bestPriority = info.priority;
            bestItemId = info.itemId;
        }
    }

    return bestItemId;
}

bool ConsumableManager::HasAura(uint32 auraId) const
{
    if (!_bot)
        return false;
    return _bot->HasAura(auraId);
}

bool ConsumableManager::HasAnyAura(const uint32* auraIds, size_t count) const
{
    if (!_bot)
        return false;

    for (size_t i = 0; i < count; ++i)
    {
        if (_bot->HasAura(auraIds[i]))
            return true;
    }
    return false;
}

bool ConsumableManager::ShouldUseConsumablesForContent() const
{
    // Only use pre-combat consumables in meaningful content
    switch (_contentType)
    {
        case ContentType::DUNGEON:
        case ContentType::RAID:
        case ContentType::DELVE:
            return true;
        case ContentType::PVP:
            return false; // PvP uses combat consumables only
        case ContentType::OPEN_WORLD:
        default:
            return false;
    }
}

bool ConsumableManager::IsEatingOrDrinking() const
{
    if (!_bot)
        return false;

    // Check for food/drink channel auras
    // Food: SPELL_AURA_MOD_REGEN with specific food flags
    // Drink: SPELL_AURA_MOD_POWER_REGEN
    // Simpler check: eating/drinking sets the player to sitting
    if (_bot->HasUnitState(UNIT_STATE_CASTING))
        return true;

    // Check for recently started eating (within last 10 seconds)
    if (_state.lastFoodBuffTime > 0)
    {
        uint32 elapsed = GameTime::GetGameTimeMS() - _state.lastFoodBuffTime;
        if (elapsed < FOOD_CHANNEL_TIME_MS)
            return true;
    }

    return false;
}

} // namespace Playerbot
