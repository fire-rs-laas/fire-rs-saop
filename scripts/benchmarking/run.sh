#!/bin/bash

# start a docker container in the background
bash docker_start.sh

pwd
bash docker_exec.sh "make build-release"

bash docker_exec.sh "python3 python/fire_rs/planning/benchmark.py --vns-conf=./vns_confs.json --vns=base --elevation=flat"
bash docker_exec.sh "python3 python/fire_rs/planning/benchmark.py --vns-conf=./vns_confs.json --vns=full --elevation=flat"
bash docker_exec.sh "python3 python/fire_rs/planning/benchmark.py --vns-conf=./vns_confs.json --vns=with_smoothing --elevation=flat"

docker rm -f saop
