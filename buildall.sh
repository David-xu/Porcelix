#!/bin/bash

pushd $(pwd)
cd ./umexec
make all
popd

pushd $(pwd)

cd ./myLoader

pushd $(pwd)
cd ./tools
make clean all
popd

pushd $(pwd)
cd ./udp_cli
make clean all
popd

make clean all install_iso

popd