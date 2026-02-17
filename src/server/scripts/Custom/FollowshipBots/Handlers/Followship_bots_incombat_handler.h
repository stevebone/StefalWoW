
constexpr float BOT_IC_THRESHOLD_POTION_MP = 20.0f;
constexpr float BOT_IC_THRESHOLD_POTION_HP = 10.0f;

namespace FSBIC
{
    bool BotICActions(Creature* bot, bool& botManaPotionUsed, bool& botHealthPotionUsed, uint32& globalCooldown, bool& botCastedCombatBuffs, const std::vector<Unit*>& botGroup);
    bool BotICPotions(Creature* bot, bool& botManaPotionUsed, bool& botHealthPotionUsed);
    bool BotICInitialBuffs(Creature* bot, uint32 globalCooldown, bool& botCastedCombatBuffs, const std::vector<Unit*>& botGroup);
    bool BotICResurrect(Creature* bot, ObjectGuid& resTargetGuid);
    bool BotICMeleeMode(Creature* bot);
    bool BotICTryDispel(Creature* bot);
    bool BotICTryOffensiveDispel(Creature* bot);
    bool BotICHealSelf(Creature* bot);
    bool BotICHealGroup(Creature* bot);
    bool BotICTryOffensiveSpell(Creature* bot);
}
