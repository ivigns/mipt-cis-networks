#!/bin/bash

# todo: tc

service ssh start
service ssh status
iperf -su -p 8080 &
iperf -s -p 8080
