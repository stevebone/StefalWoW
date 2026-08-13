-- Chromie Time

-- Add RBAC permission for .chromietime GM command
DELETE FROM `rbac_permissions` WHERE `id` = 1000;
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES (1000, 'Command: chromietime');

-- Grant to GM role (secLevel 1 = moderator)
DELETE FROM `rbac_default_permissions` WHERE `permissionId` = 1000;
INSERT INTO `rbac_default_permissions` (`secId`, `permissionId`) VALUES (2, 1000);
