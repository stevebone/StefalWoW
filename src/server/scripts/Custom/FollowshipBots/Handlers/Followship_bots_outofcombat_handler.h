
namespace FSBOOC
{
    bool BotOOCActions(Creature* bot, uint32& globalCooldown, uint32& buffTimer, uint32& selfBuffTimer, const std::vector<Unit*> botGroup, bool& demonDead, bool& botManaPotionUsed, bool& botHealthPotionUsed);
    bool BotOOCHealOwner(Creature* bot, Player* player, uint32& globalCooldown);
    bool BotOOCBuffGroup(Creature* bot, const std::vector<Unit*> botGroup, uint32& buffTimer, uint32& globalCooldown);
    bool BotOOCBuffSelf(Creature* bot, uint32& selfBuffTimer, uint32& globalCooldown);
    bool BotOOCRecovery(Creature* bot, uint32& globalCooldown);
    bool BotOOCSummonDemons(Creature* bot, uint32& globalCooldown, bool& demonDead);
    bool BotOOCBuffSoulstone(Creature* bot, uint32& globalCooldown, const std::vector<Unit*> botGroup);

    bool BotOOCClearCombatFlags(Creature* bot, bool& botManaPotionUsed, bool& botHealthPotionUsed);

    void GetBotBuffTargets(Creature* bot, uint32 buffSpellId, const std::vector<Unit*> botGroup, float maxRange, std::vector<Unit*>& outTargets);
}
