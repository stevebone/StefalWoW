-- Chromie Time

-- Add RBAC permission for .chromietime GM command
DELETE FROM `rbac_permissions` WHERE `id` = 1000;
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES (1000, 'Command: chromietime');

-- Grant to GM role (secLevel 1 = moderator)
DELETE FROM `rbac_default_permissions` WHERE `permissionId` = 1000;
INSERT INTO `rbac_default_permissions` (`secId`, `permissionId`) VALUES (2, 1000);

-- Housing commands RBAC wiring.
-- New permissions 1001-1006 (RBAC_PERM_COMMAND_HOUSING*) linked to the GM default-permission
-- group 192, so every security level >= 3 (GM and up) gets them; lower levels do not.
-- Apply order: apply to the AUTH database before restarting worldserver with the new binary.

DELETE FROM `rbac_permissions` WHERE `id` BETWEEN 1001 AND 1006;
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
(1001,'Command: housing'),
(1002,'Command: housing set level'),
(1003,'Command: housing delete'),
(1004,'Command: housing charter create'),
(1005,'Command: housing charter set type'),
(1006,'Command: housing charter delete');

DELETE FROM `rbac_linked_permissions` WHERE `id` = 192 AND `linkedId` BETWEEN 1001 AND 1006;
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
(192,1001),
(192,1002),
(192,1003),
(192,1004),
(192,1005),
(192,1006);