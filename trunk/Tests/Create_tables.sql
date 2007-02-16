create database IF NOT EXISTS mod_ndb_test;

use mod_ndb_test;

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



INSERT INTO table1 values
  ( 1 , "fred" , NULL, 5, 12 ),
  ( 2 , "mary" , "greenish" , 11, 680 ),
  ( 3 , "joe"  , "yellow" , 12 , 19 ) ,
  ( 4 , "jen"  , "brown" ,  12 , 12 ) ,
  ( 5 , "tom"  , "brown" ,  2  , 13 );
  
 
