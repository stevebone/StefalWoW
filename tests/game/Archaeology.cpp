/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 */

#include "tc_catch2.h"

#include "ArchaeologyMgr.h"
#include "DB2Stores.h"
#include "DummyData.h"
#include "SpellPackets.h"
#include <initializer_list>
#include <limits>

struct ArchaeologyMgrTestAccess
{
    static void SetEnabledBranches(std::initializer_list<uint32> branchIds)
    {
        sArchaeologyMgr->_findGameObjectsByBranch.clear();
        for (uint32 branchId : branchIds)
            sArchaeologyMgr->_findGameObjectsByBranch.emplace(branchId, 1);
    }
};

namespace
{
    constexpr uint32 BranchId = 423;
    constexpr uint32 FragmentCurrencyId = 1534;
    constexpr uint32 KeystoneItemId = 154989;

    UnitTestDataLoader::DB2<ResearchBranchEntry, &ResearchBranchEntry::ID> ResearchBranches(sResearchBranchStore);
    UnitTestDataLoader::DB2<ResearchProjectEntry, &ResearchProjectEntry::ID> ResearchProjects(sResearchProjectStore);
    UnitTestDataLoader::DB2<CurrencyTypesEntry, &CurrencyTypesEntry::ID> Currencies(sCurrencyTypesStore);
    UnitTestDataLoader::DB2<ItemSparseEntry, &ItemSparseEntry::ID> Items(sItemSparseStore);

    WorldPackets::Spells::SpellWeight Weight(uint32 type, int32 id, uint32 quantity)
    {
        return { .Type = type, .ID = id, .Quantity = quantity };
    }

    void LoadArchaeologySolveData()
    {
        ArchaeologyMgrTestAccess::SetEnabledBranches({ BranchId });

        static bool loaded = false;
        if (loaded)
            return;
        loaded = true;

        {
            auto loader = ResearchBranches.Loader();

            ResearchBranchEntry& branch = loader.Add();
            branch.ID = BranchId;
            branch.CurrencyID = FragmentCurrencyId;
            branch.ItemID = KeystoneItemId;

            ResearchBranchEntry& mismatchedCategoryBranch = loader.Add();
            mismatchedCategoryBranch.ID = 424;
            mismatchedCategoryBranch.CurrencyID = 1535;
            mismatchedCategoryBranch.ItemID = 154990;

            ResearchBranchEntry& orcBranch = loader.Add();
            orcBranch.ID = 6;
            orcBranch.CurrencyID = 397;
            orcBranch.ItemID = 64392;
        }

        {
            auto loader = Currencies.Loader();

            CurrencyTypesEntry& fragments = loader.Add();
            fragments.ID = FragmentCurrencyId;
            fragments.SpellWeight = 1;
            fragments.SpellCategory = 57;

            CurrencyTypesEntry& mismatchedCategoryFragments = loader.Add();
            mismatchedCategoryFragments.ID = 1535;
            mismatchedCategoryFragments.SpellWeight = 1;
            mismatchedCategoryFragments.SpellCategory = 57;

            CurrencyTypesEntry& orcFragments = loader.Add();
            orcFragments.ID = 397;
            orcFragments.SpellWeight = 1;
            orcFragments.SpellCategory = 0;
        }

        {
            auto loader = Items.Loader();

            ItemSparseEntry& keystone = loader.Add();
            keystone.ID = KeystoneItemId;
            keystone.SpellWeight = 12;
            keystone.SpellWeightCategory = 57;

            ItemSparseEntry& mismatchedCategoryKeystone = loader.Add();
            mismatchedCategoryKeystone.ID = 154990;
            mismatchedCategoryKeystone.SpellWeight = 12;
            mismatchedCategoryKeystone.SpellWeightCategory = 58;
        }

        {
            auto loader = ResearchProjects.Loader();

            ResearchProjectEntry& zeroSocket = loader.Add();
            zeroSocket.ID = 2520;
            zeroSocket.SpellID = 5000;
            zeroSocket.ResearchBranchID = BranchId;
            zeroSocket.NumSockets = 0;
            zeroSocket.RequiredWeight = 65;

            ResearchProjectEntry& oneSocket = loader.Add();
            oneSocket.ID = 2521;
            oneSocket.SpellID = 5001;
            oneSocket.ResearchBranchID = BranchId;
            oneSocket.NumSockets = 1;
            oneSocket.RequiredWeight = 65;

            ResearchProjectEntry& capturedTwoSocket = loader.Add();
            capturedTwoSocket.ID = 2522;
            capturedTwoSocket.SpellID = 5002;
            capturedTwoSocket.ResearchBranchID = BranchId;
            capturedTwoSocket.NumSockets = 2;
            capturedTwoSocket.RequiredWeight = 65;

            ResearchProjectEntry& fourSocket = loader.Add();
            fourSocket.ID = 2523;
            fourSocket.SpellID = 5003;
            fourSocket.ResearchBranchID = BranchId;
            fourSocket.NumSockets = 4;
            fourSocket.RequiredWeight = 89;

            ResearchProjectEntry& mismatchedCategory = loader.Add();
            mismatchedCategory.ID = 2524;
            mismatchedCategory.SpellID = 5004;
            mismatchedCategory.ResearchBranchID = 424;
            mismatchedCategory.NumSockets = 1;
            mismatchedCategory.RequiredWeight = 65;

            ResearchProjectEntry& missingBranch = loader.Add();
            missingBranch.ID = 2525;
            missingBranch.SpellID = 5005;
            missingBranch.ResearchBranchID = 425;
            missingBranch.NumSockets = 0;
            missingBranch.RequiredWeight = 65;

            for (uint32 branchId : { 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 27u, 29u, 229u, 231u })
            {
                ResearchProjectEntry& project = loader.Add();
                project.ID = 6000 + branchId;
                project.SpellID = 7000 + int32(branchId);
                project.ResearchBranchID = branchId;
                project.NumSockets = 0;
                project.RequiredWeight = 45;
            }
        }
    }
}

