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

#pragma once

#include "Define.h"

struct FollowshipBotsConfig
{
    static bool configFSBEnabled;
    static bool configFSBUseCustomRegen;
    static bool configFSBUseOOCActions;
    static bool configFSBUseICPotions;
    static bool configFSBUseAFKActions;
    static bool configFSBUseChatChannels;
    static uint32 configFSBAFKInterval;
    static uint32 configFSBAFKChance;

    static int64 configFSBPricePerLevel;
    static int64 configFSBPermanentPricePerLevel;
    static int32 configFSBHireDuration1;
    static int32 configFSBHireDuration2;
    static int32 configFSBHireDuration3;
    static uint32 configFSBChatterRate;
    static uint32 configFSBChatChannelsRate;
    static int64 configFSBChatChannelsInterval;

    static bool configFSBGeneralReplyEnabled;
    static uint32 configFSBGeneralReplyChance;
    static uint32 configFSBGeneralReplyCooldownMs;
    static uint32 configFSBBotChatMemorySize;

    static float configFSBHealthRate;
    static float configFSBPowerRate;
    static float configFSBArmorRate;
    static float configFSBDamageRate;
    static float configFSBHealthRegenRate;
    static float configFSBPowerRegenRate;

    static bool configFSBLlamaAIEnabled;
    static std::string configFSBLlamaAIHost;
    static int32 configFSBLlamaAIPort;
    static std::string configFSBLlamaAIEndpoint;
    static std::string configFSBLlamaAIModel;
    static std::string configFSBLlamaAIApiKey;
    static int32 configFSBLlamaAITimeoutMs;
    static int32 configFSBLlamaAIMaxTokens;
    static float configFSBLlamaAITemperature;
    static float configFSBLlamaAITopP;
    static float configFSBLlamaAIFrequencyPenalty;
    static float configFSBLlamaAIPresencePenalty;

    static void Load();
};
