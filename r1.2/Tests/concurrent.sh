# Concurrent operations test for mod_ndb

which httperf || echo "Cannot find httperf." && exit 1

# Default to 100 httperf connections
NTHR=${1-100}

build_test_file() {
  awk  -v nthr=$NTHR -v nrows=$2 -v mode=$1 '

  function create_row(n) {
    o1 = int(rand() * 600) + int(rand() * 600);
    o2 = int(rand() * 30);
    printf("/ndb/test/perf1 method=POST contents=\"" \
           "i=%d&c1=YesHereIsSomeText&c2=_____6789_123456789_abcdefg_" \
           "&o1=%d&o2=%d&m1=3.14&m2=99\"\n", n, o1, o2);
  }

  BEGIN { 
    batch_sz = nrows / nthr;

    for ( i = 0 ; i < nrows ; i += batch_sz) {
      for(j=0 ; j < batch_sz ; j++) {
        if(mode == "load") 
          create_row(i+j);
        else if(mode == "sel1") 
          printf("/ndb/test/perf1/item/%d\n", int(rand() * nrows))
        else if(mode == "sel2") 
          printf("/ndb/test/perf1/idx/%d\n", int(rand() * 1000) + 100 )
        else if(mode == "del") 
          printf("/ndb/test/perf1/item/%d method=DELETE \n", i+j);
        else if(mode == "mixed") {
          d = int(rand() * 1000);
          create_row(6000 + d);
          printf("/ndb/test/perf1/item/%d method=POST contents=\"m2=@++\" \n",d);
          printf("/ndb/test/perf1/item/%d\n", d + 50)
          printf("/ndb/test/perf1/item/%d method=DELETE \n", 6000 + d);
        }
      }
      printf("\n");
    }
  }'
}

run_bench_dbug() {
    httperf --send-buffer=512 --recv-buffer=1024 --port=3080 --hog $1
}

run_bench() {
    httperf --send-buffer=512 --recv-buffer=1024 --port=3080 --hog $1 \
     2>&1 | awk '
      /^Total:/ 
      /^Request rate:/
      /^Reply status:/
    '
 }

echo '=================================='
echo '== httperf concurrency test     =='
echo '==     using table perf1        =='
echo '==                              =='
echo '==  i   INT  PRIMARY KEY        =='
echo '==  c1  VARCHAR(30)             =='
echo '==  c2  VARCHAR(30)             =='
echo '==  o1  INT                     =='
echo '==  o2  INT                     =='
echo '==  m1  DECIMAL(4,2)            =='
echo '==  m2  INT UNSIGNED NOT NULL   =='
echo '==  INDEX USING BTREE (o1, o2)  =='
echo '=================================='
echo 
echo

build_test_file load 2500 > current/b1
build_test_file sel1 2500 > current/b2
build_test_file sel2 1000 > current/b3
build_test_file mixed 500 > current/b4
build_test_file del  2500 > current/b5

echo '=================================='
echo '== Create 2,500 records         =='
echo '=================================='
run_bench --wsesslog=$NTHR,0,current/b1

echo
echo '=================================='
echo '== 2,500 random GETs  (PK)      =='
echo '=================================='
run_bench --wsesslog=$NTHR,0,current/b2

echo
echo '=================================='
echo '== 1,000 random GETs (Ordered)  =='
echo '=================================='
run_bench --wsesslog=$NTHR,0,current/b3

echo
echo '=================================='
echo '== 2,000 mixed requests         =='
echo '=================================='
run_bench --wsesslog=$NTHR,0,current/b4

echo
echo '=================================='
echo '== DELETE 2,500 records         =='
echo '=================================='
run_bench --wsesslog=$NTHR,0,current/b5

