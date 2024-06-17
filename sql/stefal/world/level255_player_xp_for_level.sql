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