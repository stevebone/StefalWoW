#include <algorithm>
#include <vector>

#include "Containers.h"

#include "CreatureAI.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "ThreatManager.h"

#include "Followship_bots.h"
#include "Followship_bots_config.h"
#include "Followship_bots_utils.h"
#include "Followship_bots_ai_base.h"
#include "Followship_bots_db.h"
#include "Followship_bots_mgr.h"

struct FSBEntryClassMap
{
    uint32 entry;
    FSB_Class botClass;
};

static constexpr FSBEntryClassMap BotEntryClassTable[] =
{
    // PRIESTS
    { 141508,   FSB_Class::Priest },  // Stormwind Priest
    { 375,      FSB_Class::Priest },     // Priestess Anetta

    // WARRIORS
    //{ 90010, FSB_Class::Warrior },
    //{ 90011, FSB_Class::Warrior },

    // MAGES
    //{ 90020, FSB_Class::Mage },

    // ROGUES
    //{ 90030, FSB_Class::Rogue },
};

namespace FSBUtils
{
    void SetInitialState(Creature* creature, bool& hired, uint16& moveState)
    {
        ASSERT(creature);

        creature->SetBot(true);
        creature->setActive(true);

        hired = false;                  // now persists
        moveState = FSB_MOVE_STATE_IDLE;

        // Initial Flags and States
        creature->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
        creature->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);

        creature->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
        creature->SetReactState(REACT_DEFENSIVE);
        creature->SetFaction(FSB_FACTION_ALLIANCE);

        creature->UpdateLevelDependantStats();
        creature->SetHealth(creature->GetMaxHealth());
        creature->SetPower(creature->GetPowerType(), creature->GetMaxPower(creature->GetPowerType()));
    }

    void BotUpdateAllies(Creature* bot, GuidSet _allySet)
    {
        Unit* owner = FSBMgr::GetBotOwner(bot);

        if (!owner)
            return;

        Group* group = nullptr;
        if (Player* player = owner->ToPlayer())
            group = player->GetGroup();

        // only pet and owner/not in group->ok
        if (_allySet.size() == 1 && !group)
            return;

        // owner is in group; group members filled in already (no raid -> subgroupcount = whole count)
        if (group && !group->isRaidGroup() && _allySet.size() == (group->GetMembersCount() + 2))
            return;

        _allySet.clear();
        _allySet.insert(bot->GetGUID());
        if (group) // add group
        {
            for (GroupReference const& itr : group->GetMembers())
            {
                Player* Target = itr.GetSource();
                if (!Target->IsInMap(owner) || !group->SameSubGroup(owner->ToPlayer(), Target))
                    continue;

                if (Target->GetGUID() == owner->GetGUID())
                    continue;

                _allySet.insert(Target->GetGUID());
            }
        }
        else // remove group
            _allySet.insert(owner->GetGUID());
    }

    FSB_Class GetBotClassForEntry(uint32 entry)
    {
        for (auto const& map : BotEntryClassTable)
        {
            if (map.entry == entry)
                return map.botClass;
        }

        return FSB_Class::None;
    }

    void SetBotClass(Creature* creature, FSB_Class& outClass)
    {
        if (!creature)
            return;

        FSB_Class cls = GetBotClassForEntry(creature->GetEntry());

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Class set: {} for bot with entry {}", cls, creature->GetEntry());

        if (cls == FSB_Class::None)
        {
            TC_LOG_WARN("scripts.ai.fsb", "FSB: No class mapping found for creature entry {}", creature->GetEntry());
        }

        outClass = cls;
    }


    float GetRandomLeftAngle()
    {
        // Base left is +90 degrees (M_PI/2)
        // Add a random small offset for natural variation
        return (M_PI / 2.0f) + frand(-SIDE_OFFSET_MAX, SIDE_OFFSET_MAX);
    }

    float GetRandomRightAngle()
    {
        // Base right is -90 degrees (-M_PI/2)
        // Add a random small offset for natural variation
        return (-M_PI / 2.0f) + frand(-SIDE_OFFSET_MAX, SIDE_OFFSET_MAX);
    }

    FSB_Roles GetRole(Creature* unit)
    {
        if (!unit || !unit->IsBot())
            return FSB_Roles::FSB_ROLE_NONE;

        
            if (FSB_BaseAI* ai = dynamic_cast<FSB_BaseAI*>(unit->AI()))
                return ai->roleState;
        

        return FSB_Roles::FSB_ROLE_NONE;
    }

    void SetRole(Creature* unit, FSB_Roles role)
    {
        if (!unit || !unit->IsBot())
            return;


            if (FSB_BaseAI* ai = dynamic_cast<FSB_BaseAI*>(unit->AI()))
                ai->roleState = role;
        
    }

    bool TryChargeHire(Player* player, uint32 duration)
    {
        uint32 pLevel = player->GetLevel();

        int64 price = int64(FollowshipBotsConfig::configFSBPricePerLevel) * pLevel * duration;

        if (!player->HasEnoughMoney(price))
        {
            player->GetSession()->SendNotification(FSB_PLAYER_NOTIFICATION_PAYMENT_FAIL);
            return false;
        }

        player->ModifyMoney(-price);
        player->GetSession()->SendNotification(FSB_PLAYER_NOTIFICATION_PAYMENT_SUCCESS);
        return true;
    }   
}

