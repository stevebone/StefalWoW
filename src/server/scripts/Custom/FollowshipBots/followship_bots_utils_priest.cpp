#include "followship_bots_utils_priest.h"
#include "followship_bots_priest.h"

std::vector<FSBotSpells> AssistHealSpells =
{
    { SPELL_PRIEST_PURIFY,            8000, 90.0f },
    { SPELL_PRIEST_PRAYER_OF_MENDING, 9000, 80.0f },
    { SPELL_PRIEST_HEAL,              500,  70.0f },
    { SPELL_PRIEST_RENEW,             15000, 60.0f },
    { SPELL_PRIEST_FLASH_HEAL,        500,  50.0f }
};

std::vector<FSBotSpells> BalancedHealSpells =
{
    { SPELL_PRIEST_PURIFY,     8000, 90.0f },
    { SPELL_PRIEST_RENEW,      15000, 70.0f },
    { SPELL_PRIEST_FLASH_HEAL, 500,   60.0f }
};

std::vector<FSBotSpells> SelfHealSpells =
{
    { SPELL_PRIEST_PURIFY,            8000, 90.0f },
    { SPELL_PRIEST_PRAYER_OF_MENDING, 9000, 80.0f },
    { SPELL_PRIEST_RENEW,             15000, 70.0f },
    { SPELL_PRIEST_FLASH_HEAL,        500,  60.0f }
};

std::vector<FSBotSpells> CombatSpells =
{
    { SPELL_PRIEST_PSYCHIC_SCREAM,   45000, 100.0f },
    { SPELL_PRIEST_SHADOW_WORD_PAIN, 15000, 90.0f },
    { SPELL_PRIEST_VAMPIRIC_TOUCH,  1000,  90.0f },
    { SPELL_PRIEST_MIND_BLAST,       9000,  80.0f },
    { SPELL_PRIEST_HOLY_FIRE,        10000, 70.0f },
    { SPELL_PRIEST_SMITE,            1000,  60.0f },
    { SPELL_PRIEST_DEVOURING_PLAGUE, 10000, 50.0f },
    { SPELL_PRIEST_MIND_FLAY,        1000,  40.0f }
};
