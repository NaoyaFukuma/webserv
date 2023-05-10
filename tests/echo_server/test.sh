#!/bin/bash

# run webserv
/app/webserv /app/tests/echo_server/config.txt &

sleep 1

# run client1
/app/tests/echo_server/client/client1
