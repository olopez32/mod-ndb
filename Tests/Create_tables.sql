DROP DATABASE IF EXISTS mod_ndb_tests;
CREATE DATABASE mod_ndb_tests;
USE mod_ndb_tests;

/* data type tests */
CREATE TABLE typ1 (
  id int not null primary key,
  name char(8),
  color varchar(16),
  nvisits int unsigned,
  channel int,
  index (channel)
) engine=ndbcluster;

INSERT INTO typ1 values
  ( 1 , "fred" , NULL, 5, 12 ),
  ( 2 , "mary" , "greenish" , 11, 680 ),
  ( 3 , "joe"  , "yellow" , 12 , 19 ) ,
  ( 4 , "jen"  , "brown" ,  12 , 12 ) ,
  ( 5 , "tom"  , "brown" ,  2  , 13 );

CREATE TABLE typ2 (
  i int NOT NULL PRIMARY KEY,
  t tinyint(4),
  ut tinyint(3) unsigned,
  s smallint(6),
  us smallint(5) unsigned,
  m mediumint(9),
  um mediumint(8) unsigned
) engine=ndbcluster; 

INSERT INTO typ2 VALUES 
  (1,1,1,1,1,1,1), 
  (2,500,500,500,500,500,500),       /* will be (2,127,255,...) */
  (3,-40,-40,-40,-40,-40,-40),       /* will be (3,-40,0,-40,0,-40,0) */
  (4,900,900,900,900,900,900),       /* will be (4,127,255,900, ... ) */
  (5,-900,-900,-900,-900,-900,-900); /* will be (5,-128,0,-900,0,-900,0) */


CREATE TABLE typ3 (
  i int NOT NULL PRIMARY KEY,
  d1 decimal(8,2),
  d2 decimal(14,4),
  d3 decimal(3,1),
  d4 decimal(65,30)
) engine=ndbcluster;

CREATE TABLE typ4 (
  i int not null primary key,
  t time,
  d date,
  dt datetime,
  ts timestamp
) engine=ndbcluster;

INSERT INTO typ4 VALUES 
  (1,'10:30:00','2007-11-01','2007-11-01 10:30:00', '2007-11-01 10:30:00');

CREATE TABLE typ5 (
  i int primary key auto_increment not null,
  c int unsigned NOT NULL,
  unique index c_idx (c)
) engine=ndbcluster ;

CREATE TABLE typ6 (
  i int not null default 0,
  j int not null default 0,
  name varchar(20) not null,
  PRIMARY KEY USING HASH (i,j),
  UNIQUE KEY USING HASH (name)
) engine=ndbcluster;

/* -- "session handling" tests -- */

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


