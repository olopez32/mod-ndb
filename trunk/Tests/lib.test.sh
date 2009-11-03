# Copyright (C) 2006 - 2009 Sun Microsystems
# All rights reserved. Use is subject to license terms.

#  Test suite control script for mod_ndb

[ -d /usr/xpg4/bin ]  && PATH=/usr/xpg4/bin:$PATH
TESTDIR=`pwd`

t.huh() {
  echo "t.list      list test cases"
  echo "t.run       run test case and display server response"
  echo "t.test      run test and print OK or Fail"
  echo "t.all       run tests and display summary results"
  echo "t.diff      run test and display difference with recorded results"
  echo "t.echo      display shell command used for test case"
  echo "t.conf      show apache configuration for test case"
  echo "t.REM       remove existing results file for a test"
  echo "t.rec       run test and record results as official"
  echo "t.sql       run SQL queries used to create tables in test suite"
  echo "t.multi     run multi-threaded concurrency test"
  echo 
  echo "cov.load    pipe code coverage data into the database"
  echo "cov.all     view all results from code coverage test"
  echo "cov.missed  view code coverage points missed by testing"
}

t.help() {
  t.huh
}

t.list() {         # list test cases
  pushd $TESTDIR > /dev/null 
  awk -f runner.awk -v test=$1 -v mode=list test.list
  popd >  /dev/null
}

t.idlist() {       # list just the identifiers of test cases
  pushd $TESTDIR > /dev/null 
  awk -f runner.awk -v test=$1 -v mode=idlist test.list
  popd >  /dev/null
}

t.diff() {         # run test & print difference 
  pushd $TESTDIR > /dev/null
  awk -f runner.awk -v test=$1 -v mode=compare -v diff=1 test.list | $SHELL
  popd > /dev/null
}

t.run() {         # run test
  pushd $TESTDIR > /dev/null
  awk -f runner.awk -v test=$1 test.list | $SHELL
  popd > /dev/null
}

t.echo() {         # run test
  pushd $TESTDIR > /dev/null
  awk -f runner.awk -v test=$1 test.list
  popd > /dev/null
}

t.test() {        # run test and print OK or fail
  pushd $TESTDIR > /dev/null
  awk -f runner.awk -v test=$1 -v mode=compare test.list | $SHELL
  popd > /dev/null
}

t.all() {        # run tests and print summary of results
  pushd $TESTDIR > /dev/null
  awk -f runner.awk -v test=$1 -v mode=compare test.list \
   | $SHELL | awk -f summary.awk
  popd > /dev/null
}

t.conf() {       # display httpd.conf relevant to a particular test
  pushd $TESTDIR > /dev/null
  awk -f runner.awk -v test=$1 -v mode=config test.list | $SHELL
  popd > /dev/null
}
  
t.REM() {       # delete a result
  pushd $TESTDIR > /dev/null
  for i in `awk -f runner.awk -v test=$1 -v mode=idlist test.list`
    do
      echo "Deleting stored result for test" $i
      ./erase.sh $i
 done
 popd > /dev/null
}

t.rec() {      # run test and record results as official
  pushd $TESTDIR > /dev/null
  awk -f runner.awk -v test=$1 -v mode=record test.list | $SHELL -C
  popd > /dev/null
}

t.sql() {      # run CREATE TABLE scripts in MySQL
  pushd $TESTDIR > /dev/null
  awk -f runner.awk -v test=$1 -v mode=sql test.list | $SHELL 
  popd > /dev/null
} 

t.multi() {    # run multi-threaded test using "httperf" tool
  pushd $TESTDIR > /dev/null
  sh concurrent.sh
  popd > /dev/null
}

cov.load() {   # pipe code coverage data into the database
  awk -v url="http://localhost:3080/ndb/test/codecov" \
    -f $TESTDIR/load_json_data.awk | $SHELL
}
  
cov.all() {
  curl http://localhost:3080/ndb/test/codecov/all
}

cov.missed() {
  curl http://localhost:3080/ndb/test/codecov/missed
}