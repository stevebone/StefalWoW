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
