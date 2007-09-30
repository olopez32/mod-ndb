#  Test suite control script for mod_ndb
# unlike "run-test", this can be run from any directory 

[ -d /usr/xpg4/bin ]  && PATH=/usr/xpg4/bin:$PATH
TESTDIR=`pwd`

t.huh() {
  echo "t.list      list test cases"
  echo "t.run       run test case and display server response"
  echo "t.test      run test and print OK or Fail"
  echo "t.diff      run test and display difference"
  echo "t.echo      display command used for test case"
  echo "t.conf      show apache configuration for test case"
  echo "t.REM       remove existing results file"
  echo "t.rec       run test and record results as official"
  echo "t.sql       run SQL queries used to prepare database for test"
}

t.list() {         # list test cases
  pushd $TESTDIR > /dev/null 
  awk -f runner.awk -v test=$1 -v mode=list test.list
  popd >  /dev/null
}

t.diff() {         # run test & print difference 
  pushd $TESTDIR > /dev/null
  awk -f runner.awk -v test=$1 -v mode=compare -v diff=1 test.list | sh
  popd > /dev/null
}

t.run() {         # run test
  pushd $TESTDIR > /dev/null
  awk -f runner.awk -v test=$1 test.list | sh
  popd > /dev/null
}

t.echo() {         # run test
  pushd $TESTDIR > /dev/null
  awk -f runner.awk -v test=$1 test.list
  popd > /dev/null
}

t.test() {        # run test and print OK or fail
  pushd $TESTDIR > /dev/null
  awk -f runner.awk -v test=$1 -v mode=compare test.list | sh
  popd > /dev/null
}

t.conf() {
  pushd $TESTDIR > /dev/null
  awk -f runner.awk -v test=$1 -v mode=config test.list | sh
  popd > /dev/null
}
  
t.REM() {       # remove results file
  rm -i $TESTDIR/results/$1*
}

t.rec() {      # run test and record results as official
  pushd $TESTDIR > /dev/null
  awk -f runner.awk -v test=$1 -v mode=record test.list | sh -C
  popd > /dev/null
}

t.sql() {
  pushd $TESTDIR > /dev/null
  awk -f runner.awk -v test=$1 -v mode=sql | sh 
  popd > /dev/null
} 
