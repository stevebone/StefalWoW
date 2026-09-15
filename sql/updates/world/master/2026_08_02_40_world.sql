-- Hunter campaign Batch 2 (Champion: Rexxar): script Recruiting Rexxar (42390) + Survive the Night (42392, the night
-- vigil at Rexxar's camp -> he pledges to the Unseen Path at dawn). Unscripted base Legion content.
INSERT INTO `quest_template_addon` (`ID`) VALUES (42390),(42392) ON DUPLICATE KEY UPDATE `ID`=`ID`;
UPDATE `quest_template_addon` SET `ScriptName`='quest_recruiting_rexxar' WHERE `ID`=42390;
UPDATE `quest_template_addon` SET `ScriptName`='quest_survive_the_night' WHERE `ID`=42392;
