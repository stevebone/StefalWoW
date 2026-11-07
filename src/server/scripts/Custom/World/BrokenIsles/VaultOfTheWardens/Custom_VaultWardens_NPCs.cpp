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
#include "CreatureAI.h"
#include "GossipDef.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "TaskScheduler.h"
#include "TemporarySummon.h"

#include "Custom_VaultWardens_Defines.h"

namespace Scripts::Custom::VaultOfTheWardens
{
    using namespace Creatures;
    using namespace Quests;
    using namespace Texts;
    using namespace AnimKits;
    using namespace Paths;
    using namespace Waypoints;
    using namespace Positions;

    // 92718 - Maiev Shadowsong (Vault of Silence cell, phase 5121)
    struct npc_maiev_shadowsong_vault_silence : public ScriptedAI
    {
        npc_maiev_shadowsong_vault_silence(Creature* creature) : ScriptedAI(creature) { }

        void OnQuestAccept(Player* player, Quest const* quest) override
        {
            if (quest->GetQuestId() != BreakingOut)
                return;

            PhasingHandler::OnConditionChange(player);

            Creature* maievObject = GetClosestCreatureWithOptions(player, 10.0f, { .CreatureId = MaievShadowsong, .IgnorePhases = true });
            Creature* wardObject = GetClosestCreatureWithOptions(player, 40.0f, { .CreatureId = WardOfTheHunters, .IgnorePhases = true });
            if (!maievObject || !wardObject)
                return;

            TempSummon* maievClone = maievObject->SummonPersonalClone(maievObject->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, player);
            TempSummon* wardClone = wardObject->SummonPersonalClone(wardObject->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, player);
            if (!maievClone || !wardClone)
                return;
        }
    };

    // 92718 - Maiev Shadowsong (personal clone, freed scene)
    struct npc_maiev_shadowsong_vault_silence_private : public ScriptedAI
    {
        npc_maiev_shadowsong_vault_silence_private(Creature* creature) : ScriptedAI(creature) { }

        void JustAppeared() override
        {
            Unit* privateObjectOwner = ObjectAccessor::GetUnit(*me, me->GetPrivateObjectOwner());
            if (!privateObjectOwner)
                return;

            Talk(SayMaievFreed, privateObjectOwner);

            // strip the questgiver flag after the create block has gone out so the
            // client doesn't show the clone as a quest giver
            _scheduler.Schedule(500ms, [this](TaskContext& /*task*/)
            {
                me->ReplaceAllNpcFlags(UNIT_NPC_FLAG_NONE);
                //me->RemoveNpcFlag(UNIT_NPC_FLAG_QUESTGIVER);
                me->UpdateNearbyPlayersInteractions();
            });

            _scheduler.Schedule(1s, [this](TaskContext& /*task*/)
            {
                me->GetMotionMaster()->MovePath(MaievVaultOfSilenceFreed, false);
                me->DespawnOrUnsummon(20s);
            });
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        TaskScheduler _scheduler;
    };

    CreatureAI* MaievVaultOfSilenceAISelector(Creature* creature)
    {
        if (creature->IsPrivateObject())
            return new npc_maiev_shadowsong_vault_silence_private(creature);
        return new npc_maiev_shadowsong_vault_silence(creature);
    }

    // 100168 - Ward of the Hunters (public spawn, closed door blocker)
    struct npc_ward_of_the_hunters_vault : public ScriptedAI
    {
        npc_ward_of_the_hunters_vault(Creature* creature) : ScriptedAI(creature) { }
    };

    // 100168 - Ward of the Hunters (personal clone, plays the door-open anim)
    struct npc_ward_of_the_hunters_vault_private : public ScriptedAI
    {
        npc_ward_of_the_hunters_vault_private(Creature* creature) : ScriptedAI(creature) { }

