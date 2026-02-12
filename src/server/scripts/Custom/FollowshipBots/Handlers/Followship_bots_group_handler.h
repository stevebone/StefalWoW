
namespace FSBGroup
{
    // Builds a list containing:
    // - owner (player)
    // - the calling bot (me)
    // - other bots owned by the same player
    void CheckBotAllies(Unit* me, std::vector<Unit*>& outGroup, float searchRange = 100.0f);

    Unit* BotGetFirstGroupHealer(const std::vector<Unit*>& group);
}
