-- Mardum Creature Text
DELETE FROM `creature_text` WHERE `creatureID` IN (98484,98486,98497,98460,95226,93112,96884,93759,94654,93221,95048,93716,97034,96494,102726,96499,
102724,97706,97059);
DELETE FROM `creature_text` WHERE `creatureID` IN (98229) AND `GroupID` IN (2,3);
DELETE FROM `creature_text` WHERE `creatureID` IN (93127) AND `GroupID` IN (2);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
(97059, 0, 0, 'Want your little, broken draenei back?', 12, 0, 100, 0, 0, 55281, 99173, 0, 'King Voras to Player'),
(97059, 1, 0, 'My queen''s brood will hatch soon.', 14, 0, 100, 0, 0, 55283, 103500, 0, 'King Voras to Player'),
(97059, 2, 0, '|TInterface\Icons\inv_misc_monsterspidercarapace_01:20|tNearby |cFFFF0000|Hspell:198235|h[Spider Eggs]|h|r will hatch soon if not destroyed!.', 41, 0, 100, 0, 0, 0, 103110, 0, 'King Voras to Player'),
(97059, 3, 0, 'They''ve slain me, my queen...', 14, 0, 100, 0, 0, 55282, 99206, 0, 'King Voras to Player'),

(97706, 0, 0, 'Warn King Voras!', 12, 0, 100, 0, 0, 55175, 103526, 0, 'Fel Weaver to Player'),
(97706, 0, 1, 'Quickly, kill the demon hunter!', 12, 0, 100, 0, 0, 55185, 103528, 0, 'Fel Weaver to Player'),
(97706, 0, 2, 'Come closer, my little friend.', 12, 0, 100, 0, 0, 55178, 103519, 0, 'Fel Weaver to Player'),
(97706, 0, 3, 'Embrace the inevitable.', 12, 0, 100, 0, 0, 55179, 103520, 0, 'Fel Weaver to Player'),
(97706, 0, 4, 'All worlds fall to the Legion.', 12, 0, 100, 0, 0, 55180, 103521, 0, 'Fel Weaver to Player'),

(102724, 0, 0, 'Kill the Illidari', 12, 0, 100, 0, 0, 56992, 96720, 0, 'Vile Soulmaster to Player'),
(102724, 0, 1, 'Your soul will be ours.', 12, 0, 100, 0, 0, 57001, 99614, 0, 'Vile Soulmaster to Player'),
(102724, 0, 2, 'Kill them before they can get up to the Fel Hammer.', 12, 0, 100, 0, 0, 57002, 99615, 0, 'Vile Soulmaster to Player'),
(102724, 0, 3, 'Defend the Soul Engine.', 12, 0, 100, 0, 0, 57003, 99616, 0, 'Vile Soulmaster to Player'),
(102724, 0, 4, 'Your death will be swift and unmerciful.', 12, 0, 100, 0, 0, 57004, 102121, 0, 'Vile Soulmaster to Player'),
(102724, 0, 5, 'Your world will be purged!', 12, 0, 100, 0, 0, 56994, 96722, 0, 'Vile Soulmaster to Player'),
(102724, 1, 0, 'In Sargeras''s name.', 12, 0, 100, 0, 0, 56997, 96725, 0, 'Vile Soulmaster to Player'),

(96499, 0, 0, 'The fel lord is just ahead.', 12, 0, 100, 0, 0, 55237, 98272, 0, 'Jace Darkweaver to Player'),

(102726, 0, 0, 'They have the spectral sight!', 12, 0, 100, 0, 0, 56999, 98223, 0, 'Eredar Sorcerer to Jace Darkweaver'),

(93127, 2, 0, '$n, you made it through!', 14, 0, 100, 0, 0, 55054, 99836, 0, 'Kayn Sunfury to Player'),

(93716, 0, 0, 'The Legion conquers all.', 12, 0, 100, 0, 0, 55147, 96648, 0, 'Doom Slayer to Player'),
(93716, 0, 1, 'Succulent marrow. Crunchy bones.', 12, 0, 100, 0, 0, 55150, 96651, 0, 'Doom Slayer to Player'),

(95048, 0, 0, 'Deal with these insects, Beliash.', 12, 0, 100, 0, 0, 55071, 96146, 0, 'Brood Queen Tyranna to Player'),

