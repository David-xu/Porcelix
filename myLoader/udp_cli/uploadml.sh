#!/bin/bash

dd if=../../img/sys.img of=tmp.img bs=512 count=128
./pc_client 192.168.1.20 sendfile hd1 tmp.img 0
