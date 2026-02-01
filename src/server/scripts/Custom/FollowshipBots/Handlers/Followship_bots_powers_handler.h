
#include "Followship_bots_utils.h"


constexpr float RAGE_FROM_DAMAGE_COEFF = 0.3f;

namespace FSBPowers
{
    Powers GetBotPowerType(Creature* bot);

    bool SpendManaPct(Creature* bot, float pct);
    bool SpendManaFlat(Creature* bot, int32 cost);

    bool IsRageUser(Creature* bot);
    void GenerateRageFromDamageTaken(Creature* bot, uint32 damage);
    void GenerateRageFromDamageDone(Creature* bot, uint32 damage);
}
