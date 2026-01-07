// followship_bots_config.h
#pragma once

#include "Config.h"

struct FollowshipBotsConfig
{
    // These are static so all scripts can access them without creating an instance
    static bool configFSBEnabled;

    static int64 configFSBPricePerLevel;
    static int64 configFSBPermanentPricePerLevel;

    static int32 configFSBHireDuration1;
    static int32 configFSBHireDuration2;
    static int32 configFSBHireDuration3;

    // Call once at startup or first AI initialization
    static void Load()
    {
        configFSBEnabled = sConfigMgr->GetBoolDefault("Followship.Bots.Enabled", false);

        configFSBPricePerLevel = sConfigMgr->GetInt64Default("Followship.Bots.BaseCostPerLevel", 10);
        configFSBPermanentPricePerLevel = sConfigMgr->GetInt64Default("Followship.Bots.PermanentCostPerLevel", 10000);

        configFSBHireDuration1 = sConfigMgr->GetIntDefault("Followship.Bot.HireDuration1", 1);
        configFSBHireDuration2 = sConfigMgr->GetIntDefault("Followship.Bot.HireDuration2", 2);
        configFSBHireDuration3 = sConfigMgr->GetIntDefault("Followship.Bot.HireDuration3", 3);
    }
};
