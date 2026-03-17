/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <vector>
#include <unordered_map>
#include <string>

class Player;
class Creature;
class Item;
class GameObject;
struct Mail;

namespace Playerbot
{

/**
 * @class MailInteractionManager
 * @brief Manages all mailbox interactions for player bots
 *
 * This class provides complete mail functionality using TrinityCore's
 * mail system APIs. It handles:
 * - Sending mail with items/gold
 * - Receiving and reading mail
 * - Taking mail attachments (items and gold)
 * - Mail deletion and management
 * - Smart mail processing (auto-take valuable items)
 *
 * Performance Target: <1ms per mail operation
 * Memory Target: <15KB overhead
 *
 * TrinityCore API Integration:
 * - Player::GetMails() - Get player's mailbox
 * - MailDraft - Create outgoing mail
 * - Player::SendNewItem() - Send items via mail
 * - Mail structure - Mail data access
 */
class MailInteractionManager
{
public:
    /**
     * @brief Mail priority for processing
     */
    enum class MailPriority : uint8
    {
        CRITICAL = 0,   // Auction won, CoD
        HIGH     = 1,   // Items, gold
        MEDIUM   = 2,   // GM mail, system notifications
        LOW      = 3    // Normal mail, advertisements
    };

    /**
     * @brief Mail item information for bot evaluation
     */
    struct BotMailItemInfo
    {
        uint32 itemId;
        uint64 itemGuid;            // ObjectGuid::LowType
        uint32 stackCount;
        ::std::string itemName;

        BotMailItemInfo()
            : itemId(0), itemGuid(0), stackCount(0)
        { }
    };

    /**
     * @brief Mail evaluation result
     */
    struct MailEvaluation
    {
        uint64 mailId;
        MailPriority priority;
        bool hasItems;
        bool hasMoney;
        bool isCOD;
        uint64 moneyAmount;          // Gold in copper
        uint64 codAmount;            // COD cost in copper
        uint32 daysRemaining;        // Days until deletion
        bool shouldTake;             // Recommended to take
        bool shouldDelete;           // Recommended to delete
        ::std::vector<BotMailItemInfo> items;
        ::std::string reason;          // Human-readable reason

        MailEvaluation()
            : mailId(0), priority(MailPriority::LOW)
            , hasItems(false), hasMoney(false), isCOD(false)
            , moneyAmount(0), codAmount(0), daysRemaining(0)
            , shouldTake(false), shouldDelete(false)
        { }
    };

    /**
     * @brief Mailbox status
     */
    struct MailboxStatus
    {
        uint32 totalMails;           // Total mails in box
        uint32 unreadMails;          // Unread mails
        uint32 mailsWithItems;       // Mails containing items
        uint32 mailsWithMoney;       // Mails containing money
        uint64 totalMoney;           // Total money in all mails
        uint32 expiringMails;        // Mails expiring soon (< 3 days)

        MailboxStatus()
            : totalMails(0), unreadMails(0), mailsWithItems(0)
            , mailsWithMoney(0), totalMoney(0), expiringMails(0)
        { }

        bool HasMail() const { return totalMails > 0; }
        bool HasValuables() const { return mailsWithItems > 0 || totalMoney > 0; }
    };

    MailInteractionManager(Player* bot);
    ~MailInteractionManager() = default;

    // Core Mail Methods

    /**
     * @brief Send mail to another player
     * @param mailbox Mailbox object
     * @param recipient Recipient character name
     * @param subject Mail subject
     * @param body Mail body text
     * @param money Gold to send (in copper)
     * @param items Items to attach (nullptr for no items)
     * @return True if mail sent successfully
     */
    bool SendMail(GameObject* mailbox, ::std::string const& recipient,
                  ::std::string const& subject, ::std::string const& body,
                  uint64 money = 0, ::std::vector<Item*> const* items = nullptr);

    /**
     * @brief Take all items and money from a mail
     * @param mailbox Mailbox object
     * @param mailId Mail ID to take from
     * @return True if successfully taken
     */
    bool TakeMail(GameObject* mailbox, uint64 mailId);

    /**
     * @brief Take all items and money from all mails
     * @param mailbox Mailbox object
     * @return Number of mails processed
     */
    uint32 TakeAllMail(GameObject* mailbox);

    /**
     * @brief Delete a mail
     * @param mailbox Mailbox object
     * @param mailId Mail ID to delete
     * @return True if successfully deleted
     */
    bool DeleteMail(GameObject* mailbox, uint64 mailId);

    /**
     * @brief Return a mail to sender
     * @param mailbox Mailbox object
     * @param mailId Mail ID to return
     * @return True if successfully returned
     */
    bool ReturnMail(GameObject* mailbox, uint64 mailId);

    /**
     * @brief Smart mail processing - take valuable mail, delete spam
     * @param mailbox Mailbox object
     * @return Number of mails processed
     */
    uint32 SmartProcessMail(GameObject* mailbox);

    // Mail Analysis Methods

    /**
     * @brief Get mailbox status
     * @return MailboxStatus structure
     */
    MailboxStatus GetMailboxStatus() const;

    /**
     * @brief Get all mails in mailbox
     * @return Vector of mail evaluations
     */
    ::std::vector<MailEvaluation> GetAllMails() const;

    /**
     * @brief Evaluate a specific mail
     * @param mail Mail to evaluate
     * @return Evaluation result
     */
    MailEvaluation EvaluateMail(Mail const* mail) const;

