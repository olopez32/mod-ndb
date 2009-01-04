/* -- "session handling" tests -- */

use mod_ndb_tests ;

DROP TABLE IF EXISTS ses0;

CREATE TABLE ses0 (
  `sess_id` bigint(20) unsigned NOT NULL,
  `sess_var_name` varchar(20) NOT NULL,
  `sess_var_value` varchar(100) default NULL,
  PRIMARY KEY  (`sess_id`,`sess_var_name`)
) ENGINE=ndbcluster DEFAULT CHARSET=latin1;

INSERT INTO ses0 VALUES
 (1,'user_name','jdd'),
 (2,'user_name','tk'),
 (3,'user_name','brian'),
 (3,'last_page_visited','index.php'),
 (3,'time_zone','GMT-0400');


