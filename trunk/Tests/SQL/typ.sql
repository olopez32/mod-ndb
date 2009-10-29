DROP DATABASE IF EXISTS mod_ndb_tests;
CREATE DATABASE mod_ndb_tests;
USE mod_ndb_tests;

/* data type tests */
CREATE TABLE typ1 (
  id int not null primary key,
  name char(8),
  color varchar(16),
  nvisits int unsigned,
  channel smallint,
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

INSERT INTO typ3 
VALUES (1, pi(), pi(), pi(), pi() ),
       (2, 1.25, 1234567890.1234, 0, 0 );


CREATE TABLE typ4 (
  i int not null primary key,
  t time,
  d date,
  dt datetime,
  ts timestamp
) engine=ndbcluster;

INSERT INTO typ4 VALUES 
  (1,'10:30:00','2007-11-01','2007-11-01 10:30:00', 
  from_unixtime(1193938200)); 

CREATE TABLE typ5 (
  i int primary key auto_increment not null,
  c int unsigned NOT NULL,
  unique index c_idx (c)
) engine=ndbcluster ;

CREATE TABLE typ6 (
  i int not null default 0,
  j int not null default 0,
  name varchar(20) character set utf8 not null,
  PRIMARY KEY USING HASH (i,j),
  UNIQUE KEY USING HASH (name)
) engine=ndbcluster;

CREATE TABLE typ7 (
  id int not null primary key,
  vc01 varchar(2000),
  ts timestamp not null
) engine = ndbcluster ;

CREATE TABLE typ9 (
  i int not null primary key,
  b1 bit(9) null,
  b2 bit(17) not null,
  b3 bit(1) not null 
) engine = ndbcluster;

INSERT INTO typ9 
VALUES ( 1, NULL , 17   , 0 ),
       ( 2, 1    , 1025 , 1 ),
       ( 3, 2    , 11   , 1 ) ;
