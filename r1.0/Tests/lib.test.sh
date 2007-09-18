#  Test suite control script for mod_ndb
# unlike "run-test", this can be run from any directory 

[ -d /usr/xpg4/bin ]  && PATH=/usr/xpg4/bin:$PATH
TESTDIR=`pwd`

t.huh() {
  echo "t.lis       list test cases"
  echo "t.run       run test case and display results"
  echo "t.est       run test and print OK or Fail"
  echo "t.dif       run test and display difference"
  echo "t.REM       remove existing results file"
  echo "t.rec       run test and record results as official"
}

t.lis() {         # list test cases
  pushd $TESTDIR > /dev/null 
  awk -f runner.awk -v test=$1 -v mode=list test.list
  popd >  /dev/null
}

t.dif() {         # run test & print difference 
  pushd $TESTDIR > /dev/null
  awk -f runner.awk -v test=$1 -v mode=compare -v diff=1 test.list | sh
  popd > /dev/null
}

t.run() {         # run test
  pushd $TESTDIR > /dev/null
  awk -f runner.awk -v test=$1 test.list | sh
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