        void JustAppeared() override
        {
            Player* privateObjectOwner = ObjectAccessor::GetPlayer(*me, me->GetPrivateObjectOwner());

            if (privateObjectOwner && privateObjectOwner->GetQuestStatus(BreakingOut) == QUEST_STATUS_REWARDED)
            {
                // reward-time clone: dissolves the sealed door while the ward
                // gameobject takes over (sniffed SMSG_PLAY_ONE_SHOT_ANIM_KIT)
                me->PlayOneShotAnimKitId(WardDissolve);
                me->DespawnOrUnsummon(1900ms);
                return;
            }

            _scheduler.Schedule(1s, [this](TaskContext& /*task*/)
            {
                me->SetAIAnimKitId(WardOpen);
            });

            me->DespawnOrUnsummon(7s);
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        TaskScheduler _scheduler;
    };

    CreatureAI* WardOfTheHuntersVaultAISelector(Creature* creature)
    {
        if (creature->IsPrivateObject())
            return new npc_ward_of_the_hunters_vault_private(creature);
        return new npc_ward_of_the_hunters_vault(creature);
    }

    // 103655 / 103658 - Altruis's / Kayn's Cell (public spawn, spellclick frees the prisoner)
    struct npc_vault_prison_cell : public ScriptedAI
    {
        npc_vault_prison_cell(Creature* creature) : ScriptedAI(creature) { }

        void OnSpellClick(Unit* clicker, bool spellClickHandled) override
        {
            Player* player = clicker->ToPlayer();
            if (!player || !spellClickHandled)
                return;

            ObjectGuid playerGuid = player->GetGUID();
            if (!_pendingPlayers.insert(playerGuid).second)
                return; // this player's click is already in flight

            // the unlock cast takes ~0.8s; the clones and the kill credit land when it finishes
            _scheduler.Schedule(800ms, [this, playerGuid](TaskContext& /*task*/)
            {
                _pendingPlayers.erase(playerGuid); // re-armed once the cast-end task runs
                Player* clickerPlayer = ObjectAccessor::GetPlayer(*me, playerGuid);
                if (!clickerPlayer)
                    return;

                // cell overlay clone - plays the cage-open anim while the static cell is hidden
                me->SummonPersonalClone(me->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, clickerPlayer);

                bool altruisCell = me->GetEntry() == AltruisCell;
                // the freed clone spawns inside the cell and ReplaceObjects the
                // freed-phase rally static - retail has no in-cell prisoner spawns
                if (Creature* rallyPrisoner = GetClosestCreatureWithOptions(me, 60.0f, { .CreatureId = altruisCell ? AltruisTheSufferer : KaynSunfury, .IgnorePhases = true, .IgnorePrivateObjects = true }))
                {
                    rallyPrisoner->SummonPersonalClone(altruisCell ? AltruisCellPrisoner : KaynCellPrisoner, TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, clickerPlayer);
                    clickerPlayer->KilledMonsterCredit(altruisCell ? KillCredits::AltruisFreed : KillCredits::KaynFreed);
                    // the freed objective credit drops the cell's phase (5964/5965)
                    PhasingHandler::OnConditionChange(clickerPlayer);
                }
            });
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        TaskScheduler _scheduler;
        GuidUnorderedSet _pendingPlayers;
    };

    // 103655 / 103658 - personal clone: the opened-cage visual over the hidden static cell
    struct npc_vault_prison_cell_private : public ScriptedAI
    {
        npc_vault_prison_cell_private(Creature* creature) : ScriptedAI(creature) { }

        void JustAppeared() override
        {
            _scheduler.Schedule(1500ms, [this](TaskContext& /*task*/)
            {
                me->SetAIAnimKitId(CellOpen);
            });

            // the clone only plays the opening animation; the static cell is
            // already phased out for the clicking player by the freed objective
            me->DespawnOrUnsummon(15s);
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        TaskScheduler _scheduler;
    };

    CreatureAI* PrisonCellVaultAISelector(Creature* creature)
    {
        if (creature->IsPrivateObject())
            return new npc_vault_prison_cell_private(creature);
        return new npc_vault_prison_cell(creature);
    }

