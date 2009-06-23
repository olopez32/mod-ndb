use mod_ndb_tests;

CREATE TABLE utf1 (
  i int not null default 0,
  j int not null default 0,
  name varchar(20)  character set utf8 not null,
  PRIMARY KEY USING HASH (i,j),
  UNIQUE KEY USING HASH (name)
) engine=ndbcluster;

