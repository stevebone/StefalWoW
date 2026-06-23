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

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>

class Creature;

namespace FSBChat { struct ActiveConversation; }

namespace FSBConvPrompts
{
    enum class ConversationTopicCategory
    {
        Joke,
        Complaint,
        ZoneEvent,
        RandomTake,
        Political,
        Question,
        Story
    };

    enum class ConversationTopicSubCategory
    {
        ZoneJoke,
        NPCJoke,
        BossJoke,
        ProfessionJoke,
        ClassJoke,
        ZoneComplaint,
        NPCComplaint,
        ClassComplaint,
        ZoneEvent,
        NPCEvent,
        FactionOpinion,
        RaceOpinion,
        ClassOpinion,
        ThematicQuestion,
        AdventureStory,
        BossEncounterStory
    };

    enum class TopicDataSource : uint8
    {
        ZoneFeature,
        ZoneNPC,
        ZoneIssue,
        ZoneEventSubject,
        GlobalBoss,
        GlobalProfession,
        GlobalFaction,
        GlobalRace,
        GlobalClass,
        GlobalTheme
    };

    struct ConversationTopic
    {
        ConversationTopicCategory     category;
        ConversationTopicSubCategory  subCategory;
        std::string                   formatTemplate;
        TopicDataSource               dataSource;
    };

    struct ConversationLlamaState
    {
        std::atomic<bool> ready{ false };
        std::string result;
        std::mutex mutex;
    };

    // -------------------------------------------------------------------------
    // Global data tables
    // -------------------------------------------------------------------------

    inline const std::vector<std::string> Professions =
    {
        "Engineering", "Alchemy", "Blacksmithing", "Tailoring",
        "Enchanting", "Leatherworking", "Jewelcrafting", "Inscription",
        "Cooking", "Fishing", "First Aid", "Herbalism", "Mining", "Skinning"
    };

    inline const std::vector<std::string> Factions =
    {
        "the Alliance", "the Horde", "the Burning Legion", "the Scourge",
        "the Old Gods", "the Argent Dawn", "the Kirin Tor", "the Earthen Ring",
        "the Dragonflights", "the Illidari", "the Scarlet Crusade", "the Defias Brotherhood"
    };

    inline const std::vector<std::string> Themes =
    {
        "profession", "boss", "weapon", "mount", "hero", "villain",
        "zone", "dungeon", "race", "class", "spell", "armor", "pet", "faction"
    };

    inline const std::vector<std::string> BossNames =
    {
        "Ragnaros", "Onyxia", "Illidan Stormrage", "The Lich King",
        "Deathwing", "Garrosh Hellscream", "Sylvanas Windrunner",
        "Kel'Thuzad", "C'Thun", "Nefarian", "Malygos", "Yogg-Saron",
        "Archimonde", "Kil'jaeden", "Gul'dan", "Varimathras",
        "Lord Kazzak", "Azuregos", "Emeriss", "Nightsong", "Hogger"
    };

    // -------------------------------------------------------------------------
    // Zone-local data tables (zoneId -> keyword list)
    // -------------------------------------------------------------------------
    inline const std::unordered_map<uint32, std::vector<std::string>> ZoneFeatures =
    {
        {   12, { "the wolves", "Goldshire", "the Eastvale Logging Camp", "the roads" } },
        { 1519, { "the canals", "the Mage Quarter", "the Auction House", "the Cathedral" } },
        {    1, { "the trolls", "the Coldridge Pass", "the brewery", "the tram" } },
        { 1537, { "the Great Forge", "the Mystic Ward", "the auctioneers", "the tram station" } },
        {   14, { "the scorpions", "Razor Hill", "the troll village", "the beach" } },
        { 1637, { "the Valley of Strength", "the drag", "the bank", "the warchief" } },
        {   85, { "the Scarlet Crusade", "the Brill road", "the graveyard", "the plague" } },
        { 1497, { "the Apothecarium", "the Royal Quarter", "the sewers", "the abominations" } },
        {  141, { "the giant trees", "the spiders", "Aldrassil", "the moonwell" } },
        { 1657, { "the Temple of the Moon", "the Craftsmen's Terrace", "the bank", "the portals" } },
        {  148, { "the murlocs", "the beach", "the ruins", "the twilight cultists" } },
        {   40, { "the Defias Brotherhood", "the farms", "the harvest golems", "the lighthouse" } },
        {   44, { "the gnolls", "the bridge", "Lake Everstill", "the orcs" } },
        {   10, { "the worgen", "the dark woods", "Raven Hill", "the necromancers" } },
        {   38, { "the loch", "the troggs", "the dam", "the excavators" } },
        {   17, { "the centaur", "the crossroads", "the oasis", "the harpies" } },
    };

