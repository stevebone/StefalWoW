-- Custom Phases
-- Id starts at 800001
-- Table Hashes need to be updated when client is updated

-- Starting Areas
REPLACE INTO `hotfix_data` (`Id`, `UniqueId`, `TableHash`, `RecordId`, `Status`, `VerifiedBuild`) VALUES
(800001, 800001, 4042326115, 50001, 1, -1), -- Start zone 1
(800002, 800002, 4042326115, 50002, 1, -1); -- Start zone 2
(800003, 800003, 4042326115, 50003, 1, -1); -- Start zone 2

REPLACE INTO `phase` VALUES
(50001, 4, -1), -- Start zone 1
(50002, 4, -1), -- Start zone 2
(50003, 4, -1); -- Start zone 2