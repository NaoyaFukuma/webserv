#!/bin/bash

echo "<><><><><><><><><><> docker-entrypoint.sh here <><><><><><><><><><>"
set -eux

pwd
ls ../
make
# shellcheck disable=SC2068
exec $@