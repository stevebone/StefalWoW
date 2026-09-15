-- Revert 2026_08_05_40: that migration was built on a wrong hypothesis (that the player's
-- foothold quest was 51570 "Foothold: Zuldazar" gated by 51569 "The Zandalar Campaign").
-- The player's actual quest is 51308 "Zuldazar Foothold" (handled in 2026_08_05_70).
-- Undo the 51569 shortcut so Halford (135612) does not offer a confusing extra quest,
-- and remove the 144635 accept->credit rows that targeted the wrong foothold ids.
UPDATE `quest_template_addon` SET `PrevQuestID`=53074 WHERE `ID`=51569;
DELETE FROM `smart_scripts` WHERE `entryorguid`=144635 AND `source_type`=0 AND `id` IN (1,2,3);
