-- Custom Phases
-- Id starts at 800001
-- Table Hashes need to be updated when client is updated

-- Starting Areas
REPLACE INTO `hotfix_data` (`Id`, `UniqueId`, `TableHash`, `RecordId`, `Status`, `VerifiedBuild`) VALUES
(800001, 800001, 4042326115, 50001, 1, -1), -- Phase Start zone 1
(800002, 800002, 4042326115, 50002, 1, -1), -- Phase Start zone 2
(800003, 800003, 4042326115, 50003, 1, -1), -- Phase Start zone 2
(800004, 800004, 4042326115, 50004, 1, -1), -- Phase Westfall 1
(800005, 800005, 4042326115, 50005, 1, -1), -- Phase Westfall 2
(800006, 800006, 4042326115, 50006, 1, -1), -- Phase Westfall 3
(800007, 800007, 4042326115, 50007, 1, -1), -- Phase Old Deadmines Dungeon
(800008, 800008, 441483745,  119,   1, -1), -- Area Trigger Deadmines Portal
(800009, 800009, 441483745,  6361,   1, -1); -- Area Trigger Deadmines Cannon door outside

REPLACE INTO `phase` VALUES
(50001, 4, -1), -- Start zone 1
(50002, 4, -1), -- Start zone 2
(50003, 4, -1), -- Start zone 2
(50004, 20, -1), -- Westfall 1
(50005, 20, -1), -- Westfall 2
(50006, 20, -1), -- Westfall 3
(50007, 4, -1); -- Old Deadmines Dungeon

REPLACE INTO `area_trigger` (`PosX`, `PosY`, `PosZ`, `ID`, `ContinentID`, `PhaseUseFlags`, `PhaseID`, `PhaseGroupID`, `Radius`, `BoxLength`, `BoxWidth`, `BoxHeight`, `BoxYaw`, `ShapeType`, `ShapeID`, `AreaTriggerActionSetID`, `Flags`, `VerifiedBuild`) VALUES 
(-14.36279964447, -393.38000488281, 64.56050109863, 119, 36, 1, 0, 0, 6, 0, 0, 0, 0, 0, 0, 180, 0, 67823),
(-96.15100097656, -704.35400390625, 8.89498996735, 6361, 36, 1, 0, 0, 0, 35.56999969482, 43.72999954224, 12.57999992371, 0, 1, 0, 2162, 0, 67823);