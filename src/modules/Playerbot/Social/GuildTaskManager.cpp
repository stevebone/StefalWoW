/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * GuildTaskManager: Generates guild tasks and assigns them to bot members.
 */

#include "GuildTaskManager.h"
#include "Player.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "ObjectAccessor.h"
#include "PlayerBotHooks.h"
#include "Database/PlayerbotDatabase.h"
#include "Log.h"
#include "GameTime.h"
#include "ObjectMgr.h"
#include "SharedDefines.h"
#include <algorithm>

namespace Playerbot
{

GuildTaskManager::GuildTaskManager()
    : _nextTaskId(1)
    , _timeSinceGeneration(0)
    , _timeSinceAssignment(0)
    , _timeSinceCleanup(0)
    , _initialized(false)
{
}

bool GuildTaskManager::Initialize()
{
    if (_initialized)
        return true;

    TC_LOG_INFO("module.playerbot", "GuildTaskManager: Initializing...");

    LoadTemplatesFromDB();

    if (_templates.empty())
    {
        TC_LOG_INFO("module.playerbot", "GuildTaskManager: No templates in DB, loading defaults");
        LoadDefaultTemplates();
    }

    _initialized = true;
    TC_LOG_INFO("module.playerbot", "GuildTaskManager: Initialized with {} task templates", _templates.size());
    return true;
}

void GuildTaskManager::Update(uint32 diff)
{
    if (!_initialized)
        return;

    _timeSinceGeneration += diff;
    _timeSinceAssignment += diff;
    _timeSinceCleanup += diff;

    if (_timeSinceGeneration >= GENERATION_INTERVAL_MS)
    {
        _timeSinceGeneration = 0;
        GenerateTasksForGuilds();
    }

    if (_timeSinceAssignment >= ASSIGNMENT_INTERVAL_MS)
    {
        _timeSinceAssignment = 0;
        AssignUnassignedTasks();
    }

    if (_timeSinceCleanup >= CLEANUP_INTERVAL_MS)
    {
        _timeSinceCleanup = 0;
        CleanupExpiredTasks();
    }
}

void GuildTaskManager::Shutdown()
{
    if (!_initialized)
        return;

    TC_LOG_INFO("module.playerbot", "GuildTaskManager: Shutting down ({} guilds tracked, {} active tasks)",
        _guildBoards.size(),
        [this]() {
            uint32 count = 0;
            for (auto const& [id, board] : _guildBoards)
                for (auto const& task : board.activeTasks)
                    if (task.status == GuildTaskStatus::ASSIGNED || task.status == GuildTaskStatus::IN_PROGRESS)
                        ++count;
            return count;
        }());

    _guildBoards.clear();
    _templates.clear();
    _initialized = false;

    TC_LOG_INFO("module.playerbot", "GuildTaskManager: Shutdown complete");
}

// ============================================================================
// Task Board Queries
// ============================================================================

std::vector<GuildTask> GuildTaskManager::GetActiveTasks(uint32 guildId) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _guildBoards.find(guildId);
    if (it == _guildBoards.end())
        return {};
    return it->second.activeTasks;
}

std::vector<GuildTask> GuildTaskManager::GetBotTasks(ObjectGuid botGuid) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<GuildTask> result;
    for (auto const& [guildId, board] : _guildBoards)
        for (auto const& task : board.activeTasks)
            if (task.assigneeGuid == botGuid &&
                (task.status == GuildTaskStatus::ASSIGNED || task.status == GuildTaskStatus::IN_PROGRESS))
                result.push_back(task);
    return result;
}

std::vector<GuildTask> GuildTaskManager::GetAvailableTasks(uint32 guildId) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<GuildTask> result;
    auto it = _guildBoards.find(guildId);
    if (it == _guildBoards.end())
        return result;

    for (auto const& task : it->second.activeTasks)
        if (task.status == GuildTaskStatus::AVAILABLE)
            result.push_back(task);
    return result;
}

