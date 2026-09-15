-- Hunter class-hall intro bridge: script the unscripted credit steps of On Eagle's Wings (40953, talk-to-flightmaster
-- + eagle flight to Trueshot Lodge) and The Unseen Path (40954, induction scene). Base Legion content TC never scripted.
INSERT INTO `quest_template_addon` (`ID`) VALUES (40953),(40954) ON DUPLICATE KEY UPDATE `ID`=`ID`;
UPDATE `quest_template_addon` SET `ScriptName`='quest_on_eagles_wings' WHERE `ID`=40953;
UPDATE `quest_template_addon` SET `ScriptName`='quest_the_unseen_path' WHERE `ID`=40954;
