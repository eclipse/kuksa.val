#!/bin/bash
set -xe
basepath=`pwd`

scriptName=${0}
function printUsage {
  echo "Usage:"
  echo "    ${scriptName} <builddir> [artifactdir]"
  exit 1
}

while getopts ":b:a:h" opt; do
  case ${opt} in
    b ) builddir=$OPTARG
      ;;
    a ) artifactdir=$OPTARG
      ;;
    h|\? ) printUsage 
      ;;
  esac
done

if [ -z "${builddir}" ]
then
    builddir=build
    echo "Use default build director ${builddir}"
fi
mkdir -p ${builddir}
echo "Go to build directory ${builddir}"
cd ${builddir}

cmake -DCMAKE_BUILD_TYPE=Coverage -DBUILD_UNIT_TEST=ON -DCTEST_RESULT_CODE=no ..

make -j8

rm -rf test/unit-test/results.xml
rm -rf coverage.xml

ctest --output-on-failure
gcovr -r ..  --branches -e ../test/ -e ../3rd-party-libs/ --xml -o coverage.xml

echo "archive artifacts under ${artifactdir}"
cd ${basepath}
if [ ! -d "${artifactdir}" ] 
then
  mkdir -p ${artifactdir}
fi
cp ${builddir}/test/unit-test/results.xml ${artifactdir}/
cp ${builddir}/coverage.xml ${artifactdir}/
