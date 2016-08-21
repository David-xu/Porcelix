#!/bin/bash

pushd $(pwd)
cd ./umexec
make clean
popd

pushd $(pwd)
cd ./myLoader
make clean

pushd $(pwd)
cd ./tools
make clean
popd

pushd $(pwd)
cd ./udp_cli
make clean
popd

popd