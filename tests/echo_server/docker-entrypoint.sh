#!/bin/bash

# compile webserv
make -C /app
# compile client
c++ /app/tests/echo_server/client/client1.cpp -o /app/tests/echo_server/client/client1

# shellcheck disable=SC2068
exec "$@"