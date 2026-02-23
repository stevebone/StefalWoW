#include "Followship_bots_mgr.h"

#include "Followship_bots_chatter_handler.h"
#include "Followship_bots_events_handler.h"

std::vector<FSBChatterReplyEntry> FSBReplyTable =
{
    {
        "emote_kiss", FSB_ChatterType::Neutral,
        {
            "Oh. thank you, I guess?",
            "What was that for?",
            "You're full of surprises today.",
            "Well. that was unexpected.",
            "A kiss? Really now?",
            "Uh. okay then.",
            "Was that meant for me?",
            "Not sure what that was about.",
            "Huh. interesting choice.",
            "Well, that happened.",
            "Is this a new thing you're doing?",
            "I'll pretend that made sense.",
            "Unexpected, but alright.",
            "You do you, I guess.",
            "Not sure how to respond to that.",
            "Was that intentional?",
            "I'm. just going to ignore that.",
            "If that was a signal, I missed it.",
            "I suppose that's one way to greet someone.",
            "You're full of surprises, aren't you?",
            "I'll take that as. something.",
            "Is this normal for you?",
            "I'm not sure what you expect from me now.",
            "Well, that's one way to pass the time.",
            "I'll just act like that was totally normal."
        }
    },

    {
        "emote_kiss", FSB_ChatterType::Positive,
        {
            "Oh. thank you, That is so sweet...",
            "For me? Ah thank you!",
            "You're full of surprises today.",
            "Well. that makes my day.",
            "A kiss? Ah sure, why not...",
            "Aww, that's adorable!",
            "Well aren't you sweet!",
            "Oh! That made me smile.",
            "You're too kind, really.",
            "That brightened my day, thank you.",
            "You always know how to make things nicer.",
            "Oh! I wasn't expecting that, but I liked it.",
            "You're such a charmer, {target}.",
            "That was lovely. thank you.",
            "You're making me blush over here.",
            "How cute! Do it again.",
            "You're surprisingly affectionate today.",
            "That was sweet - keep it up.",
            "You know how to make someone feel appreciated.",
            "That was actually really nice.",
            "You're full of warmth today.",
            "A kiss? I won't say no to that.",
            "You're making this adventure much more pleasant.",
            "That was unexpectedly lovely.",
            "You're such a softie sometimes."
        }
    },

    {
        "emote_kiss", FSB_ChatterType::Negative,
        {
            "Oh. no thanks...",
            "Can you please, just... not do that?",
            "You're at it again?",
            "Not this shit again...",
            "Just stop it!",
            "Ugh. really?",
            "Do you mind? Actually, yes, you do.",
            "Why are you like this?",
            "Stop that. Seriously.",
            "If you do that again, I'm walking.",
            "I did NOT ask for that.",
            "Gross. Just. gross.",
            "Are you trying to annoy me?",
            "That was unnecessary.",
            "Please keep your lips to yourself.",
            "Nope. Absolutely not.",
            "You think that was cute? It wasn't.",
            "I swear, you're doing this on purpose.",
            "Can you not?",
            "That better not become a habit.",
            "I'm not in the mood for your nonsense.",
            "Try that again and see what happens.",
            "You're testing my patience.",
            "That was the opposite of charming.",
            "I regret standing this close to you."
        }
    },

    {
        "emote_kiss", FSB_ChatterType::None,
        {
            "{bot} blows a kiss to {target}.",
            "{bot} sends a warm kiss toward {target}.",
            "{bot} winks and blows a kiss at {target}.",
        }
    },

    {
    "emote_whistle", FSB_ChatterType::None,
        {
            "{bot} whistles idly.",
            "{bot} lets out a long, bored whistle.",
            "{bot} whistles a wandering little tune.",
            "{bot} glances around and whistles softly.",
            "{bot} taps their foot and whistles impatiently.",
            "{bot} hums a tune before breaking into a whistle.",
            "{bot} whistles to pass the time.",
            "{bot} looks around, clearly bored, and whistles.",
            "{bot} gives a casual whistle into the air.",
            "{bot} whistles while waiting for something to happen."
        }
    },

    {
        "whisper_afk", FSB_ChatterType::Neutral,
        {
            "Are we ready to move on?",
            "{target}, ready to go when you are...",
            "Are we waiting for something or someone, {target}?",
            "Just checking in. everything alright?",
            "Let me know when you're back.",
            "We can continue whenever you're ready.",
            "Still here. Still waiting.",
            "No rush, {target}. Take your time.",
            "Standing by for your next move.",
            "Let me know if you need a moment.",
            "I'm here whenever you return.",
            "Just keeping an eye on things.",
            "All quiet on my end.",
            "Still holding position.",
            "Everything okay over there?",
            "We can resume whenever you're ready.",
            "I'll stay close until you're back.",
            "Still waiting patiently.",
            "Let me know when you're ready to continue.",
            "I'm not going anywhere."
        }
    },

    {
        "whisper_afk", FSB_ChatterType::Positive,
        {
            "Take your time, {target}. I've got your back.",
            "No worries at all - I'll wait right here.",
            "Hope everything's alright on your end.",
            "Just checking in, {target}. Need anything?",
            "I'll keep watch while you're away.",
            "Don't stress, I'm not in a hurry.",
            "You deserve a moment to breathe.",
            "I'll be right here when you return.",
            "Everything's under control, {target}.",
            "If you need a break, take it.",
            "I'll keep things safe until you're back.",
            "You're doing great - take your time.",
            "Let me know if I can help with anything.",
            "I'll stay close and keep an eye out.",
            "Hope you're okay, {target}.",
            "Rest if you need to - I'll handle things.",
            "I'm here for you, no rush.",
            "Take all the time you need.",
            "I'll be ready the moment you are.",
            "You can count on me, {target}."
        }
    },

    {
        "whisper_afk", FSB_ChatterType::Negative,
        {
            "Seriously, {target}? How long is this going to take?",
            "Did you fall asleep over there?",
            "If we wait any longer, I might grow roots.",
            "Any chance you're coming back sometime today?",
            "Hello? Earth to {target}...",
            "I swear, if you're napping again.",
            "We're not getting any younger here.",
            "Should I send a search party for you?",
            "If you're gone, at least leave a note next time.",
            "I'm starting to think you abandoned me.",
            "You know we could be doing things, right?",
            "At this rate, I'll finish my entire life story.",
            "Still away? Shocking.",
            "I could've run to Stormwind and back by now.",
            "If you're waiting for me to move, think again.",
            "Come on, {target}, wake up already.",
            "I'm giving you five more minutes before I riot.",
            "You better have a good excuse for this.",
            "If you're not back soon, I'm charging you overtime.",
            "This is getting ridiculous, {target}."
        }
    },


    {
        "campfire", FSB_ChatterType::None,
        {
            "Let me warm up a bit...",
            "Starting a fire, hold on.",
            "A little warmth never hurt anyone."
        }
    }
};

namespace FSBChatter
{
    std::string GetRandomReply(
        Creature* bot,
        Unit* target,
        const std::string& category,
        FSB_ChatterType chatterType)
    {
        for (auto const& entry : FSBReplyTable)
        {
            if (entry.category == category && entry.chatterType == chatterType)
            {
                if (entry.lines.empty())
                    return "";

                std::string line = entry.lines[urand(0, (entry.lines.size() - 1))];

                // Token replacement
                if (bot)
                    ReplaceAll(line, "{bot}", bot->GetName());
                
                if (target)
                    ReplaceAll(line, "{target}", target->GetName());

                return line;
            }
        }

        return "";
    }

    void DemandTimedReply(Creature* bot, Unit* target, const std::string& category, FSB_ReplyType replyType)
    {
        if (!bot)
            return;

        if (!bot->IsAlive() || !target->IsAlive())
            return;

        FSB_ChatterType type = FSBMgr::Get()->GetBotChatterTypeForEntry(bot->GetEntry());

        std::string replyString = GetRandomReply(bot, target, category, type);

        FSBEvents::ScheduleBotEventWithChatter(bot, FSB_EVENT_HIRED_TIMED_CHATTER_REPLY, 3s, 5s, replyType, replyString, target);
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
}