(93221, 0, 0, 'They will die.', 14, 0, 100, 0, 0, 55133, 94990, 0, 'Doom Commander Beliash to Player'),
(93221, 1, 0, 'You won''t survive Inferno Peak...', 12, 0, 100, 0, 0, 55134, 94991, 0, 'Doom Commander Beliash to Player'),

(94654, 0, 0, 'A fatal mistake, mortal.', 12, 0, 100, 0, 0, 55149, 96650, 0, 'Doomguard Eradicator to Player'),
(94654, 0, 1, 'The Legion conquers all.', 12, 0, 100, 0, 0, 55147, 96648, 0, 'Doomguard Eradicator to Player'),
(94654, 0, 2, 'Intruder, your life ends now.', 12, 0, 100, 0, 0, 55146, 96647, 0, 'Doomguard Eradicator to Player'),
(94654, 0, 3, 'Doom!', 12, 0, 100, 0, 0, 55142, 96645, 0, 'Doomguard Eradicator to Player'),
(94654, 0, 4, 'Succulent marrow. Crunchy bones.', 12, 0, 100, 0, 0, 55150, 96651, 0, 'Doomguard Eradicator to Player'),
(94654, 0, 5, 'You do not belong here.', 12, 0, 100, 0, 0, 55148, 96649, 0, 'Doomguard Eradicator to Player'),

(93759, 0, 0, 'I sense greater power within you, $n. Have you stolen a demon''s essence?', 12, 0, 100, 0, 0, 55229, 96428, 0, 'Jace Darkweaver to Player'),
(93759, 1, 0, 'Use the crucible to complete the ritual.', 12, 0, 100, 0, 0, 55242, 96680, 0, 'Jace Darkweaver to Player'),
(93759, 2, 0, 'Beliash is protected by those Spires of Woe. You''ll want to deactivate them.', 12, 0, 100, 0, 0, 55230, 96689, 0, 'Jace Darkweaver to Player'),
(93759, 3, 0, 'Good luck, $n. I''ll see you up in the volcano.', 12, 0, 100, 0, 0, 55232, 101307, 0, 'Jace Darkweaver to Player'),

(96884, 0, 0, 'This demon will die.', 12, 0, 100, 0, 0, 55082, 99743, 0, 'Coilskar Sea-Caller to Player'),
(96884, 1, 0, 'Lady S''theno requested I join you.', 12, 0, 100, 0, 0, 55077, 99738, 0, 'Coilskar Sea-Caller to Player'),
(96884, 1, 1, 'The Coilskar honor their allegiance.', 12, 0, 100, 0, 0, 55084, 99745, 0, 'Coilskar Sea-Caller to Player'),
(96884, 1, 2, 'Lord Illidan leads and the Coilskar follow.', 12, 0, 100, 0, 0, 55081, 99742, 0, 'Coilskar Sea-Caller to Player'),
(96884, 1, 3, 'The Burning Legion will be destroyed.', 12, 0, 100, 0, 0, 55079, 99740, 0, 'Coilskar Sea-Caller to Player'),

(93112, 0, 0, 'For the Legion!', 12, 0, 100, 0, 0, 55192, 96663, 0, 'Felguard Sentry to Player'),
(93112, 0, 1, 'Demon hunters? How did you get here?', 12, 0, 100, 0, 0, 55187, 94934, 0, 'Felguard Sentry to Player'),
(93112, 0, 2, 'Die, Illidari fool.', 12, 0, 100, 0, 0, 55189, 94936, 0, 'Felguard Sentry to Player'),
(93112, 0, 3, 'I''ll rend you limb from limb.', 12, 0, 100, 0, 0, 55188, 94935, 0, 'Felguard Sentry to Demon Hunter'),
(93112, 0, 4, 'Invaders. Warn the Brood Queen!', 12, 0, 100, 0, 0, 55186, 94932, 0, 'Felguard Sentry to Demon Hunter'),
(93112, 0, 5, 'The fel you wield will not be enough.', 12, 0, 100, 0, 0, 55190, 96661, 0, 'Felguard Sentry to Demon Hunter'),
(93112, 0, 6, 'For Brood Queen Tyranna. For the Legion!', 12, 0, 100, 0, 0, 55193, 96664, 0, 'Felguard Sentry to Demon Hunter'),
(93112, 0, 7, 'You dare attack us here?!', 12, 0, 100, 0, 0, 55191, 96662, 0, 'Felguard Sentry to Demon Hunter'),

