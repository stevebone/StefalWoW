-- Battleground Related Fixes

-- Ring of Valor map 618 is removed.
-- Removing the script registration and template should stop the error in the worldserver.
DELETE FROM `battleground_scripts` WHERE `MapId` = 618;
DELETE FROM `battleground_template` WHERE `ID` = 11;

-- --------------------------------------------------------------------------
-- 1. Twin Peaks: the mapping already exists, restore the missing NPC flag.
-- --------------------------------------------------------------------------
UPDATE `creature_template`
SET `npcflag` = (`npcflag` | 1048576)
WHERE `entry` = 50550
  AND (`npcflag` & 1048576) = 0;

-- --------------------------------------------------------------------------
-- 2. Restore missing battleground mappings for genuine battlemasters.
--    32   = Random Battleground
--    699  = Temple of Kotmogu
--    708  = Silvershard Mines
--    1018 = Arathi Basin (modern template)
-- --------------------------------------------------------------------------
DELETE FROM `battlemaster_entry` WHERE `entry` IN (34976,49573,62415,62416,62421,62422,62423,62424,144063,144067,
18439,32169,32170,35596,35597,35598,35599,35600,35601,35602,35603,35611,35612);
INSERT INTO `battlemaster_entry` (`entry`, `bg_template`) VALUES
    (34976,  32),
    (49573,  32),
    (62415, 708),
    (62416, 708),
    (62421, 699),
    (62422, 699),
    (62423, 699),
    (62424, 699),
    (144063, 1018),
    (144067, 1018),
	
	(18439, 6),
	(32169, 32),
	(32170, 32),
	(35596, 32),
	(35597, 32),
	(35598, 32),
	(35599, 32),
	(35600, 32),
	(35601, 32),
	(35602, 32),
	(35603, 32),
	(35611, 32),
	(35612, 32);

-- Ensure every restored mapping has the modern Battlemaster flag.
UPDATE `creature_template`
SET `npcflag` = (`npcflag` | 1048576)
WHERE `entry` IN
    (18439,32169,32170,35596,35597,35598,35599,35600,35601,35602,35603,35611,35612)
  AND (`npcflag` & 1048576) = 0;
  