    // 99632 / 99631 - Altruis / Kayn (caged public spawn + freed public rally spawn)
    struct npc_vault_freed_illidari : public ScriptedAI
    {
        npc_vault_freed_illidari(Creature* creature) : ScriptedAI(creature) { }
    };

    // 99632 / 99631 - personal clone: walks out of the cell, picks up the glaives,
    // moves to the rally point where the public freed spawn takes over
    struct npc_vault_freed_illidari_private : public ScriptedAI
    {
        npc_vault_freed_illidari_private(Creature* creature) : ScriptedAI(creature) { }

        void JustAppeared() override
        {
            Unit* privateObjectOwner = ObjectAccessor::GetUnit(*me, me->GetPrivateObjectOwner());
            if (!privateObjectOwner)
                return;

            // strip the template-addon aura: the glaives are picked up at the kneel
            me->RemoveAura(Spells::GlaiveAnimReplacement);

            _scheduler.Schedule(4s, [this, ownerGuid = privateObjectOwner->GetGUID()](TaskContext& /*task*/)
            {
                Talk(me->GetEntry() == AltruisTheSufferer ? SayAltruisFreed : SayKaynFreed, ObjectAccessor::GetUnit(*me, ownerGuid));
                me->GetMotionMaster()->MovePath(me->GetEntry() == AltruisTheSufferer ? AltruisToWeapons : KaynToWeapons, false);
            });
        }

        void MovementInform(uint32 type, uint32 nodeId) override
        {
            if (type != WAYPOINT_MOTION_TYPE)
                return;

            bool altruis = me->GetEntry() == AltruisTheSufferer;

            if (!_reachedWeapons)
            {
                // the multi-node paths fire per node; only the warglaive pickup
                // spot gets the kneel and the weapon objective credit
                if (nodeId != (altruis ? AltruisToWeaponsLastNode : KaynToWeaponsLastNode))
                    return;

                _reachedWeapons = true;
                me->HandleEmoteCommand(EMOTE_ONESHOT_KNEEL);
                if (Player* owner = ObjectAccessor::GetPlayer(*me, me->GetPrivateObjectOwner()))
                    owner->KilledMonsterCredit(altruis ? KillCredits::AltruisWeapons : KillCredits::KaynWeapons);
                _scheduler.Schedule(4s, [this](TaskContext& /*task*/)
                {
                    // kneel done, glaives picked up - sniffed self-cast of the anim aura
                    me->CastSpell(me, Spells::GlaiveAnimReplacement, true);
                    me->GetMotionMaster()->MovePath(me->GetEntry() == AltruisTheSufferer ? AltruisToRally : KaynToRally, false);
                });
                return;
            }

            if (nodeId != ToRallyLastNode)
                return;

            // the End objective credit flips the phase-area condition: the public
            // freed spawn takes over as this clone despawns (sniffed phase swap)
            if (Player* owner = ObjectAccessor::GetPlayer(*me, me->GetPrivateObjectOwner()))
            {
                owner->KilledMonsterCredit(altruis ? KillCredits::AltruisEnd : KillCredits::KaynEnd);
                PhasingHandler::OnConditionChange(owner);
            }
            me->DespawnOrUnsummon(1s);
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        TaskScheduler _scheduler;
        bool _reachedWeapons = false;
    };

    CreatureAI* FreedIllidariVaultAISelector(Creature* creature)
    {
        if (creature->IsPrivateObject())
            return new npc_vault_freed_illidari_private(creature);
        return new npc_vault_freed_illidari(creature);
    }

    // 97614-97622 - freed named Illidari (private, player-owned summons from
    // the 38690 warden cells - see go_vault_warden_cell).
    // TODO: sniffed per-NPC waypoint runs to the east-hall rally
    // (4323-4327, -344..-380) incl. combat vs 92782 Felstalkers (MOVID+41..+73)
    struct npc_vault_freed_named_illidari : public ScriptedAI
    {
        npc_vault_freed_named_illidari(Creature* creature) : ScriptedAI(creature) { }