GuildTask const* GuildTaskManager::GetTask(uint32 taskId) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto const& [guildId, board] : _guildBoards)
        for (auto const& task : board.activeTasks)
            if (task.taskId == taskId)
                return &task;
    return nullptr;
}

// ============================================================================
// Task Management
// ============================================================================

uint32 GuildTaskManager::CreateTask(uint32 guildId, GuildTaskType type,
                                     uint32 targetEntry, uint32 targetCount,
                                     uint32 durationHours)
{
    std::lock_guard<std::mutex> lock(_mutex);

    GuildTask task;
    task.taskId = _nextTaskId++;
    task.guildId = guildId;
    task.type = type;
    task.difficulty = GuildTaskDifficulty::NORMAL;
    task.status = GuildTaskStatus::AVAILABLE;
    task.targetEntry = targetEntry;
    task.targetCount = targetCount;
    task.createdTime = GameTime::GetGameTime();
    task.deadline = task.createdTime + (durationHours * 3600);

    // Generate title based on type
    switch (type)
    {
        case GuildTaskType::KILL:
            task.title = "Guild Bounty: Slay Creatures";
            task.description = "Eliminate hostile creatures for the guild.";
            break;
        case GuildTaskType::GATHER:
            task.title = "Gathering Mission";
            task.description = "Gather resources for the guild.";
            break;
        case GuildTaskType::CRAFT:
            task.title = "Crafting Order";
            task.description = "Craft items for the guild.";
            break;
        case GuildTaskType::FISH:
            task.title = "Fishing Expedition";
            task.description = "Catch fish for the guild larder.";
            break;
        case GuildTaskType::MINE:
            task.title = "Mining Operation";
            task.description = "Mine ore deposits for the guild.";
            break;
        case GuildTaskType::HERB:
            task.title = "Herb Collection";
            task.description = "Gather herbs for the guild.";
            break;
        case GuildTaskType::SKIN:
            task.title = "Skinning Run";
            task.description = "Collect hides and leather for the guild.";
            break;
        case GuildTaskType::DUNGEON:
            task.title = "Dungeon Expedition";
            task.description = "Clear a dungeon for the guild.";
            break;
        case GuildTaskType::DELIVER:
            task.title = "Guild Bank Deposit";
            task.description = "Deliver gold or items to the guild bank.";
            break;
        case GuildTaskType::SCOUT:
            task.title = "Scouting Mission";
            task.description = "Explore and report on a zone.";
            break;
        default:
            task.title = "Guild Task";
            task.description = "Complete a task for the guild.";
            break;
    }

    // Base rewards
    task.rewardGold = targetCount * 500;  // 5 silver per unit
    task.rewardReputation = targetCount * 5;

    auto& board = _guildBoards[guildId];
    board.guildId = guildId;
    board.activeTasks.push_back(std::move(task));
    board.totalTasksGenerated++;

    TC_LOG_DEBUG("module.playerbot", "GuildTaskManager: Created task #{} ({}) for guild {}",
        board.activeTasks.back().taskId, board.activeTasks.back().title, guildId);

    return board.activeTasks.back().taskId;
}

bool GuildTaskManager::AssignTask(uint32 taskId, Player* bot)
{
    if (!bot)
        return false;

    std::lock_guard<std::mutex> lock(_mutex);

    for (auto& [guildId, board] : _guildBoards)
    {
        for (auto& task : board.activeTasks)
        {
            if (task.taskId == taskId && task.status == GuildTaskStatus::AVAILABLE)
            {
                task.assigneeGuid = bot->GetGUID();
                task.assignedTime = GameTime::GetGameTime();
                task.status = GuildTaskStatus::ASSIGNED;

                TC_LOG_DEBUG("module.playerbot", "GuildTaskManager: Assigned task #{} '{}' to bot {}",
                    taskId, task.title, bot->GetName());
                return true;
            }
        }
    }
    return false;
}

