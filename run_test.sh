#!/bin/sh
basepath=`pwd`

if [ $# -ne 2 ]
then
  print_usage
  echo "ERROR: Please name the build directory as the first parameter."
  echo "Example:"
  echo "    ./run_test.sh build artifacts"
  exit 1
fi

builddir=${1}
artifactdir=${2}
cd ${builddir}

rm -rf coverage
mkdir -p coverage

lcov --zerocounters  --directory .
ctest 
lcov --directory . --capture --output-file kuksa.info.tmp
lcov --extract kuksa.info.tmp "*${basepath}/include/*" --extract kuksa.info.tmp "*${basepath}/src/*" --output-file kuksa.info
genhtml --output-directory ./coverage \
  --demangle-cpp --num-spaces 2 --sort \
  --title "My Program Test Coverage" \
  --function-coverage --branch-coverage --legend \
  kuksa.info
gcovr -r ..  --branches -e ../test/ -e ../3rd-party-libs/ --xml -o coverage.xml

cd ${basepath}
cp ${builddir}/test/unit-test/results.xml ${artifactdir}/
cp ${builddir}/coverage.xml ${artifactdir}/
cp -r ${builddir}/coverage ${artifactdir}/coverage
