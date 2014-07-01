#!/bin/sh
set -e

python "$(dirname $0)/test_server.py" > test_server.log 2>&1 &
saved_pid=$!
sleep 0.5
echo "${saved_pid}" > test_server.pid
