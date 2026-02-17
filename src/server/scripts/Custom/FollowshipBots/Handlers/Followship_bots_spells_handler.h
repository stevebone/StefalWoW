
namespace FSBSpells
{
    void HandleOnSpellCast(Creature* bot, FSB_Class botClass, uint32 spellId);
    void HandleOnSpellCastGeneric(Creature*bot, uint32 spellId);

    bool BotTryDispel(Creature* bot);
    bool BotTryOffensiveDispel(Creature* bot);

    bool BotCastSpell(Creature* bot, uint32 spellId, Unit* target);
    bool BotCastSpellWithCooldown(Creature* bot, uint32 spellId, Unit* target, uint32 cooldown);
}
