
-- Level 255 Stats

-- Player XP For Levels
DELETE FROM `player_xp_for_level` WHERE level > 79;
drop procedure if exists xpstats;
DELIMITER $$
CREATE procedure xpstats()
block: BEGIN
    DECLARE i int DEFAULT 80;

 SET i = 79;
 WHILE i <= 253 DO
		INSERT INTO `player_xp_for_level` (`Level`, `Experience`) SELECT i+1, Experience+50000  FROM `player_xp_for_level` where level = i;
	
        SET i = i + 1;

 END WHILE;
END $$
DELIMITER ;
call xpstats();

-- Player Level Stats
ALTER TABLE `player_levelstats` CHANGE `str` `str` SMALLINT(4) UNSIGNED NOT NULL, CHANGE `agi` `agi` SMALLINT(4) UNSIGNED NOT NULL, CHANGE `sta` `sta` SMALLINT(4) UNSIGNED NOT NULL, CHANGE `inte` `inte` SMALLINT(4) UNSIGNED NOT NULL, CHANGE `spi` `spi` SMALLINT(4) UNSIGNED NOT NULL;
DELETE FROM `player_levelstats` WHERE `race` > 0 and `level` > 80;
drop procedure if exists levelstats;
DELIMITER $$
CREATE procedure levelstats()
block: BEGIN
    DECLARE i int DEFAULT 80;

 SET i = 80;
 WHILE i <= 254 DO
		INSERT INTO `player_levelstats` (`race`, `class`, `level`, `str`, `agi`, `sta`, `inte`, `spi`) SELECT race, class, i + 1, str + 3, agi + 3, sta + 3, inte + 3, spi + 3 FROM `player_levelstats` where level = i;
	
        SET i = i + 1;

 END WHILE;
END $$
DELIMITER ;
call levelstats();

-- Class Level Stats
ALTER TABLE `player_classlevelstats` MODIFY level SMALLINT;
ALTER TABLE `player_classlevelstats` CHANGE `basehp` `basehp` MEDIUMINT(8) UNSIGNED NOT NULL, CHANGE `basemana` `basemana` MEDIUMINT(8) UNSIGNED NOT NULL;
DELETE FROM `player_classlevelstats` WHERE `class` > 0 and `level` > 80;
drop procedure if exists classstats;
DELIMITER $$
CREATE procedure classstats()
block: BEGIN
    DECLARE i int DEFAULT 80;
    DECLARE hp16 int DEFAULT 8121;
    DECLARE hp2 int DEFAULT 6934;
    DECLARE hp3 int DEFAULT 7324;
    DECLARE hp4 int DEFAULT 7604;
    DECLARE hp5 int DEFAULT 6960;
    DECLARE hp7 int DEFAULT 6939;
    DECLARE hp8 int DEFAULT 6963;
    DECLARE hp9 int DEFAULT 7136;
    DECLARE hp11 int DEFAULT 7417;
    DECLARE mana27 int DEFAULT 4394;
    DECLARE mana3 int DEFAULT 5046;
    DECLARE mana59 int DEFAULT 3863;
    DECLARE mana8 int DEFAULT 3268;
    DECLARE mana11 int DEFAULT 3496;
 SET i = 80;
 WHILE i <= 254 DO
        INSERT INTO player_classlevelstats (class, level, basehp, basemana) VALUES (1, i+1, hp16+500, 0);
        INSERT INTO player_classlevelstats (class, level, basehp, basemana) VALUES (2, i+1, hp2+500, mana27+500);
        INSERT INTO player_classlevelstats (class, level, basehp, basemana) VALUES (3, i+1, hp3+500, mana3+500);
        INSERT INTO player_classlevelstats (class, level, basehp, basemana) VALUES (4, i+1, hp4+500, 0);
        INSERT INTO player_classlevelstats (class, level, basehp, basemana) VALUES (5, i+1, hp5+500, mana59+500);
        INSERT INTO player_classlevelstats (class, level, basehp, basemana) VALUES (6, i+1, hp16+500, 0);
        INSERT INTO player_classlevelstats (class, level, basehp, basemana) VALUES (7, i+1, hp7+500, mana27+500);
        INSERT INTO player_classlevelstats (class, level, basehp, basemana) VALUES (8, i+1, hp8+500, mana8+500);
        INSERT INTO player_classlevelstats (class, level, basehp, basemana) VALUES (9, i+1, hp9+500, mana59+500);
        INSERT INTO player_classlevelstats (class, level, basehp, basemana) VALUES (11, i+1, hp11+500, mana11+500);
	
        SET i = i + 1;
        SET hp16 = hp16+500;
        SET hp2 = hp2+500;
        SET hp3 = hp3+500;
        SET hp4 = hp4+500;
        SET hp5 = hp5+500;
        SET hp7 = hp7+500;
        SET hp8 = hp8+500;
        SET hp9 = hp8+500;
        SET hp11 = hp11+500;
		SET mana27 = mana27+500;
        SET mana3 = mana3+500;
        SET mana59 = mana59+500;
        SET mana8 = mana8+500;
        SET mana11 = mana11+500;
 END WHILE;
