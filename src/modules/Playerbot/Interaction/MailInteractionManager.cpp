/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "MailInteractionManager.h"
#include "CellImpl.h"
#include "CharacterCache.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "GameObject.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "Mail.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"
#include <chrono>

namespace Playerbot
{

MailInteractionManager::MailInteractionManager(Player* bot)
    : m_bot(bot)
    , m_stats()
    , m_cpuUsage(0.0f)
    , m_totalOperationTime(0)
    , m_operationCount(0)
    , m_cachedStatus()
    , m_lastStatusCheck(0)
{
}

// ============================================================================
// Core Mail Methods
// ============================================================================

bool MailInteractionManager::SendMail(GameObject* mailbox, std::string const& recipient,
                                       std::string const& subject, std::string const& body,
                                       uint64 money, std::vector<Item*> const* items)
{
    if (!m_bot || !mailbox)
        return false;

    auto startTime = std::chrono::high_resolution_clock::now();

    // Verify mailbox
    if (!IsMailbox(mailbox))
    {
        TC_LOG_DEBUG("module.playerbot", "MailInteractionManager[{}]: Invalid mailbox",
            m_bot->GetName());
        return false;
    }

    // Check distance
    if (!IsInMailboxRange(mailbox))
    {
        TC_LOG_DEBUG("module.playerbot", "MailInteractionManager[{}]: Too far from mailbox",
            m_bot->GetName());
        return false;
    }

    // Get recipient GUID
    ObjectGuid recipientGuid = GetRecipientGuid(recipient);
    if (recipientGuid.IsEmpty())
    {
        TC_LOG_DEBUG("module.playerbot", "MailInteractionManager[{}]: Recipient '{}' not found",
            m_bot->GetName(), recipient);
        return false;
    }

    // Calculate and check postage
    uint32 itemCount = items ? static_cast<uint32>(items->size()) : 0;
    if (!CanAffordPostage(money, itemCount))
    {
        TC_LOG_DEBUG("module.playerbot", "MailInteractionManager[{}]: Cannot afford postage",
            m_bot->GetName());
        return false;
    }

    // Execute send
    bool success = ExecuteSendMail(recipientGuid, subject, body, money, items);

    if (success)
    {
        RecordMailSent(money, itemCount);

        TC_LOG_DEBUG("module.playerbot", "MailInteractionManager[{}]: Sent mail to {} with {} copper and {} items",
            m_bot->GetName(), recipient, money, itemCount);
    }

    // Track performance
    auto endTime = std::chrono::high_resolution_clock::now();
    uint32 duration = static_cast<uint32>(
        std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count());
    m_totalOperationTime += duration;
    m_operationCount++;
    m_cpuUsage = static_cast<float>(m_totalOperationTime) / (m_operationCount * 1000.0f);

    return success;
}

bool MailInteractionManager::TakeMail(GameObject* mailbox, uint64 mailId)
{
    if (!m_bot || !mailbox)
        return false;

    auto startTime = std::chrono::high_resolution_clock::now();

    // Verify mailbox
    if (!IsMailbox(mailbox) || !IsInMailboxRange(mailbox))
        return false;

    // Execute take
    bool success = ExecuteTakeMail(mailId);

    if (success)
    {
        m_lastStatusCheck = 0; // Invalidate cache
        TC_LOG_DEBUG("module.playerbot", "MailInteractionManager[{}]: Took mail {}",
            m_bot->GetName(), mailId);
    }

    // Track performance
    auto endTime = std::chrono::high_resolution_clock::now();
    uint32 duration = static_cast<uint32>(
        std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count());
    m_totalOperationTime += duration;
    m_operationCount++;

    return success;
}

uint32 MailInteractionManager::TakeAllMail(GameObject* mailbox)
{
    if (!m_bot || !mailbox)
        return 0;

    if (!IsMailbox(mailbox) || !IsInMailboxRange(mailbox))
        return 0;

    std::vector<MailEvaluation> mails = GetAllMails();
    uint32 taken = 0;

    for (MailEvaluation const& mail : mails)
    {
        if (mail.hasItems || mail.hasMoney)
        {
            if (TakeMail(mailbox, mail.mailId))
                taken++;
        }
    }

    TC_LOG_DEBUG("module.playerbot", "MailInteractionManager[{}]: Took {} mails",
        m_bot->GetName(), taken);

    return taken;
}

bool MailInteractionManager::DeleteMail(GameObject* mailbox, uint64 mailId)
{
    if (!m_bot || !mailbox)
        return false;

    if (!IsMailbox(mailbox) || !IsInMailboxRange(mailbox))
        return false;

    bool success = ExecuteDeleteMail(mailId);

    if (success)
    {
        m_stats.mailsDeleted++;
        m_lastStatusCheck = 0; // Invalidate cache
        TC_LOG_DEBUG("module.playerbot", "MailInteractionManager[{}]: Deleted mail {}",
            m_bot->GetName(), mailId);
    }

    return success;
}

bool MailInteractionManager::ReturnMail(GameObject* mailbox, uint64 mailId)
{
    if (!m_bot || !mailbox)
        return false;

    if (!IsMailbox(mailbox) || !IsInMailboxRange(mailbox))
        return false;

    // Find the mail
    Mail* mail = nullptr;
    for (Mail* m : m_bot->GetMails())
    {
        if (m->messageID == mailId)
        {
            mail = m;
            break;
        }
    }

    if (!mail)
    {
        TC_LOG_DEBUG("module.playerbot", "MailInteractionManager[{}]: Mail {} not found",
            m_bot->GetName(), mailId);
        return false;
    }

    // Can only return mail that has items or money
    if (mail->items.empty() && mail->money == 0)
    {
        TC_LOG_DEBUG("module.playerbot", "MailInteractionManager[{}]: Mail {} has nothing to return",
            m_bot->GetName(), mailId);
        return false;
    }

    // Return the mail - notify client and mark as returned
    m_bot->SendMailResult(mailId, MAIL_RETURNED_TO_SENDER, MAIL_OK);

    // Mark mail as returned so it goes back to sender
    mail->checked |= MAIL_CHECK_MASK_RETURNED;
    mail->state = MAIL_STATE_CHANGED;

    m_stats.mailsReturned++;
    m_lastStatusCheck = 0;

    TC_LOG_DEBUG("module.playerbot", "MailInteractionManager[{}]: Returned mail {}",
        m_bot->GetName(), mailId);

    return true;
}

uint32 MailInteractionManager::SmartProcessMail(GameObject* mailbox)
{
    if (!m_bot || !mailbox)
        return 0;

    if (!IsMailbox(mailbox) || !IsInMailboxRange(mailbox))
        return 0;

    std::vector<MailEvaluation> mails = GetAllMails();
    uint32 processed = 0;

    // Sort by priority (CRITICAL first)
    std::sort(mails.begin(), mails.end(), [](MailEvaluation const& a, MailEvaluation const& b) {
        return static_cast<uint8>(a.priority) < static_cast<uint8>(b.priority);
    });

    for (MailEvaluation const& mail : mails)
    {
        if (mail.shouldTake)
        {
            // Check if COD is affordable
            if (mail.isCOD && !m_bot->HasEnoughMoney(mail.codAmount))
            {
                TC_LOG_DEBUG("module.playerbot", "MailInteractionManager[{}]: Skipping COD mail {} - cannot afford {} copper",
                    m_bot->GetName(), mail.mailId, mail.codAmount);
                continue;
            }

            if (TakeMail(mailbox, mail.mailId))
                processed++;
        }
        else if (mail.shouldDelete)
        {
            if (DeleteMail(mailbox, mail.mailId))
                processed++;
        }
    }

    TC_LOG_DEBUG("module.playerbot", "MailInteractionManager[{}]: Smart processed {} mails",
        m_bot->GetName(), processed);

    return processed;
}

// ============================================================================
// Mail Analysis Methods
// ============================================================================

MailInteractionManager::MailboxStatus MailInteractionManager::GetMailboxStatus() const
{
    if (!m_bot)
        return MailboxStatus();

    // Check cache
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (m_lastStatusCheck > 0 &&
        (currentTime - m_lastStatusCheck) < STATUS_CACHE_DURATION)
    {
        return m_cachedStatus;
    }

    MailboxStatus status;

    PlayerMails const& mails = m_bot->GetMails();
    status.totalMails = static_cast<uint32>(mails.size());

    for (Mail const* mail : mails)
    {
        if (mail->checked == 0) // MAIL_CHECK_MASK_READ
            status.unreadMails++;

        if (!mail->items.empty())
            status.mailsWithItems++;

        if (mail->money > 0)
        {
            status.mailsWithMoney++;
            status.totalMoney += mail->money;
        }

        // Check expiration
        time_t expireTime = mail->expire_time;
        time_t currentServerTime = GameTime::GetGameTime();
        if (expireTime > currentServerTime)
        {
            uint32 daysRemaining = static_cast<uint32>((expireTime - currentServerTime) / DAY);
            if (daysRemaining <= MAIL_EXPIRY_WARNING_DAYS)
                status.expiringMails++;
        }
        else
        {
            // Already expired
            status.expiringMails++;
        }
    }

    // Update cache
    m_cachedStatus = status;
    m_lastStatusCheck = currentTime;

    return status;
}

std::vector<MailInteractionManager::MailEvaluation> MailInteractionManager::GetAllMails() const
{
    std::vector<MailEvaluation> evaluations;

    if (!m_bot)
        return evaluations;

    PlayerMails const& mails = m_bot->GetMails();
    for (Mail const* mail : mails)
    {
        evaluations.push_back(EvaluateMail(mail));
    }

    return evaluations;
}

MailInteractionManager::MailEvaluation MailInteractionManager::EvaluateMail(Mail const* mail) const
{
    MailEvaluation eval;

    if (!mail)
        return eval;

    eval.mailId = mail->messageID;
    eval.priority = CalculateMailPriority(mail);
    eval.hasItems = !mail->items.empty();
    eval.hasMoney = mail->money > 0;
    eval.moneyAmount = mail->money;
    eval.isCOD = mail->COD > 0;
    eval.codAmount = mail->COD;

    // Calculate days remaining
    time_t expireTime = mail->expire_time;
    time_t currentTime = GameTime::GetGameTime();
    if (expireTime > currentTime)
        eval.daysRemaining = static_cast<uint32>((expireTime - currentTime) / DAY);
    else
        eval.daysRemaining = 0;

    // Get item details
    for (::MailItemInfo const& itemInfo : mail->items)
    {
        BotMailItemInfo info;
        info.itemGuid = itemInfo.item_guid;
        info.itemId = itemInfo.item_template;

        if (ItemTemplate const* tmpl = sObjectMgr->GetItemTemplate(info.itemId))
        {
            info.itemName = tmpl->GetName(sWorld->GetDefaultDbcLocale());
        }

        eval.items.push_back(info);
    }

    // Determine recommendations
    DetermineMailRecommendations(eval, mail);

    return eval;
}

MailInteractionManager::MailPriority MailInteractionManager::CalculateMailPriority(Mail const* mail) const
{
    if (!mail)
        return MailPriority::LOW;

    // Auction mail is critical
    if (IsAuctionMail(mail))
        return MailPriority::CRITICAL;

    // COD mail is critical
    if (mail->COD > 0)
        return MailPriority::CRITICAL;

    // Mail with items or money is high priority
    if (!mail->items.empty() || mail->money > 0)
        return MailPriority::HIGH;

    // System/GM mail is medium
    if (IsSystemMail(mail))
        return MailPriority::MEDIUM;

    return MailPriority::LOW;
}

bool MailInteractionManager::HasValuableContents(Mail const* mail) const
{
    if (!mail)
        return false;

    // Has money
    if (mail->money > 0)
        return true;

    // Has items
    if (!mail->items.empty())
    {
        for (::MailItemInfo const& itemInfo : mail->items)
        {
            ItemTemplate const* tmpl = sObjectMgr->GetItemTemplate(itemInfo.item_template);
            if (tmpl && tmpl->GetQuality() >= ITEM_QUALITY_UNCOMMON)
                return true;
        }
    }

    return false;
}

uint64 MailInteractionManager::GetTotalMailMoney() const
{
    MailboxStatus status = GetMailboxStatus();
    return status.totalMoney;
}

bool MailInteractionManager::HasExpiringMail(uint32 daysThreshold) const
{
    if (!m_bot)
        return false;

    PlayerMails const& mails = m_bot->GetMails();
    time_t currentTime = GameTime::GetGameTime();

    for (Mail const* mail : mails)
    {
        if (mail->expire_time <= currentTime)
            continue;

        uint32 daysRemaining = static_cast<uint32>((mail->expire_time - currentTime) / DAY);
        if (daysRemaining <= daysThreshold)
        {
            // Only care about mail with value
            if (mail->money > 0 || !mail->items.empty())
                return true;
        }
    }

    return false;
}

// ============================================================================
// Mailbox Interaction
// ============================================================================

bool MailInteractionManager::IsMailbox(GameObject* target) const
{
    if (!target)
        return false;

    return target->GetGoType() == GAMEOBJECT_TYPE_MAILBOX;
}

GameObject* MailInteractionManager::FindNearestMailbox(float maxRange) const
{
    if (!m_bot)
        return nullptr;

    GameObject* nearest = nullptr;
    float nearestDist = maxRange;

    std::list<GameObject*> objects;
    Trinity::GameObjectInRangeCheck check(m_bot->GetPositionX(), m_bot->GetPositionY(),
                                           m_bot->GetPositionZ(), maxRange);
    Trinity::GameObjectListSearcher<Trinity::GameObjectInRangeCheck> searcher(m_bot, objects, check);
    Cell::VisitGridObjects(m_bot, searcher, maxRange);

    for (GameObject* go : objects)
    {
        if (!go)
            continue;

        if (!IsMailbox(go))
            continue;

        float dist = m_bot->GetDistance(go);
        if (dist < nearestDist)
        {
            nearest = go;
            nearestDist = dist;
        }
    }

    return nearest;
}

bool MailInteractionManager::IsInMailboxRange(GameObject* mailbox) const
{
    if (!m_bot || !mailbox)
        return false;

    static constexpr float MAILBOX_INTERACTION_DISTANCE = 10.0f;
    return m_bot->GetDistance(mailbox) <= MAILBOX_INTERACTION_DISTANCE;
}

bool MailInteractionManager::CanAffordPostage(uint64 money, uint32 itemCount) const
{
    if (!m_bot)
        return false;

    uint32 postage = GetPostageCost(itemCount);
    uint64 totalCost = money + postage;

    return m_bot->HasEnoughMoney(totalCost);
}

uint32 MailInteractionManager::GetPostageCost(uint32 itemCount) const
{
    return MAIL_POSTAGE_BASE + (itemCount * MAIL_POSTAGE_PER_ITEM);
}

// ============================================================================
// Statistics and Performance
// ============================================================================

size_t MailInteractionManager::GetMemoryUsage() const
{
    size_t usage = sizeof(*this);
    // Add any dynamic allocation sizes
    return usage;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

bool MailInteractionManager::ExecuteSendMail(ObjectGuid recipientGuid, std::string const& subject,
                                              std::string const& body, uint64 money,
                                              std::vector<Item*> const* items)
{
    if (!m_bot)
        return false;

    // Calculate postage
    uint32 itemCount = items ? static_cast<uint32>(items->size()) : 0;
    uint32 postage = GetPostageCost(itemCount);

    // Create mail draft
    MailDraft draft(subject, body);

    // Add items if any
    if (items && !items->empty())
    {
        for (Item* item : *items)
        {
            if (item)
            {
                // Remove item from player's inventory
                m_bot->MoveItemFromInventory(item->GetBagSlot(), item->GetSlot(), true);
                item->DeleteFromInventoryDB(CharacterDatabaseTransaction(nullptr));
                item->SetOwnerGUID(ObjectGuid::Empty);

                draft.AddItem(item);
            }
        }
    }

    // Add money
    if (money > 0)
    {
        draft.AddMoney(money);
    }

    // Deduct postage and money from player
    m_bot->ModifyMoney(-static_cast<int64>(postage + money));

    // Send the mail using recipient's low GUID
    draft.SendMailTo(CharacterDatabaseTransaction(nullptr),
                     MailReceiver(recipientGuid.GetCounter()),
                     MailSender(m_bot),
                     MAIL_CHECK_MASK_NONE,
                     30 * DAY); // 30 day expiry

    m_stats.postageSpent += postage;

    return true;
}

bool MailInteractionManager::ExecuteTakeMail(uint64 mailId)
{
    if (!m_bot)
        return false;

    // Find the mail
    Mail* mail = nullptr;
    for (Mail* m : m_bot->GetMails())
    {
        if (m->messageID == mailId)
        {
            mail = m;
            break;
        }
    }

    if (!mail)
        return false;

    // Handle COD payment
    if (mail->COD > 0)
    {
        if (!m_bot->HasEnoughMoney(mail->COD))
            return false;

        m_bot->ModifyMoney(-static_cast<int64>(mail->COD));

        // Send COD money to sender
        // TrinityCore handles this automatically through the mail system
    }

    uint64 moneyTaken = mail->money;
    uint32 itemsTaken = 0;

    // Take money
    if (mail->money > 0)
    {
        m_bot->ModifyMoney(static_cast<int64>(mail->money));
        mail->money = 0;
        mail->state = MAIL_STATE_CHANGED;
    }

    // Take items
    for (auto const& itemInfo : mail->items)
    {
        // Create the item for the player
        Item* item = Item::CreateItem(itemInfo.item_template, 1, ItemContext::NONE, m_bot);
        if (!item)
            continue;

        item->SetOwnerGUID(m_bot->GetGUID());

        ItemPosCountVec dest;
        InventoryResult result = m_bot->CanStoreItem(NULL_BAG, NULL_SLOT, dest, item, false);
        if (result == EQUIP_ERR_OK)
        {
            m_bot->StoreItem(dest, item, true);
            itemsTaken++;
        }
        else
        {
            delete item;
        }
    }

    // Clear items from mail
    mail->items.clear();
    mail->state = MAIL_STATE_CHANGED;

    // Mark as read
    mail->checked |= MAIL_CHECK_MASK_READ;

    // Record statistics
    RecordMailTaken(moneyTaken, itemsTaken);

    // If mail is empty, delete it
    if (mail->items.empty() && mail->money == 0 && mail->COD == 0)
    {
        m_bot->RemoveMail(mailId);
    }

    return true;
}

bool MailInteractionManager::ExecuteDeleteMail(uint64 mailId)
{
    if (!m_bot)
        return false;

    // Find the mail
    Mail* mail = nullptr;
    for (Mail* m : m_bot->GetMails())
    {
        if (m->messageID == mailId)
        {
            mail = m;
            break;
        }
    }

    if (!mail)
        return false;

    // Don't delete mail with items or money
    if (!mail->items.empty() || mail->money > 0)
    {
        TC_LOG_DEBUG("module.playerbot", "MailInteractionManager[{}]: Cannot delete mail {} - has items or money",
            m_bot->GetName(), mailId);
        return false;
    }

    // Remove the mail
    m_bot->RemoveMail(mailId);

    return true;
}

ObjectGuid MailInteractionManager::GetRecipientGuid(std::string const& name) const
{
    // Use character cache to find the character
    ObjectGuid guid = sCharacterCache->GetCharacterGuidByName(name);
    return guid;
}

bool MailInteractionManager::IsAuctionMail(Mail const* mail) const
{
    if (!mail)
        return false;

    // Auction mail has specific stationery
    return mail->stationery == MAIL_STATIONERY_AUCTION;
}

bool MailInteractionManager::IsSystemMail(Mail const* mail) const
{
    if (!mail)
        return false;

    // System mail typically uses GM stationery or has no sender (sender == 0)
    return mail->stationery == MAIL_STATIONERY_GM ||
           mail->sender == 0;
}

void MailInteractionManager::RecordMailSent(uint64 money, uint32 /*itemCount*/)
{
    m_stats.mailsSent++;
    m_stats.moneySent += money;
}

void MailInteractionManager::RecordMailTaken(uint64 money, uint32 itemCount)
{
    m_stats.mailsReceived++;
    m_stats.moneyReceived += money;
    m_stats.itemsReceived += itemCount;
}

void MailInteractionManager::DetermineMailRecommendations(MailEvaluation& eval, Mail const* mail) const
{
    // Default: take valuable mail, delete spam
    eval.shouldTake = false;
    eval.shouldDelete = false;

    // Auction mail - always take
    if (IsAuctionMail(mail))
    {
        eval.shouldTake = true;
        eval.reason = "Auction mail - take immediately";
        return;
    }

    // COD mail - take only if affordable and valuable
    if (eval.isCOD)
    {
        if (HasValuableContents(mail) && m_bot && m_bot->HasEnoughMoney(eval.codAmount))
        {
            eval.shouldTake = true;
            eval.reason = "COD mail with valuable items";
        }
        else
        {
            eval.shouldTake = false;
            eval.reason = "COD mail - cannot afford or not valuable";
        }
        return;
    }

    // Mail with items or money - take
    if (eval.hasItems || eval.hasMoney)
    {
        eval.shouldTake = true;
        eval.reason = "Contains items or money";
        return;
    }

    // Expiring mail - delete if no value
    if (eval.daysRemaining <= 1)
    {
        eval.shouldDelete = true;
        eval.reason = "Expiring soon, no valuable contents";
        return;
    }

    // System mail - keep but don't take
    if (IsSystemMail(mail))
    {
        eval.shouldDelete = false;
        eval.reason = "System notification";
        return;
    }

    // Regular mail with no value - can be deleted
    if (!eval.hasItems && !eval.hasMoney && eval.daysRemaining <= 7)
    {
        eval.shouldDelete = true;
        eval.reason = "No valuable contents";
        return;
    }

    eval.reason = "Standard mail";
}

} // namespace Playerbot