(96494, 0, 0, 'For the Legion!', 12, 0, 100, 0, 0, 55192, 96663, 0, 'Felguard Butcher to Player'),
(96494, 0, 1, 'Demon hunters? How did you get here?', 12, 0, 100, 0, 0, 55187, 94934, 0, 'Felguard Butcher to Player'),
(96494, 0, 2, 'Die, Illidari fool.', 12, 0, 100, 0, 0, 55189, 94936, 0, 'Felguard Butcher to Player'),
(96494, 0, 3, 'I''ll rend you limb from limb.', 12, 0, 100, 0, 0, 55188, 94935, 0, 'Felguard Butcher to Demon Hunter'),
(96494, 0, 4, 'Invaders. Warn the Brood Queen!', 12, 0, 100, 0, 0, 55186, 94932, 0, 'Felguard Butcher to Demon Hunter'),
(96494, 0, 5, 'The fel you wield will not be enough.', 12, 0, 100, 0, 0, 55190, 96661, 0, 'Felguard Butcher to Demon Hunter'),
(96494, 0, 6, 'For Brood Queen Tyranna. For the Legion!', 12, 0, 100, 0, 0, 55193, 96664, 0, 'Felguard Butcher to Demon Hunter'),
(96494, 0, 7, 'You dare attack us here?!', 12, 0, 100, 0, 0, 55191, 96662, 0, 'Felguard Butcher to Demon Hunter'),

(95226, 0, 0, 'So eager to be enslaved.', 12, 0, 100, 0, 0, 55018, 97913, 0, 'Anguish Jailer to Player'),
(95226, 0, 1, 'In this place, you are the hunted.', 12, 0, 100, 0, 0, 55019, 97914, 0, 'Anguish Jailer to Player'),
(95226, 0, 2, 'I am your judge, jury, and executioner.', 12, 0, 100, 0, 0, 55020, 97915, 0, 'Anguish Jailer to Player'),
(95226, 0, 3, 'Into my cage you go.', 12, 0, 100, 0, 0, 55017, 97912, 0, 'Anguish Jailer to Player'),
(95226, 0, 4, 'I''ll crack open your flesh and feed upon your soul.', 12, 0, 100, 0, 0, 55016, 97911, 0, 'Anguish Jailer to Player'),
(95226, 0, 5, 'A new prisoner for the taking.', 12, 0, 100, 0, 0, 55014, 97909, 0, 'Anguish Jailer to Player'),
(95226, 0, 6, 'Your soul will be mine.', 12, 0, 100, 0, 0, 55013, 97908, 0, 'Anguish Jailer to Player'),
(95226, 0, 7, 'Your armies are nothing, demon hunter.', 12, 0, 100, 0, 0, 55015, 97910, 0, 'Anguish Jailer to Player'),

(98229, 2, 0, 'Cyana, Jace, Allari... find the keystone.', 12, 0, 100, 0, 0, 55144, 100341, 0, 'Kayn Sunfury <Illidari> to Player'),
(98229, 3, 0, 'Now, let''s see about activating that gateway.', 12, 0, 100, 0, 0, 55143, 100136, 0, 'Kayn Sunfury <Illidari> to Player'),

(98460, 0, 0, 'Having fun? I am!', 12, 0, 100, 0, 0, 55287, 100223, 0, 'Kor''vas Bloodthorn <Illidari> to Player'),
(98460, 0, 1, 'Hah! Racking up the kills.', 12, 0, 100, 0, 0, 55288, 100222, 0, 'Kor''vas Bloodthorn <Illidari> to Player'),

(98484, 0, 0, 'Taste my blade.', 12, 0, 100, 0, 0, 55327, 97902, 0, 'Mo''arg Brute to Player'),
(98484, 0, 1, 'Ahahahahaha! I will cut you down.', 12, 0, 100, 0, 0, 0, 94883, 0, 'Mo''arg Brute to Player'),
(98484, 0, 2, 'Hunt this.', 12, 0, 100, 0, 0, 55328, 97903, 0, 'Mo''arg Brute to Player'),

