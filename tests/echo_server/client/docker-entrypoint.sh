#!/bin/bash

c++ ./client.cpp -o ./client.out

sleep 10

exec "$@"
