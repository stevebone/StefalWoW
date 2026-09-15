-- PlayerChoice 644 "Which covenant do you want to join?" must stop offering covenants to a player who has
-- already pledged.
--
-- Reported from testing: after joining Kyrian, re-using the Oribos covenant map (GO 357095 -> spell 343884 ->
-- PlayerChoice 644) still presented all four covenants as joinable. The server side already refused the switch
-- (playerchoice_covenant_selection::OnResponse), so the panel and the server disagreed: the client offered a
-- choice that silently did nothing.
--
-- Mechanism: Player::SendPlayerChoice filters every response through
-- sConditionMgr->IsObjectMeetingPlayerChoiceResponseConditions(choiceId, responseId, player) before it is put in
-- SMSG_DISPLAY_PLAYER_CHOICE, so CONDITION_SOURCE_TYPE_PLAYER_CHOICE_RESPONSE (36) is the one mechanism that can
-- take a response off the panel instead of rejecting it after the fact. SourceGroup = ChoiceID,
-- SourceEntry = ResponseID.
--
-- Only the four "Join" responses are gated. The four "Preview Covenant" responses (2686/2706/2707/2708) are
-- client-only (C_CovenantPreview.GetCovenantInfoForPlayerChoiceResponseID / UICovenantPreview.db2) and stay
-- visible, so the map keeps working as a covenant browser after the pledge.
--
--   ResponseID | GroupID | Covenant.db2
--   -----------+---------+-------------------
--        2689  |    4    | 1 Kyrian
--        2702  |    1    | 2 Venthyr
--        2688  |    3    | 3 Night Fae
--        2687  |    2    | 4 Necrolord
--
-- Each join response gets two alternatives (rows in different ElseGroups are OR'd, rows in the same ElseGroup
-- are AND'd):
--
--   ElseGroup 0 - CONDITION_COVENANT (61) with ConditionValue1 = 0 ("in any covenant") and NegativeCondition = 1:
--                 the player has not pledged yet. This is the normal case and the only one that can actually join.
--
--   ElseGroup 1 - CONDITION_COVENANT (61) with the response's own covenant, AND
--                 CONDITION_QUEST_OBJECTIVE_PROGRESS (48) on objective 407067 ("Choose your Covenant", quest 62000
--                 "Choosing Your Purpose") still at 0. That condition also requires the quest to be in the log.
--                 This exists for characters that pledged before the objective credit was awarded at all and are
--                 therefore stuck with the quest uncompletable; re-picking their own covenant hands out the credit
--                 (playerchoice_covenant_selection::OnResponse) and this row then stops matching, so the button
--                 disappears again. A foreign covenant is never offered by either alternative.

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 36 AND `SourceGroup` = 644 AND `SourceEntry` IN (2687, 2688, 2689, 2702);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
-- 2689 - Join Kyrian (covenant 1)
(36, 644, 2689, 0, 0, 62, 0,      0, 0, 0, 1, 0, 0, '', 'PlayerChoice 644 - Join Kyrian only while the player has no covenant'),
(36, 644, 2689, 0, 1, 62, 0,      1, 0, 0, 0, 0, 0, '', 'PlayerChoice 644 - Join Kyrian: already Kyrian ...'),
(36, 644, 2689, 0, 1, 48, 0, 407067, 0, 0, 0, 0, 0, '', 'PlayerChoice 644 - Join Kyrian: ... and quest 62000 "Choose your Covenant" still uncredited'),
-- 2702 - Join Venthyr (covenant 2)
(36, 644, 2702, 0, 0, 62, 0,      0, 0, 0, 1, 0, 0, '', 'PlayerChoice 644 - Join Venthyr only while the player has no covenant'),
(36, 644, 2702, 0, 1, 62, 0,      2, 0, 0, 0, 0, 0, '', 'PlayerChoice 644 - Join Venthyr: already Venthyr ...'),
(36, 644, 2702, 0, 1, 48, 0, 407067, 0, 0, 0, 0, 0, '', 'PlayerChoice 644 - Join Venthyr: ... and quest 62000 "Choose your Covenant" still uncredited'),
-- 2688 - Join Night Fae (covenant 3)
(36, 644, 2688, 0, 0, 62, 0,      0, 0, 0, 1, 0, 0, '', 'PlayerChoice 644 - Join Night Fae only while the player has no covenant'),
(36, 644, 2688, 0, 1, 62, 0,      3, 0, 0, 0, 0, 0, '', 'PlayerChoice 644 - Join Night Fae: already Night Fae ...'),
(36, 644, 2688, 0, 1, 48, 0, 407067, 0, 0, 0, 0, 0, '', 'PlayerChoice 644 - Join Night Fae: ... and quest 62000 "Choose your Covenant" still uncredited'),
-- 2687 - Join Necrolord (covenant 4)
(36, 644, 2687, 0, 0, 62, 0,      0, 0, 0, 1, 0, 0, '', 'PlayerChoice 644 - Join Necrolord only while the player has no covenant'),
(36, 644, 2687, 0, 1, 62, 0,      4, 0, 0, 0, 0, 0, '', 'PlayerChoice 644 - Join Necrolord: already Necrolord ...'),
(36, 644, 2687, 0, 1, 48, 0, 407067, 0, 0, 0, 0, 0, '', 'PlayerChoice 644 - Join Necrolord: ... and quest 62000 "Choose your Covenant" still uncredited');
