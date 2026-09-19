-- Mardum Creature Text
DELETE FROM `creature_text` WHERE `creatureID` IN (98484,98486,98497,98460,95226, 93112);
DELETE FROM `creature_text` WHERE `creatureID` IN (98229) AND `GroupID` IN (2,3);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
(93112, 0, 0, 'For the Legion!', 12, 0, 100, 0, 0, 55192, 96663, 0, 'Felguard Sentry to Player'),
(93112, 0, 1, 'Demon hunters? How did you get here?', 12, 0, 100, 0, 0, 55187, 94934, 0, 'Felguard Sentry to Player'),
(93112, 0, 2, 'Die, Illidari fool.', 12, 0, 100, 0, 0, 55189, 94936, 0, 'Felguard Sentry to Player'),
(93112, 0, 3, 'I''ll rend you limb from limb.', 12, 0, 100, 0, 0, 55188, 94935, 0, 'Felguard Sentry to Demon Hunter'),
(93112, 0, 4, 'Invaders. Warn the Brood Queen!', 12, 0, 100, 0, 0, 55186, 94932, 0, 'Felguard Sentry to Demon Hunter'),
(93112, 0, 5, 'The fel you wield will not be enough.', 12, 0, 100, 0, 0, 55190, 96661, 0, 'Felguard Sentry to Demon Hunter'),
(93112, 0, 6, 'For Brood Queen Tyranna. For the Legion!', 12, 0, 100, 0, 0, 55193, 96664, 0, 'Felguard Sentry to Demon Hunter'),
(93112, 0, 7, 'You dare attack us here?!', 12, 0, 100, 0, 0, 55191, 96662, 0, 'Felguard Sentry to Demon Hunter'),

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

(98484, 0, 0, 'Taste my blade.', 12, 0, 100, 0, 0, 0, 82439, 0, 'Mo''arg Brute to Player'),
(98484, 0, 1, 'Ahahahahaha! I will cut you down.', 12, 0, 100, 0, 0, 0, 94883, 0, 'Mo''arg Brute to Player'),

(98486, 0, 0, 'Brood Queen Tyranna orders your death.', 12, 0, 100, 0, 0, 55366, 98762, 0, 'Wrath Warrior to Player'),
(98486, 0, 1, 'This is where you die.', 12, 0, 100, 0, 0, 55362, 98765, 0, 'Wrath Warrior to Player'),
(98486, 0, 2, 'I live to serve.', 12, 0, 100, 0, 0, 55363, 98759, 0, 'Wrath Warrior to Player'),
(98486, 0, 3, 'My life for the Legion', 12, 0, 100, 0, 0, 55364, 98760, 0, 'Wrath Warrior to Player'),
(98486, 0, 4, 'You will not gain the keystone.', 12, 0, 100, 0, 0, 55365, 98761, 0, 'Wrath Warrior to Player'),
(98486, 0, 5, 'You are outmatched and outnumbered.', 12, 0, 100, 0, 0, 55367, 98764, 0, 'Wrath Warrior to Player'),
(98486, 0, 6, 'We will cleanse the universe in fire.', 12, 0, 100, 0, 0, 55370, 98766, 0, 'Wrath Warrior to Player'),
(98486, 0, 7, 'My blade will cut through you.', 12, 0, 100, 0, 0, 55371, 98763, 0, 'Wrath Warrior to Player'),

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

