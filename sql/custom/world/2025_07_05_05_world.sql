/* Alliance Vendor Settings */
SET @A_ENTRY = 65000;
SET @C_GUID = 650000;
SET @A_MODELID = 28195;
SET @A_NAME = "Arcanist Zora";
SET @A_SUBNAME = "Heirloom Weapon Vendor";

/* Horde Vendor Settings */
SET @H_ENTRY = 65001;
SET @H_MODELID = 27328;
SET @H_NAME = "Warlord Gorn";
SET @H_SUBNAME = "Heirloom Weapon Vendor";

/* Perform Clean Up */
DELETE FROM `creature_template` WHERE `entry` = @A_ENTRY;
DELETE FROM `creature_template` WHERE `entry` = @H_ENTRY;
DELETE FROM `npc_vendor` WHERE `entry` = @A_ENTRY;
DELETE FROM `npc_vendor` WHERE `entry` = @H_ENTRY;
DELETE FROM `creature` WHERE `id` = @A_ENTRY;
DELETE FROM `creature` WHERE `id` = @H_ENTRY;

INSERT INTO `creature_template` (`entry`,`modelid1`,`name`,`subname`,`minlevel`,`maxlevel`,`exp`,`faction`,`npcflag`,`BaseAttackTime`,`RangeAttackTime`,`unit_class`,`unit_flags`,`unit_flags2`,`type`,`flags_extra`) VALUES
(@A_ENTRY,@A_MODELID,@A_NAME,@A_SUBNAME,80,80,2,35,128,2000,2000,8,768,2048,7,2),
(@H_ENTRY,@H_MODELID,@H_NAME,@H_SUBNAME,80,80,2,35,128,2000,2000,8,768,2048,7,2);

INSERT INTO `npc_vendor` (`entry`,`slot`,`item`) VALUES 
/* Alliance */
(@A_ENTRY,0,42944),    /* Balanced Heartseeker */
(@A_ENTRY,1,42943),    /* Bloodied Arcanite Reaper */
(@A_ENTRY,2,42946),    /* Charmed Ancient Bone Bow */
(@A_ENTRY,3,42948),    /* Devout Aurastone Hammer */
(@A_ENTRY,4,42947),    /* Dignified Headmaster's Charge */
(@A_ENTRY,5,48718),    /* Repurposed Lava Dredger */
(@A_ENTRY,6,42945),    /* Venerable Dal'Rend's Sacred Charge */
(@A_ENTRY,7,48716),    /* Venerable Mass of McGowan */
/* Horde */
(@H_ENTRY,0,42944),    /* Balanced Heartseeker */
(@H_ENTRY,1,42943),    /* Bloodied Arcanite Reaper */
(@H_ENTRY,2,42946),    /* Charmed Ancient Bone Bow */
(@H_ENTRY,3,42948),    /* Devout Aurastone Hammer */
(@H_ENTRY,4,42947),    /* Dignified Headmaster's Charge */
(@H_ENTRY,5,48718),    /* Repurposed Lava Dredger */
(@H_ENTRY,6,42945),    /* Venerable Dal'Rend's Sacred Charge */
(@H_ENTRY,7,48716);    /* Venerable Mass of McGowan */

INSERT INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnMask`,`phaseMask`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`wander_distance`,`currentwaypoint`,`curhealth`,`curmana`,`MovementType`,`npcflag`,`unit_flags`,`dynamicflags`,`ScriptName`,`VerifiedBuild`) VALUES 
/* Alliance */
(@C_GUID ,@A_ENTRY,0,0,0,1,1,0,0,-6215.5,326.89,383.385,2.39655,300,0,0,10080,8814,0,0,0,0,null,0),         /* Coldridge Valley */
(@C_GUID+1, @A_ENTRY,530,0,0,1,1,0,0,-4048.41,-13781.5,75.1095,5.63756,300,0,0,10080,8814,0,0,0,0,null,0),    /* Crash Site */
(@C_GUID+2, @A_ENTRY,1,0,0,1,1,0,0,10326.3,821.055,1326.45,2.29859,300,0,0,10080,8814,0,0,0,0,null,0),        /* Shadowglen */
(@C_GUID+3, @A_ENTRY,0,0,0,1,1,0,0,-8923.72,-136.917,80.9533,1.9607,300,0,0,10080,8814,0,0,0,0,null,0),       /* Northshire Valley */
(@C_GUID+4, @A_ENTRY,609,0,0,1,1,0,0,2361.17,-5647.82,426.118,4.89697,300,0,0,10080,8814,0,0,0,0,null,0),     /* Acherus: The Ebon Hold */
/* Horde */
(@C_GUID+5, @H_ENTRY,530,0,0,1,1,0,0,10340.2,-6374.12,35.4844,1.80925,300,0,0,10080,8814,0,0,0,0,null,0),     /* Sunstrider Isle */
(@C_GUID+6, @H_ENTRY,1,0,0,1,1,0,0,-607.738,-4206.13,38.4542,4.63386,300,0,0,10080,8814,0,0,0,0,null,0),      /* Valley of Trials */
(@C_GUID+7, @H_ENTRY,1,0,0,1,1,0,0,-2900.26,-273.087,53.8201,1.79071,300,0,0,10080,8814,0,0,0,0,null,0),      /* Camp Narache */
(@C_GUID+8, @H_ENTRY,0,0,0,1,1,0,0,1846.15,1605.74,94.5609,4.72063,300,0,0,10080,8814,0,0,0,0,null,0),        /* Deathknell */
(@C_GUID+9, @H_ENTRY,609,0,0,1,1,0,0,2372.47,-5664.76,426.127,2.60753,300,0,0,10080,8814,0,0,0,0,null,0);     /* Acherus: The Ebon Hold */