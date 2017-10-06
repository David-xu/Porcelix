#!/bin/bash

OVS_INST=ovs-br0

ip link set $1 up
ovs-vsctl add-port ${OVS_INST} $1
