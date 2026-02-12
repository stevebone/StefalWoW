#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"

#include "Followship_bots_warlock.h"

#include "Followship_bots_movement_handler.h"

namespace FSBMovement
{
    void BotSetMountedState(Creature* bot, bool& botMounted)
    {
        if (!bot || !bot->IsAlive())
            return;

        if (bot->HasUnitState(UNIT_STATE_CASTING))
            return;

        Player* player = FSBMgr::GetBotOwner(bot);

        if (!player)
            return;

        if (player->HasAuraType(SPELL_AURA_MOUNTED) && !botMounted)
        {
            uint32 randomSpell = FSBSpellsUtils::GetRandomMountSpellForBot(bot);
            FSB_Class cls = FSBUtils::GetBotClassForEntry(bot->GetEntry());

            if (cls == FSB_Class::Warlock)
            {
                std::vector<uint32> warlockMounts =
                {
                    randomSpell,
                    SPELL_WARLOCK_FELSTEED,
                    SPELL_WARLOCK_DREADSTEED
                };

                uint32 spellId = warlockMounts[urand(0, warlockMounts.size() - 1)];
                if (FSBSpellsUtils::BotCastMountSpell(bot, spellId))
                    botMounted = true;
            }
            else if (cls == FSB_Class::Paladin)
            {
                //uint32 spellId = RAND(randomSpell);
                //if (FSBSpellsUtils::BotCastMountSpell(bot, spellId))
                //    botMounted = true;
            }
            else
            {
                if (FSBSpellsUtils::CastRandomMountLevelSpell(bot))
                    botMounted = true;
            } 
        }
        else if (!player->HasAuraType(SPELL_AURA_MOUNTED) && botMounted)
        {
            botMounted = false;
            bot->Dismount();
            bot->RemoveAurasByType(SPELL_AURA_MOUNTED);
        }
    }
}
