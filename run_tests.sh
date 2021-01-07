#!/bin/bash
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
if [ ! -d "${builddir}" ] 
then
  echo "ERROR: Please name the build directory as the first parameter."
  printUsage
fi
echo "Go to build directory ${builddir}"
cd ${builddir}

cmake -DCMAKE_BUILD_TYPE=Coverage -DBUILD_UNIT_TEST=ON ..

rm -rf coverage

ctest 
gcovr -r ..  --branches -e ../test/ -e ../3rd-party-libs/ --xml -o coverage.xml

if [ -d "${artifactdir}" ] 
then
echo "archive artifacts under ${artifactdir}"
cd ${basepath}
cp ${builddir}/test/unit-test/results.xml ${artifactdir}/
cp ${builddir}/coverage.xml ${artifactdir}/
fi
