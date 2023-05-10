#!/bin/bash

# compile webserv
make -C /app
exec "$@"
