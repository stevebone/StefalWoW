
#include "Followship_bots_config.h"
#include "Followship_bots_utils.h"
#include "Followship_bots_priest.h"
#include "Followship_bots_utils_spells.h"


constexpr float BOT_RECOVERY_HP_PCT = 70.f;
constexpr float BOT_RECOVERY_MP_PCT = 70.f;


enum BotRecoverySpells
{
    SPELL_DRINK_CONJURED_CRYSTAL_WATER = 22734,
    SPELL_FOOD_SCALED_WITH_LVL = 1131,

    // Human spells
    SPELL_HUMAN_RECUPERATE = 1231418,

    // Mage spells
    SPELL_MAGE_CONJURED_MANA_PUDDING = 167152
};

enum class BotRecoveryIntent
{
    None,
    RecoverHealth,
    RecoverMana,
    RecoverHealthAndMana
};

enum class BotRecoveryType
{
    Health,
    Mana,
    Both
};

struct BotRecoveryAction
{
    BotRecoveryType type;
    uint8 priority = 0; // higher = preferred

    virtual ~BotRecoveryAction() = default;
    virtual uint32 GetCooldownMs() const { return 1500; }

    virtual bool CanExecute(Creature* bot, uint32 cooldown) const = 0;
    virtual void Execute(Creature* bot, uint32& _globalCooldownUntil) = 0;
};

struct ActionEatFood : BotRecoveryAction
{
    ActionEatFood()
    {
        type = BotRecoveryType::Health;
        priority = 50;
    }

    uint32 GetCooldownMs() const override
    {
        return 30000; // drinking feels slower than a heal
    }

    bool CanExecute(Creature* bot, uint32 cooldown) const override
    {
        uint32 now = getMSTime();

        return bot->GetPowerPct(POWER_MANA) < BOT_RECOVERY_HP_PCT
            && !bot->isMoving()
            && !bot->IsInCombat()
            && FSBUtilsSpells::CanCastNow(bot, now, cooldown)
            && !bot->HasAura(SPELL_FOOD_SCALED_WITH_LVL);
    }

    void Execute(Creature* bot, uint32& _globalCooldownUntil) override
    {
        uint32 now = getMSTime();

        FSBUtilsMovement::StopFollow(bot);

        bot->CastSpell(bot, SPELL_FOOD_SCALED_WITH_LVL, true);

        _globalCooldownUntil = now + GetCooldownMs(); // set cooldown to 30s to not interrup the drink spell which lasts 30 seconds max

        if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
        {
            std::string pname = "";
            Player* player = bot->GetOwner()->ToPlayer();
            if (player)
                pname = player->GetName();

            std::string msg = FSBUtilsTexts::BuildNPCSayText(pname, NULL, FSBSayType::OOCRecovery, FSBUtilsSpells::GetSpellName(SPELL_FOOD_SCALED_WITH_LVL));
            bot->Say(msg, LANG_UNIVERSAL);
        }
    }
};

struct ActionDrinkMana : BotRecoveryAction
{
    ActionDrinkMana()
    {
        type = BotRecoveryType::Mana;
        priority = 50;
    }

    uint32 GetCooldownMs() const override
    {
        return 30000; // drinking feels slower than a heal
    }

    bool CanExecute(Creature* bot, uint32 cooldown) const override
    {
        uint32 now = getMSTime();

        return bot->GetPowerPct(POWER_MANA) < BOT_RECOVERY_MP_PCT
            && !bot->isMoving()
            && !bot->IsInCombat()
            && FSBUtilsSpells::CanCastNow(bot, now, cooldown)
            && !bot->HasAura(SPELL_DRINK_CONJURED_CRYSTAL_WATER);
    }

    void Execute(Creature* bot, uint32& _globalCooldownUntil) override
    {
        uint32 now = getMSTime();

        FSBUtilsMovement::StopFollow(bot);

        bot->CastSpell(bot, SPELL_DRINK_CONJURED_CRYSTAL_WATER, true);

        _globalCooldownUntil = now + GetCooldownMs(); // set cooldown to 30s to not interrup the drink spell which lasts 30 seconds max

        if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
        {
            std::string pname = "";
            Player* player = bot->GetOwner()->ToPlayer();
            if (player)
                pname = player->GetName();

            std::string msg = FSBUtilsTexts::BuildNPCSayText(pname, NULL, FSBSayType::OOCRecovery, FSBUtilsSpells::GetSpellName(SPELL_DRINK_CONJURED_CRYSTAL_WATER));
            bot->Say(msg, LANG_UNIVERSAL);
        }
    }
};

struct ActionMageConjuredDrink : BotRecoveryAction
{
    ActionMageConjuredDrink()
    {
        type = BotRecoveryType::Both;
        priority = 80;
    }

    uint32 GetCooldownMs() const override
    {
        return 30000; // drinking feels slower than a heal
    }

