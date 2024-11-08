-- various changes to daily quests
-- increase cooking quest reward currency
-- safe to apply multiple times, it wont keep increasing every time you run the sql
UPDATE `quest_template` SET `RewardCurrencyQty1` = 25 WHERE `RewardCurrencyID1` = 81 AND `RewardCurrencyQty1` = 5;
UPDATE `quest_template` SET `RewardCurrencyQty1` = 10 WHERE `RewardCurrencyID1` = 81 AND `RewardCurrencyQty1` = 2;
UPDATE `quest_template` SET `RewardCurrencyQty1` = 5 WHERE `RewardCurrencyID1` = 81 AND `RewardCurrencyQty1` = 1;