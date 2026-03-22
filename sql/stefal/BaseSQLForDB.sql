-- Run these whenever the DB is restored to full (no updates) version from Draconic

-- hotfix DB
TRUNCATE `updates`;
TRUNCATE `updates_include`;

INSERT INTO `updates_include` VALUES
{'$/sql/custom/hotfixes', 'RELEASED'),
('$/sql/updates/hotfixes', 'RELEASED');

-- world DB
TRUNCATE `updates`;
TRUNCATE `updates_include`;

INSERT INTO `updates_include` VALUES
{'$/sql/custom/world', 'RELEASED'),
('$/sql/updates/world', 'RELEASED');


-- Updates will then be applied correctly on the first worldserver run.