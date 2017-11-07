SET interactive_timeout=31536000;
        SET wait_timeout=31536000;
        CREATE DATABASE IF NOT EXISTS app_server_db DEFAULT  CHARSET utf8;
        use app_server_db;  

-- ----------------------------
-- Table structure for favorate_config
-- ----------------------------
CREATE TABLE IF NOT EXISTS `favorate_config` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `user_id` int(10) unsigned NOT NULL,
  `point_id` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for notify_record
-- ----------------------------
CREATE TABLE IF NOT EXISTS `notify_record` (
  `notify_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `device_id` int(10) unsigned DEFAULT NULL,
  `type` int(10) unsigned NOT NULL,
  `level` int(10) unsigned NOT NULL,
  `time` int(10) unsigned NOT NULL,
  `info` varchar(512) DEFAULT NULL,
  PRIMARY KEY (`notify_id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for picture_record
-- ----------------------------
CREATE TABLE IF NOT EXISTS `picture_record` (
  `picture_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `user_id` int(10) unsigned NOT NULL,
  `file_name` varchar(68) NOT NULL,
  `description` varchar(132) DEFAULT NULL,
  `time` int(10) unsigned NOT NULL,
  `longitude` decimal(30,10) DEFAULT NULL,
  `latitude` decimal(30,10) DEFAULT NULL,
  `url` varchar(132) DEFAULT NULL,
  PRIMARY KEY (`picture_id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for pointconfig
-- ----------------------------
CREATE TABLE IF NOT EXISTS `pointconfig` (
  `point_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `group_id` varchar(36) NOT NULL, 
  `device_id` int(10) unsigned NOT NULL,
  `device_type` int(10) unsigned DEFAULT '0',
  `point_name` varchar(68) NOT NULL,
  `point_type` int(10) unsigned DEFAULT '0',
  `resolution` int(10) DEFAULT '0',
  `code_rate` int(10) DEFAULT '0',
  `frame_rate` int(10) DEFAULT '0',
  `status` int(10) unsigned DEFAULT '0',
  `ctrl_enable` int(10) unsigned DEFAULT '0',
  `stream_count` int(10) unsigned DEFAULT '1',
  `longitude` decimal(30,10) DEFAULT NULL,
  `latitude` decimal(30,10) DEFAULT NULL,
  `rtsp_url` varchar(132) DEFAULT NULL,
  `reserved_num` int(10) DEFAULT NULL,
  `reserved_str` varchar(32) DEFAULT NULL,
  PRIMARY KEY (`point_id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for serverconfig
-- ----------------------------
CREATE TABLE IF NOT EXISTS `serverconfig` (
  `server_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `server_type` int(10) unsigned DEFAULT '0',
  `server_ip` varchar(20) DEFAULT NULL,
  `mapping_ip` varchar(64) DEFAULT NULL,
  `mapping_port` int(10) unsigned DEFAULT '0',
  `status` int(10) unsigned DEFAULT '0',
  `reserved_num` int(10) DEFAULT NULL,
  `reserved_str` varchar(32) DEFAULT NULL,
  PRIMARY KEY (`server_id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for user_point_config
-- ----------------------------
CREATE TABLE IF NOT EXISTS `user_point_config` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `user_id` int(10) unsigned NOT NULL,
  `point_id` varchar(36) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for userconfig
-- ----------------------------
CREATE TABLE IF NOT EXISTS `userconfig` (
  `user_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `user_name` varchar(36) NOT NULL,
  `user_pw` varchar(32) NOT NULL,
  `salt` varchar(32) NOT NULL,
  `authority` varchar(32) NOT NULL,
  `phone_number` varchar(16) DEFAULT NULL,
  `phone_model` varchar(36) DEFAULT NULL,
  `device_type` int(10) unsigned DEFAULT '0',
  `device_id` varchar(64) DEFAULT NULL,
  `token` varchar(64) DEFAULT NULL,
  `status` int(10) unsigned DEFAULT '0',
  `time` int(10) unsigned DEFAULT '0',
  `login_ip` varchar(20) DEFAULT NULL,
  `reserved_num` int(10) DEFAULT NULL,
  `reserved_str` varchar(32) DEFAULT NULL,
  PRIMARY KEY (`user_id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for video_record
-- ----------------------------
CREATE TABLE IF NOT EXISTS `video_record` (
  `video_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `user_id` int(10) unsigned NOT NULL,
  `file_name` varchar(68) NOT NULL,
  `description` varchar(132) DEFAULT NULL,
  `time` int(10) unsigned NOT NULL,
  `longitude` decimal(30,10) DEFAULT NULL,
  `latitude` decimal(30,10) DEFAULT NULL,
  `url` varchar(132) DEFAULT NULL,
  PRIMARY KEY (`video_id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;
-- ----------------------------
-- Table structure for group_config
-- ----------------------------
CREATE TABLE `group_config` (
  `group_id` varchar(36) NOT NULL,
  `group_name` varchar(32) NOT NULL,
  `parent_id` varchar(36) NOT NULL,
  PRIMARY KEY (`group_id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;
-- ----------------------------
-- Table structure for applog_record
-- ---------------------------- 
CREATE TABLE `applog_record` (
  `log_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `login_id` varchar(36) NOT NULL,
  `app_name` varchar(64) DEFAULT NULL,
  `app_version` varchar(64) DEFAULT NULL,
  `log_type` int(10) unsigned NOT NULL DEFAULT '0',
  `log_time` int(10) unsigned NOT NULL DEFAULT '0',
  `point_id` int(10) unsigned NOT NULL DEFAULT '0',
  `group_id` varchar(36) DEFAULT NULL,
  `description` varchar(128) DEFAULT NULL,
  PRIMARY KEY (`log_id`)
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;
