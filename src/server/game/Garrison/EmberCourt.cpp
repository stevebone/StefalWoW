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

#include "EmberCourt.h"
#include "DB2Stores.h"
#include "DB2Structure.h"
#include "DBCEnums.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Garrison.h"
#include "GarrisonMgr.h"
#include "Log.h"
#include "Player.h"
#include <algorithm>

namespace
{
// The sixteen guests, in the OrderIndex the client itself uses (CriteriaTree 87983, the children of
// Achievement 14723 "Be Our Guest"). Cross-checked name-for-name against the sixteen "RSVP: <Guest>" quests
// already in `integ_world`, which is where RsvpQuestId and CreatureId come from - each of those quests is
// both started and ended by that guest's own creature. See the file header for the full derivation.
// Shorthands for the ten poles, so the roster below reads exactly like the "Likes:" strings it was copied
// from. The array is indexed by EmberCourtAttribute, so slot 0 is unused and every axis the guest does not
// care about stays EMBER_COURT_POLE_NONE.
#define L_MESSY     EMBER_COURT_POLE_LOW    // Cleanliness low
#define L_CLEAN     EMBER_COURT_POLE_HIGH   // Cleanliness high
#define L_SAFE      EMBER_COURT_POLE_LOW    // Danger low
#define L_DANGEROUS EMBER_COURT_POLE_HIGH   // Danger high
#define L_HUMBLE    EMBER_COURT_POLE_LOW    // Decadence low
#define L_DECADENT  EMBER_COURT_POLE_HIGH   // Decadence high
#define L_RELAXING  EMBER_COURT_POLE_LOW    // Excitement low
#define L_EXCITING  EMBER_COURT_POLE_HIGH   // Excitement high
#define L_CASUAL    EMBER_COURT_POLE_LOW    // Formality low
#define L_FORMAL    EMBER_COURT_POLE_HIGH   // Formality high
#define N_          EMBER_COURT_POLE_NONE

// LikedPoles is { unused, Cleanliness, Danger, Decadence, Excitement, Formality }, matching
// EmberCourtAttribute - which is itself the client's own axis order (spells 321808-321812).
EmberCourtGuest const EmberCourtGuestRoster[EMBER_COURT_GUEST_COUNT] =
{   // idx name                      RSVP   creature hosted elated  item     unused Clean       Danger       Decad       Excite      Formal
    {  0, "Baroness Vashj",          61174, 162487, 62487, 62503, 178886, { N_, N_,          L_DANGEROUS, L_DECADENT, L_EXCITING, N_       } },
    {  1, "Lady Moonberry",          61354, 172098, 62488, 62504, 181338, { N_, L_MESSY,     N_,          N_,         L_EXCITING, L_CASUAL } },
    {  2, "Mikanikos",               61173, 171647, 62489, 62505, 181339, { N_, L_CLEAN,     L_SAFE,      L_HUMBLE,   N_,         N_       } },
    {  3, "The Countess",            60948, 171106, 62490, 62506, 181340, { N_, N_,          N_,          L_DECADENT, L_RELAXING, L_FORMAL } },
    {  4, "Alexandros Mograine",     61255, 171933, 62491, 62508, 181341, { N_, N_,          L_SAFE,      L_HUMBLE,   N_,         N_       } },
    {  5, "Hunt-Captain Korayn",     61109, 171319, 62492, 62509, 181342, { N_, N_,          L_DANGEROUS, N_,         N_,         L_CASUAL } },
    {  6, "Polemarch Adrestes",      61123, 171385, 62493, 62510, 178887, { N_, L_CLEAN,     N_,          N_,         N_,         L_FORMAL } },
    {  7, "Rendle and Cudgelface",   61059, 171190, 62494, 62507, 181343, { N_, L_MESSY,     N_,          N_,         L_RELAXING, N_       } },
    {  8, "Choofa",                  61139, 160814, 62495, 62511, 178888, { N_, N_,          N_,          N_,         L_EXCITING, N_       } },
    {  9, "Cryptkeeper Kassir",      60236, 163073, 62496, 62512, 178889, { N_, N_,          N_,          N_,         N_,         L_FORMAL } },
    { 10, "Droman Aliothe",          61129, 160894, 62497, 62513, 181344, { N_, N_,          N_,          N_,         L_RELAXING, N_       } },
    { 11, "Grandmaster Vole",        61092, 163019, 62498, 62514, 181345, { N_, N_,          L_DANGEROUS, N_,         N_,         N_       } },
    { 12, "Kleia and Pelagos",       61256, 171951, 62499, 62515, 181346, { N_, N_,          N_,          L_HUMBLE,   N_,         N_       } },
    { 13, "Plague Deviser Marileth", 61105, 159930, 62500, 62516, 181347, { N_, L_MESSY,     N_,          N_,         N_,         N_       } },
    { 14, "Sika",                    61130, 166577, 62501, 62517, 181348, { N_, L_CLEAN,     N_,          N_,         N_,         N_       } },
    { 15, "Stonehead",               60916, 157199, 62502, 62518, 181349, { N_, N_,          N_,          N_,         N_,         L_CASUAL } }
};

#undef L_MESSY
#undef L_CLEAN
#undef L_SAFE
#undef L_DANGEROUS
#undef L_HUMBLE
#undef L_DECADENT
#undef L_RELAXING
#undef L_EXCITING
#undef L_CASUAL
#undef L_FORMAL
#undef N_
}

