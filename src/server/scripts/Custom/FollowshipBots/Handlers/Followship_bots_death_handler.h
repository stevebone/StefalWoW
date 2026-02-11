
namespace FSBDeath
{
    void HandlerJustDied(Creature* bot, const std::vector<Unit*>& botGroup, bool hasSS);

    void HandleDeathWithSoulstone(Creature* bot, bool& hasSS);
    void HandleDeathWithGraveyard(Creature* bot, Position botCorpse);
    void HandleDeathInDungeon(Creature* bot, float fDistance, float fAngle);
    void BotSetStateAfterCorpseRevive(Creature* bot);
}
