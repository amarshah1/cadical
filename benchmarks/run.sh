#!/bin/sh

#--------------------------------------------------------------------------#

solver=sadical # only thing to change for other solvers

#--------------------------------------------------------------------------#

if [ -f $solver ]
then
  solver=./$solver
  tests=tests/
  dprtrim=tests/dpr-trim
  precochk=tests/precochk
  makedir="tests"
elif [ -f ../$solver ]
then
  solver=../$solver
  tests=""
  dprtrim=./dpr-trim
  precochk=./precochk
  makedir="."
else
  echo "*** '$0' error: first build '$solver'" 1>&2
  echo "*** '$0' error: then call '$0' from root or 'tests' directory" 1>&2
  exit 1
fi

#--------------------------------------------------------------------------#

echo "Making 'dpr-trim' and 'precochk'."
make -C $makedir dpr-trim precochk

#--------------------------------------------------------------------------#

die () {
  echo
  echo "*** '$0' error: $*" 1>&2
  exit 1
}

first=yes

print () {
  if [ $first = yes ]
  then
    first=no
  elif [ -t 1 ]
  then
    printf '\r%78s\r' ""
  else
    echo
  fi
  printf "$*"
}

#--------------------------------------------------------------------------#

success=0

run () {
  expected=$2
  cnf=${tests}$1.cnf
  out=${tests}$1.out
  err=${tests}$1.err
  prf=${tests}$1.prf
  chk=${tests}$1.chk
  [ -f $cnf ] || die "can not find CNF '$cnf'"
  print "$solver $cnf $prf"
  rm -f $out $err $prf $chk
  $solver $cnf $prf 1>$out 2>$err
  actual=$?
  [ $expected = $actual ] || die "actual exit code $actual differs"
  if [ $actual = 10 ]
  then
    print "$precochk $cnf $out"
    $precochk $cnf $out 1>$chk 2>>$err || die "incorrect model"
  fi
  if [ $actual = 20 ]
  then
    if [ $1 = "emptyclause" ]
    then
      print "# skipping: $dprtrim $cnf $prf"
    else
      print "$dprtrim $cnf $prf"
      $dprtrim $cnf $prf 1>$chk 2>>$err || die "incorrect proof"
    fi
  fi
  success=`expr $success + 1`
}

#--------------------------------------------------------------------------#

# Add test here as 'run <base> <res>' with 'res=10' if the instance is
# satisfiable and 'res=20' if it is unsatisfiable.

run empty 10
run emptyclause 20

run unit1 10
run unit2 10
run unit3 20
run unit4 20

run eq1 10
run eq2 10

run pure1 10

run three1 10
run three2 10
run three3 10
run three4 10
run four 20

run seven1 10
run seven2 10
run seven3 10
run seven4 10
run seven5 10
run seven6 10
run seven7 10
run seven8 10
run eight 20

run regr1 20
run regr2 20
run regr3 10
run regr4 10

run full4 20
run full5 20
run full6 20
run full7 20

run ph2 20
run ph3 20
run ph4 20
run ph5 20
run ph6 20

run tph2 20
run tph3 20
run tph4 20

run add4 20
run add8 20
run add16 20
run add32 20
run add64 20
run add128 20

run prime4 10
run prime9 10
run prime25 10
run prime49 10
run prime121 10
run prime169 10
run prime289 10
run prime361 10
run prime529 10
run prime841 10
run prime961 10
run prime1369 10
run prime1681 10
run prime1849 10
run prime2209 10
run prime65537 20
# run prime4294967297 20 # pretty expensive (~ 10 seconds)


run sqrt2809 10
run sqrt3481 10
run sqrt3721 10
run sqrt4489 10
run sqrt5041 10
run sqrt5329 10
run sqrt6241 10
run sqrt6889 10
run sqrt7921 10
run sqrt9409 10
run sqrt10201 10
run sqrt10609 10
run sqrt11449 10
run sqrt11881 10
run sqrt12769 10
run sqrt16129 10
run sqrt63001 10
run sqrt259081 10
run sqrt1042441 10

run urq3b1 20
run urq3b2 20
run urq3b3 20
run urq3b4 20

#--------------------------------------------------------------------------#

print "Executed $success tests successfully."
echo
