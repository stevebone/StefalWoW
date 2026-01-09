// =========================================
// TO-DO
// =========================================
// Check Unit.h for Unit States related to movement and use those instead of custom ones see enum UnitState : uint32


#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"

#include "followship_bots.h"
#include "followship_bots_config.h"
#include "followship_bots_priest.h"
#include "followship_bots_utils.h"

std::vector<FSBotSpells> AssistHealSpells =
// SPELL ID, Cooldown, Chance
{
    { SPELL_PRIEST_PRAYER_OF_MENDING,   9000, 80.0f},
    { SPELL_PRIEST_HEAL,                500,  70.0f },
    { SPELL_PRIEST_RENEW,               15000, 60.0f},
    { SPELL_PRIEST_FLASH_HEAL,          500, 50.0f }
};

static std::vector<FSBotSpells> BalancedHealSpells =
// SPELL ID, Cooldown, Chance
{
    //{ PRIEST_PRAYER_OF_MENDING, 9000, 50.0f},
    { SPELL_PRIEST_RENEW,       15000, 70.0f},
    { SPELL_PRIEST_FLASH_HEAL,  500, 60.0f }
    //{ SPELL_PRIEST_HEAL,        0,  100.0f }
};

static std::vector<FSBotSpells> SelfHealSpells =
// SPELL ID, Cooldown, Chance
{
    { SPELL_PRIEST_PRAYER_OF_MENDING,   9000, 80.0f},
    { SPELL_PRIEST_RENEW,               15000, 70.0f},
    { SPELL_PRIEST_FLASH_HEAL,          500, 60.0f }
    //{ SPELL_PRIEST_HEAL,        0,  100.0f }
};

static std::vector<FSBotSpells> CombatSpells =
{
    { SPELL_PRIEST_PSYCHIC_SCREAM,          45000, 100.0f},
    { SPELL_PRIEST_SHADOW_WORD_PAIN,        15000, 90.0f},
    { SPELL_PRIEST_VAMPIRIC_TOUCH,          1000, 90.0f}, 
    { SPELL_PRIEST_MIND_BLAST,              9000, 80.0f},
    { SPELL_PRIEST_HOLY_FIRE,               10000, 70.0f},
    { SPELL_PRIEST_SMITE,                   1000, 60.0f},   
    {SPELL_PRIEST_MIND_FLAY,                1000, 50.0f}
    
};

class npc_followship_bots_priest : public CreatureScript
{
public:
    npc_followship_bots_priest() : CreatureScript("npc_followship_bots_priest") { }

    struct npc_followship_bots_priestAI : public ScriptedAI
    {
        npc_followship_bots_priestAI(Creature* creature) : ScriptedAI(creature)
        {
            // Runs once when creature is spawned
        }

        EventMap events;
        ObjectGuid _playerGuid;

        bool configFSBEnabled = false;

        int64 configFSBPricePerLevel = 0;
        int64 configFSBPermanentPricePerLevel = 0;

        int32 configFSBHireDuration1 = 0;
        int32 configFSBHireDuration2 = 0;
        int32 configFSBHireDuration3 = 0;

        bool hired = false;

        int moveState = FSB_MOVE_STATE_IDLE;
        int roleState = FSB_ROLE_STATE_BALANCED;

        bool updateFollowInfo = false;
        float followDistance = FOLLOW_DISTANCE_NORMAL;
        float followAngle = FOLLOW_ANGLE_BEHIND;

        uint32 _globalCooldownUntil = 0;
        uint32 _desperatePrayerReadyMs = 0;
        uint32 _recuperateReadyMs = 0;
        bool _pendingResurrection = false;
        bool _playerDead = false;
        uint32 _nextCombatSayMs = 0;

        bool _playerCombatStarted = false;
        bool _playerCombatEnded = false;


        void InitializeAI() override // Runs once after creature is spawned and AI not loaded
        {
            ScriptedAI::InitializeAI(); // always call base first

            // Load the config once
            FollowshipBotsConfig::Load();

            configFSBEnabled = FollowshipBotsConfig::configFSBEnabled;

            configFSBPricePerLevel = FollowshipBotsConfig::configFSBPricePerLevel;
            configFSBPermanentPricePerLevel = FollowshipBotsConfig::configFSBPermanentPricePerLevel;

            configFSBHireDuration1 = FollowshipBotsConfig::configFSBHireDuration1;
            configFSBHireDuration2 = FollowshipBotsConfig::configFSBHireDuration2;
            configFSBHireDuration3 = FollowshipBotsConfig::configFSBHireDuration3;

            if (!configFSBEnabled)
            {
                // Completely disable gossip
                me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);
                return;
            }

            

            // Initial Flags and States
            me->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
            me->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);

