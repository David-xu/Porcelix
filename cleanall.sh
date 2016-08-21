#!/bin/bash

#rm -rf mkiso/
#rm *.iso

pushd $(pwd)
cd ./umexec
make clean
popd

pushd $(pwd)

cd ./myLoader

pushd $(pwd)
cd ./tools
make clean
popd

pushd $(pwd)
cd ./udp_cli
make clean
popd

make clean

popd