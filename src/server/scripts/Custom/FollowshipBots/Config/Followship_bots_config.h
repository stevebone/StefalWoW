#pragma once

#include "Define.h"

struct FollowshipBotsConfig
{
    static bool  configFSBEnabled;
    static int64 configFSBPricePerLevel;
    static int64 configFSBPermanentPricePerLevel;
    static int32 configFSBHireDuration1;
    static int32 configFSBHireDuration2;
    static int32 configFSBHireDuration3;
    static bool  configFSBUseCustomRegen;
    static uint32 configFSBChatterRate;

    static void Load();
};