        void JustAppeared() override
        {
            if (!me->IsPrivateObject())
                return;

            if (Unit* privateObjectOwner = ObjectAccessor::GetUnit(*me, me->GetPrivateObjectOwner()))
                Talk(SayNamedIllidariFreed, privateObjectOwner);
        }
    };

    CreatureAI* FreedNamedIllidariVaultAISelector(Creature* creature)
    {
        return new npc_vault_freed_named_illidari(creature);
    }

    // 92980 / 92986 - Kayn / Altruis (main-hall questgiver statics)
    struct npc_vault_postreward_illidari : public ScriptedAI
    {
        npc_vault_postreward_illidari(Creature* creature) : ScriptedAI(creature) { }

        // accepting the follow-up quest phases the static out (sniffed
        // -5366/+5495 for Kayn, -5367/+5494 for Altruis) and a personal clone
        // takes over to run into the hall battle
        void OnQuestAccept(Player* player, Quest const* quest) override
        {
            uint32 followup = me->GetEntry() == KaynSunfuryPostReward ? RiseOfTheIllidari
                : me->GetEntry() == AltruisTheSuffererPostReward ? FelInfusion : 0;
            if (quest->GetQuestId() != followup)
                return;

            me->SummonPersonalClone(me->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, player);

            // the core refresh is gated on QUEST_FLAGS_UPDATE_PHASESHIFT; the
            // phase_area conditions drive -5366/+5495 and -5367/+5494 here
            if (PhasingHandler::OnConditionChange(player))
                player->UpdateObjectVisibility();
        }
    };

    // 92980 / 92986 - personal clone: appears at the rally point when 38672 is
    // rewarded (ReplaceObject hides the static), runs to the main-hall door
    // where the public questgiver spawn takes over on despawn
    struct npc_vault_postreward_illidari_private : public ScriptedAI
    {
        npc_vault_postreward_illidari_private(Creature* creature) : ScriptedAI(creature)
        {
            // strip before AddToMap so the create packet carries no questgiver
            // flag - the client never queries status and never paints an icon
            me->ReplaceAllNpcFlags(UNIT_NPC_FLAG_NONE);
        }

        void JustAppeared() override
        {
            Unit* privateObjectOwner = ObjectAccessor::GetUnit(*me, me->GetPrivateObjectOwner());
            if (!privateObjectOwner)
            {
                me->DespawnOrUnsummon();
                return;
            }

            ObjectGuid ownerGuid = privateObjectOwner->GetGUID();

            if (Player* player = privateObjectOwner->ToPlayer())
            {
                // the accept clone spawns at the static's hall spot (~-548)
                // while the follow-up quest is active in the owner's log;
                // reward clones spawn at the rally point (~-583)
                uint32 followup = me->GetEntry() == KaynSunfuryPostReward ? RiseOfTheIllidari : FelInfusion;
                QuestStatus status = player->GetQuestStatus(followup);
                _acceptScene = status != QUEST_STATUS_NONE && status != QUEST_STATUS_REWARDED
                    && me->GetPositionY() > -560.0f;
            }

            if (_acceptScene)
            {
                // sniffed channel on the accept clone (keeps the glaive anim)
                me->CastSpell(me, Spells::GlaiveAnimReplacement, true);

                if (me->GetEntry() == KaynSunfuryPostReward)
                {
                    // sniffed ~0.26s after the accept, run starts at ~4.8s
                    _scheduler.Schedule(250ms, [this, ownerGuid](TaskContext& /*task*/)
                    {
                        Talk(SayKaynAccept, ObjectAccessor::GetUnit(*me, ownerGuid));
                    });
                    _scheduler.Schedule(4800ms, [this](TaskContext& /*task*/)
                    {
                        me->GetMotionMaster()->MovePath(KaynAccept, false);
                    });
                }
                else
                {
                    _scheduler.Schedule(250ms, [this, ownerGuid](TaskContext& /*task*/)
                    {
                        Talk(SayAltruisAccept, ObjectAccessor::GetUnit(*me, ownerGuid));
                    });
                    _scheduler.Schedule(5300ms, [this](TaskContext& /*task*/)
                    {
                        me->GetMotionMaster()->MovePath(AltruisAccept, false);
                    });
                }
                return;
            }

            if (me->GetEntry() == KaynSunfuryPostReward)
            {
                // sniffed ~150ms after the reward phase swap
                _scheduler.Schedule(150ms, [this, ownerGuid](TaskContext& /*task*/)
                {
                    Talk(SayKaynPostReward, ObjectAccessor::GetUnit(*me, ownerGuid));
                });
                _scheduler.Schedule(2300ms, [this](TaskContext& /*task*/)
                {
                    me->GetMotionMaster()->MovePath(KaynPostReward, false);
                });
            }
            else
            {
                _scheduler.Schedule(3900ms, [this](TaskContext& /*task*/)
                {
                    me->GetMotionMaster()->MovePath(AltruisPostReward, false);
                });
            }
        }

