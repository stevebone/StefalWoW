
#include "Followship_bots_utils.h"


constexpr float RAGE_FROM_DAMAGE_COEFF = 0.15f;

namespace FSBPowers
{
    Powers GetBotPowerType(Creature* bot);
    void UpdateMaxPower(Creature* bot, Powers power);

    bool SpendPowerPct(Creature* bot, float pct);
    bool SpendManaFlat(Creature* bot, int32 cost);

    bool IsRageUser(Creature* bot);
    bool IsEnergyUser(Creature* bot);

    void GenerateRageFromDamageTaken(Creature* bot, uint32 damage);
    void GenerateRageFromDamageDone(Creature* bot, uint32 damage);
}
