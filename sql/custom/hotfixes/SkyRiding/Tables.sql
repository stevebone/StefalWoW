DROP TABLE IF EXISTS `drive_capability`;
CREATE TABLE `drive_capability` (
    `ID`                INT UNSIGNED NOT NULL,
    `ForwardAcceleration` FLOAT NOT NULL,
    `BackwardMaxSpeed`    FLOAT NOT NULL,
    `IdleFriction`        FLOAT NOT NULL,
    `BackwardAcceleration` FLOAT NOT NULL,
    `Field_5`             FLOAT NOT NULL,
    `Field_6`             FLOAT NOT NULL,
    `Field_7`             FLOAT NOT NULL,
    `Field_8`             FLOAT NOT NULL,
    `Field_9`             FLOAT NOT NULL,
    `Field_10`            FLOAT NOT NULL,
    `Field_11`            FLOAT NOT NULL,
    `Field_12`            FLOAT NOT NULL,
    `Field_13`            FLOAT NOT NULL,
    `Field_14`            FLOAT NOT NULL,
    `Field_15`            FLOAT NOT NULL,
    `Field_16`            FLOAT NOT NULL,
    `VerifiedBuild`       INT NOT NULL DEFAULT 0,
    PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

DROP TABLE IF EXISTS `drive_capability_tier`;
CREATE TABLE `drive_capability_tier` (
    `ID`                INT UNSIGNED NOT NULL,
    `Acceleration`      FLOAT NOT NULL,
    `MaxSpeed`          FLOAT NOT NULL,
    `DriveCapabilityID` INT NOT NULL,
    `OrderIndex`        INT NOT NULL,
    `VerifiedBuild`     INT NOT NULL DEFAULT 0,
    PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;