void GuildTaskManager::ReportProgress(uint32 taskId, uint32 incrementCount)
{
    std::lock_guard<std::mutex> lock(_mutex);

    for (auto& [guildId, board] : _guildBoards)
    {
        for (auto& task : board.activeTasks)
        {
            if (task.taskId == taskId &&
                (task.status == GuildTaskStatus::ASSIGNED || task.status == GuildTaskStatus::IN_PROGRESS))
            {
                task.status = GuildTaskStatus::IN_PROGRESS;
                task.currentCount += incrementCount;

                if (task.currentCount >= task.targetCount)
                    task.currentCount = task.targetCount;

                TC_LOG_DEBUG("module.playerbot", "GuildTaskManager: Task #{} progress {}/{}",
                    taskId, task.currentCount, task.targetCount);

                if (task.IsComplete())
                {
                    // Will be completed in next cleanup cycle or explicitly
                    task.status = GuildTaskStatus::COMPLETED;
                    task.completedTime = GameTime::GetGameTime();
                    AwardTaskRewards(task);
                    board.totalTasksCompleted++;

                    TC_LOG_INFO("module.playerbot", "GuildTaskManager: Task #{} '{}' completed by bot in guild {}",
                        taskId, task.title, guildId);
                }
                return;
            }
        }
    }
}

void GuildTaskManager::CompleteTask(uint32 taskId)
{
    std::lock_guard<std::mutex> lock(_mutex);

    for (auto& [guildId, board] : _guildBoards)
    {
        for (auto& task : board.activeTasks)
        {
            if (task.taskId == taskId &&
                task.status != GuildTaskStatus::COMPLETED &&
                task.status != GuildTaskStatus::FAILED &&
                task.status != GuildTaskStatus::EXPIRED)
            {
                task.currentCount = task.targetCount;
                task.status = GuildTaskStatus::COMPLETED;
                task.completedTime = GameTime::GetGameTime();
                AwardTaskRewards(task);
                board.totalTasksCompleted++;

                TC_LOG_INFO("module.playerbot", "GuildTaskManager: Task #{} '{}' force-completed for guild {}",
                    taskId, task.title, guildId);
                return;
            }
        }
    }
}

void GuildTaskManager::AbandonTask(uint32 taskId)
{
    std::lock_guard<std::mutex> lock(_mutex);

    for (auto& [guildId, board] : _guildBoards)
    {
        for (auto& task : board.activeTasks)
        {
            if (task.taskId == taskId &&
                (task.status == GuildTaskStatus::ASSIGNED || task.status == GuildTaskStatus::IN_PROGRESS))
            {
                task.status = GuildTaskStatus::AVAILABLE;
                task.assigneeGuid = ObjectGuid::Empty;
                task.assignedTime = 0;
                task.currentCount = 0;

                TC_LOG_DEBUG("module.playerbot", "GuildTaskManager: Task #{} '{}' abandoned, returning to pool",
                    taskId, task.title);
                return;
            }
        }
    }
}

// ============================================================================
// Metrics
// ============================================================================

GuildTaskManagerMetrics GuildTaskManager::GetMetrics() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    GuildTaskManagerMetrics metrics{};
    metrics.totalGuildsTracked = static_cast<uint32>(_guildBoards.size());

    for (auto const& [guildId, board] : _guildBoards)
    {
        for (auto const& task : board.activeTasks)
        {
            switch (task.status)
            {
                case GuildTaskStatus::AVAILABLE:
                case GuildTaskStatus::ASSIGNED:
                case GuildTaskStatus::IN_PROGRESS:
                    metrics.totalActiveTasks++;
                    break;
                case GuildTaskStatus::COMPLETED:
                    metrics.totalCompletedTasks++;
                    break;
                case GuildTaskStatus::FAILED:
                    metrics.totalFailedTasks++;
                    break;
                case GuildTaskStatus::EXPIRED:
                    metrics.totalExpiredTasks++;
                    break;
            }
        }
        metrics.totalGoldAwarded += board.totalTasksCompleted * 500; // Approximate
    }

    return metrics;
}

// ============================================================================
// Task Generation
// ============================================================================

