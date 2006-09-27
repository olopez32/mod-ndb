create database IF NOT EXISTS mod_ndb_test;

use mod_ndb_test;

CREATE TABLE `session` (
  `sess_id` bigint(20) unsigned NOT NULL,
  `sess_var_name` varchar(20) NOT NULL,
  `sess_var_value` varchar(100) default NULL,
  PRIMARY KEY  (`sess_id`,`sess_var_name`)
) ENGINE=ndbcluster DEFAULT CHARSET=latin1;

INSERT INTO `session` VALUES (1,'user_name','jdd');
INSERT INTO `session` VALUES (2,'user_name','tk');
INSERT INTO `session` VALUES (3,'user_name','brian');
INSERT INTO `session` VALUES (3,'last_page_visited','index.php');
INSERT INTO `session` VALUES (3,'time_zone','GMT-0400');
