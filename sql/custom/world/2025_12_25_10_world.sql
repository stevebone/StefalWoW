-- Quest: 61 Shipment to Stormwind
-- NPC: 253 William Pestle
-- NPC: 279 Morgan Pestle

DELETE FROM disables where sourceType = 1 AND entry = 61;

DELETE FROM `creature_queststarter` WHERE `quest` IN (61);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('253', '61', '0');

DELETE FROM `creature_questender` WHERE `quest` IN (61);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES ('279', '61', '0');