END $$
DELIMITER ;
call classstats();

-- Pet Level Stats
DELETE FROM `pet_levelstats` WHERE `creature_entry`=1 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry`=5058 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry`=5766 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry`=6250 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry`=24476 and `level` > 80;
drop procedure if exists creature_1;
DELIMITER $$
CREATE procedure creature_1()
block: BEGIN
    DECLARE i int DEFAULT 80;
    DECLARE hp int DEFAULT 5161;
    DECLARE armor int DEFAULT 9784;
    DECLARE str int DEFAULT 192;
    DECLARE agi int DEFAULT 158;
    DECLARE sta int DEFAULT 356;
    DECLARE inte int DEFAULT 70;
    DECLARE spi int DEFAULT 115;
 SET i = 80;
 WHILE i <= 254 DO
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (1, i+1, hp+100, 1, armor+10, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (5058, i+1, hp+100, 1, armor+10, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (5766, i+1, hp+100, 1, armor+10, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (6250, i+1, hp+100, 1, armor+10, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (24476, i+1, hp+100, 1, armor+10, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
        SET i = i + 1;
        SET hp = hp+100;
        SET armor=armor+10;
        SET str=str+1;
        SET agi=agi+1;
        SET sta=sta+1;
        SET inte=inte+1;
        SET spi=spi+1;
 END WHILE;
END $$
DELIMITER ;
call creature_1();

DELETE FROM `pet_levelstats` WHERE `creature_entry` = 3450 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry` = 8477 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry` = 8996 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry` = 10928 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry` = 10979 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry` = 12922 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry` = 22362 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry` = 24815 and `level` > 80;
drop procedure if exists creature_3450;
DELIMITER $$
CREATE procedure creature_3450()
block: BEGIN
    DECLARE i int DEFAULT 80;
    DECLARE hp int DEFAULT 1314;
    DECLARE mana int DEFAULT 2252;
    DECLARE str int DEFAULT 261;
    DECLARE agi int DEFAULT 50;
    DECLARE sta int DEFAULT 162;
    DECLARE inte int DEFAULT 470;
    DECLARE spi int DEFAULT 460;
 SET i = 80;
 WHILE i <= 254 DO
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (3450, i+1, hp+100, mana+100, 0, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (8477, i+1, hp+100, mana+100, 0, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (8996, i+1, hp+100, mana+100, 0, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (10928, i+1, hp+100, mana+100, 0, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (10979, i+1, hp+100, mana+100, 0, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (12922, i+1, hp+100, mana+100, 0, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (22362, i+1, hp+100, mana+100, 0, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (24815, i+1, hp+100, mana+100, 0, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
        SET i = i + 1;
        SET hp = hp+100;
        SET mana=mana+100;
        SET str=str+1;
        SET agi=agi+1;
        SET sta=sta+1;
        SET inte=inte+1;
        SET spi=spi+1;
 END WHILE;
END $$
DELIMITER ;
call creature_3450();

DELETE FROM `pet_levelstats` WHERE `creature_entry`=3939 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry`=15214 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry`=24656 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry`=25553 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry`=25566 and `level` > 80;
drop procedure if exists creature_3939;
DELIMITER $$
CREATE procedure creature_3939()
block: BEGIN
    DECLARE i int DEFAULT 80;
    DECLARE hp int DEFAULT 7320;
    DECLARE mana int DEFAULT 5170;
    DECLARE armor int DEFAULT 10087;
    DECLARE str int DEFAULT 177;
    DECLARE agi int DEFAULT 137;
    DECLARE sta int DEFAULT 361;
    DECLARE inte int DEFAULT 144;
    DECLARE spi int DEFAULT 146;
 SET i = 80;
 WHILE i <= 254 DO
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (3939, i+1, hp+100, mana+100, armor+10, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (15214, i+1, hp+100, mana+100, armor+10, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (24656, i+1, hp+100, mana+100, armor+10, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (25553, i+1, hp+100, mana+100, armor+10, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (25566, i+1, hp+100, mana+100, armor+10, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
	
        SET i = i + 1;
        SET hp = hp+100;
        SET mana=mana+100;
        SET armor=armor+10;
        SET str=str+1;
        SET agi=agi+1;
        SET sta=sta+1;
        SET inte=inte+1;
        SET spi=spi+1;
 END WHILE;
END $$
DELIMITER ;
call creature_3939();

DELETE FROM `pet_levelstats` WHERE `creature_entry`=329 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry`=15438 and `level` > 80;
drop procedure if exists creature_329;
DELIMITER $$
CREATE procedure creature_329()
block: BEGIN
    DECLARE i int DEFAULT 80;
    DECLARE hp int DEFAULT 8000;
    DECLARE mana int DEFAULT 2282;
    DECLARE armor int DEFAULT 1100;
    DECLARE str int DEFAULT 283;
    DECLARE agi int DEFAULT 160;
    DECLARE sta int DEFAULT 125;
    DECLARE inte int DEFAULT 405;
    DECLARE spi int DEFAULT 460;
 SET i = 80;
 WHILE i <= 254 DO
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (329, i+1, hp+100, mana+100, armor+10, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (15438, i+1, hp+100, mana+100, armor+10, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
	
        SET i = i + 1;
        SET hp = hp+100;
        SET mana=mana+100;
        SET armor=armor+10;
        SET str=str+1;
        SET agi=agi+1;
        SET sta=sta+1;
        SET inte=inte+1;
        SET spi=spi+1;
 END WHILE;
END $$
DELIMITER ;
call creature_329();

DELETE FROM `pet_levelstats` WHERE `creature_entry`=416 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry`=417 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry`=510 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry`=575 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry`=1860 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry`=1863 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry`=14385 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry`=15352 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry`=17252 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry`=19668 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry`=26101 and `level` > 80;
DELETE FROM `pet_levelstats` WHERE `creature_entry`=26125 and `level` > 80;
drop procedure if exists creature_416;
DELIMITER $$
CREATE procedure creature_416()
block: BEGIN
    DECLARE i int DEFAULT 80;
    DECLARE hp int DEFAULT 2129;
    DECLARE mana int DEFAULT 3228;
    DECLARE armor int DEFAULT 6273;
    DECLARE str int DEFAULT 297;
    DECLARE agi int DEFAULT 79;
    DECLARE sta int DEFAULT 118;
    DECLARE inte int DEFAULT 369;
    DECLARE spi int DEFAULT 367;
    DECLARE mindmg int DEFAULT 305;
    DECLARE maxdmg int DEFAULT 458;
 SET i = 80;
 WHILE i <= 254 DO
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (416, i+1, hp+100, mana+100, armor+10, str+1, agi+1, sta+1, inte+1, spi+1, mindmg+5, maxdmg+5);
	
        SET i = i + 1;
        SET hp = hp+100;
        SET mana=mana+100;
        SET armor=armor+10;
        SET str=str+1;
        SET agi=agi+1;
        SET sta=sta+1;
        SET inte=inte+1;
        SET spi=spi+1;
        SET mindmg=mindmg+5;
        SET maxdmg=maxdmg+5;
 END WHILE;
END $$
DELIMITER ;
call creature_416();
drop procedure if exists creature_417;
DELIMITER $$
CREATE procedure creature_417()
block: BEGIN
    DECLARE i int DEFAULT 80;
    DECLARE hp int DEFAULT 3733;
    DECLARE mana int DEFAULT 3025;
    DECLARE armor int DEFAULT 7782;
    DECLARE str int DEFAULT 314;
    DECLARE agi int DEFAULT 90;
    DECLARE sta int DEFAULT 328;
    DECLARE inte int DEFAULT 150;
    DECLARE spi int DEFAULT 209;
    DECLARE mindmg int DEFAULT 247;
    DECLARE maxdmg int DEFAULT 380;
 SET i = 80;
 WHILE i <= 254 DO
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (417, i+1, hp+100, mana+100, armor+10, str+1, agi+1, sta+1, inte+1, spi+1, mindmg+5, maxdmg+5);
	
        SET i = i + 1;
        SET hp = hp+100;
        SET mana=mana+100;
        SET armor=armor+10;
        SET str=str+1;
        SET agi=agi+1;
        SET sta=sta+1;
        SET inte=inte+1;
        SET spi=spi+1;
        SET mindmg=mindmg+5;
        SET maxdmg=maxdmg+5;
 END WHILE;
END $$
DELIMITER ;
call creature_417();
drop procedure if exists creature_510;
DELIMITER $$
CREATE procedure creature_510()
block: BEGIN
    DECLARE i int DEFAULT 80;
    DECLARE hp int DEFAULT 3029;
    DECLARE mana int DEFAULT 5540;
    DECLARE str int DEFAULT 203;
    DECLARE agi int DEFAULT 159;
    DECLARE sta int DEFAULT 90;
    DECLARE inte int DEFAULT 353;
    DECLARE spi int DEFAULT 240;
 SET i = 80;
 WHILE i <= 254 DO
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (510, i+1, hp+100, mana+100, 0, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
	
        SET i = i + 1;
        SET hp = hp+100;
        SET mana=mana+100;
        SET str=str+1;
        SET agi=agi+1;
        SET sta=sta+1;
        SET inte=inte+1;
        SET spi=spi+1;
 END WHILE;
END $$
DELIMITER ;
call creature_510();
drop procedure if exists creature_575;
DELIMITER $$
CREATE procedure creature_575()
block: BEGIN
    DECLARE i int DEFAULT 80;
    DECLARE hp int DEFAULT 7164;
    DECLARE mana int DEFAULT 5496;
    DECLARE str int DEFAULT 193;
    DECLARE agi int DEFAULT 153;
    DECLARE sta int DEFAULT 90;
    DECLARE inte int DEFAULT 350;
    DECLARE spi int DEFAULT 228;
 SET i = 80;
 WHILE i <= 254 DO
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (575, i+1, hp+100, mana+100, 0, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
	
        SET i = i + 1;
        SET hp = hp+100;
        SET mana=mana+100;
        SET str=str+1;
        SET agi=agi+1;
        SET sta=sta+1;
        SET inte=inte+1;
        SET spi=spi+1;
 END WHILE;
END $$
DELIMITER ;
call creature_575();
drop procedure if exists creature_1860;
DELIMITER $$
CREATE procedure creature_1860()
block: BEGIN
    DECLARE i int DEFAULT 80;
    DECLARE hp int DEFAULT 6401;
    DECLARE mana int DEFAULT 3997;
    DECLARE armor int DEFAULT 16148;
    DECLARE str int DEFAULT 314;
    DECLARE agi int DEFAULT 90;
    DECLARE sta int DEFAULT 328;
    DECLARE inte int DEFAULT 150;
    DECLARE spi int DEFAULT 209;
    DECLARE mindmg int DEFAULT 272;
    DECLARE maxdmg int DEFAULT 415;
 SET i = 80;
 WHILE i <= 254 DO
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (1860, i+1, hp+100, mana+100, armor+10, str+1, agi+1, sta+1, inte+1, spi+1, mindmg+5, maxdmg+5);
	
        SET i = i + 1;
        SET hp = hp+100;
        SET mana=mana+100;
        SET armor=armor+10;
        SET str=str+1;
        SET agi=agi+1;
        SET sta=sta+1;
        SET inte=inte+1;
        SET spi=spi+1;
        SET mindmg=mindmg+5;
        SET maxdmg=maxdmg+5;
 END WHILE;
END $$
DELIMITER ;
call creature_1860();
drop procedure if exists creature_1863;
DELIMITER $$
CREATE procedure creature_1863()
block: BEGIN
    DECLARE i int DEFAULT 80;
    DECLARE hp int DEFAULT 2665;
    DECLARE mana int DEFAULT 2989;
    DECLARE armor int DEFAULT 9706;
    DECLARE str int DEFAULT 314;
    DECLARE agi int DEFAULT 90;
    DECLARE sta int DEFAULT 328;
    DECLARE inte int DEFAULT 150;
    DECLARE spi int DEFAULT 209;
    DECLARE mindmg int DEFAULT 375;
    DECLARE maxdmg int DEFAULT 549;
 SET i = 80;
 WHILE i <= 254 DO
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (1863, i+1, hp+100, mana+100, armor+10, str+1, agi+1, sta+1, inte+1, spi+1, mindmg+5, maxdmg+5);
	
        SET i = i + 1;
        SET hp = hp+100;
        SET mana=mana+100;
        SET armor=armor+10;
        SET str=str+1;
        SET agi=agi+1;
        SET sta=sta+1;
        SET inte=inte+1;
        SET spi=spi+1;
        SET mindmg=mindmg+5;
        SET maxdmg=maxdmg+5;
 END WHILE;
END $$
DELIMITER ;
call creature_1863();
drop procedure if exists creature_14385;
DELIMITER $$
CREATE procedure creature_14385()
block: BEGIN
    DECLARE i int DEFAULT 80;
    DECLARE hp int DEFAULT 4665;
    DECLARE mana int DEFAULT 2186;
    DECLARE armor int DEFAULT 6993;
    DECLARE str int DEFAULT 181;
    DECLARE agi int DEFAULT 98;
    DECLARE sta int DEFAULT 95;
    DECLARE inte int DEFAULT 103;
    DECLARE spi int DEFAULT 90;
 SET i = 80;
 WHILE i <= 254 DO
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (14385, i+1, hp+100, mana+100, armor+10, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
	
        SET i = i + 1;
        SET hp = hp+100;
        SET mana=mana+100;
        SET armor=armor+10;
        SET str=str+1;
        SET agi=agi+1;
        SET sta=sta+1;
        SET inte=inte+1;
        SET spi=spi+1;
 END WHILE;
END $$
DELIMITER ;
call creature_14385();
drop procedure if exists creature_15352;
DELIMITER $$
CREATE procedure creature_15352()
block: BEGIN
    DECLARE i int DEFAULT 80;
    DECLARE hp int DEFAULT 8000;
    DECLARE armor int DEFAULT 9000;
    DECLARE str int DEFAULT 261;
    DECLARE agi int DEFAULT 90;
    DECLARE sta int DEFAULT 162;
    DECLARE inte int DEFAULT 165;
    DECLARE spi int DEFAULT 460;
 SET i = 80;
 WHILE i <= 254 DO
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (15352, i+1, hp+100, 0, armor+10, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
	
        SET i = i + 1;
        SET hp = hp+100;
        SET armor=armor+10;
        SET str=str+1;
        SET agi=agi+1;
        SET sta=sta+1;
        SET inte=inte+1;
        SET spi=spi+1;
 END WHILE;
END $$
DELIMITER ;
call creature_15352();
drop procedure if exists creature_17252;
DELIMITER $$
CREATE procedure creature_17252()
block: BEGIN
    DECLARE i int DEFAULT 80;
    DECLARE hp int DEFAULT 6872;
    DECLARE mana int DEFAULT 5266;
    DECLARE armor int DEFAULT 14033;
    DECLARE str int DEFAULT 314;
    DECLARE agi int DEFAULT 90;
    DECLARE sta int DEFAULT 328;
    DECLARE inte int DEFAULT 150;
    DECLARE spi int DEFAULT 209;
    DECLARE mindmg int DEFAULT 329;
    DECLARE maxdmg int DEFAULT 495;
 SET i = 80;
 WHILE i <= 254 DO
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (17252, i+1, hp+100, mana+100, armor+10, str+1, agi+1, sta+1, inte+1, spi+1, mindmg+5, maxdmg+5);
	
        SET i = i + 1;
        SET hp = hp+100;
        SET mana=mana+100;
        SET armor=armor+10;
        SET str=str+1;
        SET agi=agi+1;
        SET sta=sta+1;
        SET inte=inte+1;
        SET spi=spi+1;
        SET mindmg=mindmg+5;
        SET maxdmg=maxdmg+5;
 END WHILE;
END $$
DELIMITER ;
call creature_17252();
drop procedure if exists creature_19668;
DELIMITER $$
CREATE procedure creature_19668()
block: BEGIN
    DECLARE i int DEFAULT 80;
    DECLARE hp int DEFAULT 2937;
    DECLARE mana int DEFAULT 5496;
    DECLARE str int DEFAULT 219;
    DECLARE agi int DEFAULT 153;
    DECLARE sta int DEFAULT 90;
    DECLARE inte int DEFAULT 350;
    DECLARE spi int DEFAULT 228;
 SET i = 80;
 WHILE i <= 254 DO
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (19668, i+1, hp+100, mana+100, 0, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
	
        SET i = i + 1;
        SET hp = hp+100;
        SET mana=mana+100;
        SET str=str+1;
        SET agi=agi+1;
        SET sta=sta+1;
        SET inte=inte+1;
        SET spi=spi+1;
 END WHILE;
END $$
DELIMITER ;
call creature_19668();
drop procedure if exists creature_26101;
DELIMITER $$
CREATE procedure creature_26101()
block: BEGIN
    DECLARE i int DEFAULT 80;
    DECLARE hp int DEFAULT 1379;
    DECLARE mana int DEFAULT 2280;
    DECLARE str int DEFAULT 182;
    DECLARE agi int DEFAULT 42;
    DECLARE sta int DEFAULT 122;
    DECLARE inte int DEFAULT 404;
    DECLARE spi int DEFAULT 460;
 SET i = 80;
 WHILE i <= 254 DO
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (26101, i+1, hp+100, mana+100, 0, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
	
        SET i = i + 1;
        SET hp = hp+100;
        SET mana=mana+100;
        SET str=str+1;
        SET agi=agi+1;
        SET sta=sta+1;
        SET inte=inte+1;
        SET spi=spi+1;
 END WHILE;
END $$
DELIMITER ;
call creature_26101();
drop procedure if exists creature_26125;
DELIMITER $$
CREATE procedure creature_26125()
block: BEGIN
    DECLARE i int DEFAULT 80;
    DECLARE hp int DEFAULT 5338;
    DECLARE mana int DEFAULT 2134;
    DECLARE armor int DEFAULT 4513;
    DECLARE str int DEFAULT 331;
    DECLARE agi int DEFAULT 856;
    DECLARE sta int DEFAULT 361;
    DECLARE inte int DEFAULT 65;
    DECLARE spi int DEFAULT 109;
 SET i = 80;
 WHILE i <= 254 DO
        INSERT INTO pet_levelstats (creature_entry, level, hp, mana, armor, str, agi, sta, inte, spi, min_dmg, max_dmg) VALUES (26125, i+1, hp+100, mana+100, armor+10, str+1, agi+1, sta+1, inte+1, spi+1, 0, 0);
	
        SET i = i + 1;
        SET hp = hp+100;
        SET mana=mana+100;
        SET armor=armor+10;
        SET str=str+1;
        SET agi=agi+1;
        SET sta=sta+1;
        SET inte=inte+1;
        SET spi=spi+1;
 END WHILE;
END $$
DELIMITER ;
call creature_26125();