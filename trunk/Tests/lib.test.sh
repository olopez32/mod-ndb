#  Test suite control script for mod_ndb

[ -d /usr/xpg4/bin ]  && PATH=/usr/xpg4/bin:$PATH
TESTDIR=`pwd`

t.lis() {         # list test cases
  pushd $TESTDIR
  awk -f runner.awk -v test=$1 -v mode=list test.list
  popd
}

t.dif() {         # run test & print difference 
  pushd $TESTDIR
  awk -f runner.awk -v test=$1 -v mode=compare -v diff=1 test.list | sh
  popd
}

t.run() {         # run test
  pushd $TESTDIR
  awk -f runner.awk -v test=$1 test.list | sh
  popd
}

t.est() {        # run test and print OK or fail
  pushd $TESTDIR
  awk -f runner.awk -v test=$1 -v mode=compare test.list | sh
  popd
}

t.REM() {       # remove results file
  rm -i $TESTDIR/results/$1*
}

t.rec() {      # run test and record results as official
  pushd $TESTDIR
  awk -f runner.awk -v test=$1 -v mode=record test.list | sh -C
  popd
}

t.sql() {
 pushd $TESTDIR
 awk -f runner.awk -v test=$1 -v mode=sql | sh 
 popd
} 

t.huh() {
  echo "t.lis       list test cases"
  echo "t.run       run test case and display results"
  echo "t.est       run test and print OK or Fail"
  echo "t.dif       run test and display difference"
  echo "t.REM       remove existing results file"
  echo "t.rec       run test and record results as official"
}
#
#
#exe="sh"
##arg=$1 
#while shift
# do
#   test "$arg" = "-c" && mode="-v mode=config"
#   test "$arg" = "-e" && exe="cat"  
#   test "$arg" = "-h" && usage 
#   test "$arg" = "-l" && mode="-v mode=list" && exe="cat"
#   test "$arg" = "-R" && mode="-v mode=record" && exe="sh -C" # noclobber
#   test "$arg" = "-s" && mode="-v mode=sql"
#   testcase=$arg
#   arg=$1
#done
#
#awk -f runner.awk -v test=$testcase $mode test.list | $exe