(98486, 0, 0, 'Brood Queen Tyranna orders your death.', 12, 0, 100, 0, 0, 55366, 98762, 0, 'Wrath Warrior to Player'),
(98486, 0, 1, 'This is where you die.', 12, 0, 100, 0, 0, 55362, 98765, 0, 'Wrath Warrior to Player'),
(98486, 0, 2, 'I live to serve.', 12, 0, 100, 0, 0, 55363, 98759, 0, 'Wrath Warrior to Player'),
(98486, 0, 3, 'My life for the Legion', 12, 0, 100, 0, 0, 55364, 98760, 0, 'Wrath Warrior to Player'),
(98486, 0, 4, 'You will not gain the keystone.', 12, 0, 100, 0, 0, 55365, 98761, 0, 'Wrath Warrior to Player'),
(98486, 0, 5, 'You are outmatched and outnumbered.', 12, 0, 100, 0, 0, 55367, 98764, 0, 'Wrath Warrior to Player'),
(98486, 0, 6, 'We will cleanse the universe in fire.', 12, 0, 100, 0, 0, 55370, 98766, 0, 'Wrath Warrior to Player'),
(98486, 0, 7, 'My blade will cut through you.', 12, 0, 100, 0, 0, 55371, 98763, 0, 'Wrath Warrior to Player'),

(97034, 0, 0, 'Brood Queen Tyranna orders your death.', 12, 0, 100, 0, 0, 55366, 98762, 0, 'Fury Champion to Player'),
(97034, 0, 1, 'This is where you die.', 12, 0, 100, 0, 0, 55362, 98765, 0, 'Fury Champion to Player'),
(97034, 0, 2, 'I live to serve.', 12, 0, 100, 0, 0, 55363, 98759, 0, 'Fury Champion to Player'),
(97034, 0, 3, 'My life for the Legion', 12, 0, 100, 0, 0, 55364, 98760, 0, 'Fury Champion to Player'),
(97034, 0, 4, 'You will not gain the keystone.', 12, 0, 100, 0, 0, 55365, 98761, 0, 'Fury Champion to Player'),
(97034, 0, 5, 'You are outmatched and outnumbered.', 12, 0, 100, 0, 0, 55367, 98764, 0, 'Fury Champion to Player'),
(97034, 0, 6, 'We will cleanse the universe in fire.', 12, 0, 100, 0, 0, 55370, 98766, 0, 'Fury Champion to Player'),
(97034, 0, 7, 'My blade will cut through you.', 12, 0, 100, 0, 0, 55371, 98763, 0, 'Fury Champion to Player'),

(98497, 0, 0, 'I''m so hungry.', 12, 0, 100, 0, 0, 55211, 102142, 0, 'Imp Mother to Player'),
(98497, 0, 1, 'My meal comes to me.', 12, 0, 100, 0, 0, 55212, 102143, 0, 'Imp Mother to Player'),
(98497, 0, 2, 'Climb inside my mouth, tiny thing.', 12, 0, 100, 0, 0, 55213, 102144, 0, 'Imp Mother to Player'),
(98497, 0, 3, 'Come, my little imps, dance for mother.', 12, 0, 100, 0, 0, 55214, 102145, 0, 'Imp Mother Player'),
(98497, 0, 4, 'My children will cook you up nicely.', 12, 0, 100, 0, 0, 55215, 102146, 0, 'Imp Mother to Player'),
(98497, 0, 5, 'You''ll die for this!', 12, 0, 100, 0, 0, 55216, 102147, 0, 'Imp Mother to Player'),
(98497, 0, 6, 'Have you come to play with my little imps?', 12, 0, 100, 0, 0, 55217, 102149, 0, 'Imp Mother to Player'),
(98497, 0, 7, 'The Legion''s victory is inevitable, child.', 12, 0, 100, 0, 0, 55218, 102149, 0, 'Imp Mother to Player'),
(98497, 0, 8, 'I''ll deliver you to Tyranna myself.', 12, 0, 100, 0, 0, 55219, 102150, 0, 'Imp Mother to Player'),
(98497, 0, 9, 'Filthy, little elf. I can taste the fel energy on you.', 12, 0, 100, 0, 0, 55220, 102151, 0, 'Imp Mother Player'),
(98497, 0, 10, 'Intruders! Someone warn the Doom Commander!', 12, 0, 100, 0, 0, 55221, 102152, 0, 'Imp Mother to Player');

