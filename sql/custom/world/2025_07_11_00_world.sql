-- Database changes required for boat transport between Stormwind and Thalassian Highlands

INSERT IGNORE INTO gameobject_template VALUES
('300000', '15', '3015', 'Ship (The Survivor)', '', '', '', '1', '1979', '30', '1', '0', '0', '1', '725', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '', '', NULL, '12340');

INSERT IGNORE INTO gameobject_template_addon VALUES
('300000', '0', '40', '0', '0', '0', '0', '0', '0');

INSERT IGNORE INTO transports VALUES
(21, 300000, 'Thalassian Highlands to Stormwind', '');