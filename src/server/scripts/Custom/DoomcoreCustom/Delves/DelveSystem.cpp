/*
 * DoomCore Scripts - Delve System
 * Handles entry NPC (gossip-based tier selection), exit GO (CMSG_DELVE_TELEPORT_OUT),
 * and DelveData UpdateField management.
 *
 * The "Enter Delve" NPC (212407) uses a standard SMSG_GOSSIP_MESSAGE with 11
 * tier options.  The client renders the tier-selection UI when it receives a
 * gossip menu whose addon has a non-zero LfgDungeonsID.  After the menu is
 * shown the client also fires CMSG_REQUEST_PARTY_ELIGIBILITY_FOR_DELVE_TIERS
 * which the handler in DelveHandler.cpp answers.
 *
 * Routing: NPC entry 212407 is shared across all delves. Each spawn has a
 * different GossipMenuID set via gossip_menu_addon SQL. OnGossipHello reads
 * the creature's DB gossip menu ID and looks up the correct DelveInfo.
 */

#include "DelveData.h"
#include "DelvePackets.h"
#include "ScriptMgr.h"
#include "Creature.h"
#include "DBCEnums.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "GossipDef.h"
#include "Log.h"
#include "NPCPackets.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "WorldSession.h"

enum DelveSystemConsts
{
    SENDER_DELVE_TIER = 1,
};

class npc_enter_delve : public CreatureScript
{
public:
    npc_enter_delve() : CreatureScript("npc_enter_delve") { }

    struct npc_enter_delveAI : public ScriptedAI
    {
        npc_enter_delveAI(Creature* creature) : ScriptedAI(creature) { }

        bool OnGossipHello(Player* player) override
        {
            if (!player || !player->GetSession())
                return true;

            // Look up which delve this NPC spawn routes to.
            // Primary: gossipMenuId from spawn/template data.
            // Fallback 1: map ID (if NPC is inside the delve instance).
            // Fallback 2: proximity to exit coordinates (if NPC is in overworld and gossipMenuId=0).
            uint32 gossipMenuId = me->GetGossipMenuId();
            DelveInfo const* info = GetDelveInfoByGossipMenu(gossipMenuId);
            if (!info)
                info = GetDelveInfoByMap(me->GetMapId());
            if (!info)
            {
                // Proximity fallback: fog wall NPC is always near the delve's overworld exit position
                float bestDistSq = 200.0f * 200.0f;
                for (uint32 i = 0; i < DELVE_TABLE_SIZE; ++i)
                {
                    float dx = me->GetPositionX() - DELVE_TABLE[i].ExitX;
                    float dy = me->GetPositionY() - DELVE_TABLE[i].ExitY;
                    float distSq = dx * dx + dy * dy;
                    if (distSq < bestDistSq)
                    {
                        bestDistSq = distSq;
                        info = &DELVE_TABLE[i];
                    }
                }
            }
            if (!info)
            {
                TC_LOG_ERROR("scripts.delve",
                    "npc_enter_delve: No DelveInfo found for GossipMenuID {} / MapID {} on NPC {} (pos {:.1f} {:.1f})",
                    gossipMenuId, me->GetMapId(), me->GetEntry(), me->GetPositionX(), me->GetPositionY());
                return true;
            }

            TC_LOG_DEBUG("scripts.delve", "npc_enter_delve: OnGossipHello - Player {} clicked NPC {} (GossipMenuID {} ? Map {})",
                player->GetName(), me->GetEntry(), gossipMenuId, info->MapId);

            player->PlayerTalkClass->ClearMenus();
            player->PlayerTalkClass->GetGossipMenu().SetMenuId(info->GossipMenuId);

            for (uint32 i = 0; i < MAX_DELVE_TIERS; ++i)
            {
                int32 gossipOptionId = info->FirstTierGossipOptionId - static_cast<int32>(i);
                player->PlayerTalkClass->GetGossipMenu().AddMenuItem(
                    gossipOptionId,                 // gossipOptionId
                    i,                              // orderIndex
                    GossipOptionNpc::None,          // optionNpc
                    TIER_NAMES[i],                  // optionText
                    0,                              // language
                    GossipOptionFlags::None,        // flags
                    {},                             // gossipNpcOptionId
                    0,                              // actionMenuId
                    0,                              // actionPoiId
                    false,                          // boxCoded
                    0,                              // boxMoney
                    "",                             // boxText
                    TIER_SPELL_IDS[i],              // spellId
                    {},                             // overrideIconId
                    SENDER_DELVE_TIER,              // sender
                    i                               // action (tier index)
                );
            }

            WorldPackets::NPC::GossipMessage gossipMessage;
            gossipMessage.GossipGUID      = me->GetGUID();
            gossipMessage.GossipID        = info->GossipMenuId;
            gossipMessage.LfgDungeonsID   = info->LfgDungeonsId;
            gossipMessage.BroadcastTextID = info->BroadcastTextId;

            for (uint32 i = 0; i < MAX_DELVE_TIERS; ++i)
            {
                auto& opt = gossipMessage.GossipOptions.emplace_back();
                opt.GossipOptionID = info->FirstTierGossipOptionId - static_cast<int32>(i);
                opt.OrderIndex     = i;
                opt.OptionNPC      = GossipOptionNpc::None;
                opt.Text           = TIER_NAMES[i];
                opt.SpellID        = TIER_SPELL_IDS[i];

                // TODO: compute real eligibility per-player (ilvl, achievements, etc.)
                opt.Status = GossipOptionStatus::Available;

                bool lowTier = (i < 3);
                auto const* previews = lowTier ? TREASURE_3_ITEMS : TREASURE_2_ITEMS;
                uint32 count = lowTier ? 3 : 2;
                for (uint32 t = 0; t < count; ++t)
                {
                    WorldPackets::NPC::TreasureItem ti;
                    ti.Type        = GossipOptionRewardType::Item;
                    ti.ID          = previews[t].ItemID;
                    ti.Quantity    = 1;
                    ti.ItemContext = previews[t].ItemContext;
                    opt.Treasure.Items.push_back(ti);
                }
            }

            player->PlayerTalkClass->GetInteractionData().StartInteraction(
                me->GetGUID(), PlayerInteractionType::Gossip);

            player->GetSession()->SendPacket(gossipMessage.Write());

            _delveInfo = info;

            TC_LOG_DEBUG("scripts.delve",
                "npc_enter_delve: Gossip tier menu sent (GossipID {}, LfgDungeonsID {}, {} tiers)",
                info->GossipMenuId, info->LfgDungeonsId, MAX_DELVE_TIERS);

            return true;
        }

        bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override
        {
            if (!player)
                return true;

            CloseGossipMenuFor(player);

            if (gossipListId >= MAX_DELVE_TIERS)
                return true;

            if (!_delveInfo)
                _delveInfo = GetDelveInfoByGossipMenu(me->GetGossipMenuId());

            if (!_delveInfo)
            {
                TC_LOG_ERROR("scripts.delve", "npc_enter_delve::OnGossipSelect: No DelveInfo for NPC {} GossipMenuID {}",
                    me->GetEntry(), me->GetGossipMenuId());
                return true;
            }

            TC_LOG_DEBUG("scripts.delve",
                "npc_enter_delve: Player {} selected Tier {} - teleporting to Map {}",
                player->GetName(), gossipListId + 1, _delveInfo->MapId);
            player->SendUpdateWorldState(WS_DELVE_TIER, gossipListId + 1);
            player->SendUpdateWorldState(WS_DELVE_IN_DELVE_FLAG, 2);
            player->SendUpdateWorldState(WS_DELVE_MAP_ID, _delveInfo->MapId);
            player->SendUpdateWorldState(WS_DELVE_TIER_SPELL, TIER_SPELL_IDS[gossipListId]);
            if (_delveInfo->UnknownWS26903)
                player->SendUpdateWorldState(WS_DELVE_UNKNOWN_26903, _delveInfo->UnknownWS26903);
            GetPendingDelveEntries()[player->GetGUID()] = { _delveInfo->MapId, gossipListId };
            player->TeleportTo(_delveInfo->MapId,
                _delveInfo->EntryX, _delveInfo->EntryY, _delveInfo->EntryZ, _delveInfo->EntryO,
                TELE_TO_SEAMLESS);

            return true;
        }

    private:
        DelveInfo const* _delveInfo = nullptr;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_enter_delveAI(creature);
    }
};

