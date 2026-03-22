-- Westfall coast and farm areas changes and updates

-- Quest: 184 Furlbrows Deed

-- Reactivating old/deprecated quests in Stormwind
DELETE FROM `disables` WHERE `entry` = 184 AND `sourceType` = 1; -- quest source