namespace FSBUtilsMovement
{
    bool FSBUtilsMovement::EnsureInRange(Creature* me, Unit* target, float requiredRange)
    {
        if (!me || !target || !target->IsAlive())
            return false;

        float dist = me->GetDistance(target);

        // Already in range
        if (dist <= requiredRange)
            return false;

        MotionMaster* mm = me->GetMotionMaster();
        if (!mm)
            return false;

        // Avoid restarting the same chase every tick
        if (mm->GetCurrentMovementGeneratorType() == CHASE_MOTION_TYPE)
            return true;

        mm->Clear();
        mm->MoveChase(target, requiredRange);

        TC_LOG_DEBUG("scripts.ai.fsb",
            "FSB EnsureInRange: chasing {} to {:.1f} yards",
            target->GetName(), requiredRange);

        return true;
    }

    bool FSBUtilsMovement::EnsureLOS(Unit* me, Unit* target)
    {
        if (!me || !target)
            return false;

        if (me->IsWithinLOSInMap(target))
            return false; // LOS OK ? no movement needed

        MotionMaster* mm = me->GetMotionMaster();
        if (!mm)
            return false;

        // Move directly toward target to resolve LOS
        mm->MoveChase(target);

        return true; // movement started
    }

    void StopFollow(Unit* me)
    {
        if (!me)
            return;

        me->StopMoving();
        me->GetMotionMaster()->Clear();
    }

    MovementGeneratorType GetMovementType(Unit* me)
    {
        if (!me)
            return IDLE_MOTION_TYPE;

        MotionMaster* mm = me->GetMotionMaster();
        if (!mm)
            return IDLE_MOTION_TYPE;

        return mm->GetCurrentMovementGeneratorType();
    }
}


namespace FSBUtilsTexts
{
    // Converts an int64 price in copper to a string like "10 silver"
    std::string MoneyToString(int64 price)
    {
        int32 gold = price / 10000;
        int32 silver = (price % 10000) / 100;
        int32 copper = price % 100;
        std::string result;
        if (gold > 0)   result += std::to_string(gold) + " gold ";
        if (silver > 0) result += std::to_string(silver) + " silver ";
        if (copper > 0) result += std::to_string(copper) + " copper";
        return result.empty() ? "0 copper" : result;
    }

    // Builds the hire option text dynamically
    std::string BuildHireText(int64 price, uint32 hours)
    {
        return std::to_string(hours) + " hour service: " + MoneyToString(price);
    }

    void ReplaceAll(std::string& text, const std::string& from, const std::string& to)
    {
        size_t pos = 0;
        while ((pos = text.find(from, pos)) != std::string::npos)
        {
            text.replace(pos, from.length(), to);
            pos += to.length();
        }
    }

