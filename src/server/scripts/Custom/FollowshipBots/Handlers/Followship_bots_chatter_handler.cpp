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
        "emote_flirt", FSB_ChatterType::None,
        {
            "{bot} gives {target} a playful wink.",
            "{bot} flashes {target} a charming smile.",
            "{bot} tilts their head and gives {target} a flirty grin.",
            "{bot} shoots {target} a mischievous look.",
            "{bot} gives {target} a slow, teasing wink.",
            "{bot} smirks softly in {target}'s direction.",
            "{bot} gives {target} a warm, inviting smile.",
            "{bot} glances at {target} with a hint of playful interest.",
            "{bot} offers {target} a coy, knowing smile.",
            "{bot} gives {target} a subtle but unmistakably flirty look."
        }
    },

    {
        "emote_flirt", FSB_ChatterType::Neutral,
        {
            "Oh. was that supposed to be flirting?",
            "Interesting approach, {bot}. Not sure what to make of it.",
            "Huh. that was a thing you just did, {bot}.",
            "Is that how you usually flirt?",
            "Well, that happened.",
            "I'll. pretend I understood that.",
            "Not sure what you're going for, {bot} but okay.",
            "You're full of surprises today.",
            "I'm not sure how to respond to that.",
            "Was that meant for me?",
            "I'll just take that as a compliment. I think.",
            "You're being. oddly expressive.",
            "Is this a new habit of yours?",
            "I guess that was a flirt? Maybe?",
            "I'll let that one slide without comment.",
            "You're doing something {bot}. Not sure what, but something.",
            "I'll just nod and move on.",
            "That was. subtle? Maybe?",
            "I'm not sure if that was flirting or a glitch.",
            "Well, that's one way to get attention."
        }
    },

    {
        "emote_flirt", FSB_ChatterType::Positive,
        {
            "Oh? Someone's feeling bold today.",
            "Well now. aren't you charming.",
            "You're going to make me blush at this rate.",
            "Careful, {bot}. I might flirt back.",
            "Oh! That was actually kind of cute.",
            "You're full of sweet surprises.",
            "If you keep that up, I might get the wrong idea.",
            "You're quite the charmer, aren't you {bot}?",
            "Oh wow. that was smooth.",
            "You're making this adventure a lot more fun.",
            "Flirting now? I don't mind.",
            "You're adorable when you try like that.",
            "Oh! I wasn't expecting that, but I liked it.",
            "You're dangerously good at this {bot}.",
            "If that was flirting. it worked.",
            "You're going to spoil me with attention.",
            "You're surprisingly sweet today.",
            "Oh, keep going. I'm enjoying this.",
            "You're making me smile, {bot}.",
            "You're a natural at this, you know."
        }
    },

    {
    "emote_joke", FSB_ChatterType::None,
        {
            "{bot} tells a joke.",
            "{bot} tells a funny quote.",
            "{bot} whispers a small joke.",
            "{bot} makes a funny face and jokes.",
            "{bot} tells a joke to themselves.",
            "{bot} tells a funny story.",
            "{bot} recollects a funny event."
        }
    },

    {
        "emote_joke", FSB_ChatterType::Neutral,
        {
            "Huh. I think I get it.",
            "Was that the whole joke {bot}?",
            "Interesting sense of humor you've got there.",
            "I'll pretend that was funny {bot}.",
            "Not bad. {bot} not great. just there.",
            "I'm not sure how to react to that.",
            "That was. something.",
            "I think I've heard that one before.",
            "I'll give you points for trying.",
            "I guess that counts as a joke.",
            "I'm processing. still processing.",
            "That one flew right past me.",
            "I'm not sure if I should laugh or sigh.",
            "I'll allow it.",
            "That joke existed. That's all I can say.",
            "I'll give you a polite nod for that one.",
            "I'm neutral on this. Very neutral.",
            "I'll file that under 'attempted humor'.",
            "I didn't hate it. Didn't love it either.",
            "Well. {bot} that was a joke, technically."
        }
    },

    {
        "emote_joke", FSB_ChatterType::Positive,
        {
            "Haha! Okay, that one was actually good {bot}.",
            "You got me {bot} - that was funny!",
            "Oh wow, I wasn't expecting that!",
            "You're funnier than you look, {target}.",
            "That one made me smile.",
            "Keep them coming, I like your humor.",
            "Haha! Nice one!",
            "You're on a roll today!",
            "I needed that laugh {bot}, thanks.",
            "You've got a real talent for this.",
            "That joke was surprisingly good!",
            "I'll be laughing about that one later.",
            "You always know how to lighten the mood.",
            "That was adorable and funny.",
            "You're actually hilarious sometimes.",
            "I love your sense of humor.",
            "That one hit just right.",
            "You're making this adventure way more fun.",
            "Okay {bot}, that was genuinely great.",
            "You should tell more jokes like that."
        }
    },

    {
        "emote_joke", FSB_ChatterType::Negative,
        {
            "That joke hurt me on a spiritual level.",
            "Please never say that again {bot}.",
            "I think my soul just died a little.",
            "Was that supposed to be funny?",
            "I've heard murlocs make better jokes.",
            "{bot} That joke was a crime.",
            "I'm reporting you to the humor authorities.",
            "I'd laugh, but I respect myself too much.",
            "That was so bad it looped back to worse.",
            "You should be banned from telling jokes.",
            "I want the last ten seconds of my life back.",
            "If that was humor, I fear for your future.",
            "I've seen kobolds with better material.",
            "That joke physically pained me.",
            "Congratulations, you made silence awkward.",
            "I'm embarrassed for both of us {bot}.",
            "That joke belongs in a dungeon. A deep one.",
            "Please stop {bot}. For both our sakes.",
            "I'd laugh, but I don't want to encourage you.",
            "That was the worst thing I've heard all day."
        }
    },

    {
        "emote_flirt", FSB_ChatterType::Negative,
        {
            "Oh no. don't start with that.",
            "Please don't flirt with me. Seriously {bot}.",
            "Was that flirting? Ew.",
            "Nope. Absolutely not.",
            "Save the flirting for someone who wants it.",
            "You're trying way too hard.",
            "Stop. Just stop {bot}.",
            "That was painful to witness.",
            "If that was flirting, you need help.",
            "I'm not interested, {bot}.",
            "Please don't make this weird.",
            "You're making things awkward again.",
            "I'd rather fight a dragon than deal with this.",
            "You call that flirting?",
            "Try that again and I'm walking away.",
            "You're not charming. You're just annoying.",
            "I'm immune to whatever that was.",
            "No thanks. Really.",
            "You're embarrassing yourself.",
            "Let's pretend that didn't happen."
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
        "emote_talk", FSB_ChatterType::Neutral,
        {
            "So. how long are we gonna be here?",
            "I'd really rather be somewhere else right now.",
            "You ever wonder why we do all this?",
            "Feels like we've been standing here forever.",
            "I remember my family. haven't seen them in ages.",
            "Is it just me, or is it way too quiet?",
            "I could really go for a drink right now.",
            "Any day now.",
            "I once got lost for three days in Elwynn Forest.",
            "I swear my boots are getting smaller.",
            "I miss my bed. and my pillow. and not being here.",
            "So. what now?",
            "I had a weird dream last night. Don't ask.",
            "I could be napping right now.",
            "I once knew someone who talked even more than I do.",
            "Anyone else feeling watched? No? Just me?",
            "I hope this doesn't take all day. again.",
            "I should've stayed home today.",
            "You ever think about just. walking away?",
            "I'm not saying I'm bored, but. I'm bored.",
            "I miss real food. Not travel rations.",
            "I once had a pet. It ran away. Smart pet.",
            "If this goes on much longer, I'm charging overtime.",
            "I wonder what my friends are doing right now. probably something fun.",
            "I should've brought a book.",
            "You ever get the feeling you forgot something important?",
            "I used to be an adventurer. then I met you lot.",
            "If I stand still any longer, I'll turn into a statue.",
            "I heard a joke once. Forgot the punchline. Still funny though.",
            "Alright. any moment now. right?"
        }
    },

    {
        "emote_talk", FSB_ChatterType::Negative,
        {
            "Some days. it's hard to keep going.",
            "I don't even know why I'm still doing this.",
            "Feels like everything's getting heavier lately.",
            "I miss the days when things felt simple.",
            "Sometimes I wonder if anyone would notice if I disappeared.",
            "I'm tired. not physically, just tired.",
            "Feels like I'm walking through fog all the time.",
            "I wish things had turned out differently.",
            "I keep thinking about all the mistakes I've made.",
            "It's strange how lonely you can feel even in a group.",
            "I don't remember the last time I felt truly happy.",
            "Some memories hurt more than any wound.",
            "I pretend I'm fine, but I'm really not.",
            "I miss people I'll never see again.",
            "I feel like I'm fading a little more every day.",
            "Sometimes I wonder what the point of all this is.",
            "I wish I could go back. just once.",
            "Everything feels so distant lately.",
            "I don't know if I'm strong enough for this.",
            "I keep hoping things will get better. but they don't.",
            "I feel like I'm stuck in place while the world moves on.",
            "I miss the person I used to be.",
            "Some days I just want to stop and never move again.",
            "I'm surrounded by people, yet I feel alone.",
            "I wish someone would ask if I'm okay.",
            "I'm not sure I belong anywhere anymore.",
            "I keep trying, but it never feels like enough.",
            "I don't know how much longer I can pretend.",
            "Everything feels so. empty.",
            "I just want one moment of peace."
        }
    },

    {
        "emote_talk", FSB_ChatterType::Positive,
        {
            "What a great day for an adventure!",
            "I can't wait to see what's around the next corner.",
            "Feels like something exciting is about to happen.",
            "I've got a good feeling about today.",
            "Let's make some memories out here.",
            "I love being out in the world like this.",
            "Every step forward feels like a story waiting to happen.",
            "I'm ready for anything - bring it on!",
            "You know, I think we make a pretty good team.",
            "I hope we find something amazing today.",
            "I feel alive out here!",
            "There's so much to see. let's not waste a moment.",
            "I swear, the world feels brighter today.",
            "I'm excited to see where this journey takes us.",
            "I could do this all day.",
            "Let's keep moving - adventure doesn't wait!",
            "I love the smell of the wilds in the morning.",
            "Something tells me today's going to be special.",
            "I'm ready to take on the world!",
            "Feels good to be out here with you.",
            "I hope we run into something fun soon.",
            "I'm feeling unstoppable right now.",
            "Let's go find some trouble - the good kind.",
            "I can't help but smile today.",
            "I feel like we're on the edge of something big.",
            "This is exactly where I want to be.",
            "I'm in the mood to explore everything.",
            "Let's make today legendary.",
            "I'm ready for the next challenge - whatever it is.",
            "Adventure, danger, treasure. I'm here for all of it!"
        }
    },

    {
    "emote_sleep", FSB_ChatterType::None,
        {
            "{bot} yawns loudly.",
            "{bot} lays down for a nap.",
            "{bot} feels sleepy.",
            "{bot} goes for a power nap.",
            "{bot} uses the ground as a bed.",
            "{bot} lays down to rest.",
            "{bot} takes a quick rest."
        }
    },

    {
        "emote_sigh", FSB_ChatterType::None,
        {
            "{bot} lets out a long, weary sigh.",
            "{bot} sighs softly, looking a bit drained.",
            "{bot} exhales heavily, clearly tired of it all.",
            "{bot} sighs and stares off into the distance.",
            "{bot} releases a deep, frustrated sigh.",
            "{bot} sighs as if carrying the weight of the world.",
            "{bot} takes a slow breath and sighs quietly.",
            "{bot} sighs, sounding resigned.",
            "{bot} gives a tired sigh and shakes their head.",
            "{bot} sighs, clearly not impressed."
        }
    },

    {
        "emote_cooking", FSB_ChatterType::Neutral,
        {
            "Alright. fire's going. Do what you want with it.",
            "Cooking time, I guess.",
            "There. Fire's ready.",
            "Okay, that should do it.",
            "Campfire's up. Don't burn yourselves.",
            "Food or warmth, take your pick.",
            "Well, that's one way to pass the time.",
            "Fire's lit. Do whatever you need.",
            "There we go. simple enough.",
            "Alright, heat's ready.",
            "If anyone needs the fire, it's there.",
            "That should be warm enough.",
            "Okay. cooking setup complete.",
            "Fire's going. Smells. fine, I guess.",
            "There. Now we wait.",
            "Alright, who's actually hungry?",
            "Fire's up. Don't expect miracles.",
            "Well, it's something.",
            "Heat's ready if you need it.",
            "Okay. that's done."
        }
    },

    {
        "emote_cooking", FSB_ChatterType::Positive,
        {
            "Fire's ready! Who's hungry?",
            "Perfect! Let's cook something delicious!",
            "There we go - warm fire, warm hearts!",
            "Alright, everyone gather around!",
            "This is going to smell amazing in a moment.",
            "Nothing like a good fire to lift the mood!",
            "Cooking time! I hope you're ready!",
            "Let's make something tasty together!",
            "Fire's up - let's turn this into a feast!",
            "Ahh, perfect! This is my favorite part of adventuring.",
            "Come on, get closer! It's warm!",
            "I love moments like this.",
            "Who wants first pick when it's done?",
            "This is going to be great, I can feel it.",
            "Let's make this meal legendary!",
            "I hope you're hungry - I know I am!",
            "Nothing beats fresh food by a fire.",
            "This is the good life right here.",
            "Warm fire, good company - perfect.",
            "Let's cook up something amazing!"
        }
    },

    {
        "emote_cooking", FSB_ChatterType::Negative,
        {
            "There. Fire's lit. Try not to ruin it.",
            "Fine. I'll cook. Again.",
            "If this burns, it's not my fault.",
            "Great. now I smell like smoke.",
            "Don't expect gourmet food from this.",
            "Ugh. why am I always the one doing this?",
            "If someone complains, I swear.",
            "This better be worth the effort.",
            "I'm not your personal chef, you know.",
            "Fine. Eat whatever comes out of this.",
            "If anyone asks, I was forced to do this.",
            "I hope you like 'slightly burnt' flavor.",
            "Don't crowd me. I'm already annoyed.",
            "If this catches fire, that's on you.",
            "I'm only doing this once.",
            "Great. now we wait. My favorite.",
            "If someone else wants to take over, be my guest.",
            "I swear, next time someone else cooks.",
            "This is the last time I do this without complaining.",
            "There, food's coming. Happy now?"
        }
    },
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

        if (!bot->IsAlive() || (target && !target->IsAlive()))
            return;

        FSB_ChatterType type = FSBMgr::Get()->GetBotChatterTypeForEntry(bot->GetEntry());

        std::string replyString = GetRandomReply(bot, target, category, type);

        FSBEvents::ScheduleBotEventWithChatter(bot, FSB_EVENT_HIRED_TIMED_CHATTER_REPLY, 3s, 5s, replyType, replyString, target);

        // we use a dummy timed event to make the target bot do a delayed emote
        // at the same time they are also replying to the initial bot
        // example: target laughing at the bot joke
        if (category == "emote_joke" && type == FSB_ChatterType::Positive)
        {
            replyString = "emote:laugh";
            FSBEvents::ScheduleBotEventWithChatter(bot, FSB_EVENT_HIRED_TIMED_DUMMY_EMOTE, 3s, 5s, replyType, replyString, target);
        }

        else if ((category == "emote_flirt" || category == "emote_kiss") && type == FSB_ChatterType::Negative)
        {
            replyString = "emote:rudeno";
            FSBEvents::ScheduleBotEventWithChatter(bot, FSB_EVENT_HIRED_TIMED_DUMMY_EMOTE, 3s, 5s, replyType, replyString, target);
        }
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


