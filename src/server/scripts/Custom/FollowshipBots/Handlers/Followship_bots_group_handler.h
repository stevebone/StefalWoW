
namespace FSBGroup
{
    // Builds a list containing:
    // - owner (player)
    // - the calling bot (me)
    // - other bots owned by the same player
    void BuildLogicalBotGroup(Creature* bot, std::vector<Unit*>& outGroup);

    Unit* BotGetFirstGroupHealer(const std::vector<Unit*>& botGroup);
    Unit* BotGetFirstGroupTank(const std::vector<Unit*>& botGroup);
    Unit* BotGetFirstDeadMember(const std::vector<Unit*>& botGroup);
    Unit* BotGetDispelMember(Creature* bot, const DispelAbility& ability);
    std::vector<Unit*> BotGetMembersToHeal(const std::vector<Unit*>& group, float lowHpThreshold);
    float CalculateEmergencyPriority(Unit* unit);
    void SortEmergencyTargets(std::vector<Unit*>& targets);
}
