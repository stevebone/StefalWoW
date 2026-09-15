-- PlayerChoice 644 "Which covenant do you want to join?" - gating for covenant SWITCHING.
--
-- 2026_08_07_30_covenant_choice_conditions.sql took the four "Join" responses off the panel for anybody who had
-- already pledged, because the server refused a second pledge. It no longer does: covenant switching is
-- implemented (Player::SetActiveCovenant / playerchoice_covenant_selection), so the panel has to offer the three
-- FOREIGN covenants exactly when a switch is actually allowed - and keep hiding them when it is not, or the client
-- is back to offering a choice the server rejects.
--
-- The rule is the 9.1.5 one and only that one: switching is free once the character has taken ANY covenant to
-- maximum renown. Maximum renown is Renown 80 - CurrencyTypes 1829 Renown-Kyrian / 1830 -Venthyr / 1831 -NightFae /
-- 1832 -Necrolord (and the shared 1822 display currency) all publish MaxQty 79 through MaxQtyWorldStateID 19735,
-- and renown level = currency quantity + 1, which is also the highest level RenownRewards.db2 defines for
-- covenants 1-4. The launch-era re-join quest chain, lockout and renown penalty are NOT modelled here: none of
-- their values exist in the 12.0.7.68275 client data.
--
-- CONDITION_COVENANT (62) carries the renown test in ConditionValue2 (see ConditionMgr.h). With ConditionValue1 = 0
-- ("any covenant") it asks "has this character ever reached renown N on any covenant", which is the free-switch
-- rule; membership is deliberately not required, because renown is per covenant and outlives leaving one.
--
-- Rows in different ElseGroups are OR'd, rows in the same ElseGroup are AND'd. Each join response gets three
-- alternatives:
--
--   ElseGroup 0 - not in any covenant yet. The original pledge; the only path a fresh character can take.
--   ElseGroup 1 - already in THIS covenant and quest 62000's objective 407067 "Choose your Covenant" is still
--                 uncredited. Repairs a character that pledged before that credit existed; re-picking its own
--                 covenant hands out the credit and the row stops matching.
--   ElseGroup 2 - free switching is unlocked (any covenant at Renown 80) AND this is not the covenant the
--                 character is already in. This is what puts the other three covenants back on the panel.
--
-- The four "Preview Covenant" responses (2686/2706/2707/2708) stay ungated - they are client-only
-- (C_CovenantPreview / UICovenantPreview.db2) and previewing never joins anything.
--
--   ResponseID | GroupID | Covenant.db2
--   -----------+---------+-------------------
--        2689  |    4    | 1 Kyrian
--        2702  |    1    | 2 Venthyr
--        2688  |    3    | 3 Night Fae
--        2687  |    2    | 4 Necrolord
--
-- Self-contained and idempotent: it re-declares every condition on those four responses, so it does not matter
-- whether 2026_08_07_30 ran before it.

SET @COVENANT_MAX_RENOWN := 80;   -- CurrencyTypes 1829-1832 MaxQty 79, renown level = quantity + 1

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 36 AND `SourceGroup` = 644 AND `SourceEntry` IN (2687, 2688, 2689, 2702);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
-- 2689 - Join Kyrian (covenant 1)
(36, 644, 2689, 0, 0, 62, 0,      0,                    0, 0, 1, 0, 0, '', 'PlayerChoice 644 - Join Kyrian while the player has no covenant'),
(36, 644, 2689, 0, 1, 62, 0,      1,                    0, 0, 0, 0, 0, '', 'PlayerChoice 644 - Join Kyrian: already Kyrian ...'),
(36, 644, 2689, 0, 1, 48, 0, 407067,                    0, 0, 0, 0, 0, '', 'PlayerChoice 644 - Join Kyrian: ... and quest 62000 "Choose your Covenant" still uncredited'),
(36, 644, 2689, 0, 2, 62, 0,      0, @COVENANT_MAX_RENOWN, 0, 0, 0, 0, '', 'PlayerChoice 644 - Switch to Kyrian: free switching unlocked (Renown 80 on any covenant) ...'),
(36, 644, 2689, 0, 2, 62, 0,      1,                    0, 0, 1, 0, 0, '', 'PlayerChoice 644 - Switch to Kyrian: ... and the player is not already Kyrian'),
-- 2702 - Join Venthyr (covenant 2)
(36, 644, 2702, 0, 0, 62, 0,      0,                    0, 0, 1, 0, 0, '', 'PlayerChoice 644 - Join Venthyr while the player has no covenant'),
(36, 644, 2702, 0, 1, 62, 0,      2,                    0, 0, 0, 0, 0, '', 'PlayerChoice 644 - Join Venthyr: already Venthyr ...'),
(36, 644, 2702, 0, 1, 48, 0, 407067,                    0, 0, 0, 0, 0, '', 'PlayerChoice 644 - Join Venthyr: ... and quest 62000 "Choose your Covenant" still uncredited'),
(36, 644, 2702, 0, 2, 62, 0,      0, @COVENANT_MAX_RENOWN, 0, 0, 0, 0, '', 'PlayerChoice 644 - Switch to Venthyr: free switching unlocked (Renown 80 on any covenant) ...'),
(36, 644, 2702, 0, 2, 62, 0,      2,                    0, 0, 1, 0, 0, '', 'PlayerChoice 644 - Switch to Venthyr: ... and the player is not already Venthyr'),
-- 2688 - Join Night Fae (covenant 3)
(36, 644, 2688, 0, 0, 62, 0,      0,                    0, 0, 1, 0, 0, '', 'PlayerChoice 644 - Join Night Fae while the player has no covenant'),
(36, 644, 2688, 0, 1, 62, 0,      3,                    0, 0, 0, 0, 0, '', 'PlayerChoice 644 - Join Night Fae: already Night Fae ...'),
(36, 644, 2688, 0, 1, 48, 0, 407067,                    0, 0, 0, 0, 0, '', 'PlayerChoice 644 - Join Night Fae: ... and quest 62000 "Choose your Covenant" still uncredited'),
(36, 644, 2688, 0, 2, 62, 0,      0, @COVENANT_MAX_RENOWN, 0, 0, 0, 0, '', 'PlayerChoice 644 - Switch to Night Fae: free switching unlocked (Renown 80 on any covenant) ...'),
(36, 644, 2688, 0, 2, 62, 0,      3,                    0, 0, 1, 0, 0, '', 'PlayerChoice 644 - Switch to Night Fae: ... and the player is not already Night Fae'),
-- 2687 - Join Necrolord (covenant 4)
(36, 644, 2687, 0, 0, 62, 0,      0,                    0, 0, 1, 0, 0, '', 'PlayerChoice 644 - Join Necrolord while the player has no covenant'),
(36, 644, 2687, 0, 1, 62, 0,      4,                    0, 0, 0, 0, 0, '', 'PlayerChoice 644 - Join Necrolord: already Necrolord ...'),
(36, 644, 2687, 0, 1, 48, 0, 407067,                    0, 0, 0, 0, 0, '', 'PlayerChoice 644 - Join Necrolord: ... and quest 62000 "Choose your Covenant" still uncredited'),
(36, 644, 2687, 0, 2, 62, 0,      0, @COVENANT_MAX_RENOWN, 0, 0, 0, 0, '', 'PlayerChoice 644 - Switch to Necrolord: free switching unlocked (Renown 80 on any covenant) ...'),
(36, 644, 2687, 0, 2, 62, 0,      4,                    0, 0, 1, 0, 0, '', 'PlayerChoice 644 - Switch to Necrolord: ... and the player is not already Necrolord');
