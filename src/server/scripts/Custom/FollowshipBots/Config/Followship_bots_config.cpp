#include "Config.h"

#include "Followship_bots_config.h"


bool  FollowshipBotsConfig::configFSBEnabled = false;
int64 FollowshipBotsConfig::configFSBPricePerLevel = 10;
int64 FollowshipBotsConfig::configFSBPermanentPricePerLevel = 10000;
int32 FollowshipBotsConfig::configFSBHireDuration1 = 1;
int32 FollowshipBotsConfig::configFSBHireDuration2 = 2;
int32 FollowshipBotsConfig::configFSBHireDuration3 = 3;
bool  FollowshipBotsConfig::configFSBUseCustomRegen = false;
uint32 FollowshipBotsConfig::configFSBChatterRate = 10;

void FollowshipBotsConfig::Load()
{
    configFSBEnabled =
        sConfigMgr->GetBoolDefault("Followship.Bots.Enabled", false);

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
        sConfigMgr->GetBoolDefault("Followship.Bots.UseCustomRegen", false);

    configFSBChatterRate =
        sConfigMgr->GetIntDefault("Followship.Bots.ChatterRate", 10);
}
