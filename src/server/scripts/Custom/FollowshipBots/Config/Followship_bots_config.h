#pragma once

#include "Define.h"

struct FollowshipBotsConfig
{
    static bool configFSBEnabled;
    static bool configFSBUseCustomRegen;
    static bool configFSBUseOOCActions;
    static bool configFSBUseICPotions;

    static int64 configFSBPricePerLevel;
    static int64 configFSBPermanentPricePerLevel;
    static int32 configFSBHireDuration1;
    static int32 configFSBHireDuration2;
    static int32 configFSBHireDuration3;
    static uint32 configFSBChatterRate;

    static float configFSBArmorRate;

    static void Load();
};