void GuildTaskManager::GenerateTasksForGuilds()
{
    std::lock_guard<std::mutex> lock(_mutex);

    for (auto& [guildId, board] : _guildBoards)
    {
        // Count active (non-completed) tasks
        uint32 activeCount = 0;
        for (auto const& task : board.activeTasks)
            if (task.status == GuildTaskStatus::AVAILABLE ||
                task.status == GuildTaskStatus::ASSIGNED ||
                task.status == GuildTaskStatus::IN_PROGRESS)
                ++activeCount;

        if (activeCount < MAX_ACTIVE_TASKS_PER_GUILD)
            GenerateTasksForGuild(guildId);
    }
}

void GuildTaskManager::GenerateTasksForGuild(uint32 guildId)
{
    if (_templates.empty())
        return;

    // Count active tasks
    auto& board = _guildBoards[guildId];
    uint32 activeCount = 0;
    for (auto const& task : board.activeTasks)
        if (task.status == GuildTaskStatus::AVAILABLE ||
            task.status == GuildTaskStatus::ASSIGNED ||
            task.status == GuildTaskStatus::IN_PROGRESS)
            ++activeCount;

    uint32 toGenerate = MAX_ACTIVE_TASKS_PER_GUILD > activeCount ?
        MAX_ACTIVE_TASKS_PER_GUILD - activeCount : 0;
    if (toGenerate == 0)
        return;

    // Cap generation per cycle to 3
    toGenerate = std::min(toGenerate, 3u);

    // Build weighted selection from templates
    float totalWeight = 0.0f;
    for (auto const& tmpl : _templates)
        totalWeight += tmpl.weight;

    for (uint32 i = 0; i < toGenerate; ++i)
    {
        // Weighted random selection
        float roll = static_cast<float>(rand()) / RAND_MAX * totalWeight;
        float cumulative = 0.0f;

        for (auto const& tmpl : _templates)
        {
            cumulative += tmpl.weight;
            if (roll <= cumulative)
            {
                GuildTask task = GenerateTaskFromTemplate(guildId, tmpl);
                board.activeTasks.push_back(std::move(task));
                board.totalTasksGenerated++;
                break;
            }
        }
    }

    board.lastGenerationTime = GameTime::GetGameTime();

    TC_LOG_DEBUG("module.playerbot", "GuildTaskManager: Generated {} tasks for guild {} (total active: {})",
        toGenerate, guildId, board.activeTasks.size());
}

GuildTask GuildTaskManager::GenerateTaskFromTemplate(uint32 guildId, GuildTaskTemplate const& tmpl)
{
    GuildTask task;
    task.taskId = _nextTaskId++;
    task.guildId = guildId;
    task.type = tmpl.type;
    task.difficulty = tmpl.difficulty;
    task.status = GuildTaskStatus::AVAILABLE;

    // Randomize count within template range
    uint32 range = tmpl.maxCount > tmpl.minCount ? tmpl.maxCount - tmpl.minCount : 0;
    task.targetCount = tmpl.minCount + (range > 0 ? (rand() % (range + 1)) : 0);
    task.currentCount = 0;

    task.targetEntry = tmpl.targetEntry;
    task.requiredLevel = tmpl.requiredLevel;
    task.requiredSkill = tmpl.requiredSkill;
    task.requiredSkillValue = tmpl.requiredSkillValue;
    task.zoneId = tmpl.zoneId;

    // Scale rewards by count and difficulty
    float difficultyMult = 1.0f + static_cast<uint8>(tmpl.difficulty) * 0.5f;
    task.rewardGold = static_cast<uint32>(tmpl.baseGoldReward * task.targetCount * difficultyMult);
    task.rewardReputation = static_cast<uint32>(tmpl.baseRepReward * task.targetCount * difficultyMult);

    task.createdTime = GameTime::GetGameTime();
    task.deadline = task.createdTime + (tmpl.durationHours * 3600);

    // Use template title/description or generate
    if (!tmpl.titleFormat.empty())
        task.title = tmpl.titleFormat;
    else
    {
        static char const* taskTypeNames[] = {
            "Bounty Hunt", "Gathering Mission", "Crafting Order", "Fishing Expedition",
            "Mining Operation", "Herb Collection", "Skinning Run", "Dungeon Run",
            "Guild Delivery", "Scouting Mission"
        };
        uint8 idx = static_cast<uint8>(tmpl.type);
        task.title = idx < static_cast<uint8>(GuildTaskType::MAX_TASK_TYPE) ?
            taskTypeNames[idx] : "Guild Task";
    }

    if (!tmpl.descriptionFormat.empty())
        task.description = tmpl.descriptionFormat;
    else
        task.description = "Complete this task for the guild.";

    return task;
}

