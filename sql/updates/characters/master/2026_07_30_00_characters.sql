--
-- WoD Shipyard: persist the built shipyard tier per garrison. The shipyard is a garrison sub-feature (GarrBuilding
-- 205/206/207 = Lunarfall/Frostwall Shipyard L1/L2/L3, BuildingType 9) that is NOT placed on a normal architect
-- plot (it has no GarrBuildingPlotInst entry) and lives on the naval map. We track only its current building tier;
-- 0 = no shipyard built.
--
ALTER TABLE `character_garrison` ADD COLUMN IF NOT EXISTS `shipyardBuildingId` INT UNSIGNED NOT NULL DEFAULT 0 AFTER `cacheLastUsed`;
