/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
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
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "Log.h"
#include "Player.h"
#include "PlayerChoice.h"
#include "SpellDefines.h"

/*
 * Covenant selection ("Which covenant do you want to join?").
 *
 * The client drives the covenant sanctum choice through PlayerChoice 644, launched by
 * SPELL_EFFECT_LAUNCH_QUEST_CHOICE (spell 343884, MiscValue 644) off the Oribos UI-link
 * gameobject. It is a plain player choice, so the server side is entirely
 * PlayerChoiceScript::OnResponse - CMSG_CHOICE_RESPONSE only validates and forwards.
 *
 * PlayerChoice 644 has eight responses, paired two per covenant by GroupID: one
 * "Preview Covenant" response (client-only; C_CovenantPreview.GetCovenantInfoForPlayerChoiceResponseID
 * looks it up in UICovenantPreview.db2, which stores exactly these four ids) and one "Join"
 * response that actually commits the choice.
 *
 *   GroupID | Preview response (UICovenantPreview) | Join response | Covenant.db2
 *   --------+--------------------------------------+---------------+--------------------
 *      4    | 2708                                 | 2689          | 1 Kyrian
 *      1    | 2686                                 | 2702          | 2 Venthyr
 *      3    | 2707                                 | 2688          | 3 Night Fae
 *      2    | 2706                                 | 2687          | 4 Necrolord
 *
 * Only the four join responses are mapped below; every other response id (the four previews
 * included) falls through the switch and is a no-op, so previewing never joins a covenant.
 *
 * Joining is done by casting the covenant's own reward spell rather than by poking
 * Player::SetActiveCovenant directly, so the Blizzlike side effects happen as on retail. Each
 * spell carries, on top of SPELL_EFFECT_SET_COVENANT (272) with the covenant id as MiscValue,
 * a SPELL_EFFECT_QUEST_COMPLETE (16) for its own covenant-choice quest and three
 * SPELL_EFFECT_QUEST_FAIL (139) for the covenants that were turned down:
 *
 *   299204 "Kyrian Covenant"    -> complete 56066, fail 56067/56068/56069, set covenant 1
 *   299205 "Venthyr Covenant"   -> complete 56067, fail 56066/56068/56069, set covenant 2
 *   299206 "Night Fae Covenant" -> complete 56068, fail 56066/56067/56069, set covenant 3
 *   299207 "Necrolord Covenant" -> complete 56069, fail 56066/56067/56068, set covenant 4
 *
 * All five effects target TARGET_UNIT_CASTER, so a self-cast delivers them.
 */
enum CovenantChoiceData
{
    // PlayerChoice
    PLAYER_CHOICE_COVENANT_SELECTION            = 644,

    // Join responses of PlayerChoice 644
    PLAYERCHOICE_RESPONSE_JOIN_KYRIAN           = 2689,
    PLAYERCHOICE_RESPONSE_JOIN_VENTHYR          = 2702,
    PLAYERCHOICE_RESPONSE_JOIN_NIGHT_FAE        = 2688,
    PLAYERCHOICE_RESPONSE_JOIN_NECROLORD        = 2687,

    // Covenant reward spells (SPELL_EFFECT_SET_COVENANT)
    SPELL_KYRIAN_COVENANT                       = 299204,
    SPELL_VENTHYR_COVENANT                      = 299205,
    SPELL_NIGHT_FAE_COVENANT                    = 299206,
    SPELL_NECROLORD_COVENANT                    = 299207,

    // Covenant.db2
    COVENANT_KYRIAN                             = 1,
    COVENANT_VENTHYR                            = 2,
    COVENANT_NIGHT_FAE                          = 3,
    COVENANT_NECROLORD                          = 4
};

// 644 - Playerchoice
class playerchoice_covenant_selection : public PlayerChoiceScript
{
public:
    playerchoice_covenant_selection() : PlayerChoiceScript("playerchoice_covenant_selection") { }

    void OnResponse(WorldObject* /*object*/, Player* player, PlayerChoice const* /*choice*/, PlayerChoiceResponse const* response, uint16 /*clientIdentifier*/) override
    {
        uint32 covenantId = 0;
        uint32 covenantSpellId = 0;

        switch (response->ResponseId)
        {
            case PLAYERCHOICE_RESPONSE_JOIN_KYRIAN:
                covenantId = COVENANT_KYRIAN;
                covenantSpellId = SPELL_KYRIAN_COVENANT;
                break;
            case PLAYERCHOICE_RESPONSE_JOIN_VENTHYR:
                covenantId = COVENANT_VENTHYR;
                covenantSpellId = SPELL_VENTHYR_COVENANT;
                break;
            case PLAYERCHOICE_RESPONSE_JOIN_NIGHT_FAE:
                covenantId = COVENANT_NIGHT_FAE;
                covenantSpellId = SPELL_NIGHT_FAE_COVENANT;
                break;
            case PLAYERCHOICE_RESPONSE_JOIN_NECROLORD:
                covenantId = COVENANT_NECROLORD;
                covenantSpellId = SPELL_NECROLORD_COVENANT;
                break;
            default:
                // "Preview Covenant" responses (2686/2706/2707/2708) are handled entirely client side
                // by C_CovenantPreview - previewing must never join a covenant.
                return;
        }

        // Choice 644 is the initial covenant pledge only. Switching covenants is a separate sanctum
        // flow (spell 338503 "Reset Covenant" first), and re-running a covenant spell here would
        // fail the already-completed covenant-choice quests and desync the active soulbind, so a
        // player who already belongs to a covenant is refused.
        if (uint32 activeCovenantId = player->GetActiveCovenant())
        {
            if (activeCovenantId != covenantId)
                TC_LOG_DEBUG("scripts", "playerchoice_covenant_selection: {} answered PlayerChoice {} with covenant {} while already in covenant {}, ignored",
                    player->GetGUID().ToString(), uint32(PLAYER_CHOICE_COVENANT_SELECTION), covenantId, activeCovenantId);

            return;
        }

        player->CastSpell(player, covenantSpellId, CastSpellExtraArgsInit{ .TriggerFlags = TRIGGERED_FULL_MASK });

        // The cast above is instant and triggered, so the covenant must be set by the time it returns.
        // If the reward spell is missing from the client data the join would otherwise be lost, so
        // commit the covenant directly instead of leaving the player with nothing.
        if (player->GetActiveCovenant() != covenantId)
        {
            TC_LOG_ERROR("scripts", "playerchoice_covenant_selection: spell {} did not set covenant {} for {}, applying it directly (check SpellEffect data for SPELL_EFFECT_SET_COVENANT)",
                covenantSpellId, covenantId, player->GetGUID().ToString());

            player->SetActiveCovenant(covenantId);
        }
    }
};

void AddSC_covenant_playerchoice_scripts()
{
    new playerchoice_covenant_selection();
}