// ============================================================================
// Task Assignment
// ============================================================================

void GuildTaskManager::AssignUnassignedTasks()
{
    std::lock_guard<std::mutex> lock(_mutex);

    for (auto& [guildId, board] : _guildBoards)
    {
        Guild* guild = sGuildMgr->GetGuildById(static_cast<ObjectGuid::LowType>(guildId));
        if (!guild)
            continue;

        for (auto& task : board.activeTasks)
        {
            if (task.status != GuildTaskStatus::AVAILABLE)
                continue;

            // Find best bot for this task
            Player* bestBot = nullptr;
            float bestScore = 0.0f;

            for (auto const& [memberGuid, member] : guild->GetMembers())
            {
                Player* player = ObjectAccessor::FindPlayer(memberGuid);
                if (!player || !player->IsInWorld() || !PlayerBotHooks::IsPlayerBot(player))
                    continue;

                // Check if bot already has max tasks
                uint32 botTaskCount = 0;
                for (auto const& t : board.activeTasks)
                    if (t.assigneeGuid == player->GetGUID() &&
                        (t.status == GuildTaskStatus::ASSIGNED || t.status == GuildTaskStatus::IN_PROGRESS))
                        ++botTaskCount;

                if (botTaskCount >= MAX_TASKS_PER_BOT)
                    continue;

                if (!CanBotDoTask(player, task))
                    continue;

                float score = ScoreBotForTask(player, task);
                if (score > bestScore)
                {
                    bestScore = score;
                    bestBot = player;
                }
            }

            if (bestBot)
            {
                task.assigneeGuid = bestBot->GetGUID();
                task.assignedTime = GameTime::GetGameTime();
                task.status = GuildTaskStatus::ASSIGNED;

                TC_LOG_DEBUG("module.playerbot", "GuildTaskManager: Auto-assigned task #{} '{}' to bot {} (score: {:.2f})",
                    task.taskId, task.title, bestBot->GetName(), bestScore);
            }
        }
    }
}

bool GuildTaskManager::CanBotDoTask(Player* bot, GuildTask const& task) const
{
    if (!bot || !bot->IsAlive())
        return false;

    // Level check
    if (task.requiredLevel > 0 && bot->GetLevel() < task.requiredLevel)
        return false;

    // Skill check (professions)
    if (task.requiredSkill > 0)
    {
        uint16 skillValue = bot->GetSkillValue(task.requiredSkill);
        if (skillValue < task.requiredSkillValue)
            return false;
    }

    // Type-specific checks
    switch (task.type)
    {
        case GuildTaskType::FISH:
            if (bot->GetSkillValue(SKILL_FISHING) == 0)
                return false;
            break;
        case GuildTaskType::MINE:
            if (bot->GetSkillValue(SKILL_MINING) == 0)
                return false;
            break;
        case GuildTaskType::HERB:
            if (bot->GetSkillValue(SKILL_HERBALISM) == 0)
                return false;
            break;
        case GuildTaskType::SKIN:
            if (bot->GetSkillValue(SKILL_SKINNING) == 0)
                return false;
            break;
        case GuildTaskType::DUNGEON:
            // Need to be high enough level for dungeon content
            if (bot->GetLevel() < 15)
                return false;
            break;
        default:
            break;
    }

    return true;
}

