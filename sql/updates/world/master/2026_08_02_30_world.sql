-- Hunter order-hall induction: script the credit steps of Oath of Service (40955), Tactical Matters (40958),
-- The Campaign Begins (40959) - unscripted base Legion content. See HUNTER_ORDERHALL_CAMPAIGN_PLAN.md.
INSERT INTO `quest_template_addon` (`ID`) VALUES (40955),(40958),(40959) ON DUPLICATE KEY UPDATE `ID`=`ID`;
UPDATE `quest_template_addon` SET `ScriptName`='quest_oath_of_service'     WHERE `ID`=40955;
UPDATE `quest_template_addon` SET `ScriptName`='quest_tactical_matters'    WHERE `ID`=40958;
UPDATE `quest_template_addon` SET `ScriptName`='quest_the_campaign_begins' WHERE `ID`=40959;
