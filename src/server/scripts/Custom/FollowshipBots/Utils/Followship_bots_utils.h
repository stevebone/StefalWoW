#pragma once

#include <cmath>
#include <string>
#include <vector>
#include <random>
#include <cstdint>

#include "Creature.h"
#include "MotionMaster.h"
#include "Player.h"
#include "Unit.h"

//#include "followship_bots_utils_spells.h"
#include "followship_bots_ai_base.h"

// Converts an int64 price in copper to a string like "10 silver"
inline std::string MoneyToString(int64 price)
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
inline std::string BuildHireText(int64 price, uint32 hours)
{
    return std::to_string(hours) + " hour service: " + MoneyToString(price);
}

enum class FSBSayType
{
    Hire,           // NPC hired by player
    PHire,          // NPC permanent hired by player
    Fire,           // NPC Dismissed or Duration Expired
    Stay,           // NPC asked to stay
    Follow,         // NPC asked to follow or after hire
    Buffed,         // NPC received a positive spell buff
    HealTarget,     // NPC Heals target (not self)
    HealSelf,       // NPC Heals self
    Resurrect,      // NPC Resurrects the player
    PlayerDead,     // NPC reacts to dead player
    SpellOnTarget,  // NPC reacts when casting combat spell on target
    CombatMana,     // NPC IC OOM and uses mana potion
    BotDeath,       // NPC Dies
    TargetDeath     // NPC Kills Target
};

// Helper to pick a random element from a vector
inline const std::string& RandomChoice(const std::vector<std::string>& options)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<> dis(0, options.size() - 1);
    return options[dis(gen)];
}

