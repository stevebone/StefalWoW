
namespace FSBDeath
{
    void HandlerJustDied(Creature* bot, const std::vector<Unit*>& botGroup, bool hasSS);

    void HandleDeathWithSoulstone(Creature* bot, bool& hasSS);
    void HandleDeathWithGraveyard(Creature* bot, Position botCorpse);
    void HandleDeathInDungeon(Creature* bot, float fDistance, float fAngle);
    void BotSetStateAfterCorpseRevive(Creature* bot);
    // Periodic check to determine if a member is dead
    bool CheckBotMemberDeath(Creature* bot);
    // Actions when bot is hit with resurrect spell
    void HandleSpellResurrection(Creature* bot, uint32 spellId);
    // Timed actions after bot is hit with resurrect spell
    void HandleSpellResurrectionDelayedAction(Creature* bot);
}
