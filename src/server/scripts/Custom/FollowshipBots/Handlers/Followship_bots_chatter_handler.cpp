#include "Followship_bots_config.h"
#include "Followship_bots_mgr.h"

#include "Followship_bots_chatter_handler.h"
#include "Followship_bots_events_handler.h"

std::vector<FSBChatterReplyEntry> FSBReplyTable =
{
    {
        FSB_ChatterCategory::emote_kiss, FSB_ChatterType::Neutral,
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
        FSB_ChatterCategory::emote_kiss, FSB_ChatterType::Positive,
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
            "You're such a charmer, {bot}.",
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
        FSB_ChatterCategory::emote_kiss, FSB_ChatterType::Negative,
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
        FSB_ChatterCategory::emote_kiss, FSB_ChatterType::None,
        {
            "{bot} blows a kiss to {target}.",
            "{bot} sends a warm kiss toward {target}.",
            "{bot} winks and blows a kiss at {target}.",
        }
    },

    {
        FSB_ChatterCategory::emote_whistle, FSB_ChatterType::None,
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
        FSB_ChatterCategory::emote_flirt, FSB_ChatterType::None,
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
        FSB_ChatterCategory::emote_flirt, FSB_ChatterType::Neutral,
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
        FSB_ChatterCategory::emote_flirt, FSB_ChatterType::Positive,
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
        FSB_ChatterCategory::emote_flirt, FSB_ChatterType::Negative,
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
        FSB_ChatterCategory::emote_joke, FSB_ChatterType::None,
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
        FSB_ChatterCategory::emote_joke, FSB_ChatterType::Neutral,
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
        FSB_ChatterCategory::emote_joke, FSB_ChatterType::Positive,
        {
            "Haha! Okay, that one was actually good {bot}.",
            "You got me {bot} - that was funny!",
            "Oh wow, I wasn't expecting that!",
            "You're funnier than you look, {bot}.",
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
        FSB_ChatterCategory::emote_joke, FSB_ChatterType::Negative,
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
        FSB_ChatterCategory::whisper_afk, FSB_ChatterType::Neutral,
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
        FSB_ChatterCategory::whisper_afk, FSB_ChatterType::Positive,
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
        FSB_ChatterCategory::whisper_afk, FSB_ChatterType::Negative,
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
        FSB_ChatterCategory::emote_talk, FSB_ChatterType::Neutral,
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
        FSB_ChatterCategory::emote_talk, FSB_ChatterType::Negative,
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
        FSB_ChatterCategory::emote_talk, FSB_ChatterType::Positive,
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
    FSB_ChatterCategory::emote_sleep, FSB_ChatterType::None,
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
        FSB_ChatterCategory::emote_sigh, FSB_ChatterType::None,
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
        FSB_ChatterCategory::emote_cooking, FSB_ChatterType::Neutral,
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
        FSB_ChatterCategory::emote_cooking, FSB_ChatterType::Positive,
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
        FSB_ChatterCategory::emote_cooking, FSB_ChatterType::Negative,
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

    {
        FSB_ChatterCategory::emote_oom, FSB_ChatterType::None,
        {
            "I'm out of mana!",
            "No mana left - I need a moment!",
            "I'm dry here, can't cast anything!",
            "Out of mana, someone cover me!",
            "I need mana, now!",
            "I'm tapped out - can't help much!",
            "No mana! I'm useless like this!",
            "I'm empty - need time to recover!",
            "I can't cast, I'm out!",
            "Mana's gone - watch my back!",
            "I'm running on fumes here!",
            "I've got nothing left to cast!",
            "I'm drained - need a breather!",
            "I'm completely out of mana!",
            "I can't keep this up without mana!",
            "Mana's gone - someone take over!",
            "I'm dry! Need mana badly!",
            "I'm out - can't support right now!",
            "No mana! I need a second!",
            "I'm spent - someone else handle this!"
        }
    },

    {
        FSB_ChatterCategory::emote_help, FSB_ChatterType::None,
        {
            "I need help over here!",
            "Someone get this thing off me!",
            "I can't handle this alone!",
            "Little help would be great!",
            "I'm getting overwhelmed!",
            "Help! I'm in trouble!",
            "I need backup, now!",
            "This one's too much for me!",
            "I can't keep this up alone!",
            "Anyone? A little help!",
            "I'm getting torn apart here!",
            "I need assistance!",
            "Someone cover me!",
            "I can't hold this thing!",
            "Help me out here!",
            "I'm struggling - need support!",
            "This is bad - help!",
            "I need someone on this target!",
            "I can't take much more of this!",
            "Back me up!"
        }
    },

    {
        FSB_ChatterCategory::emote_heal, FSB_ChatterType::None,
        {
            "I need healing!",
            "I'm not gonna last much longer!",
            "Healer! I need you!",
            "I'm badly hurt - help!",
            "I need a heal, now!",
            "I'm going down if I don't get healed!",
            "I'm bleeding out here!",
            "Someone patch me up!",
            "I can't take another hit!",
            "I'm in trouble - heal me!",
            "I'm hurt bad!",
            "I need healing support!",
            "I'm barely standing!",
            "I'm about to drop!",
            "Heals! I need heals!",
            "I'm critical - help!",
            "I can't survive like this!",
            "I'm one hit from dying!",
            "Please heal me!",
            "I need urgent healing!"
        }
    },

    {
        FSB_ChatterCategory::targetKilled, FSB_ChatterType::None,
        {
            "Rest in pieces, {target}.",
            "{target}? Yeah. that didn't end well for you.",
            "I warned you, {target}. Well. internally.",
            "And that's why I read my spellbook.",
            "One less {target}.",
            "And that's how you solve a {target} problem.",
        }
    },

    {
        FSB_ChatterCategory::targetKilledHired, FSB_ChatterType::None,
        {
            "Another one down. You're welcome, {player}.",
            "Rest in pieces, {target}.",
            "{target}? Yeah. that didn't end well for them.",
            "See, {player}? Perfectly under control.",
            "I warned you, {target}. Well. internally.",
            "That went about as expected. For me.",
            "{player}, please tell me you saw that.",
            "And that's why I read my spellbook.",
            "One less {target}. Try to keep up, {player}.",
            "I call that a successful application of violence.",
            "That's one less problem, {player}. Efficient, right?",
            "{target} really should've picked a different fight.",
            "Did you see that, {player}? Textbook execution.",
            "{target} has officially exited the conversation.",
            "Clean hit. You're welcome, {player}.",
            "And that's how you solve a {target} problem.",
            "I handled it, {player}. You can relax now.",
            "{target} didn't stand a chance. Not with me here.",
            "One more down. Our teamwork is terrifyingly effective.",
            "I hope you were watching, {player}. That was impressive.",
            "Another flawless performance. I'm on a roll today.",
            "That {target} folded faster than I expected.",
            "You see that, {player}? That's what competence looks like.",
            "I almost feel bad for {target}. Almost.",
            "That was satisfying. What's next, {player}?",
            "Another clean takedown. I should start keeping score.",
            "If only {target} knew who they were dealing with.",
            "Handled. Smoothly. Professionally. Stylishly.",
            "That's how we do it, {player}. On to the next.",
            "One less {target} to worry about. Easy."
            "That's one less problem, {player}. Efficient, right?",
            "{target} really should've picked a different fight.",
            "Did you see that, {player}? Textbook execution.",
            "{target} has officially exited the conversation.",
            "Clean hit. You're welcome, {player}.",
            "And that's how you solve a {target} problem.",
            "I handled it, {player}. You can relax now.",
            "{target} didn't stand a chance. Not with me here.",
            "One more down. Our teamwork is terrifyingly effective.",
            "I hope you were watching, {player}. That was impressive.",
            "Another flawless performance. I'm on a roll today.",
            "That {target} folded faster than I expected.",
            "You see that, {player}? That's what competence looks like.",
            "I almost feel bad for {target}. Almost.",
            "That was satisfying. What's next, {player}?",
            "Another clean takedown. I should start keeping score.",
            "If only {target} knew who they were dealing with.",
            "Handled. Smoothly. Professionally. Stylishly.",
            "That's how we do it, {player}. On to the next.",
            "One less {target} to worry about. Easy.",
        }
    },

    {
        FSB_ChatterCategory::botDismissed, FSB_ChatterType::None,
        {
            "Time's up. Coin's spent. I'm off - good luck out there.",
            "That's it, {player}. Contract fulfilled. Try not to die without me.",
            "Well, that was a few hours of my life I'll never get back. Farewell.",
            "My watch is over. If you need me again, bring more coin.",
            "Alright, {player}, that's my cue. Duty done, boots moving.",
            "The gold's gone and so am I. Fair deal.",
            "Contract's finished. I suggest you find another friend. quickly.",
            "That's enough adventuring for one pay period. I'm out.",
            "No hard feelings, {player}. Business is business.",
            "Well, this is where I leave you. Try not to pull anything too big next time.",
            "Our deal's done, {player}. Try not to get into trouble five minutes after I leave.",
            "Contract complete. I'll be around. assuming your coin purse recovers.",
            "That's my shift over. You're on your own from here, {player}.",
            "Alright, that's all you paid for. Don't spend it all in one dungeon.",
            "Duty fulfilled. I'll consider returning if the pay improves.",
            "My time's up. You've been. tolerable, {player}.",
            "That concludes our arrangement. Walk safe out there.",
            "I'm heading out. If you need me again, you know the price.",
            "Our business is concluded. Try not to summon me for nonsense next time.",
            "That's the end of the contract. I'll see myself out.",
            "Well, {player}, it's been real. And by real, I mean adequately compensated.",
            "My services are officially rendered. Good luck surviving the next hour.",
            "We're square. Don't get yourself killed before I'm hired again.",
            "That's all from me. If danger shows up, pretend I'm still here.",
            "Contract expired. I'm off to find a softer job. or better pay.",
            "Alright, {player}, I'm clocking out. Try not to miss me too much.",
            "Our deal's wrapped up. I'll be around if your wallet gets heavier.",
            "That's my last task done. You're free to panic now, {player}.",
            "Time's up. I'm leaving before you find more trouble.",
            "We're done here. Don't worry, {player}-I'll remember the good parts."
        }
    },

    {
        FSB_ChatterCategory::botAcknowledge, FSB_ChatterType::None,
        {
            "On it, {player}. Consider it handled.",
            "Alright, moving now.",
            "Got your command loud and clear, {player}.",
            "Understood. Executing.",
            "As you wish. Let's get it done.",
            "Acknowledged. I'll take care of it.",
            "Right away, {player}.",
            "Okay, shifting focus.",
            "Command received. Acting on it.",
            "Sure thing. I'm on the move.",
            "Got it. Don't worry, I know what to do.",
            "Alright, {player}, doing it now.",
            "Copy that. Adjusting course.",
            "Understood. I'll handle the details.",
            "Yep, I'm already on it.",
            "Very well. Proceeding as instructed.",
            "Okay, {player}. Let's make it look easy.",
            "Message received. Executing your plan.",
            "Alright, I'll take point on this.",
            "Done. Or it will be in a moment."
        }
    },

    {
        FSB_ChatterCategory::botFollow, FSB_ChatterType::None,
        {
            "Alright, I'm back with you.",
            "Following you again. Let's move.",
            "There you are. Lead the way, {player}.",
            "I was starting to enjoy standing still. but alright.",
            "Back on your heels. Try not to run too fast.",
            "Following, {player}. Let's see where this goes.",
            "Right behind you. More or less.",
            "Okay, I'm coming. No need to shout.",
            "Time to move again. I'm with you.",
            "Let's go, {player}. I'm ready.",
            "Ah, adventure calls again. I'm following.",
            "Standing still was nice. Back to walking it is.",
            "Alright, {player}. I'll keep up. probably."
        }
    },

    {
        FSB_ChatterCategory::botStay, FSB_ChatterType::None,
        {
            "Alright, I'll hold this spot. Don't take too long.",
            "Staying put. If something bites me, I'm blaming you.",
            "Very well, {player}. I'll stand guard right here.",
            "You want me here? Fine. I wasn't going anywhere anyway.",
            "Holding position. Try not to forget about me.",
            "Right here, {player}. I'll keep watch.",
            "I'll stay. But if a dragon shows up, that's on you.",
            "Standing still. which is harder than it sounds.",
            "This spot? Really? Alright. I'll make it work.",
            "As you wish, {player}. I'll be right here when you return.",
            "Standing guard. This better be important.",
            "I'll stay. Just don't expect me to enjoy it.",
            "Holding position. Someone's got to be the responsible one."
        }
    },

    {
        FSB_ChatterCategory::botMemberDied, FSB_ChatterType::Positive,
        {
            "Hang in there, {target}! We'll get you back up.",
            "Don't worry, {target}, we've got this under control.",
            "You fought well, {target}. We'll finish what you started.",
            "Stay calm, {target}. We'll pick you up soon.",
            "Rest for a moment, {target}. We'll handle the rest.",
            "You're not out of the fight yet, {target}!",
            "We'll carry on for you, {target}.",
            "It's okay, {target}. Happens to the best of us.",
            "You did great, {target}. We'll clean up the mess.",
            "Hold tight, {target}. We'll get you back on your feet.",
            "We'll avenge you, {target}. Promise.",
            "You gave it your all, {target}. We'll finish strong.",
            "No shame in that fall, {target}. We've got your back.",
            "Take a breather, {target}. We'll wrap this up.",
            "You're still part of the team, {target}. Just. temporarily horizontal.",
            "We'll make sure your effort wasn't in vain.",
            "You'll be back in action soon, {target}.",
            "We'll keep pushing forward for you.",
            "You did your part, {target}. Now it's our turn.",
            "Stay with us, {target}. We're not done yet."
        }
    },

    {
        FSB_ChatterCategory::botMemberDied, FSB_ChatterType::Neutral,
        {
            "{target} is down. Adjusting strategy.",
            "One member down. Keep focus.",
            "{target} has fallen. Repositioning.",
            "Noted. {target} is out of the fight.",
            "We've lost one. Continue the plan.",
            "{target} is down. Maintaining formation.",
            "Acknowledged. Party member eliminated.",
            "Target {target} has died. Updating priorities.",
            "One less fighter. Stay alert.",
            "Member down. Proceeding with caution.",
            "{target} is out. Compensating.",
            "Death registered. Continuing engagement.",
            "We're short one now. Keep moving.",
            "Status update: {target} is dead.",
            "Another one down. Staying on task.",
            "Not ideal, but manageable.",
            "Logging event: {target} has fallen.",
            "We continue with reduced numbers.",
            "Death confirmed. Adjusting behavior.",
            "Noted. {target} won't be contributing for a bit."
        }
    },

    {
        FSB_ChatterCategory::botMemberDied, FSB_ChatterType::Negative,
        {
            "Ah, shiiiiiit, nooooo {target}!",
            "Hey {target}, what are you doing?!",
            "How did we let this happen?!",
            "Seriously, {target}? Now?!",
            "Why does this always happen at the worst time?",
            "We were so close. yet so far away.",
            "Hope you won't blame me for this, {target}!",
            "No, no, no-{target} is down!",
            "Unbelievable. {target} just dropped.",
            "This is bad. Really bad.",
            "Come on, {target}! You had ONE job!",
            "Why? Why now?",
            "Great. just great. {target} is dead.",
            "This is falling apart fast.",
            "How did I let this happen?",
            "Death is just a number, {target}! But still!",
            "Ugh. that's going to leave a mark.",
            "We're doomed. Absolutely doomed.",
            "Not again. {target}, why?!",
            "Perfect. Exactly what we needed. another corpse."
        }
    },

    {
        FSB_ChatterCategory::botRevived, FSB_ChatterType::Positive,
        {
            "Ahh, feels good to breathe again.",
            "Back on my feet! Let's not do that again.",
            "Alright, round two. I'm ready.",
            "That walk wasn't too bad. I've had worse.",
            "I'm alive! Again! Still counts.",
            "Okay, I'm back. What did I miss?",
            "Good news: I'm no longer dead.",
            "Freshly revived and feeling optimistic.",
            "Alright, let's get back to business.",
            "I return stronger. or at least less dead.",
            "That was a refreshing stroll.",
            "Okay, I'm upright. Let's go.",
            "Back from the great beyond!",
            "I lived! .Eventually.",
            "Alright, I'm functional again.",
            "That wasn't so bad. Could've been worse.",
            "Respawn complete. Confidence restored.",
            "I'm back in action. Let's keep moving.",
            "Good walk, good revive, good to go.",
            "Okay, I'm revived. Let's pretend that never happened."
        }
    },

    {
        FSB_ChatterCategory::botRevived, FSB_ChatterType::Neutral,
        {
            "Revival complete. Resuming operations.",
            "Corpse reached. Status: restored.",
            "Back online. Continuing activity.",
            "Resurrection successful. Proceeding.",
            "Returned to functional state.",
            "Revived. Rejoining the group.",
            "Walk completed. Body recovered.",
            "Back from the graveyard. Moving on.",
            "Systems stable. Ready to continue.",
            "Death penalty cleared. Resuming tasks.",
            "Revival acknowledged. Standing by.",
            "Operational again. Awaiting instructions.",
            "Corpse retrieval successful.",
            "Reactivated. No further issues.",
            "Back to normal parameters.",
            "Revived. Maintaining formation.",
            "Recovery complete. Continuing route.",
            "Status update: alive again.",
            "Back in working condition.",
            "Revival logged. Proceeding."
        }
    },

    {
        FSB_ChatterCategory::botRevived, FSB_ChatterType::Negative,
        {
            "Ugh. that walk took forever.",
            "Finally! I thought I'd never reach my body.",
            "That was the longest jog of my life.",
            "Great. I'm alive again. Barely worth it.",
            "I swear the graveyard gets farther every time.",
            "My legs hurt more than the death.",
            "That was miserable. Absolutely miserable.",
            "If I die again, I'm staying down.",
            "Why is my corpse always in the worst spot?",
            "That walk was punishment enough.",
            "I'm back. and annoyed.",
            "Next time, someone carry me.",
            "I hate this part of adventuring.",
            "That was a marathon, not a corpse run.",
            "I'm alive, but my patience isn't.",
            "Never making that walk again. Ever.",
            "I need a break after that trek.",
            "That revive better have been worth it.",
            "I'm back, but I'm not happy about it.",
            "If I die again, I'm blaming someone."
        }
    },

    {
        FSB_ChatterCategory::botDeathHired, FSB_ChatterType::None,
        {
            "Well. this seems suboptimal. {player}, I blame you.",
            "I was promised healing. This feels like betrayal, {player}.",
            "Tell my spellbook I loved it. And {player}, you're still paying for repairs.",
            "Next time, maybe don't pull half the place, {player}.",
            "Good news, {player}: I found the floor. With my face.",
            "I'm not saying this is your fault, {player}. but it's definitely your fault.",
            "Remember me as I was: alive, and judging your decisions, {player}.",
            "Ah yes. the sweet embrace of death. Thanks, {player}.",
            "Well played, {target}. You got me. {player}, take notes.",
            "I hope that was worth it, {player}.",
            "This is fine. Everything is fine. I am absolutely-dead.",
            "If anyone asks, {player}, this was your idea.",
            "I regret everything. mostly trusting you, {player}.",
            "Congratulations, {target}. You win this round.",
            "I'll be at the graveyard if you need me, {player}.",
            "I died doing what I loved: panicking while {player} made bad choices.",
            "Oh look, {target} killed me. Again. Lovely.",
            "Well, {player}, at least YOU survived. That's what matters, right?",
            "Tell my loot I loved it. And that I'll miss it.",
            "I hope you're happy, {player}. I'm not.",
            "Note to self: never trust {player} with my health bar.",
            "I'm dead. Again. Shocking, I know.",
            "If you need me, {player}, I'll be haunting the graveyard.",
            "Hey {target}, next time try killing {player} instead.",
            "I didn't stand a chance. not with {player} leading.",
            "Well, {target} hit harder than expected. Or maybe I expected too much from you, {player}.",
            "I fall so you may. continue making questionable decisions, {player}.",
            "Death comes for us all. But mostly for me, apparently.",
            "I hope you have a plan, {player}, because I'm fresh out.",
            "Goodbye, cruel world. And goodbye, {player}. I trusted you."
        }
    },

    {
        FSB_ChatterCategory::botDeath, FSB_ChatterType::None,
        {
            "Ah yes. the sweet embrace of the spirit healer.",
            "Well, {target}, you got me. Enjoy the victory.",
            "Tell the spirit healer I said hi. Again.",
            "I guess this is my life now. Or. unlife.",
            "I hope you're proud of yourself, {target}.",
            "Great. Another scenic walk from the graveyard.",
            "I didn't want to live anyway. Too much responsibility.",
            "Spirit healer, warm up the glow. I'm on my way.",
            "I swear the graveyard gets farther every time.",
            "Well, that escalated quickly.",
            "Good fight, {target}. I mean. for you.",
            "I'm starting to think the spirit healer knows me by name.",
            "Ah yes, death. My oldest friend.",
            "I'll be back. Probably annoyed.",
            "At least the spirit healer doesn't judge me.",
            "I fall, {target}, but I fall fabulously.",
            "Another day, another corpse run.",
            "I hope the spirit healer offers loyalty points.",
            "Well, that was embarrassing.",
            "I regret nothing. Except everything.",
            "If anyone asks, {target}, this never happened.",
            "I'm not dead. I'm just. temporarily defeated.",
            "I'll rise again. Like a very irritated phoenix.",
            "Note to self: avoid {target}.",
            "I'm beginning to think I'm bad at this 'living' thing.",
            "Ah well. Time to go touch my corpse again.",
            "I hope my body is somewhere convenient this time.",
            "Death count: too many. Patience: zero.",
            "I'll be back, {target}. And next time, I'm bringing snacks.",
            "Spirit healer, prepare the resurrection glow. I'm coming home."
        }
    },

    {
        FSB_ChatterCategory::botHiredPermanent, FSB_ChatterType::None,
        {
            "Looks like we're in this together now, {player}.",
            "Alright. I'm yours until the end - wherever that may be.",
            "No contracts, no timers. I'll stay with you.",
            "Seems I've chosen a side. Let's make it a good one, {player}.",
            "You've got yourself a companion, not just hired help.",
            "I'm in. Wherever you go, I follow.",
            "Well then. looks like I'm part of your story now.",
            "No turning back now. I've got your back, {player}.",
            "Guess this makes us partners.",
            "From now on, I fight by your side.",
            "Permanent, huh? Bold choice. Let's see how this goes.",
            "I suppose this is what loyalty feels like.",
            "No clock ticking anymore. I'm with you."
        }
    },

    {
        FSB_ChatterCategory::botRevivedTarget, FSB_ChatterType::None,
        {
            "Hey, {target}, Don't you die on me again, ok?",
            "Ah, you're good as new, {target}!",
            "Aren't you glad you brought me along?",
            "Hope I don't have to revive you again, {target}!",
            "One resurrect for you and another for... oh nevermind!",
            "This resurrect is for free, next one...",
            "Hope you appreciate the revival, {target}!",
            "Rise, {target}! The spirits aren't done with you yet.",
            "Back from the brink, {target}. Try not to visit again so soon.",
            "The Light has plans for you, {target}. Don't waste them.",
            "On your feet, {target}. Death was getting bored of you.",
            "I dragged your soul back myself, {target}. You're welcome.",
            "Wake up, {target}. The afterlife said 'no refunds'.",
            "You owe me one, {target}. Preferably something shiny.",
            "Arise, {target}! Your corpse was lowering team morale.",
            "Death rejected your application, {target}. Try again later.",
            "The ancestors said you still have chores to do, {target}.",
            "Get up, {target}. The battle isn't going to lose itself.",
            "I stitched your soul back in place, {target}. Don't pull the seams.",
            "You're alive again, {target}. Try to stay that way this time.",
            "The spirits whispered your name, {target}. I told them 'not today'.",
            "Up you go, {target}. Gravity won't keep you forever.",
            "I bring you back, {target}. What you do with that is. questionable.",
            "Your story continues, {target}. Try not to make it a short one.",
            "The Light shines upon you, {target}. Mostly because I forced it to.",
            "Welcome back, {target}. Your corpse was starting to smell.",
            "There you are, {target}. Don't scare me like that again."
        }
    },

    {
        FSB_ChatterCategory::botHealTarget, FSB_ChatterType::None,
        {
            "Hey, {target}, Don't you feel stronger already!",
            "Ah, just what you needed, {target}!",
            "Power surging through you!",
            "Heal coming up your way, {target}!",
            "One heal for you and another heal for you...",
            "This heal is for free, next one...",
            "Hope you appreciate the heal, {target}!",
            "Feel that, {target}? That's the warm glow of not dying.",
            "Hold still, {target}. I'm trying to fix whatever that was.",
            "There you go, {target}. Try not to leak everywhere again.",
            "Light's blessing upon you, {target}. Or at least. my best attempt.",
            "You're fine, {target}. Mostly. Probably.",
            "Healing you again, {target}. It's becoming a hobby.",
            "Stay still, {target}. I can't heal what keeps running away.",
            "There, {target}. Good as. well, better than before.",
            "You're welcome, {target}. I expect snacks later.",
            "A little magic, a little hope. and you're patched up, {target}.",
            "Try not to undo my hard work, {target}.",
            "Healing surge incoming, {target}! Don't waste it.",
            "You look terrible, {target}. Let me fix that.",
            "The spirits say you should stop getting hit, {target}.",
            "You owe me for this one, {target}. Preferably gold.",
            "Feel the power, {target}! And maybe some mild tingling.",
            "You're healed, {target}. Now stop screaming.",
            "I can only heal so fast, {target}. Stop testing my limits.",
            "There, {target}. I've glued your soul back in place.",
            "A touch of Light for you, {target}. Don't spend it all at once.",
            "You're patched up, {target}. Try not to explode again.",
            "Healing spell deployed! {target}, please stop standing in fire.",
            "You're fine now, {target}. Go pretend you're invincible again.",
            "Another heal for you, {target}. I'm starting a collection.",
            "You're welcome, {target}. Again. Always again.",
            "I swear, {target}, you take more damage than a training dummy.",
            "Hold on, {target}. I'm healing your questionable decisions.",
            "There you go, {target}. Freshly repaired.",
            "Your health bar was embarrassing, {target}. I fixed it.",
            "All healed, {target}. Now go make more problems for me."
        }
    },

    {
        FSB_ChatterCategory::botCombatMana, FSB_ChatterType::None,
        {
            "Ugh. my brain's running on empty!",
            "Time for a little {spell} snack!",
            "If I run out of mana now, it's over. gulp!",
            "Someone toss me a {spell} potion, stat!",
            "I can almost feel my powers fading.",
            "Nothing a sip of magic can't fix!",
            "Mana low. desperation high!",
            "This {spell} better work. or I'm toast!",
            "Friend have a {spell} for me? No...?! Ok, I'll use mine..."
        }
    },

    {
        FSB_ChatterCategory::botCombatHealth, FSB_ChatterType::None,
        {
            "This {spell} is my last... wish I had more!",
            "No! Am too young to meet the spirit healer...",
            "{spell} I choose you! Don't let me down!",
            "Ugh. Think am gonna faint soon!",
            "Time for a little {spell} snack!",
            "If I run out of juice now, it's over. gulp!",
            "Someone toss me a {spell} potion, stat!",
            "I can almost feel my strength fading.",
            "Nothing a sip of magic can't fix!",
            "Health low. desperation high!",
            "This {spell} better work. or I have a date with the spirit healer!",
            "Can you spare a {spell} for me? No...?! Ok, I'll use mine..."
        }
    },

    {
        FSB_ChatterCategory::botCombatSpell, FSB_ChatterType::Neutral,
        {
            "Hey, {target}, do you like my {spell}",
            "Take that, you {target}!",
            "Feel the power of my {spell}!",
            "You're gonna feel this one, {target}!",
            "One {spell} for you and another for you {target}.",
            "I've got more {spell} from where this came from!",
            "Hope you like my {spell}, you {target}!",
            "Hey {target}, how about a taste of my {spell}?",
            "Take this {spell}, {target}! Freshly cast.",
            "Feel the sting of my {spell}, {target}!",
            "Incoming {spell}! Try to dodge this one, {target}.",
            "One {spell} for you, {target}, and plenty more where that came from.",
            "Hope you enjoy my {spell}, {target}!",
            "Let's see how you handle this {spell}, {target}.",
            "Brace yourself, {target}. My {spell} isn't gentle.",
            "Here comes a friendly little {spell}. just kidding, {target}.",
            "You're gonna feel this {spell}, {target}!",
            "My {spell} says hello, {target}.",
            "Don't take it personally, {target}. It's just a {spell}.",
            "Try not to explode from this {spell}, {target}.",
            "I cast {spell}! You deal with it, {target}.",
            "Hope you like surprises, {target}. This one's called {spell}.",
            "Catch this {spell}, {target}! No refunds.",
            "You look like you need a {spell} to the face, {target}.",
            "Here, {target}-a complimentary {spell}.",
            "My {spell} is coming for you, {target}. Good luck.",
            "Let's brighten your day with a {spell}, {target}.",
            "Let's see how you handle this.",
            "Hope you're ready, {target}!",
            "Casting {spell}! Try not to blink.",
            "You look like you need a little magical encouragement.",
            "Incoming! Don't say I didn't warn you.",
            "Alright {target}, here comes something spicy.",
            "A little {spell} should brighten your day.",
            "Hold still, this works better if you don't move.",
            "Let's test your reflexes, {target}.",
            "Time for a bit of fireworks.",
        }
    },

    {
        FSB_ChatterCategory::botCombatSpell, FSB_ChatterType::Negative,
        {
            "Hey, {target}, do you like my {spell}",
            "Take that, you {target}!",
            "Feel the power of my {spell}!",
            "You're gonna feel this one, {target}!",
            "One {spell} for you and another for you {target}.",
            "I've got more {spell} from where this came from!",
            "Hope you like my {spell}, you {target}!",
            "Scream for me, {target}! My {spell} is hungry!",
            "You can't escape this {spell}, {target}. Not today.",
            "Fall before my {spell}, {target}! Fall HARD.",
            "I will break you, {target}, one {spell} at a time!",
            "Feel the wrath of my {spell}, {target}! FEEL IT!",
            "Run if you want, {target}. My {spell} will find you.",
            "Your suffering fuels my {spell}, {target}!",
            "Kneel, {target}! My {spell} demands it!",
            "I hope you like pain, {target}, because my {spell} brings plenty.",
            "You look fragile, {target}. Let my {spell} finish the job.",
            "My {spell} will carve your defeat into the battlefield, {target}.",
            "Beg all you want, {target}. My {spell} doesn't listen.",
            "I'll crush you with this {spell}, {target}!",
            "Your end begins with this {spell}, {target}.",
            "Try to survive this {spell}, {target}. I dare you.",
            "My {spell} will be the last thing you remember, {target}.",
            "You're nothing before my {spell}, {target}!",
            "Fall, {target}! My {spell} commands it!",
            "I will enjoy watching this {spell} hit you, {target}.",
            "Your defeat is inevitable, {target}. My {spell} makes sure of it.",
            "You're already doomed, {target}. This just speeds it up.",
            "My {spell} is going to ruin your whole day.",
            "Try surviving this, I dare you.",
            "You picked the wrong fight, {target}.",
            "I hope you like pain, because here comes plenty.",
            "This {spell} is the beginning of your end.",
            "I'm not holding back anymore.",
            "Fall already, {target}. You're wasting my time.",
            "Let's see how loud you scream when this hits.",
            "You won't forget this {spell}. assuming you survive.",
        }
    },

    {
        FSB_ChatterCategory::botCombatSpell, FSB_ChatterType::Positive,
        {
            "I truly wish this wasn't necessary.",
            "May this {spell} guide you toward better choices, {target}.",
            "Let's resolve this quickly and calmly.",
            "I hope this doesn't hurt too much.",
            "Balance must be restored. even if it requires a {spell}.",
            "Please forgive me, {target}.",
            "I strike only to end the conflict.",
            "May harmony return after this.",
            "I'll try to make this gentle. but {spell} has its own ideas.",
            "Let's finish this without further harm.",
            "Hey, {target}, do you like my {spell}",
            "Take that, you {target}!",
            "Feel the power of my {spell}!",
            "You're gonna feel this one, {target}!",
            "One {spell} for you and another for you {target}.",
            "I've got more {spell} from where this came from!",
            "Hope you like my {spell}, you {target}!",
            "Forgive me, {target}. but this {spell} must be done.",
            "I take no joy in this, {target}. May this {spell} be swift.",
            "Peace be with you, {target}. Even as my {spell} lands.",
            "I wish there were another way, {target}. Please accept this {spell}.",
            "May harmony guide us both, {target}. Even through this {spell}.",
            "I hope this {spell} teaches balance, not pain, {target}.",
            "Breathe deeply, {target}. The {spell} will pass.",
            "Let this {spell} be a gentle reminder, {target}.",
            "I strike only to restore balance, {target}.",
            "May this {spell} bring clarity, {target}, not suffering.",
            "I mean no harm, {target}. But my {spell} disagrees.",
            "Let us learn from this moment, {target}. Starting with this {spell}.",
            "I hope you find peace after this {spell}, {target}.",
            "Please don't take this personally, {target}. It's just a {spell}.",
            "May your spirit remain calm, {target}, even as my {spell} lands.",
            "Violence is not my wish, {target}. But {spell} insists.",
            "I cast this {spell} with a heavy heart, {target}.",
            "May this {spell} guide you toward wisdom, {target}.",
            "I apologize in advance, {target}. My {spell} is unavoidable.",
            "Let this {spell} be the last strike between us, {target}."
        }
    },

    {
        FSB_ChatterCategory::botHealSelf, FSB_ChatterType::None,
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
            "Staying alive.",
            "This heal better do its trick.",
            "I needed this heal.",
            "All patched up and good to go."
        }
    },

    {
        FSB_ChatterCategory::botBuffSelf, FSB_ChatterType::None,
        {
            "Am gonna be needing this!",
            "This {spell} is sure gonna help.",
            "At least I have my <spell>!",
            "There's no <spell> like mine!",
            "One {spell} for me and another for... me... later.",
            "I've got more {spell} from where this came from!",
            "This buffs me buff... haha...haha!",
            "Ahh, that's the good stuff.",
            "Time to give myself a little boost.",
            "Nothing like a fresh {spell} to start the fight right.",
            "Feeling stronger already!",
            "This should keep me standing a bit longer.",
            "A little magic never hurt. well, me at least.",
            "Let's see them try to break through this.",
            "One self-buff coming right up!",
            "I deserve this.",
            "This {spell} should do the trick.",
            "Alright, power time!",
            "A bit of protection never hurts.",
            "I'll be needing this for what's coming.",
            "Let's make things interesting.",
            "This {spell} always makes me feel unstoppable.",
            "Buffing up! Don't mind me.",
            "I feel. surprisingly heroic now.",
            "A little extra strength never hurt anyone.",
            "This should keep me alive longer than expected.",
            "I knew saving this {spell} was a good idea.",
            "Time to shine!",
            "Oh yes, this feels right.",
            "If I don't take care of myself, who will?",
            "Let's get serious.",
            "This {spell} makes me feel like a champion.",
            "I'm not going down that easily.",
            "A touch of magic for yours truly.",
            "I'll take all the help I can get.",
            "This buff makes me buff. again!",
            "Alright world, try me now.",
        }
    },

    {
        FSB_ChatterCategory::botBuffTarget, FSB_ChatterType::Neutral,
        {
            "Hey, {target}, no need to thank me for {spell}",
            "I've got something for you, {target}!",
            "My {spell} makes you stronger!",
            "You're gonna feel this buff, {target}!",
            "This {spell} is for free {target}.",
            "Buffing was not stated in my contract but here you go!",
            "One {spell}, coming for you {target}!",
            "Buffing you up, {target}.",
            "Here comes a {spell}-don't waste it.",
            "You'll need this more than I do.",
            "A little boost for you.",
            "This {spell} should help out.",
            "Hold still. there we go.",
            "You're good to go now.",
            "Take this. Might come in handy.",
            "A quick {spell} for you, {target}.",
            "Let's make you a bit tougher.",
            "You'll thank me later.",
            "Alright, {target}, you're buffed.",
            "This should keep you alive longer.",
            "Here, take some extra power.",
            "A little magical insurance never hurts.",
            "You're stronger now. Probably.",
            "Let's top you off with a {spell}.",
            "There. You're welcome.",
            "Boosting your stats, {target}.",
            "Try not to lose this buff immediately.",
        }
    },

    {
        FSB_ChatterCategory::botBuffTarget, FSB_ChatterType::Positive,
        {
            "A little strength for you, {target}. Use it well.",
            "May this {spell} bring you clarity and courage.",
            "Here, {target}. I hope this helps you shine.",
            "Let this blessing guide your steps.",
            "You fight bravely, {target}. Allow me to support you.",
            "Take this {spell}. You've earned it.",
            "May this magic keep you safe.",
            "I'm honored to lend you my strength, {target}.",
            "Let this {spell} steady your spirit.",
            "You carry us far, {target}. Let me carry you for a moment.",
            "A touch of power for a worthy ally.",
            "This {spell} should help you stand strong.",
            "You're doing great, {target}. Here's a little boost.",
            "May this gift strengthen your resolve.",
            "Let this magic remind you you're not alone.",
            "You've got this, {target}. I believe in you.",
            "A small blessing for a big heart.",
            "Take this {spell}. I'm glad to fight beside you.",
            "May harmony follow you after this.",
            "For you, {target}. With respect.",
        }
    },

    {
        FSB_ChatterCategory::botBuffTarget, FSB_ChatterType::Negative,
        {
            "Fine, {target}. Take this {spell}. You owe me.",
            "I better get paid for this.",
            "Here's your {spell}. Don't expect another free one.",
            "You want a buff? Then stop whining.",
            "Take this and don't make me regret it.",
            "I swear, {target}, you burn through buffs like gold.",
            "This {spell} isn't cheap, you know.",
            "You better make this worth my mana.",
            "Ugh, fine. Hold still.",
            "If I buff you, you better stop dying.",
            "This is the last {spell} you get without compensation.",
            "I'm not your personal buff machine, {target}.",
            "Take the buff and get out there.",
            "You owe me big time for this one.",
            "Don't waste my {spell}, {target}. Seriously.",
            "I expect a favor later. A big one.",
            "If this {spell} doesn't help, that's on you.",
            "Alright, alright-here's your buff. Happy now?",
            "You better not lose this buff in five seconds again.",
            "I'm only doing this because someone has to.",
        }
    },

    {
        FSB_ChatterCategory::botOOCRecoveryHired, FSB_ChatterType::None,
        {
            "Hey, {player}, I really need a break...",
            "Let's sit for a moment yeah, {player}!",
            "My {spell} will make me like new...",
            "You're gonna have to wait for me, {player}!",
            "This {spell} is exactly what I need.",
            "We can't go on an empty stomach, now can we?",
            "I need my {spell}, break now {player}!",
            "Just catching my breath, {player}. Don't mind me.",
            "Whew. I needed that. Thanks for watching my back, {player}.",
            "Give me a moment, {player}. Even heroes get tired.",
            "I'll be ready again soon, {player}. Promise.",
            "Let me sip some mana. don't judge me, {player}.",
            "Hold on, {player}. I'm patching myself up.",
            "Almost recovered. keep an eye out, {player}.",
            "Thanks for waiting, {player}. I'll be back in the fight shortly.",
            "I swear I'm usually tougher than this, {player}.",
            "Just a quick recharge from {spell}. You didn't see anything, {player}.",
            "Ahh. power flowing again. {spell} helped. Appreciate the patience, {player}.",
            "Give me a sec, {player}. My legs forgot how to leg.",
            "Healing up. don't wander off, {player}.",
            "I'll be good as new in a moment, {player}.",
            "Let me just. breathe. okay, still alive.",
            "Thanks for sticking around, {player}. Recovery in progress.",
            "I'll be fine, {player}. Just need a moment of peace and my {spell}",
            "Almost done recovering. Try not to pull anything, {player}.",
            "I'm recharging my inner spirit. and my outer everything.",
            "Alright, {player}, I'm nearly ready. Just one more deep breath.",
        }
    },

    {
        FSB_ChatterCategory::botOOCRecovery, FSB_ChatterType::None,
        {
            "Okay. deep breaths. I can do this.",
            "Ugh, I really need a break.",
            "Mana, don't fail me now.",
            "Just a moment. I'm not as young as I look.",
            "Why does recovering always take forever?",
            "Note to self: stop tanking with my face.",
            "Alright, body. heal faster, please.",
            "I swear I used to recover quicker.",
            "Come on mana, flow like you mean it.",
            "I'm too tired for this nonsense.",
            "Just a quick rest. nobody saw that.",
            "Okay, okay. still alive. Good.",
            "I need a vacation after this.",
            "Why do I always end up half-dead?",
            "Let me just. sit here. and pretend everything's fine.",
            "If anyone asks, I fought ten enemies at once.",
            "Recovering. again. Story of my life.",
            "I should really invest in better gear.",
            "Alright, back to breathing normally.",
            "One day I'll learn not to stand in fire. today is not that day."
        }
    },

    {
        FSB_ChatterCategory::botHire, FSB_ChatterType::None,
        {
            "Hey {player}, I am all yours for the next {duration}.",
            "Alright {player}, let's band together for {duration}.",
            "Ok, I'll come along, {player}, since you paid me for {duration}.",
            "Only {duration}? Sure thing, {player}.",
            "Well {player}, my calendar is empty for {duration}. Let's make poor life choices.",
            "I suppose {duration} with you won't kill me, {player}. Probably.",
            "You pay, I follow, {player}. {duration} it is.",
            "For {duration}, {player}, I am your loyal companion. After that, we renegotiate.",
            "Ah yes, {duration} of danger, glory, and questionable decisions, {player}.",
            "Very well {player}, I shall tolerate you for {duration}.",
            "{duration}? Fine. But if we die, it's your fault, {player}.",
            "I was bored anyway, {player}. {duration} sounds perfect.",
            "Gold talks, {player}. And it says '{duration} of adventure.'",
            "Let's do this, {player}. {duration} in, we pretend we planned it."
        }
    },
};

namespace FSBChatter
{
    std::string GetRandomReply(Creature* bot, Unit* target, FSB_ChatterCategory category, FSB_ChatterType chatterType, uint32 spellId, uint8 duration)
    {
        Player* player = FSBMgr::Get()->GetBotOwner(bot);

        for (auto const& entry : FSBReplyTable)
        {
            if (entry.category == category && entry.chatterType == chatterType)
            {
                if (entry.lines.empty())
                {
                    TC_LOG_WARN("scripts.fsb.chatter", "FSB: Chatter GetRandomReply: No string found for category {} and chatterType {}", category, chatterType);
                    return "";
                }

                std::string line = entry.lines[urand(0, (entry.lines.size() - 1))];

                // Token replacement
                if (bot)
                    ReplaceAll(line, "{bot}", bot->GetName());
                
                if (target)
                    ReplaceAll(line, "{target}", target->GetName());

                if(player)
                    ReplaceAll(line, "{player}", player->GetName());

                if (spellId)
                    ReplaceAll(line, "{spell}", FSBSpellsUtils::GetSpellName(spellId));

                if(duration == 1)
                    ReplaceAll(line, "{duration}", "1 hour");

                if (duration > 1)
                    ReplaceAll(line, "{duration}", std::to_string(duration) + " hours");

                TC_LOG_DEBUG("scripts.fsb.chatter", "FSB: Chatter GetRandomReply: String [{}] selected for category {} and chatterType {}", line, category, chatterType);

                return line;
            }
        }

        return "";
    }

    void DemandTimedReply(Creature* bot, Unit* target, FSB_ChatterCategory category, FSB_ReplyType replyType, FSB_ChatterSource chatterSource)
    {
        if (!bot)
            return;

        // we only expect the bot to be dead for those 2 categories
        if ((category != FSB_ChatterCategory::botDeath &&
            category != FSB_ChatterCategory::botDeathHired) &&
            !bot->IsAlive())
            return;

        if (target && !target->IsAlive())
            return;

        FSB_ChatterType type = FSB_ChatterType::None;

        if(target && chatterSource == FSB_ChatterSource::Target)
            type = FSBMgr::Get()->GetBotChatterTypeForEntry(target->GetEntry());

        else if (chatterSource == FSB_ChatterSource::Bot)
            type = FSBMgr::Get()->GetBotChatterTypeForEntry(bot->GetEntry());

        std::string replyString = GetRandomReply(bot, target, category, type, 0);

        TC_LOG_DEBUG("scripts.fsb.chatter", "FSB: Chatter DemandTimedReply: Bot {} String {} selected for category {} and chatterType {}", bot->GetName(), replyString, category, type);

        if (category == FSB_ChatterCategory::botDeath ||
            category == FSB_ChatterCategory::botDeathHired ||
            category == FSB_ChatterCategory::botMemberDied ||
            category == FSB_ChatterCategory::botRevivedTarget)
            target = nullptr;

        FSBEvents::ScheduleBotEventWithChatter(bot, FSB_EVENT_HIRED_TIMED_CHATTER_REPLY, 3s, 5s, replyType, replyString, target);

        // we use a dummy timed event to make the target bot do a delayed emote
        // at the same time they are also replying to the initial bot
        // example: target laughing at the bot joke
        if (category == FSB_ChatterCategory::emote_joke && type == FSB_ChatterType::Positive)
        {
            replyString = "emote:laugh";
            FSBEvents::ScheduleBotEventWithChatter(bot, FSB_EVENT_HIRED_TIMED_DUMMY_EMOTE, 3s, 5s, replyType, replyString, target);
        }

        else if ((category == FSB_ChatterCategory::emote_flirt || category == FSB_ChatterCategory::emote_kiss) && type == FSB_ChatterType::Negative)
        {
            replyString = "emote:rudeno";
            FSBEvents::ScheduleBotEventWithChatter(bot, FSB_EVENT_HIRED_TIMED_DUMMY_EMOTE, 3s, 5s, replyType, replyString, target);
        }

        else if (category == FSB_ChatterCategory::emote_oom && type == FSB_ChatterType::None)
        {
            replyString = "emote:oom";
            FSBEvents::ScheduleBotEventWithChatter(bot, FSB_EVENT_HIRED_TIMED_DUMMY_EMOTE, 3s, 5s, replyType, replyString, target);
        }

        else if (category == FSB_ChatterCategory::emote_heal && type == FSB_ChatterType::None)
        {
            replyString = "emote:heal";
            FSBEvents::ScheduleBotEventWithChatter(bot, FSB_EVENT_HIRED_TIMED_DUMMY_EMOTE, 3s, 5s, replyType, replyString, target);
        }

        else if (category == FSB_ChatterCategory::emote_help && type == FSB_ChatterType::None)
        {
            replyString = "emote:help";
            FSBEvents::ScheduleBotEventWithChatter(bot, FSB_EVENT_HIRED_TIMED_DUMMY_EMOTE, 3s, 5s, replyType, replyString, target);
        }
    }

    void DemandBotChatter(Creature* bot, Unit* target, FSB_ChatterCategory category, FSB_ReplyType replyType, FSB_ChatterSource chatterSource, uint32 spellId)
    {
        if (!bot)
            return;

        if (urand(0, 99) > FollowshipBotsConfig::configFSBChatterRate)
            return;

        FSB_ChatterType type = FSB_ChatterType::None;

        if (chatterSource == FSB_ChatterSource::Bot)
            type = FSBMgr::Get()->GetBotChatterTypeForEntry(bot->GetEntry());

        std::string chatter = GetRandomReply(bot, target, category, type, spellId);

        if (chatter.empty())
            return;

        if (replyType == FSB_ReplyType::Say)
            bot->Say(chatter, LANG_UNIVERSAL);

        if (replyType == FSB_ReplyType::Yell)
            bot->Yell(chatter, LANG_UNIVERSAL);

        if (replyType == FSB_ReplyType::Whisper)
        {
            Player* player = FSBMgr::Get()->GetBotOwner(bot);

            if(player)
                bot->Whisper(chatter, LANG_UNIVERSAL, player);
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


