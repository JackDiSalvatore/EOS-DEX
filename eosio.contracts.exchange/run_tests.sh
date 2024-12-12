#!/usr/bin/env bash

docker run --rm -v $PWD:/eosio.contracts.exchange -w /eosio.contracts.exchange/build-docker jackdisalvatore/eos1.8:v3 ctest .  