#!/bin/sh
set -e

python "$(dirname $0)/test_server.py" > test_server.log 2>&1 &
echo $! > test_server.pid
