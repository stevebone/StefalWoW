/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotPacketSniffer.h"
#include "Core/Events/GenericEventBus.h"
#include "NPC/NPCEvents.h"
#include "NPCPackets.h"
#include "BankPackets.h"
#include "PetitionPackets.h"
#include "Player.h"
#include "WorldSession.h"
#include "Log.h"
#include "PetitionMgr.h"
#include "CharacterCache.h"
#include "Guild.h"
#include "World.h"
#include "ObjectAccessor.h"

namespace Playerbot
{

/**
 * @brief SMSG_GOSSIP_MESSAGE - Gossip menu received from NPC
 */
void ParseTypedGossipMessage(WorldSession* session, WorldPackets::NPC::GossipMessage const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    // Extract gossip option IDs
    ::std::vector<Playerbot::GossipMenuItem> items;
    items.reserve(packet.GossipOptions.size());
    for (auto const& option : packet.GossipOptions)
    {
        Playerbot::GossipMenuItem item;
        item.menuId = packet.GossipID;
        item.optionIndex = option.OrderIndex;
        item.text = option.Text;
        item.icon = static_cast<uint32>(option.OptionNPC);
        items.push_back(item);
    }

    NPCEvent event = NPCEvent::GossipMenuReceived(
        bot->GetGUID(),
        packet.GossipGUID,
        packet.GossipID,
        ::std::move(items),
        ::std::to_string(packet.RandomTextID.value_or(0))
    );

    EventBus<NPCEvent>::instance()->PublishEvent(event);

    TC_LOG_TRACE("playerbot.packets", "Bot {} received GOSSIP_MESSAGE (typed): npc={}, menu={}, options={}",
        bot->GetName(), packet.GossipGUID.ToString(), packet.GossipID, items.size());
}

/**
 * @brief SMSG_GOSSIP_COMPLETE - Gossip interaction complete
 */
void ParseTypedGossipComplete(WorldSession* session, WorldPackets::NPC::GossipComplete const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    // Note: GossipComplete doesn't include NPC GUID, use last known NPC
    NPCEvent event = NPCEvent::GossipComplete(
        bot->GetGUID(),
        ObjectGuid::Empty  // No NPC GUID available in packet
    );

    EventBus<NPCEvent>::instance()->PublishEvent(event);

    TC_LOG_TRACE("playerbot.packets", "Bot {} received GOSSIP_COMPLETE (typed)",
        bot->GetName());
}

/**
 * @brief SMSG_VENDOR_INVENTORY - Vendor item list received
 */
void ParseTypedVendorInventory(WorldSession* session, WorldPackets::NPC::VendorInventory const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    // Extract item IDs from vendor inventory
    ::std::vector<uint32> items;
    items.reserve(packet.Items.size());
    for (auto const& vendorItem : packet.Items)
        items.push_back(vendorItem.Item.ItemID);

    NPCEvent event = NPCEvent::VendorListReceived(
        bot->GetGUID(),
        packet.Vendor,
        0,  // Vendor entry not in packet
        ::std::move(items)
    );

    EventBus<NPCEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received VENDOR_INVENTORY (typed): vendor={}, items={}",
        bot->GetName(), packet.Vendor.ToString(), items.size());
}

/**
 * @brief SMSG_TRAINER_LIST - Trainer spell list received
 */
void ParseTypedTrainerList(WorldSession* session, WorldPackets::NPC::TrainerList const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    // Extract spell data from trainer list
    ::std::vector<Playerbot::TrainerSpell> spells;
    spells.reserve(packet.Spells.size());
    for (auto const& spell : packet.Spells)
    {
        Playerbot::TrainerSpell ts;
        ts.spellId = spell.SpellID;
        ts.reqLevel = spell.ReqLevel;
        ts.reqSkill = spell.ReqSkillLine;
        ts.cost = spell.MoneyCost;
        spells.push_back(ts);
    }

    NPCEvent event = NPCEvent::TrainerListReceived(
        bot->GetGUID(),
        packet.TrainerGUID,
        packet.TrainerID,
        ::std::move(spells),
        packet.Greeting
    );

    EventBus<NPCEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received TRAINER_LIST (typed): trainer={}, spells={}",
        bot->GetName(), packet.TrainerGUID.ToString(), spells.size());
}

/**
 * @brief SMSG_TRAINER_BUY_FAILED - Trainer service result
 */
void ParseTypedTrainerBuyFailed(WorldSession* session, WorldPackets::NPC::TrainerBuyFailed const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    NPCEvent event = NPCEvent::TrainerServiceResult(
        bot->GetGUID(),
        packet.TrainerGUID,
        packet.TrainerFailedReason,
        "" // Error message not provided in packet
    );

    EventBus<NPCEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received TRAINER_BUY_FAILED (typed): reason={}",
        bot->GetName(), packet.TrainerFailedReason);
}

/**
 * @brief SMSG_NPC_INTERACTION_OPEN_RESULT - NPC interaction opened (used for bank)
 */
void ParseTypedNPCInteractionOpen(WorldSession* session, WorldPackets::NPC::NPCInteractionOpenResult const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    // Check if this is a bank interaction
    // PlayerInteractionType enum values not directly accessible, check success
    if (packet.Success)
    {
        NPCEvent event = NPCEvent::BankOpened(
            bot->GetGUID(),
            packet.Npc
        );

        EventBus<NPCEvent>::instance()->PublishEvent(event);

        TC_LOG_DEBUG("playerbot.packets", "Bot {} received NPC_INTERACTION_OPEN (typed): npc={}, type={}",
            bot->GetName(), packet.Npc.ToString(), static_cast<int32>(packet.InteractionType));
    }
}

/**
 * @brief SMSG_PETITION_SHOW_LIST - Petition list received
 */
