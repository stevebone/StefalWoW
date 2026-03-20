
-- Quest: 14079 Learn to Ride in Elwynn Forest

UPDATE `quest_template` SET `RewardFactionID1` = '72', `Expansion` = '0', `AllowableRaces` = 0 WHERE (`ID` = '14079');


-- Reactivating old/deprecated quests in Elwynn Forest
DELETE FROM `disables` WHERE (`sourceType` = '1') and (`entry` = '14079');


