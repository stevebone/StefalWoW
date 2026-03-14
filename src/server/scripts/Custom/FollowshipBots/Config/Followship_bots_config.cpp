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

#include "Config.h"

#include "Followship_bots_config.h"


bool  FollowshipBotsConfig::configFSBEnabled = false;
int64 FollowshipBotsConfig::configFSBPricePerLevel = 10;
int64 FollowshipBotsConfig::configFSBPermanentPricePerLevel = 10000;
int32 FollowshipBotsConfig::configFSBHireDuration1 = 1;
int32 FollowshipBotsConfig::configFSBHireDuration2 = 2;
int32 FollowshipBotsConfig::configFSBHireDuration3 = 3;

bool  FollowshipBotsConfig::configFSBUseOOCActions = true;
bool  FollowshipBotsConfig::configFSBUseICPotions = true;
bool  FollowshipBotsConfig::configFSBUseCustomRegen = true;
bool  FollowshipBotsConfig::configFSBUseChatChannels = true;

uint32 FollowshipBotsConfig::configFSBChatterRate = 10;
uint32 FollowshipBotsConfig::configFSBChatChannelsRate = 10;
int64 FollowshipBotsConfig::configFSBChatChannelsInterval = 10000;

float FollowshipBotsConfig::configFSBHealthRate = 1.f;
float FollowshipBotsConfig::configFSBPowerRate = 1.f;
float FollowshipBotsConfig::configFSBDamageRate = 1.f;
float FollowshipBotsConfig::configFSBArmorRate = 0.4f;

void FollowshipBotsConfig::Load()
{
    configFSBEnabled =
        sConfigMgr->GetBoolDefault("Followship.Bots.Enabled", false);

    configFSBUseChatChannels =
        sConfigMgr->GetBoolDefault("Followship.Bots.UseChatChannels", true);

    configFSBPricePerLevel =
        sConfigMgr->GetInt64Default("Followship.Bots.BaseCostPerLevel", 10);

    configFSBPermanentPricePerLevel =
        sConfigMgr->GetInt64Default("Followship.Bots.PermanentCostPerLevel", 10000);

    configFSBHireDuration1 =
        sConfigMgr->GetIntDefault("Followship.Bot.HireDuration1", 1);

    configFSBHireDuration2 =
        sConfigMgr->GetIntDefault("Followship.Bot.HireDuration2", 2);

    configFSBHireDuration3 =
        sConfigMgr->GetIntDefault("Followship.Bot.HireDuration3", 3);

    configFSBUseCustomRegen =
        sConfigMgr->GetBoolDefault("Followship.Bots.UseCustomRegen", true);

    configFSBUseOOCActions =
        sConfigMgr->GetBoolDefault("Followship.Bots.UseOOCActions", true);

    configFSBUseICPotions =
        sConfigMgr->GetBoolDefault("Followship.Bots.UseICPotions", true);

    configFSBChatterRate =
        sConfigMgr->GetIntDefault("Followship.Bots.ChatterRate", 10);

    configFSBChatChannelsInterval =
        sConfigMgr->GetInt64Default("Followship.Bots.ChatChannelsInterval", 10000);

    configFSBChatChannelsRate =
        sConfigMgr->GetIntDefault("Followship.Bots.ChatChannelsRate", 10);

    configFSBArmorRate =
        sConfigMgr->GetFloatDefault("Followship.Bots.ArmorRate", 0.4f);

    configFSBHealthRate =
        sConfigMgr->GetFloatDefault("Followship.Bots.HealthRate", 1.f);

    configFSBPowerRate =
        sConfigMgr->GetFloatDefault("Followship.Bots.PowerRate", 1.f);

    configFSBDamageRate =
        sConfigMgr->GetFloatDefault("Followship.Bots.DamageRate", 1.f);
}
