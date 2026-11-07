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

#include "GameObject.h"
#include "GameObjectAI.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Position.h"
#include "QuestDef.h"
#include "ScriptMgr.h"
#include "SpawnData.h"
#include "TemporarySummon.h"

#include "Custom_VaultWardens_Defines.h"

namespace Scripts::Custom::VaultOfTheWardens
{
    using namespace Creatures;
    using namespace GameObjects;
    using namespace Quests;
    using namespace Texts;
    using namespace KillCredits;

    struct CellPrisoner
    {
        ObjectGuid::LowType SpawnId;
        uint32 Entry;
        Position SpawnPos;
    };

    // 244588 - Warden Cell -> freed Illidari (38690 "Rise of the Illidari").
    // Mapping verified in the retail sniff: each CMSG_GAME_OBJ_REPORT_USE guid
    // resolves to a cell position, the subsequent QUEST_UPDATE_ADD_CREDIT names
    // the prisoner, and their summon sits ~3-4y in front of the cell.
    static constexpr CellPrisoner CellPrisoners[] =
    {
        { 70012705, KorvasBloodthorn,  { 4290.173f, -457.68f,  -274.635f, 5.497787f  } }, // @OGUID+101
        { 70012712, MariusFelbane,     { 4280.579f, -457.95f,  -287.482f, 4.6251225f } }, // @OGUID+108
        { 70012717, JaceDarkweaver,    { 4345.944f, -427.24f,  -281.401f, 4.4331365f } }, // @OGUID+113
        { 70012719, AshaRavensong,     { 4361.385f, -457.887f, -274.634f, 3.9269907f } }, // @OGUID+115
        { 70012761, FalaraNightsong,   { 4305.78f,  -559.222f, -281.4f,   1.2740903f } }, // @OGUID+157
        { 70012764, LyanaDarksorrow,   { 4391.804f, -513.403f, -281.4f,   2.8274333f } }, // @OGUID+160
        { 70012774, AllariTheSouleater,{ 4361.7f,   -528.955f, -274.635f, 2.3561945f } }, // @OGUID+170
        { 70012780, CyanaNightglaive,  { 4280.509f, -528.168f, -274.669f, 1.6231562f } }, // @OGUID+176
    };

    CellPrisoner const* GetCellPrisoner(ObjectGuid::LowType spawnId)
    {
        for (CellPrisoner const& prisoner : CellPrisoners)
            if (prisoner.SpawnId == spawnId)
                return &prisoner;
        return nullptr;
    }

    // 244588 - Warden Cell (8 quest-interactable cells in the hall, 38690)
    //
    // Each cell's spawn tracking is bound 1:1 to its named Illidari objective
    // (280065-280072), so the used-state derives from that cell's own prisoner
    // credit. The client sends both CMSG_GAME_OBJ_USE and
    // CMSG_GAME_OBJ_REPORT_USE per click, so the used-state check gates both:
    //   - OnGossipHello aborts GameObject::Use() on used cells, before the
    //     goober branch can re-cast the playerCast spell that carries the
    //     kill credit
    //   - OnReportUse credits the cell's named-prisoner objective with the
    //     GO guid as victim (also marks the cell's spawn tracking complete:
    //     animkit 9340 + worldeffect 5424 + GO_FLAG_IN_USE, force update
    //     sent) and summons the prisoner as a private, player-owned temp
    //     summon
    struct go_vault_warden_cell : public GameObjectAI
    {
        go_vault_warden_cell(GameObject* go) : GameObjectAI(go) { }

        bool OnGossipHello(Player* player) override
        {
            // Consume CMSG_GAME_OBJ_USE on cells this player already opened.
            // Returning true aborts GameObject::Use() before the goober branch,
            // so the playerCast spell (which carries the kill credit) never
            // runs a second time.
            return IsUsedBy(player);
        }

        bool OnReportUse(Player* player) override
        {
            CellPrisoner const* prisoner = GetCellPrisoner(me->GetSpawnId());
            if (!prisoner)
                return false;

            if (player->GetQuestStatus(RiseOfTheIllidari) != QUEST_STATUS_INCOMPLETE)
                return true;

            // cell already freed by this player - the client grays it out via
            // GO_FLAG_IN_USE; this guards double-click race duplicates (checked
            // directly rather than via IsSpawnTrackedGooberUsedBy, which always
            // returns false for GameMasters)
            if (IsUsedBy(player))
                return true;

            player->KilledMonsterCredit(prisoner->Entry, me->GetGUID());
            player->SummonCreature(prisoner->Entry, prisoner->SpawnPos, TEMPSUMMON_TIMED_DESPAWN, 90s, 0, 0, player->GetGUID());
            return true;
        }

    private:
        bool IsUsedBy(Player const* player) const
        {
            SpawnMetadata const* data = sObjectMgr->GetSpawnMetadata(SPAWN_TYPE_GAMEOBJECT, me->GetSpawnId());
            return data && data->spawnTrackingData && !data->spawnTrackingQuestObjectives.empty()
                && player->GetSpawnTrackingStateByObjectives(data->spawnTrackingData->SpawnTrackingId, data->spawnTrackingQuestObjectives) == SpawnTrackingState::Complete;
        }
    };

}

void AddSC_custom_vault_wardens_gameobjects()
{
    using namespace Scripts::Custom::VaultOfTheWardens;

    RegisterGameObjectAI(go_vault_warden_cell);
}
