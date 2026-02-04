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
#include "Followship_bots_powers_handler.h"

struct FSBEntryClassMap
{
    uint32 entry;
    FSB_Class botClass;
    FSB_Race botRace;
};

static constexpr FSBEntryClassMap BotEntryClassTable[] =
{
    // PRIESTS
    { 141508,   FSB_Class::Priest,      FSB_Race::Human },  // Stormwind Priest
    { 375,      FSB_Class::Priest,      FSB_Race::Human },     // Priestess Anetta

    // WARRIORS
    { 911, FSB_Class::Warrior,          FSB_Race::Human },  // Llane Beshere
    { 50595, FSB_Class::Warrior,        FSB_Race::Human },  // Stormwind Defender

    // MAGES
    { 198, FSB_Class::Mage,             FSB_Race::Human }, // Khelden Bremen

    // ROGUES
    //{ 90030, FSB_Class::Rogue },
};

namespace FSBUtils
{
    void SetInitialState(Creature* creature, bool& hired, FSB_MovementStates& moveState)
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

        //creature->UpdateLevelDependantStats();
        //creature->SetHealth(creature->GetMaxHealth());
        //creature->SetPowerType(FSBPowers::GetBotPowerType(creature), true);
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

    FSB_Race GetBotRaceForEntry(uint32 entry)
    {
        for (auto const& map : BotEntryClassTable)
        {
            if (map.entry == entry)
                return map.botRace;
        }

        return FSB_Race::None;
    }

    void SetBotRace(Creature* creature, FSB_Race& outRace)
    {
        if (!creature)
            return;

        FSB_Race race = GetBotRaceForEntry(creature->GetEntry());

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Race set: {} for bot with entry {}", race, creature->GetEntry());

        if (race == FSB_Race::None)
        {
            TC_LOG_WARN("scripts.ai.fsb", "FSB: No race mapping found for creature entry {}", creature->GetEntry());
        }

        outRace = race;
    }

    bool GetBotClassAndRaceForEntry(uint32 entry, FSB_Class& outClass, FSB_Race& outRace)
    {
        for (auto const& map : BotEntryClassTable)
        {
            if (map.entry == entry)
            {
                outClass = map.botClass;
                outRace = map.botRace;
                return true;
            }
        }

        outClass = FSB_Class::None;
        outRace = FSB_Race::None;
        return false;
    }

    void SetBotClassAndRace(Creature* creature, FSB_Class& outClass, FSB_Race& outRace)
    {
        if (!creature)
            return;

        bool found = GetBotClassAndRaceForEntry(
            creature->GetEntry(),
            outClass,
            outRace
        );

        if (!found)
        {
            TC_LOG_WARN(
                "scripts.ai.fsb",
                "FSB: No class/race mapping found for creature entry {}",
                creature->GetEntry()
            );
            return;
        }

        TC_LOG_DEBUG(
            "scripts.ai.fsb",
            "FSB: Class set to {} and Race set to {} for bot with entry {}",
            outClass,
            outRace,
            creature->GetEntry()
        );
    }

    const char* BotClassToString(FSB_Class cls)
    {
        switch (cls)
        {
        case FSB_Class::Warrior: return "Warrior";
        case FSB_Class::Priest:  return "Priest";
        case FSB_Class::Mage:    return "Mage";
        case FSB_Class::Rogue:   return "Rogue";
        case FSB_Class::Druid:   return "Druid";
        case FSB_Class::Paladin: return "Paladin";
        case FSB_Class::Hunter:  return "Hunter";
        case FSB_Class::Warlock: return "Warlock";
        default:                 return "Unknown";
        }
    }

    const char* BotRaceToString(FSB_Race race)
    {
        switch (race)
        {
        case FSB_Race::Human:    return "Human";
        case FSB_Race::Dwarf:    return "Dwarf";
        case FSB_Race::Draenei:  return "Draenei";
        case FSB_Race::NightElf: return "NightElf";
        case FSB_Race::Gnome:    return "Gnome";
        case FSB_Race::Worgen:   return "Worgen";
        case FSB_Race::Pandaren: return "Pandaren";
        default:                 return "Unknown";
        }
    }

