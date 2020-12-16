#!/bin/sh
basepath=`pwd`

if [ $# -lt 1 ]
then
  echo "ERROR: Please name the build directory as the first parameter."
  echo "Usage:"
  echo "    ./run_test.sh <builddir> [artifactdir]"
  exit 1
fi

builddir=${1}
artifactdir=${2}
cd ${builddir}

cmake -DCMAKE_BUILD_TYPE=Coverage -DBUILD_UNIT_TEST=ON ..

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

if [ -d "${artifactdir}" ] 
then
cd ${basepath}
cp ${builddir}/test/unit-test/results.xml ${artifactdir}/
cp ${builddir}/coverage.xml ${artifactdir}/
cp -r ${builddir}/coverage ${artifactdir}/coverage
fi
