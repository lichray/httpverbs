#!/bin/sh
set -e

python test_server.py > test_server.log 2>&1 &
echo $! > test_server.pid