EmberCourt::EmberCourt(Player* owner) : _owner(owner)
{
}

std::vector<EmberCourtGuest> const& EmberCourt::GetGuestRoster()
{
    static std::vector<EmberCourtGuest> const roster(std::begin(EmberCourtGuestRoster), std::end(EmberCourtGuestRoster));
    return roster;
}

EmberCourtGuest const* EmberCourt::GetGuestInfo(uint8 guestIndex)
{
    if (guestIndex >= EMBER_COURT_GUEST_COUNT)
        return nullptr;

    return &EmberCourtGuestRoster[guestIndex];
}

char const* EmberCourt::GetAttributeName(EmberCourtAttribute attribute)
{
    // UiWidgetVisualization 1438/1440/1437/1439/1435 - "Capture Bar - N. <Axis> (<low>><high>)".
    switch (attribute)
    {
        case EMBER_COURT_ATTRIBUTE_CLEANLINESS: return "Cleanliness";
        case EMBER_COURT_ATTRIBUTE_DANGER:      return "Danger";
        case EMBER_COURT_ATTRIBUTE_DECADENCE:   return "Decadence";
        case EMBER_COURT_ATTRIBUTE_EXCITEMENT:  return "Excitement";
        case EMBER_COURT_ATTRIBUTE_FORMALITY:   return "Formality";
        default:                                return "none";
    }
}

char const* EmberCourt::GetAttributePoleName(EmberCourtAttribute attribute, EmberCourtAttributePole pole)
{
    // CriteriaTree 88024-88033, the children of Achievement 14726 "It's Certainly Never Boring", in their
    // OrderIndex order: Messy, Clean, Safe, Dangerous, Humble, Decadent, Relaxing, Exciting, Casual, Formal.
    if (pole != EMBER_COURT_POLE_LOW && pole != EMBER_COURT_POLE_HIGH)
        return "none";

    bool const low = pole == EMBER_COURT_POLE_LOW;
    switch (attribute)
    {
        case EMBER_COURT_ATTRIBUTE_CLEANLINESS: return low ? "Messy"    : "Clean";
        case EMBER_COURT_ATTRIBUTE_DANGER:      return low ? "Safe"     : "Dangerous";
        case EMBER_COURT_ATTRIBUTE_DECADENCE:   return low ? "Humble"   : "Decadent";
        case EMBER_COURT_ATTRIBUTE_EXCITEMENT:  return low ? "Relaxing" : "Exciting";
        case EMBER_COURT_ATTRIBUTE_FORMALITY:   return low ? "Casual"   : "Formal";
        default:                                return "none";
    }
}

char const* EmberCourt::GetMoodName(EmberCourtMood mood)
{
    // SpellName 327199/327200/327201/327781/327202, and the same five as literal "Mood: <Rung>" strings in
    // UiWidgetStringSource.
    switch (mood)
    {
        case EMBER_COURT_MOOD_MISERABLE:        return "Miserable";
        case EMBER_COURT_MOOD_UNCOMFORTABLE:    return "Uncomfortable";
        case EMBER_COURT_MOOD_HAPPY:            return "Happy";
        case EMBER_COURT_MOOD_VERY_HAPPY:       return "Very Happy";
        case EMBER_COURT_MOOD_ELATED:           return "Elated";
        default:                                return "none";
    }
}

