
// Player Notifications
constexpr auto FSB_PLAYER_NOTIFICATION_PAYMENT_SUCCESS = "Payment successful!";
constexpr auto FSB_PLAYER_NOTIFICATION_PAYMENT_FAIL = "Not enough money!";

// Menu items - these have sub items
constexpr auto FSB_GOSSIP_MENU_HIRE = "I'd like to hire you!";
constexpr auto FSB_GOSSIP_MENU_INSTRUCTIONS = "Instructions";

constexpr auto FSB_GOSSIP_MENU_FOLLOW_DIST = "Follow Distance";
constexpr auto FSB_GOSSIP_MENU_FOLLOW_ANGLE = "Follow Angle";
constexpr auto FSB_GOSSIP_MENU_ROLES = "Roles";

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

constexpr auto FSB_GOSSIP_ITEM_ROLE_ASSIST = "Assist Role: mostly heals and light combat assist";
constexpr auto FSB_GOSSIP_ITEM_ROLE_DAMAGE = "Damage Dealer: only damage on all targets";
constexpr auto FSB_GOSSIP_ITEM_ROLE_BALANCED = "Balanced Role: a mix of heals and damage spells";


constexpr auto FSB_GOSSIP_ITEM_INFO = "Tell me more about yourself";

// NPC Say
constexpr auto FSB_SAY_FOLLOW_INFO_CHANGED = "Ok, understood that!";

constexpr uint32 REACT_BUFFED_CHANCE_PERCENT = 10;

// Spell Related
static constexpr uint32 NPC_GCD_MS = 1500;

// MoveChase range for spell casters
constexpr float SPELL_MAX_RANGE = 30.0f;
constexpr float SPELL_MIN_RANGE = 18.0f;

enum FSB_GossipMenus
{
    FSB_GOSSIP_DEFAULT_MENU = 900000,
    FSB_GOSSIP_ROLES_MENU = 900001,
    FSB_GOSSIP_HIRE_MENU = 900002,
    FSB_GOSSIP_INSTRUCTIONS_MENU = 900003,
    FSB_GOSSIP_FOLLOW_DIST_MENU = 900004,
    FSB_GOSSIP_FOLLOW_ANGLE_MENU = 900005

};

enum FSB_GenericEvents
{
    FSB_EVENT_HIRE_EXPIRED = 1,
    FSB_EVENT_HIRE_LEAVE = 2,
    FSB_EVENT_HIRE_DISMISSED = 3,
    FSB_EVENT_PERIODIC_MAINTENANCE = 4
};

enum FSB_MoveEvents
{
    FSB_EVENT_MOVE_STAY = 10,
    FSB_EVENT_MOVE_FOLLOW = 11
};

enum FSB_PriestSpellEvents
{
    //FSB_EVENT_PRIEST_CHECK_BUFFS = 20,
    //FSB_EVENT_PRIEST_CHECK_HP = 21,
    FSB_EVENT_PRIEST_COMBAT_SPELLS = 22,
    FSB_EVENT_PRIEST_CAST_SPELL = 23,
    FSB_EVENT_CHECK_COMBAT = 24,
    FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_PLAYER = 25,
    FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_SELF = 26
};

enum FSB_MovementStates
{
    FSB_MOVE_STATE_IDLE = 0,
    FSB_MOVE_STATE_FOLLOWING = 1,
    FSB_MOVE_STATE_STAY = 2
};

enum FSB_RoleStates
{
    FSB_ROLE_STATE_NONE = 0,
    FSB_ROLE_STATE_ASSIST = 1,
    FSB_ROLE_STATE_DAMAGE = 2,
    FSB_ROLE_STATE_BALANCED = 3
};

enum FSB_Actions
{
    FSB_ACTION_COMBAT_SPELLS = 1,
    FSB_ACTION_RESURRECT_PLAYER = 2,
    FSB_ACTION_INITIAL_COMBAT_SPELLS_SELF = 3,
    FSB_ACTION_INITIAL_COMBAT_SPELLS_PLAYER = 4
};

constexpr float FOLLOW_DISTANCE_CLOSE = 3.0f;
constexpr float FOLLOW_DISTANCE_NORMAL = 5.0f;
constexpr float FOLLOW_DISTANCE_WIDE = 8.0f;

constexpr float FOLLOW_ANGLE_BEHIND = 0.0f;
constexpr float FOLLOW_ANGLE_FRONT = M_PI;
constexpr float FOLLOW_ANGLE_LEFT = M_PI / 2.0f;
constexpr float FOLLOW_ANGLE_RIGHT = -M_PI / 2.0f;
