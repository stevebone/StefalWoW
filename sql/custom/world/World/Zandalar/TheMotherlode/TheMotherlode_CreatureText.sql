-- ============================================================
-- The MOTHERLODE!! - Boss Voice Lines (Map 1594)
-- ============================================================
-- creature_text rows consumed by the BossAI scripts via Talk(group).
-- Rows carry BroadcastTextId so the client plays the real localized
-- voice and shows locale text - English base rows only.
--
-- Group layout per boss (matches the scripts' Voice namespaces):
--   129214 Pummeler : 0 aggro, 1 static pulse, 2 footbomb emote,
--                     3 shocking claw, 4 coin magnet, 5 death
--   129227 Azerokk  : 0 aggro, 1 call earthrager, 2 azerite infusion,
--                     3 resonant quake, 4 tectonic smash, 5 slay, 6 death
--   129231 Rixxa    : 0 intro, 1 aggro, 2 azerite catalyst,
--                     3 propellant blast, 4 chemical burn, 5 death
--   129232 Razdunk  : 0 aggro, 1 slay, 2-3 gatling gun, 4 boomba drone,
--                     5/8/9 drill smash calls, 6 drill target emote,
--                     7 stage-two insurance, 10 slay alt,
--                     11 big red rocket calls, 12 death
--
-- Groups 0-10 for 129214/129232 follow the canonical TALK_* layout.
-- Rixxa's repack rows were placeholder stubs and are replaced with the
-- real retail BroadcastText lines here. Razdunk groups 11-12 extend
-- the canonical set with real lines that had no canonical group.
-- ============================================================

DELETE FROM `creature_text` WHERE `CreatureID` IN (129214, 129231, 129232);
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`SoundPlayType`,`BroadcastTextId`,`TextRange`,`comment`) VALUES
-- Coin-Operated Crowd Pummeler (129214)
(129214,0,0,'Venture Company thanks you for your patronage. Please enjoy your purchase of the basic pummeling package.',14,0,100.0,0,0,97390,0,143082,0,'Coin-Operated Crowd Pummeler to Player - TALK_AGGRO'),
(129214,0,1,'Systems engaged. Commence pummeling.',14,0,100.0,0,0,97384,0,143089,0,'Coin-Operated Crowd Pummeler - TALK_AGGRO'),
(129214,1,0,'Voltage increased.',14,0,100.0,0,0,97381,0,143090,0,'Coin-Operated Crowd Pummeler - TALK_STATIC_PULSE'),
(129214,2,0,'|TINTERFACE\\ICONS\\INV_MISC_SOCCERBALL.BLP:20|t Coin-Operated Crowd Pummeler casts |cFFFF0000|Hspell:256214|h[Footbomb Launcher]|h|r!',41,0,100.0,0,0,115344,0,0,0,'Coin-Operated Crowd Pummeler - TALK_FOOTBOMB_LAUNCHER'),
(129214,3,0,'Lethal force authorized.',14,0,100.0,0,0,97380,0,143064,0,'Coin-Operated Crowd Pummeler - TALK_SHOCKING_CLAW'),
(129214,4,0,'Damaging this unit violates the terms of use.',14,0,100.0,0,0,97379,0,143068,0,'Coin-Operated Crowd Pummeler to 129246 - TALK_COIN_MAGNET'),
(129214,5,0,'Systems... failing. Coin release... malfunction...',14,0,100.0,0,0,97385,0,143072,0,'Coin-Operated Crowd Pummeler to Player - TALK_DEATH'),
-- Rixxa Fluxflame (129231) - real retail BroadcastText lines
(129231,0,0,'Hey! Where do you think you\'re going?',14,0,100.0,0,0,97448,0,143169,0,'Rixxa Fluxflame - TALK_INTRO'),
(129231,1,0,'If you want a job done right... use bigger explosives!',14,0,100.0,0,0,97441,0,143171,0,'Rixxa Fluxflame - TALK_AGGRO'),
(129231,1,1,'You\'re the reason my work detail split?',14,0,100.0,0,0,97449,0,143170,0,'Rixxa Fluxflame - TALK_AGGRO'),
(129231,2,0,'Lookin\' for Azerite? Have a face full!',14,0,100.0,0,0,97440,0,143183,0,'Rixxa Fluxflame - TALK_AZERITE_CATALYST'),
(129231,3,0,'I love the smell of propellant in the morning!',14,0,100.0,0,0,99432,0,148698,0,'Rixxa Fluxflame - TALK_PROPELLANT_BLAST'),
(129231,3,1,'Safety first!',14,0,100.0,0,0,99433,0,148699,0,'Rixxa Fluxflame - TALK_PROPELLANT_BLAST'),
(129231,4,0,'Burn! Yes! Burn!',14,0,100.0,0,0,97438,0,143184,0,'Rixxa Fluxflame - TALK_CHEMICAL_BURN'),
(129231,4,1,'You\'re goin\' up in flames!',14,0,100.0,0,0,97439,0,143185,0,'Rixxa Fluxflame - TALK_CHEMICAL_BURN'),
(129231,5,0,'I shoulda... gotten... hazard pay...',14,0,100.0,0,0,97442,0,143174,0,'Rixxa Fluxflame - TALK_DEATH'),
-- Mogul Razdunk (129232)
(129232,0,0,'Do you bums realize how much property damage you\'ve done!?',14,0,100.0,0,0,97410,0,143202,0,'Mogul Razdunk to Player - TALK_AGGRO'),
(129232,0,1,'You\'ll never take me alive!',12,0,100.0,0,0,97408,0,143209,0,'Mogul Razdunk - TALK_AGGRO'),
(129232,1,0,'Right where you belong--under my heel!',14,0,100.0,0,0,97413,0,143212,0,'Mogul Razdunk - TALK_SLAY'),
(129232,1,1,'Eat dirt!',14,0,100.0,0,0,97412,0,143211,0,'Mogul Razdunk - TALK_SLAY'),
(129232,1,2,'I\'ll bill your next of kin for the spent ammunition.',14,0,100.0,0,0,97414,0,143207,0,'Mogul Razdunk - TALK_SLAY'),
(129232,2,0,'Taste some high-caliber carnage!',14,0,100.0,0,0,97405,0,143214,0,'Mogul Razdunk - TALK_GATLING_GUN'),
(129232,3,0,'Get a load of 300 rounds per minute!',14,0,100.0,0,0,97404,0,143213,0,'Mogul Razdunk - TALK_GATLING_GUN'),
(129232,4,0,'What am I payin\' you fools for?! Get out here and fix this!',14,0,100.0,0,0,97409,0,143215,0,'Mogul Razdunk to Mogul Razdunk - TALK_BOOMBA'),
(129232,5,0,'Pulverize!',14,0,100.0,0,0,97415,0,143197,0,'Mogul Razdunk to Drill Smash Target Stalker - TALK_DRILL_SMASH'),
(129232,6,0,'|TINTERFACE\\ICONS\\ABILITY_SIEGE_ENGINEER_SOCKWAVE_MISSILE.BLP:20|t You have been targeted by |cFFFF0000|Hspell:260838|h[Drill Smash]|h|r!',42,0,100.0,0,0,97406,0,0,0,'Mogul Razdunk to Player - EMOTE_DRILL_SMASH'),
(129232,7,0,'Doh! My insurance premiums!',14,0,100.0,0,0,97406,0,143216,0,'Mogul Razdunk to Mogul Razdunk - TALK_STAGE_TWO'),
(129232,8,0,'Crush!',14,0,100.0,0,0,97417,0,143199,0,'Mogul Razdunk to Drill Smash Target Stalker - TALK_DRILL_SMASH'),
(129232,9,0,'Smash!',14,0,100.0,0,0,97416,0,143198,0,'Mogul Razdunk to Drill Smash Target Stalker - TALK_DRILL_SMASH'),
(129232,10,0,'You\'ll pay for that!',14,0,100.0,0,0,97407,0,143218,0,'Mogul Razdunk to Mogul Razdunk - TALK_SLAY'),
(129232,11,0,'Look out below, dirtbags!',14,0,100.0,0,0,97402,0,143066,0,'Mogul Razdunk - TALK_BIG_RED_ROCKET'),
(129232,11,1,'Bombs away!',14,0,100.0,0,0,97403,0,143067,0,'Mogul Razdunk - TALK_BIG_RED_ROCKET'),
(129232,12,0,'Shoulda... sprung... for the... ejector seat package...',14,0,100.0,0,0,97411,0,143208,0,'Mogul Razdunk - TALK_DEATH'),
-- Pummeler extras - real BroadcastText lines recovered from hotfixes
(129214,0,2,'Venture Company thanks you for your patronage. Please enjoy your purchase of the elite pummeling package.',14,0,100.0,0,0,97389,0,143084,0,'Coin-Operated Crowd Pummeler - TALK_AGGRO'),
(129214,6,0,'Another satisfied customer.',14,0,100.0,0,0,97386,0,143061,0,'Coin-Operated Crowd Pummeler - TALK_SLAY'),
(129214,6,1,'Funeral services available at reasonable prices.',14,0,100.0,0,0,97387,0,143062,0,'Coin-Operated Crowd Pummeler - TALK_SLAY');

