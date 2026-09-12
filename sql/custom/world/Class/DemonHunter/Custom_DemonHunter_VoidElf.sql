-- Demon Hunter fixes for Void Elf race

-- Void Elf (29) Demon Hunter (12) enablement (world DB)  
-- 1) Creation template: fixes Player::Create "invalid race/class pair (29/12)"  
DELETE FROM `playercreateinfo` WHERE `race` = 29 AND `class` = 12;  
INSERT INTO `playercreateinfo`  
(`race`, `class`, `map`, `position_x`, `position_y`, `position_z`, `orientation`,  
 `npe_map`, `npe_position_x`, `npe_position_y`, `npe_position_z`, `npe_orientation`,  
 `npe_transport_guid`, `intro_movie_id`, `intro_scene_id`, `npe_intro_scene_id`)  
VALUES  
(29, 12, 1865, 2121, 3318, 54.7061, 0.0872665, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 1903, NULL);

-- 2) Expansion gate: avoids CHAR_CREATE_EXPANSION_CLASS  
DELETE FROM `class_expansion_requirement` WHERE `ClassID` = 12 AND `RaceID` = 29;  
INSERT INTO `class_expansion_requirement` (`ClassID`, `RaceID`, `ActiveExpansionLevel`, `AccountExpansionLevel`)  
VALUES (12, 29, 0, 0); 

-- 3) Starting action bar (button 6 = 256948 Spatial Rift, Void Elf racial)  
DELETE FROM `playercreateinfo_action` WHERE `race` = 29 AND `class` = 12;  
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES  
(29, 12, 1, 131347, 0),  
(29, 12, 2, 188501, 0),  
(29, 12, 3, 344865, 0),  
(29, 12, 4, 344859, 0),  
(29, 12, 5, 344862, 0),  
(29, 12, 6, 256948, 0);