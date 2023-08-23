#!/bin/bash

# Input
# BUILD_DIR - Main build directory
# CC - Main C compiler
# CXX - Main C++ compiler
# EXTERNALS_DIR - (Optional) Directory where externals need to be located
# COMMS_REPO - (Optional) Repository of the COMMS library
# COMMS_TAG - (Optional) Tag of the COMMS library
# CC_MQTT5_REPO - (Optional) Repository of the cc.mqtt5.generated
# CC_MQTT5_TAG - (Optional) Tag of the cc.mqtt5.generated
# COMMON_INSTALL_DIR - (Optional) Common directory to perform installations
# COMMON_BUILD_TYPE - (Optional) CMake build type
# COMMON_CXX_STANDARD - (Optional) CMake C++ standard

#####################################

if [ -z "${BUILD_DIR}" ]; then
    echo "BUILD_DIR hasn't been specified"
    exit 1
fi

if [ -z "${EXTERNALS_DIR}" ]; then
    EXTERNALS_DIR=${BUILD_DIR}/externals
fi

if [ -z "${COMMS_REPO}" ]; then
    COMMS_REPO=https://github.com/commschamp/comms.git
fi

if [ -z "${COMMS_TAG}" ]; then
    COMMS_TAG=master
fi

if [ -z "${CC_MQTT5_REPO}" ]; then
    CC_MQTT5_REPO=https://github.com/commschamp/cc.mqtt5.generated.git
fi

if [ -z "${CC_MQTT5_TAG}" ]; then
    CC_MQTT5_TAG=master
fi

if [ -z "${COMMON_BUILD_TYPE}" ]; then
    COMMON_BUILD_TYPE=Debug
fi

COMMS_SRC_DIR=${EXTERNALS_DIR}/comms
COMMS_BUILD_DIR=${BUILD_DIR}/externals/comms/build
COMMS_INSTALL_DIR=${COMMS_BUILD_DIR}/install
if [ -n "${COMMON_INSTALL_DIR}" ]; then
    COMMS_INSTALL_DIR=${COMMON_INSTALL_DIR}
fi

CC_MQTT5_SRC_DIR=${EXTERNALS_DIR}/cc.mqtt5.generated
CC_MQTT5_BUILD_DIR=${BUILD_DIR}/externals/cc.mqtt5.generated/build
CC_MQTT5_INSTALL_DIR=${CC_MQTT5_BUILD_DIR}/install
if [ -n "${COMMON_INSTALL_DIR}" ]; then
    CC_MQTT5_INSTALL_DIR=${COMMON_INSTALL_DIR}
fi

procs=$(nproc)
if [ -n "${procs}" ]; then
    procs_param="-- -j${procs}"
fi

#####################################

function build_comms() {
    if [ -e ${COMMS_SRC_DIR}/.git ]; then
        echo "Updating COMMS library..."
        cd ${COMMS_SRC_DIR}
        git fetch --all
        git checkout .
        git checkout ${COMMS_TAG}
        git pull --all
    else
        echo "Cloning COMMS library..."
        mkdir -p ${EXTERNALS_DIR}
        git clone -b ${COMMS_TAG} ${COMMS_REPO} ${COMMS_SRC_DIR}
    fi

    echo "Building COMMS library..."
    mkdir -p ${COMMS_BUILD_DIR}
    cmake -S ${COMMS_SRC_DIR} -B ${COMMS_BUILD_DIR} -DCMAKE_INSTALL_PREFIX=${COMMS_INSTALL_DIR} \
        -DCMAKE_BUILD_TYPE=${COMMON_BUILD_TYPE} -DCMAKE_CXX_STANDARD=${COMMON_CXX_STANDARD}
    cmake --build ${COMMS_BUILD_DIR} --config ${COMMON_BUILD_TYPE} --target install ${procs_param}
}

function build_mqtt5() {
    if [ -e ${CC_MQTT5_SRC_DIR}/.git ]; then
        echo "Updating cc.mqtt5.generated library..."
        cd ${CC_MQTT5_SRC_DIR}
        git fetch --all
        git checkout .
        git checkout ${CC_MQTT5_TAG}
        git pull --all
    else
        echo "Cloning cc.mqtt5.generated library..."
        mkdir -p ${EXTERNALS_DIR}
        git clone -b ${CC_MQTT5_TAG} ${CC_MQTT5_REPO} ${CC_MQTT5_SRC_DIR}
    fi

    echo "Building cc.mqtt5.generated library..."
    mkdir -p ${CC_MQTT5_BUILD_DIR}
    cmake -S ${CC_MQTT5_SRC_DIR} -B ${CC_MQTT5_BUILD_DIR} \
        -DCMAKE_INSTALL_PREFIX=${CC_MQTT5_INSTALL_DIR} -DCMAKE_BUILD_TYPE=${COMMON_BUILD_TYPE} \
        -DCMAKE_CXX_STANDARD=${COMMON_CXX_STANDARD} -DOPT_REQUIRE_COMMS_LIB=OFF
    cmake --build ${CC_MQTT5_BUILD_DIR} --config ${COMMON_BUILD_TYPE} --target install ${procs_param}
}

set -e
export VERBOSE=1
build_comms
build_mqtt5


