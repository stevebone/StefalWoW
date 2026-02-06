
namespace FSBOOC
{
    bool BotOOCActions(Creature* bot, uint32& globalCooldown, uint32& buffTimer, uint32& selfBuffTimer, std::vector<Unit*> botGroup);
    bool BotOOCHealOwner(Creature* bot, Player* player, uint32 globalCooldown);
    bool BotOOCBuffGroup(Creature* bot, std::vector<Unit*> botGroup, uint32& buffTimer, uint32& globalCooldown);
    bool BotOOCBuffSelf(Creature* bot, uint32& selfBuffTimer, uint32& globalCooldown);

    void GetBotBuffTargets(Creature* bot, uint32 buffSpellId, std::vector<Unit*> botGroup, float maxRange, std::vector<Unit*>& outTargets);
}