bool EmberCourt::IsAttributeLiked(uint8 guestIndex, EmberCourtAttribute attribute, EmberCourtAttributePole pole)
{
    EmberCourtGuest const* guest = GetGuestInfo(guestIndex);
    if (!guest || attribute == EMBER_COURT_ATTRIBUTE_NONE || attribute > EMBER_COURT_ATTRIBUTE_MAX)
        return false;

    if (pole != EMBER_COURT_POLE_LOW && pole != EMBER_COURT_POLE_HIGH)
        return false;

    return guest->LikedPoles[attribute] == pole;
}

bool EmberCourt::IsAttributeDisliked(uint8 guestIndex, EmberCourtAttribute attribute, EmberCourtAttributePole pole)
{
    if (attribute == EMBER_COURT_ATTRIBUTE_NONE || attribute > EMBER_COURT_ATTRIBUTE_MAX)
        return false;

    if (pole != EMBER_COURT_POLE_LOW && pole != EMBER_COURT_POLE_HIGH)
        return false;

    // The 68275 build publishes no dislikes at all - every guest item carries only a "Likes:" list. The
    // opposite pole of a like is deliberately NOT treated as a dislike; that would be a guess. A dislike
    // exists only when it has been authored.
    EmberCourtGuestTemplate const* guestTemplate = sGarrisonMgr.GetEmberCourtGuest(guestIndex);
    return guestTemplate && guestTemplate->DislikedAttribute == attribute && guestTemplate->DislikedPole == pole;
}

uint32 EmberCourt::GetEmberCourtTreeId() const
{
    if (!_owner || _owner->GetActiveCovenant() != COVENANT_ID_VENTHYR)
        return 0;

    if (std::vector<GarrTalentTreeEntry const*> const* trees = sGarrisonMgr.GetTalentTreesForGarrType(GARRISON_TYPE_COVENANT))
        for (GarrTalentTreeEntry const* tree : *trees)
            if (tree->FeatureTypeIndex == GARR_TALENT_FEATURE_UNIQUE && tree->FeatureSubtypeIndex == COVENANT_ID_VENTHYR)
                return tree->ID;

    return 0;
}

bool EmberCourt::HasTalentAtTier(uint32 tier) const
{
    uint32 const treeId = GetEmberCourtTreeId();
    if (!treeId)
        return false;

    Garrison const* garrison = _owner->GetGarrison(GARRISON_TYPE_COVENANT);
    if (!garrison)
        return false;

    std::vector<GarrTalentEntry const*> const* talents = sGarrisonMgr.GetTalentsForTree(treeId);
    if (!talents)
        return false;

    // Tree 324 is a CLASSIC tree (GarrTalentTreeType 1), so a researched COUNT would not tell us WHICH unlock
    // the player owns. Each unlock is read off the talent that carries it, found by its GarrTalent.Tier.
    for (GarrTalentEntry const* talent : *talents)
    {
        if (uint32(talent->Tier) != tier)
            continue;

        if (Garrison::Talent const* owned = garrison->GetTalent(talent->ID))
            if (owned->Rank >= 1)
                return true;
    }

    return false;
}

uint32 EmberCourt::GetResearchedTiers() const
{
    uint32 const treeId = GetEmberCourtTreeId();
    if (!treeId)
        return 0;

    Garrison const* garrison = _owner->GetGarrison(GARRISON_TYPE_COVENANT);
    if (!garrison)
        return 0;

    std::vector<GarrTalentEntry const*> const* talents = sGarrisonMgr.GetTalentsForTree(treeId);
    if (!talents)
        return 0;

    uint32 unlocked = 0;
    for (GarrTalentEntry const* talent : *talents)
        if (Garrison::Talent const* owned = garrison->GetTalent(talent->ID))
            if (owned->Rank >= 1)
                ++unlocked;

    return std::min<uint32>(unlocked, EMBER_COURT_MAX_TIERS);
}

bool EmberCourt::IsAccessible() const
{
    // Talent 1111 "A New Court" (Tier 0) is the feature itself; nothing else in the tree matters without it.
    return HasTalentAtTier(EMBER_COURT_TIER_UNLOCK);
}

