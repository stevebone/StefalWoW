
struct FSBAFKAction
{
    std::function<bool(Creature*)> action;
    uint8 chance; // 0-100
};

namespace FSBOOC
{
    bool BotOOCActions(FSB_BaseAI* ai/*, const std::vector<Unit*> botGroup*/);
    bool BotOOCHealOwner(Creature* bot, Player* player, uint32& globalCooldown);
    bool BotOOCBuffGroup(Creature* bot, const std::vector<Unit*>& botGroup, uint32& buffTimer, uint32& globalCooldown);
    bool BotOOCBuffSelf(Creature* bot, uint32& selfBuffTimer, uint32& globalCooldown);
    bool BotOOCRecovery(Creature* bot, uint32& globalCooldown);
    bool BotOOCSummonDemons(Creature* bot, uint32& globalCooldown, bool& botHasDemon);
    bool BotOOCBuffSoulstone(Creature* bot, uint32& globalCooldown, const std::vector<Unit*> botGroup);
    bool BotOOCResurrect(Creature* bot, ObjectGuid& resTargetGuid);
    bool BotOOCResurrectTarget(Creature* bot);
    bool BotOOCDoRandomEvent(Creature* bot);
    bool BotOOCSpawnCompanion(Creature* bot);
    bool BotOOCActionPlayerAFK(Creature* bot);

    bool BotOOCClearFlagsStates(Creature* bot);

    void GetBotBuffTargets(Creature* bot, uint32 buffSpellId, const std::vector<Unit*>& botGroup, float maxRange, std::vector<Unit*>& outTargets);
}