TEST_CASE("Archaeology project selection follows enabled branch policy", "[Archaeology]")
{
    LoadArchaeologySolveData();
    // Twelve-branch introduction policy (Phase 3A): Cataclysm nine-branch set plus Mantid /
    // Pandaren / Mogu. TEMPLATE digsites 949/951 are excluded in dig-site SQL (not rolled —
    // IsSurveyableDigSite requires archaeology_dig_site + polygon). Out-of-policy rejection below.
    ArchaeologyMgrTestAccess::SetEnabledBranches({ 1, 2, 3, 4, 5, 6, 7, 8, 27, 29, 229, 231 });

    for (uint32 branchId : { 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 27u, 29u, 229u, 231u })
    {
        CHECK(sArchaeologyMgr->IsResearchBranchEnabled(branchId));
        CHECK(sArchaeologyMgr->RollResearchProject(branchId, {}) == 6000 + branchId);
    }

    CHECK_FALSE(sArchaeologyMgr->IsResearchBranchEnabled(423));
    CHECK(sArchaeologyMgr->RollResearchProject(423, {}) == 0);
    // Still out of policy when MoP is enabled (WoD+/BFA branches stay off).
    CHECK_FALSE(sArchaeologyMgr->IsResearchBranchEnabled(315));
    CHECK(sArchaeologyMgr->RollResearchProject(315, {}) == 0);

    // Orc solve plan accepted when branch 6 is enabled (policy + DB2 weights).
    {
        auto plan = sArchaeologyMgr->BuildSolvePlan(7006, { Weight(1, 397, 45) });
        REQUIRE(plan.has_value());
        CHECK(plan->BranchID == 6);
        CHECK(plan->FragmentCount == 45);
    }
}

