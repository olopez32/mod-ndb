use mod_ndb_tests;

DROP TABLE IF EXISTS code_cov;

CREATE TABLE code_cov ( 
  file varchar(60) not null,
  name varchar(60) not null, 
  count int unsigned,
  primary key using hash (file, name),
  index count_idx (count)
) engine = ndbcluster; 


  