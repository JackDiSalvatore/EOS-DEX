#!/usr/bin/env bash

docker run --rm -v $PWD:/eosio.contracts.exchange -w /eosio.contracts.exchange jackdisalvatore/eos1.8:v3 bash -c "mkdir -p build-docker && cd build-docker && cmake .. && make"  