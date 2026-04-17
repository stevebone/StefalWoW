/*
 * This file is part of the Stefal WoW Project.
 * It is designed to work exclusively with the TrinityCore framework.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * This code is provided for personal and educational use within the
 * Stefal WoW Project. It is not intended for commercial distribution,
 * resale, or any form of monetization.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "PhasingHandler.h"

#include "Custom_TheWanderingIsle_Defines.h"

namespace Scripts::Custom::TheWanderingIsle
{
    // 29422
    class quest_29422_huo_the_spirit_of_fire : public QuestScript
    {
    public:
        quest_29422_huo_the_spirit_of_fire() : QuestScript("quest_29422_huo_the_spirit_of_fire") {}

        void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
        {
            switch (newStatus)
            {
            case QUEST_STATUS_NONE:
            case QUEST_STATUS_INCOMPLETE:
            case QUEST_STATUS_COMPLETE:
                PhasingHandler::OnConditionChange(player);
                break;
            default:
                break;
            }
        }
    };

    // 29423
    class quest_29423_the_passion_of_shen_zin_su : public QuestScript
    {
    public:
        quest_29423_the_passion_of_shen_zin_su() : QuestScript("quest_29423_the_passion_of_shen_zin_su") {}

        void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
        {
            if (newStatus == QUEST_STATUS_NONE)
            {
                player->CastSpell(player, Spells::spell_despawn_spirit_of_fire, true);
                player->RemoveAurasDueToSpell(Spells::spell_summon_spirit_of_fire);
                player->RemoveAurasDueToSpell(Spells::spell_summon_spirit_of_fire_on_relog);
                PhasingHandler::OnConditionChange(player);
            }
            else if (newStatus == QUEST_STATUS_INCOMPLETE)
            {
                player->CastSpell(player, Spells::spell_summon_spirit_of_fire, true);
                PhasingHandler::OnConditionChange(player);
            }
        }
    };

    // 29679
    class quest_29679_a_new_friend : public QuestScript
    {
    public:
        quest_29679_a_new_friend() : QuestScript("quest_29679_a_new_friend") { }

        void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
        {
            if (newStatus == QUEST_STATUS_COMPLETE)
            {
                player->CastSpell(player, Spells::spell_aysa_congrats_timer);
                player->CastSpell(player, Spells::spell_aysa_congrats_trigger_aura);
                player->CastSpell(player, Spells::spell_summon_spirit_of_water);
            }
        }
    };

    // 29785
    class quest_29785_dafeng_the_spririt_of_air : public QuestScript
    {
    public:
        quest_29785_dafeng_the_spririt_of_air() : QuestScript("quest_29785_dafeng_the_spririt_of_air") {}

        void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
        {
            if (newStatus == QUEST_STATUS_REWARDED)
                PhasingHandler::OnConditionChange(player, true);
        }
    };

    // 29786
    class quest_29786_battle_for_the_skies : public QuestScript
    {
    public:
        quest_29786_battle_for_the_skies() : QuestScript("quest_29786_battle_for_the_skies") {}

        void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
        {
            if (newStatus == QUEST_STATUS_INCOMPLETE)
            {
                PhasingHandler::OnConditionChange(player, true);

                Creature* dafeng = player->FindNearestCreature(Npcs::npc_dafeng_q29785, 10.f);
                if (dafeng)
                    dafeng->DespawnOrUnsummon(2s);

                Creature* aysa = player->FindNearestCreature(Npcs::npc_aysa_q29785, 10.f);
                if (aysa)
                    aysa->DespawnOrUnsummon(2s);
            }

            if(newStatus == QUEST_STATUS_COMPLETE)
                PhasingHandler::OnConditionChange(player, true);
        }
    };

    // 29787
    class quest_29787_worthy_of_passing : public QuestScript
    {
    public:
        quest_29787_worthy_of_passing() : QuestScript("quest_29787_worthy_of_passing") {}

        void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
        {
            if (newStatus == QUEST_STATUS_REWARDED)
                PhasingHandler::OnConditionChange(player, true);
        }
    };

    // 29790 & 29791
    class quest_29790_passing_wisdom_29791 : public QuestScript
    {
    public:
        quest_29790_passing_wisdom_29791() : QuestScript("quest_29790_passing_wisdom_29791") {}

        void OnQuestStatusChange(Player* player, Quest const* quest, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
        {
            if(quest->GetQuestId() == Quests::quest_passing_wisdom)
                if (newStatus == QUEST_STATUS_COMPLETE)
                    PhasingHandler::OnConditionChange(player, true);

            if(quest->GetQuestId() == Quests::quest_suffering_of_shenzinsu)
                if (newStatus == QUEST_STATUS_INCOMPLETE)
                {
                    Creature* balloon = player->FindNearestCreatureWithOptions(20.f, { .CreatureId = Npcs::npc_balloon_q29791, .IgnorePhases = true } );
                    if(balloon)
                        balloon->RemoveUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
                }
        }
    };
}

void AddSC_custom_the_wandering_isle_quests()
{
    using namespace Scripts::Custom::TheWanderingIsle;
    new quest_29422_huo_the_spirit_of_fire();
    new quest_29423_the_passion_of_shen_zin_su();
    new quest_29679_a_new_friend();
    new quest_29785_dafeng_the_spririt_of_air();
    new quest_29786_battle_for_the_skies();
    new quest_29787_worthy_of_passing();
    new quest_29790_passing_wisdom_29791();
}