    inline const std::unordered_map<uint32, std::vector<std::string>> ZoneNPCs =
    {
        {   12, { "Marshal McBride", "Remy Two-Times", "Falkhaan Isenstrider", "Smith Argus" } },
        { 1519, { "King Varian Wrynn", "Anduin Wrynn", "Gryan Stoutmantle", "Archbishop Benedictus" } },
        {    1, { "Captain Tharran", "Bruuk Barleybeard", "Sten Stoutarm", "Rejold Barleybrew" } },
        { 1537, { "Magni Bronzebeard", "High Tinker Mekkatorque", "Grumnus Steelshaper", "Belgrum" } },
        {   14, { "Gar'thok", "Lar Prowltusk", "Rezlak", "Ophek" } },
        { 1637, { "Thrall", "Cairne Bloodhoof", "Vol'jin", "Sylvanas Windrunner" } },
        {   85, { "Novice Thaivand", "Executor Zygand", "Apothecary Johaan", "Magistrate Sevren" } },
        { 1497, { "Sylvanas Windrunner", "Varimathras", "Apothecary Faranell", "Lady Sylvanas" } },
        {  141, { "Tenaron Stormgrip", "Athridas Bearmantle", "Dirania Silvershine", "Ilthalaine" } },
        { 1657, { "Tyrande Whisperwind", "Malfurion Stormrage", "Elune", "Belysra Starbreeze" } },
        {  148, { "Sentinel Tysha Moonblade", "Thundris Windweaver", "Terenthis", "Asterion" } },
        {   40, { "Gryan Stoutmantle", "Farmer Saldean", "Scout Galiaan", "Captain Danuvin" } },
        {   44, { "Magistrate Solomon", "Marshal Marris", "Guard Parker", "Foreman Oslow" } },
        {   10, { "Abercrombie", "Clerk Daltry", "Jitters", "Elaine Carevin" } },
        {   38, { "Mountaineer Stormpike", "Pilot Bellowfiz", "Prospector Ironband", "Merrin Rockweaver" } },
        {   17, { "Sergra Darkthorn", "Tonga Runetotem", "Mankrik", "Zargh" } },
    };

    inline const std::unordered_map<uint32, std::vector<std::string>> ZoneIssues =
    {
        {   12, { "the wolves", "the long walks", "the lack of a real inn" } },
        { 1519, { "the canal smell", "the crowds", "the expensive repairs" } },
        {    1, { "the cold", "the trolls", "the long tunnels" } },
        { 1537, { "the noise", "the smoke", "the gnomes arguing" } },
        {   14, { "the heat", "the scorpions", "the dry wind" } },
        { 1637, { "the smell", "the crowds", "the warchief's temper" } },
        {   85, { "the undead", "the smell", "the Scarlet fanatics" } },
        { 1497, { "the sewers", "the abominations", "the gloom" } },
        {  141, { "the spiders", "the height", "the endless branches" } },
        { 1657, { "the isolation", "the burning tree", "the night-only schedule" } },
        {  148, { "the murlocs", "the rain", "the cultists" } },
        {   40, { "the Defias", "the dust", "the broken farms" } },
        {   44, { "the gnolls", "the bridge toll", "the orc raids" } },
        {   10, { "the worgen", "the darkness", "the undead" } },
        {   38, { "the troggs", "the dam noise", "the cold water" } },
        {   17, { "the centaur", "the heat", "the long roads" } },
    };

    inline const std::unordered_map<uint32, std::vector<std::string>> ZoneEventSubjects =
    {
        {   12, { "a wolf attack", "a lost shipment", "a traveling merchant" } },
        { 1519, { "a canal incident", "a mage experiment", "a guard chase" } },
        {    1, { "a cave-in", "a troll raid", "a brewing accident" } },
        { 1537, { "a forge explosion", "a gnome invention", "a council debate" } },
        {   14, { "a scorpion swarm", "a shipwreck", "a vision quest" } },
        { 1637, { "a bar brawl", "a shaman ritual", "a trade dispute" } },
        {   85, { "a grave robbing", "a plague outbreak", "a Scarlet patrol" } },
        { 1497, { "an experiment gone wrong", "a sewer blockage", "a Forsaken rally" } },
        {  141, { "a spider infestation", "a moonwell disturbance", "a druid gathering" } },
        { 1657, { "a portal malfunction", "a crafting contest", "a Sentinel drill" } },
        {  148, { "a murloc raid", "a ship arrival", "a twilight sighting" } },
        {   40, { "a Defias ambush", "a harvest golem rampage", "a lighthouse signal" } },
        {   44, { "a gnoll attack", "a bridge collapse", "a black dragon sighting" } },
        {   10, { "a worgen howl", "a graveyard disturbance", "a necromancer ritual" } },
        {   38, { "a trogg breach", "a dam overflow", "a prospector discovery" } },
        {   17, { "a centaur raid", "an oasis dispute", "a trade caravan arrival" } },
    };

    ConversationTopic PickRandomTopic();
    std::string ResolveDataItem(Creature* bot, TopicDataSource source);
    std::string BuildTopicDescription(Creature* bot, ConversationTopic const& topic);
    std::string BuildConversationSystemPrompt(Creature* bot);
    void DispatchConversationTurn(Creature* speaker, FSBChat::ActiveConversation& conv);
    std::string GetFallbackConversationLine(FSBChat::ActiveConversation const& conv);
}
