#pragma once

#include <cmath>
#include <string>
#include <vector>
#include <random>
#include <cstdint>

#include "Creature.h"
#include "MotionMaster.h"
#include "Player.h"
#include "Unit.h"

#include "Followship_bots_defines.h"
#include "Followship_bots_utils_combat.h"
#include "Followship_bots_utils_gossip.h"
#include "Followship_bots_utils_spells.h"
#include "Followship_bots_utils_stats.h"


namespace FSBUtils
{
    FSB_Class GetBotClassForEntry(uint32 entry);
    void SetBotClass(Creature* creature, FSB_Class& outClass);

    FSB_Race GetBotRaceForEntry(uint32 entry);
    void SetBotRace(Creature* creature, FSB_Race& outRace);

    const char* BotClassToString(FSB_Class cls);
    const char* BotRaceToString(FSB_Race race);
    const char* PowerTypeToString(Powers power);

    constexpr float SIDE_OFFSET_MAX = float(M_PI) / 6.0f; // +-30 degrees from pure left/right

    // Returns a random angle slightly to the left of the player
    float GetRandomLeftAngle();

    // Returns a random angle slightly to the right of the player
    float GetRandomRightAngle();

    // Get the role of a bot (returns FSB_ROLE_NONE if not a bot or AI not present)
    FSB_Roles GetRole(Creature* unit);

    // Set the role of a bot (does nothing if not a bot or AI not present)
    void SetRole(Creature* unit, FSB_Roles role);

    bool TryChargeHire(Player* player, uint32 duration);

    bool BotIsHealerClass(Creature* bot);
}

namespace FSBUtilsMovement
{
    // Returns true if chase was started or is ongoing
    bool EnsureInRange(Creature* me, Unit* target, float requiredRange);
    bool EnsureLOS(Unit* me, Unit* target);

    
}

enum class FSBSayType
{
    Hire,           // NPC hired by player
    PHire,          // NPC permanent hired by player
    Fire,           // NPC Dismissed or Duration Expired
    Stay,           // NPC asked to stay
    Follow,         // NPC asked to follow or after hire
    Buffed,         // NPC received a positive spell buff
    HealTarget,     // NPC Heals target (not self)
    HealSelf,       // NPC Heals self
    Resurrect,      // NPC Resurrects the player
    PlayerOrMemberDead,     // NPC reacts to dead player or bot
    SpellOnTarget,  // NPC reacts when casting combat spell on target
    CombatMana,     // NPC IC OOM and uses mana potion
    CombatHealth,
    BotDeath,       // NPC Dies
    TargetDeath,    // NPC Kills Target
    BuffTarget,
    BuffSelf,
    OOCRecovery
};

namespace FSBUtilsTexts
{
    // Converts an int64 price in copper to a string like 10 silver
    std::string MoneyToString(int64 price);

    // Builds NPC say text dynamically, inserts placeholders
    std::string BuildNPCSayText(
        const std::string& playerName,
        uint32 duration,
        FSBSayType type,
        const std::string& string2 = std::string()
    );

    void ReplaceAll(std::string& text, const std::string& from, const std::string& to);

    // Builds the hire option text dynamically
    std::string BuildHireText(int64 price, uint32 hours);

    void OnKilledTargetSay(Creature* creature, Unit* victim);
}
