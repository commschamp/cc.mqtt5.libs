#!/bin/bash

if [ -z "${CC}" -o -z "${CXX}" ]; then
    echo "ERROR: Compilers are not provided"
    exit 1
fi

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR=$( dirname ${SCRIPT_DIR} )
export BUILD_DIR="${ROOT_DIR}/build.full.${CC}"
if [ -n "${BUILD_TYPE}" ]; then
    BUILD_DIR="${BUILD_DIR}.${BUILD_TYPE}"
else
    BUILD_TYPE="None"
fi

export COMMON_INSTALL_DIR=${BUILD_DIR}/install
export COMMON_BUILD_TYPE=Debug
export EXTERNALS_DIR=${ROOT_DIR}/externals
mkdir -p ${BUILD_DIR}

${SCRIPT_DIR}/prepare_externals.sh

cd ${BUILD_DIR}
cmake .. -DCMAKE_INSTALL_PREFIX=${COMMON_INSTALL_DIR} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCC_MQTT5_USE_CCACHE=ON \
    -DCC_MQTT5_WITH_DEFAULT_SANITIZERS=ON \
    -DCC_MQTT5_BUILD_UNIT_TESTS=ON -DCC_MQTT5_BUILD_INTEGRATION_TESTS=ON  \
    -DCC_MQTT5_CLIENT_AFL_FUZZ=ON \
    -DCC_MQTT5_CUSTOM_CLIENT_CONFIG_FILES="${ROOT_DIR}/client/lib/script/BareMetalTestConfig.cmake;${ROOT_DIR}/client/lib/script/Qos1TestConfig.cmake;${ROOT_DIR}/client/lib/script/Qos0TestConfig.cmake" \
    "$@"

procs=$(nproc)
if [ -n "${procs}" ]; then
    procs_param="--parallel ${procs}"
fi

cmake --build ${BUILD_DIR} --config ${COMMON_BUILD_TYPE} --target install ${procs_param}