DELETE FROM `creature_text` WHERE `CreatureID` IN (129227,130435,130485,130488,132338,132713,133345,134012,136135,136470,136688);
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`SoundPlayType`,`BroadcastTextId`,`TextRange`,`comment`) VALUES
-- Azerokk (129227) - real retail lines recovered from broadcast_text;
-- the repack's rows for him were placeholder stubs and are not used.
(129227,0,0,'You trespass in my domain!',14,0,100.0,0,0,97428,0,143106,0,'Azerokk - TALK_AGGRO'),
(129227,0,1,'The world bleeds and you tear at its wounds?! Die!',14,0,100.0,0,0,97433,0,143318,0,'Azerokk - TALK_AGGRO'),
(129227,1,0,'Arise!',14,0,100.0,0,0,97427,0,143129,0,'Azerokk - TALK_CALL_EARTHRAGER'),
(129227,2,0,'Unleash your power!',14,0,100.0,0,0,97423,0,143132,0,'Azerokk - TALK_AZERITE_INFUSION'),
(129227,2,1,'The blood of Azeroth flows through you!',14,0,100.0,0,0,97424,0,143134,0,'Azerokk - TALK_AZERITE_INFUSION'),
(129227,3,0,'Shatter!',14,0,100.0,0,0,97425,0,143135,0,'Azerokk - TALK_RESONANT_QUAKE'),
(129227,4,0,'The ground will consume you!',14,0,100.0,0,0,97426,0,143137,0,'Azerokk - TALK_TECTONIC_SMASH'),
(129227,5,0,'You were nothing but parasites.',14,0,100.0,0,0,97432,0,143110,0,'Azerokk - TALK_SLAY'),
(129227,5,1,'What is your blood worth?',14,0,100.0,0,0,97430,0,143125,0,'Azerokk - TALK_SLAY'),
(129227,5,2,'You can do no further harm.',14,0,100.0,0,0,97431,0,143126,0,'Azerokk - TALK_SLAY'),
(129227,6,0,'The wounds... must be mended...',14,0,100.0,0,0,97429,0,143108,0,'Azerokk - TALK_DEATH'),
-- Trash / ambient NPCs - ambient banter rows; playback needs zone scripts
(130435,0,0,'Cars that drive on water instead of land!',12,0,100.0,6,0,0,0,155005,0,'Addled Thug to Refreshment Vendor'),
(130485,0,0,'It\'s bashin\' time!',12,0,100.0,463,0,0,0,150980,0,'Mechanized Peacekeeper to Mech Jockey'),
(130488,0,0,'Alright, I\'m done fightin\' fair.',12,0,100.0,0,0,0,0,150870,0,'Mech Jockey to Player'),
(130488,1,0,'Bunkboats! We\'ll save so much room in the ocean for more activities. I call top!',12,0,100.0,0,0,0,0,154998,0,'Mech Jockey to Refreshment Vendor'),
(132338,0,0,'|TINTERFACE\\ICONS\\ABILITY_RACIAL_ROCKETBARRAGE.BLP:20|t You are targeted by |cFFFF0000|Hspell:260838|h[Homing Missile]|h|r!',42,0,100.0,0,0,0,0,0,0,'Homing Missile Stalker to Player'),
(132713,0,0,'Shoulda... sprung... for the... ejector seat package...',14,0,100.0,0,0,97411,0,143208,0,'Mogul Razdunk to Player'),
(133345,0,0,'I am outta here!',12,0,100.0,432,0,0,0,147801,0,'Feckless Assistant'),
(134012,0,0,'Crush these interlopers or your hazard pay is mine, whelps!',14,0,100.0,0,0,0,0,152455,0,'Taskmaster Askari to Player'),
(136135,0,0,'Keep that deadbeat outta my game, chums!',12,0,100.0,0,0,0,0,151042,0,'Dice Dealer to Player'),
(136470,0,0,'Bards!',12,0,100.0,0,0,0,0,155004,0,'Refreshment Vendor to Refreshment Vendor'),
(136470,1,0,'Some kind of arcane crystal that makes you better in every way! A bit unstable, maybe, but that\'s ok!',12,0,100.0,0,0,0,0,155001,0,'Refreshment Vendor to Refreshment Vendor'),
(136470,2,0,'You dumb lout! Those were expensive!',12,0,100.0,14,0,0,0,150901,0,'Refreshment Vendor to Player'),
(136688,0,0,'Sweet sweet Azerite!',12,0,100.0,0,0,0,0,152453,0,'Fanatical Driller to Azerite Extractor');
