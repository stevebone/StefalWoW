-- Hunter BM artifact "Never Hunt Alone" (42185) now grants the artifact weapon Titanstrike (128861) on turn-in.
-- The Legion artifact acquisitions were built to COMPLETE (objectives + turn-in) but never granted the actual weapon
-- (the "Weapons of Legend" grant was deferred). This wires the BM hunter weapon as the quest reward. (Systematic gap:
-- every spec's acquisition quest needs its artifact as a reward + the order-hall handoff — this is the first, Hunter BM.)
UPDATE `quest_template` SET `RewardItem1` = 128861, `RewardAmount1` = 1 WHERE `ID` = 42185;
