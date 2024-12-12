#!/usr/bin/env bash

SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo 'Cleaning up last run'
pkill nodeos
pkill keosd
rm -rf ${SOURCE_DIR}/blockchain

# Make Data Directory
mkdir -p "${SOURCE_DIR}/blockchain/data"

curl --output /dev/null \
    --silent \
    --head \
    --fail \
    localhost:8888

retval=$?
if [ $retval == 0 ]; then
    echo "You have a process already using port 8888, which is preventing nodeos from starting"
    exit 1
fi

function start_nodeos {
    nodeos -e -p eosio -d "${SOURCE_DIR}/blockchain/data" \
    --config-dir "${SOURCE_DIR}/blockchain/data/config" \
    --delete-all-blocks \
    --http-validate-host=false \
    --plugin eosio::producer_api_plugin \
    --plugin eosio::chain_api_plugin \
    --plugin eosio::http_plugin \
    --http-server-address=0.0.0.0:8888 \
    --access-control-allow-origin=* \
    --access-control-allow-headers=* \
    --max-transaction-time=1000 \
    --contracts-console \
    --verbose-http-errors > "${SOURCE_DIR}/blockchain/data/nodeos.log"

    until $(curl --output /dev/null \
                --silent \
                --head \
                --fail \
                localhost:8888/v1/chain/get_info)
    do
        echo "Waiting for EOSIO blockchain to be started..."
        sleep 2s
    done

    echo "EOSIO Blockchain Started"
}

${SOURCE_DIR}/run_chain.sh &
start_nodeos
