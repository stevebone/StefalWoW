namespace FSBCombat
{
    void EvaluateAttackNeeded(Creature* bot);
    Unit* GetNextAttackTarget(Creature* bot);
    bool BotCanAttack(Creature* bot, Unit* target);
    void BotDoAttack(Creature* bot, Unit* target);

    bool ShouldSwitchToMelee(Creature* bot);
    void EnterMeleeMode(Creature* bot);
}
