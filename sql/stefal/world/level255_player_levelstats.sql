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