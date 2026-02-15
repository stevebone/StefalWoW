
namespace FSBGroup
{
    // Builds a list containing:
    // - owner (player)
    // - the calling bot (me)
    // - other bots owned by the same player
    void BuildLogicalBotGroup(Creature* bot, std::vector<Unit*>& outGroup);
    void CheckBotAllies(Unit* me, std::vector<Unit*>& outGroup, float searchRange = 100.0f);

    Unit* BotGetFirstGroupHealer(const std::vector<Unit*>& botGroup);
    Unit* BotGetFirstGroupTank(const std::vector<Unit*>& botGroup);
    Unit* BotGetFirstDeadMember(const std::vector<Unit*>& botGroup);
}
