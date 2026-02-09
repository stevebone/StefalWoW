
#include "Followship_bots_config.h"
#include "Followship_bots_utils.h"
#include "Followship_bots_paladin.h"
#include "Followship_bots_priest.h"
#include "Followship_bots_utils_spells.h"


constexpr float BOT_RECOVERY_HP_PCT = 50.f;
constexpr float BOT_RECOVERY_MP_PCT = 50.f;


enum BotRecoverySpells
{
    SPELL_DRINK_CONJURED_CRYSTAL_WATER = 22734,
    SPELL_FOOD_SCALED_WITH_LVL = 1131,

    SPELL_RECUPERATE = 1231418,

    // Draenei spells
    SPELL_DRAENEI_GIFT_NAARU = 28880, //416250,

    // Mage spells
    SPELL_MAGE_CONJURED_MANA_PUDDING = 167152
};

enum class BotRecoveryIntent
{
    None,
    RecoverHealth,
    RecoverMana,
    RecoverHealthAndMana
};

enum class BotRecoverAction
{
    None,
    DrinkEat,
    Drink,
    ClassDrinkEat,
    Eat,
    Recuperate,
    ClassHeal,
    RaceHeal
};

enum BotDrinkEatType
{
    DRINK = 0,
    EAT = 1,
    DRINK_EAT = 2,
    CLASS_DRINK_EAT = 3,
    RECUPERATE = 4,
    CLASS_HEAL = 5,
    RACE_HEAL = 6
};

struct DrinkFoodPerLevel
{
    uint8 minLevel;
    uint8 maxLevel;
    int32 value;
};

// min level, max level, value
static constexpr DrinkFoodPerLevel DrinkFoodTable[] =
{
    { 1, 10,  10 }, // level 1-10
    { 11, 20, 25 }, // level 11-20
    { 21, 30, 45 },
    { 31, 40, 90 },
    { 41, 50, 120 },
    { 51, 60, 220 },
};

namespace FSBRecovery
{
    BotRecoveryIntent DetermineRecoveryIntent(Creature* bot);
    BotRecoverAction GetRecoveryAction(Creature* bot, BotRecoveryIntent intent);

    bool TryRecoverAction(Creature* bot, BotRecoverAction action, uint32& globalCooldown);

    bool TryOOCRecovery(Creature* bot, BotRecoveryIntent intent, uint32& globalCooldown);

    // Gets drink amount of mana regen per level
    int32 GetDrinkFoodPerLevel(uint8 level, DrinkFoodPerLevel const* table, size_t tableSize);

    // Wrapper for GetDrinkManaRegenPerLevel
    int32 GetDrinkFood(uint16 level);
}