    const char* PowerTypeToString(Powers power)
    {
        switch (power)
        {
        case POWER_MANA:   return "Mana";
        case POWER_RAGE:   return "Rage";
        case POWER_ENERGY: return "Energy";
        default:           return "Other";
        }
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
                return ai->botRole;
        

        return FSB_Roles::FSB_ROLE_NONE;
    }

    void SetRole(Creature* unit, FSB_Roles role)
    {
        if (!unit || !unit->IsBot())
            return;


            if (FSB_BaseAI* ai = dynamic_cast<FSB_BaseAI*>(unit->AI()))
                ai->botRole = role;
        
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
                "Very well <name>, I shall tolerate you for <duration> hours.",
                "<duration> hours? Fine. But if we die, it's your fault, <name>.",
                "I was bored anyway, <name>. <duration> hours sounds perfect.",
                "Gold talks, <name>. And it says '<duration> hours of adventure.'",
                "Let's do this, <name>. <duration> hours in, we pretend we planned it."
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
                "Alright, <name>, that's my cue. Duty done, boots moving.",
                "The gold's gone and so am I. Fair deal.",
                "Contract's finished. I suggest you find another friend. quickly.",
                "That's enough adventuring for one pay period. I'm out.",
                "No hard feelings, <name>. Business is business.",
                "Well, this is where I leave you. Try not to pull anything too big next time."
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
                "You want me here? Fine. I wasn't going anywhere anyway.",
                "Holding position. Try not to forget about me.",
                "Right here, <name>. I'll keep watch.",
                "I'll stay. But if a dragon shows up, that's on you.",
                "Standing still. which is harder than it sounds.",
                "This spot? Really? Alright. I'll make it work.",
                "As you wish, <name>. I'll be right here when you return.",
                "Standing guard. This better be important.",
                "I'll stay. Just don't expect me to enjoy it.",
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
                "I was starting to enjoy standing still. but alright.",
                "Back on your heels. Try not to run too fast.",
                "Following, <name>. Let's see where this goes.",
                "Right behind you. More or less.",
                "Okay, I'm coming. No need to shout.",
                "Time to move again. I'm with you.",
                "Let's go, <name>. I'm ready.",
                "Ah, adventure calls again. I'm following.",
                "Standing still was nice. Back to walking it is.",
                "Alright, <name>. I'll keep up. probably."
            };

