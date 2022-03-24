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

cmake -DBUILD_UNIT_TEST=ON -DCTEST_RESULT_CODE=no -DCATCH_ENABLE_COVERAGE=ON ..

make -j8

rm -rf lcov/data/capture/all_targets.info

ctest --output-on-failure
#gcovr -r ..  --branches -e ../test/ -e ../3rd-party-libs/ --xml -o coverage.xml
make lcov 
lcov --list lcov/data/capture/all_targets.info

echo "archive artifacts under ${artifactdir}"
cd ${basepath}
if [ ! -d "${artifactdir}" ] 
then
  mkdir -p ${artifactdir}
fi
cp ${builddir}/lcov/data/capture/all_targets.info ${artifactdir}/
