CREATE DATABASE IF NOT EXISTS mod_ndb_tests;

USE mod_ndb_tests; 

DROP TABLE IF EXISTS perf1;

CREATE TABLE perf1 (
  i int not null,
  c1 varchar(30),
  c2 varchar(30),
  o1 int,
  o2 int,
  m1 decimal(4,2),
  m2 int unsigned NOT NULL,
  PRIMARY KEY USING HASH (i),
  INDEX USING BTREE (o1, o2)
) engine = ndbcluster;

DROP TABLE IF EXISTS perf2;

CREATE TABLE perf2 (
  i int not null,
  bi bigint not null,
  c1 varchar(30),
  m1 int,
  PRIMARY KEY USING HASH (i),
  UNIQUE INDEX USING HASH (bi)
) engine=ndbcluster;