    bool CanExecute(Creature* bot, uint32 cooldown) const override
    {
        uint32 now = getMSTime();

        return FSBUtils::GetBotClassForEntry(bot->GetEntry()) == FSB_Class::Mage
            && (bot->GetPowerPct(POWER_MANA) < BOT_RECOVERY_MP_PCT || bot->GetHealthPct() < BOT_RECOVERY_HP_PCT)
            && !bot->isMoving()
            && !bot->IsInCombat()
            && FSBUtilsSpells::CanCastNow(bot, now, cooldown)
            && !bot->HasAura(SPELL_MAGE_CONJURED_MANA_PUDDING);
    }

    void Execute(Creature* bot, uint32& _globalCooldownUntil) override
    {
        uint32 now = getMSTime();

        FSBUtilsMovement::StopFollow(bot);

        bot->CastSpell(bot, SPELL_MAGE_CONJURED_MANA_PUDDING, true);

        _globalCooldownUntil = now + GetCooldownMs(); // set cooldown to 30s to not interrup the drink spell which lasts 30 seconds max

        if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
        {
            std::string pname = "";
            Player* player = bot->GetOwner()->ToPlayer();
            if (player)
                pname = player->GetName();

            std::string msg = FSBUtilsTexts::BuildNPCSayText(pname, NULL, FSBSayType::OOCRecovery, FSBUtilsSpells::GetSpellName(SPELL_MAGE_CONJURED_MANA_PUDDING));
            bot->Say(msg, LANG_UNIVERSAL);
        }
    }
};

struct ActionHumanRecuperate : BotRecoveryAction
{
    ActionHumanRecuperate()
    {
        type = BotRecoveryType::Health;
        priority = 70;
    }

    uint32 GetCooldownMs() const override
    {
        return 10000; // drinking feels slower than a heal
    }

    bool CanExecute(Creature* bot, uint32 cooldown) const override
    {
        uint32 now = getMSTime();

        return FSBUtils::GetBotRaceForEntry(bot->GetEntry()) == FSB_Race::Human
            && bot->GetHealthPct() < BOT_RECOVERY_HP_PCT
            && !bot->isMoving()
            && !bot->IsInCombat()
            && FSBUtilsSpells::CanCastNow(bot, now, cooldown)
            && !bot->HasAura(SPELL_HUMAN_RECUPERATE);
    }

    void Execute(Creature* bot, uint32& _globalCooldownUntil) override
    {
        uint32 now = getMSTime();

        FSBUtilsMovement::StopFollow(bot);

        bot->CastSpell(bot, SPELL_HUMAN_RECUPERATE, false);

        _globalCooldownUntil = now + GetCooldownMs(); // set cooldown to 10s to not interrup the recuperate spell which lasts 10 seconds max

        if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
        {
            std::string pname = "";
            Player* player = bot->GetOwner()->ToPlayer();
            if (player)
                pname = player->GetName();

            std::string msg = FSBUtilsTexts::BuildNPCSayText(pname, NULL, FSBSayType::OOCRecovery, FSBUtilsSpells::GetSpellName(SPELL_HUMAN_RECUPERATE));
            bot->Say(msg, LANG_UNIVERSAL);
        }
    }
};

struct ActionPriestHeal : BotRecoveryAction
{
    ActionPriestHeal()
    {
        type = BotRecoveryType::Health;
        priority = 90;
    }

    bool CanExecute(Creature* bot, uint32 cooldown) const override
    {
        uint32 now = getMSTime();

        return FSBUtils::GetBotClassForEntry(bot->GetEntry()) == FSB_Class::Priest
            && bot->GetHealthPct() < BOT_RECOVERY_HP_PCT
            && !bot->isMoving()
            && !bot->IsInCombat()
            && FSBUtilsSpells::CanCastNow(bot, now, cooldown);
    }

    void Execute(Creature* bot, uint32& _globalCooldownUntil) override
    {
        uint32 now = getMSTime();

        bot->CastSpell(bot, SPELL_PRIEST_HEAL, false);

        _globalCooldownUntil = now + GetCooldownMs(); // GCD

        if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
        {
            std::string msg = FSBUtilsTexts::BuildNPCSayText("", NULL, FSBSayType::HealSelf, "");
            bot->Say(msg, LANG_UNIVERSAL);
        }
    }
};



namespace FSBRecovery
{
    using RecoveryActionList = std::vector<std::unique_ptr<BotRecoveryAction>>;

    BotRecoveryIntent DetermineRecoveryIntent(Creature* bot);
    bool MatchesIntent(BotRecoveryType type, BotRecoveryIntent intent);

    void BuildRecoveryActions(Creature* bot, RecoveryActionList& _recoveryActions);
    void TryRecovery(Creature* bot, RecoveryActionList& _recoveryActions, uint32 _globalCooldownUntil, bool& _isRecovering, uint32& _recoveryLockUntil);
    void TryRecover(Creature* bot, BotRecoveryIntent intent, RecoveryActionList& _recoveryActions, uint32 _globalCooldownUntil, bool& _isRecovering, uint32& _recoveryLockUntil);
}
