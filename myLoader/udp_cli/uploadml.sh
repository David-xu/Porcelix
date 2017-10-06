#!/bin/bash

dd if=../../disk.img of=tmp.img bs=512 count=128
./pc_client 220.1.1.20 hdwrite hd0 tmp.img 0