    std::string BuildNPCSayText(const std::string& playerName, uint32 duration, FSBSayType type, const std::string& string2)
    {
        switch (type)
        {
        case FSBSayType::Hire:
        {
            static const std::vector<std::string> texts =
            {
                "Hey <name>, I am all yours for the next <duration> hours.",
                "Alright <name>, let's band together for <duration> hours.",
                "Ok, I'll come along, <name>, since you paid me for <duration> hours.",
                "Only <duration> hours? Sure thing, <name>.",
                "Well <name>, my calendar is empty for <duration> hours. Let's make poor life choices.",
                "I suppose <duration> hours with you won't kill me, <name>. Probably.",
                "You pay, I follow, <name>. <duration> hours it is.",
                "For <duration> hours, <name>, I am your loyal companion. After that, we renegotiate.",
                "Ah yes, <duration> hours of danger, glory, and questionable decisions, <name>.",
                "Very well <name>, I shall tolerate you for <duration> hours."
            };

            std::string chosen = Trinity::Containers::SelectRandomContainerElement(texts);
            ReplaceAll(chosen, "<name>", playerName);
            ReplaceAll(chosen, "<duration>", std::to_string(duration));
            return chosen;
        }

        case FSBSayType::Fire:
        {
            static const std::vector<std::string> texts =
            {
                "Time's up. Coin's spent. I'm off - good luck out there.",
                "That's it, <name>. Contract fulfilled. Try not to die without me.",
                "Well, that was a few hours of my life I'll never get back. Farewell.",
                "My watch is over. If you need me again, bring more coin.",
                "Alright, <name>, that's my cue. Duty done, boots moving."
            };

            std::string chosen = Trinity::Containers::SelectRandomContainerElement(texts);
            ReplaceAll(chosen, "<name>", playerName);
            return chosen;
        }

        case FSBSayType::Stay:
        {
            static const std::vector<std::string> texts =
            {
                "Alright, I'll hold this spot. Don't take too long.",
                "Staying put. If something bites me, I'm blaming you.",
                "Very well, <name>. I'll stand guard right here.",
                "Holding position. Someone's got to be the responsible one."
            };

            std::string chosen = Trinity::Containers::SelectRandomContainerElement(texts);
            ReplaceAll(chosen, "<name>", playerName);
            return chosen;
        }

        case FSBSayType::Follow:
        {
            static const std::vector<std::string> texts =
            {
                "Alright, I'm back with you.",
                "Following you again. Let's move.",
                "There you are. Lead the way, <name>.",
                "Right behind you. More or less."
            };

            std::string chosen = Trinity::Containers::SelectRandomContainerElement(texts);
            ReplaceAll(chosen, "<name>", playerName);
            return chosen;
        }

        case FSBSayType::HealSelf:
        {
            static const std::vector<std::string> texts =
            {
                "Still standing.",
                "That one actually hurt.",
                "Not today.",
                "I've had worse.",
                "Staying alive."
            };

            return Trinity::Containers::SelectRandomContainerElement(texts);
        }

        case FSBSayType::SpellOnTarget:
        {
            static const std::vector<std::string> texts =
            {
                "Hey, <name>, do you like my <spell>?",
                "Feel the power of my <spell>!",
                "You're gonna feel this one, <name>!"
            };

            std::string chosen = Trinity::Containers::SelectRandomContainerElement(texts);
            ReplaceAll(chosen, "<name>", playerName);
            ReplaceAll(chosen, "<spell>", string2);
            return chosen;
        }

        case FSBSayType::BotDeath:
        {
            static const std::vector<std::string> texts =
            {
                "Well. this seems suboptimal. <name>, I blame you.",
                "I was promised healing. This feels like betrayal, <name>.",
                "Tell my spellbook I loved it.",
                "Remember me as I was. Alive. And judging your decisions, <name>."
            };

            std::string chosen = Trinity::Containers::SelectRandomContainerElement(texts);
            ReplaceAll(chosen, "<name>", playerName);
            return chosen;
        }

        case FSBSayType::TargetDeath:
        {
            static const std::vector<std::string> texts =
            {
                "Another one down. You're welcome, <name>.",
                "Rest in pieces, <target>.",
                "<target>? Yeah. that didn't end well."
            };

            std::string chosen = Trinity::Containers::SelectRandomContainerElement(texts);
            ReplaceAll(chosen, "<name>", playerName);
            ReplaceAll(chosen, "<target>", string2);
            return chosen;
        }

        default:
            return "Hello " + playerName + ".";
        }
    }

    void OnKilledTargetSay(Creature* creature, Unit* victim)
    {
        Unit* owner = creature->GetOwner();
        Player* player = owner ? owner->ToPlayer() : nullptr;

        std::string pName = player ? player->GetName() : "";

        std::string msg = FSBUtilsTexts::BuildNPCSayText(pName, NULL, FSBSayType::TargetDeath, victim->GetName());
        creature->Say(msg, LANG_UNIVERSAL);
    }
}