void ParseTypedPetitionShowList(WorldSession* session, WorldPackets::Petition::ServerPetitionShowList const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    NPCEvent event = NPCEvent::PetitionListReceived(
        bot->GetGUID(),
        packet.Unit,
        {} // Empty vector - petition entries not provided in show list packet
    );

    EventBus<NPCEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received PETITION_SHOW_LIST (typed): npc={}, price={}",
        bot->GetName(), packet.Unit.ToString(), packet.Price);
}

/**
 * @brief SMSG_PETITION_SHOW_SIGNATURES - Petition signatures received
 *
 * When a player offers a guild charter to a bot, automatically sign it.
 * This enables players to easily create guilds with bot signatures.
 */
void ParseTypedPetitionShowSignatures(WorldSession* session, WorldPackets::Petition::ServerPetitionShowSignatures const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    TC_LOG_INFO("playerbot.guild", "Bot {} received petition offer from {} (Item: {})",
        bot->GetName(), packet.Owner.ToString(), packet.Item.ToString());

    // Publish event for any subscribers
    NPCEvent event = NPCEvent::PetitionListReceived(
        bot->GetGUID(),
        packet.Owner,
        { static_cast<uint32>(packet.PetitionID) }
    );
    EventBus<NPCEvent>::instance()->PublishEvent(event);

    // ========================================================================
    // AUTO-SIGN GUILD CHARTER FOR BOTS
    // ========================================================================
    // When a player presents a guild charter to a bot, automatically sign it.
    // This allows players to easily create guilds in single-player mode.
    // ========================================================================

    // Get the petition
    Petition* petition = sPetitionMgr->GetPetition(packet.Item);
    if (!petition)
    {
        TC_LOG_INFO("playerbot.guild", "Bot {} cannot sign petition - petition {} not found",
            bot->GetName(), packet.Item.ToString());
        return;
    }

    ObjectGuid ownerGuid = petition->OwnerGuid;

    // Don't sign your own petition
    if (ownerGuid == bot->GetGUID())
    {
        TC_LOG_INFO("playerbot.guild", "Bot {} cannot sign own petition", bot->GetName());
        return;
    }

    // Check faction compatibility (unless cross-faction guilds are allowed)
    if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD))
    {
        uint32 ownerTeam = sCharacterCache->GetCharacterTeamByGuid(ownerGuid);
        if (bot->GetTeam() != ownerTeam)
        {
            TC_LOG_INFO("playerbot.guild", "Bot {} cannot sign petition - different faction (bot: {}, owner: {})",
                bot->GetName(), bot->GetTeam(), ownerTeam);
            return;
        }
    }

    // Bot cannot sign if already in a guild
    if (bot->GetGuildId())
    {
        TC_LOG_INFO("playerbot.guild", "Bot {} cannot sign petition - already in guild {}",
            bot->GetName(), bot->GetGuildId());
        return;
    }

    // Bot cannot sign if already invited to a guild
    if (bot->GetGuildIdInvited())
    {
        TC_LOG_INFO("playerbot.guild", "Bot {} cannot sign petition - already invited to guild",
            bot->GetName());
        return;
    }

    // Check if maximum signatures reached
    if (petition->Signatures.size() >= 10)
    {
        TC_LOG_INFO("playerbot.guild", "Bot {} cannot sign petition - max signatures reached ({})",
            bot->GetName(), petition->Signatures.size());
        return;
    }

    // Check if bot's account already signed this petition
    if (petition->IsPetitionSignedByAccount(session->GetAccountId()))
    {
        TC_LOG_INFO("playerbot.guild", "Bot {} account {} already signed petition",
            bot->GetName(), session->GetAccountId());
        return;
    }

    // Sign the petition!
    petition->AddSignature(session->GetAccountId(), bot->GetGUID(), false);

    TC_LOG_INFO("playerbot.guild", "Bot {} AUTO-SIGNED guild charter {} (owner: {}, signatures: {})",
        bot->GetName(), packet.Item.ToString(), ownerGuid.ToString(),
        petition->Signatures.size());

    // Send sign result to bot
    WorldPackets::Petition::PetitionSignResults signResult;
    signResult.Player = bot->GetGUID();
    signResult.Item = packet.Item;
    signResult.Error = int32(PETITION_SIGN_OK);
    session->SendPacket(signResult.Write());

    // Notify the petition owner if online
    if (Player* owner = ObjectAccessor::FindConnectedPlayer(ownerGuid))
    {
        owner->SendDirectMessage(signResult.GetRawPacket());
        TC_LOG_INFO("playerbot.guild", "Notified owner {} of bot {} signature", owner->GetName(), bot->GetName());
    }
}

/**
 * @brief Register all NPC packet typed handlers
 */
void RegisterNPCPacketHandlers()
{
    // Register gossip handlers
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::NPC::GossipMessage>(&ParseTypedGossipMessage);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::NPC::GossipComplete>(&ParseTypedGossipComplete);

    // Register vendor/trainer handlers
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::NPC::VendorInventory>(&ParseTypedVendorInventory);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::NPC::TrainerList>(&ParseTypedTrainerList);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::NPC::TrainerBuyFailed>(&ParseTypedTrainerBuyFailed);

    // Register bank/interaction handler
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::NPC::NPCInteractionOpenResult>(&ParseTypedNPCInteractionOpen);

    // Register petition handlers
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Petition::ServerPetitionShowList>(&ParseTypedPetitionShowList);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Petition::ServerPetitionShowSignatures>(&ParseTypedPetitionShowSignatures);

    TC_LOG_INFO("playerbot", "PlayerbotPacketSniffer: Registered {} NPC packet typed handlers", 8);
}

} // namespace Playerbot
