#!/bin/bash

pushd $(pwd)
cd ./umexec
make all
popd

pushd $(pwd)

cd ./myLoader

pushd $(pwd)
cd ./tools
make all
popd

pushd $(pwd)
cd ./udp_cli
make all
popd

make clean all install_iso

popd