        void MovementInform(uint32 type, uint32 nodeId) override
        {
            if (type != WAYPOINT_MOTION_TYPE)
                return;

            if (_acceptScene)
            {
                // sniffed despawn ~0.15s after reaching the battle line
                uint32 lastNode = me->GetEntry() == KaynSunfuryPostReward ? KaynAcceptLastNode : AltruisAcceptLastNode;
                if (nodeId == lastNode)
                    me->DespawnOrUnsummon(200ms);
                return;
            }

            if (me->GetEntry() == KaynSunfuryPostReward)
            {
                if (nodeId != KaynPostRewardLastNode)
                    return;

                // sniffed ~0.8s linger on arrival, then the static takes over
                me->DespawnOrUnsummon(800ms);
                return;
            }

            if (nodeId != AltruisPostRewardLastNode)
                return;

            Unit* privateObjectOwner = ObjectAccessor::GetUnit(*me, me->GetPrivateObjectOwner());
            if (!privateObjectOwner)
                return;

            // sniffed ~0.5s after reaching the door, ~3.2s total linger
            ObjectGuid ownerGuid = privateObjectOwner->GetGUID();
            _scheduler.Schedule(500ms, [this, ownerGuid](TaskContext& /*task*/)
            {
                Talk(SayAltruisPostReward, ObjectAccessor::GetUnit(*me, ownerGuid));
            });
            me->DespawnOrUnsummon(3200ms);
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        TaskScheduler _scheduler;
        bool _acceptScene = false;
    };

    CreatureAI* PostRewardIllidariVaultAISelector(Creature* creature)
    {
        if (creature->IsPrivateObject())
            return new npc_vault_postreward_illidari_private(creature);
        return new npc_vault_postreward_illidari(creature);
    }
}

void AddSC_custom_vault_wardens_npcs()
{
    using namespace Scripts::Custom::VaultOfTheWardens;

    new FactoryCreatureScript<CreatureAI, &MaievVaultOfSilenceAISelector>("npc_maiev_shadowsong_vault_silence");
    new FactoryCreatureScript<CreatureAI, &WardOfTheHuntersVaultAISelector>("npc_ward_of_the_hunters_vault");
    new FactoryCreatureScript<CreatureAI, &PrisonCellVaultAISelector>("npc_vault_prison_cell");
    new FactoryCreatureScript<CreatureAI, &FreedIllidariVaultAISelector>("npc_vault_freed_illidari");
    new FactoryCreatureScript<CreatureAI, &FreedNamedIllidariVaultAISelector>("npc_vault_freed_named_illidari");
    new FactoryCreatureScript<CreatureAI, &PostRewardIllidariVaultAISelector>("npc_vault_postreward_illidari");
}
