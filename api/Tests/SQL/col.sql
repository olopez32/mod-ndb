use mod_ndb_tests;

drop table if exists col0;
CREATE TABLE col0 (
  b int NOT NULL, 
  c int NOT NULL,
  a varchar(20) NOT NULL,
  d int NOT NULL, 
  e int NOT NULL, 
  f int NOT NULL,
  g int NOT NULL, 
  PRIMARY KEY USING HASH (d, b),
  UNIQUE INDEX gf_idx USING HASH (g, f), 
  INDEX ce_idx (c, e)
) engine=ndbcluster;


INSERT INTO col0 (a,b,c,d,e,f,g) values
  ("green", 0, 10, 50, 100, 1000, 5000 ),
  ("blue",  1, 11, 51, 101, 1001, 5001 ),
  ("red",   2, 12, 52, 102, 1002, 5002 ),
  ("orange",3, 13, 53, 103, 1003, 5003 ),
  ("indigo",4, 14, 54, 104, 1004, 5004 ),
  ("yellow",5, 15, 55, 105, 1005, 5005 ),
  ("cyan",  6, 16, 56, 106, 1006, 5006 )
;