            std::string chosen = Trinity::Containers::SelectRandomContainerElement(texts);
            ReplaceAll(chosen, "<name>", playerName);
            return chosen;
        }

        case FSBSayType::PHire:
        {
            static const std::vector<std::string> phireTexts =
            {
                "Looks like we're in this together now, <name>.",
                "Alright. I'm yours until the end - wherever that may be.",
                "No contracts, no timers. I'll stay with you.",
                "Seems I've chosen a side. Let's make it a good one, <name>.",
                "You've got yourself a companion, not just hired help.",
                "I'm in. Wherever you go, I follow.",
                "Well then. looks like I'm part of your story now.",
                "No turning back now. I've got your back, <name>.",
                "Guess this makes us partners.",
                "From now on, I fight by your side.",
                "Permanent, huh? Bold choice. Let's see how this goes.",
                "I suppose this is what loyalty feels like.",
                "No clock ticking anymore. I'm with you."
            };

            std::string chosen = Trinity::Containers::SelectRandomContainerElement(phireTexts);
            size_t pos;
            while ((pos = chosen.find("<name>")) != std::string::npos)
                chosen.replace(pos, 6, playerName);
            //while ((pos = chosen.find("<duration>")) != std::string::npos)
            //    chosen.replace(pos, 10, std::to_string(duration));
            return chosen;
        }

        case FSBSayType::HealTarget:
        {
            static const std::vector<std::string> healTargetTexts =
            {
                "Hey, <name>, Don't you feel stronger already!",
                "Ah, just what you needed, <name>!",
                "Power surging through you!",
                "Heal coming up your way, <name>!",
                "One heal for you and another heal for you...",
                "This heal is for free, next one...",
                "Hope you appreciate the heal, <name>!"
            };

            std::string chosen = Trinity::Containers::SelectRandomContainerElement(healTargetTexts);
            size_t pos;
            while ((pos = chosen.find("<name>")) != std::string::npos)
                chosen.replace(pos, 6, playerName);
            //while ((pos = chosen.find("<duration>")) != std::string::npos)
            //    chosen.replace(pos, 10, std::to_string(duration));
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
                "Just a scratch.",
                "Focus. Breathe.",
                "Pain is temporary.",
                "Not done yet.",
                "That'll keep me going.",
                "Staying alive."
            };

            return Trinity::Containers::SelectRandomContainerElement(texts);
        }

        case FSBSayType::Resurrect:
        {
            static const std::vector<std::string> resurrectTexts =
            {
                "Hey, <name>, Don't you die on me again, ok?",
                "Ah, you're good as new, <name>!",
                "Aren't you glad you brought me along?",
                "Hope I don't have to revive you again, <name>!",
                "One resurrect for you and another for... oh nevermind!",
                "This resurrect is for free, next one...",
                "Hope you appreciate the revival, <name>!"
            };

            std::string chosen = Trinity::Containers::SelectRandomContainerElement(resurrectTexts);
            size_t pos;
            while ((pos = chosen.find("<name>")) != std::string::npos)
                chosen.replace(pos, 6, playerName);
            //while ((pos = chosen.find("<duration>")) != std::string::npos)
            //    chosen.replace(pos, 10, std::to_string(duration));
            return chosen;
        }

        case FSBSayType::PlayerOrMemberDead:
        {
            static const std::vector<std::string> PlayerOrMemberDeadTexts =
            {
                "Hey <name>, what are you doing?",
                "Ah, shiiiiiit, nooooo <name>!",
                "How did I let this happen?",
                "Hope you won't blame me for this, <name>!",
                "Why? Why now?",
                "We were so close... yet so far away",
                "Death is just a number, <name>!"
            };

            std::string chosen = Trinity::Containers::SelectRandomContainerElement(PlayerOrMemberDeadTexts);
            size_t pos;
            while ((pos = chosen.find("<name>")) != std::string::npos)
                chosen.replace(pos, 6, playerName);
            //while ((pos = chosen.find("<duration>")) != std::string::npos)
            //    chosen.replace(pos, 10, std::to_string(duration));
            return chosen;
        }

        case FSBSayType::SpellOnTarget:
        {
            static const std::vector<std::string> texts =
            {
                "Hey, <name>, do you like my <spell>",
                "Take that, you <name>!",
                "Feel the power of my <spell>!",
                "You're gonna feel this one, <name>!",
                "One <spell> for you and another for you <name>.",
                "I've got more <spell> from where this came from!",
                "Hope you like my <spell>, you <name>!"
            };

            std::string chosen = Trinity::Containers::SelectRandomContainerElement(texts);
            ReplaceAll(chosen, "<name>", playerName);
            ReplaceAll(chosen, "<spell>", string2);
            return chosen;
        }

        case FSBSayType::CombatMana:
        {
            static const std::vector<std::string> combatManaTexts =
            {
                "Ugh. my brain's running on empty!",
                "Time for a little <spell> snack!",
                "If I run out of mana now, it's over. gulp!",
                "Someone toss me a <spell> potion, stat!",
                "I can almost feel my powers fading.",
                "Nothing a sip of magic can't fix!",
                "Mana low. desperation high!",
                "This <spell> better work. or I'm toast!",
                "Friend have a <spell> for me? No...?! Ok, I'll use mine..."
            };

            std::string chosen = Trinity::Containers::SelectRandomContainerElement(combatManaTexts);
            size_t pos;
            //while ((pos = chosen.find("<name>")) != std::string::npos)
            //    chosen.replace(pos, 6, playerName);
            while ((pos = chosen.find("<spell>")) != std::string::npos)
                chosen.replace(pos, 7, string2);
            return chosen;
        }

        case FSBSayType::BotDeath:
        {
            static const std::vector<std::string> texts =
            {
                "Well. this seems suboptimal. <name> , I blame you.",
                "I was promised healing.This feels like betrayal, <name>.",
                "Tell my spellbook. I loved it. <name>, you're still paying for repairs.",
                "Next time, maybe don't pull half the place, <name>.",
                "Ah yes. the sweet embrace of the spirit healer.",
                "<name>. I hope that was worth it.",
                "I'm not saying this is your fault, <name>. but it's definitely your fault."
                "Good news, <name>: I found the floor.With my face."
                "This is fine.Everything is fine.I am absolutely- dead."
                "Remember me as I was. alive. and judging your decisions, <name>."
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
                "<target>? Yeah. that didn't end well for them.",
                "See, <name>? Perfectly under control.",
                "I warned you, <target>. Well. internally.",
                "That went about as expected. For me.",
                "<name>, please tell me you saw that.",
                "And that's why I read my spellbook.",
                "One less <target>. Try to keep up, <name>.",
                "I call that a successful application of violence."
            };

            std::string chosen = Trinity::Containers::SelectRandomContainerElement(texts);
            ReplaceAll(chosen, "<name>", playerName);
            ReplaceAll(chosen, "<target>", string2);
            return chosen;
        }

        case FSBSayType::BuffSelf:
        {
            static const std::vector<std::string> texts =
            {
                "Am gonna be needing this!",
                "This <spell> is sure gonna help.",
                "At least I have my <spell>!",
                "There's no <spell> like mine!",
                "One <spell> for me and another for... me... later.",
                "I've got more <spell> from where this came from!",
                "This buffs me buff... haha...haha!"
            };

            std::string chosen = Trinity::Containers::SelectRandomContainerElement(texts);
            ReplaceAll(chosen, "<name>", playerName);
            ReplaceAll(chosen, "<spell>", string2);
            return chosen;
        }

        case FSBSayType::BuffTarget:
        {
            static const std::vector<std::string> texts =
            {
                "Hey, <name>, no need to thank me for <spell>",
                "I've got something for you, <name>!",
                "My <spell> makes you stronger!",
                "You're gonna feel this buff, <name>!",
                "This <spell> is for free <name>.",
                "Buffing was not stated in my contract but here you go!",
                "One <spell>, coming for you <name>!"
            };

            std::string chosen = Trinity::Containers::SelectRandomContainerElement(texts);
            ReplaceAll(chosen, "<name>", playerName);
            ReplaceAll(chosen, "<spell>", string2);
            return chosen;
        }

        case FSBSayType::OOCRecovery:
        {
            static const std::vector<std::string> texts =
            {
                "Hey, <name>, I really need a break...",
                "Let's sit for a moment yeah, <name>!",
                "My <spell> will make me like new...",
                "You're gonna have to wait for me, <name>!",
                "This <spell> is exactly what I need.",
                "We can't go on an empty stomach, now can we?",
                "I need my <spell>, break now <name>!"
            };

            std::string chosen = Trinity::Containers::SelectRandomContainerElement(texts);
            ReplaceAll(chosen, "<name>", playerName);
            ReplaceAll(chosen, "<spell>", string2);
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

        if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
        {
            std::string msg = FSBUtilsTexts::BuildNPCSayText(pName, NULL, FSBSayType::TargetDeath, victim->GetName());
            creature->Say(msg, LANG_UNIVERSAL);
        }
    }
}


