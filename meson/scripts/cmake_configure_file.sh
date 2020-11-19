#!/usr/bin/env bash

set -e

cmake_source_dir="${1}"
cmake_build_dir="${2}"
cmake_args="${@:5}"

configure_file_source="${3}"
configure_file_dest="${4}"

start="$(pwd)"
mkdir -p "${cmake_build_dir}" && cd "${cmake_build_dir}"
cmake ${cmake_args} -S "${cmake_source_dir}"
cd "${start}"

if [[ ! -f "${configure_file_source}" ]]; then
    echo "[ERR] $(basename ${0}): Generate $(basename ${configure_file_source}) failed." 1>&2
    exit 1
fi

cp "${configure_file_source}" "${configure_file_dest}"

