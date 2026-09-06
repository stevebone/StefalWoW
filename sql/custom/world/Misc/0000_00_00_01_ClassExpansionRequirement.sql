-- Add Haranir class info
-- Class expansion requirements for Haranir
DELETE FROM `class_expansion_requirement` WHERE `RaceID` IN (86, 91) AND `ClassID` IN (1, 3, 4, 5, 7, 8, 9, 10, 11);
INSERT INTO `class_expansion_requirement` (`ClassID`, `RaceID`, `ActiveExpansionLevel`, `AccountExpansionLevel`) VALUES
(1, 86, 11, 11),
(3, 86, 11, 11),
(4, 86, 11, 11),
(5, 86, 11, 11),
(7, 86, 11, 11),
(8, 86, 11, 11),
(9, 86, 11, 11),
(10, 86, 11, 11),
(11, 86, 11, 11),
(1, 91, 11, 11),
(3, 91, 11, 11),
(4, 91, 11, 11),
(5, 91, 11, 11),
(7, 91, 11, 11),
(8, 91, 11, 11),
(9, 91, 11, 11),
(10, 91, 11, 11),
(11, 91, 11, 11);