float GuildTaskManager::ScoreBotForTask(Player* bot, GuildTask const& task) const
{
    float score = 1.0f;

    // Higher level bots score higher for kill/dungeon tasks
    if (task.type == GuildTaskType::KILL || task.type == GuildTaskType::DUNGEON)
        score += static_cast<float>(bot->GetLevel()) / 80.0f;

    // Profession skill matching
    if (task.requiredSkill > 0)
    {
        uint16 skillValue = bot->GetSkillValue(task.requiredSkill);
        score += static_cast<float>(skillValue) / 600.0f; // Max skill ~600 in WoW 11.x
    }

    // Gathering specialists get bonus
    switch (task.type)
    {
        case GuildTaskType::FISH:
            score += static_cast<float>(bot->GetSkillValue(SKILL_FISHING)) / 600.0f;
            break;
        case GuildTaskType::MINE:
            score += static_cast<float>(bot->GetSkillValue(SKILL_MINING)) / 600.0f;
            break;
        case GuildTaskType::HERB:
            score += static_cast<float>(bot->GetSkillValue(SKILL_HERBALISM)) / 600.0f;
            break;
        case GuildTaskType::SKIN:
            score += static_cast<float>(bot->GetSkillValue(SKILL_SKINNING)) / 600.0f;
            break;
        default:
            break;
    }

    // Idle bots get a slight preference (not in combat, not in dungeon)
    if (!bot->IsInCombat())
        score += 0.5f;

    return score;
}

// ============================================================================
// Task Lifecycle
// ============================================================================

void GuildTaskManager::CleanupExpiredTasks()
{
    std::lock_guard<std::mutex> lock(_mutex);
    uint32 now = GameTime::GetGameTime();

    for (auto& [guildId, board] : _guildBoards)
    {
        board.activeTasks.erase(
            std::remove_if(board.activeTasks.begin(), board.activeTasks.end(),
                [&](GuildTask& task) -> bool
                {
                    // Remove completed/failed tasks older than 1 hour
                    if ((task.status == GuildTaskStatus::COMPLETED ||
                         task.status == GuildTaskStatus::FAILED ||
                         task.status == GuildTaskStatus::EXPIRED) &&
                        task.completedTime > 0 && (now - task.completedTime) > 3600)
                    {
                        return true;
                    }

                    // Expire overdue tasks
                    if (task.IsExpired(now) && task.status != GuildTaskStatus::COMPLETED)
                    {
                        task.status = GuildTaskStatus::EXPIRED;
                        task.completedTime = now;
                        board.totalTasksFailed++;
                        TC_LOG_DEBUG("module.playerbot", "GuildTaskManager: Task #{} '{}' expired for guild {}",
                            task.taskId, task.title, guildId);
                    }

                    return false;
                }),
            board.activeTasks.end());

        board.lastCleanupTime = now;
    }
}

void GuildTaskManager::AwardTaskRewards(GuildTask const& task)
{
    if (task.assigneeGuid.IsEmpty())
        return;

    Player* bot = ObjectAccessor::FindPlayer(task.assigneeGuid);
    if (!bot || !bot->IsInWorld())
        return;

    // Award gold
    if (task.rewardGold > 0)
        bot->ModifyMoney(static_cast<int64>(task.rewardGold));

    // Award item reward if specified
    if (task.rewardItemId > 0 && task.rewardItemCount > 0)
    {
        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(task.rewardItemId);
        if (itemTemplate)
        {
            // Add items to bot inventory
            ItemPosCountVec dest;
            if (bot->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, task.rewardItemId, task.rewardItemCount) == EQUIP_ERR_OK)
                bot->StoreNewItem(dest, task.rewardItemId, true);
        }
    }

    TC_LOG_DEBUG("module.playerbot", "GuildTaskManager: Awarded {} copper and {} reputation to {} for task #{}",
        task.rewardGold, task.rewardReputation, bot->GetName(), task.taskId);
}

// ============================================================================
// Template Loading
// ============================================================================

