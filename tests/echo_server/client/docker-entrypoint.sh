#!/bin/bash

# compile client
c++ /app/tests/echo_server/client/client.cpp -o /app/tests/echo_server/client/client

exec "$@"
