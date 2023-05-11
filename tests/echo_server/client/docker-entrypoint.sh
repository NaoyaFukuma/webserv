#!/bin/bash

# compile client
c++ ./client.cpp -o ./client

exec "$@"