struct go_leave_delve : public GameObjectAI
{
    go_leave_delve(GameObject* go) : GameObjectAI(go) { }

    bool OnGossipHello(Player* player) override
    {
        if (!player)
            return true;
        DelveInfo const* info = GetDelveInfoByMap(player->GetMapId());

        TC_LOG_DEBUG("scripts.delve", "go_leave_delve: Player {} using Leave Delve portal (Map {})",
            player->GetName(), player->GetMapId());

        player->ClearDelveData();

        if (info)
        {
            // Reset entry WorldStates
            player->SendUpdateWorldState(WS_DELVE_TIER, 0);
            player->SendUpdateWorldState(WS_DELVE_IN_DELVE_FLAG, 0);
            player->SendUpdateWorldState(WS_DELVE_MAP_ID, 0);
            player->SendUpdateWorldState(WS_DELVE_TIER_SPELL, 0);
            player->SendUpdateWorldState(WS_DELVE_UNKNOWN_26903, 0);

            // Teleport to overworld entrance seamless = no loading screen :)
            player->TeleportTo(0, info->ExitX, info->ExitY, info->ExitZ, info->ExitO,
                TELE_TO_SEAMLESS);
        }
        else
        {
            // Fallback for unknown delve maps
            if (AreaTriggerTeleport const* at = sObjectMgr->GetGoBackTrigger(player->GetMapId()))
                player->TeleportTo(at->Loc);
            else
                player->TeleportTo(player->m_homebind);
        }

        return true;
    }
};

struct go_delve_campfire : public GameObjectAI
{
    go_delve_campfire(GameObject* go) : GameObjectAI(go) { }

    bool OnGossipHello(Player* player) override
    {
        if (!player)
            return true;

        TC_LOG_DEBUG("scripts.delve", "go_delve_campfire: Player {} interacted with Delve Campfire",
            player->GetName());
        // Cast the campfire eating spell on the player
        me->CastSpell(player, 1272119);

        return true;
    }
};


// npc_delvers_supplies - Delvers' Supplies (NPC 207283)
// Universal vendor/repair + companion summon NPC.
// GossipID 31704 with 3 options:
//   OptionID 2: <View goods and repair gear.>    (vendor)
//   OptionID 3: <Signal your companion...>       (companion summon)
//   OptionID 5: <View companion supplies.>       (companion vendor)
// Standard gossip - DB-driven via gossip_menu_option SQL.
// This AI only needs to handle the companion summon option (3).

struct npc_delvers_supplies : public ScriptedAI
{
    npc_delvers_supplies(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_PASSIVE);
    }

    bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override
    {
        if (!player)
            return true;

        // gossipListId maps to the option's order index in the menu
        // Option 3 = "Signal your companion to gather here"
        // For now, log it - companion spawn is P3
        TC_LOG_DEBUG("scripts.delve", "npc_delvers_supplies: Player {} selected option {} on NPC {}",
            player->GetName(), gossipListId, me->GetEntry());

        return false;
    }
};

// npc_valeera_companion - Delve Companion (NPC 248567)
// PlayerCompanionInfo ID 11, DelvesSeasonID 4
// TraitTreeID 1168, TraitSystemID 43, WidgetSetID 1733
// SubTrees: DPS 108, Heal 109, Tank 110
// Clicking her opens SMSG_SHOW_DELVES_COMPANION_CONFIGURATION_UI
struct npc_valeera_companion : public ScriptedAI
{
    npc_valeera_companion(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_PASSIVE);
    }

    bool OnGossipHello(Player* player) override
    {
        if (!player || !player->GetSession())
            return true;

        TC_LOG_DEBUG("scripts.delve", "npc_valeera_companion: Player {} opened companion config UI",
            player->GetName());

        // Send companion configuration UI packet
        WorldPackets::Delve::ShowDelvesCompanionConfigurationUI configUI;
        configUI.CompanionConfigValue = 257755; // From sniff - companion TraitConfig ID
        player->GetSession()->SendPacket(configUI.Write());

        return true;
    }
};

void AddSC_delve_system()
{
    new npc_enter_delve();
    RegisterGameObjectAI(go_leave_delve);
    RegisterGameObjectAI(go_delve_campfire);
    RegisterCreatureAI(npc_delvers_supplies);
    RegisterCreatureAI(npc_valeera_companion);
}
