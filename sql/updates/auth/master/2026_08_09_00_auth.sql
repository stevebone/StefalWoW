--
-- In-game Shop catalog administration RBAC permissions.
--   886 = Command: reload shop_catalog  (.reload shop_catalog)
--   887 = Command: shop                 (.shop list/enable/disable/price/window/feature/preview)
-- Linked into the same groups as the neighbouring reload / GM command permissions so the default
-- GM roles pick them up (196 = reload group, 197 = GM command group).

DELETE FROM `rbac_permissions` WHERE `id` IN (886,887);
INSERT INTO `rbac_permissions` (`id`,`name`) VALUES
(886,'Command: reload shop_catalog'),
(887,'Command: shop');

DELETE FROM `rbac_linked_permissions` WHERE `linkedId` IN (886,887);
INSERT INTO `rbac_linked_permissions` (`id`,`linkedId`) VALUES
(196,886),
(197,887);