    /**
     * @brief Calculate mail priority
     * @param mail Mail to evaluate
     * @return Priority level
     */
    MailPriority CalculateMailPriority(Mail const* mail) const;

    /**
     * @brief Check if mail has valuable contents
     * @param mail Mail to check
     * @return True if valuable
     */
    bool HasValuableContents(Mail const* mail) const;

    /**
     * @brief Get total money available in all mails
     * @return Total money in copper
     */
    uint64 GetTotalMailMoney() const;

    /**
     * @brief Check if any mails are expiring soon
     * @param daysThreshold Days threshold for "soon"
     * @return True if any mail expiring within threshold
     */
    bool HasExpiringMail(uint32 daysThreshold = 3) const;

    // Mailbox Interaction

    /**
     * @brief Check if at a mailbox
     * @param target Potential mailbox
     * @return True if valid mailbox
     */
    bool IsMailbox(GameObject* target) const;

    /**
     * @brief Find nearest mailbox
     * @param maxRange Maximum search range
     * @return Nearest mailbox or nullptr
     */
    GameObject* FindNearestMailbox(float maxRange = 100.0f) const;

    /**
     * @brief Check if bot is in mailbox interaction range
     * @param mailbox Mailbox object
     * @return True if in range
     */
    bool IsInMailboxRange(GameObject* mailbox) const;

    /**
     * @brief Check if bot can afford to send mail
     * @param money Money to send
     * @param itemCount Number of items to attach
     * @return True if can afford postage
     */
    bool CanAffordPostage(uint64 money, uint32 itemCount = 0) const;

    /**
     * @brief Get mail postage cost
     * @param itemCount Number of items
     * @return Postage cost in copper
     */
    uint32 GetPostageCost(uint32 itemCount) const;

    // Statistics and Performance

    struct Statistics
    {
        uint32 mailsSent;           // Total mails sent
        uint32 mailsReceived;       // Total mails taken
        uint32 mailsDeleted;        // Total mails deleted
        uint32 mailsReturned;       // Total mails returned
        uint32 itemsReceived;       // Total items received via mail
        uint64 moneyReceived;       // Total money received via mail
        uint64 moneySent;           // Total money sent via mail
        uint64 postageSpent;        // Total postage spent

        Statistics()
            : mailsSent(0), mailsReceived(0), mailsDeleted(0)
            , mailsReturned(0), itemsReceived(0), moneyReceived(0)
            , moneySent(0), postageSpent(0)
        { }
    };

    Statistics const& GetStatistics() const { return m_stats; }
    void ResetStatistics() { m_stats = Statistics(); }

    float GetCPUUsage() const { return m_cpuUsage; }
    size_t GetMemoryUsage() const;

private:
    // Internal Helper Methods

    /**
     * @brief Execute send mail via TrinityCore API
     * @param recipientGuid Recipient GUID
     * @param subject Subject line
     * @param body Body text
     * @param money Money amount
     * @param items Items to attach
     * @return True if successful
     */
    bool ExecuteSendMail(ObjectGuid recipientGuid, ::std::string const& subject,
                         ::std::string const& body, uint64 money,
                         ::std::vector<Item*> const* items);

    /**
     * @brief Execute take mail via TrinityCore API
     * @param mailId Mail ID
     * @return True if successful
     */
    bool ExecuteTakeMail(uint64 mailId);

    /**
     * @brief Execute delete mail via TrinityCore API
     * @param mailId Mail ID
     * @return True if successful
     */
    bool ExecuteDeleteMail(uint64 mailId);

    /**
     * @brief Get recipient GUID from name
     * @param name Character name
     * @return Character GUID or empty if not found
     */
    ObjectGuid GetRecipientGuid(::std::string const& name) const;

    /**
     * @brief Check if mail is from auction house
     * @param mail Mail to check
     * @return True if auction mail
     */
    bool IsAuctionMail(Mail const* mail) const;

    /**
     * @brief Check if mail is a system notification
     * @param mail Mail to check
     * @return True if system mail
     */
    bool IsSystemMail(Mail const* mail) const;

    /**
     * @brief Determine take/delete recommendations for a mail
     * @param eval Evaluation to fill recommendations into
     * @param mail Mail to evaluate
     */
    void DetermineMailRecommendations(MailEvaluation& eval, Mail const* mail) const;

    /**
     * @brief Record mail sent in statistics
     */
    void RecordMailSent(uint64 money, uint32 itemCount);

    /**
     * @brief Record mail taken in statistics
     */
    void RecordMailTaken(uint64 money, uint32 itemCount);

    // Member Variables
    Player* m_bot;
    Statistics m_stats;

    // Performance Tracking
    float m_cpuUsage;
    uint32 m_totalOperationTime;   // microseconds
    uint32 m_operationCount;

    // Cache
    mutable MailboxStatus m_cachedStatus;
    mutable uint32 m_lastStatusCheck;
    static constexpr uint32 STATUS_CACHE_DURATION = 30000; // 30 seconds

    // Mail constants
    static constexpr uint32 MAIL_POSTAGE_BASE = 30;        // Base postage in copper
    static constexpr uint32 MAIL_POSTAGE_PER_ITEM = 30;    // Per-item postage
    static constexpr uint32 MAIL_EXPIRY_WARNING_DAYS = 3;  // Days before warning
};

} // namespace Playerbot
