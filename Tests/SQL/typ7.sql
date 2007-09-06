
CREATE TABLE typ7 (
  id int not null auto_increment primary key,
  vc01 varchar(2000),
  ts timestamp not null
) engine = ndbcluster ;