void GuildTaskManager::LoadTemplatesFromDB()
{
    QueryResult result = sPlayerbotDatabase->Query(
        "SELECT template_id, task_type, difficulty, title_format, description_format, "
        "target_entry, min_count, max_count, required_level, required_skill, "
        "required_skill_value, zone_id, base_gold_reward, base_rep_reward, "
        "duration_hours, weight FROM playerbot_guild_task_templates");

    if (!result)
    {
        TC_LOG_INFO("module.playerbot", "GuildTaskManager: No task templates found in database");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        GuildTaskTemplate tmpl;

        tmpl.templateId         = fields[0].GetUInt32();
        tmpl.type               = static_cast<GuildTaskType>(fields[1].GetUInt8());
        tmpl.difficulty         = static_cast<GuildTaskDifficulty>(fields[2].GetUInt8());
        tmpl.titleFormat        = fields[3].GetString();
        tmpl.descriptionFormat  = fields[4].GetString();
        tmpl.targetEntry        = fields[5].GetUInt32();
        tmpl.minCount           = fields[6].GetUInt32();
        tmpl.maxCount           = fields[7].GetUInt32();
        tmpl.requiredLevel      = fields[8].GetUInt32();
        tmpl.requiredSkill      = fields[9].GetUInt32();
        tmpl.requiredSkillValue = fields[10].GetUInt32();
        tmpl.zoneId             = fields[11].GetUInt32();
        tmpl.baseGoldReward     = fields[12].GetUInt32();
        tmpl.baseRepReward      = fields[13].GetUInt32();
        tmpl.durationHours      = fields[14].GetUInt32();
        tmpl.weight             = fields[15].GetFloat();

        if (static_cast<uint8>(tmpl.type) >= static_cast<uint8>(GuildTaskType::MAX_TASK_TYPE))
        {
            TC_LOG_ERROR("module.playerbot", "GuildTaskManager: Template {} has invalid type {}, skipping",
                tmpl.templateId, static_cast<uint8>(tmpl.type));
            continue;
        }

        _templates.push_back(std::move(tmpl));
        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("module.playerbot", "GuildTaskManager: Loaded {} task templates from database", count);
}

void GuildTaskManager::LoadDefaultTemplates()
{
    auto addTemplate = [this](GuildTaskType type, GuildTaskDifficulty diff,
                              char const* title, char const* desc,
                              uint32 minCount, uint32 maxCount,
                              uint32 reqLevel, uint32 reqSkill, uint32 reqSkillVal,
                              uint32 baseGold, uint32 baseRep, uint32 hours, float weight)
    {
        GuildTaskTemplate tmpl;
        tmpl.templateId = static_cast<uint32>(_templates.size()) + 1;
        tmpl.type = type;
        tmpl.difficulty = diff;
        tmpl.titleFormat = title;
        tmpl.descriptionFormat = desc;
        tmpl.minCount = minCount;
        tmpl.maxCount = maxCount;
        tmpl.requiredLevel = reqLevel;
        tmpl.requiredSkill = reqSkill;
        tmpl.requiredSkillValue = reqSkillVal;
        tmpl.baseGoldReward = baseGold;
        tmpl.baseRepReward = baseRep;
        tmpl.durationHours = hours;
        tmpl.weight = weight;
        _templates.push_back(std::move(tmpl));
    };

    // KILL tasks - any class can do these
    addTemplate(GuildTaskType::KILL, GuildTaskDifficulty::EASY,
        "Pest Control", "Eliminate hostile creatures near guild territory.",
        5, 15, 10, 0, 0, 200, 3, 24, 2.0f);

    addTemplate(GuildTaskType::KILL, GuildTaskDifficulty::NORMAL,
        "Bounty Hunt", "Track and eliminate dangerous creatures.",
        10, 30, 30, 0, 0, 500, 5, 24, 1.5f);

    addTemplate(GuildTaskType::KILL, GuildTaskDifficulty::HARD,
        "Elite Extermination", "Slay elite creatures threatening our lands.",
        3, 8, 60, 0, 0, 1000, 10, 48, 0.8f);

    // GATHER tasks
    addTemplate(GuildTaskType::GATHER, GuildTaskDifficulty::EASY,
        "Supply Run", "Gather general supplies for the guild.",
        5, 20, 5, 0, 0, 150, 2, 24, 1.5f);

    // FISH tasks
    addTemplate(GuildTaskType::FISH, GuildTaskDifficulty::EASY,
        "Gone Fishing", "Catch fish for the guild feast.",
        5, 15, 10, SKILL_FISHING, 1, 200, 3, 24, 1.0f);

    addTemplate(GuildTaskType::FISH, GuildTaskDifficulty::NORMAL,
        "Deep Sea Fishing", "Catch rare fish from challenging waters.",
        10, 25, 40, SKILL_FISHING, 200, 500, 5, 48, 0.7f);

    // MINE tasks
    addTemplate(GuildTaskType::MINE, GuildTaskDifficulty::EASY,
        "Ore Collection", "Mine ore for guild crafters.",
        5, 15, 10, SKILL_MINING, 1, 200, 3, 24, 1.0f);

    addTemplate(GuildTaskType::MINE, GuildTaskDifficulty::NORMAL,
        "Deep Mining Expedition", "Mine rare minerals from deep veins.",
        10, 20, 40, SKILL_MINING, 200, 500, 5, 48, 0.7f);

    // HERB tasks
    addTemplate(GuildTaskType::HERB, GuildTaskDifficulty::EASY,
        "Herb Gathering", "Pick herbs for the guild alchemist.",
        5, 15, 10, SKILL_HERBALISM, 1, 200, 3, 24, 1.0f);

    addTemplate(GuildTaskType::HERB, GuildTaskDifficulty::NORMAL,
        "Rare Herb Expedition", "Gather rare herbs from dangerous areas.",
        10, 20, 40, SKILL_HERBALISM, 200, 500, 5, 48, 0.7f);

    // SKIN tasks
    addTemplate(GuildTaskType::SKIN, GuildTaskDifficulty::EASY,
        "Leather Procurement", "Skin creatures for guild leatherworkers.",
        5, 15, 10, SKILL_SKINNING, 1, 200, 3, 24, 0.8f);

    // CRAFT tasks
    addTemplate(GuildTaskType::CRAFT, GuildTaskDifficulty::NORMAL,
        "Crafting Commission", "Craft items for the guild.",
        3, 8, 30, 0, 0, 500, 5, 48, 0.6f);

    // DUNGEON tasks
    addTemplate(GuildTaskType::DUNGEON, GuildTaskDifficulty::HARD,
        "Dungeon Expedition", "Clear a dungeon for guild prestige.",
        1, 1, 15, 0, 0, 2000, 15, 72, 0.5f);

    addTemplate(GuildTaskType::DUNGEON, GuildTaskDifficulty::ELITE,
        "Heroic Dungeon Challenge", "Complete a heroic dungeon run.",
        1, 1, 70, 0, 0, 5000, 30, 72, 0.3f);

    // DELIVER tasks
    addTemplate(GuildTaskType::DELIVER, GuildTaskDifficulty::EASY,
        "Guild Bank Deposit", "Deposit gold into the guild bank.",
        1, 1, 10, 0, 0, 0, 5, 24, 1.2f);

    // SCOUT tasks
    addTemplate(GuildTaskType::SCOUT, GuildTaskDifficulty::EASY,
        "Zone Patrol", "Explore and patrol a zone.",
        1, 1, 10, 0, 0, 300, 3, 24, 0.8f);

    addTemplate(GuildTaskType::SCOUT, GuildTaskDifficulty::NORMAL,
        "Contested Territory Scout", "Scout enemy-controlled zones.",
        1, 1, 40, 0, 0, 600, 5, 48, 0.5f);

    TC_LOG_INFO("module.playerbot", "GuildTaskManager: Loaded {} default task templates", _templates.size());
}

} // namespace Playerbot
