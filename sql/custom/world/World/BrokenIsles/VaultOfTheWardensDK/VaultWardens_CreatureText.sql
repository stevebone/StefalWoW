-- Vault of the Wardens Creature Text

DELETE FROM `creature_text` WHERE `creatureID` IN (92718, 96847, 99631, 99632, 92980, 92986, 92776, 97069, 97614, 97615, 97616, 97617, 97618, 97619, 97620, 97622);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
(92776, 0, 0, 'For the Legion!', 12, 0, 100, 0, 0, 55192, 96663, 0, 'Fel Shocktrooper to Player'),
(92776, 0, 1, 'The Burning Legion conquers all.', 12, 0, 100, 0, 0, 55198, 102133, 0, 'Fel Shocktrooper to Player'),
(92776, 0, 2, 'You will die by my blade.', 12, 0, 100, 0, 0, 55195, 102130, 0, 'Fel Shocktrooper to Player'),
(92776, 0, 3, 'You are easily dealt with.', 12, 0, 100, 0, 0, 55196, 102131, 0, 'Fel Shocktrooper to Player'),
(92776, 0, 4, 'Mortal flesh parts so easily.', 12, 0, 100, 0, 0, 55197, 102132, 0, 'Fel Shocktrooper to Player'),
(92776, 0, 5, 'I serve.', 12, 0, 100, 0, 0, 55194, 102129, 0, 'Fel Shocktrooper to Player'),
(92776, 0, 6, 'Die, Illidari fool.', 12, 0, 100, 0, 0, 55189, 94936, 0, 'Fel Shocktrooper to Player'),
(92776, 0, 7, 'I''ll rend you limb from limb.', 12, 0, 100, 0, 0, 55188, 94935, 0, 'Fel Shocktrooper to Demon Hunter'),
(92776, 0, 8, 'The fel you wield will not be enough.', 12, 0, 100, 0, 0, 55190, 96661, 0, 'Fel Shocktrooper to Demon Hunter'),

(92718, 0, 0, 'I will need your help to stop Gul\'dan. Free the other Illidari, quickly!', 12, 0, 100, 0, 0, 57338, 98734, 0, 'Maiev Shadowsong to Player'),

(96847, 0, 0, 'I hope Maiev was right about freeing you, demon hunter. Go on - I will hold the line.', 12, 0, 100, 0, 0, 57291, 98753, 0, 'Drelanim Whisperwind to Player'),

(99632, 0, 0, 'How many years were wasted in that cell?', 12, 0, 100, 0, 0, 57343, 100833, 0, 'Altruis the Sufferer to Player'),
(99632, 1, 0, 'It has been a long time, Kayn. I still believe in the cause. But, I do not blindly follow anyone... even Illidan.', 12, 0, 100, 1, 0, 57285, 91916, 0, 'Altruis the Sufferer to Player'),

(99631, 0, 0, 'I am awake? How is this possible?', 12, 0, 100, 0, 0, 57294, 100787, 0, 'Kayn Sunfury to Player'),
(99631, 1, 0, 'Why did you let THAT one free? He opposed Lord Illidan!', 12, 0, 100, 603, 0, 57320, 91600, 0, 'Kayn Sunfury to Player'),

(92980, 0, 0, 'Why would Maiev free us? There could only be one reason...', 12, 0, 100, 0, 0, 57387, 101987, 0, 'Kayn Sunfury to Player'),
(92980, 1, 0, 'The demons aim to destroy our world. We need to release our allies.', 12, 0, 100, 0, 0, 57340, 98738, 0, 'Kayn Sunfury to Player'),

(92986, 0, 0, 'The Legion is here.', 12, 0, 100, 0, 0, 57388, 101988, 0, 'Altruis the Sufferer to Player'),
(92986, 1, 0, 'Time for answers later. There are demons to kill.', 12, 0, 100, 0, 0, 57339, 98737, 0, 'Altruis the Sufferer to Player'),

(97614, 0, 0, 'The Wardens seek our help? Things must be desperate.', 12, 0, 100, 1, 0, 57334, 98708, 0, 'Allari the Souleater to Player'),
(97615, 0, 0, 'I will go ahead and secure the elevators.', 12, 0, 100, 1, 0, 57292, 99504, 0, 'Jace Darkweaver to Player'),
(97616, 0, 0, 'Years and years with only the voices in my head. I thought I would go insane.', 12, 0, 100, 1, 0, 57290, 98712, 0, 'Cyana Nightglaive to Player'),
(97617, 0, 0, 'Ahhhh, it feels good to MOVE again. Thank you, $n.', 12, 0, 100, 1, 0, 57287, 98709, 0, 'Asha Ravensong to Player'),
(97618, 0, 0, 'We will need weapons and supplies if we are to break out of here. I am on it, $n.', 12, 0, 100, 1, 0, 57289, 98711, 0, 'Falara Nightsong to Player'),
(97619, 0, 0, 'I sense demons nearby. After all that time in stasis, I am ready to hunt.', 12, 0, 100, 1, 0, 57344, 99505, 0, 'Marius Felbane to Player'),
(97620, 0, 0, 'My blades are ready, $n. I will check on the situation ahead.', 12, 0, 100, 1, 0, 57293, 99506, 0, 'Lyana Darksorrow to Player'),
(97622, 0, 0, '$n, it has been a long time since Mardum. I am ready for another fight.', 12, 0, 100, 1, 0, 57288, 98710, 0, 'Kor\'vas Bloodthorn to Player'),

(97069, 0, 0, 'All goes according to HIS plan.', 12, 0, 100, 0, 0, 57392, 102952, 0, 'Wrath-Lord Lekos to Player'),
(97069, 1, 0, 'Lord Gul''dan will lead us to victory. You cannot stop us.', 12, 0, 100, 0, 0, 57312, 102954, 0, 'Wrath-Lord Lekos to Player');
