
#include "Followship_bots_utils.h"

namespace FSBRegen
{
    void ApplyBotRegen(Unit* unit, FSB_Class botClass, const FSBUtilsStatsMods& mods, bool doHealth, bool doMana);
    void ProcessBotCustomRegenTick(Creature* creature, FSB_Class botClass, const FSBUtilsStatsMods& _baseStatsMods, const FSBUtilsStatsMods& _statsMods);
}
