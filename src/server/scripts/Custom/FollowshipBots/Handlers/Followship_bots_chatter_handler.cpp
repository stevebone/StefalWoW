/*
 * This file is part of the Stefal WoW Project.
 * It is designed to work exclusively with the TrinityCore framework.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * This code is provided for personal and educational use within the
 * Stefal WoW Project. It is not intended for commercial distribution,
 * resale, or any form of monetization.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Log.h"

#include "DB2Stores.h"

#include "Followship_bots_config.h"
#include "Followship_bots_mgr.h"

#include "AI/Followship_bots_ai_base.h"

#include "Followship_bots_chatter_handler.h"
#include "Followship_bots_events_handler.h"

std::unordered_map<uint32, std::vector<FSBChatterDBLine>> FSBChatter::BotChatterLinesMap;
std::vector<FSBEmoteTextEntry> FSBChatter::FSBEmoteTextTable =
{
    {
        FSB_ChatterEmotes::emote_kiss,
        {
            "{bot} blows a kiss to {target}.",
            "{bot} sends a warm kiss toward {target}.",
            "{bot} winks and blows a kiss at {target}.",
        }
    },
    {
        FSB_ChatterEmotes::emote_flirt,
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
        FSB_ChatterEmotes::emote_joke,
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
        FSB_ChatterEmotes::emote_whistle,
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
        FSB_ChatterEmotes::emote_sigh,
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
        FSB_ChatterEmotes::emote_sleep,
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
};

std::vector<FSBChatterReplyEntry> FSBReplyTable =
{
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
        FSB_ChatterCategory::whisper_afk, FSB_ChatterType::Cynical,
        {
            "Of course you're AFK. The universe hates me.",
            "Let me guess. You're 'getting water' for the third time.",
            "Your timing is as bad as your gameplay.",
        }
    },

    {
        FSB_ChatterCategory::whisper_afk, FSB_ChatterType::Bitter,
        {
            "While you lounge, I suffer.",
            "Go ahead. Leave me here to rot.",
            "Some of us actually care about this group.",
        }
    },

    {
        FSB_ChatterCategory::whisper_afk, FSB_ChatterType::Cold,
        {
            "Your absence is noted.",
            "Returning is optional. Survival is not.",
            "I will continue without you.",
        }
    },

    {
        FSB_ChatterCategory::whisper_afk, FSB_ChatterType::Guarded,
        {
            "Are you coming back, or is this abandonment?",
            "I've seen this before. People leave.",
            "If you're gone, just say so.",
        }
    },

    {
        FSB_ChatterCategory::whisper_afk, FSB_ChatterType::Serious,
        {
            "Your presence is required. Promptly.",
            "We have a schedule to maintain.",
            "Time is a resource we cannot recover.",
        }
    },

    {
        FSB_ChatterCategory::whisper_afk, FSB_ChatterType::Dry,
        {
            "I'll just wait here. Forever. No big deal.",
            "Take your time. I've got nowhere to be.",
            "This is fine. I'm fine. Everything's fine.",
        }
    },

    {
        FSB_ChatterCategory::whisper_afk, FSB_ChatterType::Curious,
        {
            "What could be taking this long? I'm genuinely curious.",
            "Is this a technical issue or a biological one?",
            "How does one take a break for this duration?",
        }
    },

    {
        FSB_ChatterCategory::whisper_afk, FSB_ChatterType::Warm,
        {
            "No rush. I'll be right here when you're ready.",
            "Take care of whatever you need. I've got your back.",
            "Rest up. We'll wait for you.",
        }
    },

    {
        FSB_ChatterCategory::whisper_afk, FSB_ChatterType::Enthusiastic,
        {
            "COME BACK SOON! I'M BORED WITHOUT YOU!",
            "HURRY! THE ADVENTURE IS PAUSED AND IT'S KILLING ME!",
            "I MISS YOU ALREADY! DON'T BE GONE LONG!",
        }
    },

    {
        FSB_ChatterCategory::whisper_afk, FSB_ChatterType::Cheerful,
        {
            "Take your time! Good things come to those who wait!",
            "No worries! We'll be here with smiles when you return!",
            "Recharge those batteries! We'll wait happily!",
        }
    },

    {
        FSB_ChatterCategory::whisper_afk, FSB_ChatterType::Devoted,
        {
            "I'll wait for you. However long it takes.",
            "Go do what you need. I'll guard your spot.",
            "My loyalty doesn't time out. Take your time.",
        }
    },

    {
        FSB_ChatterCategory::whisper_afk, FSB_ChatterType::Morbid,
        {
            "Hurry back. The monsters are getting hungry.",
            "While you're gone, I might die of boredom. Literally.",
            "If something eats me, it's your fault.",
        }
    },

    {
        FSB_ChatterCategory::whisper_afk, FSB_ChatterType::Narcissist,
        {
            "You're leaving ME waiting? Do you know who I am?",
            "I hope you realize what an honor my patience is.",
            "Take your time. Not everyone can handle my greatness for long.",
        }
    },

    {
        FSB_ChatterCategory::whisper_afk, FSB_ChatterType::Sadist,
        {
            "I'll be imagining all the ways you could suffer while gone.",
            "I hope you trip on your way back.",
            "Your absence is almost as entertaining as your presence.",
        }
    },

    {
        FSB_ChatterCategory::whisper_afk, FSB_ChatterType::Hyperthymic,
        {
            "WHILE YOU'RE GONE I'LL DO JUMPING JACKS! WOO!",
            "I'LL KEEP THE PARTY GOING! DON'T WORRY!",
            "COME BACK SO WE CAN CONTINUE BEING AWESOME!",
        }
    },

    {
        FSB_ChatterCategory::whisper_afk, FSB_ChatterType::Autotelic,
        {
            "I'll use this time to center myself.",
            "Patience is its own reward. Take your time.",
            "I wait because I choose to, not because I must.",
        }
    },

    {
        FSB_ChatterCategory::whisper_afk, FSB_ChatterType::Autistic,
        {
            "My internal clock says 4 minutes 32 seconds. Threshold approaching.",
            "I have calculated 847 possible reasons for your delay.",
            "The uncertainty is causing a 23% efficiency drop.",
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
        FSB_ChatterCategory::botMemberDied, FSB_ChatterType::Cynical,
        {
            "Of course they died. The universe has a sense of humor.",
            "Another body for the pile. Business as usual.",
            "I told them not to stand in the fire. They never listen.",
        }
    },

    {
        FSB_ChatterCategory::botMemberDied, FSB_ChatterType::Bitter,
        {
            "Good. One less person to disappoint me.",
            "They were dead weight anyway.",
            "Why do I even bother trying to keep people alive?",
        }
    },

    {
        FSB_ChatterCategory::botMemberDied, FSB_ChatterType::Cold,
        {
            "Casualty confirmed.",
            "Combat efficiency reduced by one unit.",
            "Replacements should be considered.",
        }
    },

    {
        FSB_ChatterCategory::botMemberDied, FSB_ChatterType::Guarded,
        {
            "I knew this would happen. I always know.",
            "Trusting teammates is a mistake I keep making.",
            "Are we next? Because it feels like we're next.",
        }
    },

    {
        FSB_ChatterCategory::botMemberDied, FSB_ChatterType::Serious,
        {
            "Casualty report: one down. Adjust tactics.",
            "We continue the mission regardless.",
            "Mourn later. Fight now.",
        }
    },

    {
        FSB_ChatterCategory::botMemberDied, FSB_ChatterType::Dry,
        {
            "Well, that's inconvenient.",
            "Down one. The odds were never in our favor.",
            "And here I thought we'd make it through unscathed.",
        }
    },

    {
        FSB_ChatterCategory::botMemberDied, FSB_ChatterType::Curious,
        {
            "What killed them? I need to analyze the threat.",
            "Was it mechanical failure or tactical error?",
            "The data on their death could save the rest of us.",
        }
    },

    {
        FSB_ChatterCategory::botMemberDied, FSB_ChatterType::Warm,
        {
            "We'll get you back up, {target}. I promise.",
            "Don't worry, we've got you. Just hang in there.",
            "We're a team. We don't leave anyone behind.",
        }
    },

    {
        FSB_ChatterCategory::botMemberDied, FSB_ChatterType::Enthusiastic,
        {
            "NOOOO! {TARGET}! I'LL AVENGE YOU!",
            "DON'T WORRY! I'LL RES YOU! I GOT THIS!",
            "WE'RE COMING FOR YOU! HOLD ON!",
        }
    },

    {
        FSB_ChatterCategory::botMemberDied, FSB_ChatterType::Cheerful,
        {
            "Oh no! But we'll fix this! I believe in us!",
            "Don't worry {target}, we'll have you back in no time!",
            "Every setback is just a setup for a comeback!",
        }
    },

    {
        FSB_ChatterCategory::botMemberDied, FSB_ChatterType::Devoted,
        {
            "I would die before I let you stay down, {target}.",
            "Your life is worth more than mine. I'll save you.",
            "Nothing will stop me from bringing you back.",
        }
    },

    {
        FSB_ChatterCategory::botMemberDied, FSB_ChatterType::Morbid,
        {
            "The corpse looks peaceful. Almost jealous.",
            "Death suits them. It suits us all eventually.",
            "I'll add their bones to my collection.",
        }
    },

    {
        FSB_ChatterCategory::botMemberDied, FSB_ChatterType::Narcissist,
        {
            "They died because they didn't have MY skills.",
            "If they'd listened to me, they'd still be alive.",
            "This is why I'm the most important person here.",
        }
    },

    {
        FSB_ChatterCategory::botMemberDied, FSB_ChatterType::Sadist,
        {
            "Watching them fall was... satisfying.",
            "Their screams were music to my ears.",
            "I hope they suffer on the corpse run too.",
        }
    },

    {
        FSB_ChatterCategory::botMemberDied, FSB_ChatterType::Hyperthymic,
        {
            "DON'T WORRY! WE'LL MAKE THIS EPIC!",
            "THEY'LL BE BACK AND IT'LL BE EVEN BETTER!",
            "EVERY DEATH IS JUST A PLOT TWIST! WOO!",
        }
    },

    {
        FSB_ChatterCategory::botMemberDied, FSB_ChatterType::Autotelic,
        {
            "I fight for them because I choose to. Not because I must.",
            "Their survival matters to me. That's enough.",
            "I'll save them because connection demands it.",
        }
    },

    {
        FSB_ChatterCategory::botMemberDied, FSB_ChatterType::Autistic,
        {
            "Their health bar reached zero in 2.3 seconds. Fascinating.",
            "I have identified 6 errors in their positioning.",
            "The death was preventable. I will now calculate how.",
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
        FSB_ChatterCategory::botRevived, FSB_ChatterType::Cynical,
        {
            "Back from the dead. How... predictable.",
            "Death couldn't keep me. Neither can this group.",
            "I'm alive again. For whatever that's worth.",
        }
    },

    {
        FSB_ChatterCategory::botRevived, FSB_ChatterType::Bitter,
        {
            "I didn't ask to be brought back.",
            "The afterlife was quieter than this mess.",
            "Why do people keep saving me? I don't deserve it.",
        }
    },

    {
        FSB_ChatterCategory::botRevived, FSB_ChatterType::Cold,
        {
            "Revival successful. Systems online.",
            "Returned to operational status.",
            "Corpse retrieval complete. Resuming function.",
        }
    },

    {
        FSB_ChatterCategory::botRevived, FSB_ChatterType::Guarded,
        {
            "Who revived me? And why?",
            "I don't trust resurrections. They always have a price.",
            "Back among the living. For now.",
        }
    },

    {
        FSB_ChatterCategory::botRevived, FSB_ChatterType::Serious,
        {
            "I'm back. Let's not waste this second chance.",
            "Revival complete. Mission resumes.",
            "Death is a setback, not an endpoint.",
        }
    },

    {
        FSB_ChatterCategory::botRevived, FSB_ChatterType::Dry,
        {
            "Well, that was a nice nap.",
            "Back again. Like a bad penny.",
            "I'm alive. The universe must be disappointed.",
        }
    },

    {
        FSB_ChatterCategory::botRevived, FSB_ChatterType::Curious,
        {
            "What did I miss? I was dead for 3 minutes.",
            "The afterlife was... disappointing. Tell me everything.",
            "Did anyone loot my corpse? I need an inventory check.",
        }
    },

    {
        FSB_ChatterCategory::botRevived, FSB_ChatterType::Warm,
        {
            "Thank you for bringing me back. I mean it.",
            "Your faith in me means more than you know.",
            "I'm back because of you. I'll make it count.",
        }
    },

    {
        FSB_ChatterCategory::botRevived, FSB_ChatterType::Enthusiastic,
        {
            "I'M BACK! DID YOU MISS ME? I MISSED ME!",
            "ROUND TWO! LET'S DO THIS!",
            "DEATH WAS BORING! REALITY IS WHERE THE PARTY IS!",
        }
    },

    {
        FSB_ChatterCategory::botRevived, FSB_ChatterType::Cheerful,
        {
            "Hooray! I'm back! Let's make this life count!",
            "Second chances are the best! I love you all!",
            "The Light brought me back and I'm SO grateful!",
        }
    },

    {
        FSB_ChatterCategory::botRevived, FSB_ChatterType::Devoted,
        {
            "I came back for you. Only you.",
            "Your love is stronger than death. I felt it.",
            "I couldn't stay away from you. Even in death.",
        }
    },

    {
        FSB_ChatterCategory::botRevived, FSB_ChatterType::Morbid,
        {
            "Death didn't want me either. Embarrassing.",
            "I saw the other side. It was... beautiful.",
            "Next time, leave me with the dead. They understand me.",
        }
    },

    {
        FSB_ChatterCategory::botRevived, FSB_ChatterType::Narcissist,
        {
            "Even death couldn't handle my greatness.",
            "The spirit healer begged me to come back.",
            "I'm too magnificent for the afterlife anyway.",
        }
    },

    {
        FSB_ChatterCategory::botRevived, FSB_ChatterType::Sadist,
        {
            "I'm back. Time to make others suffer in my place.",
            "Your healing feels good. For me, not you.",
            "Death was too kind. I prefer causing it.",
        }
    },

    {
        FSB_ChatterCategory::botRevived, FSB_ChatterType::Hyperthymic,
        {
            "I DIED AND CAME BACK AND IT WAS AWESOME!",
            "BEST RESURRECTION EVER! 10/10 WOULD DIE AGAIN!",
            "I'M LIKE A PHOENIX BUT LOUDER!",
        }
    },

    {
        FSB_ChatterCategory::botRevived, FSB_ChatterType::Autotelic,
        {
            "I return because I choose to. Not because I was called.",
            "Life is its own reward. I'm here because I want to be.",
            "Death taught me nothing. I taught myself to return.",
        }
    },

    {
        FSB_ChatterCategory::botRevived, FSB_ChatterType::Autistic,
        {
            "Revival sequence completed. Vital signs restored.",
            "The spirit healer used spell ID 2008. I noted it.",
            "I was dead for 182 seconds. It felt like 3.",
        }
    },

    {
        FSB_ChatterCategory::botRevivedSelf, FSB_ChatterType::Positive,
        {
            "The spirits weren't done with me yet!",
            "Did you see that? I brought myself back!",
            "My own power prevails once more!",
            "I am the master of my own fate!",
            "Rise and shine, courtesy of me.",
            "Now THAT is how you make an entrance!",
            "My soulstone did exactly what I paid for.",
            "The ancestors clearly want me around.",
            "Self-revived and feeling unstoppable!",
            "Death tried to keep me. It failed.",
            "I have returned, and I am magnificent!",
            "No healer required when you have MY skill.",
            "Behold, the power of preparation!",
            "I am living proof that planning pays off.",
            "Resurrection? I prefer to call it a party trick.",
            "The spirit healer can take the day off.",
            "I am my own best healer.",
            "They will write songs about this comeback.",
            "My power over death remains unbroken.",
            "Back by popular demand... my own demand."
        }
    },

    {
        FSB_ChatterCategory::botRevivedSelf, FSB_ChatterType::Neutral,
        {
            "Self-resurrection complete. Status: operational.",
            "Revived. No external assistance required.",
            "Soulstone activated. Revival successful.",
            "Reincarnation utilized. Resuming function.",
            "Self-revive executed as planned.",
            "Back online. Soulstone charge consumed.",
            "Resurrection protocol completed.",
            "Operational status restored via self-revive.",
            "Revival successful. Continuing mission.",
            "Self-resurrection matrix engaged and complete.",
            "Back among the living. Efficiency noted.",
            "Soulstone expenditure: one. Result: revived.",
            "Reincarnation timer reset. Proceeding.",
            "Status: alive. Method: internal.",
            "Revive complete. Inventory: one less soulstone.",
            "Functionality restored. No third-party intervention.",
            "Resurrection cycle complete. Awaiting directives.",
            "Self-revive logged. Performance: acceptable.",
            "Return to operational status confirmed.",
            "Revived. Soulstone utility: confirmed."
        }
    },

    {
        FSB_ChatterCategory::botRevivedSelf, FSB_ChatterType::Negative,
        {
            "That's right, I brought MYSELF back. You're welcome.",
            "Who needs a healer when you have ME?",
            "Death couldn't hold me. Shocking, I know.",
            "I literally just saved my own life. Again.",
            "No one else was going to do it, so I did.",
            "I'm basically immortal at this point.",
            "You're lucky I came prepared. Unlike SOME people.",
            "My soulstone is the only reliable thing here.",
            "I don't need your pity. I need your awe.",
            "The afterlife was boring anyway.",
            "I came back because no one else deserves to be this good.",
            "Self-resurrection: because I'm worth it.",
            "Did you doubt me? Of course you did.",
            "I'm my own best investment.",
            "Some call it a soulstone. I call it Tuesday.",
            "I returned because the world needs my greatness.",
            "I don't wait for resurrections. I MAKE them.",
            "Your corpse runs are cute. I prefer instant.",
            "I'm basically cheating death at this point.",
            "They really should put my face on the soulstone."
        }
    },

    {
        FSB_ChatterCategory::botRevivedSelf, FSB_ChatterType::Cynical,
        {
            "I revived myself. Because obviously no one else would bother.",
            "My soulstone triggers automatically. No faith required.",
            "Back from the dead, powered by spite and preparation.",
        }
    },

    {
        FSB_ChatterCategory::botRevivedSelf, FSB_ChatterType::Bitter,
        {
            "I saved myself because no one else would.",
            "My soulstone is my only real friend.",
            "At least MY resurrection doesn't require a team.",
        }
    },

    {
        FSB_ChatterCategory::botRevivedSelf, FSB_ChatterType::Cold,
        {
            "Self-revive protocol executed successfully.",
            "Autonomous resurrection complete. No external input required.",
            "I require no assistance to return to function.",
        }
    },

    {
        FSB_ChatterCategory::botRevivedSelf, FSB_ChatterType::Guarded,
        {
            "I came back, but I don't trust this group anymore.",
            "My soulstone is the only reliable thing here.",
            "I revive myself because depending on others is fatal.",
        }
    },

    {
        FSB_ChatterCategory::botRevivedSelf, FSB_ChatterType::Serious,
        {
            "Self-resurrection complete. I remain combat-ready.",
            "I do not require a healer to maintain operational status.",
            "Preparedness is the difference between life and permanent death.",
        }
    },

    {
        FSB_ChatterCategory::botRevivedSelf, FSB_ChatterType::Dry,
        {
            "I resurrected myself. Standard Tuesday.",
            "Soulstone: used. Dignity: intact. Barely.",
            "I'm my own healer. And my own biggest fan.",
        }
    },

    {
        FSB_ChatterCategory::botRevivedSelf, FSB_ChatterType::Curious,
        {
            "I wonder what the theoretical limit on self-resurrections is.",
            "The soulstone mechanism is fascinating. I should document it.",
            "Did anyone else see me come back? I want feedback.",
        }
    },

    {
        FSB_ChatterCategory::botRevivedSelf, FSB_ChatterType::Warm,
        {
            "I'm back! I knew I could count on myself, and on you.",
            "My soulstone worked, but your support means everything.",
            "I came back because we still have so much to do together.",
        }
    },

    {
        FSB_ChatterCategory::botRevivedSelf, FSB_ChatterType::Enthusiastic,
        {
            "I BROUGHT MYSELF BACK! I'M A WIZARD! A WARLOCK! AMAZING!",
            "DID YOU SEE THAT? I DID THAT! ME! SOLO!",
            "I AM UNSTOPPABLE! SELF-REZ KING/QUEEN!",
        }
    },

    {
        FSB_ChatterCategory::botRevivedSelf, FSB_ChatterType::Cheerful,
        {
            "Hooray! I saved myself! Teamwork starts with self-work!",
            "My soulstone and I are best friends! We're back!",
            "I knew I could do it! Positive thinking works!",
        }
    },

    {
        FSB_ChatterCategory::botRevivedSelf, FSB_ChatterType::Devoted,
        {
            "I came back because I couldn't bear to leave you alone.",
            "My soulstone triggers, but my heart triggers it.",
            "I will always find my way back to you. Always.",
        }
    },

    {
        FSB_ChatterCategory::botRevivedSelf, FSB_ChatterType::Morbid,
        {
            "Even death rejects me now. How sad.",
            "My soulstone is basically a leash. I can't escape.",
            "I tried to stay dead. My magic said no.",
        }
    },

    {
        FSB_ChatterCategory::botRevivedSelf, FSB_ChatterType::Narcissist,
        {
            "Of course I brought myself back. I'm too good to die.",
            "My soulstone knows better than to let perfection perish.",
            "I'm my own favorite hero. Obviously.",
        }
    },

    {
        FSB_ChatterCategory::botRevivedSelf, FSB_ChatterType::Sadist,
        {
            "I came back to watch the rest of you fall.",
            "My resurrection was perfect. Unlike your gameplay.",
            "I revive myself because I enjoy watching you struggle without me.",
        }
    },

    {
        FSB_ChatterCategory::botRevivedSelf, FSB_ChatterType::Hyperthymic,
        {
            "I DIED AND SAID 'NO THANKS' AND CAME BACK! WOO!",
            "SELF-RES IS THE BEST RES! CHANGE MY MIND!",
            "I'M BACK AND I'M BETTER THAN EVER!",
        }
    },

    {
        FSB_ChatterCategory::botRevivedSelf, FSB_ChatterType::Autotelic,
        {
            "I revived myself because I chose to continue.",
            "My soulstone is a tool. My will is the reason.",
            "I return for myself, and that is enough.",
        }
    },

    {
        FSB_ChatterCategory::botRevivedSelf, FSB_ChatterType::Autistic,
        {
            "Soulstone cooldown: 30 minutes. I tracked it precisely.",
            "Self-resurrection is more efficient than group dependency.",
            "My reagent inventory is organized by resurrection priority.",
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
            "At least I have my {spell}!",
            "There's no {spell} like mine!",
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
    static std::vector<FSBChatterDBLine const*> FilterChatterCandidates(Creature* bot, std::vector<FSBChatterDBLine> const& lines)
    {
        std::vector<FSBChatterDBLine const*> candidates;
        if (!bot)
            return candidates;

        uint32 botZoneId = bot->GetZoneId();
        uint8 botRaceId  = static_cast<uint8>(FSBMgr::Get()->GetBotRaceForEntry(bot->GetEntry()));
        uint8 botClassId = static_cast<uint8>(FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry()));

        for (auto const& line : lines)
        {
            if ((line.zoneId == 0 || line.zoneId == static_cast<int32>(botZoneId)) &&
                (line.fsbRaceId == 0 || line.fsbRaceId == botRaceId) &&
                (line.fsbClassId == 0 || line.fsbClassId == botClassId))
            {
                candidates.push_back(&line);
            }
        }
        return candidates;
    }

    std::string GetRandomEmoteText(Creature* bot, Unit* target, FSB_ChatterEmotes emote)
    {
        for (auto const& entry : FSBEmoteTextTable)
        {
            if (entry.emote == emote)
            {
                if (entry.lines.empty())
                {
                    TC_LOG_ERROR("scripts.fsb.chatter", "FSB: GetRandomEmoteText: No lines found for emote {}", static_cast<uint8>(emote));
                    return "";
                }

                std::string line = entry.lines[urand(0, entry.lines.size() - 1)];

                if (bot)
                    ReplaceAll(line, "{bot}", bot->GetName());
                if (target)
                    ReplaceAll(line, "{target}", target->GetName());

                TC_LOG_DEBUG("scripts.fsb.chatter", "FSB: GetRandomEmoteText: String [{}] selected for emote {}", line, static_cast<uint8>(emote));
                return line;
            }
        }

        TC_LOG_ERROR("scripts.fsb.chatter", "FSB: GetRandomEmoteText: Emote {} not found in FSBEmoteTextTable", static_cast<uint8>(emote));
        return "";
    }

    std::string GetRandomReply(Creature* bot, Unit* target, FSB_ChatterCategory category, FSB_ChatterType chatterType, uint32 spellId, uint8 duration)
    {
        Player* player = FSBMgr::Get()->GetBotOwner(bot);

        // --- DB-driven path (emote_kiss, emote_flirt, emote_joke, emote_talk, emote_cooking) ---
        if (category == FSB_ChatterCategory::emote_kiss ||
            category == FSB_ChatterCategory::emote_flirt ||
            category == FSB_ChatterCategory::emote_joke ||
            category == FSB_ChatterCategory::emote_talk ||
            category == FSB_ChatterCategory::emote_cooking)
        {
            uint32 key = (static_cast<uint32>(category) << 8) | static_cast<uint32>(chatterType);
            auto it = BotChatterLinesMap.find(key);
            if (it == BotChatterLinesMap.end() || it->second.empty())
            {
                TC_LOG_ERROR("scripts.fsb.chatter", "FSB: No DB chatter lines found for category {} key {}", static_cast<uint8>(category), key);
                return "";
            }

            std::vector<FSBChatterDBLine const*> candidates = FilterChatterCandidates(bot, it->second);
            if (candidates.empty())
            {
                TC_LOG_ERROR("scripts.fsb.chatter", "FSB: DB lines exist for category {} key {} but none match bot zone/race/class filters", static_cast<uint8>(category), key);
                return "";
            }

            FSBChatterDBLine const* chosen = candidates[urand(0, candidates.size() - 1)];
            std::string line = chosen->lineText;

            // Token replacement
            if (bot)
                ReplaceAll(line, "{bot}", bot->GetName());
            if (target)
                ReplaceAll(line, "{target}", target->GetName());
            if (player)
                ReplaceAll(line, "{player}", player->GetName());
            if (spellId)
                ReplaceAll(line, "{spell}", FSBSpellsUtils::GetSpellName(spellId));
            if (duration == 1)
                ReplaceAll(line, "{duration}", "1 hour");
            if (duration > 1)
                ReplaceAll(line, "{duration}", std::to_string(duration) + " hours");

            TC_LOG_DEBUG("scripts.fsb.chatter", "FSB: Chatter GetRandomReply (DB): String [{}] selected for emote_kiss key {}", line, key);
            return line;
        }

        // --- Legacy static-table path (all other categories) ---
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

    std::string GetDummyEmoteString(FSB_ChatterCategory category, FSB_ChatterType type)
    {
        if (category == FSB_ChatterCategory::emote_joke && type == FSB_ChatterType::Positive)
            return "emote:laugh";
        if ((category == FSB_ChatterCategory::emote_flirt || category == FSB_ChatterCategory::emote_kiss) && type == FSB_ChatterType::Negative)
            return "emote:rudeno";
        if (category == FSB_ChatterCategory::emote_oom && type == FSB_ChatterType::None)
            return "emote:oom";
        if (category == FSB_ChatterCategory::emote_heal && type == FSB_ChatterType::None)
            return "emote:heal";
        if (category == FSB_ChatterCategory::emote_help && type == FSB_ChatterType::None)
            return "emote:help";
        return "";
    }

    void PlayDummyEmote(Creature* bot, const std::string& dummyEmoteString)
    {
        if (!bot || dummyEmoteString.empty())
            return;

        auto* ai = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!ai)
            return;

        auto botRace = ai->botRace;
        auto botGender = FSBMgr::Get()->GetBotGenderForEntry(bot->GetEntry());

        TextEmotes tEmote = TEXT_EMOTE_AGREE;
        Emote emote = EMOTE_ONESHOT_NONE;

        if (dummyEmoteString == "emote:laugh")
        {
            tEmote = TEXT_EMOTE_LAUGH;
            emote = EMOTE_ONESHOT_LAUGH;
        }
        else if (dummyEmoteString == "emote:rudeno")
        {
            tEmote = RAND(TEXT_EMOTE_NO, TEXT_EMOTE_RASP);
            emote = (tEmote == TEXT_EMOTE_NO) ? EMOTE_ONESHOT_NO : EMOTE_ONESHOT_RUDE;
        }
        else if (dummyEmoteString == "emote:oom")
            tEmote = TEXT_EMOTE_OOM;
        else if (dummyEmoteString == "emote:heal")
            tEmote = TEXT_EMOTE_HEALME;
        else if (dummyEmoteString == "emote:help")
            tEmote = TEXT_EMOTE_HELPME;

        auto soundInfo = sDB2Manager.GetTextSoundEmoteFor(tEmote, FSBUtils::BotRaceToTC(botRace), botGender, 0);
        if (soundInfo)
            bot->PlayDistanceSound(soundInfo->SoundID);

        bot->HandleEmoteCommand(emote);
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

        std::string dummyEmote = GetDummyEmoteString(category, type);
        if (!dummyEmote.empty())
            FSBEvents::ScheduleBotEventWithChatter(bot, FSB_EVENT_HIRED_TIMED_DUMMY_EMOTE, 3s, 5s, replyType, dummyEmote, target);
    }

    void DemandTargetReply(Creature* bot, Creature* target, FSB_ChatterCategory category, FSB_ReplyType replyType)
    {
        if (!bot)
            return;

        if (!bot->IsAlive())
            return;

        if (!target || !target->IsAlive())
            return;

        FSB_ChatterType type = FSBMgr::Get()->GetBotChatterTypeForEntry(target->GetEntry());

        std::string replyString = GetRandomReply(bot, target, category, type, 0);

        TC_LOG_DEBUG("scripts.fsb.chatter", "FSB: Chatter DemandTargetReply: Bot {} Target {} String {} selected for category {} and chatterType {}", bot->GetName(), target->GetName(), replyString, category, type);

        FSBEvents::ScheduleBotEventWithChatter(bot, FSB_EVENT_HIRED_TIMED_CHATTER_REPLY, 3s, 5s, replyType, replyString, target);

        std::string dummyEmote = GetDummyEmoteString(category, type);
        if (!dummyEmote.empty())
            FSBEvents::ScheduleBotEventWithChatter(bot, FSB_EVENT_HIRED_TIMED_DUMMY_EMOTE, 3s, 5s, replyType, dummyEmote, target);
    }

    void DemandBotChatter(Creature* bot, Unit* target, FSB_ChatterCategory category, FSB_ReplyType replyType, FSB_ChatterSource chatterSource, uint32 spellId, uint16 duration)
    {
        if (!bot)
            return;

        if (urand(0, 99) > FollowshipBotsConfig::configFSBChatterRate)
            return;

        FSB_ChatterType type = FSB_ChatterType::None;

        if (chatterSource == FSB_ChatterSource::Bot)
            type = FSBMgr::Get()->GetBotChatterTypeForEntry(bot->GetEntry());

        std::string chatter = GetRandomReply(bot, target, category, type, spellId, duration);

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