uint32 EmberCourt::GetGuestSlots() const
{
    if (!IsAccessible())
        return 0;

    // Talent 1114 "Court Influencer" grants "a THIRD guest" and 1112 "Discerning Taste" "a FOURTH guest", so
    // the base list is two. The count is stated by the talents, not chosen here.
    uint32 slots = EMBER_COURT_BASE_GUEST_SLOTS;
    if (HasTalentAtTier(EMBER_COURT_TIER_THIRD_GUEST))
        ++slots;
    if (HasTalentAtTier(EMBER_COURT_TIER_FOURTH_GUEST))
        ++slots;

    return slots;
}

bool EmberCourt::HasDredgerButler() const
{
    return HasTalentAtTier(EMBER_COURT_TIER_BUTLER);
}

uint32 EmberCourt::GetStaffSlots() const
{
    return HasTalentAtTier(EMBER_COURT_TIER_STAFF) ? uint32(EMBER_COURT_STAFF_SLOTS) : 0u;
}

bool EmberCourt::IsGuestUnlocked(uint8 guestIndex) const
{
    if (!_owner)
        return false;

    EmberCourtGuest const* guest = GetGuestInfo(guestIndex);
    if (!guest || !guest->RsvpQuestId)
        return false;

    // The game's own unlock: the guest's "RSVP: <Guest>" quest, already present in this world DB. No separate
    // server-side flag is invented for it.
    return _owner->IsQuestRewarded(guest->RsvpQuestId);
}

bool EmberCourt::IsGuestInvited(uint8 guestIndex) const
{
    auto itr = _guests.find(guestIndex);
    return itr != _guests.end() && itr->second.Invited;
}

std::vector<uint8> EmberCourt::GetInvitedGuests() const
{
    std::vector<uint8> invited;
    for (auto const& [guestIndex, state] : _guests)
        if (state.Invited)
            invited.push_back(guestIndex);

    std::sort(invited.begin(), invited.end());
    return invited;
}

EmberCourtGuestState const* EmberCourt::GetGuestState(uint8 guestIndex) const
{
    auto itr = _guests.find(guestIndex);
    return itr != _guests.end() ? &itr->second : nullptr;
}

std::vector<EmberCourtGuestState const*> EmberCourt::GetGuestStates() const
{
    std::vector<EmberCourtGuestState const*> states;
    states.reserve(_guests.size());
    for (auto const& [guestIndex, state] : _guests)
        states.push_back(&state);

    std::sort(states.begin(), states.end(),
        [](EmberCourtGuestState const* l, EmberCourtGuestState const* r) { return l->GuestIndex < r->GuestIndex; });
    return states;
}

EmberCourtGuestState& EmberCourt::GetOrCreateGuestState(uint8 guestIndex)
{
    EmberCourtGuestState& state = _guests[guestIndex];
    state.GuestIndex = guestIndex;
    return state;
}

EmberCourtError EmberCourt::InviteGuest(uint8 guestIndex)
{
    if (!_owner || _owner->GetActiveCovenant() != COVENANT_ID_VENTHYR)
        return EMBER_COURT_ERROR_NOT_VENTHYR;

    if (!IsAccessible())
        return EMBER_COURT_ERROR_NOT_UNLOCKED;

    EmberCourtGuest const* guest = GetGuestInfo(guestIndex);
    if (!guest)
        return EMBER_COURT_ERROR_UNKNOWN_GUEST;

    if (!IsGuestUnlocked(guestIndex))
        return EMBER_COURT_ERROR_GUEST_NOT_UNLOCKED;

    if (IsGuestInvited(guestIndex))
        return EMBER_COURT_ERROR_GUEST_ALREADY_INVITED;

    if (uint32(GetInvitedGuests().size()) >= GetGuestSlots())
        return EMBER_COURT_ERROR_GUEST_SLOTS_FULL;

    GetOrCreateGuestState(guestIndex).Invited = true;
    MarkChanged();

    TC_LOG_DEBUG("garrison", "EmberCourt: player {} invited guest {} ({}); guest list now {}/{}.",
        _owner->GetGUID().ToString(), guestIndex, guest->Name, uint32(GetInvitedGuests().size()), GetGuestSlots());

    return EMBER_COURT_OK;
}

