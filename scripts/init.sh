#!/bin/bash

ip link add dev vcan0 type vcan
ip link set vcan0 mtu 72
ip link set up vcan0
