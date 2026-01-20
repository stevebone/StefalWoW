
// Player Notifications
constexpr auto FSB_PLAYER_NOTIFICATION_PAYMENT_SUCCESS = "Payment successful!";
constexpr auto FSB_PLAYER_NOTIFICATION_PAYMENT_FAIL = "Not enough money!";


// Spell Related
static constexpr uint32 NPC_GCD_MS = 1500;

// MoveChase range for spell casters
constexpr float SPELL_MAX_RANGE = 30.0f;
constexpr float SPELL_MIN_RANGE = 18.0f;



enum FSB_Data
{
    FSB_DATA_HIRED = 1,
    FSB_DATA_HIRE_TIME_LEFT = 2,
};

enum FSB_Factions
{
    FSB_FACTION_ALLIANCE = 1
};

enum FSB_GenericEvents
{
    FSB_EVENT_HIRE_EXPIRED = 1,
    FSB_EVENT_HIRE_LEAVE = 2,
    FSB_EVENT_HIRE_DISMISSED = 3,
    
    FSB_EVENT_RESUME_FOLLOW = 5,
    
    FSB_EVENT_CHECK_DESPAWN = 7,
    FSB_EVENT_MOVE_STAY = 10,
    FSB_EVENT_MOVE_FOLLOW = 11,
    // = 12,
};

enum FSB_PriestSpellEvents
{
    
    // = 21,

    // = 23,
    FSB_EVENT_CHECK_COMBAT = 24,
    
    FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_SELF = 26,
    // = 27,
    // = 28
};

enum FSB_EventsHiredState
{
    FSB_EVENT_HIRED_MAINTENANCE = 30,
    FSB_EVENT_HIRED_CHECK_OWNER_COMBAT = 31,
    FSB_EVENT_HIRED_CHECK_TELEPORT = 32,
    FSB_EVENT_HIRED_UPDATE_BOT_LEVEL = 33,
    FSB_EVENT_HIRED_CHECK_ALLIES = 34,

    FSB_EVENT_COMBAT_MAINTENANCE = 41,
    FSB_EVENT_COMBAT_SPELL_CHECK = 42,
    FSB_EVENT_COMBAT_IC_ACTIONS = 44,

    FSB_EVENT_PERIODIC_MAINTENANCE = 50,
    FSB_EVENT_PERIODIC_CUSTOM_REGEN = 51,
    FSB_EVENT_PERIODIC_OOC_ACTIONS = 52
};

enum FSB_MovementStates
{
    FSB_MOVE_STATE_IDLE = 0,
    FSB_MOVE_STATE_FOLLOWING = 1,
    FSB_MOVE_STATE_STAY = 2
};

enum FSB_Actions
{
    FSB_ACTION_RESURRECT_PLAYER = 2,
    


    
    FSB_ACTION_OOC_ACTIONS = 21,

    FSB_ACTION_COMBAT_IC_ACTIONS = 23,
};

constexpr float FOLLOW_DISTANCE_CLOSE = 3.0f;
constexpr float FOLLOW_DISTANCE_NORMAL = 5.0f;
constexpr float FOLLOW_DISTANCE_WIDE = 8.0f;

constexpr float FOLLOW_ANGLE_BEHIND = 0.0f;
constexpr float FOLLOW_ANGLE_FRONT = float(M_PI);
constexpr float FOLLOW_ANGLE_LEFT = float(M_PI) / 2.0f;
constexpr float FOLLOW_ANGLE_RIGHT =  float(-M_PI) / 2.0f;
