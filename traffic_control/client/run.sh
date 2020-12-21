#!/bin/bash

iperf -seu -p 8080 &
iperf -se -p 8080 &

while true; do
    sshpass -p "test" ssh -oStrictHostKeyChecking=no -oConnectTimeout=5 \
    -i id_rsa test@172.20.1.0 echo "successful ssh request"
    ping 172.20.1.0 -c 5
done