// Builds NPC say text dynamically, inserts player name and duration
inline std::string BuildNPCSayText(const std::string& playerName, uint32 duration, FSBSayType type, const std::string& string2)
{
    switch (type)
    {
    case FSBSayType::Hire:
    {
        std::vector<std::string> hireTexts =
        {
            "Hey <name>, I am all yours for the next <duration> hours.",
            "Alright <name>, let's band together for <duration> hours.",
            "Ok, I'll come along, <name>, since you paid me for <duration> hours.",
            "Only <duration> hours? Sure thing, <name>."

            // Funny / flavor variants
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

        std::string chosen = RandomChoice(hireTexts);
        // Replace placeholders
        size_t pos;
        while ((pos = chosen.find("<name>")) != std::string::npos)
            chosen.replace(pos, 6, playerName);
        while ((pos = chosen.find("<duration>")) != std::string::npos)
            chosen.replace(pos, 10, std::to_string(duration));
        return chosen;
    }

    case FSBSayType::Fire:
    {
        std::vector<std::string> fireTexts =
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

        std::string chosen = RandomChoice(fireTexts);
        size_t pos;
        while ((pos = chosen.find("<name>")) != std::string::npos)
            chosen.replace(pos, 6, playerName);
        //while ((pos = chosen.find("<duration>")) != std::string::npos)
        //    chosen.replace(pos, 10, std::to_string(duration));
        return chosen;
    }

    case FSBSayType::Stay:
    {
        static const std::vector<std::string> stayTexts =
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

        std::string chosen = RandomChoice(stayTexts);
        size_t pos;
        while ((pos = chosen.find("<name>")) != std::string::npos)
            chosen.replace(pos, 6, playerName);
        //while ((pos = chosen.find("<duration>")) != std::string::npos)
        //    chosen.replace(pos, 10, std::to_string(duration));
        return chosen;
    }

    case FSBSayType::Follow:
    {
        static const std::vector<std::string> followTexts =
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

        std::string chosen = RandomChoice(followTexts);
        size_t pos;
        while ((pos = chosen.find("<name>")) != std::string::npos)
            chosen.replace(pos, 6, playerName);
        //while ((pos = chosen.find("<duration>")) != std::string::npos)
        //    chosen.replace(pos, 10, std::to_string(duration));
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

        std::string chosen = RandomChoice(phireTexts);
        size_t pos;
        while ((pos = chosen.find("<name>")) != std::string::npos)
            chosen.replace(pos, 6, playerName);
        //while ((pos = chosen.find("<duration>")) != std::string::npos)
        //    chosen.replace(pos, 10, std::to_string(duration));
        return chosen;
    }

    case FSBSayType::Buffed:
    {
        static const std::vector<std::string> buffedTexts =
        {
            "Thanks, <name>, I feel stronger already!",
            "Ah, just what I needed, <name>!",
            "Power surging through me!",
            "I appreciate the buff, <name>!"
        };

        std::string chosen = RandomChoice(buffedTexts);
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

        std::string chosen = RandomChoice(healTargetTexts);
        size_t pos;
        while ((pos = chosen.find("<name>")) != std::string::npos)
            chosen.replace(pos, 6, playerName);
        //while ((pos = chosen.find("<duration>")) != std::string::npos)
        //    chosen.replace(pos, 10, std::to_string(duration));
        return chosen;
    }

    case FSBSayType::HealSelf:
    {
        static const std::vector<std::string> healSelfTexts =
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

        std::string chosen = RandomChoice(healSelfTexts);
        size_t pos;
        while ((pos = chosen.find("<name>")) != std::string::npos)
            chosen.replace(pos, 6, playerName);
        //while ((pos = chosen.find("<duration>")) != std::string::npos)
        //    chosen.replace(pos, 10, std::to_string(duration));
        return chosen;
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

        std::string chosen = RandomChoice(resurrectTexts);
        size_t pos;
        while ((pos = chosen.find("<name>")) != std::string::npos)
            chosen.replace(pos, 6, playerName);
        //while ((pos = chosen.find("<duration>")) != std::string::npos)
        //    chosen.replace(pos, 10, std::to_string(duration));
        return chosen;
    }

    case FSBSayType::PlayerDead:
    {
        static const std::vector<std::string> playerDeadTexts =
        {
            "Hey <name>, what are you doing?",
            "Ah, shiiiiiit, nooooo <name>!",
            "How did I let this happen?",
            "Hope you won't blame me for this, <name>!",
            "Why? Why now?",
            "We were so close... yet so far away",
            "Death is just a number, <name>!"
        };

        std::string chosen = RandomChoice(playerDeadTexts);
        size_t pos;
        while ((pos = chosen.find("<name>")) != std::string::npos)
            chosen.replace(pos, 6, playerName);
        //while ((pos = chosen.find("<duration>")) != std::string::npos)
        //    chosen.replace(pos, 10, std::to_string(duration));
        return chosen;
    }

    case FSBSayType::SpellOnTarget:
    {
        static const std::vector<std::string> combatTargetTexts =
        {
            "Hey, <name>, do you like my <spell>",
            "Take that, you <name>!",
            "Feel the power of my <spell>!",
            "You're gonna feel this one, <name>!",
            "One <spell> for you and another for you <name>.",
            "I've got more <spell> from where this came from!",
            "Hope you like my <spell>, you <name>!"
        };

        std::string chosen = RandomChoice(combatTargetTexts);
        size_t pos;
        while ((pos = chosen.find("<name>")) != std::string::npos)
            chosen.replace(pos, 6, playerName);
        while ((pos = chosen.find("<spell>")) != std::string::npos)
            chosen.replace(pos, 7, string2);
        return chosen;
    }

    case FSBSayType::CombatMana:
    {
        static const std::vector<std::string> combatManaTexts =
        {
            "Ugh. my brain's running on empty!",
            "Time for a little <spell> snack!",
            "<name>, If I run out now, it's over. gulp!",
            "Someone toss me a <spell> potion, stat!",
            "I can almost feel my powers fading.",
            "Nothing a sip of magic can't fix!",
            "Mana low. desperation high!",
            "This <spell> better work. or I'm toast!",
            "<name> have a <spell> for me? No...?! Ok, I'll use mine..."
        };

        std::string chosen = RandomChoice(combatManaTexts);
        size_t pos;
        while ((pos = chosen.find("<name>")) != std::string::npos)
            chosen.replace(pos, 6, playerName);
        while ((pos = chosen.find("<spell>")) != std::string::npos)
            chosen.replace(pos, 7, string2);
        return chosen;
    }

    case FSBSayType::BotDeath:
    {
        static const std::vector<std::string> botDeathTexts =
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

        std::string chosen = RandomChoice(botDeathTexts);
        size_t pos;
        while ((pos = chosen.find("<name>")) != std::string::npos)
            chosen.replace(pos, 6, playerName);
        while ((pos = chosen.find("<spell>")) != std::string::npos)
            chosen.replace(pos, 7, string2);
        return chosen;
    }

    case FSBSayType::TargetDeath:
    {
        static const std::vector<std::string> targetDeathTexts =
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

        std::string chosen = RandomChoice(targetDeathTexts);
        size_t pos;
        while ((pos = chosen.find("<name>")) != std::string::npos)
            chosen.replace(pos, 6, playerName);
        while ((pos = chosen.find("<target>")) != std::string::npos)
            chosen.replace(pos, 8, string2);
        return chosen;
    }

    default:
        return "Hello " + playerName + ", duration: " + std::to_string(duration) + " hour(s).";
    }
}

constexpr float SIDE_OFFSET_MAX = M_PI / 6.0f; // ±30 degrees from pure left/right

// Returns a random angle slightly to the left of the player
inline float GetRandomLeftAngle()
{
    // Base left is +90 degrees (M_PI/2)
    // Add a random small offset for natural variation
    return (M_PI / 2.0f) + frand(-SIDE_OFFSET_MAX, SIDE_OFFSET_MAX);
}

// Returns a random angle slightly to the right of the player
inline float GetRandomRightAngle()
{
    // Base right is -90 degrees (-M_PI/2)
    // Add a random small offset for natural variation
    return (-M_PI / 2.0f) + frand(-SIDE_OFFSET_MAX, SIDE_OFFSET_MAX);
}

namespace FSBUtils
{
    // Get the role of a bot (returns FSB_ROLE_NONE if not a bot or AI not present)
    FSB_Roles GetRole(Creature* unit);

    // Set the role of a bot (does nothing if not a bot or AI not present)
    void SetRole(Creature* unit, FSB_Roles role);

}

namespace FSBUtilsMovement
{
    // Returns true if chase was started or is ongoing
    bool EnsureInRange(Creature* me, Unit* target, float requiredRange);
    bool EnsureLOS(Unit* me, Unit* target);

    void StopFollow(Unit* me);

    MovementGeneratorType GetMovementType(Unit* me);
}

