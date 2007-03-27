create database IF NOT EXISTS mod_ndb_test;

use mod_ndb_test;

CREATE TABLE numtypes (
  i int NOT NULL PRIMARY KEY,
  t tinyint(4),
  ut tinyint(3) unsigned,
  s smallint(6),
  us smallint(5) unsigned,
  m mediumint(9),
  um mediumint(8) unsigned,
) engine=ndbcluster; 

CREATE TABLE datetypes (
  i int not null primary key,
  t time,
  d date,
  dt datetime,
  ts timestamp
) engine=ndbcluster;

CREATE TABLE table1 (
  id int not null primary key,
  name char(8),
  color varchar(16),
  nvisits int unsigned,
  channel int,
  index (channel)
) engine=ndbcluster;

CREATE TABLE table2 (
  i int primary key auto_increment not null,
  c int unsigned NOT NULL
) engine=ndbcluster ;

CREATE TABLE table4 (
  i int not null default 0,
  j int not null default 0,
  name varchar(20) not null,
  PRIMARY KEY USING HASH (i,j),
  UNIQUE KEY USING HASH (name)
) engine=ndbcluster;


INSERT INTO numtypes VALUES 
  (1,1,1,1,1,1,1),
  (2,500,500,500,500,500,500),  /* will be (2,127,255,...) */
  (3,-40,-40,-40,-40,-40,-40),  /* will be (3,-40,0,-40,0,-40,0) */
  (4,900,900,900,900,900,900),  /* will be (4,127,255,900, ... ) */
  (5,-900,-900,-900,-900,-900); /* will be (5,-128,0,-900,0,-900,0) */

INSERT INTO datetypes VALUES 
  (1,'10:30:00','2007-11-01','2007-11-01 10:30:00', now());

INSERT INTO table1 values
  ( 1 , "fred" , NULL, 5, 12 ),
  ( 2 , "mary" , "greenish" , 11, 680 ),
  ( 3 , "joe"  , "yellow" , 12 , 19 ) ,
  ( 4 , "jen"  , "brown" ,  12 , 12 ) ,
  ( 5 , "tom"  , "brown" ,  2  , 13 );
  
 