TEST_CASE("Archaeology solve plans use DB2 weights and project socket caps", "[Archaeology]")
{
    LoadArchaeologySolveData();
    ArchaeologyMgrTestAccess::SetEnabledBranches({ BranchId });

    SECTION("Currency-only zero-socket solve")
    {
        auto plan = sArchaeologyMgr->BuildSolvePlan(5000, { Weight(1, FragmentCurrencyId, 65) });
        REQUIRE(plan.has_value());
        CHECK(plan->FragmentCount == 65);
        CHECK(plan->KeystoneCount == 0);
    }

    SECTION("One-socket solve")
    {
        auto plan = sArchaeologyMgr->BuildSolvePlan(5001,
            { Weight(2, KeystoneItemId, 1), Weight(1, FragmentCurrencyId, 53) });
        REQUIRE(plan.has_value());
        CHECK(plan->FragmentCount == 53);
        CHECK(plan->KeystoneCount == 1);
    }

    SECTION("Captured two-socket solve")
    {
        auto plan = sArchaeologyMgr->BuildSolvePlan(5002,
            { Weight(2, KeystoneItemId, 2), Weight(1, FragmentCurrencyId, 41) });
        REQUIRE(plan.has_value());
        CHECK(plan->ProjectID == 2522);
        CHECK(plan->FragmentCount == 41);
        CHECK(plan->KeystoneCount == 2);
        CHECK(plan->RequiredWeight == 65);
    }

    SECTION("Four sockets are represented by item quantity")
    {
        auto plan = sArchaeologyMgr->BuildSolvePlan(5003,
            { Weight(2, KeystoneItemId, 4), Weight(1, FragmentCurrencyId, 41) });
        REQUIRE(plan.has_value());
        CHECK(plan->KeystoneCount == 4);
        CHECK(plan->FragmentCount == 41);
    }
}

TEST_CASE("Archaeology solve plans reject invalid data-backed resource shapes", "[Archaeology]")
{
    LoadArchaeologySolveData();
    ArchaeologyMgrTestAccess::SetEnabledBranches({ BranchId });

    CHECK_FALSE(sArchaeologyMgr->BuildSolvePlan(5002,
        { Weight(2, 154990, 2), Weight(1, FragmentCurrencyId, 41) }).has_value());
    CHECK_FALSE(sArchaeologyMgr->BuildSolvePlan(5002,
        { Weight(2, KeystoneItemId, 2), Weight(1, 1535, 41) }).has_value());
    CHECK_FALSE(sArchaeologyMgr->BuildSolvePlan(5004,
        { Weight(2, 154990, 1), Weight(1, 1535, 53) }).has_value());
    CHECK_FALSE(sArchaeologyMgr->BuildSolvePlan(5002,
        { Weight(2, KeystoneItemId, 0), Weight(1, FragmentCurrencyId, 65) }).has_value());
    CHECK_FALSE(sArchaeologyMgr->BuildSolvePlan(5002,
        { Weight(2, KeystoneItemId, 3), Weight(1, FragmentCurrencyId, 29) }).has_value());
    CHECK_FALSE(sArchaeologyMgr->BuildSolvePlan(5002,
        { Weight(2, KeystoneItemId, 2), Weight(1, FragmentCurrencyId, 40) }).has_value());
    CHECK_FALSE(sArchaeologyMgr->BuildSolvePlan(5002,
        { Weight(2, KeystoneItemId, 2), Weight(1, FragmentCurrencyId, 42) }).has_value());
    CHECK_FALSE(sArchaeologyMgr->BuildSolvePlan(5005,
        { Weight(1, FragmentCurrencyId, 65) }).has_value());
    CHECK_FALSE(sArchaeologyMgr->BuildSolvePlan(9999,
        { Weight(1, FragmentCurrencyId, 65) }).has_value());
    CHECK_FALSE(sArchaeologyMgr->BuildSolvePlan(5002,
        { Weight(1, FragmentCurrencyId, std::numeric_limits<uint32>::max()),
          Weight(1, FragmentCurrencyId, std::numeric_limits<uint32>::max()),
          Weight(1, FragmentCurrencyId, std::numeric_limits<uint32>::max()) }).has_value());
}