EmberCourtError EmberCourt::UninviteGuest(uint8 guestIndex)
{
    if (!_owner || _owner->GetActiveCovenant() != COVENANT_ID_VENTHYR)
        return EMBER_COURT_ERROR_NOT_VENTHYR;

    if (!GetGuestInfo(guestIndex))
        return EMBER_COURT_ERROR_UNKNOWN_GUEST;

    if (!IsGuestInvited(guestIndex))
        return EMBER_COURT_ERROR_GUEST_NOT_INVITED;

    GetOrCreateGuestState(guestIndex).Invited = false;
    MarkChanged();
    return EMBER_COURT_OK;
}

void EmberCourt::ClearInvitations()
{
    for (auto& [guestIndex, state] : _guests)
    {
        if (state.Invited)
        {
            state.Invited = false;
            MarkChanged();
        }
    }
}

EmberCourtError EmberCourt::StartCourt() const
{
    if (!_owner || _owner->GetActiveCovenant() != COVENANT_ID_VENTHYR)
        return EMBER_COURT_ERROR_NOT_VENTHYR;

    if (!IsAccessible())
        return EMBER_COURT_ERROR_NOT_UNLOCKED;

    if (GetInvitedGuests().empty())
        return EMBER_COURT_ERROR_NO_GUESTS_INVITED;

    // Scenario 1791 in AreaTable 13329 on map 2222 is real client data, but nothing in this world DB
    // instantiates it: no `scenarios` row for the map, Temel 164966 and all five staff NPCs have zero spawns,
    // and area 13329 holds zero creatures and zero gameobjects. Holding a party nobody could attend - or
    // worse, quietly "completing" one - would be a lie. Refuse until the venue is authored.
    if (!sGarrisonMgr.IsEmberCourtVenueAuthored())
        return EMBER_COURT_ERROR_NO_VENUE_CONTENT;

    // NOTE there is deliberately no check on `garrison_ember_court_guest` here. That table now carries only
    // each guest's DISLIKE, which the 68275 build does not publish at all and which a court does not need -
    // every guest's LIKES are client data and are already in the roster. An empty table is a normal state.
    return EMBER_COURT_OK;
}

EmberCourtError EmberCourt::CompleteCourt(std::unordered_map<uint8, uint8> const& moods)
{
    if (!_owner || _owner->GetActiveCovenant() != COVENANT_ID_VENTHYR)
        return EMBER_COURT_ERROR_NOT_VENTHYR;

    if (!IsAccessible())
        return EMBER_COURT_ERROR_NOT_UNLOCKED;

    std::vector<uint8> const invited = GetInvitedGuests();
    if (invited.empty())
        return EMBER_COURT_ERROR_NO_GUESTS_INVITED;

    // A mood is only ever REPORTED to this class, never computed by it (the thresholds behind the rungs are
    // server-side WorldStates that the build does not publish). It is still validated against the ladder, so
    // a caller cannot smuggle in a sixth rung and have it stored as a high-water mark.
    for (auto const& [guestIndex, mood] : moods)
        if (mood > EMBER_COURT_MOOD_MAX)
            return EMBER_COURT_ERROR_INVALID_MOOD;

    for (uint8 guestIndex : invited)
    {
        EmberCourtGuestState& state = GetOrCreateGuestState(guestIndex);
        ++state.TimesHosted;
        state.LastHostedTime = GameTime::GetGameTime();
        state.Invited = false;

        auto moodItr = moods.find(guestIndex);
        uint8 const mood = moodItr != moods.end() ? moodItr->second : uint8(0);
        if (mood > state.HighestMood)
            state.HighestMood = mood;

        // The achievements are not re-implemented here. Achievement 14723 "Be Our Guest" and 14724 "People
        // Pleaser" are ordinary CriteriaType::CompleteQuest criteria (Type 27) over per-guest hidden credit
        // quests, so crediting those quest ids lets the existing criteria system pay out exactly what the
        // client data says. NOTE: those quest ids are NOT in `quest_template` in this world DB, so the
        // criteria will simply not fire until they are authored - which is correct, and better than
        // inventing a parallel achievement path.
        if (EmberCourtGuest const* guest = GetGuestInfo(guestIndex))
        {
            if (guest->HostedCriteriaQuestId)
                _owner->UpdateCriteria(CriteriaType::CompleteQuest, guest->HostedCriteriaQuestId);

            // Achievement 14724 "People Pleaser" asks for the ELATED rung specifically, and Elated is the top
            // of the five-rung ladder the client publishes - so the bar is a derived constant, not a tunable.
            if (guest->ElatedCriteriaQuestId && mood >= EMBER_COURT_MOOD_ELATED)
                _owner->UpdateCriteria(CriteriaType::CompleteQuest, guest->ElatedCriteriaQuestId);
        }
    }

    ++_courtsHeld;
    _lastCourtTime = GameTime::GetGameTime();
    MarkChanged();

    TC_LOG_DEBUG("garrison", "EmberCourt: player {} completed court #{} with {} guest(s).",
        _owner->GetGUID().ToString(), _courtsHeld, uint32(invited.size()));

    return EMBER_COURT_OK;
}

