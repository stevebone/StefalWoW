constexpr uint32 RANDOM_EVENT_INTERVAL = 120000;
constexpr uint32 SPAWN_COMPANION_CHANCE = 30;
constexpr uint32 RANDOM_AFK_EVENT_CHANCE = 10;


enum FSB_AFK_ACTIONS
{
    FSB_AFK_ACTION_MAKE_PICNIC,
    FSB_AFK_ACTION_MAKE_FIRE,
    FSB_AFK_ACTION_COOKING_POT,
    FSB_AFK_ACTION_COOK_SAUSAGES,
    FSB_AFK_ACTION_BEG,
    FSB_AFK_ACTION_KISS,
    FSB_AFK_ACTION_WHISTLE,
    FSB_AFK_ACTION_SLEEP,
    FSB_AFK_ACTION_REST,
    FSB_AFK_ACTION_TALK,
    FSB_AFK_ACTION_FLIRT,
    FSB_AFK_ACTION_JOKE,
    FSB_AFK_ACTION_DANCE,
    FSB_AFK_ACTION_SIGH,
    FSB_AFK_ACTION_WHISPER,
    FSB_AFK_ACTION_SHAMAN_WOLF,
    FSB_AFK_ACTION_COMPANION,

    FSB_AFK_ACTION_NOTHING, // yes we want a nothing action

    FSB_AFK_MAX_ACTIONS
};

namespace FSBOOC
{
    bool BotOOCActions(Creature* bot);
    bool BotOOCHealOwner(Creature* bot, Player* player, uint32& globalCooldown);
    bool BotOOCBuffGroup(Creature* bot, const std::vector<Unit*>& botGroup, uint32& buffTimer, uint32& globalCooldown);
    bool BotOOCBuffSelf(Creature* bot, uint32& selfBuffTimer, uint32& globalCooldown);
    bool BotOOCRecovery(Creature* bot, uint32& globalCooldown);
    bool BotOOCSummonPetOrDemon(Creature* bot);
    bool BotOOCBuffSoulstone(Creature* bot, uint32& globalCooldown, const std::vector<Unit*> botGroup);
    bool BotOOCResurrect(Creature* bot);
    bool BotOOCResurrectTarget(Creature* bot);
    bool BotOOCBuffBeacon(Creature* bot);
    bool BotOOCDoRandomEvent(Creature* bot);
    bool BotOOCSpawnCompanion(Creature* bot);
    bool BotOOCActionPlayerAFK(Creature* bot, bool force);
    bool BotOOCAFKAction(Creature* bot, uint8 action);

    void BotOOCClearFlagsStates(Creature* bot);

    void GetBotBuffTargets(Creature* bot, uint32 buffSpellId, const std::vector<Unit*>& botGroup, float maxRange, std::vector<Unit*>& outTargets);

    bool BotOOCActionCook(Creature* bot, uint32 spellId, uint32 goId);
    bool BotOOCActionFlirt(Creature* bot);
    bool BotOOCActionJoke(Creature* bot);
    bool BotOOCActionKiss(Creature* bot);
    bool BotOOCActionSigh(Creature* bot);
    bool BotOOCActionSleep(Creature* bot);
}
