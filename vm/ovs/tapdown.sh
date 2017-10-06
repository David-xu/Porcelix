#!/bin/bash

OVS_INST=ovs-br0

ip link set $1 down
ovs-vsctl del-port ${OVS_INST} $1
