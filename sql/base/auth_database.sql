-- MySQL dump 10.13  Distrib 8.0.42, for Linux (x86_64)
--
-- Host: localhost    Database: auth
-- ------------------------------------------------------
-- Server version	8.0.42-0ubuntu0.22.04.1

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!50503 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `account`
--

DROP TABLE IF EXISTS `account`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `account` (
  `id` int unsigned NOT NULL AUTO_INCREMENT COMMENT 'Identifier',
  `username` varchar(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `salt` binary(32) NOT NULL,
  `verifier` binary(32) NOT NULL,
  `session_key_auth` binary(40) DEFAULT NULL,
  `session_key_bnet` varbinary(64) DEFAULT NULL,
  `totp_secret` varbinary(128) DEFAULT NULL,
  `email` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `reg_mail` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `joindate` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `last_ip` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '127.0.0.1',
  `last_attempt_ip` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '127.0.0.1',
  `failed_logins` int unsigned NOT NULL DEFAULT '0',
  `locked` tinyint unsigned NOT NULL DEFAULT '0',
  `lock_country` varchar(2) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '00',
  `last_login` timestamp NULL DEFAULT NULL,
  `online` tinyint unsigned NOT NULL DEFAULT '0',
  `expansion` tinyint unsigned NOT NULL DEFAULT '10',
  `mutetime` bigint NOT NULL DEFAULT '0',
  `mutereason` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `muteby` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `client_build` int unsigned NOT NULL DEFAULT '0',
  `locale` tinyint unsigned NOT NULL DEFAULT '0',
  `os` varchar(4) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `timezone_offset` smallint NOT NULL DEFAULT '0',
  `recruiter` int unsigned NOT NULL DEFAULT '0',
  `battlenet_account` int unsigned DEFAULT NULL,
  `battlenet_index` tinyint unsigned DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `idx_username` (`username`),
  UNIQUE KEY `uk_bnet_acc` (`battlenet_account`,`battlenet_index`),
  CONSTRAINT `fk_bnet_acc` FOREIGN KEY (`battlenet_account`) REFERENCES `battlenet_accounts` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Account System';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `account`
--

LOCK TABLES `account` WRITE;
/*!40000 ALTER TABLE `account` DISABLE KEYS */;
/*!40000 ALTER TABLE `account` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `account_access`
--

DROP TABLE IF EXISTS `account_access`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `account_access` (
  `AccountID` int unsigned NOT NULL,
  `SecurityLevel` tinyint unsigned NOT NULL,
  `RealmID` int NOT NULL DEFAULT '-1',
  `Comment` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  PRIMARY KEY (`AccountID`,`RealmID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `account_access`
--

LOCK TABLES `account_access` WRITE;
/*!40000 ALTER TABLE `account_access` DISABLE KEYS */;
/*!40000 ALTER TABLE `account_access` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `account_banned`
--

DROP TABLE IF EXISTS `account_banned`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `account_banned` (
  `id` int unsigned NOT NULL DEFAULT '0' COMMENT 'Account id',
  `bandate` int unsigned NOT NULL DEFAULT '0',
  `unbandate` int unsigned NOT NULL DEFAULT '0',
  `bannedby` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `banreason` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `active` tinyint unsigned NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`,`bandate`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Ban List';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `account_banned`
--

LOCK TABLES `account_banned` WRITE;
/*!40000 ALTER TABLE `account_banned` DISABLE KEYS */;
/*!40000 ALTER TABLE `account_banned` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `account_last_played_character`
--

DROP TABLE IF EXISTS `account_last_played_character`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `account_last_played_character` (
  `accountId` int unsigned NOT NULL,
  `region` tinyint unsigned NOT NULL,
  `battlegroup` tinyint unsigned NOT NULL,
  `realmId` int unsigned DEFAULT NULL,
  `characterName` varchar(12) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `characterGUID` bigint unsigned DEFAULT NULL,
  `lastPlayedTime` int unsigned DEFAULT NULL,
  PRIMARY KEY (`accountId`,`region`,`battlegroup`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `account_last_played_character`
--

LOCK TABLES `account_last_played_character` WRITE;
/*!40000 ALTER TABLE `account_last_played_character` DISABLE KEYS */;
/*!40000 ALTER TABLE `account_last_played_character` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `account_muted`
--

DROP TABLE IF EXISTS `account_muted`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `account_muted` (
  `guid` int unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `mutedate` int unsigned NOT NULL DEFAULT '0',
  `mutetime` int unsigned NOT NULL DEFAULT '0',
  `mutedby` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `mutereason` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  PRIMARY KEY (`guid`,`mutedate`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='mute List';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `account_muted`
--

LOCK TABLES `account_muted` WRITE;
/*!40000 ALTER TABLE `account_muted` DISABLE KEYS */;
/*!40000 ALTER TABLE `account_muted` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `autobroadcast`
--

DROP TABLE IF EXISTS `autobroadcast`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `autobroadcast` (
  `realmid` int NOT NULL DEFAULT '-1',
  `id` tinyint unsigned NOT NULL AUTO_INCREMENT,
  `weight` tinyint unsigned DEFAULT '1',
  `text` longtext CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  PRIMARY KEY (`id`,`realmid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `autobroadcast`
--

LOCK TABLES `autobroadcast` WRITE;
/*!40000 ALTER TABLE `autobroadcast` DISABLE KEYS */;
/*!40000 ALTER TABLE `autobroadcast` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `battle_pet_declinedname`
--

DROP TABLE IF EXISTS `battle_pet_declinedname`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `battle_pet_declinedname` (
  `guid` bigint NOT NULL,
  `genitive` varchar(12) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `dative` varchar(12) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `accusative` varchar(12) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `instrumental` varchar(12) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `prepositional` varchar(12) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  PRIMARY KEY (`guid`),
  CONSTRAINT `fk_battle_pet__battle_pet_declinedname` FOREIGN KEY (`guid`) REFERENCES `battle_pets` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `battle_pet_declinedname`
--

LOCK TABLES `battle_pet_declinedname` WRITE;
/*!40000 ALTER TABLE `battle_pet_declinedname` DISABLE KEYS */;
/*!40000 ALTER TABLE `battle_pet_declinedname` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `battle_pet_slots`
--

DROP TABLE IF EXISTS `battle_pet_slots`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `battle_pet_slots` (
  `id` tinyint NOT NULL,
  `battlenetAccountId` int NOT NULL,
  `battlePetGuid` bigint NOT NULL,
  `locked` tinyint NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`,`battlenetAccountId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `battle_pet_slots`
--

LOCK TABLES `battle_pet_slots` WRITE;
/*!40000 ALTER TABLE `battle_pet_slots` DISABLE KEYS */;
/*!40000 ALTER TABLE `battle_pet_slots` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `battle_pets`
--

DROP TABLE IF EXISTS `battle_pets`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `battle_pets` (
  `guid` bigint NOT NULL,
  `battlenetAccountId` int NOT NULL,
  `species` int NOT NULL,
  `breed` smallint NOT NULL,
  `displayId` int NOT NULL DEFAULT '0',
  `level` smallint NOT NULL DEFAULT '1',
  `exp` smallint NOT NULL DEFAULT '0',
  `health` int NOT NULL DEFAULT '1',
  `quality` tinyint NOT NULL DEFAULT '0',
  `flags` smallint NOT NULL DEFAULT '0',
  `name` varchar(12) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `nameTimestamp` bigint NOT NULL DEFAULT '0',
  `owner` bigint DEFAULT NULL,
  `ownerRealmId` int DEFAULT NULL,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `battle_pets`
--

LOCK TABLES `battle_pets` WRITE;
/*!40000 ALTER TABLE `battle_pets` DISABLE KEYS */;
/*!40000 ALTER TABLE `battle_pets` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `battlenet_account_bans`
--

DROP TABLE IF EXISTS `battlenet_account_bans`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `battlenet_account_bans` (
  `id` int unsigned NOT NULL DEFAULT '0' COMMENT 'Account id',
  `bandate` int unsigned NOT NULL DEFAULT '0',
  `unbandate` int unsigned NOT NULL DEFAULT '0',
  `bannedby` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `banreason` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  PRIMARY KEY (`id`,`bandate`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Ban List';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `battlenet_account_bans`
--

LOCK TABLES `battlenet_account_bans` WRITE;
/*!40000 ALTER TABLE `battlenet_account_bans` DISABLE KEYS */;
/*!40000 ALTER TABLE `battlenet_account_bans` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `battlenet_account_heirlooms`
--

DROP TABLE IF EXISTS `battlenet_account_heirlooms`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `battlenet_account_heirlooms` (
  `accountId` int unsigned NOT NULL,
  `itemId` int unsigned NOT NULL DEFAULT '0',
  `flags` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`accountId`,`itemId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `battlenet_account_heirlooms`
--

LOCK TABLES `battlenet_account_heirlooms` WRITE;
/*!40000 ALTER TABLE `battlenet_account_heirlooms` DISABLE KEYS */;
/*!40000 ALTER TABLE `battlenet_account_heirlooms` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `battlenet_account_mounts`
--

DROP TABLE IF EXISTS `battlenet_account_mounts`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `battlenet_account_mounts` (
  `battlenetAccountId` int unsigned NOT NULL,
  `mountSpellId` int unsigned NOT NULL,
  `flags` tinyint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`battlenetAccountId`,`mountSpellId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `battlenet_account_mounts`
--

LOCK TABLES `battlenet_account_mounts` WRITE;
/*!40000 ALTER TABLE `battlenet_account_mounts` DISABLE KEYS */;
/*!40000 ALTER TABLE `battlenet_account_mounts` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `battlenet_account_player_data_element`
--

DROP TABLE IF EXISTS `battlenet_account_player_data_element`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `battlenet_account_player_data_element` (
  `battlenetAccountId` int unsigned NOT NULL,
  `playerDataElementAccountId` int unsigned NOT NULL,
  `floatValue` float DEFAULT NULL,
  `int64Value` bigint DEFAULT NULL,
  PRIMARY KEY (`battlenetAccountId`,`playerDataElementAccountId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `battlenet_account_player_data_element`
--

LOCK TABLES `battlenet_account_player_data_element` WRITE;
/*!40000 ALTER TABLE `battlenet_account_player_data_element` DISABLE KEYS */;
/*!40000 ALTER TABLE `battlenet_account_player_data_element` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `battlenet_account_player_data_flag`
--

DROP TABLE IF EXISTS `battlenet_account_player_data_flag`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `battlenet_account_player_data_flag` (
  `battlenetAccountId` int unsigned NOT NULL,
  `storageIndex` int unsigned NOT NULL,
  `mask` bigint unsigned NOT NULL,
  PRIMARY KEY (`battlenetAccountId`,`storageIndex`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `battlenet_account_player_data_flag`
--

LOCK TABLES `battlenet_account_player_data_flag` WRITE;
/*!40000 ALTER TABLE `battlenet_account_player_data_flag` DISABLE KEYS */;
/*!40000 ALTER TABLE `battlenet_account_player_data_flag` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `battlenet_account_toys`
--

DROP TABLE IF EXISTS `battlenet_account_toys`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `battlenet_account_toys` (
  `accountId` int unsigned NOT NULL,
  `itemId` int NOT NULL DEFAULT '0',
  `isFavourite` tinyint(1) DEFAULT '0',
  `hasFanfare` tinyint(1) DEFAULT '0',
  PRIMARY KEY (`accountId`,`itemId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `battlenet_account_toys`
--

LOCK TABLES `battlenet_account_toys` WRITE;
/*!40000 ALTER TABLE `battlenet_account_toys` DISABLE KEYS */;
/*!40000 ALTER TABLE `battlenet_account_toys` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `battlenet_account_transmog_illusions`
--

DROP TABLE IF EXISTS `battlenet_account_transmog_illusions`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `battlenet_account_transmog_illusions` (
  `battlenetAccountId` int unsigned NOT NULL,
  `blobIndex` smallint unsigned NOT NULL,
  `illusionMask` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`battlenetAccountId`,`blobIndex`),
  CONSTRAINT `battlenet_account_transmog_illusions_ibfk_1` FOREIGN KEY (`battlenetAccountId`) REFERENCES `battlenet_accounts` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `battlenet_account_transmog_illusions`
--

LOCK TABLES `battlenet_account_transmog_illusions` WRITE;
/*!40000 ALTER TABLE `battlenet_account_transmog_illusions` DISABLE KEYS */;
/*!40000 ALTER TABLE `battlenet_account_transmog_illusions` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `battlenet_account_warband_scenes`
--

DROP TABLE IF EXISTS `battlenet_account_warband_scenes`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `battlenet_account_warband_scenes` (
  `battlenetAccountId` int unsigned NOT NULL,
  `warbandSceneId` int NOT NULL DEFAULT '0',
  `isFavorite` tinyint(1) DEFAULT '0',
  `hasFanfare` tinyint(1) DEFAULT '0',
  PRIMARY KEY (`battlenetAccountId`,`warbandSceneId`),
  CONSTRAINT `fk_battlenet_account_warband_scenes__accountId` FOREIGN KEY (`battlenetAccountId`) REFERENCES `battlenet_accounts` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `battlenet_account_warband_scenes`
--

LOCK TABLES `battlenet_account_warband_scenes` WRITE;
/*!40000 ALTER TABLE `battlenet_account_warband_scenes` DISABLE KEYS */;
/*!40000 ALTER TABLE `battlenet_account_warband_scenes` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `battlenet_accounts`
--

DROP TABLE IF EXISTS `battlenet_accounts`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `battlenet_accounts` (
  `id` int unsigned NOT NULL AUTO_INCREMENT COMMENT 'Identifier',
  `email` varchar(320) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `srp_version` tinyint NOT NULL DEFAULT '1',
  `salt` binary(32) NOT NULL,
  `verifier` blob NOT NULL,
  `joindate` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `last_ip` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '127.0.0.1',
  `failed_logins` int unsigned NOT NULL DEFAULT '0',
  `locked` tinyint unsigned NOT NULL DEFAULT '0',
  `lock_country` varchar(2) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '00',
  `last_login` timestamp NULL DEFAULT NULL,
  `online` tinyint unsigned NOT NULL DEFAULT '0',
  `locale` tinyint unsigned NOT NULL DEFAULT '0',
  `os` varchar(4) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `LastCharacterUndelete` int unsigned NOT NULL DEFAULT '0',
  `LoginTicket` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `LoginTicketExpiry` int unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Account System';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `battlenet_accounts`
--

LOCK TABLES `battlenet_accounts` WRITE;
/*!40000 ALTER TABLE `battlenet_accounts` DISABLE KEYS */;
/*!40000 ALTER TABLE `battlenet_accounts` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `battlenet_item_appearances`
--

DROP TABLE IF EXISTS `battlenet_item_appearances`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `battlenet_item_appearances` (
  `battlenetAccountId` int unsigned NOT NULL,
  `blobIndex` smallint unsigned NOT NULL,
  `appearanceMask` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`battlenetAccountId`,`blobIndex`),
  CONSTRAINT `fk_battlenet_item_appearances` FOREIGN KEY (`battlenetAccountId`) REFERENCES `battlenet_accounts` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `battlenet_item_appearances`
--

LOCK TABLES `battlenet_item_appearances` WRITE;
/*!40000 ALTER TABLE `battlenet_item_appearances` DISABLE KEYS */;
/*!40000 ALTER TABLE `battlenet_item_appearances` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `battlenet_item_favorite_appearances`
--

DROP TABLE IF EXISTS `battlenet_item_favorite_appearances`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `battlenet_item_favorite_appearances` (
  `battlenetAccountId` int unsigned NOT NULL,
  `itemModifiedAppearanceId` int unsigned NOT NULL,
  PRIMARY KEY (`battlenetAccountId`,`itemModifiedAppearanceId`),
  CONSTRAINT `fk_battlenet_item_favorite_appearances` FOREIGN KEY (`battlenetAccountId`) REFERENCES `battlenet_accounts` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `battlenet_item_favorite_appearances`
--

LOCK TABLES `battlenet_item_favorite_appearances` WRITE;
/*!40000 ALTER TABLE `battlenet_item_favorite_appearances` DISABLE KEYS */;
/*!40000 ALTER TABLE `battlenet_item_favorite_appearances` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `build_auth_key`
--

DROP TABLE IF EXISTS `build_auth_key`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `build_auth_key` (
  `build` int NOT NULL,
  `platform` char(4) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `arch` char(4) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `type` char(4) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `key` binary(16) NOT NULL,
  PRIMARY KEY (`build`,`platform`,`arch`,`type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `build_auth_key`
--

LOCK TABLES `build_auth_key` WRITE;
/*!40000 ALTER TABLE `build_auth_key` DISABLE KEYS */;
INSERT INTO `build_auth_key` VALUES
(25549,'Mac','x64','WoW',0x66FC5E09B8706126795F140308C8C1D8),
(25549,'Win','x64','WoW',0x1252624ED8CBD6FAC7D33F5D67A535F3),
(25549,'Win','x86','WoW',0xFE594FC35E7F9AFF86D99D8A364AB297),
(25996,'Mac','x64','WoW',0x210B970149D6F56CAC9BADF2AAC91E8E),
(25996,'Win','x64','WoW',0xC7FF932D6A2174A3D538CA7212136D2B),
(25996,'Win','x86','WoW',0x23C59C5963CBEF5B728D13A50878DFCB),
(26124,'Mac','x64','WoW',0xC9CA997AB8EDE1C65465CB2920869C4E),
(26124,'Win','x64','WoW',0x46DF06D0147BA67BA49AF553435E093F),
(26124,'Win','x86','WoW',0xF8C05AE372DECA1D6C81DA7A8D1C5C39),
(26365,'Mac','x64','WoW',0xDBE7F860276D6B400AAA86B35D51A417),
(26365,'Win','x64','WoW',0x59A53F307288454B419B13E694DF503C),
(26365,'Win','x86','WoW',0x2AAC82C80E829E2CA902D70CFA1A833A),
(26654,'Mac','x64','WoW',0x9234C1BD5E9687ADBD19F764F2E0E811),
(26654,'Win','x64','WoW',0xA752640E8B99FE5B57C1320BC492895A),
(26654,'Win','x86','WoW',0xFAC2D693E702B9EC9F750F17245696D8),
(26822,'Mac','x64','WoW',0x91003668C245D14ECD8DF094E065E06B),
(26822,'Win','x64','WoW',0x2B05F6D746C0C6CC7EF79450B309E595),
(26822,'Win','x86','WoW',0x283E8D77ECF7060BE6347BE4EB99C7C7),
(26899,'Mac','x64','WoW',0x8368EFC2021329110A16339D298200D4),
(26899,'Win','x64','WoW',0x3551EF0028B51E92170559BD25644B03),
(26899,'Win','x86','WoW',0xF462CD2FE4EA3EADF875308FDBB18C99),
(26972,'Mac','x64','WoW',0x341CFEFE3D72ACA9A4407DC535DED66A),
(26972,'Win','x64','WoW',0x6E212DEF6A0124A3D9AD07F5E322F7AE),
(26972,'Win','x86','WoW',0x797ECC19662DCBD5090A4481173F1D26),
(28153,'Win','x64','WoW',0xDD626517CC6D31932B479934CCDC0ABF),
(30706,'Win','x64','WoW',0xBB6D9866FE4A19A568015198783003FC),
(30993,'Win','x64','WoW',0x2BAD61655ABC2FC3D04893B536403A91),
(31229,'Win','x64','WoW',0x8A46F23670309F2AAE85C9A47276382B),
(31429,'Win','x64','WoW',0x7795A507AF9DC3525EFF724FEE17E70C),
(31478,'Win','x64','WoW',0x7973A8D54BDB8B798D9297B096E771EF),
(32305,'Win','x64','WoW',0x21F5A6FC7AD89FBF411FDA8B8738186A),
(32494,'Win','x64','WoW',0x58984ACE04919401835C61309A848F8A),
(32580,'Win','x64','WoW',0x87C2FAA0D7931BF016299025C0DDCA14),
(32638,'Win','x64','WoW',0x5D07ECE7D4A867DDDE615DAD22B76D4E),
(32722,'Win','x64','WoW',0x1A09BE1D38A122586B4931BECCEAD4AA),
(32750,'Mac','x64','WoW',0xEF1F4E4D099EA2A81FD4C0DEBC1E7086),
(32750,'Win','x64','WoW',0xC5CB669F5A5B237D1355430877173207),
(32978,'Mac','x64','WoW',0x1852C1F847E795D6EB45278CD433F339),
(32978,'Win','x64','WoW',0x76AE2EA03E525D97F5688843F5489000),
(33369,'Mac','x64','WoW',0xF5A849C70A1054F07EA3AB833EBF6671),
(33369,'Win','x64','WoW',0x5986AC18B04D3C403F56A0CF8C4F0A14),
(33528,'Win','x64','WoW',0x0ECE033CA9B11D92F7D2792C785B47DF),
(33724,'Win','x64','WoW',0x38F7BBCF284939DD20E8C64CDBF9FE77),
(33775,'Mac','x64','WoW',0x354D2DE619D124EE1398F76B0436FCFC),
(33775,'Win','x64','WoW',0xB826300A8449ED0F6EF16EA747FA2D2E),
(33941,'Win','x64','WoW',0x88AF1A36D2770D0A6CA086497096A889),
(34220,'Win','x64','WoW',0xB5E35B976C6BAF82505700E7D9666A2C),
(34601,'Win','x64','WoW',0x0D7DF38F725FABA4F009257799A10563),
(34769,'Win','x64','WoW',0x93F9B9AF6397E3E4EED94D36D16907D2),
(34963,'Mac','x64','WoW',0xC5658A17E702163447BAAAE46D130A1B),
(34963,'Win','x64','WoW',0x7BA50C879C5D04221423B02AC3603A11),
(35249,'Win','x64','WoW',0xC7B11F9AE9FF1409F5582902B3D10D1C),
(35284,'Mac','x64','WoW',0xA6201B0AC5A73D13AB2FDCC79BB252AF),
(35284,'Win','x64','WoW',0xEA3818E7DCFD2009DBFC83EE3C1E4F1B),
(35435,'Mac','x64','WoW',0x8FE657C14A46BCDB2CE6DA37E430450E),
(35435,'Win','x64','WoW',0xBB397A92FE23740EA52FC2B5BA2EC8E0),
(35662,'Mac','x64','WoW',0x5966016C368ED9F7AAB603EE6703081C),
(35662,'Win','x64','WoW',0x578BC94870C278CB6962F30E6DC203BB),
(36753,'Win','x64','WoW',0x386FDE8559B5EAD67B791B490B200B88),
(36839,'Win','x64','WoW',0x356EB4412B8EFCF72E3AF50D5181D529),
(36949,'Win','x64','WoW',0x51C074CD8A09A75384B9B44188C56981),
(37142,'Win','x64','WoW',0x5D9CFB3139F0D1B6C2B304261F9EABC9),
(37176,'Win','x64','WoW',0x3C725EA504EC3DAED143EB6FF3B48CDA),
(37474,'Mac','x64','WoW',0x024C9BE7E44237B7E81C6D42E678D433),
(37474,'Win','x64','WoW',0x0DE685BBB0551086E7FBDC0B4BB06A5B),
(38134,'Win','x64','WoW',0x32275ED0F13B357C28BDB0E611EF9E31),
(38556,'Win','x64','WoW',0xEC7D5AF64364AC3E7181F3FBA1B3A882),
(39653,'Win','x64','WoW',0x10D015AB1EEB91310428D9C57EE24632),
(39804,'Win','x64','WoW',0xE42D2BBA12ED260A76F9B1E477E19EA5),
(40000,'Win','x64','WoW',0x4CB1433AB637F09F03FBBD1B221B04B0),
(40120,'Mac','x64','WoW',0x853F0F2985CEAED46DF422583CD07A7C),
(40120,'Win','x64','WoW',0x04F47EAEFD8BDEFE14AA0350EA336678),
(40443,'Win','x64','WoW',0x8597BB43E8AB38C85504E8BFB72ABBF5),
(40593,'Win','x64','WoW',0xBA14570F2D62D5F61953394164A8DAE2),
(40725,'Win','x64','WoW',0xC1EBDBEB9BB2956EBCCEF7C9D27A1B3B),
(40906,'Win','x64','WoW',0xF5FC259C8635488AFE0D0CD023F361D4),
(40944,'Win','x64','WoW',0x368FC7FABAF487A8A049C11970657074),
(40966,'Win','x64','WoW',0xD90F47AF21F381D2D8F3763B994BAC88),
(41031,'Win','x64','WoW',0x019A0FACD6B0D6374B7BA69A5B677449),
(41079,'Win','x64','WoW',0xF8853CF823BC0BBE8A9677A762DFAEE1),
(41288,'Win','x64','WoW',0x871C0C9691DBC536EB24B68EC73FAD5B),
(41323,'Win','x64','WoW',0xE53D0DF1FAC1A59A1C8071B295A04A1D),
(41359,'Win','x64','WoW',0x5F8D7F2A690A4375A1B52A28D6D681FA),
(41488,'Win','x64','WoW',0x1BC91EC368705815F3F532B553DAD981),
(41793,'Win','x64','WoW',0xB3B47DA3B7615570742A55B96614EE1C),
(42010,'Win','x64','WoW',0x302970161D16417B5BE553CC530E011A),
(42423,'Win','x64','WoW',0x0B03614A7E94DD57548596BE420E9DC2),
(42488,'Win','x64','WoW',0xA78755E6928D83A271C5D1EE3CDB6F15),
(42521,'Win','x64','WoW',0x5FE6C12FC407C6B15B4A5D3B5B4A5D3B),
(42538,'Win','x64','WoW',0x71A7504BD53F8DE518F24265D37310AE),
(42560,'Win','x64','WoW',0x115FE8C38A8D67CA4664BB192E0F0DFE),
(42614,'Win','x64','WoW',0x772BE726FEEF42124255D2EA7973CA18),
(42698,'Win','x64','WoW',0xB4497B1CD11FC974C5FB09548AC27269),
(42825,'Win','x64','WoW',0xA14DA228C6A6AFF1DDBA51218939E557),
(42852,'Win','x64','WoW',0xDE9F9F0C3CC8FD54D3AFF99CEFFCE129),
(42937,'Win','x64','WoW',0xF5FC75E70874752C92846B3333920E63),
(42979,'Win','x64','WoW',0xE1DD38AE6450FC4D2AE4609233C59E54),
(43114,'Win','x64','WoW',0xF75C9380CCB24A48A24EEE52C1594A7E),
(43206,'Win','x64','WoW',0xDDE806532C7704FFB75F256DC5F1F3D9),
(43340,'Win','x64','WoW',0x70E46D2D888E84DF9316EA849B068CF4),
(43345,'Win','x64','WoW',0xD911ABFCDA030DEE8CAF4EE3F60DEE13),
(43971,'Win','x64','WoW',0x681CF99E61FB0005A5C7D31D0AAD1ED9),
(44015,'Win','x64','WoW',0xFCF0BDA7C98BFEF92AE6D8C39A217ABD),
(44061,'Win','x64','WoW',0xFD2B5C0B3293FE0E9CAA6EB0B7788119),
(44127,'Win','x64','WoW',0x787887CEC9FCC9B516481F60E4FC34A8),
(44232,'Win','x64','WoW',0x81F0A71DF7E9873BB3750022D64D33CF),
(44325,'Win','x64','WoW',0x138A7D524D268A7F9934C3D148E8F01B),
(44730,'Win','x64','WoW',0xFC0B18C47BB4C79F4300CA0FF3E5CAC7),
(44908,'Win','x64','WoW',0xBFFAEC40C9BCD591C7C959A9D5A8BA8C),
(45114,'Win','x64','WoW',0xD7AFE240BD00F06C30D0C2D16E54A8BE),
(45161,'Win','x64','WoW',0x74BD2E787A98B145B063BDA9A98F6CBD),
(45338,'Win','x64','WoW',0x5CE2094A41B61EDA9DF56378BC3B1DE0),
(45745,'Win','x64','WoW',0x0F6DC90161694D765A595A3AF603166B),
(46479,'Win','x64','WoW',0xCB9AF4D89B60A3ABA288D395D315D932),
(46658,'Win','x64','WoW',0x3F8EFB085428D75360E9EFE25CD8639A),
(46689,'Win','x64','WoW',0xD9A11D188D6AD60906F5467510EFD3AA),
(46702,'Win','x64','WoW',0x01B4D1688FF97DC9AAFCCD0A0B842C0B),
(46741,'Win','x64','WoW',0x4C0F4A7EC2098AF1FBA745848EC79A78),
(46801,'Win','x64','WoW',0xE6AC18D1EA5D36ABFFAE5EDED80630DF),
(46879,'Win','x64','WoW',0xEFEC43936051DD1A210633AF1F6B63DB),
(46924,'Win','x64','WoW',0xE6CE0B1A8119F069ECF1E7DBAA7BB2F8),
(47067,'Win','x64','WoW',0x63862CFCDEA6BD2BD7F740EB36B65657),
(47187,'Win','x64','WoW',0x711F8455C5000C237292E1E6E90631E1),
(47213,'Win','x64','WoW',0x23C50D88CEAC0A8696ADDE8DD244D4A2),
(47631,'Win','x64','WoW',0xF986AB91D0AEB20822EFB72F4256713C),
(47777,'Win','x64','WoW',0xA88C04915AB9E035A104E55C4DCF5F9F),
(47799,'Win','x64','WoW',0x7364EB093C23DB2CDC9513D5A7B4933E),
(47825,'Win','x64','WoW',0x82A3B94E5E727AF3A208B471FF2054C0),
(47849,'Win','x64','WoW',0xDD8BBE2087A28C0AF4984CBE23A1C707),
(47871,'Win','x64','WoW',0x8E4F7D30EE4982B02B3B3F8837C2C4F2),
(47884,'Win','x64','WoW',0x2B7A002BC359F2C31104BC2DE04302BF),
(47936,'Win','x64','WoW',0x833D30D8FBC43B3FAE99CD3898D70849),
(47967,'Win','x64','WoW',0xCFE225D0089E224D9E7541D3B5C26478),
(48001,'Win','x64','WoW',0x4B0260A37BD95B615E71048469E6D5BB),
(48069,'Win','x64','WoW',0x558CDF958FA082E95849779C7C6945E5),
(48317,'Win','x64','WoW',0xC096E37B45B43244E9C79916604DD4AF),
(48397,'Win','x64','WoW',0x64BA8779EAA97E6C57982B6B1A5B32E7),
(48526,'Win','x64','WoW',0xD5B7D3303A2A741E6913EE1AEB0BCB65),
(48676,'Win','x64','WoW',0xE059FB74DFF6438CC20C7F28900F64CA),
(48749,'Win','x64','WoW',0x92DBCCA0E33DFB8A17A2B6A39246B288),
(48838,'Win','x64','WoW',0x9E6F4E1E46EF228D2DE90F7BC48AAA96),
(48865,'Win','x64','WoW',0x4B774ABE7B34D6702571B4279A4B6A13),
(48892,'Win','x64','WoW',0xAA31BF27458321B03A1A346964DD7B9D),
(48966,'Win','x64','WoW',0x823142CA131FBB715FF55D4343E55C6D),
(48999,'Win','x64','WoW',0x79BA6FF0F9672EEF875F64155C8B62D4),
(49267,'Win','x64','WoW',0xEEE77EA5A216E0731ADBB41AEFB1DF31),
(49318,'Win','x64','WoW',0xAF439AEE62EE48B36C1725111E3D9BBF),
(49343,'Win','x64','WoW',0x301A0B4C0942B9B6F605B903AD6C1F60),
(49407,'Win','x64','WoW',0x6413820DC9885BB0693B37090CBB2F30),
(49426,'Win','x64','WoW',0xD85EDFBFE9A94A55E2B4510BE41C19B2),
(49444,'Win','x64','WoW',0x363B2B05285BDD8857419D2866316D3C),
(49474,'Win','x64','WoW',0x44A7D2B352EE3D098A3CB4C2F1065E37),
(49570,'Win','x64','WoW',0xB024DE67ACAEB9E8EE6DB38DC53E8281),
(49679,'Win','x64','WoW',0x9CE59B68D8797EBF00581F41138F4316),
(49741,'Win','x64','WoW',0x0EF181E2BB0E946CF3B7422ADEB6CD1A),
(49801,'Win','x64','WoW',0x0832179567B66CA85DBD5678B604C683),
(49890,'Win','x64','WoW',0x22A5B8A1EB797A64995F705B3DBCB14C),
(50000,'Win','x64','WoW',0x02F06FFA2296FD66384295DBFD5A4C91),
(50401,'Win','x64','WoW',0x3EEF52D902CCE81D16D0E255F0AA4938),
(50438,'Win','x64','WoW',0x0B5F68F06B129CB4C57702F6D30F260B),
(50467,'Win','x64','WoW',0x5E996B1CDCEE68432D6340138E68D1EB),
(50469,'Win','x64','WoW',0x1768CCB6589E16AB3BEFA9D608A393A2),
(50504,'Win','x64','WoW',0x7D5FD20C0B32C9AF5DD65433B391D49C),
(50585,'Win','x64','WoW',0xC4F7CC38A3B84935A485F7EDAD3E764B),
(50622,'Win','x64','WoW',0xD23A26FD75FD9A6073EB7060AA28E6A7),
(50747,'Win','x64','WoW',0x2D3C386A9C45C27304ED3A3C6EB3F7C8),
(50791,'Win','x64','WoW',0x0BE7D0BB07EF37C25CBC682409091EA0),
(51130,'Win','x64','WoW',0x44CD2C91E4F0655DA387483726CE4035),
(51187,'Win','x64','WoW',0x74E2055D3965269447B5CB1B31FC71C6),
(51237,'Win','x64','WoW',0xC8660A21B766646FBD67F481CFCF55C3),
(51261,'Win','x64','WoW',0x1BEBB57AE450331E9F8C301AA7876FAB),
(51313,'Win','x64','WoW',0x35419ED0AB16735CF720858F45DC300C),
(51421,'Win','x64','WoW',0x45E24D6F3335269787DF2B2063939002),
(51485,'Win','x64','WoW',0xEC549E1D0A5DD85C03E7A9D93B7DC6D1),
(51536,'Win','x64','WoW',0x570EEA10A8EC169C3FF9621D1B635BB4),
(51754,'Win','x64','WoW',0xBED5A861C071AB41FEF6087E0C37BB1A),
(51886,'Win','x64','WoW',0x09CF8919FD2EABDEAEBC0C810F53B511),
(51972,'Win','x64','WoW',0x444DC7EF3544B6670C18884DADA00428),
(52038,'Win','x64','WoW',0xA8EF004ADED8A3AFF5A67D2BB8D95795),
(52068,'Win','x64','WoW',0xA44F842BACCC7EE8E2975FAF01F12474),
(52095,'Win','x64','WoW',0xBA36382887D16D274EA9149695F0C9C8),
(52106,'Win','x64','WoW',0x95F43869B7D881212CBC1690B8F393ED),
(52129,'Win','x64','WoW',0x02DD842F2A7162EEB8FD5B9D325606F8),
(52148,'Win','x64','WoW',0x8A969717C8CDC6E7FF4C54D5CB00C224),
(52188,'Win','x64','WoW',0x977DF9993E94855DED5E328BA7A2F21F),
(52301,'Win','x64','WoW',0x821AA3BB237B400B82F44970250539AA),
(52393,'Win','x64','WoW',0xB013ED23B7EF51B29A45594D9BBB0D03),
(52485,'Win','x64','WoW',0x5805CEB4650730AE489258DD30E34441),
(52545,'Win','x64','WoW',0xFB52179A8355A46EDBFBDC8E8E5CDAFD),
(52607,'Win','x64','WoW',0x8F002E4AADCAEABB08ABC2880B31AD60),
(52649,'Win','x64','WoW',0xD0B779FBECEBC1ED5A85D83F03C8A75B),
(52808,'Win','x64','WoW',0x6276712B6C8AEA21CD5D94D52FEE70EE),
(52902,'Win','x64','WoW',0xD4F0A24CDF165628538C1C387A326AF3),
(52968,'Win','x64','WoW',0x2D247FD440C44D4F1BF80B075B8720F2),
(52983,'Win','x64','WoW',0xB1E5ADA5FDD06C9AB5E5D8A6983324AC),
(53007,'Win','x64','WoW',0xA21AFB4D381C56AF471D994258C0EEF5),
(53040,'Win','x64','WoW',0x2F1283BF7B7F307B70DBBD75CC42D7C3),
(53104,'Win','x64','WoW',0xDBD79EC8DF044B53C78931B985CAB406),
(53162,'Win','x64','WoW',0x8A67511FBF8984EEE2B630F7CB23376A),
(53212,'Win','x64','WoW',0x08761EFF2F9B639364B9A9FBFFFFB949),
(53262,'Win','x64','WoW',0x614A72D53126348A4927EC0F53FD2B7A),
(53441,'Win','x64','WoW',0xBFDD7D0FE87D5F75E6DEB4F5C99D7C99),
(53584,'Win','x64','WoW',0xCDD7A93659A03460B5A6CE1C4ACE5554),
(53840,'Win','x64','WoW',0xAC97D745C60DD3DC5F973E55C0E3649E),
(53877,'Win','x64','WoW',0x16320F95B63846A2276E1D2612C34AD4),
(53913,'Win','x64','WoW',0x475680680B2192EBCF6744D14F755199),
(53989,'Win','x64','WoW',0x3AEB90ACB9E18B88BA1021F52D51B857),
(54070,'Win','x64','WoW',0xFDA08264B7587250CF78F9B960218169),
(54205,'Win','x64','WoW',0x589D59EB3F0D6D77C2175D9302F78FDD),
(54358,'Win','x64','WoW',0x89C733D366E845FB9264404CD48CCA89),
(54499,'Win','x64','WoW',0x6AC1F6180FC4C390F782DB933BD83326),
(54577,'Win','x64','WoW',0xCBBBBBFFB2C652F0F4A7848D86E1B063),
(54601,'Win','x64','WoW',0xF8B866CF7DB9A12273F35B3929627375),
(54604,'Win','x64','WoW',0xA5FF4CE535F69D4D7F638CB2C11F2243),
(54630,'Win','x64','WoW',0xF37935510BA6B559363C160E1DF18230),
(54673,'Win','x64','WoW',0xAF8B9C8A9C6128498E85228B965A004D),
(54717,'Win','x64','WoW',0x862BBA15158DFF6B56BCF71C1B34D41A),
(54736,'Win','x64','WoW',0xFE58F039D91E1C248886A2828127CA2F),
(54762,'Win','x64','WoW',0x5F7FDA560030255445B713456A286CFA),
(54847,'Win','x64','WoW',0x32FDA0353160EC51EB36195CFACEF2B7),
(54904,'Win','x64','WoW',0x09CBEC3C0DEA28BCA79F1FD9B879B0E3),
(54988,'Win','x64','WoW',0x8F2791553226871977B4F9D7F1137311),
(55142,'Win','x64','WoW',0x298B99B5AD67EA3D2AB4B3E07A415305),
(55165,'Win','x64','WoW',0x26D4D1C01434A8A6723DCABC944B6FD0),
(55261,'Win','x64','WoW',0x84E153A3DD1F8249274B91047AAC7AED),
(55461,'Win','x64','WoW',0xC4522674F854EFECC76C898EC9545099),
(55664,'Win','x64','WoW',0xDBCA58486FAAA0FE54EA287A3047E923),
(55666,'Win','x64','WoW',0xF7E5A88E4D3615B652C8B9D76E7F617C),
(55792,'Win','x64','WoW',0xC4DECDAA44BC548FF09EF3BB837D2147),
(55793,'Win','x64','WoW',0xF9CF3232AD1C38C2028668D5BB64198F),
(55818,'Win','x64','WoW',0x903A9B546248F71B16D9D9B06A072C24),
(55824,'Win','x64','WoW',0x8A6F13269A2896067A1E88789FB41BA7),
(55846,'Win','x64','WoW',0xBB5E1BED705872C226834B95A9E9F8A9),
(55933,'Win','x64','WoW',0xC34D42CB351C400D1319D221197CF18D),
(55939,'Win','x64','WoW',0x91529F4CE41DE4E54E132660ACDCADC5),
(55959,'Win','x64','WoW',0xED57E0538FD1CE99B171BE34823148A9),
(55960,'Win','x64','WoW',0x078C056CFC7B371BDE1BAF308E6C19AF),
(56008,'Win','x64','WoW',0x00FCD6BFFCCCBA7A8E71E307EA6EBD0C),
(56110,'Win','x64','WoW',0x800BCA228E2FAE37ED160F3C09E21F3A),
(56162,'Win','x64','WoW',0x6F5C93BA436665A94DB67CE5361D4EA8),
(56196,'Mac','A64','WoW',0x778F6A5DF79A4EF1B86F651F3B303CE7),
(56196,'Win','x64','WoW',0x81FC14274BFF0347FFF9C8B1AA6BED9B),
(56263,'Win','x64','WoW',0xEA8472C99236CC617BF518A924014D56),
(56288,'Mac','A64','WoW',0x41710C793EF021721F14B06EC1896D3F),
(56288,'Win','x64','WoW',0x03544E35D0480938E5C1238B3A26BDDD),
(56311,'Mac','A64','WoW',0x412D3200715AAFDC0522DF031A941F0E),
(56311,'Win','x64','WoW',0xAC6415575AEE4D32735FFAD0C57226B2),
(56313,'Mac','A64','WoW',0xA83ED19EE659BC95CC322D1E49BDEDBB),
(56313,'Win','x64','WoW',0xEAF769009A9AD3B1AA0414E1C344074B),
(56380,'Win','x64','WoW',0xD081784493233E2869933FCC01F1A3C1),
(56380,'Win','x64','WoWC',0x4FB3D72D89F5FF01DD7F4E028494A4E6),
(56382,'Win','x64','WoW',0x67C36A3EF655534C58AD50942192BFDA),
(56382,'Win','x64','WoWC',0xA5BD7A1C33039F58CD45CD5C7EBDF122),
(56421,'Mac','A64','WoW',0x5892FFABAEFDCECB0CFFAAA55D2F9B13),
(56421,'Mac','A64','WoWC',0x629666D6D9EFFE4B73C1EDD74638DEAA),
(56421,'Mac','x64','WoW',0x4F0FCD1113F783C484BF45C98327DE62),
(56421,'Mac','x64','WoWC',0x4B61E4F4F3B220FF5B48A6988F0FFB42),
(56421,'Win','A64','WoW',0x178888C71560707CC9C1C1D6B45B4838),
(56421,'Win','x64','WoW',0x3BDFA9AA4B70041F2C8B8CDE3C8DC255),
(56421,'Win','x64','WoWC',0x0DDB8F8738647F3CD8FD585A3A78ED1B),
(56461,'Mac','A64','WoW',0xE51FE6EAE45322991916F619481DB0B1),
(56461,'Mac','A64','WoWC',0x85447CD695C2DC2F618B23996B4A503C),
(56461,'Mac','x64','WoW',0xE26878DF3437782F6E93E833B4CD843A),
(56461,'Mac','x64','WoWC',0x8B60B0DDD55907673FD1C11EDAE58859),
(56461,'Win','A64','WoW',0x2F862AB724092B99C8A1F6A7B53F6A9D),
(56461,'Win','x64','WoW',0x153CD89E30C4D5C3DBE603EECB470FB4),
(56461,'Win','x64','WoWC',0x79BD4C88012F0DA42605D76A153722CC),
(56513,'Mac','A64','WoW',0xC7E757E0DE67619251F942B6C98A53B7),
(56513,'Mac','A64','WoWC',0x3CC0640E46D557C4D9A6250AA8D5B387),
(56513,'Mac','x64','WoW',0xD1443824E37B73CB1FD993CE2DFAA098),
(56513,'Mac','x64','WoWC',0xEAD9A0B761C4C9C812FAD54CE0D96B46),
(56513,'Win','A64','WoW',0x753468F753E8D3841B5D6B061E300501),
(56513,'Win','x64','WoW',0x9BD8D105049CC6459FE32EC7099AB60E),
(56513,'Win','x64','WoWC',0xBB5A35C2EB6E2026FFE01202FDEBD6E9),
(56625,'Mac','A64','WoW',0xC7AD97B9970260D0BDE7AA03EEF87E2D),
(56625,'Mac','A64','WoWC',0xDEBDB4589756730F5EF60B337226E515),
(56625,'Mac','x64','WoW',0xEB5665FDC81E3538944BA82E435A53E6),
(56625,'Mac','x64','WoWC',0xAA5B3910A3D13C89F3D9467058080836),
(56625,'Win','A64','WoW',0x684153873E5023671685B270DA879668),
(56625,'Win','x64','WoW',0x80859C23C3E7F827829D47A08A1B8C92),
(56625,'Win','x64','WoWC',0x606914ED212277FDDBBAE49BE6E8CCAF),
(56647,'Mac','A64','WoW',0x1911B50FA20E5DEF8C1E5D3C7479E4E2),
(56647,'Mac','A64','WoWC',0xF0F2BEC3DE590AE7A59CCFDB42201D5F),
(56647,'Mac','x64','WoW',0x022A06825C7CF55528A5ED03E87D6AE4),
(56647,'Win','A64','WoW',0x0F36C48D68E358598318F0B30FDFCB61),
(56647,'Win','x64','WoW',0x2E9C456B66C0D5CAD601E8FD7A2FEA7A),
(56647,'Win','x64','WoWC',0x543A69AB802D0873D84AB08DEC4D8CAC),
(56819,'Mac','A64','WoW',0x1553CD215A62E2DC92CD61C4FB5DF9C3),
(56819,'Mac','A64','WoWC',0x1B7EF05D342064F931E53264031E3497),
(56819,'Mac','x64','WoW',0x9D5B7E1B5E59E54A3B96E5CCBA6EF465),
(56819,'Mac','x64','WoWC',0x2BDCD952BAA47E6CBD71C780BC8CCD47),
(56819,'Win','A64','WoW',0x7DC752479B560884B8289BC5B5C85F3F),
(56819,'Win','x64','WoW',0x4373F45E9D617636332569671ED5D5E9),
(56819,'Win','x64','WoWC',0xB9E422DF76DA635E193B75BD54ADD8DE),
(57171,'Mac','A64','WoW',0x4327288A88EBDC1EBE58F27BA30F69E6),
(57171,'Mac','A64','WoWC',0x2279265138F9516F382CA160CABDB128),
(57171,'Mac','x64','WoW',0x28D502237C9E250B31CE081A6C540D7F),
(57171,'Mac','x64','WoWC',0x36B3B18FA95E1F153289340FA437C311),
(57171,'Win','A64','WoW',0x9F1E52B1F144DA385F8C6F1E9167D057),
(57171,'Win','x64','WoW',0x967B99503C5D7A0B8B47941E16F60948),
(57171,'Win','x64','WoWC',0x3E17E628F236DA642C9EB0B8624C8499),
(57212,'Mac','A64','WoW',0x5AF00EAFB241FAA2AF7485FA62AA094E),
(57212,'Mac','A64','WoWC',0xA4B115B6BA907AEC5AEF622F813BD81C),
(57212,'Mac','x64','WoW',0xFB59E41DAC6FF691A87A20696B3AC44E),
(57212,'Mac','x64','WoWC',0x32EB0F3BB5EA60EEC4945F71DBD2B4F3),
(57212,'Win','A64','WoW',0xA1D949D062EC49E5A2978BD1932C8D0E),
(57212,'Win','x64','WoW',0xB8ADB57D27EF98C8DDEF9943BE07EE94),
(57212,'Win','x64','WoWC',0xD18B79CEF34B5E10BA8C779EFE6D9EA0),
(57292,'Mac','A64','WoW',0xAB48CDD3449440EAA6E72D43914A3824),
(57292,'Mac','A64','WoWC',0x46E6FC16392B1B0B686BDC12C92A6D18),
(57292,'Mac','x64','WoW',0x375B0734567192503CC9F9C8028DC6F3),
(57292,'Mac','x64','WoWC',0xE59F7530C7BFF77CB62E62D4E6D3EEC8),
(57292,'Win','A64','WoW',0xF42B862D0559CCD3204B2DF58E995761),
(57292,'Win','x64','WoW',0xED8353327D5538F75FAF2E0C584D2EF3),
(57292,'Win','x64','WoWC',0x67FC9D56CC96C6F4ACE29151375962F4),
(57388,'Mac','A64','WoW',0xFA9D6F044063948AB74FB3FDE335C0D5),
(57388,'Mac','A64','WoWC',0x3962974506F57EF33BBBD4C904A82157),
(57388,'Mac','x64','WoW',0xE7E3432357BC634EBD7B76BA40D51FD2),
(57388,'Mac','x64','WoWC',0xBFFDD44D892A2143B61BB59E448E348C),
(57388,'Win','A64','WoW',0x92129DE92D2BA5F67581C5DE642B1289),
(57388,'Win','x64','WoW',0x16F9CB724C662E3E8029A442443C6305),
(57388,'Win','x64','WoWC',0xDFD9BFFD00FBEFECBF0AEF0FFC9E8F2D),
(57534,'Mac','A64','WoW',0x073B823E8991A0DF012AE33D661A6060),
(57534,'Mac','A64','WoWC',0xC8824559DC24858EA7E3BD0971F98A15),
(57534,'Mac','x64','WoW',0x0C7C67AAAD4011AA9D81AB838B255CDF),
(57534,'Mac','x64','WoWC',0xCE9CD9A02B04F0C3B126F3C7E1BF7228),
(57534,'Win','A64','WoW',0xCC612F1485F158C8B2730B7DFD926563),
(57534,'Win','x64','WoW',0x948857BF76A28FD24C5A06D18B4F2FCF),
(57534,'Win','x64','WoWC',0x62B0812A4518C21360685D4422C2A312),
(57637,'Mac','A64','WoW',0x25DB682BB3B56D01B3780C446811956E),
(57637,'Mac','A64','WoWC',0x722FCA435AE1E916A4D96EDF8DDCFB3B),
(57637,'Mac','x64','WoW',0xCBB255E59E1031F8CBEB098D26B8E2FA),
(57637,'Mac','x64','WoWC',0x8DA67ECB7268B7FE759CE94F13165B32),
(57637,'Win','A64','WoW',0x4B39C74A7035B6A1C799F0F4FC6735D2),
(57637,'Win','x64','WoW',0x6C4185E973C9CB8382D3721AF1060C66),
(57637,'Win','x64','WoWC',0x71B57F027872393C902BDCBBDDC43A26),
(57689,'Mac','A64','WoW',0x5D064F1364C285F07FF804DCE98A6CC5),
(57689,'Mac','A64','WoWC',0xAB3DF8B79F5BAE839764988EE6530101),
(57689,'Mac','x64','WoW',0x988459790B4B38301350BE4286F979A2),
(57689,'Mac','x64','WoWC',0xCA7AD91741F8E2063CE227D475247212),
(57689,'Win','A64','WoW',0x4910E2142A21616934CD4D714691748C),
(57689,'Win','x64','WoW',0x93B4E50E651A28D8CC4E7CE7447901F6),
(57689,'Win','x64','WoWC',0x6AEDD8258D4E9701F737CBA852728A74),
(58123,'Mac','A64','WoW',0x7C63CE8A068940D4D019C7CF999E3DC7),
(58123,'Mac','A64','WoWC',0xCF2625300DA905CC9B6D10379C58B590),
(58123,'Mac','x64','WoW',0x1B68981860A6EB24BEAA070FD022B3B9),
(58123,'Mac','x64','WoWC',0x3037FB1C6A73F7468DD11180000BB133),
(58123,'Win','A64','WoW',0x84B75C8F0FF380C3B3970EB63AFB4962),
(58123,'Win','x64','WoW',0x29050AA0D62EF99634CCB7B5F9F6AC1E),
(58123,'Win','x64','WoWC',0x2D5EEEAB57AFFE31856D0DEAC73944DA),
(58162,'Mac','A64','WoW',0x086BB93AF9122791E6CDA58A5F20AAC1),
(58162,'Mac','A64','WoWC',0x280BD3A4F223E1D8F3CF893BBFAEFBC5),
(58162,'Mac','x64','WoW',0x47586B32BD556FA08C966D7F4FC9FF24),
(58162,'Mac','x64','WoWC',0xE861315673972C1B986650C1242196FE),
(58162,'Win','A64','WoW',0x5B1F97D43762F0F1BEC75E99CDD18D35),
(58162,'Win','x64','WoW',0xDA3B48B9A60F4486F27BA9D8858D5688),
(58162,'Win','x64','WoWC',0x25146ED771DC629F2B8CF58126BA562C),
(58187,'Mac','A64','WoW',0x88D993926D777DC12059988E009A73D6),
(58187,'Mac','A64','WoWC',0x9E794FD5CD76EFF400837338876BA64F),
(58187,'Mac','x64','WoW',0xDE1377202943C1155607090CF884FF66),
(58187,'Mac','x64','WoWC',0xE7777DCE18DB45B7ED3E7ADF23BCB1CB),
(58187,'Win','A64','WoW',0xFFEAC8B1B547FD10EFCA8BCF30DF9F51),
(58187,'Win','x64','WoW',0xD92C35D6904B0945992B0801E53822E7),
(58187,'Win','x64','WoWC',0x8AC21CC640359F276C2F6153FAA28638),
(58238,'Mac','A64','WoW',0x87705176E4D29CD7DCA47A3DB8B20CE9),
(58238,'Mac','A64','WoWC',0xEC7206B740D162429B4ACCC637132C3D),
(58238,'Mac','x64','WoW',0x1A40852B2F5EEEE2EB80497B691C88AF),
(58238,'Mac','x64','WoWC',0x48E25221E6F84CA49685B91B96E1345A),
(58238,'Win','A64','WoW',0x6AA610F0820BE535EA7017E4C5BD857B),
(58238,'Win','x64','WoW',0x9D91F6D23C62F0CB538D8D4E4BAA4FC1),
(58238,'Win','x64','WoWC',0x7B4A168DADC9A598A40794D6165EA95C),
(58533,'Mac','A64','WoW',0xE637AA32D5DE914B119AF68CC9C9BAAB),
(58533,'Mac','A64','WoWC',0xE693DD275CEEE2D682EC973D79251977),
(58533,'Mac','x64','WoW',0xDE3723EF7EB991A1B27684E6513934D6),
(58533,'Mac','x64','WoWC',0xA93EF4385174001B69F43FF0DF345701),
(58533,'Win','A64','WoW',0x3823D2E16D6E314884CD586949524F2C),
(58533,'Win','x64','WoW',0x18490A077F4FBA0973E0B62A489EE664),
(58533,'Win','x64','WoWC',0x3DC9E22F553D631A5CA067F7809A3D2D),
(58608,'Mac','A64','WoW',0x3A3B639B7FC344EDB4314860638110F1),
(58608,'Mac','A64','WoWC',0x8F46148144EDF86BD45D23B0F23CE38B),
(58608,'Mac','x64','WoW',0x29EC11F81992F19AD9707B272419ADDA),
(58608,'Mac','x64','WoWC',0x2BF5613D82AD3A242B5DD0B140CE4416),
(58608,'Win','A64','WoW',0x6FD8AD7CCB3B7DAD45115CA93D3A68F3),
(58608,'Win','x64','WoW',0x21B3EDEB1A65388DD5988056D2BE72B2),
(58608,'Win','x64','WoWC',0xAC65E4DC868EFE4362E72BBCDD746853),
(58630,'Mac','A64','WoW',0x045A964F3B8D153E273A3AD862C5C38A),
(58630,'Mac','A64','WoWC',0x3CDC1868FD7BB4E50629421328ED8269),
(58630,'Mac','x64','WoW',0x0D58880FA5FA7A7FD5C2466CEB398601),
(58630,'Mac','x64','WoWC',0x9745E7626C0D7D0A7F57DF1BB1EA4F16),
(58630,'Win','A64','WoW',0xF74A9CB9D6918780C813E2EEE0F24DE0),
(58630,'Win','x64','WoW',0x8C9CDCC711892EAAEA793DBBF3B0C850),
(58630,'Win','x64','WoWC',0xC6A3638E0BAB3A98F40B8371E1BDAECA),
(58680,'Mac','A64','WoW',0x5EE6ABE5386E08867F642B3DFD15A9D4),
(58680,'Mac','A64','WoWC',0x22C636BB4ABBC4AD9AE8F30B6D947774),
(58680,'Mac','x64','WoWC',0xA5FF81211CDCDD60B53C14F932732969),
(58680,'Win','A64','WoW',0x9D6DC8BED265C898836593A9E654A1E1),
(58680,'Win','x64','WoW',0xC1CEFF499F2C2ABF1913E55F65CF278C),
(58680,'Win','x64','WoWC',0x0D6FC77C1D3830AAA871A9E29DCBE380),
(58773,'Mac','A64','WoW',0x52D40564B2DF8D1DC02AC698F3FCF9C1),
(58773,'Mac','A64','WoWC',0xF0EBD5EF23A71883AF137174E4B1B01A),
(58773,'Mac','x64','WoW',0xAC2A56DCB12639F43F15F43BA9A9F996),
(58773,'Mac','x64','WoWC',0x8431C2D85A026E13E3E114C7AE83F0DE),
(58773,'Win','A64','WoW',0x9ADF3B9E70DF0176CEC196480BA407A3),
(58773,'Win','x64','WoW',0x648474DEE9F16AF1C02BD63F7F3E9B01),
(58773,'Win','x64','WoWC',0x362C80C4498FFA1DE3CC0AF243E1337A),
(58867,'Mac','A64','WoW',0x0BA2958770DFBFDB933851A4AADB4950),
(58867,'Mac','A64','WoWC',0x0D747A5931DF5B155B5CC4892D63E1D8),
(58867,'Mac','x64','WoW',0xA3696DF215AAB11F4F340C5C4393FB89),
(58867,'Mac','x64','WoWC',0xB7C65FC1DA519ED5304019262FEFEFA6),
(58867,'Win','A64','WoW',0xEF8C2258197FDBC6DA5C39699BDEB7A1),
(58867,'Win','x64','WoW',0x7C1627A7325C57FAD21A29D57BBF80A6),
(58867,'Win','x64','WoWC',0x3F8E07E581847B644D6F76489477540D),
(58911,'Mac','A64','WoW',0xDEE67D3A0C9D8EDD5A94C7BD8FA1E657),
(58911,'Mac','A64','WoWC',0x6D3C242B3C76686FEEE9B18444C113C7),
(58911,'Mac','x64','WoW',0x78D3576614AED3DE8FB8C90AFF838F3B),
(58911,'Mac','x64','WoWC',0xA4C80CDBC805A19FAB8EF1BAA483DB9A),
(58911,'Win','A64','WoW',0xF02130BA7BFF2C0292F25F3C6240B2E8),
(58911,'Win','x64','WoW',0x5EF341D9F5A27F796D5BD786A5BD5A16),
(58911,'Win','x64','WoWC',0xA9C0BEA1B7351665091E6754A0A90A16),
(59207,'Mac','A64','WoW',0x2EC5EAA28DA705573D808A7E911F5D00),
(59207,'Mac','A64','WoWC',0x23A35FDC661E9761FEAC93BF20A926EE),
(59207,'Mac','x64','WoW',0x9E2B97FF7DEA87E951DDE91A65C2595B),
(59207,'Mac','x64','WoWC',0x5FB60ADC82C68D1E50B70956BFBC77BC),
(59207,'Win','A64','WoW',0x41AF3D9F83318D4FC4406BF5C23B20AB),
(59207,'Win','x64','WoW',0x170FB48807771D186547DF96C59E5306),
(59207,'Win','x64','WoWC',0xA9038B171DE62AF741304A4C0F273814),
(59302,'Mac','A64','WoW',0x70A6BE00213BB70AB96C51280D72D186),
(59302,'Mac','A64','WoWC',0x6A5CFE0D3A4BDEC7D16D33E9ACA7D71D),
(59302,'Mac','x64','WoW',0x9FD959000AFAA2E5F10A4ABE33AC62BE),
(59302,'Mac','x64','WoWC',0xD39B478EFD342D568DCBC2E85FCA574D),
(59302,'Win','A64','WoW',0x60B1757CA707789C9ACD6F1FB69E28D5),
(59302,'Win','x64','WoW',0x330E4E7EF1E07DA8382C813650D7E7B5),
(59302,'Win','x64','WoWC',0xB5A0C54F6B334515E68260F6334E7593),
(59347,'Mac','A64','WoW',0xC7B81BB6F9207DA0CCEBFAC3186FC073),
(59347,'Mac','A64','WoWC',0xBAB986A50C7FDB89467AA5613A6BA345),
(59347,'Mac','x64','WoW',0x1A11343133BF1ABDBF0FF6B287B37C23),
(59347,'Mac','x64','WoWC',0x3A624C37EE150CC9B71CA3AD1F4532CF),
(59347,'Win','A64','WoW',0x80CAC49F6EA99AC68CA9B834466CA5DC),
(59347,'Win','x64','WoW',0x63228339538F9552D16855976C64848B),
(59347,'Win','x64','WoWC',0xBB45FAC562EB7CF3FC4C137C1AE93AF6),
(59425,'Mac','A64','WoW',0x14A01BEF9E9DB7C5A40438CE87E99B85),
(59425,'Mac','A64','WoWC',0x38B98DA1F8765C927C724CFE4E1D92C2),
(59425,'Mac','x64','WoW',0x9D230A7C010BA2E5B385CB4E24A59529),
(59425,'Mac','x64','WoWC',0xB0BF826891290E30173DCA5CF2B39F50),
(59425,'Win','A64','WoW',0xF40FED2615A2454E31B044B1AD564B7D),
(59425,'Win','x64','WoW',0x5FE3E3980A15A1D4ED64240BC0C6932D),
(59425,'Win','x64','WoWC',0xECA5DA344AF973B0D25AC1103FB694B8),
(59466,'Mac','A64','WoW',0xF6CB8EB7AB66F8909ADCA85052BFAE40),
(59466,'Mac','A64','WoWC',0x213104EC1BEF7656FC3930DCE4176184),
(59466,'Mac','x64','WoW',0x99075D27FF98A87368A08E87A3F105C3),
(59466,'Mac','x64','WoWC',0x516115232E1A8B3F1E3CA709F20EA84D),
(59466,'Win','A64','WoW',0x04F0986C0EAD648F8C7B27736A30088A),
(59466,'Win','x64','WoW',0x118FFD674F5B51C277F5C8505E9479D3),
(59466,'Win','x64','WoWC',0xC299CB416E332E4BBE5AA1AAE905A437),
(59538,'Mac','A64','WoW',0x1DDC6F14195D1F85B7026AEDCCFE2875),
(59538,'Mac','A64','WoWC',0xE34E2419F557FA85EE5D026CB1BC6246),
(59538,'Mac','x64','WoW',0xCBC396C3E20090798DF9CF8B67DCDBEE),
(59538,'Mac','x64','WoWC',0x73DDA3618C8D13A0DAA79C2007B894DD),
(59538,'Win','A64','WoW',0x498203B085106FA1A039A8B5E55DD31C),
(59538,'Win','x64','WoW',0x72B905CC388F523E1A30FBF23DA0C9AB),
(59538,'Win','x64','WoWC',0xA198A8FF303007EAC43B74AA90489BBC),
(59570,'Mac','A64','WoW',0x6318C6C300367C8E114042584DA7F82C),
(59570,'Mac','A64','WoWC',0x6F1554122133FA8F4EE189022CFFD122),
(59570,'Mac','x64','WoW',0x13968B08B91BD4A922C3340A77E30C87),
(59570,'Mac','x64','WoWC',0x29D0C5C6E3F8408D064EA76930EE92E7),
(59570,'Win','A64','WoW',0x1ED3EA5DBEF495E4A01AE989E8F44CD8),
(59570,'Win','x64','WoW',0x7571E0C3459C7F9BFDC64BF5AF035F08),
(59570,'Win','x64','WoWC',0xC5CA2210F4E39657EE415FCAA53D4B23),
(59679,'Mac','A64','WoW',0x721E18C6BB25D4878A91801985A0D18F),
(59679,'Mac','A64','WoWC',0x45CB8600B7404B46B032183CCE4B340A),
(59679,'Mac','x64','WoW',0x2E3CD97FD83976BB7D56D3489CE40D79),
(59679,'Mac','x64','WoWC',0x4D28DE9F333624311D3150F871C472EF),
(59679,'Win','A64','WoW',0xE4435EE6E4968B5106E6596B60EE2DB6),
(59679,'Win','x64','WoW',0x54E4C53C5DC1930794DE4D1DD0365FE2),
(59679,'Win','x64','WoWC',0xB23BFD99EBCD872EA39230E1AABCD19B),
(59888,'Mac','A64','WoW',0xB64283161581FFC8D5FA9D5EF2689C1D),
(59888,'Mac','A64','WoWC',0xB9E0C8C20A63C3B7EA8037A6968C1305),
(59888,'Mac','x64','WoW',0x92D62F3B10B01DF8A6FF042BE8567D19),
(59888,'Mac','x64','WoWC',0xB0F4010BAF63ECC5E798BC49827016E9),
(59888,'Win','A64','WoW',0x7BC71337460CB90B26FEBEC9FC14B553),
(59888,'Win','x64','WoW',0x17B6A2C47452572A8EBF9200C1CA1C92),
(59888,'Win','x64','WoWC',0x709AD6F51891994D41CBA1F49F812123),
(60037,'Mac','A64','WoW',0x2D9FE76E6A93DD0C93DF0D3D8B192FAD),
(60037,'Mac','A64','WoWC',0x7AE0E5804DD7DD5D707BE74DB290C807),
(60037,'Mac','x64','WoW',0xBCD051513C5A8DB78BF7B0A8EC18E5EC),
(60037,'Mac','x64','WoWC',0x64E76DF99C69D852656979829D8600CE),
(60037,'Win','A64','WoW',0x2A4939EC8440AB977FE9282A2CCE3E24),
(60037,'Win','x64','WoW',0x474FF52EF47D47521EF138291B43723D),
(60037,'Win','x64','WoWC',0x75EBBD3C1C1D088F2597E40CA3924B5E),
(60189,'Mac','A64','WoW',0x7C532E0F4D4429430D54BB1DD5D88336),
(60189,'Mac','A64','WoWC',0xCF51D223D48B246FC13A9376A8C933F5),
(60189,'Mac','x64','WoW',0x59B56E50D2A1D3EB28325F57BF637428),
(60189,'Mac','x64','WoWC',0x40CE6676DC47E77D6BED686D31429BE6),
(60189,'Win','A64','WoW',0xE8F789C79EED914C872AEF6D7351EDF8),
(60189,'Win','x64','WoW',0xD9A3ACC3E1222318D951DC0F3FB8AEC4),
(60189,'Win','x64','WoWC',0xA7A07FBE9E0C2D9E9CA3DFBD2670A0AD),
(60228,'Mac','A64','WoW',0xB14CFCF7D92AE48EFCF4BCC7D263039D),
(60228,'Mac','A64','WoWC',0xF3D26ACC7F83A3FC6CEB494818C27756),
(60228,'Mac','x64','WoW',0x8BB5518B0818D0FD086A055D06AD2515),
(60228,'Mac','x64','WoWC',0x9F1CE1C8259537F6B0F8B9BCC423D241),
(60228,'Win','A64','WoW',0xD05C9119A81C564EDC48EABFB6F9D06F),
(60228,'Win','x64','WoW',0x5B6F19D30DCC37D1C3D9CB94A44EB34E),
(60228,'Win','x64','WoWC',0xCEC40F04254DFD2314D1AAC86762182A),
(60257,'Mac','A64','WoW',0xB59EBBBA00139714D2CA755DBA71A58A),
(60257,'Mac','A64','WoWC',0x01DCFC44C27452F3B7E33EE6F01E067F),
(60257,'Mac','x64','WoW',0x3A0DB5ECBECCA5C80B704FA405DA5389),
(60257,'Mac','x64','WoWC',0x174D184200D89BF13A62DEDF6E265827),
(60257,'Win','A64','WoW',0x5DE295571BA573F16EC0D27F9B256953),
(60257,'Win','x64','WoW',0x04863309B886141AAB63893CD2414960),
(60257,'Win','x64','WoWC',0x6D386FDACBABA88604AFAE3826FE31FA),
(60392,'Mac','A64','WoW',0x2C1FB8AE803BD91C457D08D6DF1C6128),
(60392,'Mac','A64','WoWC',0xDFCE89D38B558A97E34B82D38C6972C9),
(60392,'Mac','x64','WoW',0xDD8F3F8D4290BA24F006106397DD00FA),
(60392,'Mac','x64','WoWC',0xDE6A3F2957C68A4E100ABAEB8547E127),
(60392,'Win','A64','WoW',0xF1FE18ECAF86E0FEF417D6AC8F450444),
(60392,'Win','x64','WoW',0xD2262204981078AE85E98BDAFF11EB41),
(60392,'Win','x64','WoWC',0x896CE1BADC6CD1D65169AD63803BD51C),
(60428,'Mac','A64','WoW',0x2E7D82DB6D2AB614A9D523B773C1D921),
(60428,'Mac','A64','WoWC',0x0DA35A86AF0709859B22F9CAE128CD49),
(60428,'Mac','x64','WoW',0xFAF413508F53006209110192CD6C1DCF),
(60428,'Mac','x64','WoWC',0x21838345675138FC55002B4E2BB95F37),
(60428,'Win','A64','WoW',0x32AC6106995042A6355B1DFDDE62BC75),
(60428,'Win','x64','WoW',0x264E5A8300CB3FBDD183BF90F4EFAD4F),
(60428,'Win','x64','WoWC',0xA70F28AEE99CE8D2993B0AB11876C668),
(60490,'Mac','A64','WoW',0xB9289C46A7581BCFBB3E4F2DA4E88C4A),
(60490,'Mac','A64','WoWC',0xE2CB722BA3091FB93E11D0A45FDE245D),
(60490,'Mac','x64','WoW',0x0A8D47F840CC05F8E2F37CF9397BB5B2),
(60490,'Mac','x64','WoWC',0x90845571462EE2250F4B5C572C9188DA),
(60490,'Win','A64','WoW',0xBD4A878CFFF3262669351D582949F2CD),
(60490,'Win','x64','WoW',0xBE091C2F2AEA1D1800CDA7E6875F3665),
(60490,'Win','x64','WoWC',0x22822461C16260D124680E05875F9D9D),
(60568,'Mac','A64','WoW',0x470C274987087337796E8B9E2CAE74DB),
(60568,'Mac','A64','WoWC',0x4CB6793447021056A4D3A4DC0D187AA9),
(60568,'Mac','x64','WoW',0x97CE8361B5B5F0E63463C3CB028C4553),
(60568,'Mac','x64','WoWC',0x215E50B6E560D093731B8D2EA7A9125B),
(60568,'Win','A64','WoW',0x788B2E434F8F94C2F30912640E1C28E6),
(60568,'Win','x64','WoW',0x2EA108C13157170527F2CDF2581216A2),
(60568,'Win','x64','WoWC',0x2D180D7E0F83BBAE38177A02273AC016),
(60822,'Mac','A64','WoW',0xAFC18687E2318BB620E4972B5A3D69BF),
(60822,'Mac','A64','WoWC',0x9F6E6D114F14EED0D0FB202842572E2E),
(60822,'Mac','x64','WoW',0xDE216E4A7FA5861A9526E4914CFD867A),
(60822,'Mac','x64','WoWC',0xC07CBC1D65C06EDCC6D90914A59D76DA),
(60822,'Win','A64','WoW',0x5696DD4936320D7F8781688A0730950A),
(60822,'Win','x64','WoW',0x3AAC5F8EB768DFC9161C1506903E19AB),
(60822,'Win','x64','WoWC',0x0B5F6A7B210ED77012F4341DEF9445F7),
(61122,'Mac','A64','WoW',0xD898830AEFC000E41A8DEEAA7DD04B51),
(61122,'Mac','A64','WoWC',0x58AF686E1A37B550D31375028105D6CB),
(61122,'Mac','x64','WoW',0x56AEFFFCFEB892A2A8C4788AF03AD7BC),
(61122,'Win','A64','WoW',0x9AC43579FE8BE4E76A20251A2F563C61),
(61122,'Win','x64','WoW',0x5DE6B3E0046D8F38E0566DD620469916),
(61122,'Win','x64','WoWC',0x8E849FE11AA7F2181E16585A4691F451),
(61188,'Mac','A64','WoW',0x5FECA4BA5E5CFE4D965E94C39FEE718A),
(61188,'Mac','A64','WoWC',0xA390F2B83D7A7E35AA30FFCFDAE8E18D),
(61188,'Mac','x64','WoW',0x5F9EA790FD85BCE75E869BDBE8799238),
(61188,'Mac','x64','WoWC',0xC7D2C3C134C5744DBD32C563B5BE9C89),
(61188,'Win','A64','WoW',0x999BF7D43893EF11247355389155B0E7),
(61188,'Win','x64','WoW',0xF8A42EB146A4DDD94FD35FF9AD1E75DF),
(61188,'Win','x64','WoWC',0x43983A4D6435939F6EE7EAE955D6C8AA),
(61265,'Mac','A64','WoW',0xDEA96215DAEE2552A432AE1F30753C3D),
(61265,'Mac','A64','WoWC',0x3AEED019A7B37FC429F0D484B8599D03),
(61265,'Mac','x64','WoW',0xDF74D9839DDE90D1681C83829BEE4904),
(61265,'Mac','x64','WoWC',0xA601E42069510E0D5EB08FE1155E21DF),
(61265,'Win','A64','WoW',0x87D07F5340DEC51CB965935E13A035A7),
(61265,'Win','x64','WoW',0xDAC655ECD4795C36E2A7DF63AB7E3915),
(61265,'Win','x64','WoWC',0xC06106A58586E5B0A13411BC53B8CFA5),
(61491,'Mac','A64','WoW',0x6A63F9B14228B85AF4EE9F6AAD2DC108),
(61491,'Mac','A64','WoWC',0x0F263AA0C47B7F9AF2DECC62CFBC87B8),
(61491,'Mac','x64','WoW',0x42754222FBEF4B872AB34F5A53166707),
(61491,'Mac','x64','WoWC',0x9513DE4CAB9E345FE327A216B9C4CAF3),
(61491,'Win','A64','WoW',0xA42E81E60866C0114F59CE5F5F6458D4),
(61491,'Win','x64','WoW',0xECE38E2B4833BA220C3C33EED4C004D1),
(61491,'Win','x64','WoWC',0x4E106C27490D03932C7985F192AC35E6),
(61559,'Mac','A64','WoW',0x3C0726C9D66742F43C8D105C015364CF),
(61559,'Mac','A64','WoWC',0x32F6AB87E9E9776BEAACE8B6683A39BA),
(61559,'Mac','x64','WoW',0xD2039CB1EEE0B59595E145B8A9626001),
(61559,'Mac','x64','WoWC',0xE73978C9576833F36694293036E55409),
(61559,'Win','A64','WoW',0xAA7AD5624979EB36A6963A9F7DE019B9),
(61559,'Win','x64','WoW',0x17928A7329F53E273891D85A973248C0),
(61559,'Win','x64','WoWC',0xD9E57238DD0742B44E68F05D59A6234B),
(61609,'Mac','A64','WoW',0x9D3B5F3D2DDEB29ABB6DB48EF29D38C9),
(61609,'Mac','A64','WoWC',0x5949F3AE2B51A3B03155A13952E82F77),
(61609,'Mac','x64','WoW',0x7C8665EED3E4C902E8B2FC4972F4636C),
(61609,'Mac','x64','WoWC',0x36FBCBD0CC7A8E88AAAF9BD3CF14FC81),
(61609,'Win','A64','WoW',0x21493F8D8C078FDA669E42AEA8724351),
(61609,'Win','x64','WoW',0x9F18C02CC5868A925CDD4201A8D7F9DA),
(61609,'Win','x64','WoWC',0x15F4CA6C215113B4B47EC3AB65C65419),
(61967,'Mac','A64','WoW',0xDAB6F778C37531255AC22F3FB9065EAA),
(61967,'Mac','A64','WoWC',0x3F10FF17698D9F938B04707C0B8A7E00),
(61967,'Mac','x64','WoW',0x2B9DB4F2B70E849871A8F95FEEF357E9),
(61967,'Mac','x64','WoWC',0x4DE3D440D1D46A38EC32D0D524004ABA),
(61967,'Win','A64','WoW',0x7B443BB6B4F355B9E389F4F0ACB04FE8),
(61967,'Win','x64','WoW',0x6E194B4B53038E76E01E9538107AFF4D),
(61967,'Win','x64','WoWC',0x9E135BF0B40BDD210EAF318AEB72801C);
/*!40000 ALTER TABLE `build_auth_key` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `build_executable_hash`
--

DROP TABLE IF EXISTS `build_executable_hash`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `build_executable_hash` (
  `build` int NOT NULL,
  `platform` char(4) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `executableHash` binary(20) NOT NULL,
  PRIMARY KEY (`build`,`platform`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `build_executable_hash`
--

LOCK TABLES `build_executable_hash` WRITE;
/*!40000 ALTER TABLE `build_executable_hash` DISABLE KEYS */;
INSERT INTO `build_executable_hash` VALUES
(5875,'OSX',0x8D173CC381961EEBABF336F5E6675B101BB513E5),
(5875,'Win',0x95EDB27C7823B363CBDDAB56A392E7CB73FCCA20),
(8606,'OSX',0xD8B0ECFE534BC1131E19BAD1D4C0E813EEE4994F),
(8606,'Win',0x319AFAA3F2559682F9FF658BE01456255F456FB1),
(12340,'OSX',0xB706D13FF2F4018839729461E3F8A0E2B5FDC034),
(12340,'Win',0xCDCBBD5188315E6B4D19449D492DBCFAF156A347);
/*!40000 ALTER TABLE `build_executable_hash` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `build_info`
--

DROP TABLE IF EXISTS `build_info`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `build_info` (
  `build` int NOT NULL,
  `majorVersion` int DEFAULT NULL,
  `minorVersion` int DEFAULT NULL,
  `bugfixVersion` int DEFAULT NULL,
  `hotfixVersion` char(3) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  PRIMARY KEY (`build`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `build_info`
--

LOCK TABLES `build_info` WRITE;
/*!40000 ALTER TABLE `build_info` DISABLE KEYS */;
INSERT INTO `build_info` VALUES
(5875,1,12,1,NULL),
(6005,1,12,2,NULL),
(6141,1,12,3,NULL),
(8606,2,4,3,NULL),
(9947,3,1,3,NULL),
(10505,3,2,2,'a'),
(11159,3,3,0,'a'),
(11403,3,3,2,NULL),
(11723,3,3,3,'a'),
(12340,3,3,5,'a'),
(13623,4,0,6,'a'),
(13930,3,3,5,'a'),
(14545,4,2,2,NULL),
(15595,4,3,4,NULL),
(19116,6,0,3,NULL),
(19243,6,0,3,NULL),
(19342,6,0,3,NULL),
(19702,6,1,0,NULL),
(19802,6,1,2,NULL),
(19831,6,1,2,NULL),
(19865,6,1,2,NULL),
(20182,6,2,0,'a'),
(20201,6,2,0,NULL),
(20216,6,2,0,NULL),
(20253,6,2,0,NULL),
(20338,6,2,0,NULL),
(20444,6,2,2,NULL),
(20490,6,2,2,'a'),
(20574,6,2,2,'a'),
(20726,6,2,3,NULL),
(20779,6,2,3,NULL),
(20886,6,2,3,NULL),
(21355,6,2,4,NULL),
(21463,6,2,4,NULL),
(21742,6,2,4,NULL),
(22248,7,0,3,NULL),
(22293,7,0,3,NULL),
(22345,7,0,3,NULL),
(22410,7,0,3,NULL),
(22423,7,0,3,NULL),
(22498,7,0,3,NULL),
(22522,7,0,3,NULL),
(22566,7,0,3,NULL),
(22594,7,0,3,NULL),
(22624,7,0,3,NULL),
(22747,7,0,3,NULL),
(22810,7,0,3,NULL),
(22900,7,1,0,NULL),
(22908,7,1,0,NULL),
(22950,7,1,0,NULL),
(22995,7,1,0,NULL),
(22996,7,1,0,NULL),
(23171,7,1,0,NULL),
(23222,7,1,0,NULL),
(23360,7,1,5,NULL),
(23420,7,1,5,NULL),
(23911,7,2,0,NULL),
(23937,7,2,0,NULL),
(24015,7,2,0,NULL),
(24330,7,2,5,NULL),
(24367,7,2,5,NULL),
(24415,7,2,5,NULL),
(24430,7,2,5,NULL),
(24461,7,2,5,NULL),
(24742,7,2,5,NULL),
(25549,7,3,2,NULL),
(25996,7,3,5,NULL),
(26124,7,3,5,NULL),
(26365,7,3,5,NULL),
(26654,7,3,5,NULL),
(26822,7,3,5,NULL),
(26899,7,3,5,NULL),
(26972,7,3,5,NULL),
(28153,8,0,1,NULL),
(30706,8,1,5,NULL),
(30993,8,2,0,NULL),
(31229,8,2,0,NULL),
(31429,8,2,0,NULL),
(31478,8,2,0,NULL),
(32305,8,2,5,NULL),
(32494,8,2,5,NULL),
(32580,8,2,5,NULL),
(32638,8,2,5,NULL),
(32722,8,2,5,NULL),
(32750,8,2,5,NULL),
(32978,8,2,5,NULL),
(33369,8,3,0,NULL),
(33528,8,3,0,NULL),
(33724,8,3,0,NULL),
(33775,8,3,0,NULL),
(33941,8,3,0,NULL),
(34220,8,3,0,NULL),
(34601,8,3,0,NULL),
(34769,8,3,0,NULL),
(34963,8,3,0,NULL),
(35249,8,3,7,NULL),
(35284,8,3,7,NULL),
(35435,8,3,7,NULL),
(35662,8,3,7,NULL),
(36753,9,0,2,NULL),
(36839,9,0,2,NULL),
(36949,9,0,2,NULL),
(37142,9,0,2,NULL),
(37176,9,0,2,NULL),
(37474,9,0,2,NULL),
(38134,9,0,5,NULL),
(38556,9,0,5,NULL),
(39653,9,1,0,NULL),
(39804,9,1,0,NULL),
(40000,9,1,0,NULL),
(40120,9,1,0,NULL),
(40443,9,1,0,NULL),
(40593,9,1,0,NULL),
(40725,9,1,0,NULL),
(40906,9,1,5,NULL),
(40944,9,1,5,NULL),
(40966,9,1,5,NULL),
(41031,9,1,5,NULL),
(41079,9,1,5,NULL),
(41288,9,1,5,NULL),
(41323,9,1,5,NULL),
(41359,9,1,5,NULL),
(41488,9,1,5,NULL),
(41793,9,1,5,NULL),
(42010,9,1,5,NULL),
(42423,9,2,0,NULL),
(42488,9,2,0,NULL),
(42521,9,2,0,NULL),
(42538,9,2,0,NULL),
(42560,9,2,0,NULL),
(42614,9,2,0,NULL),
(42698,9,2,0,NULL),
(42825,9,2,0,NULL),
(42852,9,2,0,NULL),
(42937,9,2,0,NULL),
(42979,9,2,0,NULL),
(43114,9,2,0,NULL),
(43206,9,2,0,NULL),
(43340,9,2,0,NULL),
(43345,9,2,0,NULL),
(43971,9,2,5,NULL),
(44015,9,2,5,NULL),
(44061,9,2,5,NULL),
(44127,9,2,5,NULL),
(44232,9,2,5,NULL),
(44325,9,2,5,NULL),
(44730,9,2,5,NULL),
(44908,9,2,5,NULL),
(45114,9,2,7,NULL),
(45161,9,2,7,NULL),
(45338,9,2,7,NULL),
(45745,9,2,7,NULL),
(46479,10,0,2,NULL),
(46658,10,0,2,NULL),
(46689,10,0,2,NULL),
(46702,10,0,2,NULL),
(46741,10,0,2,NULL),
(46801,10,0,2,NULL),
(46879,10,0,2,NULL),
(46924,10,0,2,NULL),
(47067,10,0,2,NULL),
(47187,10,0,2,NULL),
(47213,10,0,2,NULL),
(47631,10,0,2,NULL),
(47777,10,0,5,NULL),
(47799,10,0,5,NULL),
(47825,10,0,5,NULL),
(47849,10,0,5,NULL),
(47871,10,0,5,NULL),
(47884,10,0,5,NULL),
(47936,10,0,5,NULL),
(47967,10,0,5,NULL),
(48001,10,0,5,NULL),
(48069,10,0,5,NULL),
(48317,10,0,5,NULL),
(48397,10,0,5,NULL),
(48526,10,0,5,NULL),
(48676,10,0,7,NULL),
(48749,10,0,7,NULL),
(48838,10,0,7,NULL),
(48865,10,0,7,NULL),
(48892,10,0,7,NULL),
(48966,10,0,7,NULL),
(48999,10,0,7,NULL),
(49267,10,0,7,NULL),
(49318,10,1,0,NULL),
(49343,10,0,7,NULL),
(49407,10,1,0,NULL),
(49426,10,1,0,NULL),
(49444,10,1,0,NULL),
(49474,10,1,0,NULL),
(49570,10,1,0,NULL),
(49679,10,1,0,NULL),
(49741,10,1,0,NULL),
(49801,10,1,0,NULL),
(49890,10,1,0,NULL),
(50000,10,1,0,NULL),
(50401,10,1,5,NULL),
(50438,10,1,5,NULL),
(50467,10,1,5,NULL),
(50469,10,1,5,NULL),
(50504,10,1,5,NULL),
(50585,10,1,5,NULL),
(50622,10,1,5,NULL),
(50747,10,1,5,NULL),
(50791,10,1,5,NULL),
(51130,10,1,5,NULL),
(51187,10,1,7,NULL),
(51237,10,1,7,NULL),
(51261,10,1,7,NULL),
(51313,10,1,7,NULL),
(51421,10,1,7,NULL),
(51485,10,1,7,NULL),
(51536,10,1,7,NULL),
(51754,10,1,7,NULL),
(51886,10,1,7,NULL),
(51972,10,1,7,NULL),
(52038,10,2,0,NULL),
(52068,10,2,0,NULL),
(52095,10,2,0,NULL),
(52106,10,2,0,NULL),
(52129,10,2,0,NULL),
(52148,10,2,0,NULL),
(52188,10,2,0,NULL),
(52301,10,2,0,NULL),
(52393,10,2,0,NULL),
(52485,10,2,0,NULL),
(52545,10,2,0,NULL),
(52607,10,2,0,NULL),
(52649,10,2,0,NULL),
(52808,10,2,0,NULL),
(52902,10,2,5,NULL),
(52968,10,2,5,NULL),
(52983,10,2,5,NULL),
(53007,10,2,5,NULL),
(53040,10,2,5,NULL),
(53104,10,2,5,NULL),
(53162,10,2,5,NULL),
(53212,10,2,5,NULL),
(53262,10,2,5,NULL),
(53441,10,2,5,NULL),
(53584,10,2,5,NULL),
(53840,10,2,6,NULL),
(53877,10,2,6,NULL),
(53913,10,2,6,NULL),
(53989,10,2,6,NULL),
(54070,10,2,6,NULL),
(54205,10,2,6,NULL),
(54358,10,2,6,NULL),
(54499,10,2,6,NULL),
(54577,10,2,7,NULL),
(54601,10,2,7,NULL),
(54604,10,2,7,NULL),
(54630,10,2,7,NULL),
(54673,10,2,7,NULL),
(54717,10,2,7,NULL),
(54736,10,2,7,NULL),
(54762,10,2,7,NULL),
(54847,10,2,7,NULL),
(54904,10,2,7,NULL),
(54988,10,2,7,NULL),
(55142,10,2,7,NULL),
(55165,10,2,7,NULL),
(55261,10,2,7,NULL),
(55461,10,2,7,NULL),
(55664,10,2,7,NULL),
(55666,11,0,0,NULL),
(55792,11,0,0,NULL),
(55793,11,0,0,NULL),
(55818,11,0,0,NULL),
(55824,11,0,0,NULL),
(55846,11,0,0,NULL),
(55933,11,0,0,NULL),
(55939,11,0,0,NULL),
(55959,11,0,2,NULL),
(55960,11,0,0,NULL),
(56008,11,0,0,NULL),
(56110,11,0,2,NULL),
(56162,11,0,2,NULL),
(56196,11,0,2,NULL),
(56263,11,0,2,NULL),
(56288,11,0,2,NULL),
(56311,11,0,2,NULL),
(56313,11,0,2,NULL),
(56380,11,0,2,NULL),
(56382,11,0,2,NULL),
(56421,11,0,2,NULL),
(56461,11,0,2,NULL),
(56513,11,0,2,NULL),
(56625,11,0,2,NULL),
(56647,11,0,2,NULL),
(56819,11,0,2,NULL),
(57171,11,0,5,NULL),
(57212,11,0,5,NULL),
(57292,11,0,5,NULL),
(57388,11,0,5,NULL),
(57534,11,0,5,NULL),
(57637,11,0,5,NULL),
(57689,11,0,5,NULL),
(58123,11,0,7,NULL),
(58162,11,0,7,NULL),
(58187,11,0,7,NULL),
(58238,11,0,7,NULL),
(58533,11,0,7,NULL),
(58608,11,0,7,NULL),
(58630,11,0,7,NULL),
(58680,11,0,7,NULL),
(58773,11,0,7,NULL),
(58867,11,0,7,NULL),
(58911,11,0,7,NULL),
(59207,11,0,7,NULL),
(59302,11,0,7,NULL),
(59347,11,1,0,NULL),
(59425,11,1,0,NULL),
(59466,11,1,0,NULL),
(59538,11,1,0,NULL),
(59570,11,1,0,NULL),
(59679,11,1,0,NULL),
(59888,11,1,0,NULL),
(60037,11,1,0,NULL),
(60189,11,1,0,NULL),
(60228,11,1,0,NULL),
(60257,11,1,0,NULL),
(60392,11,1,5,NULL),
(60428,11,1,5,NULL),
(60490,11,1,5,NULL),
(60568,11,1,5,NULL),
(60822,11,1,5,NULL),
(61122,11,1,5,NULL),
(61188,11,1,5,NULL),
(61265,11,1,5,NULL),
(61491,11,1,7,NULL),
(61559,11,1,7,NULL),
(61609,11,1,7,NULL),
(61967,11,1,7,NULL);
/*!40000 ALTER TABLE `build_info` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `ip_banned`
--

DROP TABLE IF EXISTS `ip_banned`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `ip_banned` (
  `ip` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '127.0.0.1',
  `bandate` int unsigned NOT NULL,
  `unbandate` int unsigned NOT NULL,
  `bannedby` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '[Console]',
  `banreason` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'no reason',
  PRIMARY KEY (`ip`,`bandate`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Banned IPs';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `ip_banned`
--

LOCK TABLES `ip_banned` WRITE;
/*!40000 ALTER TABLE `ip_banned` DISABLE KEYS */;
/*!40000 ALTER TABLE `ip_banned` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `logs`
--

DROP TABLE IF EXISTS `logs`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `logs` (
  `time` int unsigned NOT NULL,
  `realm` int unsigned NOT NULL,
  `type` varchar(250) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `level` tinyint unsigned NOT NULL DEFAULT '0',
  `string` mediumtext CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `logs`
--

LOCK TABLES `logs` WRITE;
/*!40000 ALTER TABLE `logs` DISABLE KEYS */;
/*!40000 ALTER TABLE `logs` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `logs_ip_actions`
--

DROP TABLE IF EXISTS `logs_ip_actions`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `logs_ip_actions` (
  `id` int unsigned NOT NULL AUTO_INCREMENT COMMENT 'Unique Identifier',
  `account_id` int unsigned NOT NULL COMMENT 'Account ID',
  `character_guid` bigint unsigned NOT NULL COMMENT 'Character Guid',
  `realm_id` int unsigned NOT NULL DEFAULT '0' COMMENT 'Realm ID',
  `type` tinyint unsigned NOT NULL,
  `ip` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '127.0.0.1',
  `systemnote` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci COMMENT 'Notes inserted by system',
  `unixtime` int unsigned NOT NULL COMMENT 'Unixtime',
  `time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'Timestamp',
  `comment` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci COMMENT 'Allows users to add a comment',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Used to log ips of individual actions';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `logs_ip_actions`
--

LOCK TABLES `logs_ip_actions` WRITE;
/*!40000 ALTER TABLE `logs_ip_actions` DISABLE KEYS */;
/*!40000 ALTER TABLE `logs_ip_actions` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `rbac_account_permissions`
--

DROP TABLE IF EXISTS `rbac_account_permissions`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `rbac_account_permissions` (
  `accountId` int unsigned NOT NULL COMMENT 'Account id',
  `permissionId` int unsigned NOT NULL COMMENT 'Permission id',
  `granted` tinyint(1) NOT NULL DEFAULT '1' COMMENT 'Granted = 1, Denied = 0',
  `realmId` int NOT NULL DEFAULT '-1' COMMENT 'Realm Id, -1 means all',
  PRIMARY KEY (`accountId`,`permissionId`,`realmId`),
  KEY `fk__rbac_account_roles__rbac_permissions` (`permissionId`),
  CONSTRAINT `fk__rbac_account_permissions__account` FOREIGN KEY (`accountId`) REFERENCES `account` (`id`) ON DELETE CASCADE,
  CONSTRAINT `fk__rbac_account_roles__rbac_permissions` FOREIGN KEY (`permissionId`) REFERENCES `rbac_permissions` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Account-Permission relation';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `rbac_account_permissions`
--

LOCK TABLES `rbac_account_permissions` WRITE;
/*!40000 ALTER TABLE `rbac_account_permissions` DISABLE KEYS */;
/*!40000 ALTER TABLE `rbac_account_permissions` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `rbac_default_permissions`
--

DROP TABLE IF EXISTS `rbac_default_permissions`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `rbac_default_permissions` (
  `secId` int unsigned NOT NULL COMMENT 'Security Level id',
  `permissionId` int unsigned NOT NULL COMMENT 'permission id',
  `realmId` int NOT NULL DEFAULT '-1' COMMENT 'Realm Id, -1 means all',
  PRIMARY KEY (`secId`,`permissionId`,`realmId`),
  KEY `fk__rbac_default_permissions__rbac_permissions` (`permissionId`),
  CONSTRAINT `fk__rbac_default_permissions__rbac_permissions` FOREIGN KEY (`permissionId`) REFERENCES `rbac_permissions` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Default permission to assign to different account security levels';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `rbac_default_permissions`
--

LOCK TABLES `rbac_default_permissions` WRITE;
/*!40000 ALTER TABLE `rbac_default_permissions` DISABLE KEYS */;
INSERT INTO `rbac_default_permissions` VALUES
(3,192,-1),
(2,193,-1),
(1,194,-1),
(0,195,-1);
/*!40000 ALTER TABLE `rbac_default_permissions` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `rbac_linked_permissions`
--

DROP TABLE IF EXISTS `rbac_linked_permissions`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `rbac_linked_permissions` (
  `id` int unsigned NOT NULL COMMENT 'Permission id',
  `linkedId` int unsigned NOT NULL COMMENT 'Linked Permission id',
  PRIMARY KEY (`id`,`linkedId`),
  KEY `fk__rbac_linked_permissions__rbac_permissions1` (`id`),
  KEY `fk__rbac_linked_permissions__rbac_permissions2` (`linkedId`),
  CONSTRAINT `fk__rbac_linked_permissions__rbac_permissions1` FOREIGN KEY (`id`) REFERENCES `rbac_permissions` (`id`) ON DELETE CASCADE,
  CONSTRAINT `fk__rbac_linked_permissions__rbac_permissions2` FOREIGN KEY (`linkedId`) REFERENCES `rbac_permissions` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Permission - Linked Permission relation';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `rbac_linked_permissions`
--

LOCK TABLES `rbac_linked_permissions` WRITE;
/*!40000 ALTER TABLE `rbac_linked_permissions` DISABLE KEYS */;
INSERT INTO `rbac_linked_permissions` VALUES
(192,21),
(192,42),
(192,43),
(192,193),
(192,196),
(192,776),
(192,778),
(192,779),
(192,780),
(192,781),
(192,782),
(192,783),
(192,784),
(192,785),
(192,786),
(192,787),
(192,788),
(192,789),
(192,790),
(192,791),
(192,792),
(192,793),
(192,794),
(192,795),
(192,796),
(192,835),
(192,844),
(192,845),
(192,846),
(192,847),
(192,848),
(192,849),
(192,850),
(192,851),
(192,853),
(192,854),
(193,48),
(193,194),
(193,197),
(194,1),
(194,2),
(194,9),
(194,11),
(194,12),
(194,13),
(194,14),
(194,15),
(194,16),
(194,17),
(194,18),
(194,19),
(194,20),
(194,22),
(194,23),
(194,25),
(194,26),
(194,27),
(194,28),
(194,29),
(194,30),
(194,31),
(194,32),
(194,33),
(194,34),
(194,35),
(194,36),
(194,37),
(194,38),
(194,39),
(194,40),
(194,41),
(194,44),
(194,46),
(194,47),
(194,51),
(194,195),
(194,198),
(194,632),
(194,798),
(195,3),
(195,4),
(195,5),
(195,6),
(195,24),
(195,49),
(195,199),
(196,7),
(196,10),
(196,202),
(196,203),
(196,204),
(196,205),
(196,206),
(196,208),
(196,212),
(196,213),
(196,214),
(196,215),
(196,216),
(196,226),
(196,227),
(196,230),
(196,231),
(196,233),
(196,234),
(196,235),
(196,238),
(196,239),
(196,240),
(196,241),
(196,242),
(196,243),
(196,244),
(196,245),
(196,246),
(196,247),
(196,248),
(196,249),
(196,250),
(196,251),
(196,252),
(196,253),
(196,254),
(196,255),
(196,256),
(196,257),
(196,258),
(196,259),
(196,260),
(196,261),
(196,262),
(196,264),
(196,265),
(196,266),
(196,267),
(196,268),
(196,269),
(196,270),
(196,271),
(196,272),
(196,279),
(196,280),
(196,283),
(196,287),
(196,288),
(196,289),
(196,290),
(196,291),
(196,292),
(196,293),
(196,294),
(196,295),
(196,296),
(196,297),
(196,298),
(196,299),
(196,302),
(196,303),
(196,304),
(196,305),
(196,306),
(196,309),
(196,310),
(196,314),
(196,319),
(196,320),
(196,321),
(196,322),
(196,323),
(196,324),
(196,325),
(196,326),
(196,327),
(196,328),
(196,329),
(196,330),
(196,331),
(196,335),
(196,336),
(196,337),
(196,339),
(196,340),
(196,341),
(196,342),
(196,343),
(196,344),
(196,345),
(196,346),
(196,347),
(196,348),
(196,349),
(196,350),
(196,351),
(196,352),
(196,353),
(196,354),
(196,355),
(196,356),
(196,357),
(196,358),
(196,359),
(196,360),
(196,361),
(196,362),
(196,363),
(196,364),
(196,365),
(196,366),
(196,373),
(196,375),
(196,400),
(196,401),
(196,402),
(196,403),
(196,404),
(196,405),
(196,406),
(196,407),
(196,417),
(196,418),
(196,419),
(196,420),
(196,421),
(196,422),
(196,423),
(196,424),
(196,425),
(196,426),
(196,427),
(196,428),
(196,429),
(196,434),
(196,435),
(196,436),
(196,437),
(196,438),
(196,439),
(196,440),
(196,441),
(196,442),
(196,443),
(196,444),
(196,445),
(196,446),
(196,447),
(196,448),
(196,449),
(196,450),
(196,451),
(196,452),
(196,453),
(196,454),
(196,455),
(196,456),
(196,457),
(196,458),
(196,459),
(196,460),
(196,461),
(196,463),
(196,464),
(196,465),
(196,472),
(196,473),
(196,474),
(196,475),
(196,476),
(196,477),
(196,478),
(196,488),
(196,489),
(196,491),
(196,492),
(196,493),
(196,495),
(196,497),
(196,498),
(196,499),
(196,500),
(196,502),
(196,503),
(196,505),
(196,508),
(196,511),
(196,513),
(196,514),
(196,516),
(196,519),
(196,522),
(196,523),
(196,526),
(196,527),
(196,529),
(196,530),
(196,533),
(196,535),
(196,536),
(196,537),
(196,538),
(196,539),
(196,540),
(196,541),
(196,556),
(196,581),
(196,582),
(196,592),
(196,593),
(196,596),
(196,602),
(196,603),
(196,604),
(196,605),
(196,606),
(196,607),
(196,608),
(196,609),
(196,610),
(196,611),
(196,612),
(196,613),
(196,615),
(196,616),
(196,617),
(196,618),
(196,619),
(196,620),
(196,621),
(196,623),
(196,624),
(196,625),
(196,626),
(196,627),
(196,628),
(196,629),
(196,630),
(196,631),
(196,633),
(196,634),
(196,635),
(196,636),
(196,637),
(196,638),
(196,639),
(196,640),
(196,641),
(196,642),
(196,643),
(196,644),
(196,645),
(196,646),
(196,647),
(196,648),
(196,649),
(196,650),
(196,651),
(196,652),
(196,653),
(196,654),
(196,655),
(196,656),
(196,657),
(196,658),
(196,659),
(196,660),
(196,661),
(196,663),
(196,665),
(196,666),
(196,667),
(196,668),
(196,669),
(196,670),
(196,671),
(196,672),
(196,673),
(196,674),
(196,675),
(196,676),
(196,677),
(196,678),
(196,679),
(196,680),
(196,681),
(196,682),
(196,683),
(196,684),
(196,685),
(196,686),
(196,687),
(196,688),
(196,689),
(196,690),
(196,691),
(196,693),
(196,694),
(196,695),
(196,696),
(196,697),
(196,698),
(196,699),
(196,700),
(196,701),
(196,702),
(196,703),
(196,704),
(196,707),
(196,708),
(196,709),
(196,710),
(196,711),
(196,712),
(196,713),
(196,714),
(196,715),
(196,716),
(196,717),
(196,718),
(196,719),
(196,721),
(196,722),
(196,723),
(196,724),
(196,725),
(196,726),
(196,727),
(196,728),
(196,729),
(196,730),
(196,733),
(196,734),
(196,735),
(196,736),
(196,738),
(196,739),
(196,753),
(196,757),
(196,773),
(196,777),
(196,809),
(196,817),
(196,825),
(196,829),
(196,830),
(196,831),
(196,832),
(196,833),
(196,836),
(196,837),
(196,838),
(196,839),
(196,840),
(196,842),
(196,843),
(196,852),
(196,866),
(196,869),
(196,870),
(196,871),
(196,872),
(196,873),
(196,875),
(196,876),
(196,877),
(196,878),
(196,879),
(196,880),
(196,881),
(196,882),
(196,883),
(197,232),
(197,236),
(197,237),
(197,273),
(197,274),
(197,275),
(197,276),
(197,277),
(197,284),
(197,285),
(197,286),
(197,301),
(197,311),
(197,387),
(197,388),
(197,389),
(197,390),
(197,391),
(197,392),
(197,393),
(197,394),
(197,395),
(197,396),
(197,397),
(197,398),
(197,399),
(197,479),
(197,480),
(197,481),
(197,482),
(197,485),
(197,486),
(197,487),
(197,494),
(197,501),
(197,506),
(197,509),
(197,510),
(197,517),
(197,518),
(197,521),
(197,542),
(197,543),
(197,550),
(197,558),
(197,568),
(197,571),
(197,572),
(197,573),
(197,574),
(197,575),
(197,576),
(197,577),
(197,578),
(197,579),
(197,580),
(197,583),
(197,584),
(197,585),
(197,586),
(197,587),
(197,588),
(197,589),
(197,590),
(197,591),
(197,594),
(197,595),
(197,601),
(197,761),
(197,762),
(197,763),
(197,764),
(197,765),
(197,766),
(197,767),
(197,768),
(197,769),
(197,770),
(197,771),
(197,772),
(197,774),
(197,775),
(197,805),
(197,811),
(197,813),
(197,819),
(197,821),
(197,827),
(197,856),
(197,857),
(197,858),
(197,859),
(197,860),
(197,865),
(198,218),
(198,300),
(198,312),
(198,315),
(198,316),
(198,317),
(198,318),
(198,367),
(198,368),
(198,369),
(198,370),
(198,371),
(198,372),
(198,374),
(198,376),
(198,377),
(198,408),
(198,409),
(198,410),
(198,411),
(198,412),
(198,413),
(198,414),
(198,415),
(198,430),
(198,431),
(198,432),
(198,433),
(198,462),
(198,466),
(198,467),
(198,468),
(198,469),
(198,470),
(198,471),
(198,483),
(198,484),
(198,490),
(198,504),
(198,512),
(198,515),
(198,520),
(198,524),
(198,528),
(198,531),
(198,532),
(198,544),
(198,545),
(198,547),
(198,548),
(198,549),
(198,551),
(198,552),
(198,553),
(198,554),
(198,555),
(198,557),
(198,559),
(198,560),
(198,561),
(198,562),
(198,563),
(198,564),
(198,565),
(198,566),
(198,567),
(198,569),
(198,570),
(198,597),
(198,598),
(198,599),
(198,600),
(198,737),
(198,740),
(198,741),
(198,742),
(198,802),
(198,803),
(198,804),
(198,806),
(198,807),
(198,808),
(198,810),
(198,812),
(198,814),
(198,815),
(198,816),
(198,818),
(198,820),
(198,822),
(198,823),
(198,824),
(198,826),
(198,828),
(198,855),
(198,868),
(199,207),
(199,209),
(199,210),
(199,211),
(199,217),
(199,221),
(199,222),
(199,223),
(199,225),
(199,263),
(199,378),
(199,379),
(199,380),
(199,496),
(199,507),
(199,525),
(199,534),
(199,797);
/*!40000 ALTER TABLE `rbac_linked_permissions` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `rbac_permissions`
--

DROP TABLE IF EXISTS `rbac_permissions`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `rbac_permissions` (
  `id` int unsigned NOT NULL DEFAULT '0' COMMENT 'Permission id',
  `name` varchar(100) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT 'Permission name',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Permission List';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `rbac_permissions`
--

LOCK TABLES `rbac_permissions` WRITE;
/*!40000 ALTER TABLE `rbac_permissions` DISABLE KEYS */;
INSERT INTO `rbac_permissions` VALUES
(1,'Instant logout'),
(2,'Skip Queue'),
(3,'Join Normal Battleground'),
(4,'Join Random Battleground'),
(5,'Join Arenas'),
(6,'Join Dungeon Finder'),
(7,'Skip idle connection check'),
(8,'Cannot earn achievements'),
(9,'Cannot earn realm first achievements'),
(10,'Use character templates'),
(11,'Log GM trades'),
(12,'Skip character creation demon hunter min level check'),
(13,'Skip Instance required bosses check'),
(14,'Skip character creation team mask check'),
(15,'Skip character creation class mask check'),
(16,'Skip character creation race mask check'),
(17,'Skip character creation reserved name check'),
(18,'Skip character creation death knight min level check'),
(19,'Skip needed requirements to use channel check'),
(20,'Skip disable map check'),
(21,'Skip reset talents when used more than allowed check'),
(22,'Skip spam chat check'),
(23,'Skip over-speed ping check'),
(24,'Two side faction characters on the same account'),
(25,'Allow say chat between factions'),
(26,'Allow channel chat between factions'),
(27,'Two side mail interaction'),
(28,'See two side who list'),
(29,'Add friends of other faction'),
(30,'Save character without delay with .save command'),
(31,'Use params with .unstuck command'),
(32,'Can be assigned tickets with .assign ticket command'),
(33,'Notify if a command was not found'),
(34,'Check if should appear in list using .gm ingame command'),
(35,'See all security levels with who command'),
(36,'Filter whispers'),
(37,'Use staff badge in chat'),
(38,'Resurrect with full Health Points'),
(39,'Restore saved gm setting states'),
(40,'Allows to add a gm to friend list'),
(41,'Use Config option START_GM_LEVEL to assign new character level'),
(42,'Allows to use CMSG_WORLD_TELEPORT opcode'),
(43,'Allows to use CMSG_WHOIS opcode'),
(44,'Receive global GM messages/texts'),
(45,'Join channels without announce'),
(46,'Change channel settings without being channel moderator'),
(47,'Enables lower security than target check'),
(48,'Enable IP, Last Login and EMail output in pinfo'),
(49,'Forces to enter the email for confirmation on password change'),
(50,'Allow user to check his own email with .account'),
(51,'Allow trading between factions'),
(192,'Role: Sec Level Administrator'),
(193,'Role: Sec Level Gamemaster'),
(194,'Role: Sec Level Moderator'),
(195,'Role: Sec Level Player'),
(196,'Role: Administrator Commands'),
(197,'Role: Gamemaster Commands'),
(198,'Role: Moderator Commands'),
(199,'Role: Player Commands'),
(200,'Command: rbac'),
(201,'Command: rbac account'),
(202,'Command: rbac account list'),
(203,'Command: rbac account grant'),
(204,'Command: rbac account deny'),
(205,'Command: rbac account revoke'),
(206,'Command: rbac list'),
(207,'Command: battlenetaccount'),
(208,'Command: battlenetaccount create'),
(209,'Command: battlenetaccount lock country'),
(210,'Command: battlenetaccount lock ip'),
(211,'Command: battlenetaccount password'),
(212,'Command: battlenetaccount set'),
(213,'Command: battlenetaccount set password'),
(214,'Command: bnetaccount link'),
(215,'Command: bnetaccount unlink'),
(216,'Command: bnetaccount gameaccountcreate'),
(217,'Command: account'),
(218,'Command: account addon'),
(219,'Command: account create'),
(220,'Command: account delete'),
(221,'Command: account lock'),
(222,'Command: account lock country'),
(223,'Command: account lock ip'),
(224,'Command: account onlinelist'),
(225,'Command: account password'),
(226,'Command: account set'),
(227,'Command: account set addon'),
(228,'Command: account set gmlevel'),
(229,'Command: account set password'),
(230,'Command: achievement'),
(231,'Command: achievement add'),
(232,'Command: arena'),
(233,'Command: arena captain'),
(234,'Command: arena create'),
(235,'Command: arena disband'),
(236,'Command: arena info'),
(237,'Command: arena lookup'),
(238,'Command: arena rename'),
(239,'Command: ban'),
(240,'Command: ban account'),
(241,'Command: ban character'),
(242,'Command: ban ip'),
(243,'Command: ban playeraccount'),
(244,'Command: baninfo'),
(245,'Command: baninfo account'),
(246,'Command: baninfo character'),
(247,'Command: baninfo ip'),
(248,'Command: banlist'),
(249,'Command: banlist account'),
(250,'Command: banlist character'),
(251,'Command: banlist ip'),
(252,'Command: unban'),
(253,'Command: unban account'),
(254,'Command: unban character'),
(255,'Command: unban ip'),
(256,'Command: unban playeraccount'),
(257,'Command: bf'),
(258,'Command: bf start'),
(259,'Command: bf stop'),
(260,'Command: bf switch'),
(261,'Command: bf timer'),
(262,'Command: bf enable'),
(263,'Command: account email'),
(264,'Command: account set sec'),
(265,'Command: account set sec email'),
(266,'Command: account set sec regmail'),
(267,'Command: cast'),
(268,'Command: cast back'),
(269,'Command: cast dist'),
(270,'Command: cast self'),
(271,'Command: cast target'),
(272,'Command: cast dest'),
(273,'Command: character'),
(274,'Command: character customize'),
(275,'Command: character changefaction'),
(276,'Command: character changerace'),
(277,'Command: character deleted'),
(279,'Command: character deleted list'),
(280,'Command: character deleted restore'),
(283,'Command: character level'),
(284,'Command: character rename'),
(285,'Command: character reputation'),
(286,'Command: character titles'),
(287,'Command: levelup'),
(288,'Command: pdump'),
(289,'Command: pdump load'),
(290,'Command: pdump write'),
(291,'Command: cheat'),
(292,'Command: cheat casttime'),
(293,'Command: cheat cooldown'),
(294,'Command: cheat explore'),
(295,'Command: cheat god'),
(296,'Command: cheat power'),
(297,'Command: cheat status'),
(298,'Command: cheat taxi'),
(299,'Command: cheat waterwalk'),
(300,'Command: debug'),
(301,'Command: debug anim'),
(302,'Command: debug areatriggers'),
(303,'Command: debug arena'),
(304,'Command: debug bg'),
(305,'Command: debug entervehicle'),
(306,'Command: debug getitemstate'),
(309,'Command: debug combat'),
(310,'Command: debug itemexpire'),
(311,'Command: debug lootrecipient'),
(312,'Command: debug los'),
(314,'Command: debug moveflags'),
(315,'Command: debug play'),
(316,'Command: debug play cinematics'),
(317,'Command: debug play movie'),
(318,'Command: debug play sound'),
(319,'Command: debug send'),
(320,'Command: debug send buyerror'),
(321,'Command: debug send channelnotify'),
(322,'Command: debug send chatmessage'),
(323,'Command: debug send equiperror'),
(324,'Command: debug send largepacket'),
(325,'Command: debug send opcode'),
(326,'Command: debug send qinvalidmsg'),
(327,'Command: debug send qpartymsg'),
(328,'Command: debug send sellerror'),
(329,'Command: debug send setphaseshift'),
(330,'Command: debug send spellfail'),
(331,'Command: debug setaurastate'),
(335,'Command: debug setvid'),
(336,'Command: debug spawnvehicle'),
(337,'Command: debug threat'),
(339,'Command: debug worldstate'),
(340,'Command: wpgps'),
(341,'Command: deserter'),
(342,'Command: deserter bg'),
(343,'Command: deserter bg add'),
(344,'Command: deserter bg remove'),
(345,'Command: deserter instance'),
(346,'Command: deserter instance add'),
(347,'Command: deserter instance remove'),
(348,'Command: disable'),
(349,'Command: disable add'),
(350,'Command: disable add criteria'),
(351,'Command: disable add battleground'),
(352,'Command: disable add map'),
(353,'Command: disable add mmap'),
(354,'Command: disable add outdoorpvp'),
(355,'Command: disable add quest'),
(356,'Command: disable add spell'),
(357,'Command: disable add vmap'),
(358,'Command: disable remove'),
(359,'Command: disable remove criteria'),
(360,'Command: disable remove battleground'),
(361,'Command: disable remove map'),
(362,'Command: disable remove mmap'),
(363,'Command: disable remove outdoorpvp'),
(364,'Command: disable remove quest'),
(365,'Command: disable remove spell'),
(366,'Command: disable remove vmap'),
(367,'Command: event info'),
(368,'Command: event activelist'),
(369,'Command: event start'),
(370,'Command: event stop'),
(371,'Command: gm'),
(372,'Command: gm chat'),
(373,'Command: gm fly'),
(374,'Command: gm ingame'),
(375,'Command: gm list'),
(376,'Command: gm visible'),
(377,'Command: go'),
(378,'Command: account 2fa'),
(379,'Command: account 2fa setup'),
(380,'Command: account 2fa remove'),
(381,'Command: account set 2fa'),
(387,'Command: gobject'),
(388,'Command: gobject activate'),
(389,'Command: gobject add'),
(390,'Command: gobject add temp'),
(391,'Command: gobject delete'),
(392,'Command: gobject info'),
(393,'Command: gobject move'),
(394,'Command: gobject near'),
(395,'Command: gobject set'),
(396,'Command: gobject set phase'),
(397,'Command: gobject set state'),
(398,'Command: gobject target'),
(399,'Command: gobject turn'),
(400,'debug transport'),
(401,'Command: guild'),
(402,'Command: guild create'),
(403,'Command: guild delete'),
(404,'Command: guild invite'),
(405,'Command: guild uninvite'),
(406,'Command: guild rank'),
(407,'Command: guild rename'),
(408,'Command: honor'),
(409,'Command: honor add'),
(410,'Command: honor add kill'),
(411,'Command: honor update'),
(412,'Command: instance'),
(413,'Command: instance listbinds'),
(414,'Command: instance unbind'),
(415,'Command: instance stats'),
(417,'Command: learn'),
(418,'Command: learn all'),
(419,'Command: learn all my'),
(420,'Command: learn all my class'),
(421,'Command: learn all my pettalents'),
(422,'Command: learn all my spells'),
(423,'Command: learn all my talents'),
(424,'Command: learn all gm'),
(425,'Command: learn all crafts'),
(426,'Command: learn all default'),
(427,'Command: learn all lang'),
(428,'Command: learn all recipes'),
(429,'Command: unlearn'),
(430,'Command: lfg'),
(431,'Command: lfg player'),
(432,'Command: lfg group'),
(433,'Command: lfg queue'),
(434,'Command: lfg clean'),
(435,'Command: lfg options'),
(436,'Command: list'),
(437,'Command: list creature'),
(438,'Command: list item'),
(439,'Command: list object'),
(440,'Command: list auras'),
(441,'Command: list mail'),
(442,'Command: lookup'),
(443,'Command: lookup area'),
(444,'Command: lookup creature'),
(445,'Command: lookup event'),
(446,'Command: lookup faction'),
(447,'Command: lookup item'),
(448,'Command: lookup itemset'),
(449,'Command: lookup object'),
(450,'Command: lookup quest'),
(451,'Command: lookup player'),
(452,'Command: lookup player ip'),
(453,'Command: lookup player account'),
(454,'Command: lookup player email'),
(455,'Command: lookup skill'),
(456,'Command: lookup spell'),
(457,'Command: lookup spell id'),
(458,'Command: lookup taxinode'),
(459,'Command: lookup tele'),
(460,'Command: lookup title'),
(461,'Command: lookup map'),
(462,'Command: announce'),
(463,'Command: channel'),
(464,'Command: channel set'),
(465,'Command: channel set ownership'),
(466,'Command: gmannounce'),
(467,'Command: gmnameannounce'),
(468,'Command: gmnotify'),
(469,'Command: nameannounce'),
(470,'Command: notify'),
(471,'Command: whispers'),
(472,'Command: group'),
(473,'Command: group leader'),
(474,'Command: group disband'),
(475,'Command: group remove'),
(476,'Command: group join'),
(477,'Command: group list'),
(478,'Command: group summon'),
(479,'Command: pet'),
(480,'Command: pet create'),
(481,'Command: pet learn'),
(482,'Command: pet unlearn'),
(483,'Command: send'),
(484,'Command: send items'),
(485,'Command: send mail'),
(486,'Command: send message'),
(487,'Command: send money'),
(488,'Command: additem'),
(489,'Command: additemset'),
(490,'Command: appear'),
(491,'Command: aura'),
(492,'Command: bank'),
(493,'Command: bindsight'),
(494,'Command: combatstop'),
(495,'Command: cometome'),
(496,'Command: commands'),
(497,'Command: cooldown'),
(498,'Command: damage'),
(499,'Command: dev'),
(500,'Command: die'),
(501,'Command: dismount'),
(502,'Command: distance'),
(503,'Command: flusharenapoints'),
(504,'Command: freeze'),
(505,'Command: gps'),
(506,'Command: guid'),
(507,'Command: help'),
(508,'Command: hidearea'),
(509,'Command: itemmove'),
(510,'Command: kick'),
(511,'Command: linkgrave'),
(512,'Command: listfreeze'),
(513,'Command: maxskill'),
(514,'Command: movegens'),
(515,'Command: mute'),
(516,'Command: neargrave'),
(517,'Command: pinfo'),
(518,'Command: playall'),
(519,'Command: possess'),
(520,'Command: recall'),
(521,'Command: repairitems'),
(522,'Command: respawn'),
(523,'Command: revive'),
(524,'Command: saveall'),
(525,'Command: save'),
(526,'Command: setskill'),
(527,'Command: showarea'),
(528,'Command: summon'),
(529,'Command: unaura'),
(530,'Command: unbindsight'),
(531,'Command: unfreeze'),
(532,'Command: unmute'),
(533,'Command: unpossess'),
(534,'Command: unstuck'),
(535,'Command: wchange'),
(536,'Command: mmap'),
(537,'Command: mmap loadedtiles'),
(538,'Command: mmap loc'),
(539,'Command: mmap path'),
(540,'Command: mmap stats'),
(541,'Command: mmap testarea'),
(542,'Command: morph'),
(543,'Command: demorph'),
(544,'Command: modify'),
(545,'Command: modify arenapoints'),
(547,'Command: modify drunk'),
(548,'Command: modify energy'),
(549,'Command: modify faction'),
(550,'Command: modify gender'),
(551,'Command: modify honor'),
(552,'Command: modify hp'),
(553,'Command: modify mana'),
(554,'Command: modify money'),
(555,'Command: modify mount'),
(556,'Command: modify phase'),
(557,'Command: modify rage'),
(558,'Command: modify reputation'),
(559,'Command: modify runicpower'),
(560,'Command: modify scale'),
(561,'Command: modify speed'),
(562,'Command: modify speed all'),
(563,'Command: modify speed backwalk'),
(564,'Command: modify speed fly'),
(565,'Command: modify speed walk'),
(566,'Command: modify speed swim'),
(567,'Command: modify spell'),
(568,'Command: modify standstate'),
(569,'Command: modify talentpoints'),
(570,'Command: npc'),
(571,'Command: npc add'),
(572,'Command: npc add formation'),
(573,'Command: npc add item'),
(574,'Command: npc add move'),
(575,'Command: npc add temp'),
(576,'Command: npc add delete'),
(577,'Command: npc add delete item'),
(578,'Command: npc add follow'),
(579,'Command: npc add follow stop'),
(580,'Command: npc set'),
(581,'Command: npc set allowmove'),
(582,'Command: npc set entry'),
(583,'Command: npc set factionid'),
(584,'Command: npc set flag'),
(585,'Command: npc set level'),
(586,'Command: npc set link'),
(587,'Command: npc set model'),
(588,'Command: npc set movetype'),
(589,'Command: npc set phase'),
(590,'Command: npc set spawndist'),
(591,'Command: npc set spawntime'),
(592,'Command: npc set data'),
(593,'Command: npc info'),
(594,'Command: npc near'),
(595,'Command: npc move'),
(596,'Command: npc playemote'),
(597,'Command: npc say'),
(598,'Command: npc textemote'),
(599,'Command: npc whisper'),
(600,'Command: npc yell'),
(601,'Command: npc tame'),
(602,'Command: quest'),
(603,'Command: quest add'),
(604,'Command: quest complete'),
(605,'Command: quest remove'),
(606,'Command: quest reward'),
(607,'Command: reload'),
(608,'Command: reload access_requirement'),
(609,'Command: reload criteria_data'),
(610,'Command: reload achievement_reward'),
(611,'Command: reload all'),
(612,'Command: reload all achievement'),
(613,'Command: reload all area'),
(615,'Command: reload all gossips'),
(616,'Command: reload all item'),
(617,'Command: reload all locales'),
(618,'Command: reload all loot'),
(619,'Command: reload all npc'),
(620,'Command: reload all quest'),
(621,'Command: reload all scripts'),
(623,'Command: reload areatrigger_involvedrelation'),
(624,'Command: reload areatrigger_tavern'),
(625,'Command: reload areatrigger_teleport'),
(626,'Command: reload auctions'),
(627,'Command: reload autobroadcast'),
(628,'Command: reload command'),
(629,'Command: reload conditions'),
(630,'Command: reload config'),
(631,'Command: reload battleground_template'),
(632,'Command: .mutehistory'),
(633,'Command: reload creature_linked_respawn'),
(634,'Command: reload creature_loot_template'),
(635,'Command: reload creature_onkill_reputation'),
(636,'Command: reload creature_questender'),
(637,'Command: reload creature_queststarter'),
(638,'Command: reload creature_summon_groups'),
(639,'Command: reload creature_template'),
(640,'Command: reload creature_text'),
(641,'Command: reload disables'),
(642,'Command: reload disenchant_loot_template'),
(643,'Command: reload event_scripts'),
(644,'Command: reload fishing_loot_template'),
(645,'Command: reload game_graveyard_zone'),
(646,'Command: reload game_tele'),
(647,'Command: reload gameobject_questender'),
(648,'Command: reload gameobject_loot_template'),
(649,'Command: reload gameobject_queststarter'),
(650,'Command: reload support'),
(651,'Command: reload gossip_menu'),
(652,'Command: reload gossip_menu_option'),
(653,'Command: reload item_random_bonus_list_template'),
(654,'Command: reload item_loot_template'),
(655,'Command: reload item_set_names'),
(656,'Command: reload lfg_dungeon_rewards'),
(657,'Command: reload locales_achievement_reward'),
(658,'Command: reload locales_creature'),
(659,'Command: reload locales_creature_text'),
(660,'Command: reload locales_gameobject'),
(661,'Command: reload locales_gossip_menu_option'),
(663,'Command: reload locales_item_set_name'),
(665,'Command: reload locales_page_text'),
(666,'Command: reload locales_points_of_interest'),
(667,'Command: reload quest_locale'),
(668,'Command: reload mail_level_reward'),
(669,'Command: reload mail_loot_template'),
(670,'Command: reload milling_loot_template'),
(671,'Command: reload npc_spellclick_spells'),
(672,'Command: reload npc_trainer'),
(673,'Command: reload npc_vendor'),
(674,'Command: reload page_text'),
(675,'Command: reload pickpocketing_loot_template'),
(676,'Command: reload points_of_interest'),
(677,'Command: reload prospecting_loot_template'),
(678,'Command: reload quest_poi'),
(679,'Command: reload quest_template'),
(680,'Command: reload rbac'),
(681,'Command: reload reference_loot_template'),
(682,'Command: reload reserved_name'),
(683,'Command: reload reputation_reward_rate'),
(684,'Command: reload reputation_spillover_template'),
(685,'Command: reload skill_discovery_template'),
(686,'Command: reload skill_extra_item_template'),
(687,'Command: reload skill_fishing_base_level'),
(688,'Command: reload skinning_loot_template'),
(689,'Command: reload smart_scripts'),
(690,'Command: reload spell_required'),
(691,'Command: reload spell_area'),
(693,'Command: reload spell_group'),
(694,'Command: reload spell_learn_spell'),
(695,'Command: reload spell_loot_template'),
(696,'Command: reload spell_linked_spell'),
(697,'Command: reload spell_pet_auras'),
(698,'Command: character changeaccount'),
(699,'Command: reload spell_proc'),
(700,'Command: reload spell_scripts'),
(701,'Command: reload spell_target_position'),
(702,'Command: reload spell_threats'),
(703,'Command: reload spell_group_stack_rules'),
(704,'Command: reload trinity_string'),
(707,'Command: reload waypoint_path'),
(708,'Command: reload vehicle_accessory'),
(709,'Command: reload vehicle_template_accessory'),
(710,'Command: reset'),
(711,'Command: reset achievements'),
(712,'Command: reset honor'),
(713,'Command: reset level'),
(714,'Command: reset spells'),
(715,'Command: reset stats'),
(716,'Command: reset talents'),
(717,'Command: reset all'),
(718,'Command: server'),
(719,'Command: server corpses'),
(720,'Command: server exit'),
(721,'Command: server idlerestart'),
(722,'Command: server idlerestart cancel'),
(723,'Command: server idleshutdown'),
(724,'Command: server idleshutdown cancel'),
(725,'Command: server info'),
(726,'Command: server plimit'),
(727,'Command: server restart'),
(728,'Command: server restart cancel'),
(729,'Command: server set'),
(730,'Command: server set closed'),
(731,'Command: server set difftime'),
(732,'Command: server set loglevel'),
(733,'Command: server set motd'),
(734,'Command: server shutdown'),
(735,'Command: server shutdown cancel'),
(736,'Command: server motd'),
(737,'Command: tele'),
(738,'Command: tele add'),
(739,'Command: tele del'),
(740,'Command: tele name'),
(741,'Command: tele group'),
(742,'Command: ticket'),
(753,'Command: ticket reset'),
(757,'Command: ticket togglesystem'),
(761,'Command: titles'),
(762,'Command: titles add'),
(763,'Command: titles current'),
(764,'Command: titles remove'),
(765,'Command: titles set'),
(766,'Command: titles set mask'),
(767,'Command: wp'),
(768,'Command: wp add'),
(769,'Command: wp event'),
(770,'Command: wp load'),
(771,'Command: wp modify'),
(772,'Command: wp unload'),
(773,'Command: wp reload'),
(774,'Command: wp show'),
(775,'Command: modify currency'),
(776,'Command: debug phase'),
(777,'Command: mailbox'),
(778,'Command: ahbot'),
(779,'Command: ahbot items'),
(780,'Command: ahbot items gray'),
(781,'Command: ahbot items white'),
(782,'Command: ahbot items green'),
(783,'Command: ahbot items blue'),
(784,'Command: ahbot items purple'),
(785,'Command: ahbot items orange'),
(786,'Command: ahbot items yellow'),
(787,'Command: ahbot ratio'),
(788,'Command: ahbot ratio alliance'),
(789,'Command: ahbot ratio horde'),
(790,'Command: ahbot ratio neutral'),
(791,'Command: ahbot rebuild'),
(792,'Command: ahbot reload'),
(793,'Command: ahbot status'),
(794,'Command: .guild info'),
(795,'Command: instance setbossstate'),
(796,'Command: instance getbossstate'),
(797,'Command: pvpstats'),
(798,'Command: .mod xp'),
(802,'Command: .ticket bug'),
(803,'Command: .ticket complaint'),
(804,'Command: .ticket suggestion'),
(805,'Command: .ticket bug assign'),
(806,'Command: .ticket bug close'),
(807,'Command: .ticket bug closedlist'),
(808,'Command: .ticket bug comment'),
(809,'Command: .ticket bug delete'),
(810,'Command: .ticket bug list'),
(811,'Command: .ticket bug unassign'),
(812,'Command: .ticket bug view'),
(813,'Command: .ticket complaint assign'),
(814,'Command: .ticket complaint close'),
(815,'Command: .ticket complaint closedlist'),
(816,'Command: .ticket complaint comment'),
(817,'Command: .ticket complaint delete'),
(818,'Command: .ticket complaint list'),
(819,'Command: .ticket complaint unassign'),
(820,'Command: .ticket complaint view'),
(821,'Command: .ticket suggestion assign'),
(822,'Command: .ticket suggestion close'),
(823,'Command: .ticket suggestion closedlist'),
(824,'Command: .ticket suggestion comment'),
(825,'Command: .ticket suggestion delete'),
(826,'Command: .ticket suggestion list'),
(827,'Command: .ticket suggestion unassign'),
(828,'Command: .ticket suggestion view'),
(829,'Command: .ticket reset all'),
(830,'Command: .bnetaccount listgameaccounts'),
(831,'Command: .ticket reset bug'),
(832,'Command: .ticket reset complaint'),
(833,'Command: .ticket reset suggestion'),
(835,'Command: debug loadcells'),
(836,'Command: .debug boundary'),
(837,'Command: npc evade'),
(838,'Command: pet level'),
(839,'Command: server shutdown force'),
(840,'Command: server restart force'),
(842,'Command: reload character_template'),
(843,'Command: reload quest_greeting'),
(844,'Command: scene'),
(845,'Command: scene debug'),
(846,'Command: scene play'),
(847,'Command: scene play package'),
(848,'Command: scene cancel'),
(849,'Command: list scenes'),
(850,'Command: reload scenes'),
(851,'Command: reload areatrigger_templates'),
(852,'Command: debug dummy'),
(853,'Command: .reload conversation_template'),
(854,'Command: .debug conversation'),
(855,'Command: debug play music'),
(856,'Command: npc spawngroup'),
(857,'Command: npc despawngroup'),
(858,'Command: gobject spawngroup'),
(859,'Command: gobject despawngroup'),
(860,'Command: list respawns'),
(865,'Command: npc showloot'),
(866,'Command: list spawnpoints'),
(868,'Command: modify power'),
(869,'Command: debug send playerchoice'),
(870,'Command: debug threatinfo'),
(871,'Command: debug instancespawn'),
(872,'Command: server debug'),
(873,'Command: reload creature_movement_override'),
(874,'Command: debug asan'),
(875,'Command: lookup map id'),
(876,'Command: lookup item id'),
(877,'Command: lookup quest id'),
(878,'Command: debug questreset'),
(879,'Command: debug poolstatus'),
(880,'Command: pdump copy'),
(881,'Command: reload vehicle_template'),
(882,'Command: reload spell_script_names'),
(883,'Command: quest objective complete');
/*!40000 ALTER TABLE `rbac_permissions` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `realmcharacters`
--

DROP TABLE IF EXISTS `realmcharacters`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `realmcharacters` (
  `realmid` int unsigned NOT NULL DEFAULT '0',
  `acctid` int unsigned NOT NULL,
  `numchars` tinyint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`realmid`,`acctid`),
  KEY `acctid` (`acctid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Realm Character Tracker';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `realmcharacters`
--

LOCK TABLES `realmcharacters` WRITE;
/*!40000 ALTER TABLE `realmcharacters` DISABLE KEYS */;
/*!40000 ALTER TABLE `realmcharacters` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `realmlist`
--

DROP TABLE IF EXISTS `realmlist`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `realmlist` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `address` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '127.0.0.1',
  `localAddress` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '127.0.0.1',
  `address3` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `address4` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `localSubnetMask` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '255.255.255.0',
  `port` smallint unsigned NOT NULL DEFAULT '8085',
  `icon` tinyint unsigned NOT NULL DEFAULT '0',
  `flag` tinyint unsigned NOT NULL DEFAULT '2',
  `timezone` tinyint unsigned NOT NULL DEFAULT '0',
  `allowedSecurityLevel` tinyint unsigned NOT NULL DEFAULT '0',
  `population` float NOT NULL DEFAULT '0',
  `gamebuild` int unsigned NOT NULL DEFAULT '61967',
  `Region` tinyint unsigned NOT NULL DEFAULT '1',
  `Battlegroup` tinyint unsigned NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`),
  UNIQUE KEY `idx_name` (`name`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Realm System';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `realmlist`
--

LOCK TABLES `realmlist` WRITE;
/*!40000 ALTER TABLE `realmlist` DISABLE KEYS */;
INSERT INTO `realmlist` VALUES
(1,'Trinity','127.0.0.1','127.0.0.1',NULL,NULL,'255.255.255.0',8085,0,0,1,0,0,61967,1,1);
/*!40000 ALTER TABLE `realmlist` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `secret_digest`
--

DROP TABLE IF EXISTS `secret_digest`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `secret_digest` (
  `id` int unsigned NOT NULL,
  `digest` varchar(100) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `secret_digest`
--

LOCK TABLES `secret_digest` WRITE;
/*!40000 ALTER TABLE `secret_digest` DISABLE KEYS */;
/*!40000 ALTER TABLE `secret_digest` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `updates`
--

DROP TABLE IF EXISTS `updates`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `updates` (
  `name` varchar(200) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT 'filename with extension of the update.',
  `hash` char(40) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT '' COMMENT 'sha1 hash of the sql file.',
  `state` enum('RELEASED','ARCHIVED') CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'RELEASED' COMMENT 'defines if an update is released or archived.',
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'timestamp when the query was applied.',
  `speed` int unsigned NOT NULL DEFAULT '0' COMMENT 'time the query takes to apply in ms.',
  PRIMARY KEY (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='List of all applied updates in this database.';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `updates`
--

LOCK TABLES `updates` WRITE;
/*!40000 ALTER TABLE `updates` DISABLE KEYS */;
INSERT INTO `updates` VALUES
('2014_10_04_00_auth.sql','C3BC70A6EC381474B7308F442346F1E721176BC6','ARCHIVED','2015-03-21 16:55:52',0),
('2014_10_19_00_auth.sql','7472B490A4F86C9D3DA609CDD3197499CB80C87C','ARCHIVED','2015-03-21 16:55:52',0),
('2014_10_26_00_auth.sql','75CC67ADE2A3B2E54FD57D6B0DCAA8FE50F4EE35','ARCHIVED','2015-03-21 16:55:52',0),
('2014_11_03_00_auth.sql','5948C9F286CF0FEA8E241785C0259FF36B73BDC5','ARCHIVED','2015-03-21 16:55:52',0),
('2014_11_04_00_auth.sql','3AFC68B2375C2A417DDEA94583C53AFF83DE50DF','ARCHIVED','2015-03-21 16:55:52',0),
('2014_11_09_00_auth.sql','B8DD1A7047C0FDDB80344B239343EC33BF1A0D97','ARCHIVED','2015-03-21 16:55:52',0),
('2014_11_10_00_auth_from_335.sql','0E3CB119442D09DD88E967015319BBC8DAFBBFE0','ARCHIVED','2014-11-10 00:00:00',0),
('2014_11_10_00_auth.sql','8FBA737A1D3FF4631A1E662A5B500A8BD304EC63','ARCHIVED','2015-03-21 16:55:52',0),
('2014_11_10_01_auth.sql','327E77A1DA3546D5275AB249915DD57EDD6FDD3D','ARCHIVED','2014-11-10 00:00:01',0),
('2014_11_23_00_auth.sql','0BBEB3EB3AED0FEF277A062819B6B2C00084A742','ARCHIVED','2015-03-21 16:55:52',0),
('2014_11_25_00_auth.sql','4F45CDB26BDBB3EE83F1988E3D7818C5926ADC02','ARCHIVED','2015-03-21 16:55:52',0),
('2014_12_05_00_auth.sql','6A7BBCEF43111C73A2D2C3CCB6911BE50DE7DD94','ARCHIVED','2015-03-21 16:55:52',0),
('2014_12_10_00_auth.sql','821703A96D80F9080074852B5A46E2909C9562EA','ARCHIVED','2014-12-10 00:00:00',0),
('2014_12_19_00_auth.sql','44D8E12FFF327AD07878FBDF8D9C16B6B7DCB122','ARCHIVED','2015-03-21 16:55:52',0),
('2014_12_20_00_auth.sql','4DAA02AE285C02AE6C82EA2C8B97AC71990F1085','ARCHIVED','2015-03-21 16:55:52',0),
('2014_12_25_00_auth.sql','61411930F482BC73FC7FD2C370C811E944F5FF92','ARCHIVED','2015-03-21 16:55:52',0),
('2014_12_27_00_auth.sql','CE2E5D2CD82E79C25294539ADED27A1429105B43','ARCHIVED','2014-12-21 00:00:00',0),
('2014_12_28_00_auth.sql','0A913217610E76AFF119C27259737BBC523090E6','ARCHIVED','2015-03-21 16:55:52',0),
('2015_02_22_00_auth.sql','21CCCF8B01252E16CA3D6C9E3E8DAA4C9B28ED6E','ARCHIVED','2015-03-21 16:55:52',0),
('2015_03_01_00_auth.sql','911881E273207FF6182D1FDAC8C85FFAE8F1C852','ARCHIVED','2015-03-21 16:55:52',0),
('2015_03_10_00_auth.sql','2CC8502C11412EFEB5C11BE166761A8754A59009','ARCHIVED','2015-03-21 16:55:52',0),
('2015_03_15_00_auth.sql','1D8E107FBEFE5E7F47E09F45240DFF499B77CDED','ARCHIVED','2015-05-02 13:57:57',0),
('2015_03_20_00_auth.sql','B761760804EA73BD297F296C5C1919687DF7191C','ARCHIVED','2015-03-21 16:55:52',0),
('2015_03_20_01_auth.sql','5CCEDF20C8189FB1E8DF064A9F0DDC342841FBF0','ARCHIVED','2015-03-21 16:55:52',0),
('2015_03_20_02_auth.sql','85E4ACD9AA099C0C4AC034575F2BB07D348EAC72','ARCHIVED','2015-03-21 16:56:46',0),
('2015_03_26_00_auth.sql','34AC8543E6A9C6C832DE58EAB33618EEEF70B9F9','ARCHIVED','2015-05-02 13:57:57',0),
('2015_04_04_00_auth.sql','57146B35E54A2EC7869C945034AB078358020311','ARCHIVED','2015-05-02 13:57:57',0),
('2015_04_06_00_auth.sql','2A8049DC2923420A002D42FB6F02C2FFCC5CDD22','ARCHIVED','2015-05-02 13:57:57',0),
('2015_04_08_00_auth.sql','4D7D8EEF285C982BB676836602266501BEC26764','ARCHIVED','2015-05-02 13:57:57',0),
('2015_04_10_00_auth.sql','4AE68FD97A95CEE5143EA20FD33F5D557367AC1F','ARCHIVED','2015-05-02 13:57:57',0),
('2015_04_11_00_auth.sql','80A71C8921CFEBB547D264558B6DE27201685B84','ARCHIVED','2015-05-02 13:57:57',0),
('2015_04_11_01_auth.sql','3E88183E1A85D11BFD74CF9A32A725C44AE02EEC','ARCHIVED','2015-05-02 13:57:57',0),
('2015_04_21_00_auth.sql','1B3B48DBA06368B985C548D166C515C9DD598CB9','ARCHIVED','2015-05-02 13:57:57',0),
('2015_05_02_00_auth.sql','96AB595E0D2A088750E3F48B0AF0A8A14F3CFE1E','ARCHIVED','2015-05-02 13:57:57',0),
('2015_05_02_01_auth.sql','FB11FB834E488B0FD3AFDABCC1A3113092E7C2E5','ARCHIVED','2015-05-02 13:57:57',0),
('2015_07_02_00_auth.sql','E5EE3842AB9B01851E49B360FBAF6FFEEAB2A8DA','ARCHIVED','2015-07-10 19:30:56',0),
('2015_07_06_00_auth.sql','6D1ADBA496DC6E6D7B3BF887DA8D4D17D3FBACE0','ARCHIVED','2015-07-10 19:30:56',0),
('2015_07_08_00_auth.sql','CB54020AFD1E31742FD8BF9CE16879625E289788','ARCHIVED','2015-07-10 19:30:56',0),
('2015_07_08_01_auth.sql','74D281CB82E0DA36D628BDC7AC797AE5498DB461','ARCHIVED','2015-07-10 19:30:56',0),
('2015_07_16_00_auth.sql','A057E95B5553B6A57A1642FE3FEC8E2E62EDE3C6','ARCHIVED','2015-10-10 08:30:48',0),
('2015_07_29_00_auth.sql','0000FECBC413E96C7C45F303D162E263EFBA7116','ARCHIVED','2015-10-10 08:30:48',0),
('2015_08_26_00_auth.sql','3071C02A2EB7DCBF4CEE10279FEFAB7C29A43A3A','ARCHIVED','2015-10-10 08:30:48',0),
('2015_09_05_00_auth.sql','F765D82B37873FA67447347D5B83C99C159FB452','ARCHIVED','2015-10-10 08:30:48',0),
('2015_09_05_01_auth.sql','97A72DBCBF14D27A1863834A22296905FF276086','ARCHIVED','2015-10-10 08:30:48',0),
('2015_09_09_00_auth.sql','495A0CF1B1C49205D4A5D3C25A4E1EB95616D6B4','ARCHIVED','2015-10-10 08:30:48',0),
('2015_09_15_00_auth.sql','D1FEFDA4C98F30384DF4B64D5A53187303EB5786','ARCHIVED','2015-10-10 08:30:48',0),
('2015_10_09_00_auth.sql','B6D643D444C6AE711503F73B96B6252A852913D6','ARCHIVED','2015-10-10 08:30:48',0),
('2015_10_16_00_auth.sql','366AFFD1088762866091A81CE1EC64138B8B35F1','ARCHIVED','2015-11-08 00:46:02',0),
('2015_10_17_00_auth.sql','AC0D45E905033F42093852D2C4476663BDACCB3D','ARCHIVED','2015-10-17 12:39:12',0),
('2015_11_01_00_auth_2015_08_21_00.sql','C31A9E1D28E11B60BE8F8198637DD51F6D75123F','ARCHIVED','2015-08-21 00:00:00',0),
('2015_11_08_00_auth.sql','0ACDD35EC9745231BCFA701B78056DEF94D0CC53','ARCHIVED','2015-11-07 00:00:00',0),
('2015_11_21_00_auth.sql','575A1D697CC6C7C517F7CCB950988267C99CE7FA','ARCHIVED','2015-11-21 21:25:38',0),
('2015_12_07_00_auth.sql','24A07AC1F38E5D26A3599FC06D29E267418F69F3','ARCHIVED','2015-12-07 20:55:48',0),
('2016_01_13_00_auth.sql','114527BCCB0DE286CBE6FDA3029DD0523D1037FA','ARCHIVED','2016-01-13 21:39:13',0),
('2016_03_22_01_auth_2016_01_13_00_auth.sql','24615CC69B3CD7BB4699874647C35BA86E8A93FD','ARCHIVED','2016-03-22 22:55:13',0),
('2016_03_28_00_auth.sql','BA14D23D81FA24565F04A359090DE86C5E195209','ARCHIVED','2016-03-28 16:49:32',0),
('2016_04_11_00_auth.sql','0ACDD35EC9745231BCFA701B78056DEF94D0CC53','ARCHIVED','2016-04-11 00:00:00',0),
('2016_04_17_00_auth.sql','83399B64D1221B56F73A0FFB51889F11A70521BC','ARCHIVED','2016-04-17 00:22:05',0),
('2016_05_07_00_auth.sql','7E36DCC4F06FCDCDA7155AF3C5EDF8D3A720565F','ARCHIVED','2016-05-07 01:00:21',0),
('2016_05_19_00_auth.sql','FB52E6BF35682CE6FA667B552B551F4FBD72AC30','ARCHIVED','2016-05-19 22:18:06',0),
('2016_07_19_00_auth.sql','D5498F28A1E21F4AD0E0D7C2B96FCF7292C14C4D','ARCHIVED','2016-07-19 14:00:28',0),
('2016_07_19_01_auth.sql','EBFE5D7D7E7CFA0CDA76AC49A1E8D4FA461A12BE','ARCHIVED','2016-07-19 16:06:39',0),
('2016_07_23_00_auth.sql','1048F6A922ACD9BFC2E4518A71AF7037F79A85C4','ARCHIVED','2016-07-23 14:39:21',0),
('2016_07_23_01_auth.sql','5897C7D8B8DE15895286FBCD1535FC75E1B70F62','ARCHIVED','2016-07-23 17:35:11',0),
('2016_07_30_00_auth.sql','0FD4147840F7F02E2F1828A904B269F5B66097E0','ARCHIVED','2016-07-30 15:07:02',0),
('2016_08_07_00_auth.sql','D9DD23851822E32E1312FFABEE2DB721C8651443','ARCHIVED','2016-08-07 15:33:42',0),
('2016_08_11_00_auth.sql','0C79A86A4DFC53746BECF3D8A145482F94AE5FC9','ARCHIVED','2016-08-11 17:02:20',0),
('2016_08_13_00_auth.sql','ED2286C4FF3D80D0F4DEE3D3121BCC15544470BE','ARCHIVED','2016-08-13 01:11:49',0),
('2016_08_26_00_auth.sql','3C566371B6026EFEEA19CD215EC9F02C6DA9EAB3','ARCHIVED','2016-08-26 14:09:52',0),
('2016_08_27_00_auth.sql','65ABEF7ACBCEA974C744ED42F95FBBD29226917B','ARCHIVED','2016-08-27 07:02:45',0),
('2016_08_30_00_auth.sql','E16C19A938FE6370921658D2B713EE28A633FD56','ARCHIVED','2016-08-30 00:00:00',0),
('2016_09_02_00_auth.sql','08932DAC4BDE74D3C39A43DDE404522F23EDD035','ARCHIVED','2016-09-02 00:00:00',0),
('2016_09_03_00_auth_2016_05_11_00_auth.sql','401EFD3586772BDED66B4A944C20A1AC18A22D3A','ARCHIVED','2016-09-03 11:29:38',0),
('2016_09_03_01_auth.sql','08B5ABCB74BBF25A30D37AF639F0EA1B10640673','ARCHIVED','2016-09-03 13:24:32',0),
('2016_09_03_02_auth_2016_06_06_00_auth.sql','A0A8D73A952D0618833416513D53F73A70E7EA25','ARCHIVED','2016-06-06 00:00:00',0),
('2016_09_03_03_auth.sql','9BF1C03EE39B6DC7E817BA46BE7D12A41AFBFDF7','ARCHIVED','2016-09-03 15:56:50',0),
('2016_09_15_00_auth.sql','CD65F822AF1B5B7776E39804D0362F3E34AA6445','ARCHIVED','2016-09-15 16:30:36',0),
('2016_09_21_00_auth.sql','57219A16B88080240EED94CDD41FC2764B8A32C5','ARCHIVED','2016-09-21 17:08:43',0),
('2016_09_25_00_auth.sql','E811EFD8CE92ABEC5B8C02A09E643035939CF96D','ARCHIVED','2016-09-25 15:56:58',0),
('2016_10_01_00_auth.sql','7C444FF1B03BA3C83472BDA409854754D052D6FB','ARCHIVED','2016-10-01 13:32:43',0),
('2016_10_06_00_auth.sql','6A415F9813EFB5B95EB2AA2B326E1A6791E25EDB','ARCHIVED','2016-10-06 23:16:24',0),
('2016_10_12_00_auth.sql','671D57BBA183AC70B9580DEE19B7EC046AF2EA87','ARCHIVED','2016-10-12 00:01:05',0),
('2016_10_17_00_auth.sql','A0EF594CD73690D46A46031137DB0E895F079235','ARCHIVED','2016-10-16 16:33:05',0),
('2016_10_25_00_auth.sql','5743FB1AC3F564FE4192DCFA90260BAD5E501882','ARCHIVED','2016-10-25 19:27:02',0),
('2016_10_28_00_auth.sql','C1B9B1DD20B2183C6CB44CAED9B91BA7C63B8C49','ARCHIVED','2016-10-28 00:07:48',0),
('2016_11_04_00_auth.sql','3F4FE06DCE019EB3223B5A6E0F80E2239078967F','ARCHIVED','2016-11-04 20:25:23',0),
('2016_11_09_00_auth.sql','56432F8AEC2943A398A5B8B77843138B5B704257','ARCHIVED','2016-11-09 18:46:48',0),
('2016_11_17_00_auth.sql','18E8F8FC93CC38755AB571638960AAFB98C0F3F1','ARCHIVED','2016-11-17 23:47:51',0),
('2016_12_04_00_auth.sql','B1623681EAB651D2A091E3F4D4D4E476CF6D3AEA','ARCHIVED','2016-12-04 00:41:36',0),
('2016_12_11_00_auth.sql','24CA34537DB697962DDD69EEE4BB5E79D2A573DA','ARCHIVED','2016-12-11 18:18:59',0),
('2016_12_18_00_auth.sql','7AB53E033680CF7439F142EF83CD13E6F5D0ACB9','ARCHIVED','2016-12-18 12:15:48',0),
('2017_01_14_00_auth.sql','1B514D1364042DB4CE68929EB54A94F86983441D','ARCHIVED','2017-01-14 20:50:47',0),
('2017_01_26_00_auth.sql','723E1B69981A32A2F28A67C64902BA1AE7E98E48','ARCHIVED','2017-01-26 17:10:15',0),
('2017_01_29_00_auth.sql','B76C514678903F540302505AF66886F7D2C89E30','ARCHIVED','2017-01-29 00:00:00',0),
('2017_03_11_00_auth.sql','2F2F67E51439346B212C27B7224E4614C00E1AEB','ARCHIVED','2017-03-11 00:00:00',0),
('2017_03_17_00_auth.sql','4902E9B1B063F399F928C2DD7AFD60427738E227','ARCHIVED','2017-03-17 00:00:00',0),
('2017_04_17_00_auth.sql','86299FAB21D895E84272286309CC8EE80F9DA8C7','ARCHIVED','2017-04-17 00:00:00',0),
('2017_04_19_00_auth.sql','9903AAF50DF384F52E81F7E2892FE5271E000490','ARCHIVED','2017-04-18 23:16:18',0),
('2017_04_22_00_auth.sql','843663B18D28FBA1EB12548500EC93953881E5F0','ARCHIVED','2017-04-22 19:28:22',0),
('2017_04_27_00_auth.sql','308B797B47FA803D492C9C9A4C26DBEC546DBBD9','ARCHIVED','2017-04-28 09:10:11',0),
('2017_05_14_00_auth.sql','B7E76CCDCC9A2C8103427DA4C43C7B0366ECE8B4','ARCHIVED','2017-05-14 12:00:00',0),
('2017_06_12_01_auth.sql','661B4935E101AF188BEBF43203144104E89F8C54','ARCHIVED','2017-06-12 00:00:01',0),
('2017_06_12_02_auth.sql','166F059E411FAA4901BBBA09A41EF07B1CADC4B6','ARCHIVED','2017-06-12 00:00:02',0),
('2017_06_15_00_auth.sql','DD71F25C1E61FD5F836931B02703BE3BD1B4F156','ARCHIVED','2017-06-15 15:20:50',0),
('2017_06_17_00_auth.sql','4A172895CB9DA8EFE1270434D6ECB22D4F4DCB17','ARCHIVED','2017-06-17 00:00:00',0),
('2017_06_18_00_auth.sql','7200968BFC2D76499149937B19F2153FD2ABC397','ARCHIVED','2017-06-18 22:13:37',0),
('2017_06_25_00_auth.sql','A2DA6A64D4217992EF766915DEBD517DB0834E01','ARCHIVED','2017-06-25 00:54:10',0),
('2017_06_28_00_auth_master.sql','6E58300D4D4DAAEE89107ECB3CB7DA8529DA738F','ARCHIVED','2017-06-28 19:11:09',0),
('2017_06_28_00_auth_rbac.sql','D32EF80F57F629C23395D80F06E91D7E40719F83','ARCHIVED','2017-06-28 00:00:00',0),
('2017_06_30_00_auth.sql','C73BD277D211DBE1BB86BB1B443CA8F292D8ADEE','ARCHIVED','2017-06-30 16:18:51',0),
('2017_08_01_00_auth.sql','6ECE808AF52345177189E962C0606B769B6806A6','ARCHIVED','2017-08-01 00:00:00',0),
('2017_08_04_00_auth.sql','2E994A704C64FECE3CE0883ED0CAC5E5A0E3A36C','ARCHIVED','2017-08-04 23:46:32',0),
('2017_08_13_00_auth_2016_09_22_00_auth.sql','70047954E3556BFA430ADD5680EF8797F74A4B9E','ARCHIVED','2017-08-13 12:00:00',0),
('2017_09_22_00_auth.sql','9313CCE80A18212E6F0C78D83316DE8582AE8084','ARCHIVED','2017-09-22 18:05:17',0),
('2017_10_13_00_auth.sql','87674E0D166AC60E3725B445714427892E42C6FE','ARCHIVED','2017-10-13 00:00:00',0),
('2017_11_11_01_auth.sql','0D6EDB6B2FC8B9FBDF11ECD79B4B8E943328B6A9','ARCHIVED','2017-11-11 18:49:45',0),
('2017_12_17_00_auth.sql','2CD99730D4D32DBF0584CD5B1AA6F8F4AE3DA975','ARCHIVED','2017-12-17 00:00:00',0),
('2017_12_30_00_auth.sql','F360E9555AC68E28834E3FF807E4E37A090EF363','ARCHIVED','2017-12-30 00:23:32',0),
('2017_12_30_01_auth.sql','1E11C78BA6D1D8E8CED7423DF92D1D197D6061EE','ARCHIVED','2017-12-30 23:00:00',0),
('2017_12_31_00_auth.sql','1721ACBD35EB95FAE33B9E95F8C4E4B1FB70A5E4','ARCHIVED','2017-12-31 20:15:23',0),
('2018_01_02_00_auth.sql','CD9B826B9D95697DC412DEF780E814FA3991D6CD','ARCHIVED','2018-01-02 20:40:37',0),
('2018_01_09_00_auth.sql','A5D4EC8FCFAB4F2DCE70EDCAD1ACBFB484FD68D5','ARCHIVED','2018-01-09 00:00:00',0),
('2018_01_24_00_auth.sql','167B17D8A253D62A8112F8A7EB21C6E99CAEF1E4','ARCHIVED','2018-01-24 00:00:00',0),
('2018_02_18_00_auth.sql','8489DD3EFFE14A7486B593435F0BA2BC69B6EABF','ARCHIVED','2018-02-18 16:35:55',0),
('2018_02_19_00_auth.sql','07CE658C5EF88693D3C047EF8E724F94ADA74C15','ARCHIVED','2018-02-19 22:33:32',0),
('2018_02_28_00_auth.sql','E92EF4ABF7FA0C66649E1633DD0459F44C09EB83','ARCHIVED','2018-02-28 23:07:59',0),
('2018_03_08_00_auth.sql','624C58A07E0B4DDC4C1347E8BA8EFEEFD5B43DA7','ARCHIVED','2018-03-08 00:00:00',0),
('2018_03_14_00_auth.sql','2D71E93DF7419A30D0D21D8A80CF05698302575A','ARCHIVED','2018-03-14 23:07:59',0),
('2018_04_06_00_auth.sql','D8416F0C4751763202B1997C81423F6EE2FCF9A6','ARCHIVED','2018-04-06 18:00:32',0),
('2018_05_13_00_auth.sql','A9E20F2EB1E2FDBB982DB6B00DB7301852B27CD4','ARCHIVED','2018-05-13 20:22:32',0),
('2018_05_24_00_auth.sql','B98FD71AAA13810856729E034E6B8C9F8D5D4F6B','ARCHIVED','2018-05-24 22:32:49',0),
('2018_06_14_00_auth.sql','67EAB915BF0C7F2D410BE45F885A1A39D42C8C14','ARCHIVED','2018-06-14 23:06:59',0),
('2018_06_22_00_auth.sql','9DA24F70B8A365AFDEF58A9B578255CDEDFCA47C','ARCHIVED','2018-06-22 17:45:45',0),
('2018_06_23_00_auth.sql','BE35312C386A127D047E5A7CE0D14DB41D905F8E','ARCHIVED','2018-06-23 00:00:00',0),
('2018_06_29_00_auth.sql','03AAEA7E52848FA5522C3F0C6D9C38B988407480','ARCHIVED','2018-06-29 22:34:04',0),
('2018_08_30_00_auth.sql','22F69864361D3E72F800379338310172C0576D1C','ARCHIVED','2018-08-30 00:00:00',0),
('2018_09_06_00_auth.sql','309D21E0DF82ED8921F77EAFDE741F38AC32BB13','ARCHIVED','2018-09-06 00:00:00',0),
('2018_09_17_00_auth.sql','4DB671F0A4FA1A93AF28FB6426AF13DE72C7DA3D','ARCHIVED','2018-09-17 00:00:00',0),
('2018_12_09_00_auth_2017_01_06_00_auth.sql','6CCFE6A9774EC733C9863D36A0F15F3534189BBD','ARCHIVED','2017-01-06 00:00:00',0),
('2018_12_09_01_auth.sql','576C2A11BE671D8420FA3EB705E594E381ECCC56','ARCHIVED','2018-12-09 14:49:17',0),
('2019_04_27_00_auth.sql','84B1EB9CC9B09BAF55E6295D202EC57D99B1B60E','ARCHIVED','2019-04-27 18:07:18',0),
('2019_06_06_00_auth.sql','6DE8159E04BEE7BA0A4A81D72D160EB74934B6A5','ARCHIVED','2019-06-06 18:09:54',0),
('2019_06_08_00_auth.sql','EA5A78F5A26C17BC790481EA9B3772D3A6912459','ARCHIVED','2019-05-20 17:21:20',0),
('2019_06_08_01_auth.sql','8165B1B787E3ECF0C8C0AD2D641513270977ABB4','ARCHIVED','2019-06-04 16:51:31',0),
('2019_06_08_02_auth.sql','B39DCBD902290700A81C9D028F54B58601C19A99','ARCHIVED','2019-06-05 16:26:31',0),
('2019_06_08_03_auth.sql','F483B657015D39D4F63E3905C27C3AA48241AB03','ARCHIVED','2019-06-08 17:14:21',0),
('2019_06_16_00_auth.sql','B14AED4D3387B56FF8C8161D3671750AEEAE0F2E','ARCHIVED','2019-06-15 23:32:12',0),
('2019_06_21_00_auth.sql','C519239830204B68E710F698BC0C9E89B6D5FD24','ARCHIVED','2019-06-20 19:43:50',0),
('2019_07_14_00_auth.sql','94C2B877BD906538E1E008350BEA8D8B58E0A158','ARCHIVED','2019-07-14 19:22:08',0),
('2019_07_15_00_auth.sql','3649248104CFEC70553016273069A9AE744798E3','ARCHIVED','2019-07-15 19:22:08',0),
('2019_07_16_00_auth.sql','36CB53A9EBD64BFDCF7030083E36E534F1753773','ARCHIVED','2019-07-16 00:00:00',0),
('2019_07_17_00_auth.sql','4F983F039904894ACC483BE885676C5F0A18F06B','ARCHIVED','2019-07-17 00:00:00',0),
('2019_07_26_00_auth.sql','DC9D0651602AE78B1243B40555A1A7B8447D01B2','ARCHIVED','2019-07-26 18:21:34',0),
('2019_08_10_00_auth.sql','3568A1C9C6D62BBCD470C0623C1580E332D545D2','ARCHIVED','2022-01-02 21:18:52',0),
('2019_08_10_01_auth.sql','C58357260F0C70DA226A71F7E05DE2C49AAEFD74','ARCHIVED','2019-08-10 00:00:00',0),
('2019_08_11_00_auth.sql','04DCC2ABDA15BC7C015E8BFEA383C62A362B166F','ARCHIVED','2019-08-11 10:56:39',0),
('2019_08_18_00_auth.sql','0479A04B669A67D2E5A498CFB91507E742EFB34F','ARCHIVED','2019-08-17 11:51:02',0),
('2019_10_27_00_auth.sql','C943A651B5C9AC51BB7DF69821886F4B59F57153','ARCHIVED','2019-10-27 13:06:06',0),
('2019_11_13_00_auth.sql','EB680BA7D6B3A21A432687F452CDD86FB2DA677C','ARCHIVED','2019-11-13 11:49:55',0),
('2019_11_20_00_auth.sql','9BC11595D9CEA486AC1540A204DCE9D86A008D7D','ARCHIVED','2019-11-20 12:31:56',0),
('2019_11_23_00_auth.sql','098708CDC5614B4523AD3B17670939671E661443','ARCHIVED','2019-11-23 12:20:03',0),
('2019_12_04_00_auth.sql','4EB3D028DD80B18DA9B9250ADA4D22AA0D3C0447','ARCHIVED','2019-12-04 18:03:39',0),
('2019_12_07_00_auth.sql','F354DA31D5B300609C6AE8A25667CA4DE0A7349F','ARCHIVED','2019-12-07 12:57:23',0),
('2019_12_10_00_auth.sql','CC0DC6211FB2A1271EBF5D81F47B3EF1CED7A7AD','ARCHIVED','2019-12-10 17:50:44',0),
('2019_12_14_00_auth.sql','CF577A0B6F9747658CDECE3F690B05B89C5B4470','ARCHIVED','2019-12-14 11:17:56',0),
('2020_01_11_00_auth.sql','A0C4863741C4B93D343B7837FBA38D6023A237F9','ARCHIVED','2020-01-11 13:44:51',0),
('2020_01_12_00_auth.sql','5E4ECF243259B9866A877E2D3798D1D753738E24','ARCHIVED','2020-01-12 11:33:48',0),
('2020_02_17_00_auth.sql','456FA32A7B7C6ABA479F2B099822988A24A2344B','ARCHIVED','2020-02-17 23:28:22',0),
('2020_02_24_00_auth.sql','47E22129056EFD8B67754506231BCC4B0DDA6930','ARCHIVED','2020-02-24 17:19:33',0),
('2020_03_03_00_auth.sql','F473DFEABAC661704ECEA3D72E93A3280122CA30','ARCHIVED','2020-03-03 22:49:13',0),
('2020_03_20_00_auth.sql','0F72572D7792236D8F1E556D9EC62352A0A149CA','ARCHIVED','2020-03-20 10:01:23',0),
('2020_03_23_00_auth.sql','B90D27BB8E3CEDF65881BDA0C7CE6FBC5EF310E0','ARCHIVED','2020-03-23 17:57:46',0),
('2020_03_31_00_auth.sql','BA82A58E95730A397922B6723DA027986E6CD535','ARCHIVED','2020-03-31 17:00:16',0),
('2020_04_04_00_auth.sql','5F118989A9F8AFA3B2065AB9C2C0BB7D9A0EB97A','ARCHIVED','2020-04-04 13:23:53',0),
('2020_04_07_00_auth.sql','6D73A4E1EC5382F10C39F20E2E6E764510A8A5E6','ARCHIVED','2020-04-07 22:23:35',0),
('2020_04_18_00_auth.sql','BD962B50760771B60F2785027D6957EEF2009240','ARCHIVED','2020-04-18 14:09:28',0),
('2020_04_30_00_auth.sql','2FD304B8EF82D529D69287BF22EF061A267F827E','ARCHIVED','2020-04-30 00:39:29',0),
('2020_05_19_00_auth.sql','12D9F26538F63546B74793499E8A71BD885E8E5F','ARCHIVED','2020-05-19 12:00:00',0),
('2020_06_04_00_auth.sql','BA797B558196B1A07F8FF66E5288AD04659CF6AC','ARCHIVED','2020-06-04 09:57:07',0),
('2020_06_17_00_auth.sql','8BAB0BF5C90EBDE68685A9FB772EA90DD214E6D6','ARCHIVED','2020-06-17 09:56:25',0),
('2020_06_17_01_auth.sql','8FBF37B875B5C0E8A609FFB1A2C02F2920A3D3F4','ARCHIVED','2020-06-17 17:04:56',0),
('2020_06_20_00_auth.sql','85345FAF20B91DA7B157AE1E17DF5B6446C2E109','ARCHIVED','2020-06-11 10:48:00',0),
('2020_07_02_00_auth.sql','08D0F9D70AE625285172B3E02A3DAFE17D88E118','ARCHIVED','2020-07-02 10:29:25',0),
('2020_07_03_00_auth.sql','ED7175E51F248ADC5EF60E7CEA9627CC3191ED4C','ARCHIVED','2020-07-03 20:09:39',0),
('2020_07_23_00_auth.sql','5F47E1CEECA9F837C85C2DAC7ECD47AED321F502','ARCHIVED','2020-07-23 19:54:42',0),
('2020_07_24_00_auth.sql','06598162E9C84DDF8AA1F83D0410D056C3F7F69E','ARCHIVED','2020-07-24 00:44:34',0),
('2020_07_25_00_auth.sql','BE376B619ECB6FE827270D5022F311E20AD6E177','ARCHIVED','2020-07-25 00:00:49',0),
('2020_08_02_00_auth.sql','B0290F6558C59262D9DDD8071060A8803DD56930','ARCHIVED','2020-08-02 00:00:00',0),
('2020_08_03_00_auth.sql','492CA77C0FAEEEF3E0492121B3A92689373ECFA3','ARCHIVED','2020-08-03 09:24:47',0),
('2020_08_03_01_auth.sql','EC1063396CA20A2303D83238470D41EF4439EC72','ARCHIVED','2020-08-03 00:00:01',0),
('2020_08_06_00_auth.sql','5D3C5B25132DAFCA3933E9CBE14F5E8A290C4AFA','ARCHIVED','2020-08-06 20:26:11',0),
('2020_08_08_00_auth.sql','BC6A08BE42A6F2C30C9286CBDD47D57B718C4635','ARCHIVED','2020-08-08 00:16:57',0),
('2020_08_11_00_auth.sql','14C99177E43003D83A4D6F2227722F15FC15A1D0','ARCHIVED','2020-08-11 00:00:00',0),
('2020_08_14_00_auth.sql','DFB9B07A7846FC0E124EE4CC099E49FE5742FB66','ARCHIVED','2020-08-14 21:41:24',0),
('2020_08_26_00_auth.sql','D5EF787DECB41D898379588F101A0453B46F04D9','ARCHIVED','2020-08-26 21:00:34',0),
('2020_09_06_00_auth.sql','DC4B5D4C65EB138D5609F137799C3289B9CC2493','ARCHIVED','2020-09-06 00:00:00',0),
('2020_09_17_00_auth.sql','BBC0A8B2BBED38A57A83999909EB066753A893C5','ARCHIVED','2020-09-17 00:00:00',0),
('2020_09_25_00_auth.sql','3CCA78EF89223724BA6784A4F3783DED30416637','ARCHIVED','2020-09-25 19:52:40',0),
('2020_10_20_00_auth.sql','1835C5EFD5816DEF914F27E867C8C8D5E08B3F68','ARCHIVED','2020-10-20 21:36:49',0),
('2020_12_06_00_auth.sql','FA254400D3D7D53E9C350EABFEABFF4EC3AD40DA','ARCHIVED','2020-12-06 20:25:10',0),
('2020_12_07_00_auth.sql','23626805735CB9BEEEBD756D4A39AFBCDA6E366C','ARCHIVED','2020-12-07 21:12:53',0),
('2020_12_15_00_auth.sql','37DA3C4830ABA30C49370A8647F5B6B3E1821E57','ARCHIVED','2020-12-15 19:33:15',0),
('2020_12_22_00_auth.sql','1AED5AD7D93C30CF75E62EBEBCC64FFEDC58F00A','ARCHIVED','2020-12-22 22:00:39',0),
('2020_12_31_00_auth.sql','05C9C105D55C6588CDA0D75AE3B135B7E6B54C06','ARCHIVED','2020-12-31 12:58:21',0),
('2021_01_10_00_auth.sql','4430F5CC9A31DBBEA4E437E980E9F26AC919C016','ARCHIVED','2021-01-10 12:29:57',0),
('2021_01_13_00_auth.sql','F7C15519FFA0FE27EED859343B58714624A302E6','ARCHIVED','2021-01-13 11:19:52',0),
('2021_02_09_00_auth.sql','F00ABFF6E3C3F5ACE3444C9D70BADC764C8B3CE2','ARCHIVED','2021-02-09 17:22:24',0),
('2021_04_10_00_auth.sql','7B92AC4F76507940EF2257897F25304CF0F306EB','ARCHIVED','2021-04-10 19:42:42',0),
('2021_05_12_00_auth.sql','7F37DAD1777D62FDB00C19C0DF5E7DB742CCD5AD','ARCHIVED','2021-05-12 01:08:42',0),
('2021_06_20_00_auth.sql','7CA418D570DC1444C19AAD18F4A50FF187642310','ARCHIVED','2021-06-20 17:29:17',0),
('2021_07_04_00_auth.sql','3CF056F8F04E49C1E236060202AA8DA7E186B590','ARCHIVED','2021-07-04 22:23:24',0),
('2021_08_07_00_auth.sql','D615C2CACC999FF8804AEF56BAAA08D02217D671','ARCHIVED','2021-08-07 23:18:57',0),
('2021_08_18_00_auth.sql','162590897AC0020E68EB6845637901C3EB6509B4','ARCHIVED','2021-08-18 15:14:17',0),
('2021_08_19_00_auth.sql','DE008EDFB1FEBA49567E245A64BFE70DA72D9E7B','ARCHIVED','2021-08-19 10:58:58',0),
('2021_09_02_00_auth.sql','907344F4F0113A13D5E0A1D95E2C3E4C4150090C','ARCHIVED','2021-09-02 12:38:08',0),
('2021_09_10_00_auth.sql','DE94812ABC7B395C6C3405FB6718A8AF2C9F8FEC','ARCHIVED','2021-09-10 12:18:54',0),
('2021_10_07_00_auth.sql','45F2D92E28382F0CBE1F9B3A97693C0CC69E50BC','ARCHIVED','2021-10-07 10:32:05',0),
('2021_10_13_00_auth.sql','220E63385CACCBCEC36C57717DE369F2FCABCAAF','ARCHIVED','2021-10-13 21:15:05',0),
('2021_10_15_00_auth.sql','C8AA212AB2BB2DB5B3C2C9622A3874475AEFBD7B','ARCHIVED','2021-10-15 10:11:47',0),
('2021_10_15_01_auth.sql','72A0437F0ADEC59FF9D6839DF845C473F693CA5B','ARCHIVED','2021-10-16 00:15:25',0),
('2021_10_16_00_auth.sql','FDC45C7BEFBAFC9BCE6C77377B026A59AE52EE21','ARCHIVED','2021-10-16 11:24:39',0),
('2021_10_22_00_auth.sql','07D7397061A5A906357DC6E91FC33C74638EFDC3','ARCHIVED','2021-11-17 13:21:03',0),
('2021_10_23_00_auth.sql','97A8F2C2CEDB99C942D38F5B65DAD1DC11296E20','ARCHIVED','2021-10-23 00:00:00',0),
('2021_10_26_00_auth.sql','91E8B308267847569D9A669BC34794F154242ECF','ARCHIVED','2021-10-26 00:41:04',0),
('2021_11_05_00_auth.sql','4A4510436578B6486E8399602D3060376E96A8C7','ARCHIVED','2021-11-05 00:33:00',0),
('2021_11_06_00_auth.sql','3646F9356429CCE7C1CECC7D9BA7960E011C7B6B','ARCHIVED','2021-11-06 11:54:12',0),
('2021_11_10_00_auth.sql','EB3D26EFD3109BC17447B3BAC7573473F5103F65','ARCHIVED','2021-11-10 14:07:05',0),
('2021_11_12_00_auth.sql','012C088794362FE57BAEA0C3BD05356B40289028','ARCHIVED','2021-11-12 12:17:24',0),
('2021_11_17_00_auth.sql','298DA8468B30042B15FA17A90325C72879DF6D8E','ARCHIVED','2021-11-17 13:23:17',0),
('2021_11_19_00_auth.sql','BE4F77E254D76A59DBF28B2CEEA5CAF6777B650E','ARCHIVED','2021-11-19 00:37:56',0),
('2021_11_20_00_auth.sql','E476B6DAD9C47FC81E1DA5016DC79AB527F1847A','ARCHIVED','2021-11-20 18:40:53',0),
('2021_11_25_00_auth.sql','7A01CEB201CB825BFD565BBF5EED0162BEA733E7','ARCHIVED','2021-11-25 19:32:21',0),
('2021_12_02_00_auth.sql','ED40A45A8F5E5B1BB68216A3053D721B3BA3A556','ARCHIVED','2021-12-02 11:48:11',0),
('2021_12_04_00_auth.sql','00C4A37A60F53A5E893CAADAC882E5A28375A4D2','ARCHIVED','2021-12-04 12:49:04',0),
('2021_12_08_00_auth.sql','9B1A7C86F56159CA50A45B9CB4BC6422A3378231','ARCHIVED','2021-12-08 00:28:19',0),
('2021_12_16_00_auth.sql','EF5050D779CC6CEAAFB4C7E0CFA26824D92B675E','ARCHIVED','2021-12-16 12:21:11',0),
('2021_12_31_00_auth.sql','16AA1CFB93CC42DC9CC7C0C787C64D3CE9662EE5','ARCHIVED','2022-01-02 21:18:52',0),
('2021_12_31_01_auth.sql','336E62A8850A3E78A1D0BD3E81FFD5769184BDF8','ARCHIVED','2021-12-31 15:58:32',0),
('2022_01_02_00_auth.sql','F0AF198C5F7529508A5DB1F29D153256368AD1B4','ARCHIVED','2022-01-02 21:22:35',0),
('2022_01_08_00_auth.sql','3C9853058A77817DD62943D0332418D84CA6BDA1','ARCHIVED','2022-01-15 23:21:37',0),
('2022_01_15_00_auth.sql','11552D29BEDF73626FB8D932AB4362882964B4F0','ARCHIVED','2022-01-15 23:24:57',0),
('2022_01_22_00_auth.sql','24A9BB761E805608EFDD8F647BF733602B337018','ARCHIVED','2022-01-22 01:39:01',0),
('2022_02_25_00_auth.sql','1556FEDB9B46643634AE5BD0E38E7FF447FFC081','ARCHIVED','2022-02-25 12:39:15',0),
('2022_02_26_00_auth.sql','955F8E13B0D91CA06FEEAD4E6C75E5495DA6DDF3','ARCHIVED','2022-02-26 01:01:22',0),
('2022_03_01_00_auth.sql','53636BBB7DF4FFC2496456C1EEF2BD271D1C87E0','ARCHIVED','2022-03-01 16:07:38',0),
('2022_03_02_00_auth.sql','928C11D145B98E90FB82D2A871C2456C848AB6C1','ARCHIVED','2022-03-02 10:10:19',0),
('2022_03_03_00_auth.sql','408CCCF7D47FB5C876E976F883B4BDBFFEC5D146','ARCHIVED','2022-03-03 01:09:55',0),
('2022_03_06_00_auth.sql','2883FD8D2CB8B2FC3DF7D20D3216301262E7A7C3','ARCHIVED','2022-03-06 15:12:24',0),
('2022_03_08_00_auth.sql','E2C6B4E26FE55F5827C587CD668F6518EB2B51E8','ARCHIVED','2022-03-08 15:24:10',0),
('2022_03_12_00_auth.sql','1A476DB06BC1F096E6F15225078373B3AD094C1B','ARCHIVED','2022-03-12 05:52:02',0),
('2022_03_22_00_auth.sql','16A58234A1EF4F13ABD6EF78733BDBF5152AA70C','ARCHIVED','2022-03-22 00:32:54',0),
('2022_03_25_00_auth.sql','173D4F7B5417AF11DDDE6EC1A58ECFA6783C7FAF','ARCHIVED','2022-03-25 00:26:53',0),
('2022_03_30_00_auth.sql','37177AB41D7DF26CF4F908C0522EDEAF13094D7E','ARCHIVED','2022-03-30 00:37:34',0),
('2022_03_30_01_auth.sql','AACD3E4E8673F6D90677C97D5B0B0F292D0C1763','ARCHIVED','2022-03-30 04:28:42',0),
('2022_04_08_00_auth.sql','C0BE7634D6B84D860111AF5EEDF1D023875F3137','ARCHIVED','2022-04-08 21:01:16',0),
('2022_04_14_00_auth.sql','4D79D1C7282CA8F1626D957AF17E711BFF94334B','ARCHIVED','2022-04-14 04:39:49',0),
('2022_04_22_00_auth.sql','835EE2D6981AD7A7467490242D6A4B0E0B69E4F4','ARCHIVED','2022-04-22 20:04:53',0),
('2022_05_03_00_auth.sql','0874FBE9821F2659BA51B91E9D69B9E6CA6D2EC9','ARCHIVED','2022-05-03 11:07:21',0),
('2022_06_01_00_auth.sql','DCFC7EC6C52993769B568EAF87CA2DAA10359AEB','ARCHIVED','2022-06-02 00:52:17',0),
('2022_06_06_00_auth.sql','68D73F068598D37FD6FBC84362F1BA7BA4EC2709','ARCHIVED','2022-06-06 21:35:16',0),
('2022_06_07_00_auth.sql','76B4D21F13B0024445E5C0B48C630C1DF7E80966','ARCHIVED','2022-06-07 16:09:58',0),
('2022_06_08_00_auth.sql','250081465C76AC9668E3F66D386CE2AAC05379E9','ARCHIVED','2022-06-08 10:45:01',0),
('2022_06_09_00_auth.sql','29C2A4209FB977373440666F00B2E04F0E095247','ARCHIVED','2022-06-09 18:31:38',0),
('2022_06_15_00_auth.sql','137223C2750CC3559EFE11AFF1A780D5DA070193','ARCHIVED','2022-06-15 11:16:51',0),
('2022_06_18_00_auth.sql','63B75F9D79D83581AB3257C9EF86EDB626D8FDDA','ARCHIVED','2022-06-18 11:48:42',0),
('2022_06_27_00_auth.sql','CF613CCAAF8B6F08AAE1C48DBC4AF4D224291D8D','ARCHIVED','2022-06-27 21:13:58',0),
('2022_07_23_00_auth.sql','FEA7A8DA363F097A090F3BB39401C3FD7AE8E9B5','ARCHIVED','2022-07-25 18:40:38',0),
('2022_07_25_00_auth.sql','8F5BA8F7E010EDCA70F49FDE947B2F89476A2F95','ARCHIVED','2022-07-25 18:44:10',0),
('2022_08_02_00_auth.sql','4B97D20928B05086C7863497F4DDD408A51619BE','ARCHIVED','2022-08-02 18:28:31',0),
('2022_08_17_00_auth.sql','1A45DAE660690A7F4D0822C514116BF44A3185BB','ARCHIVED','2022-08-17 10:16:00',0),
('2022_08_19_00_auth.sql','332E7CC2E69D69BF274E5C61768FB80D1C217BDB','ARCHIVED','2022-08-19 09:52:06',0),
('2022_08_19_01_auth.sql','8B32826AE09C27B98C9480EF4D61205666F68318','ARCHIVED','2022-08-19 23:43:01',0),
('2022_08_21_00_auth.sql','5DEC0CB848F99D575B90356D82276749F2473B72','ARCHIVED','2022-08-21 00:02:03',0),
('2022_09_02_00_auth.sql','E2ED8B4B90829CFD283C9679AE265A9C9B2CF762','ARCHIVED','2022-09-02 15:52:22',0),
('2022_09_08_00_auth.sql','20B4503E316E042432AC25F7CEE9DCD9EDC631C6','ARCHIVED','2022-09-08 15:38:35',0),
('2022_09_23_00_auth.sql','F7DB1B903982D99295BC0D7D2BC205C5A2145F03','ARCHIVED','2022-09-23 03:44:55',0),
('2022_10_03_00_auth.sql','B956A37F71B42EB0289C2066A15D1F6C02F21E5A','ARCHIVED','2022-10-03 21:32:38',0),
('2022_11_20_00_auth.sql','37123D83589CFD96472D9187799C1F3FD67645DD','ARCHIVED','2022-11-20 11:05:20',0),
('2022_12_16_00_auth.sql','249B00480ACC8B67C908435748C202D8363C6EDE','ARCHIVED','2022-12-16 22:39:07',0),
('2022_12_17_00_auth.sql','0D3963AC2DBF74A4C8B88EA4A680C046FCCF8E70','ARCHIVED','2022-12-17 07:34:53',0),
('2022_12_17_01_auth.sql','5D1E2EA3C3CE087F7FB647CD0DE000979961863C','ARCHIVED','2022-12-17 13:09:19',0),
('2022_12_20_00_auth.sql','BA88146743B060A14937688C0DB94BF11C6CF1BA','ARCHIVED','2022-12-20 03:10:07',0),
('2022_12_21_00_auth.sql','B395CAE993D65E035AA621941D4C384E2E2E7DF5','ARCHIVED','2022-12-21 01:16:56',0),
('2022_12_22_00_auth.sql','BBF3CDD7927520F0381ECEF1F30152CBD5344D6A','ARCHIVED','2022-12-22 16:35:36',0),
('2023_01_17_00_auth.sql','EAEA99DF10DCC648C161D836FFA681D5B0F4CDC3','ARCHIVED','2023-01-17 18:41:32',0),
('2023_01_28_00_auth.sql','94A640018494B9203100178EC67A582987456B8B','ARCHIVED','2023-01-28 00:11:59',0),
('2023_01_28_01_auth.sql','4BDA614300858ADE6D58A119680724D867B0A355','ARCHIVED','2023-01-28 16:39:41',0),
('2023_02_01_00_auth.sql','9C1FB6820EF3A543AB7DE2E2913014AFF445F91E','ARCHIVED','2023-02-01 10:10:30',0),
('2023_02_02_00_auth.sql','8AC163759C83D887D5D7A48CFB8272FFBA71B801','ARCHIVED','2023-02-02 10:44:28',0),
('2023_02_03_00_auth.sql','0309A99757DE1FAE595C6C32586B2B8F8CA4C134','ARCHIVED','2023-02-03 01:13:52',0),
('2023_02_07_00_auth.sql','92D6C7A47B2F98ED93D62F6B7293D604C8F6BE16','ARCHIVED','2023-02-07 10:39:13',0),
('2023_02_09_00_auth.sql','F4B797776CA62D9A048D231C8117D51AA312932C','ARCHIVED','2023-02-09 00:19:27',0),
('2023_02_14_00_auth.sql','033F4460715121A8B6E9DD0F7456AE930DD18A7A','ARCHIVED','2023-02-14 10:02:49',0),
('2023_02_28_00_auth.sql','F57F70D6E1BBB1CB799E338C3358C265FD7F8689','ARCHIVED','2023-02-28 21:32:14',0),
('2023_03_08_00_auth.sql','2490CEA2EBDB0ECD3590F1D9328DD88266E179F3','ARCHIVED','2023-03-08 21:58:27',0),
('2023_03_11_00_auth.sql','6532C5E043692E7361689090DA19E6350705B591','ARCHIVED','2023-03-11 00:10:17',0),
('2023_03_22_00_auth.sql','C589D68CF88A62E03F2E797E03CF2F237371BD34','ARCHIVED','2023-03-21 18:12:28',0),
('2023_03_27_00_auth.sql','25B04268224275D0A90EF13E62460CBF61B90CEF','ARCHIVED','2023-03-27 21:22:59',0),
('2023_03_31_00_auth.sql','3F8CB31A261BCFE5C9A08B12945221CAA652AB24','ARCHIVED','2023-03-31 11:15:43',0),
('2023_03_31_01_auth.sql','A70E14B46537BC9208663B94EDF6CE51CB1B23BA','ARCHIVED','2023-03-31 23:16:09',0),
('2023_04_02_00_auth.sql','0238E0CE22D6422B19F648D026349A018CD4DB04','ARCHIVED','2023-04-02 01:02:26',0),
('2023_04_05_00_auth.sql','67317FC9DAA66EBF68468E60F99E1F6DD5B237E8','ARCHIVED','2023-04-05 10:39:51',0),
('2023_04_06_00_auth.sql','FECA06F32D077B1660D9FF8204D94F5C8E4065B4','ARCHIVED','2023-04-06 04:18:58',0),
('2023_04_12_00_auth.sql','E28F892FCC2A923683E5EEE24E98C618A9534318','ARCHIVED','2023-04-12 00:26:10',0),
('2023_04_25_00_auth.sql','AF297F37715F4A4C84E84182358C26CA83B0C655','ARCHIVED','2023-04-25 11:14:36',0),
('2023_04_28_00_auth.sql','779248686CB60F21CA7E9514E33B2D3E37C91B9E','ARCHIVED','2023-07-14 08:19:57',0),
('2023_05_04_00_auth.sql','1015EC7619C1F43B9FD70C8971F883D0CBF4D002','ARCHIVED','2023-05-04 16:02:32',0),
('2023_05_05_00_auth.sql','C0F435B417D238619DC390F52B27BA0E08DDE2CF','ARCHIVED','2023-05-05 00:55:38',0),
('2023_05_09_00_auth.sql','E14DC7567533284034ADCD74ED99486A4AD331AE','ARCHIVED','2023-05-09 01:07:29',0),
('2023_05_15_00_auth.sql','B2A9E5D5ECDC04C44136B4BAC7350AAF1522E916','ARCHIVED','2023-05-15 00:36:20',0),
('2023_05_23_00_auth.sql','C58C31ABA0AF08508B1946143746C44FB6ACB824','ARCHIVED','2023-05-23 09:23:42',0),
('2023_05_25_00_auth.sql','52C460A556EE08EF149E55E021AF0A9B5EC9AE13','ARCHIVED','2023-05-25 00:34:18',0),
('2023_05_27_00_auth.sql','C7B7718915274FC21BAE243D42D7419F67F93792','ARCHIVED','2023-05-27 12:10:20',0),
('2023_06_06_00_auth.sql','F327269C75DF6D09B3D7B33137681E0C1188120A','ARCHIVED','2023-06-06 00:50:17',0),
('2023_06_12_00_auth.sql','5BEE858205C3EDE75C5A5A1E46FBEE2257F97B83','ARCHIVED','2023-06-12 23:47:51',0),
('2023_07_12_00_auth.sql','E610FC5F0B1079070F69B5AAA6D6BDA5630B081F','ARCHIVED','2023-07-12 11:21:50',0),
('2023_07_13_00_auth.sql','BF718B6F8F2A324092D95BC1370120F0EE699BD2','ARCHIVED','2023-07-13 19:53:43',0),
('2023_07_14_00_auth.sql','B66CDB7EE7E554992BB2A9DAB3C16122411C81DF','ARCHIVED','2023-07-14 08:24:44',0),
('2023_07_14_01_auth.sql','8037C5101A08824DA6FE6B16E0004704DFC5B8FA','ARCHIVED','2023-07-14 19:35:41',0),
('2023_07_15_00_auth.sql','89D51F3E0EAAA957A7C0E4A4A7812505F61F12E6','ARCHIVED','2023-07-15 00:15:31',0),
('2023_07_19_00_auth.sql','8FE0EAB6C8DA5B060E95A9715F3D374340E361DD','ARCHIVED','2023-07-19 19:31:35',0),
('2023_07_22_00_auth.sql','38BDC8DE5697366E26588552830E34E420861008','ARCHIVED','2023-07-22 13:57:13',0),
('2023_07_28_00_auth.sql','1CC6C4E639ED9FD2EABFD0713C4D809C707E5E3F','ARCHIVED','2023-07-28 20:03:06',0),
('2023_08_03_00_auth.sql','57B92FF9D84AFE5F37A533F8F7187E26A708D8EE','ARCHIVED','2023-08-03 22:49:11',0),
('2023_08_09_00_auth.sql','3A0B9E91EB66D237785CD3F3CDFE5A6EAB33578E','ARCHIVED','2023-08-09 10:01:04',0),
('2023_08_31_00_auth.sql','3A2242F0755CCC7658F458847B12E308FE75A314','ARCHIVED','2023-08-31 19:06:27',0),
('2023_09_07_00_auth.sql','9127F7B6723477DE25886D451FE174ABF2039B94','ARCHIVED','2023-09-07 00:30:01',0),
('2023_09_08_00_auth.sql','AA4E52CC2344F503151C88284807E8B7319B7C69','ARCHIVED','2023-09-08 21:46:01',0),
('2023_09_13_00_auth.sql','49C44AE960C71C71DC9966D10D8DAA127976D22B','ARCHIVED','2023-09-13 00:30:28',0),
('2023_09_20_00_auth.sql','6B3EAEB21A617564907EE843FC43272F0C020760','ARCHIVED','2023-09-20 04:49:35',0),
('2023_09_23_00_auth.sql','B6250AE892CF5988FD4EB08EC35DE25096B52115','ARCHIVED','2023-09-23 01:57:24',0),
('2023_09_28_00_auth.sql','E69955264CD347921DDD1B52BC31E8C39EC41B21','ARCHIVED','2023-09-28 05:36:07',0),
('2023_10_06_00_auth.sql','3480687DEEB3E12ECC9632809A518425F9FA0FCC','ARCHIVED','2023-10-06 00:40:46',0),
('2023_10_17_00_auth.sql','EE4C430E9535EC3466E4D2FABA7F009F87AF18BD','ARCHIVED','2023-10-17 21:07:51',0),
('2023_10_25_00_auth.sql','86B16D5D78A8ED31FDD8553D223CF56F013B00DB','ARCHIVED','2023-10-25 00:06:14',0),
('2023_11_01_00_auth.sql','4EA6010E9035AFC80326FE56C642C7918254F2BE','ARCHIVED','2023-11-01 10:47:18',0),
('2023_11_09_00_auth.sql','C8A9223E6868593904634193ACBD421F40078FE5','ARCHIVED','2023-11-09 00:53:45',0),
('2023_11_09_01_auth.sql','BC9BC28D41608A78166B5A38F3A7F598FBDB879D','ARCHIVED','2023-11-09 18:21:59',0),
('2023_11_14_00_auth.sql','192D729737C5E3332D7B5B9B7F9DBDD9626D7B98','ARCHIVED','2023-11-14 11:36:05',0),
('2023_11_15_00_auth.sql','85CE6DCBE9391F0FB3819C7579067E2775D7C20E','ARCHIVED','2023-11-15 00:48:07',0),
('2023_11_15_01_auth.sql','622218EB74372055943D7B62AD30B52F959CC94B','ARCHIVED','2023-11-15 00:53:47',0),
('2023_11_16_00_auth.sql','2EF3FE83B74EFC10B8536E2EB6AFAE7074FC59BD','ARCHIVED','2023-11-16 23:19:09',0),
('2023_11_21_00_auth.sql','146E5E6EF94C5DB78343372A8FDB32B062B80040','ARCHIVED','2023-11-21 11:24:11',0),
('2023_11_24_00_auth.sql','AC1B5136CC97264A21933BD1074D02E88D819488','ARCHIVED','2023-11-24 19:37:38',0),
('2023_11_30_00_auth.sql','49E92311D7373965320CCEE3720D4EF9F1A28F97','ARCHIVED','2023-11-30 13:39:17',0),
('2023_12_05_00_auth.sql','25DC15C708E6A962DC322293D0CE5D52941F560A','ARCHIVED','2023-12-05 16:18:14',0),
('2023_12_12_00_auth.sql','EDC092787956178A08D15B9245EE4716ED0847B0','ARCHIVED','2023-12-12 05:47:47',0),
('2023_12_13_00_auth.sql','C3D6AA45BECD5A7F8A420FE0022AAF6A349C5E3F','ARCHIVED','2023-12-13 06:42:48',0),
('2023_12_19_00_auth.sql','EDC1C8E58EEF18F952A3509D40D27E17CAC49563','ARCHIVED','2024-02-08 00:48:00',0),
('2023_12_21_00_auth.sql','DB294EF35C00AA92C79786F7A0BFBCE739D4E193','ARCHIVED','2023-12-21 09:08:30',0),
('2023_12_24_00_auth.sql','F59B3A895750FD83177324B89BFCEBD8A43DD577','ARCHIVED','2023-12-24 06:24:58',0),
('2023_12_26_00_auth.sql','5C8716F7F6E2792E15A42BDA8F2D855A7DE95FC5','ARCHIVED','2023-12-26 13:38:58',0),
('2024_01_05_00_auth.sql','7F401D473B08BBE5212551E96A86F85107CE7C8E','ARCHIVED','2023-12-19 10:05:39',0),
('2024_01_10_00_auth.sql','75F06894D95986AEAB2933F141DB7693FABB0324','ARCHIVED','2024-01-10 11:14:55',0),
('2024_01_21_00_auth.sql','5F12B88EAADC5390AD42843290BD13CEF3BF2E0B','ARCHIVED','2024-01-21 21:21:50',0),
('2024_01_23_00_auth.sql','55E3C2CC1FAF02916EB47711CD2278443F7AA183','ARCHIVED','2024-01-23 09:40:35',0),
('2024_01_31_00_auth.sql','7328692BBD9B455D51ABE90B9C6571EA889EE26F','ARCHIVED','2024-01-31 11:58:00',0),
('2024_02_01_00_auth.sql','1822B9B3E28AB0E77CF253C6A96F78EB5020447A','ARCHIVED','2024-02-01 10:46:27',0),
('2024_02_07_00_auth.sql','F6A65673A7020B4C64083AFC1CAD4206BC960170','ARCHIVED','2024-02-07 12:26:24',0),
('2024_02_08_00_auth.sql','E62D51C3D536FDF79EBEDFE060EB9D232ACE1EDD','ARCHIVED','2024-02-08 00:56:26',0),
('2024_02_08_01_auth.sql','9972EE1527DFC182527FA3C1AC086B189574BDF0','ARCHIVED','2024-02-08 22:32:41',0),
('2024_02_24_00_auth.sql','BEB6F94703C4574088289DFFC8E7660D223E2841','ARCHIVED','2024-02-24 00:27:13',0),
('2024_03_05_00_auth.sql','61341FABF7243FBCBB6BDE35CECE1F7DFD988F26','ARCHIVED','2024-03-05 20:43:51',0),
('2024_03_21_00_auth.sql','5D4F3479B0F4E16E59B1CC9EB21E5CE5531555B9','ARCHIVED','2024-03-21 16:56:21',0),
('2024_03_22_00_auth.sql','4F184E4BF361195C560A569E57A6FF6429077849','ARCHIVED','2024-03-22 20:29:29',0),
('2024_03_25_00_auth.sql','B4EF57DE84195E0B886B070ACE58B66853FFC52C','ARCHIVED','2024-03-25 23:20:57',0),
('2024_03_29_00_auth.sql','F1F62C913F5DFB277DB3D1258022EC334EFC419B','ARCHIVED','2024-03-29 10:49:54',0),
('2024_04_08_00_auth.sql','B05516DB306A06982206180A2A18407D9E0012DE','ARCHIVED','2024-04-08 21:56:33',0),
('2024_04_12_00_auth.sql','3D2DCEA21AE5104EF192CACAAFBB5131AD355791','ARCHIVED','2024-04-12 12:07:49',0),
('2024_04_22_00_auth.sql','83566EA54235D588697D747D8130CD6D37445CC5','ARCHIVED','2024-04-22 23:05:46',0),
('2024_05_01_00_auth.sql','2A1C2B4E54706BDD1FDD57AFB943058BEE8D8852','ARCHIVED','2024-05-01 21:32:43',0),
('2024_05_08_00_auth.sql','F2EEE0D225CB82DBB3A478E0BF6A0116C9265355','ARCHIVED','2024-05-08 20:35:53',0),
('2024_05_10_00_auth.sql','8A6E48012413EA769601A8E4C9F82BEFBE1FCA5C','ARCHIVED','2024-05-10 00:50:46',0),
('2024_05_11_00_auth.sql','8D703BE97247DBC4A4935EAEA9CB681B0EEFBB3C','ARCHIVED','2024-05-11 03:06:52',0),
('2024_05_14_00_auth.sql','C36612C0215E255E1D4917876FD52C8F1F9BCDA7','ARCHIVED','2024-05-14 03:39:01',0),
('2024_05_16_00_auth.sql','5980F77073B6AD55ABCF0F9735408A4F35C97159','ARCHIVED','2024-05-16 08:13:11',0),
('2024_05_17_00_auth.sql','E02B96055C887BED76442B2C3C49C51659BCC602','ARCHIVED','2024-05-17 14:24:15',0),
('2024_05_18_00_auth.sql','C3392BDA5AAC304AD72BAC631DA1C9802D20EDAC','ARCHIVED','2024-05-18 01:52:01',0),
('2024_05_25_00_auth.sql','A34E299F5D75535D17C6295023910539897D1392','ARCHIVED','2024-05-25 12:10:08',0),
('2024_05_29_00_auth.sql','6F42C6565CB748E3C1231F4EA0A6D04F2F95C0A3','ARCHIVED','2024-05-29 19:12:53',0),
('2024_06_05_00_auth.sql','B6306773C94879872E0B3FDAFE12ECDEA4C9264F','ARCHIVED','2024-06-05 09:37:55',0),
('2024_06_15_00_auth.sql','FDF2F29F5AB0213C9C2AB937852F0BAE552CFDCE','ARCHIVED','2024-06-15 00:01:40',0),
('2024_06_17_00_auth.sql','3781820B8F3B238F89BC8E83EC164B767A63BBB3','ARCHIVED','2024-06-17 22:34:47',0),
('2024_06_26_00_auth.sql','428762D840B37F2569FC477E0B5B452C7E98B301','ARCHIVED','2024-06-26 13:59:17',0),
('2024_07_09_00_auth.sql','C58A8D47CAFA34055EEB079E72AA820EF7C55368','ARCHIVED','2024-07-09 12:00:23',0),
('2024_07_16_00_auth.sql','4161C07EACA71CE3D0269D2067563594D26DEC27','ARCHIVED','2024-07-16 19:02:24',0),
('2024_08_02_00_auth.sql','6BE2A6DA2F2BE21B18396F120427E973F1CAA58F','ARCHIVED','2024-08-02 00:45:10',0),
('2024_08_07_00_auth.sql','88070C3A95DE7242B3E3662D7B5B9648128DC187','ARCHIVED','2024-08-07 01:06:23',0),
('2024_08_09_00_auth.sql','854C24AAF0B3F673DAD8C0F3059DB266B0F34488','ARCHIVED','2024-08-09 15:40:42',0),
('2024_08_15_00_auth.sql','06E9AD907DC2B2CC0CF78414AB516BBA3ED0CE65','ARCHIVED','2024-08-15 19:40:50',0),
('2024_08_17_00_auth.sql','3584B5A910334447D6E948DBAA0EEC43C42FE631','ARCHIVED','2024-08-17 15:17:22',0),
('2024_08_18_00_auth.sql','5C1D0A3FE0245F4030FE446288AE533556EC6C9E','ARCHIVED','2024-08-17 23:01:21',0),
('2024_08_21_00_auth.sql','6F2844107F0501E7631C8196CC04E75853381319','ARCHIVED','2024-08-21 15:25:46',0),
('2024_08_22_00_auth.sql','D5466F5E9D1475323ED90553361D7F7B4CF83BF7','ARCHIVED','2024-08-22 23:35:36',0),
('2024_08_23_00_auth.sql','A591DA576EAEA3F48AB9E8269A2E4071B2C3C930','ARCHIVED','2024-08-23 17:35:52',0),
('2024_08_23_01_auth.sql','1ABD54E76B2D6712BF3DE06DD60A18C38BCF9CF1','ARCHIVED','2024-08-23 19:59:36',0),
('2024_08_24_00_auth.sql','6D2CE419F8E8512051B63D0F68B30BAA769F9D4A','ARCHIVED','2024-08-24 10:23:33',0),
('2024_08_28_00_auth.sql','F39CA1C18CAC207418C3D3703DA0F038471D308A','ARCHIVED','2024-08-28 10:50:31',0),
('2024_08_28_01_auth.sql','BC5D74553AF2D92606F55C1C462D2700FE73BD34','ARCHIVED','2024-08-28 14:55:05',0),
('2024_08_30_00_auth.sql','BD76942F1C29AAA2450E051E7CA552672B5E331B','ARCHIVED','2024-08-30 19:24:30',0),
('2024_08_30_01_auth.sql','0AAC95A9216114177AF0A3A7DE664D061FED21EA','ARCHIVED','2024-08-30 22:23:08',0),
('2024_08_31_00_auth.sql','D10F12D25D526A2C5E12FCEA2DEB55C342879F06','ARCHIVED','2024-08-31 10:32:16',0),
('2024_08_31_01_auth.sql','3ADC34DAE405697983EDDD32D0F3FC6F122CA819','ARCHIVED','2024-09-01 15:48:43',0),
('2024_09_03_00_auth.sql','BF65B550256CA64F0936D98A619E8A2A7E7A9B59','ARCHIVED','2024-09-03 00:47:42',0),
('2024_09_05_00_auth.sql','9CD84EB94AE50F14A392C216E7CA7BA33E92376E','ARCHIVED','2024-09-05 13:38:34',0),
('2024_09_05_01_auth.sql','42D5E703382C57C4ADD862CF215F1166625100E5','ARCHIVED','2024-09-05 23:42:10',0),
('2024_09_11_00_auth.sql','8B810B5EB20D769A50A2D01411BBD065732E4EE6','ARCHIVED','2024-09-11 12:23:18',0),
('2024_09_18_00_auth.sql','073B6E954B585CD81BFADD58CDAD166E85D2653A','ARCHIVED','2024-09-18 10:26:42',0),
('2024_09_18_01_auth.sql','6AE1437E24D4837EB2347ADAA80A4093CA9F6D67','ARCHIVED','2024-09-18 21:30:31',0),
('2024_09_23_00_auth.sql','CBAB00B40360D8942AD1E4EDBE0F0097F3F6FC9B','ARCHIVED','2024-09-23 22:48:10',0),
('2024_09_26_00_auth.sql','E37C3997FD7851EA360774AC568912846C448272','ARCHIVED','2024-09-26 18:27:26',0),
('2024_10_01_00_auth.sql','665EC51B03F52C373F713D96A1426054D7DF98C8','ARCHIVED','2024-10-01 23:39:44',0),
('2024_10_23_00_auth.sql','78203AE9051E866F103CC7B8A6EE09D5C25BCB9A','ARCHIVED','2024-10-23 15:48:13',0),
('2024_10_23_01_auth.sql','06257BB5FF23564549529C14822DD5780AA7F7C3','ARCHIVED','2024-10-23 22:30:54',0),
('2024_10_30_00_auth.sql','B4C930B0CE499704CEBF208A71871061DC27008C','ARCHIVED','2024-10-30 13:04:40',0),
('2024_11_01_00_auth.sql','62EF7FC9CD2DCD8D24CE1BCED3BF9E2917B6674B','ARCHIVED','2024-11-01 13:26:56',0),
('2024_11_11_00_auth.sql','F47CDFB857DB4105306739AF4FBBB3C92CA43363','ARCHIVED','2024-11-11 13:34:09',0),
('2024_11_12_00_auth.sql','5A236A557291758C0F2C46FDEC02692E7C53F751','ARCHIVED','2024-11-12 11:42:13',0),
('2024_11_14_00_auth.sql','646A438FAD5A83D80DCDDEDC71BB6CF21A1FE490','ARCHIVED','2024-11-14 10:29:52',0),
('2024_11_16_00_auth.sql','94794542B3320D76E4A3615DB2418EEF38BDFEA1','ARCHIVED','2024-11-16 21:57:39',0),
('2024_11_20_00_auth.sql','444C765DD16E9B70C1D1163155238E8EE481B17D','ARCHIVED','2024-11-20 20:39:53',0),
('2024_12_18_00_auth.sql','1F3145F30A79FEE13F5CD9817F54FAFA9A85DEE6','ARCHIVED','2024-12-18 22:04:44',0),
('2024_12_19_00_auth.sql','B01D54F8420CF680F0DDA903210028F0174E310D','ARCHIVED','2024-12-19 11:32:53',0),
('2024_12_21_00_auth.sql','9DA5E7911D231D46FE9630D20BDD4AEA0B3CF650','ARCHIVED','2024-12-21 01:03:37',0),
('2024_12_22_00_auth.sql','6FDF0ABE9952CC96BE04A6BAA0F1161900B4AB88','ARCHIVED','2024-12-22 02:56:17',0),
('2025_01_11_00_auth.sql','B32F963095657E8877F3C1EEBB09C8AED55EEECC','ARCHIVED','2025-01-11 10:51:47',0),
('2025_01_15_00_auth.sql','8EA1C0A88F6F1EC3DA67DA01CDCF056A530DBB5F','ARCHIVED','2025-01-15 00:44:53',0),
('2025_01_15_01_auth.sql','885A553B190CCD7A22791AF4EC3BC15C43DDC2D2','ARCHIVED','2025-01-15 10:27:39',0),
('2025_01_22_00_auth.sql','E08C40B02DB391DC1A478B6C714FDDDDB144F24F','ARCHIVED','2025-01-22 20:58:45',0),
('2025_01_29_00_auth.sql','7334CFE779D5F79AFAC8EA269950FC07E625810F','ARCHIVED','2025-01-29 12:57:32',0),
('2025_01_31_00_auth.sql','DF2640D25C15E41358C1ED46B4F86043583FC8FB','ARCHIVED','2025-01-31 11:51:23',0),
('2025_02_03_00_auth.sql','87033116EF72051499AF23FE14545D828F5A9B59','ARCHIVED','2025-02-03 22:51:31',0),
('2025_02_19_00_auth.sql','6D0EBB3E2DD1B26682A424EDDC090F515BCA5B32','ARCHIVED','2025-02-19 00:16:52',0),
('2025_02_21_00_auth.sql','E9EA49CEB396561D641A7C82251DCBF9DF3E9983','ARCHIVED','2025-02-21 00:53:17',0),
('2025_03_02_00_auth.sql','1CEAB7B308823D5FAA51360A1B2A7CA0E89A6081','ARCHIVED','2025-03-02 14:50:10',0),
('2025_03_04_00_auth.sql','AB20016BC7251E6DE80E2AC658F18F6076EA81AA','ARCHIVED','2025-03-04 20:01:43',0),
('2025_03_05_00_auth.sql','25DA7985BDEA177E1AE7C8DC82063D1C11F7BF92','ARCHIVED','2025-03-05 11:20:32',0),
('2025_03_08_00_auth.sql','DA9CEE1C87F8A1CF7FFDF684FD7DDFCC7B04D5AE','ARCHIVED','2025-03-08 01:37:14',0),
('2025_03_13_00_auth.sql','942D432EBC68783F76FF3A3CBE6F1FAF32121D18','ARCHIVED','2025-03-13 19:58:25',0),
('2025_03_26_00_auth.sql','AACB673BEED68EADC53D121E107E1C780DFE0B88','ARCHIVED','2025-03-26 10:32:59',0),
('2025_03_29_00_auth.sql','AB55F260FB970455392E1FD4D760965EE48F7074','ARCHIVED','2025-03-29 01:12:13',0),
('2025_03_31_00_auth.sql','0522AF5AE6CE2B2EB14265CC005CC209DB4834C0','ARCHIVED','2025-03-31 21:19:14',0),
('2025_04_09_00_auth.sql','7543CD6152ABDA64B5FC75FC28211A54F16B9E40','ARCHIVED','2025-04-09 10:06:12',0),
('2025_04_10_00_auth.sql','93EBDB49570AFD980449CE6858AFA588ED763FAB','ARCHIVED','2025-04-10 20:45:44',0),
('2025_04_12_00_auth.sql','A33D1A1E50EA4B6E93DD6BAB5AD624EDAEC39BDF','ARCHIVED','2025-04-12 00:54:47',0),
('2025_04_25_00_auth.sql','CD64B1560DE7A3715B561FC4E712C5E61C126A1D','ARCHIVED','2025-04-25 19:43:41',0),
('2025_04_30_00_auth.sql','B8708581D23575920082C18665E828C629F07DED','ARCHIVED','2025-04-30 13:15:42',0),
('2025_05_14_00_auth.sql','E4C40DFB18415ADB60223BA7518A2F0FCD491712','ARCHIVED','2025-05-14 10:24:56',0),
('2025_05_30_00_auth.sql','37191DD7967326918A3BE9EB919CEECBA42098A2','ARCHIVED','2025-05-30 11:42:16',0),
('2025_05_31_00_auth.sql','5EF8B1EBE4CF99E5D1C02BD6141FB35EA19FED6C','ARCHIVED','2025-05-31 19:45:56',0),
('2025_06_03_00_auth.sql','2ADB0D37ED7223C2EA94497B0F7B7C0BF91A72B8','ARCHIVED','2025-06-03 01:11:51',0),
('2025_06_05_00_auth.sql','E40328A6899D04BCDB6A36337BD243837AF92205','ARCHIVED','2025-06-05 00:26:43',0),
('2025_06_05_02_auth.sql','C2B67F688AC54CF6994F4709D0ECE692C968F346','ARCHIVED','2025-06-05 16:22:53',0),
('2025_06_18_00_auth.sql','AB5F6069BD37C93050022700F1C4B814D9D139C1','ARCHIVED','2025-06-17 23:13:05',0),
('2025_06_19_00_auth.sql','1C0ACAEEFC934F91F44C113E6CD9D7A40ED1C979','ARCHIVED','2025-06-18 22:51:15',0),
('2025_06_25_00_auth.sql','27DC7FB423FFB3788082CCFC18D5432650B09FB3','ARCHIVED','2025-06-25 01:15:04',0),
('2025_06_27_00_auth.sql','243C89DFED0058323EF9690D124C1F20036D461B','ARCHIVED','2025-06-27 14:22:49',0),
('2025_07_14_00_auth.sql','5F975A202CF84F2BFEA366E0BEDF8FA63035CD62','ARCHIVED','2025-07-14 23:10:54',0),
('2025_07_21_00_auth.sql','44AA781EE1B1C66E8433E50A2E20916EB1BEFE23','ARCHIVED','2025-07-21 22:51:05',0);
/*!40000 ALTER TABLE `updates` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `updates_include`
--

DROP TABLE IF EXISTS `updates_include`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `updates_include` (
  `path` varchar(200) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT 'directory to include. $ means relative to the source directory.',
  `state` enum('RELEASED','ARCHIVED') CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'RELEASED' COMMENT 'defines if the directory contains released or archived updates.',
  PRIMARY KEY (`path`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='List of directories where we want to include sql updates.';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `updates_include`
--

LOCK TABLES `updates_include` WRITE;
/*!40000 ALTER TABLE `updates_include` DISABLE KEYS */;
INSERT INTO `updates_include` VALUES
('$/sql/custom/auth','RELEASED'),
('$/sql/old/10.x/auth','ARCHIVED'),
('$/sql/old/11.x/auth','ARCHIVED'),
('$/sql/old/6.x/auth','ARCHIVED'),
('$/sql/old/7/auth','ARCHIVED'),
('$/sql/old/8.x/auth','ARCHIVED'),
('$/sql/old/9.x/auth','ARCHIVED'),
('$/sql/updates/auth','RELEASED');
/*!40000 ALTER TABLE `updates_include` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `uptime`
--

DROP TABLE IF EXISTS `uptime`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `uptime` (
  `realmid` int unsigned NOT NULL,
  `starttime` int unsigned NOT NULL DEFAULT '0',
  `uptime` int unsigned NOT NULL DEFAULT '0',
  `maxplayers` smallint unsigned NOT NULL DEFAULT '0',
  `revision` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'Trinitycore',
  PRIMARY KEY (`realmid`,`starttime`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Uptime system';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `uptime`
--

LOCK TABLES `uptime` WRITE;
/*!40000 ALTER TABLE `uptime` DISABLE KEYS */;
/*!40000 ALTER TABLE `uptime` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Temporary view structure for view `vw_log_history`
--

DROP TABLE IF EXISTS `vw_log_history`;
/*!50001 DROP VIEW IF EXISTS `vw_log_history`*/;
SET @saved_cs_client     = @@character_set_client;
/*!50503 SET character_set_client = utf8mb4 */;
/*!50001 CREATE VIEW `vw_log_history` AS SELECT 
 1 AS `First Logged`,
 1 AS `Last Logged`,
 1 AS `Occurrences`,
 1 AS `Realm`,
 1 AS `type`,
 1 AS `level`,
 1 AS `string`*/;
SET character_set_client = @saved_cs_client;

--
-- Temporary view structure for view `vw_rbac`
--

DROP TABLE IF EXISTS `vw_rbac`;
/*!50001 DROP VIEW IF EXISTS `vw_rbac`*/;
SET @saved_cs_client     = @@character_set_client;
/*!50503 SET character_set_client = utf8mb4 */;
/*!50001 CREATE VIEW `vw_rbac` AS SELECT 
 1 AS `Permission ID`,
 1 AS `Permission Group`,
 1 AS `Security Level`,
 1 AS `Permission`*/;
SET character_set_client = @saved_cs_client;

--
-- Dumping routines for database 'auth'
--

--
-- Final view structure for view `vw_log_history`
--

/*!50001 DROP VIEW IF EXISTS `vw_log_history`*/;
/*!50001 SET @saved_cs_client          = @@character_set_client */;
/*!50001 SET @saved_cs_results         = @@character_set_results */;
/*!50001 SET @saved_col_connection     = @@collation_connection */;
/*!50001 SET character_set_client      = utf8mb4 */;
/*!50001 SET character_set_results     = utf8mb4 */;
/*!50001 SET collation_connection      = utf8mb4_unicode_ci */;
/*!50001 CREATE ALGORITHM=UNDEFINED */
/*!50013 SQL SECURITY INVOKER */
/*!50001 VIEW `vw_log_history` AS select from_unixtime(min(`logs`.`time`)) AS `First Logged`,from_unixtime(max(`logs`.`time`)) AS `Last Logged`,count(0) AS `Occurrences`,`realmlist`.`name` AS `Realm`,`logs`.`type` AS `type`,`logs`.`level` AS `level`,`logs`.`string` AS `string` from (`logs` left join `realmlist` on((`logs`.`realm` = `realmlist`.`id`))) group by `logs`.`string`,`logs`.`type`,`logs`.`realm` */;
/*!50001 SET character_set_client      = @saved_cs_client */;
/*!50001 SET character_set_results     = @saved_cs_results */;
/*!50001 SET collation_connection      = @saved_col_connection */;

--
-- Final view structure for view `vw_rbac`
--

/*!50001 DROP VIEW IF EXISTS `vw_rbac`*/;
/*!50001 SET @saved_cs_client          = @@character_set_client */;
/*!50001 SET @saved_cs_results         = @@character_set_results */;
/*!50001 SET @saved_col_connection     = @@collation_connection */;
/*!50001 SET character_set_client      = utf8mb4 */;
/*!50001 SET character_set_results     = utf8mb4 */;
/*!50001 SET collation_connection      = utf8mb4_unicode_ci */;
/*!50001 CREATE ALGORITHM=UNDEFINED */
/*!50013 SQL SECURITY INVOKER */
/*!50001 VIEW `vw_rbac` AS select `t1`.`linkedId` AS `Permission ID`,`t1`.`id` AS `Permission Group`,ifnull(`t2`.`secId`,'linked') AS `Security Level`,`t3`.`name` AS `Permission` from ((`rbac_linked_permissions` `t1` left join `rbac_default_permissions` `t2` on((`t1`.`id` = `t2`.`permissionId`))) left join `rbac_permissions` `t3` on((`t1`.`linkedId` = `t3`.`id`))) */;
/*!50001 SET character_set_client      = @saved_cs_client */;
/*!50001 SET character_set_results     = @saved_cs_results */;
/*!50001 SET collation_connection      = @saved_col_connection */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2025-07-21 22:51:07
