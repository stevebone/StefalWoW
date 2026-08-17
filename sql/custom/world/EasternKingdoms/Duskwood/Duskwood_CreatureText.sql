-- Duskwood Creature Texts

DELETE FROM `creature_text` WHERE `CreatureID` IN (315,43453,43797,43814);
INSERT INTO `creature_text` VALUES
(315, 0, 0, 'My ring... Who holds my family ring... Tilloa, is that you?', 12, 0, 100, 0, 0, 0, 0, 43963, 0, 'Stalvan Mistmantle - to Player'),
(315, 1, 0, 'Tobias...', 12, 0, 100, 0, 0, 0, 0, 43964, 0, 'Stalvan Mistmantle to Player'),
(315, 2, 0, 'It''s all true, brother. Every word. You doubted it?', 12, 0, 100, 11, 0, 0, 0, 43965, 0, 'Stalvan Mistmantle to Player'),
(315, 3, 0, 'You know why!', 12, 0, 100, 25, 0, 0, 0, 43968, 0, 'Stalvan Mistmantle to Player'),
(315, 4, 0, 'Surely you''ve felt anger. Anger so foul and vicious that it makes you want to tear someone to shreds...', 12, 0, 100, 1, 0, 0, 0, 43978, 0, 'Stalvan Mistmantle to Player'),
(315, 5, 0, 'Aren''t you feeling it right now?', 12, 0, 100, 6, 0, 0, 0, 43979, 0, 'Stalvan Mistmantle to Player'),
(315, 6, 0, 'You see, brother... we''re not so different...', 12, 0, 100, 6, 0, 0, 0, 43981, 0, 'Stalvan Mistmantle to Tobias Mistmantle'),
(43453, 0, 0, 'Brother!', 12, 0, 100, 0, 0, 0, 0, 43966, 0, 'Tobias Mistmantle to Player'),
(43453, 1, 0, 'Tell me it''s not true, brother. Tell me you didn''t die a murderer!', 12, 0, 100, 25, 0, 0, 0, 43967, 0, 'Tobias Mistmantle to Player'),
(43453, 2, 0, 'But why?! How could you?', 12, 0, 100, 5, 0, 0, 0, 43973, 0, 'Tobias Mistmantle to Player'),
(43797, 3, 0, 'No... NO! STOP IT!', 14, 0, 100, 15, 0, 0, 0, 43980, 0, 'Tobias Mistmantle to Player'),
(43797, 4, 0, 'No...', 12, 0, 100, 0, 0, 0, 0, 43982, 0, 'Tobias Mistmantle to Stalvan Mistmantle'),

(43814, 0, 0, 'The worgen stares and hesitates!', 41, 0, 100, 0, 0, 0, 0, 44126, 0, 'Lurking Worgen to Player');

DELETE FROM `creature_text` WHERE `CreatureID` IN (43858,43859,43950,43861);
INSERT INTO `creature_text` VALUES
(43859, 0, 0, 'I... I can''t...', 12, 0, 100, 0, 0, 0, 0, 44289, 0, 'Jitters to Player'),
(43858, 0, 0, 'Here we go...', 12, 0, 100, 396, 0, 0, 0, 44300, 0, 'Oliver Harris to Player'),
(43858, 1, 0, 'It''s working. Hold him still, Jitters.', 12, 0, 100, 396, 0, 0, 0, 44288, 0, 'Oliver Harris to Player'),
(43858, 2, 0, 'Damn it, Jitters, I said HOLD!', 12, 0, 100, 5, 0, 0, 0, 44290, 0, 'Oliver Harris to Player'),
(43858, 3, 0, 'Letting him go is the only thing that''s going to separate you from the beasts now, my friend.', 12, 0, 100, 0, 0, 0, 0, 44296, 0, 'Oliver Harris to Player'),
(43950, 0, 0, 'Jitters...', 12, 0, 100, 0, 0, 0, 0, 44291, 0, 'Lurking Worgen to Player'),
(43950, 1, 0, 'JITTERS!', 14, 0, 100, 0, 0, 0, 0, 44292, 0, 'Lurking Worgen to Player'),
(43950, 2, 0, 'I remember now... it''s all your fault!', 12, 0, 100, 0, 0, 0, 0, 44293, 0, 'Lurking Worgen to Player'),
(43950, 3, 0, 'You brought the worgen to Duskwood! You led the Dark Riders to my farm, and hid while they murdered my family!', 12, 0, 100, 0, 0, 0, 0, 44294, 0, 'Lurking Worgen to Player'),
(43950, 4, 0, 'Every speck of suffering in my life is YOUR PATHETIC FAULT! I SHOULD KILL YOU!', 14, 0, 100, 0, 0, 0, 0, 44295, 0, 'Lurking Worgen to Player'),
(43861, 0, 0, 'You''ve got a lot to make up for, Jitters. I won''t give you the easy way out.', 12, 0, 100, 0, 0, 0, 0, 44297, 0, 'Lurking Worgen to Player');
