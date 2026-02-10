// Menu items - these have sub items
constexpr auto FSB_GOSSIP_MENU_HIRE = "I'd like to hire you!";
constexpr auto FSB_GOSSIP_MENU_INSTRUCTIONS = "Instructions";

constexpr auto FSB_GOSSIP_MENU_FOLLOW_DIST = "Follow Distance";
constexpr auto FSB_GOSSIP_MENU_FOLLOW_ANGLE = "Follow Angle";
constexpr auto FSB_GOSSIP_MENU_ROLES = "Roles";

constexpr auto FSB_GOSSIP_MENU_PORTAL = "I need a portal";

// Non menu items
constexpr auto FSB_GOSSIP_ITEM_PHIRE = "I need you for a longer time...";
constexpr auto FSB_GOSSIP_ITEM_FIRE = "Dismiss!";
constexpr auto FSB_GOSSIP_ITEM_BACKMAIN = "Let's talk about something else";

constexpr auto FSB_GOSSIP_ITEM_STAY_HERE = "Stay here!";
constexpr auto FSB_GOSSIP_ITEM_FOLLOW = "Follow me!";

constexpr auto FSB_GOSSIP_ITEM_FOLLOW_DIST1 = "Stay close: 3 feet...";
constexpr auto FSB_GOSSIP_ITEM_FOLLOW_DIST2 = "Normal distance: 5 feet...";
constexpr auto FSB_GOSSIP_ITEM_FOLLOW_DIST3 = "Keep your distance: 8 feet...";

constexpr auto FSB_GOSSIP_ITEM_FOLLOW_ANGLE_FRONT = "Take the lead! Move in front!";
constexpr auto FSB_GOSSIP_ITEM_FOLLOW_ANGLE_BACK = "Get behind me!";
constexpr auto FSB_GOSSIP_ITEM_FOLLOW_ANGLE_LEFT = "Move on my left!";
constexpr auto FSB_GOSSIP_ITEM_FOLLOW_ANGLE_RIGHT = "Move on my right!";

constexpr auto FSB_GOSSIP_ITEM_ROLE_ASSIST = "Healer Role: mostly heals and light combat assist";
constexpr auto FSB_GOSSIP_ITEM_ROLE_DAMAGE = "Damage Dealer: only damage on all targets";
constexpr auto FSB_GOSSIP_ITEM_ROLE_BALANCED = "Assist Role: a mix of heals and damage spells";
constexpr auto FSB_GOSSIP_ITEM_ROLE_TANK = "Tank Role: take a beating";
constexpr auto FSB_GOSSIP_ITEM_ROLE_MELEE = "Melee DPS Role: use close range weapons and attacks";
constexpr auto FSB_GOSSIP_ITEM_ROLE_HEALER = "Healer Role: mostly heals and light combat assist";
constexpr auto FSB_GOSSIP_ITEM_ROLE_ARCANE = "Arcane Damage Role: mostly arcane spells";
constexpr auto FSB_GOSSIP_ITEM_ROLE_FROST = "Frost Damage Dealer: mostly frost and ice spells";
constexpr auto FSB_GOSSIP_ITEM_ROLE_FIRE = "Fire Damage Role: mostly fire spells";
constexpr auto FSB_GOSSIP_ITEM_ROLE_AFFLICTION = "Affliction: A master of shadow magic";
constexpr auto FSB_GOSSIP_ITEM_ROLE_DEMONOLOGY = "Demonology: A master of demons";
constexpr auto FSB_GOSSIP_ITEM_ROLE_DESTRUCTION = "Destruction: A master of chaos";

constexpr auto FSB_GOSSIP_ITEM_PORTAL_STORMWIND = "Stormwind";
constexpr auto FSB_GOSSIP_ITEM_PORTAL_DARNASSUS = "Darnassus";
constexpr auto FSB_GOSSIP_ITEM_PORTAL_IRONFORGE = "Ironforge";
constexpr auto FSB_GOSSIP_ITEM_PORTAL_EXODAR = "Exodar";

constexpr auto FSB_GOSSIP_ITEM_INFO = "Tell me more about yourself";

// NPC SAY
constexpr auto FSB_SAY_FOLLOW_INFO_CHANGED = "Ok, understood that!";
constexpr auto FSB_SAY_DUPLICATE_FOLLOWER = "I'm sorry but I cannot come with you... one of us is already with you!";

enum FSB_GossipMenus
{
    FSB_GOSSIP_DEFAULT_MENU = 900000,
    FSB_GOSSIP_ROLES_MENU = 900001,
    FSB_GOSSIP_HIRE_MENU = 900002,
    FSB_GOSSIP_INSTRUCTIONS_MENU = 900003,
    FSB_GOSSIP_FOLLOW_DIST_MENU = 900004,
    FSB_GOSSIP_FOLLOW_ANGLE_MENU = 900005,
    FSB_GOSSIP_PORTAL_MENU = 900006

};

enum FSB_GossipRoleItems
{
    FSB_GOSSIP_ROLE_1,
    FSB_GOSSIP_ROLE_2,
    FSB_GOSSIP_ROLE_3
};

namespace FSBGossip
{
    bool HandleDefaultGossipHello(Creature* bot, Player* player, bool hired, ObjectGuid& outPlayerGuid);

    bool HandleGossipMenuFollowAngle(Creature* bot, Player* player);
    bool HandleGossipMenuFollowDistance(Creature* bot, Player* player);
    bool HandleGossipMenuHire(Creature* bot, Player* player);
    bool HandleGossipMenuInstructions(Creature* bot, Player* player);
    bool HandleGossipMenuPortals(Creature* bot, Player* player);
    bool HandleGossipMenuRoles(Creature* bot, Player* player);

    void HandleGossipItemInfo(Creature* bot, Player* player);
    void HandleGossipItemRole(Creature* bot, FSB_Class botClass, uint8 roleOption, bool& demonDead);
}
