DROP PROCEDURE DropColumnIfExists;
DELIMITER //

CREATE PROCEDURE DropColumnIfExists()
BEGIN
    DECLARE column_exists INT DEFAULT 0;

    -- Check if the column exists
    SELECT COUNT(*) INTO column_exists 
    FROM information_schema.columns 
    WHERE table_schema = DATABASE() 
      AND TABLE_NAME = 'creature' 
      AND COLUMN_NAME = 'size';

    -- If the column exists, drop it
    IF column_exists > 0 THEN
        ALTER TABLE creature DROP COLUMN size;
    END IF;
END //

DELIMITER ;
CALL DropColumnIfExists();

DROP PROCEDURE DropColumnIfExists;
DELIMITER //

CREATE PROCEDURE DropColumnIfExists()
BEGIN
    DECLARE column_exists INT DEFAULT 0;

    -- Check if the column exists
    SELECT COUNT(*) INTO column_exists 
    FROM information_schema.columns 
    WHERE table_schema = DATABASE() 
      AND TABLE_NAME = 'gameobject' 
      AND COLUMN_NAME = 'size';

    -- If the column exists, drop it
    IF column_exists > 0 THEN
        ALTER TABLE gameobject DROP COLUMN size;
    END IF;
END //

DELIMITER ;
CALL DropColumnIfExists();

DROP PROCEDURE DropColumnIfExists;
DELIMITER //

CREATE PROCEDURE DropColumnIfExists()
BEGIN
    DECLARE column_exists INT DEFAULT 0;

    -- Check if the column exists
    SELECT COUNT(*) INTO column_exists 
    FROM information_schema.columns 
    WHERE table_schema = DATABASE() 
      AND TABLE_NAME = 'gameobject' 
      AND COLUMN_NAME = 'visibility';

    -- If the column exists, drop it
    IF column_exists > 0 THEN
        ALTER TABLE gameobject DROP COLUMN visibility;
    END IF;
END //

DELIMITER ;
CALL DropColumnIfExists();
DROP PROCEDURE DropColumnIfExists;