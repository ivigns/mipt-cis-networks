#!/bin/bash

echo "=== SEND TCP ==="
iperf -c 172.20.1.0 -p 8080 -t 5
echo "=== SEND SSH ==="
sshpass -p "test" ssh -oStrictHostKeyChecking=no -i id_rsa test@172.20.1.0 \
    echo "successful ssh request"
echo "=== SEND ICMP ==="
ping 172.20.1.0 -c 5
echo "=== SEND UDP ==="
iperf -c 172.20.1.0 -u -p 8080 -t 5
echo "=== DONE ==="
