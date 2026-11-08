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

#include "Creature.h"
#include "EventProcessor.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "TemporarySummon.h"

#include "Custom_VaultWardens_Defines.h"

namespace Scripts::Custom::VaultOfTheWardens
{
    using namespace Creatures;
    using namespace Quests;
    using namespace Texts;
    using namespace Positions;

    // 38672 - Breaking Out
    // Bound through quest_template_addon.ScriptName - only fires for this quest.
    class quest_vault_wardens_breaking_out : public QuestScript
    {
    public:
        quest_vault_wardens_breaking_out() : QuestScript("quest_vault_wardens_breaking_out") { }

        void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
        {
            // every status transition re-evaluates the area conditions
            // (cell phases 5964/5965, freed phases 5124/5125, Maiev/ward phases)
            PhasingHandler::OnConditionChange(player);

            if (newStatus == QUEST_STATUS_NONE)
            {
                // abandoning mid-scene: remove leftover personal clones so the
                // static/tracked spawns take over again (cell phases re-enable)
                for (uint32 entry : { AltruisCell, KaynCell, AltruisTheSufferer, KaynSunfury, MaievShadowsong, WardOfTheHunters, AltruisTheSuffererPostReward, KaynSunfuryPostReward })
                {
                    std::list<Creature*> creatures;
                    player->GetCreatureListWithEntryInGrid(creatures, entry, 150.0f);
                    for (Creature* creature : creatures)
                        if (creature->IsPrivateObject() && creature->GetPrivateObjectOwner() == player->GetGUID())
                            creature->DespawnOrUnsummon();
                }
            }
            else if (newStatus == QUEST_STATUS_COMPLETE)
            {
                // the freed Illidari react (creature_text group 1); the static
                // rally spawns talk, so private objects are excluded
                if (Creature* kayn = player->FindNearestCreatureWithOptions(40.0f, { .CreatureId = KaynSunfury, .IgnorePrivateObjects = true }))
                    kayn->AI()->Talk(SayKaynRally, player); // "Why did you let THAT one free?"

                player->m_Events.AddEventAtOffset([player]()
                {
                    if (Creature* altruis = player->FindNearestCreatureWithOptions(40.0f, { .CreatureId = AltruisTheSufferer, .IgnorePrivateObjects = true }))
                        altruis->AI()->Talk(SayAltruisRally, player); // "It has been a long time, Kayn..."
                }, 5s);
            }
            else if (newStatus == QUEST_STATUS_REWARDED)
            {
                // bonus quest enters the log with the reward (sniffed quest-log
                // update in the reward packet)
                if (Quest const* vaultQuest = sObjectMgr->GetQuestTemplate(VaultOfTheWardensQuest))
                    player->AddQuestAndCheckCompletion(vaultQuest, nullptr);

                // the ward static is already phased out by the condition refresh
                // above; its clone dissolves the sealed door for this player
                if (Creature* ward = player->FindNearestCreatureWithOptions(50.0f, { .CreatureId = WardOfTheHunters, .IgnorePhases = true, .IgnorePrivateObjects = true }))
                    ward->SummonPersonalClone(ward->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, player);

                // post-reward clones replace the main-hall questgiver statics at
                // the rally point, then run to them; the statics reappear on
                // clone despawn (sniffed ReplaceObject swap)
                if (Creature* kayn = player->FindNearestCreatureWithOptions(60.0f, { .CreatureId = KaynSunfuryPostReward, .IgnorePrivateObjects = true }))
                    kayn->SummonPersonalClone(KaynPostRewardSpawn, TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, player);

                if (Creature* altruis = player->FindNearestCreatureWithOptions(60.0f, { .CreatureId = AltruisTheSuffererPostReward, .IgnorePrivateObjects = true }))
                    altruis->SummonPersonalClone(AltruisPostRewardSpawn, TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, player);
            }
        }
    };

    // 38689 - Fel Infusion
    // Bound through quest_template_addon.ScriptName - only fires for this quest.
    // Retail applies the invisible aura 133508 on accept - it is the gate for
    // the soul-absorb sweep (spell_vault_fel_infusion). The sniffed completion
    // sequence removes it in the same tick as SMSG_QUEST_UPDATE_COMPLETE
    // (aura slot cleared + alternate Power/MaxPower zeroed), so it only lives
    // while the quest is INCOMPLETE.
    class quest_vault_wardens_fel_infusion : public QuestScript
    {
    public:
        quest_vault_wardens_fel_infusion() : QuestScript("quest_vault_wardens_fel_infusion") { }

        void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
        {
            if (newStatus != QUEST_STATUS_INCOMPLETE)
                player->RemoveAurasDueToSpell(Spells::FelInfusionAura);
            else if (!player->HasAura(Spells::FelInfusionAura))
            {
                player->CastSpell(player, Spells::FelInfusionAura, true);
                // 133509 fires once as a child of the accept cast on retail -
                // energizes 0, it only brings up the empty alternate power bar
                player->CastSpell(player, Spells::FillFelBar, true);
            }
        }
    };
}

void AddSC_custom_vault_wardens_quests()
{
    using namespace Scripts::Custom::VaultOfTheWardens;

    new quest_vault_wardens_breaking_out();
    new quest_vault_wardens_fel_infusion();
}