void EmberCourt::Update()
{
    if (!_owner)
        return;

    // A talent reset (or a covenant switch) can shrink the guest list under invitations that are already
    // standing, and an "RSVP: <Guest>" quest is the only thing that authorises an invitation at all. Nothing
    // is deleted - hosting history stands - but an invitation that is no longer legal must not survive into a
    // court, so trim from the highest guest index down until the list fits.
    uint32 const slots = GetGuestSlots();

    std::vector<uint8> invited = GetInvitedGuests();
    for (uint8 guestIndex : invited)
    {
        if (!IsGuestUnlocked(guestIndex))
        {
            _guests[guestIndex].Invited = false;
            MarkChanged();
        }
    }

    invited = GetInvitedGuests();
    while (invited.size() > slots)
    {
        _guests[invited.back()].Invited = false;
        invited.pop_back();
        MarkChanged();
    }
}

void EmberCourt::LoadFromDB(PreparedQueryResult result)
{
    _guests.clear();
    _courtsHeld = 0;
    _lastCourtTime = 0;
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint8 const guestIndex = fields[0].GetUInt8();

        // Guard the roster bound: a row outside 0-15 cannot be resolved to a guest and every gate below would
        // read it as unknown. Drop it loudly rather than carry it.
        if (!GetGuestInfo(guestIndex))
        {
            TC_LOG_ERROR("garrison", "EmberCourt: dropping stored guest index {} for player {} - the roster only "
                "has {} guests (CriteriaTree 87983).", uint32(guestIndex), _owner->GetGUID().ToString(),
                uint32(EMBER_COURT_GUEST_COUNT));
            continue;
        }

        EmberCourtGuestState& state = GetOrCreateGuestState(guestIndex);
        state.TimesHosted    = fields[1].GetUInt32();
        // Clamp to the five-rung ladder so a hand-edited row cannot resurrect a sixth rung.
        state.HighestMood    = std::min<uint8>(fields[2].GetUInt8(), uint8(EMBER_COURT_MOOD_MAX));
        state.LastHostedTime = fields[3].GetInt64();
        state.Invited        = fields[4].GetBool();

        // Court-level counters are denormalised onto every guest row so the feature keeps a single table; they
        // agree across rows, so the last one read wins.
        _courtsHeld    = fields[5].GetUInt32();
        _lastCourtTime = fields[6].GetInt64();
    } while (result->NextRow());
}

void EmberCourt::SaveToDB(CharacterDatabaseTransaction trans) const
{
    // Nothing was ever done and nothing changed - skip the delete/insert churn.
    if (!_needsSave && _guests.empty())
        return;

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER_GARRISON_EMBER_COURT);
    stmt->setUInt64(0, _owner->GetGUID().GetCounter());
    trans->Append(stmt);

    for (auto const& [guestIndex, state] : _guests)
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_GARRISON_EMBER_COURT);
        stmt->setUInt64(0, _owner->GetGUID().GetCounter());
        stmt->setUInt8(1, state.GuestIndex);
        stmt->setUInt32(2, state.TimesHosted);
        stmt->setUInt8(3, state.HighestMood);
        stmt->setInt64(4, state.LastHostedTime);
        stmt->setBool(5, state.Invited);
        stmt->setUInt32(6, _courtsHeld);
        stmt->setInt64(7, _lastCourtTime);
        trans->Append(stmt);
    }
}