            me->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
            me->SetReactState(REACT_DEFENSIVE);
            me->SetFaction(1);
        }

        void Reset() override // Runs at creature respawn, evade or when triggered
        {
            events.Reset();
            me->SetBot(true);
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Entered Reset. Is bot: {}", me->IsBot());
            // Schedule Generic Events
            // These can take place even when BOT is NOT hired or no player is around
            events.ScheduleEvent(FSB_EVENT_PERIODIC_MAINTENANCE, 10s);
        }

        // Runs once when creature is summoned by another unit
        // This only works if creature is spawned by summon (NOT DB spawn)
        void IsSummonedBy(WorldObject* summoner) override
        {

        }

        void JustAppeared() override // Runs once when creature appeared in world, works for DB spawns
        {

        }

        bool OnGossipHello(Player* player) override // Runs once when opening creature gossip
        {
            me->SetOwnerGUID(player->GetGUID());
            _playerGuid = player->GetGUID();

                InitGossipMenuFor(player, FSB_GOSSIP_DEFAULT_MENU);

                if (hired)
                {
                    AddGossipItemFor(player, GossipOptionNpc::Auctioneer, FSB_GOSSIP_ITEM_FIRE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
                    AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_INFO, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                    AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_MENU_ROLES, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
                    AddGossipItemFor(player, GossipOptionNpc::Trainer, FSB_GOSSIP_MENU_INSTRUCTIONS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);
                }
                else
                {
                    AddGossipItemFor(player, GossipOptionNpc::Auctioneer, FSB_GOSSIP_MENU_HIRE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                    AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_INFO, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                }

                //player->PlayerTalkClass->SendGossipMenu(FSB_GOSSIP_DEFAULT_MENU, me->GetGUID());
                SendGossipMenuFor(player, player->GetGossipTextId(me), me->GetGUID());
                //TC_LOG_DEBUG("scripts.ai.core", "NPCBOTS OnGossipHello2");
                return true;
        }

        bool OnGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override // Runs once when gossip item selected
        {
            int32 pLevel = player->GetLevel();

            uint32 const action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
            ClearGossipMenuFor(player);

            switch (action)
            {
                // Hire Menu
            case GOSSIP_ACTION_INFO_DEF + 1:
            {
                // we first calculate hire price based configs base price and hire duration options and actual player level
                int64 hirePrice1 = (configFSBPricePerLevel * pLevel * configFSBHireDuration1);
                std::string hireText1 = BuildHireText(hirePrice1, configFSBHireDuration1);

                int64 hirePrice2 = (configFSBPricePerLevel * pLevel * configFSBHireDuration2);
                std::string hireText2 = BuildHireText(hirePrice2, configFSBHireDuration2);

                int64 hirePrice3 = (configFSBPricePerLevel * pLevel * configFSBHireDuration3);
                std::string hireText3 = BuildHireText(hirePrice3, configFSBHireDuration3);

                AddGossipItemFor(player, GossipOptionNpc::Auctioneer, hireText1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 10);
                AddGossipItemFor(player, GossipOptionNpc::Auctioneer, hireText2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 11);
                AddGossipItemFor(player, GossipOptionNpc::Auctioneer, hireText3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 12);
                AddGossipItemFor(player, GossipOptionNpc::Auctioneer, FSB_GOSSIP_ITEM_PHIRE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 13);
                AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_BACKMAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);

                player->PlayerTalkClass->SendGossipMenu(FSB_GOSSIP_HIRE_MENU, me->GetGUID());
                return true;
            }

                // Bot Info - to do make it as SAY
            case GOSSIP_ACTION_INFO_DEF + 2:
            {
                player->GetSession()->SendNotification("I am a priest from Stormwind. I charge gold for my services");
                player->PlayerTalkClass->SendCloseGossip();
                break;
            }

                // Roles Menu
            case GOSSIP_ACTION_INFO_DEF + 3:
            {
                AddGossipItemFor(player, GossipOptionNpc::Trainer, FSB_GOSSIP_ITEM_ROLE_ASSIST, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 23);
                AddGossipItemFor(player, GossipOptionNpc::Trainer, FSB_GOSSIP_ITEM_ROLE_DAMAGE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 24);
                AddGossipItemFor(player, GossipOptionNpc::Trainer, FSB_GOSSIP_ITEM_ROLE_BALANCED, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 25);
                AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_BACKMAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);

                player->PlayerTalkClass->SendGossipMenu(FSB_GOSSIP_ROLES_MENU, me->GetGUID());
                return true;
            }

                // Bot Gossip Back to main
            case GOSSIP_ACTION_INFO_DEF + 4:
            {
                OnGossipHello(player);
                return true;
            }

                // Bot Dismiss
            case GOSSIP_ACTION_INFO_DEF + 5:
            {
                events.ScheduleEvent(FSB_EVENT_HIRE_DISMISSED, 1s);
                break;
            }

                // Bot Instructions Menu
            case GOSSIP_ACTION_INFO_DEF + 6:
            {
                if(moveState == FSB_MOVE_STATE_FOLLOWING)
                {
                    AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_STAY_HERE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 20);
                    AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_MENU_FOLLOW_DIST, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 7);
                    AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_MENU_FOLLOW_ANGLE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 8);
                    AddGossipItemFor(player, GossipOptionNpc::Auctioneer, FSB_GOSSIP_ITEM_BACKMAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
                }
                else if(moveState == FSB_MOVE_STATE_STAY)
                {
                    AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 21);
                    AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_MENU_FOLLOW_DIST, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 7);
                    AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_MENU_FOLLOW_ANGLE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 8);
                    AddGossipItemFor(player, GossipOptionNpc::Auctioneer, FSB_GOSSIP_ITEM_BACKMAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
                }

                player->PlayerTalkClass->SendGossipMenu(FSB_GOSSIP_INSTRUCTIONS_MENU, me->GetGUID());
                return true;
            }

                // Bot Follow Distance Menu
            case GOSSIP_ACTION_INFO_DEF + 7:
            {
                AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW_DIST1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 14);
                AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW_DIST2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 15);
                AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW_DIST3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 16);
                AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_BACKMAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);

                player->PlayerTalkClass->SendGossipMenu(FSB_GOSSIP_FOLLOW_DIST_MENU, me->GetGUID());
                return true;
            }

                // Bot Follow Angle Menu
            case GOSSIP_ACTION_INFO_DEF + 8:
            {
                AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW_ANGLE_FRONT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 17);
                AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW_ANGLE_BACK, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 18);
                AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW_ANGLE_RIGHT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 19);
                AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_FOLLOW_ANGLE_LEFT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 22);
                AddGossipItemFor(player, GossipOptionNpc::None, FSB_GOSSIP_ITEM_BACKMAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);

                player->PlayerTalkClass->SendGossipMenu(FSB_GOSSIP_FOLLOW_ANGLE_MENU, me->GetGUID());
                return true;
            }

                // Bot Hire Option 1
            case GOSSIP_ACTION_INFO_DEF + 10:
            {
                // End price is base cost per level times player level times duration of contract
                if (player->HasEnoughMoney(int64(configFSBPricePerLevel * pLevel * configFSBHireDuration1)))
                {
                    player->ModifyMoney(-int64(configFSBPricePerLevel * pLevel * configFSBHireDuration1));
                    player->GetSession()->SendNotification(FSB_PLAYER_NOTIFICATION_PAYMENT_SUCCESS);

                    hired = true;

                    events.ScheduleEvent(FSB_EVENT_HIRE_EXPIRED, std::chrono::minutes(configFSBHireDuration1 * 60));
                    events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 100ms);

                    //me->SetReactState(REACT_ASSIST);
                    //me->SetReactState(REACT_AGGRESSIVE);
                    events.ScheduleEvent(FSB_EVENT_CHECK_COMBAT, 2s);

                    std::string msg = BuildNPCSayText(player->GetName(), configFSBHireDuration1, FSBSayType::Hire, "");
                    me->Say(msg, LANG_UNIVERSAL);
                    //me->Say(FSB_SAY_HIRED60, LANG_UNIVERSAL);
                }
                else
                    player->GetSession()->SendNotification(FSB_PLAYER_NOTIFICATION_PAYMENT_FAIL);

                player->PlayerTalkClass->SendCloseGossip();
                return true;
            }

                // Bot Hire Option 2
            case GOSSIP_ACTION_INFO_DEF + 11:
            {
                // End price is base cost per level times player level times duration of contract
                if (player->HasEnoughMoney(int64(configFSBPricePerLevel * pLevel * configFSBHireDuration2)))
                {
                    player->ModifyMoney(-int64(configFSBPricePerLevel * pLevel * configFSBHireDuration2));
                    player->GetSession()->SendNotification(FSB_PLAYER_NOTIFICATION_PAYMENT_SUCCESS);

                    hired = true;

                    events.ScheduleEvent(FSB_EVENT_HIRE_EXPIRED, std::chrono::minutes(configFSBHireDuration2 * 60));
                    events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 100ms);
                    //events.ScheduleEvent(FSB_EVENT_PRIEST_CHECK_BUFFS, 100ms);
                    //events.ScheduleEvent(FSB_EVENT_PRIEST_CHECK_HP, 100ms);

                    //me->SetReactState(REACT_ASSIST);
                    //me->SetReactState(REACT_AGGRESSIVE);
                    events.ScheduleEvent(FSB_EVENT_CHECK_COMBAT, 2s);

                    std::string msg = BuildNPCSayText(player->GetName(), configFSBHireDuration2, FSBSayType::Hire, "");
                    me->Say(msg, LANG_UNIVERSAL);
                    //me->Say(FSB_SAY_HIRED120, LANG_UNIVERSAL);
                }
                else
                    player->GetSession()->SendNotification(FSB_PLAYER_NOTIFICATION_PAYMENT_FAIL);

                player->PlayerTalkClass->SendCloseGossip();
                return true;
            }

                // Bot Hire Option 3
            case GOSSIP_ACTION_INFO_DEF + 12:
            {
                // End price is base cost per level times player level times duration of contract
                if (player->HasEnoughMoney(int64(configFSBPricePerLevel * pLevel * configFSBHireDuration3)))
                {
                    player->ModifyMoney(-int64(configFSBPricePerLevel * pLevel * configFSBHireDuration3));
                    player->GetSession()->SendNotification(FSB_PLAYER_NOTIFICATION_PAYMENT_SUCCESS);

                    hired = true;

                    events.ScheduleEvent(FSB_EVENT_HIRE_EXPIRED, std::chrono::minutes(configFSBHireDuration3 * 60));
                    events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 100ms);
                    //events.ScheduleEvent(FSB_EVENT_PRIEST_CHECK_BUFFS, 100ms);
                    //events.ScheduleEvent(FSB_EVENT_PRIEST_CHECK_HP, 100ms);

                    //me->SetReactState(REACT_ASSIST);
                    //me->SetReactState(REACT_AGGRESSIVE);
                    events.ScheduleEvent(FSB_EVENT_CHECK_COMBAT, 2s);

                    std::string msg = BuildNPCSayText(player->GetName(), configFSBHireDuration3, FSBSayType::Hire, "");
                    me->Say(msg, LANG_UNIVERSAL);
                    //me->Say(FSB_SAY_HIRED120, LANG_UNIVERSAL);
                }
                else
                    player->GetSession()->SendNotification(FSB_PLAYER_NOTIFICATION_PAYMENT_FAIL);

                player->PlayerTalkClass->SendCloseGossip();
                return true;
            }

                // Bot Hire Option Permanent
            case GOSSIP_ACTION_INFO_DEF + 13:
            {
                // End price is base cost per level times player level
                if (player->HasEnoughMoney(int64(configFSBPermanentPricePerLevel * pLevel)))
                {
                    player->ModifyMoney(-int64(configFSBPermanentPricePerLevel * pLevel));
                    player->GetSession()->SendNotification(FSB_PLAYER_NOTIFICATION_PAYMENT_SUCCESS);

                    hired = true;

                    events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 100ms);
                    //events.ScheduleEvent(FSB_EVENT_PRIEST_CHECK_BUFFS, 100ms);
                    //events.ScheduleEvent(FSB_EVENT_PRIEST_CHECK_HP, 100ms);

                    //me->SetReactState(REACT_ASSIST);
                    //me->SetReactState(REACT_AGGRESSIVE);
                    events.ScheduleEvent(FSB_EVENT_CHECK_COMBAT, 2s);

                    std::string msg = BuildNPCSayText(player->GetName(), NULL, FSBSayType::PHire, "");
                    me->Say(msg, LANG_UNIVERSAL);
                    //me->Say(FSB_SAY_HIRED120, LANG_UNIVERSAL);
                }
                else
                    player->GetSession()->SendNotification(FSB_PLAYER_NOTIFICATION_PAYMENT_FAIL);

                player->PlayerTalkClass->SendCloseGossip();
                return true;
            }

                // Bot Follow Distance Option 1
            case GOSSIP_ACTION_INFO_DEF + 14:
            {
                followDistance = FOLLOW_DISTANCE_CLOSE;
                updateFollowInfo = true;
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 100ms);
                break;
            }

                // Bot Follow Distance Option 2
            case GOSSIP_ACTION_INFO_DEF + 15:
            {
                followDistance = FOLLOW_DISTANCE_NORMAL;
                updateFollowInfo = true;
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 100ms);
                break;
            }

                // Bot Follow Distance Option 3
            case GOSSIP_ACTION_INFO_DEF + 16:
            {
                followDistance = FOLLOW_DISTANCE_WIDE;
                updateFollowInfo = true;
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 100ms);
                break;
            }

                // Bot Follow Angle Option 1
            case GOSSIP_ACTION_INFO_DEF + 17:
            {
                followAngle = FOLLOW_ANGLE_FRONT;
                updateFollowInfo = true;
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 1s);
                break;
            }

                // Bot Follow Angle Option 2
            case GOSSIP_ACTION_INFO_DEF + 18:
            {
                followAngle = FOLLOW_ANGLE_BEHIND;
                updateFollowInfo = true;
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 1s);
                break;
            }

                // Bot Follow Angle Option 3
            case GOSSIP_ACTION_INFO_DEF + 19:
            {
                followAngle = GetRandomRightAngle(); //FOLLOW_ANGLE_RIGHT;
                updateFollowInfo = true;
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 1s);
                break;
            }

                // Bot Stay Option
            case GOSSIP_ACTION_INFO_DEF + 20:
            {
                events.ScheduleEvent(FSB_EVENT_MOVE_STAY, 100ms);
                break;
            }

                // Bot Follow Option
            case GOSSIP_ACTION_INFO_DEF + 21:
            {
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 100ms);
                break;
            }

                // Bot Follow Angle Option 4
            case GOSSIP_ACTION_INFO_DEF + 22:
            {
                followAngle = GetRandomLeftAngle(); //FOLLOW_ANGLE_LEFT;
                updateFollowInfo = true;
                events.ScheduleEvent(FSB_EVENT_MOVE_FOLLOW, 1s);
                break;
            }

                // Bot Role Option Assist
            case GOSSIP_ACTION_INFO_DEF + 23:
            {
                roleState = FSB_ROLE_STATE_ASSIST;
                me->SetReactState(REACT_DEFENSIVE);
                if (me->HasAura(SPELL_PRIEST_SHADOWFORM))
                    me->RemoveAurasDueToSpell(SPELL_PRIEST_SHADOWFORM);
                me->Say(FSB_SAY_FOLLOW_INFO_CHANGED, LANG_UNIVERSAL);
                break;
            }

                // Bot Role Option Damage Deal
            case GOSSIP_ACTION_INFO_DEF + 24:
            {
                roleState = FSB_ROLE_STATE_DAMAGE;
                me->SetReactState(REACT_AGGRESSIVE);
                me->CastSpell(me, SPELL_PRIEST_SHADOWFORM);
                me->Say(FSB_SAY_FOLLOW_INFO_CHANGED, LANG_UNIVERSAL);
                break;
            }

            // Bot Role Option Balanced
            case GOSSIP_ACTION_INFO_DEF + 25:
            {
                roleState = FSB_ROLE_STATE_BALANCED;
                me->SetReactState(REACT_ASSIST);
                if (me->HasAura(SPELL_PRIEST_SHADOWFORM))
                    me->RemoveAurasDueToSpell(SPELL_PRIEST_SHADOWFORM);
                me->Say(FSB_SAY_FOLLOW_INFO_CHANGED, LANG_UNIVERSAL);
                break;
            }

            default:
                break;
            }
            CloseGossipMenuFor(player);
            return true;
        }

        void JustEngagedWith(Unit* who) override // Runs every time creature gets in combat
        {
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Entered JustEngagedWith: {}", who->GetName());

            if(roleState == FSB_ROLE_STATE_BALANCED || roleState == FSB_ROLE_STATE_DAMAGE)
                events.ScheduleEvent(FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_SELF, 100ms);
            //me->DoMeleeAttackIfReady();
            //DoZoneInCombat();
        }

        // Runs every time creature takes damage
        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
        }

        void EnterEvadeMode(EvadeReason /*why*/) override // Runs every time creature evades
        {

        }

        void KilledUnit(Unit* victim) override // Runs every time the creature kills an unit
        {
                //if (victim->GetTypeId() == TYPEID_PLAYER)
                //    Talk(SAY_KILL);
        }

        void OnSpellCast(SpellInfo const* spell) override // Runs every time the creature casts a spell
        {
            TC_LOG_DEBUG("scripts.core.fsb", "FSB Priest Casting spell id {}", spell->Id);
        }

        void SpellHit(WorldObject* caster, SpellInfo const* spellInfo) override
        {
            /*
            // Only react some of the time
             // 10% chance to say something

            if (urand(0, 99) >= REACT_BUFFED_CHANCE_PERCENT)
                return; // Skip reaction this time

            Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);
            if (!player)
                return;

            if (caster == me)
                return;

            // Positive spell detection
            if (spellInfo->IsPositive()) // TrinityCore helper
            {
                std::string msg = BuildNPCSayText(player->GetName(), NULL, FSBSayType::Buffed);
                me->Say(msg, LANG_UNIVERSAL);
            }
            */
        }

        void SpellHitTarget(WorldObject* target, SpellInfo const* spellInfo) override
        {

        }

        void JustSummoned(Creature* summon) override // Runs every time the creature summons another creature
        {

        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override // Runs everytime the creature's summon dies - pet or minion
        {

        }

        void JustDied(Unit* /*killer*/) override // Runs once when creature dies
        {

        }

        uint32 GetData(uint32 type) const override // Runs once to check what data exists on the creature
        {
            return 0;
        }

        void SetData(uint32 type, uint32 value) override // Runs once to check set data is SET on the creature
        {

        }

        void DoAction(int32 action) override
        {
            switch (action)
            {
            case FSB_ACTION_COMBAT_SPELLS:
            {
                TC_LOG_DEBUG("scripts.ai.fsb", "FSB DoAction triggered: Combat Spells");

                uint32 now = getMSTime();

                if (_globalCooldownUntil > now)
                    return;

                // =============================
                // EMERGENCY: Desperate Prayer
                // =============================
                if (me->IsAlive())
                {
                    if (me->GetHealthPct() <= 15.f && now >= _desperatePrayerReadyMs && !me->HasUnitState(UNIT_STATE_CASTING))
                    {
                        TC_LOG_DEBUG("scripts.ai.core", "FSB Emergency: Desperate Prayer");

                        me->CastSpell(me, SPELL_PRIEST_DESPERATE_PRAYER, false);

                        _desperatePrayerReadyMs = now + DESPERATE_PRAYER_CD_MS; // 90 sec cooldown (adjust if needed)
                        _globalCooldownUntil = now + NPC_GCD_MS;

                        me->Say("I'm getting desperate here!!!", LANG_UNIVERSAL);

                        return; // NOTHING overrides this
                    }
                }
                // =============================

                //Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);

                Unit* owner = me->GetOwner();
                Player* player = owner ? owner->ToPlayer() : nullptr;

                // =============================
                // HEALING: Player or Bot based on Role
                // =============================
                bool castedHealThisTick = false; // reset each tick

                std::vector<Unit*> healCandidates;

                // Define HP thresholds per role
                float lowHpThreshold = 30.f;
                float midHpThreshold = 55.f;
                float highHpThreshold = 70.f;

                switch (roleState)
                {
                case FSB_ROLE_STATE_ASSIST:
                    // Heal player first, then self if needed
                    if (player && player->IsAlive() && player->GetHealthPct() < 90)
                        healCandidates.push_back(player);

                    if (me->GetHealthPct() < 40)
                        healCandidates.push_back(me);
                    break;

                case FSB_ROLE_STATE_DAMAGE:
                    // Only heal self
                    if (me->GetHealthPct() < 60)
                        healCandidates.push_back(me);
                    break;

                case FSB_ROLE_STATE_BALANCED:
                    // Heal bot first, then player
                    if (me->GetHealthPct() < 70)
                        healCandidates.push_back(me);

                    if (player && player->IsAlive() && player->GetHealthPct() < 70)
                        healCandidates.push_back(player);
                    break;
                }

                Unit* healTarget = nullptr;
                float highestRisk = 0.f;

                for (Unit* u : healCandidates)
                {
                    if (!u || !u->IsAlive())
                        continue;

                    float risk = 100.f - u->GetHealthPct(); // low HP = higher risk
                    if (me->IsInCombat())
                        risk += me->GetThreatManager().GetThreat(u) * 0.01f;

                    if (risk > highestRisk)
                    {
                        highestRisk = risk;
                        healTarget = u;
                    }
                }

                if (healTarget)
                {
                    std::vector<FSBotSpells>* healTable = nullptr;

                    switch (roleState)
                    {
                    case FSB_ROLE_STATE_ASSIST:
                        healTable = &AssistHealSpells;
                        break;
                    case FSB_ROLE_STATE_BALANCED:
                        healTable = &BalancedHealSpells;
                        break;
                    case FSB_ROLE_STATE_DAMAGE:
                        healTable = &SelfHealSpells;
                        break;
                    }

                    for (auto& spell : *healTable)
                    {
                        if (spell.nextReadyMs > now)
                            continue;

                        float hpPct = healTarget->GetHealthPct();

                        // Role-based thresholds
                        bool skip = false;

                        switch (spell.spellId)
                        {
                        case SPELL_PRIEST_HEAL:
                            skip = hpPct > lowHpThreshold;
                            break;
                        case SPELL_PRIEST_FLASH_HEAL:
                            skip = hpPct > midHpThreshold;
                            break;
                        case SPELL_PRIEST_RENEW:
                            skip = hpPct > highHpThreshold;
                            break;
                        default:
                            break;
                        }

                        if (skip)
                            continue;

                        if (spell.chance < frand(0.f, 100.f))
                            continue;

                        if (!healTarget->HasAura(spell.spellId))
                        {
                            me->CastSpell(healTarget, spell.spellId, false);
                            castedHealThisTick = true;
                            spell.nextReadyMs = now + spell.cooldownMs;
                            _globalCooldownUntil = now + NPC_GCD_MS;

                            if (now >= _nextCombatSayMs)
                            {
                                if (healTarget == me)
                                {
                                    std::string msg = BuildNPCSayText("", NULL, FSBSayType::HealSelf, "");
                                    me->Say(msg, LANG_UNIVERSAL);
                                    // Random cooldown between 3-5 minutes
                                    _nextCombatSayMs = now + urand(3 * MINUTE * IN_MILLISECONDS,
                                        5 * MINUTE * IN_MILLISECONDS);
                                }
                                else if (healTarget == player && player)
                                {
                                    std::string msg = BuildNPCSayText(player->GetName(), NULL, FSBSayType::HealTarget, "");
                                    me->Say(msg, LANG_UNIVERSAL);
                                    // Random cooldown between 3-5 minutes
                                    _nextCombatSayMs = now + urand(3 * MINUTE * IN_MILLISECONDS,
                                        5 * MINUTE * IN_MILLISECONDS);
                                }
                                else
                                {
                                    if (healTarget)
                                    {
                                        // We may need this one if we have more bots at the same time
                                        std::string msg = BuildNPCSayText(healTarget->GetName(), NULL, FSBSayType::HealTarget, "");
                                        me->Say(msg, LANG_UNIVERSAL);
                                        // Random cooldown between 3-5 minutes
                                        _nextCombatSayMs = now + urand(3 * MINUTE * IN_MILLISECONDS,
                                            5 * MINUTE * IN_MILLISECONDS);
                                    }
                                }
                            }

                            // Only one heal per tick
                        }
                        break;
                    }
                }

                // -------------------------
                // 2?? Combat logic
                // -------------------------

                if (castedHealThisTick)
                    return;

                std::vector<Unit*> combatTargets;

                if (roleState == FSB_ROLE_STATE_BALANCED)
                {
                    if (player && player->IsAlive())
                    {

                        // Attack player's target(s)
                        if (Unit* playerTarget = player->GetSelectedUnit())
                            combatTargets.push_back(playerTarget);
                    }
                    else
                    {
                        auto const& threatRefs = me->GetThreatManager().GetSortedThreatList();
                        for (ThreatReference const* ref : threatRefs)
                        {
                            if (Unit* u = ref->GetVictim())
                                combatTargets.push_back(u);
                        }
                    }
                }
                else if (roleState == FSB_ROLE_STATE_DAMAGE)
                {
                    // Aggroed mobs from NPC & Player AND player's target
                    auto const& threatRefs = me->GetThreatManager().GetSortedThreatList();
                    for (ThreatReference const* ref : threatRefs)
                    {
                        if (Unit* u = ref->GetVictim())
                            combatTargets.push_back(u);
                    }

                    auto const& threatRefs2 = player->GetThreatManager().GetSortedThreatList();
                    if (player && player->IsAlive())
                    {
                        if (Unit* playerTarget = player->GetSelectedUnit())
                            combatTargets.push_back(playerTarget);

                        for (ThreatReference const* ref2 : threatRefs2)
                        {
                            if (Unit* u = ref2->GetVictim())
                                combatTargets.push_back(u);
                        }
                    }
                }
                else if (roleState == FSB_ROLE_STATE_ASSIST)
                {
                    // Only mobs aggroed by the bot
                    // We are a healer after all
                    auto const& threatRefs = me->GetThreatManager().GetSortedThreatList();
                    for (ThreatReference const* ref : threatRefs)
                    {
                        if (Unit* u = ref->GetVictim())
                            combatTargets.push_back(u);
                    }
                }

                for (Unit* target : combatTargets)
                {
                    //if (!target || !target->IsAlive() || !me->IsHostileTo(target))
                    //    continue;
                    if (target && target->IsAlive())
                    {

                        // Sync combat state with target
                        if (!me->GetThreatManager().IsThreatenedBy(target))
                        {
                            me->GetThreatManager().AddThreat(target, 100.0f);
                            //me->SetInCombatWith(target);
                            //target->SetInCombatWith(me);
                            me->Attack(target, true);
                        }

                        for (auto& spell : CombatSpells)
                        {
                            if (spell.nextReadyMs > now)
                                continue;

                            // ---------- Psychic Scream special case ----------
                            if (spell.spellId == SPELL_PRIEST_PSYCHIC_SCREAM)
                            {
                                if (CountAttackersOn(me) + (player ? CountAttackersOn(player) : 0) < 3)
                                    continue;

                                if (!me->IsWithinDistInMap(target, 8.0f))
                                    continue;

                                StopFollowing();

                                me->CastSpell(me, spell.spellId, false);
                                spell.nextReadyMs = now + spell.cooldownMs;
                                _globalCooldownUntil = now + NPC_GCD_MS;

                                ResumeFollow(player);
                                return;
                            }

                            // --------------- Role Based Handling -------
                            if (roleState == FSB_ROLE_STATE_DAMAGE)
                            {
                                if (spell.spellId == SPELL_PRIEST_SMITE || spell.spellId == SPELL_PRIEST_HOLY_FIRE)
                                    continue;
                            }

                            if (roleState == FSB_ROLE_STATE_ASSIST || roleState == FSB_ROLE_STATE_BALANCED)
                            {
                                if (spell.spellId == SPELL_PRIEST_MIND_FLAY || spell.spellId == SPELL_PRIEST_VAMPIRIC_TOUCH)
                                    continue;
                            }

                            // ---------- Ranged spell movement ----------
                            float dist = me->GetDistance(target);
                            float desiredRange = frand(SPELL_MIN_RANGE, SPELL_MAX_RANGE);

                            if (dist > desiredRange)
                            {
                                StopFollowing();

                                me->GetMotionMaster()->MoveChase(target, desiredRange);
                                return; // movement is the action this tick
                            }

                            // ---------- Chance roll ----------
                            if (spell.chance < frand(0.f, 100.f))
                                continue;

                            // ---------- Cast ----------
                            StopFollowing();

                            if (!target->HasAura(spell.spellId))
                                me->CastSpell(target, spell.spellId, false);
                            else continue;

                            spell.nextReadyMs = now + spell.cooldownMs;
                            _globalCooldownUntil = now + NPC_GCD_MS;

                            ResumeFollow(player);

                            if (now >= _nextCombatSayMs)
                            {
                                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spell.spellId, DIFFICULTY_NONE);
                                std::string spellName = "unknown spell";

                                if (spellInfo && spellInfo->SpellName)
                                {
                                    spellName = spellInfo->SpellName->Str[LOCALE_enUS];

                                    if (spellName.empty())
                                        spellName = "unknown spell";
                                }

                                std::string msg = BuildNPCSayText(target->GetName(), NULL, FSBSayType::SpellOnTarget, spellName);
                                me->Say(msg, LANG_UNIVERSAL);
                                // Random cooldown between 3-5 minutes
                                _nextCombatSayMs = now + urand(3 * MINUTE * IN_MILLISECONDS,
                                    5 * MINUTE * IN_MILLISECONDS);
                            }


                            // Only cast one combat spell per tick
                            break;
                        }
                    }
                }

                break;
            }
            case FSB_ACTION_RESURRECT_PLAYER:
            {
                if (!_pendingResurrection)
                    return;

                //Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);
                Unit* owner = me->GetOwner();
                Player* player = owner ? owner->ToPlayer() : nullptr;

                if (!player)
                    return;

                // Must be fully safe
                if (me->IsInCombat() || player->IsInCombat())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (player->IsAlive())
                {
                    _pendingResurrection = false;
                    return;
                }

                // Must be close enough (important!)
                if (!me->IsWithinDistInMap(player, 30.0f))
                    return;

                TC_LOG_DEBUG("scripts.ai.fsb", "FSB: ACTION Resurrect Player");

                me->StopMoving(); // optional but immersive
                me->CastSpell(player, SPELL_PRIEST_RESURRECTION, false);

                break;
            }
            case FSB_ACTION_INITIAL_COMBAT_SPELLS_SELF:
            {
                uint32 now = getMSTime();

                if (_globalCooldownUntil < now && !me->HasUnitState(UNIT_STATE_CASTING))
                {

                    if (!me->HasAura(SPELL_PRIEST_POWER_WORD_SHIELD))
                    {
                        me->CastSpell(me, SPELL_PRIEST_POWER_WORD_SHIELD);
                        _globalCooldownUntil = now + NPC_GCD_MS;
                        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Initial Combat SELF Spell Cast: PWS");
                        events.ScheduleEvent(FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_SELF, 1500ms);
                    }
                    else if (!me->HasAura(SPELL_PRIEST_RENEW))
                    {
                        me->CastSpell(me, SPELL_PRIEST_RENEW);
                        _globalCooldownUntil = now + NPC_GCD_MS;
                        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Initial Combat SELF Spell Cast: Renew");
                    }
                }
                else {
                    events.ScheduleEvent(FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_SELF, 1500ms);
                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Initial Combat SELF Spell Cast: GCD");
                }
                break;
            }

            case FSB_ACTION_INITIAL_COMBAT_SPELLS_PLAYER:
            {
                uint32 now = getMSTime();

                if (_globalCooldownUntil < now)
                {
                    Unit* owner = me->GetOwner();
                    Player* player = owner ? owner->ToPlayer() : nullptr;

                    if (!player->HasAura(SPELL_PRIEST_POWER_WORD_SHIELD))
                    {
                        me->CastSpell(player, SPELL_PRIEST_POWER_WORD_SHIELD);
                        _globalCooldownUntil = now + NPC_GCD_MS;
                        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Initial Combat PLAYER Spell Cast: PWS");
                        events.ScheduleEvent(FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_PLAYER, 1500ms);
                    }
                    else if (!player->HasAura(SPELL_PRIEST_RENEW))
                    {
                        me->CastSpell(player, SPELL_PRIEST_RENEW);
                        _globalCooldownUntil = now + NPC_GCD_MS;
                        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Initial Combat PLAYER Spell Cast: Renew");
                    }
                }
                else {
                    events.ScheduleEvent(FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_PLAYER, 1500ms);
                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Initial Combat PLAYER Spell Cast: GCD");
                }
                break;
            }

            default:
                break;
            }
        }



        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            //Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);
            Unit* owner = me->GetOwner();
            Player* player = owner ? owner->ToPlayer() : nullptr;

            if (!player)
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case FSB_EVENT_PRIEST_COMBAT_SPELLS:
                {
                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB: UpdateAI Event Priest Combat Spells triggered with role = {}", roleState);

                    DoAction(FSB_ACTION_COMBAT_SPELLS);

                    // ?? ALWAYS reschedule while combat exists
                    if (IsCombatActive())
                    {
                        events.ScheduleEvent(FSB_EVENT_PRIEST_COMBAT_SPELLS, 1s);
                    }
                    break;
                }

                    // Priest casts Renew and PWS on Engagement
                    // Only in Balanced and Damage roles
                    // Triggered from: JustEngagedWith()
                case FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_SELF:
                {
                    if (me->IsInCombat() && me->IsAlive())
                    {
                        DoAction(FSB_ACTION_INITIAL_COMBAT_SPELLS_SELF);
                    }

                    break;
                }

                    // Priest casts Renew and PWS on Player
                    // Only in Assist role
                    // Triggered from Periodic Maintenance check and flag
                case FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_PLAYER:
                {
                    DoAction(FSB_ACTION_INITIAL_COMBAT_SPELLS_PLAYER);
                    break;
                }

                case FSB_EVENT_HIRE_EXPIRED:
                case FSB_EVENT_HIRE_DISMISSED:
                {
                    hired = false;
                    me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);
                    events.Reset();
                    events.ScheduleEvent(FSB_EVENT_MOVE_STAY, 100ms);
                    //me->Say("Hey pal, my time on the job is up. I am out of here.", LANG_UNIVERSAL);
                    std::string msg = BuildNPCSayText(player->GetName(), NULL, FSBSayType::Fire, "");
                    me->Say(msg, LANG_UNIVERSAL);
                    events.ScheduleEvent(FSB_EVENT_HIRE_LEAVE, 5s);
                    break;
                }

                case FSB_EVENT_HIRE_LEAVE:
                {
                    //float x, y, z;
                    //me->GetRandomPoint(me->GetPosition(), 10.0f, x, y, z); // 10 yd radius
                    //me->GetMotionMaster()->MovePoint(1, me->GetPositionX() + 30, me->GetPositionY(), me->GetPositionZ(), true);
                    float x, y, z;
                    me->GetRandomPoint(me->GetPosition(), 30.0f, x, y, z);
                    me->GetMotionMaster()->MovePoint(1, x, y, z);
                    me->DespawnOrUnsummon(10s);
                    break;
                }

                case FSB_EVENT_MOVE_STAY:
                {
                    moveState = FSB_MOVE_STATE_STAY;
                    me->GetMotionMaster()->Clear();
                    me->StopMoving();
                    if (hired)
                    {
                        std::string msg = BuildNPCSayText(player->GetName(), NULL, FSBSayType::Stay, "");
                        me->Say(msg, LANG_UNIVERSAL);
                    }
                    break;
                }

                case FSB_EVENT_MOVE_FOLLOW:
                {
                    moveState = FSB_MOVE_STATE_FOLLOWING;
                    me->GetMotionMaster()->MoveFollow(player, followDistance, followAngle);

                    if (updateFollowInfo)
                    {
                        updateFollowInfo = false;
                        me->Say(FSB_SAY_FOLLOW_INFO_CHANGED, LANG_UNIVERSAL);
                    }
                    else
                    {
                        std::string msg = BuildNPCSayText(player->GetName(), NULL, FSBSayType::Follow, "");
                        me->Say(msg, LANG_UNIVERSAL);
                    }
                    break;
                }

                case FSB_EVENT_PERIODIC_MAINTENANCE:
                {
                    uint32 now = getMSTime();

                    bool castedThisTick = false;

                    //Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);

                    Unit* owner = me->GetOwner();
                    Player* player = owner ? owner->ToPlayer() : nullptr;

                    // BOT check if player dead
                    if (player && !player->IsAlive() && !_pendingResurrection)
                    {
                        if (!_playerDead)
                        {
                            std::string msg = BuildNPCSayText(player->GetName(), NULL, FSBSayType::Resurrect, "");
                            me->Say(msg, LANG_UNIVERSAL);
                        }

                        _playerDead = true;
                        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Player found dead in Periodic Check");

                        if (!me->IsInCombat())
                        {
                            _pendingResurrection = true;
                            DoAction(FSB_ACTION_RESURRECT_PLAYER);
                        }
                    }
                    else if (player && player->IsAlive() && _pendingResurrection && _playerDead)
                    {
                        _pendingResurrection = false;
                        _playerDead = false;
                        
                        std::string msg = BuildNPCSayText(player->GetName(), NULL, FSBSayType::Resurrect, "");
                        me->Say(msg, LANG_UNIVERSAL);
                    }

                    // BOT LEVEL Update
                    if (player && hired)
                    {
                        uint8 pLevel = player->GetLevel();
                        if (me->GetLevel() != pLevel)
                        {
                            me->SetLevel(pLevel);
                            // Recalculate stats properly
                            me->UpdateLevelDependantStats();

                            // Reset health & power cleanly
                            me->SetHealth(me->GetMaxHealth());
                            me->SetPower(me->GetPowerType(), me->GetMaxPower(me->GetPowerType()));
                        }
                    }

                    // BOT CAST Recuperate & OOC Heal / Mana
                    if(!me->IsInCombat() && me->IsAlive() && !me->HasUnitState(UNIT_STATE_CASTING))
                    {
                        if (CanCastNow(now) && !castedThisTick && me->GetPowerPct(POWER_MANA) < 20)
                        {
                            if (!me->HasAura(22734))
                            {
                                me->CastSpell(me, 22734); // Conjured Crystal Water
                                _globalCooldownUntil = now + NPC_GCD_MS;
                                castedThisTick = true;
                                me->Say("I'm gonna take a mana break!", LANG_UNIVERSAL); // TO-DO maybe turn this into a choice of lines
                            }
                            // TO-DO find some mana refil spells and move after global cooldown check
                        }

                        if (CanCastNow(now))
                        {

                            if (!castedThisTick && me->GetHealthPct() <= 50 && now >= _recuperateReadyMs)
                            {
                                TC_LOG_DEBUG("scripts.ai.core", "FSB Out-of-combat: Recuperate");

                                me->Say("I need a breather!", LANG_UNIVERSAL); // TO-DO maybe turn this into a choice of lines

                                me->CastSpell(me, SPELL_PRIEST_RECUPERATE, false);

                                castedThisTick = true;
                                _recuperateReadyMs = now + 60000; // 1 minute
                                _globalCooldownUntil = now + NPC_GCD_MS;
                            }
                            else if (!castedThisTick && me->GetHealthPct() <= 70)
                            {
                                me->CastSpell(me, SPELL_PRIEST_HEAL, false);

                                _globalCooldownUntil = now + NPC_GCD_MS;
                                castedThisTick = true;
                            }
                        }
                    }

                    // BOT OOM In Combat
                    if (me->IsInCombat() && me->IsAlive())
                    {
                        if (me->GetPowerPct(POWER_MANA) < 10)
                        {
                            me->Say("Please wrap this up, I'm out of mana", LANG_UNIVERSAL); // TO-DO maybe turn this into a choice of lines

                            // TO-DO find some combat/instant mana refil spells
                        }
                    }

                    // Fortitude Buff check
                    if (!me->IsInCombat() && me->IsAlive())
                    {
                        Unit* fortTarget = nullptr;

                        if (player && player->IsAlive() && !player->HasAura(SPELL_PRIEST_POWER_WORD_FORTITUDE))
                            fortTarget = player;
                        else if (!me->HasAura(SPELL_PRIEST_POWER_WORD_FORTITUDE))
                            fortTarget = me;

                        if (fortTarget && !castedThisTick && CanCastNow(now))
                        {
                            me->CastSpell(fortTarget, SPELL_PRIEST_POWER_WORD_FORTITUDE);
                            castedThisTick = true;
                            _globalCooldownUntil = now + NPC_GCD_MS;

                            TC_LOG_DEBUG("scripts.ai.core", "FSB: UpdateAI Event Maintenance Fortitude buff check");

                            if(fortTarget == me)
                            {
                                me->Say("Am gonna need this buff!", LANG_UNIVERSAL); // TO-DO maybe turn this into a choice of lines
                            }
                            else
                                me->Say("Here's something for ya!", LANG_UNIVERSAL); // TO-DO maybe turn this into a choice of lines
                        }
                    }

                    // BOT PLAYER OOC Heal
                    if (!me->IsInCombat() && me->IsAlive() && !me->HasUnitState(UNIT_STATE_CASTING))
                    {
                        if (CanCastNow(now) && player && !player->IsInCombat())
                        {

                            if (!castedThisTick && player->GetHealthPct() <= 50)
                            {
                                TC_LOG_DEBUG("scripts.ai.core", "FSB Out-of-combat: Player Heal");

                                me->CastSpell(player, SPELL_PRIEST_HEAL, false);
                                _globalCooldownUntil = now + NPC_GCD_MS;
                                castedThisTick = true;

                                // Only react some of the time
                                // 10% chance to say something

                                if (urand(0, 99) <= REACT_BUFFED_CHANCE_PERCENT)
                                    me->Say(BuildNPCSayText(player->GetName(), NULL, FSBSayType::HealTarget, ""), LANG_UNIVERSAL);
                            }
                            else if (!castedThisTick && player->GetHealthPct() <= 70)
                            {
                                me->CastSpell(player, SPELL_PRIEST_FLASH_HEAL, false);
                                _globalCooldownUntil = now + NPC_GCD_MS;
                                castedThisTick = true;

                                // Only react some of the time
                                // 10% chance to say something

                                if (urand(0, 99) <= REACT_BUFFED_CHANCE_PERCENT)
                                    me->Say(BuildNPCSayText(player->GetName(), NULL, FSBSayType::HealTarget, ""), LANG_UNIVERSAL);
                            }
                        }

                    }

                    // BOT Check move states
                    if (hired && me->IsAlive())
                    {
                        MotionMaster* mm = me->GetMotionMaster();            

                        MovementGeneratorType type = mm->GetCurrentMovementGeneratorType();
                        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Event Periodic Maintenance Movement check: type {} and move state {}", type, moveState);
                        if (hired && moveState == FSB_MOVE_STATE_FOLLOWING && type == IDLE_MOTION_TYPE)
                        {
                            ResumeFollow(player);
                            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Event Periodic Maintenance we are following");
                        }

                    }

                    // PLAYER check on combat for buffs and combat spells
                    // Different Triggers per role
                    if (player && player->IsAlive() && player->IsInCombat() && !_playerCombatStarted)
                    {
                        _playerCombatStarted = true;

                        if(roleState == FSB_ROLE_STATE_ASSIST)
                            events.ScheduleEvent(FSB_EVENT_PRIEST_INITIAL_COMBAT_SPELLS_PLAYER, 100ms);
                        
                        events.ScheduleEvent(FSB_EVENT_PRIEST_COMBAT_SPELLS, 2s);
                    }

                    if (player && player->IsAlive() && !player->IsInCombat() && _playerCombatStarted)
                    {
                        _playerCombatStarted = false;
                    }

                    events.ScheduleEvent(FSB_EVENT_PERIODIC_MAINTENANCE, 1s);

                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Event Periodic Maintenance Reached the end");

                    break;
                }

                default:
                    break;
                }
            }
        }

        // Helper methods

        // Check if BOT can cast spell due to GCD
        bool CanCastNow(uint32 now) const
        {
            return now >= _globalCooldownUntil && !me->HasUnitState(UNIT_STATE_CASTING);
        }

        // Check if BOT or PLAYER in combat
        bool IsCombatActive() const
        {
            Unit* owner = me->GetOwner();
            Player* player = owner ? owner->ToPlayer() : nullptr;
            //Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);
            return me->IsInCombat() || (player && player->IsInCombat());
        }

        /// Count Attackers
        /// 
        /// Checks how many active mobs are having Threat on BOT
        /// Returns a count
        uint8 CountActiveAttackers()
        {
            uint8 count = 0;

            for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
            {
                Unit* u = ref->GetVictim();
                if (!u || !u->IsAlive())
                    continue;

                if (!me->IsInMap(u))
                    continue;

                // Optional: only count if actually targeting us
                //if (u->GetVictim() == me)
                    ++count;
            }

            return count;
        }

        /// Count Attackers on
        ///
        /// Checks how many active mobs are having Threat on specified UNIT
        /// Requires Unit type parameter
        uint8 CountAttackersOn(Unit* who)
        {
            uint8 count = 0;

            for (ThreatReference const* ref : who->GetThreatManager().GetUnsortedThreatList())
            {
                Unit* u = ref->GetVictim();
                if (u && u->IsAlive() && u->GetVictim() == who)
                    ++count;
            }
            return count;
        }

        // Stop Follow
        void StopFollowing()
        {
            //if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == FOLLOW_MOTION_TYPE)
            me->StopMoving();
            me->GetMotionMaster()->Clear();
            moveState = FSB_MOVE_STATE_IDLE;
        }

        // Resume Follow
        void ResumeFollow(Player* player)
        {
            if (!player)
                return;

            me->GetMotionMaster()->MoveFollow(player, followDistance, followAngle);
            moveState = FSB_MOVE_STATE_FOLLOWING;
        }

    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_followship_bots_priestAI(creature);
    }
};


void AddSC_followship_bots_priest()
{
    new npc_followship_bots_priest();
}
