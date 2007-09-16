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

CREATE TABLE typ7 (
  id int not null primary key,
  vc01 varchar(2000),
  ts timestamp not null
) engine = ndbcluster ;

CREATE TABLE typ8 (
  id int not null primary key,
  doc text 
) engine = ndbcluster ;

INSERT INTO `typ8` VALUES (1,'\nJohannes dei gracia rex Anglie, dominus Hibernie, dux Normannie, Aquitannie et comes Andegavie, archiepiscopis, episcopis, abbatibus, comitibus, baronibus, justiciariis, forestariis, vicecomitibus, prepositis, ministris et omnibus ballivis et fidelibus suis salutem. Sciatis nos intuitu Dei et pro salute anime nostre et omnium antecessorum et heredum nostrorum ad honorem Dei et exaltacionem sancte Ecclesie, et emendacionem regni nostri, per consilium venerabilium patrum nostrorum, Stephani Cantuariensis archiepiscopi totius Anglie primatis et sancte Romane ecclesie cardinalis, Henrici Dublinensis archiepiscopi, Willelmi Londoniensis, Petri Wintoniensis, Joscelini Bathoniensis et Glastoniensis, Hugonis Lincolniensis, Walteri Wygorniensis, Willelmi Coventrensis, et Benedicti Roffensis, episcoporum; magistri Pandulfi domini pape subdiaconi et familiaris, fratris Aymerici magistri milicie Templi in Anglia; et nobilium virorum Willelmi Mariscalli comitis Penbrocie, Willelmi comitis Sarrisberie, Willelmi comitis Warennie, Willelmi comitis Arundellie, Alani de Galeweya constabularii Scocie, Warini filii Geroldi, Petri filii Hereberti, Huberti de Burgo senescalli Pictavie, Hugonis de Nevilla, Mathei filli Hereberti, Thome Basset, Alani Basset, Philippi de Albiniaco, Roberti de Roppel\', Johannis Mariscalli, Johannis filii Hugonis et aliorum fidelium nostrorum:\n\n1. In primis concessisse Deo et hac presenti carta nostra confirmasse, pro nobis et heredibus nostris in perpetuum, quod Anglicana ecclesie libera sit, et habeat jura sua integra, et libertates suas illesas; et ita volumus observari; quod apparet ex eo quod libertatem electionum, que maxima et magis necessaria reputatur ecclesie Anglicane, mera et spontanea voluntate, ante discordiam inter nos et barones nostros motam, concessimus et carta nostra confirmavimus, et eam obtinuimus a domino papa Innocentio tercio confirmari; quam et nos observabimus et ab heredibus nostris in perpetuum bona fide volumus observari. Concessimus eciam omnibus liberis hominibus regni nostri, pro nobis et heredibus nostris in perpetuum, omnes libertates subscriptas, habendas et tenendas eis et heredibus suis, de nobis et heredibus nostris.\n\n2. Si quis comitum vel baronum nostrorum, sive aliorum tenencium de nobis in capite per servicium militare, mortuus fuerit, et cum decesserit heres suus plene etatis fuerit et relevium debeat, habeat hereditatem suam per antiquum relevium; scilicet heres vel heredes comitis de baronia comitis integra per centum libras; heres veI heredes baronis de baronia integra per centum libras; heres vel heredes militis de feodo militis integro per centum solidos ad plus; et qui minus debuerit minus det secundum antiquam consuetudinem feodorum. [Articles, c. 1; 1225, c. 2.]\n\n3. Si autem heres alicujus talium fuerit infra etatem et fuerit in custodia, cum ad etatem pervenerit, habeat hereditatem suam sine relevio et sine fine. [Articles, c. 2; 1225, c. 3.]\n\n4. Custos terre hujusmodi heredis qui infra etatem fuerit, non capiat de terra heredis nisi racionabiles exitus, et racionabiles consuetudines, et racionabilia servicia, et hoc sine destructione et vasto hominum vel rerum; et si nos commiserimus custodiam alicujus talis terre vicecomiti vel alicui alii qui de exitibus illius nobis respondere debeat, et ille destructionem de custodia fecerit veI vastum, nos ab illo capiemus emendam, et terra committatur duobus legalibus et discretis hominibus de feodo illo, qui de exitibus respondeant nobis vel ei cui eos assignaverimus; et si dederimus vel vendiderimus alicui custodiam alicujus talis terre, et ille destructionem inde fecerit vel vastum, amittat ipsam custodiam, et tradatur duobus legalibus et discretis hominibus de feodo illo qui similiter nobis respondeant sicut predictum est. [Articles, c. 3; 1225, c. 4.]\n\n5. Custos autem, quamdiu custodiam terre habuerit, sustentet domos, parcos, vivaria, stagna, molendina, et cetera ad terram illam pertinencia, de exitibus terre ejusdem; et reddat heredi cum ad plenam etatem pervenerit, terram suam totam instauratam de carucis et waynagiis, secundum quod tempus waynagii exiget et exitus terre racionabiliter poterunt sustinere. [Articles, cc. 3, 35; 1225, c. 5.]\n\n6. Heredes maritentur absque disparagacione, ita tamen quod, antequam contrahatur matrimonium, ostendatur propinquis de consanguinitate ipsius heredis. [Articles, c. 3; 1225, c. 6.]\n\n7. Vidua post mortem mariti sui statim et sine difficultate habeat maritagium et hereditatem suam, nec aliquid det pro dote sua, vel pro maritagio suo, vel hereditate sua, quam hereditatem maritus suus et ipsa tenuerint die obitus ipsius mariti, et maneat in domo mariti sul per quadraginta dies post mortem ipsius, infra quos assignetur ei dos sua. [Articles, c. 4; 1225, c. 7.]\n\n8. Nulla vidua distringatur ad se maritandum, dum voluerit vivere sine marito, ita tamen quod securitatem faciat quod se non maritabit sine assensu nostro, si de nobis tenuerit, vel sine assensu domini sui de quo tenuerit, si de alio tenuerit. [Articles, c. 17; 1225, c. 7.]\n\n9. Nec nos nec ballivi nostri seisiemus terram aliquam nec redditum pro debito aliquo, quamdiu catalla debitoris sufficiunt ad debitum reddendum; nec plegii ipsius debitoris distringantur quamdiu ipse capitalis debitor sufficit ad solucionem debiti; et si capitalis debitor defecerit in solucione debiti, non habens unde solvat, plegii respondeant de debito; et si voluerint, habeant terras et redditus debitoris, donec sit eis satisfactum de debito quod ante pro eo solverint, nisi capitalis debitor monstraverit se esse quietum inde versus eosdem plegios. [Articles, c. 5; 1225, c. 8.]\n\n10.Si quis mutuo ceperit aliquid a Judeis, plus vel minus, et moriatur antequam debitum illud solvatur, debitum non usuret quamdiu heres fuerit infra etatem, de quocumque teneat; et si debitum illud inciderit in manus nostras, nos non capiemus nisi catallum contentum in carta. [Articles, C. 34.]\n');
