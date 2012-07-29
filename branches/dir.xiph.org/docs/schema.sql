-- MySQL dump 10.11
--
-- Host: localhost    Database: dir_xiph_org
-- ------------------------------------------------------
-- Server version	5.0.45-Debian_1ubuntu3.3-log

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `media_type`
--

DROP TABLE IF EXISTS `media_type`;
CREATE TABLE `media_type` (
  `id` int(10) unsigned NOT NULL,
  `media_type_name` varchar(255) collate utf8_unicode_ci NOT NULL,
  `media_type_url` varchar(255) collate utf8_unicode_ci NOT NULL,
  PRIMARY KEY  (`id`),
  KEY `media_type_url` (`media_type_url`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

--
-- Table structure for table `mountpoint`
--

DROP TABLE IF EXISTS `mountpoint`;
CREATE TABLE `mountpoint` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `stream_name` varchar(255) collate utf8_unicode_ci NOT NULL,
  `description` varchar(255) collate utf8_unicode_ci default NULL,
  `url` varchar(255) collate utf8_unicode_ci default NULL,
  `listeners` int(10) unsigned NOT NULL,
  `max_listeners` int(10) unsigned default NULL,
  `current_song` char(255) collate utf8_unicode_ci default NULL,
  `media_type_id` int(10) unsigned default NULL,
  `bitrate` varchar(25) collate utf8_unicode_ci default NULL,
  `channels` tinyint(3) unsigned default NULL,
  `samplerate` mediumint(8) unsigned default NULL,
  `cluster_password` varchar(255) collate utf8_unicode_ci default NULL,
  PRIMARY KEY  (`id`),
  KEY `cluster_password` (`cluster_password`),
  KEY `stream_name` (`stream_name`),
  KEY `listeners` (`listeners`),
  KEY `fk_media_type` (`media_type_id`,`listeners`)
) ENGINE=InnoDB AUTO_INCREMENT=3829469 DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

DELIMITER ;;
/*!50003 SET SESSION SQL_MODE="" */;;
/*!50003 CREATE */ /*!50017 DEFINER=`root`@`localhost` */ /*!50003 TRIGGER `cascade_listeners_on_delete_mountpoint` AFTER DELETE ON `mountpoint` FOR EACH ROW DELETE FROM mountpoint_data WHERE mountpoint_id = OLD.id */;;

DELIMITER ;
/*!50003 SET SESSION SQL_MODE=@OLD_SQL_MODE */;

--
-- Table structure for table `mountpoint_data`
--

DROP TABLE IF EXISTS `mountpoint_data`;
CREATE TABLE `mountpoint_data` (
  `mountpoint_id` int(10) unsigned NOT NULL,
  `listeners` int(11) NOT NULL,
  `current_song` char(255) default NULL,
  PRIMARY KEY  (`mountpoint_id`),
  KEY `listeners` (`listeners`)
) ENGINE=MEMORY DEFAULT CHARSET=utf8;

--
-- Table structure for table `mountpoints_tags`
--

DROP TABLE IF EXISTS `mountpoints_tags`;
CREATE TABLE `mountpoints_tags` (
  `mountpoint_id` int(10) unsigned NOT NULL,
  `tag_id` int(10) unsigned NOT NULL,
  PRIMARY KEY  (`mountpoint_id`,`tag_id`)
) ENGINE=MEMORY DEFAULT CHARSET=latin1;

--
-- Table structure for table `server`
--

DROP TABLE IF EXISTS `server`;
CREATE TABLE `server` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `mountpoint_id` int(10) unsigned NOT NULL,
  `sid` char(36) character set ascii NOT NULL,
  `listen_url` char(255) character set latin1 NOT NULL,
  `listeners` int(10) unsigned NOT NULL,
  `max_listeners` int(10) unsigned default NULL,
  `current_song` char(255) default NULL,
  `checked` tinyint(3) unsigned NOT NULL default '0',
  `checked_at` timestamp NULL default NULL,
  `last_touched_at` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `last_touched_from` int(10) unsigned NOT NULL,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `listen_url` (`listen_url`),
  KEY `sid` (`sid`),
  KEY `mountpoint_id` (`mountpoint_id`)
) ENGINE=MEMORY AUTO_INCREMENT=45458591 DEFAULT CHARSET=utf8;

DELIMITER ;;
/*!50003 SET SESSION SQL_MODE="" */;;
/*!50003 CREATE */ /*!50017 DEFINER=`root`@`localhost` */ /*!50003 TRIGGER `cascade_listeners_on_insert` AFTER INSERT ON `server` FOR EACH ROW UPDATE `mountpoint` AS m SET m.`listeners` = m.`listeners` + NEW.`listeners`, m.`max_listeners` = m.`max_listeners` + NEW.`max_listeners` WHERE m.`id` = NEW.`mountpoint_id` */;;

/*!50003 SET SESSION SQL_MODE="" */;;
/*!50003 CREATE */ /*!50017 DEFINER=`root`@`localhost` */ /*!50003 TRIGGER `cascade_listeners_on_update` AFTER UPDATE ON `server` FOR EACH ROW UPDATE `mountpoint` AS m SET m.`listeners` = m.`listeners` - OLD.`listeners` + NEW.`listeners`, m.`max_listeners` = m.`max_listeners` - OLD.`max_listeners` + NEW.`max_listeners`, m.`current_song` = NEW.`current_song` WHERE m.`id` = NEW.`mountpoint_id` */;;

/*!50003 SET SESSION SQL_MODE="" */;;
/*!50003 CREATE */ /*!50017 DEFINER=`root`@`localhost` */ /*!50003 TRIGGER `cascade_listeners_on_delete` AFTER DELETE ON `server` FOR EACH ROW UPDATE `mountpoint` AS m SET m.`listeners` = m.`listeners` - OLD.`listeners`, m.`max_listeners` = m.`max_listeners` - OLD.`max_listeners` WHERE m.`id` = OLD.`mountpoint_id` */;;

DELIMITER ;
/*!50003 SET SESSION SQL_MODE=@OLD_SQL_MODE */;

--
-- Table structure for table `tag`
--

DROP TABLE IF EXISTS `tag`;
CREATE TABLE `tag` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `tag_name` varchar(20) collate utf8_unicode_ci default NULL,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `tag_name` (`tag_name`)
) ENGINE=MEMORY AUTO_INCREMENT=8339 DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

--
-- Table structure for table `tag_cloud`
--

DROP TABLE IF EXISTS `tag_cloud`;
CREATE TABLE `tag_cloud` (
  `tag_id` int(10) unsigned NOT NULL,
  `tag_usage` int(11) NOT NULL,
  PRIMARY KEY  (`tag_id`),
  KEY `tag_usage` (`tag_usage`)
) ENGINE=MEMORY DEFAULT CHARSET=latin1;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2012-07-29 18:29:25
