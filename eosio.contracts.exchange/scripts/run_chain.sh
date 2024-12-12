#!/usr/bin/env bash

# Account Info
OWNER_PUBLIC_KEY=EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
OWNER_PRIVATE_KEY=5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3

ALICE_ACCOUNT=trader1alice
BARRY_ACCOUNT=trader2barry
EXCHANGE_ACCOUNT=exchange5432
FIAT_ACCOUNT=fiatcryptos1
TOKEN_ACCOUNT=tradertokens

ALICE_PRIVATE_KEY=5JLL42ShsECocztJmQReVmWPk9MmgXMkun9DgQQokY6nk5mrXJZ
ALICE_PUBLIC_KEY=EOS6WDqtfqmFE6k4qtHAr5xDZZX4BwgrKbpHXg7zTc6PTwAPjtReQ

BARRY_PRIVATE_KEY=5JLL42ShsECocztJmQReVmWPk9MmgXMkun9DgQQokY6nk5mrXJZ
BARRY_PUBLIC_KEY=EOS6WDqtfqmFE6k4qtHAr5xDZZX4BwgrKbpHXg7zTc6PTwAPjtReQ

set -e

SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

BOOT_ADDR=${BOOT_ADDR:-127.0.0.1:8888}

boot_url=http://${BOOT_ADDR}
wallet_host=127.0.0.1:8899
wdurl=http://${wallet_host}
ecmd="cleos --wallet-url ${wdurl} --url ${boot_url}"
wcmd="cleos --wallet-url ${wdurl} wallet"


function wait_boot_node_ready {
    for (( i=0 ; i<10; i++ )); do
      ! $ecmd get info || break
      sleep 3
    done
}

function wait_wallet_ready() {
    for (( i=0 ; i<10; i++ )); do
      ! $wcmd list 2>/tmp/wallet.txt || [ -s /tmp/wallets.txt ] || break
      sleep 3
    done
}

function create_private_key {
    # Create required system accounts
    readarray syskey <<< $(cleos create key --to-console)
    echo ${syskey[0]#"Private key: "} | xargs ## xargs is usd to remove leading and trailing whitespaces
}

function import_private_key {
    local privkey=$1
    $wcmd import -n ignition --private-key $privkey | sed 's/[^:]*: //'
}

function wait_account_ready {
    local account_name=$1
    local resource=$2
    while ! $ecmd get account $account_name "4,SYS" | grep "$resource"; do
      sleep 3
    done
}

function account_exists {
    local account_name=$1
    $ecmd get account $account_name "4,SYS"
}

function setup_wallet {
    rm -rf ${HOME}/eosio-wallet
    keosd --http-server-address ${wallet_host} &
    wait_wallet_ready
    $wcmd create --to-console -n ignition
}

function is_contract_set {
    local contract_name=$1
    local code_hash=`$ecmd get code $contract_name` || return 1
    # contrat is set only when the returned code hash is not all zeros
    [ -z "$(echo $code_hash | grep 'code hash: 0\+$')" ]
}

function try_till_success {
    max_retry=10
    for (( i=0 ; i<$max_retry; i++ )); do
      ! "$@" || return 0
      sleep 10
    done
    return 1
}

function set_contract {
    try_till_success $ecmd set contract "$@"
}

function split_string {
    local data=$1
    local delimiter=$2
    readarray -td$delimiter result <<<"$data"
    declare -g result
}

function kms_decrypt {
    ## Give the input in the form "gcp-kms:projects/[PROJECT]/locations/[LOCATION]/keyRings/[KEYRING]/cryptoKeys/[KEY]:[CYPHERTEXT]"
    ## It decrypts the cyphertext with GCP KMS and output the text. If the input does not conform the input pattern, it simply echo the input.
    DATA=$1
    ## if $DATA does not starts with "gcp-kms" return the original data
    if [[ $DATA == gcp-kms* ]]; then
        split_string "$DATA" :
        resource_id=${result[1]}
        ciphertext=${result[2]}
        split_string "$resource_id" /
        project=${result[1]}
        location=${result[3]}
        keyring=${result[5]}
        key=${result[7]}
        echo $ciphertext | base64 -d | gcloud kms decrypt --ciphertext-file=- --plaintext-file=- --key=$key --keyring=$keyring --location=$location --project=$project
    else
        echo $DATA
    fi
}

SYSTEM_PRIVKEY=$(kms_decrypt ${SYSTEM_PRIVKEY})
contracts_dir=/eos/build/unittests/contracts
exchange_contract_dir=${SOURCE_DIR}/../build-docker/contracts/token.exchange

function activate_pre_activate_feature {
    curl -s -X POST http://${BOOT_ADDR}/v1/producer/schedule_protocol_feature_activations -d '{"protocol_features_to_activate": ["0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd"]}'
}

function activate_feature {
  if ! MSG=$($ecmd push action eosio activate "[\"$1\"]" -p eosio 2>&1); then
    echo $MSG | grep "is already activated" > /dev/null
  fi
}

function create_account {
  if ! MSG=$($ecmd create account eosio $1 eosio@owner eosio@active 2>&1); then
    echo $MSG | grep "name is already taken" > /dev/null
  fi
}

function create_account_with_key {
  if ! MSG=$($ecmd create account eosio $1 $2 $2 2>&1); then
    echo $MSG | grep "name is already taken" > /dev/null
  fi
}

function set_eosio_code_permission {
  $ecmd set account permission $1 active --add-code
}

function setup_eosio {
    activate_pre_activate_feature
    sleep 1
    until activate_pre_activate_feature | grep "has already been activated"; do
      sleep 1
    done
    set +x
    # local pubsyskey=$(import_private_key $SYSTEM_PRIVKEY)
    import_private_key $ALICE_PRIVATE_KEY
    import_private_key $BARRY_PRIVATE_KEY
    local pubsyskey=$(import_private_key $OWNER_PRIVATE_KEY)
    set -x
    set_contract eosio $contracts_dir/eosio.bios
    activate_feature f0af56d2c5a48d60a4a5b5c903edfb7db3a736a94ed589d0b797df33ff9d3e1d
    activate_feature 2652f5f96006294109b3dd0bbde63693f55324af452b799ee137a81a905eed25
    activate_feature 8ba52fe7a3956c5cd3a656a3174b931d3bb2abb45578befc59f283ecd816a405
    activate_feature ad9e3d8f650687709fd68f4b90b41f7d825a365b02c23a636cef88ac2ac00c43
    activate_feature 68dcaa34c0517d19666e6b33add67351d8c5f69e999ca1e37931bc410a297428
    activate_feature e0fb64b1085cc5538970158d05a009c24e276fb94e1a0bf6a528b48fbc4ff526
    activate_feature ef43112c6543b88db2283a2e077278c315ae2c84719a8b25f25cc88565fbea99
    activate_feature 4a90c00d55454dc5b059055ca213579c6ea856967712a56017487886a4d4cc0f
    activate_feature 1a99a59d87e06e09ec5b028a9cbb7749b4a5ad8819004365d02dc4379a8b7241
    activate_feature 4e7bf348da00a945489b2a681749eb56f5de00b900014e137ddae39f48f69d67
    create_account eosio.bpay
    create_account eosio.msig
    create_account eosio.names
    create_account eosio.ram
    create_account eosio.ramfee
    create_account eosio.saving
    create_account eosio.stake
    create_account eosio.token
    create_account eosio.vpay
    create_account eosio.wrap
    create_account eosio.rex
    $ecmd push action eosio setpriv '{"account":"eosio.msig","is_priv": 1}' -p eosio
    set_contract eosio.token $contracts_dir/eosio.token
    set_contract eosio.msig  $contracts_dir/eosio.msig
    set_contract eosio.wrap  $contracts_dir/eosio.wrap
    initial_issue="1000000000.0000 EOS"
    system_token_symbol="4,EOS"
    $ecmd push action eosio.token create "[\"eosio\",\"$initial_issue\"]" -p eosio.token
    $ecmd push action eosio.token issue "[\"eosio\",\"$initial_issue\",\"memo\"]" -p eosio
    # For some reason set system contract is timming out
    # set_contract eosio $contracts_dir/eosio.system
    # $ecmd push action eosio init "{\"version\":\"0\",\"core\":\"$system_token_symbol\"}" -p eosio
}


# ACCOUNT FUNCTIONS

function setup_user_accounts {
    create_account_with_key ${ALICE_ACCOUNT} ${ALICE_PUBLIC_KEY}
    create_account_with_key ${BARRY_ACCOUNT} ${BARRY_PUBLIC_KEY}

    send_tokens ${FIAT_ACCOUNT} ${FIAT_ACCOUNT} ${ALICE_ACCOUNT} "10000.00 USD"
    send_tokens ${FIAT_ACCOUNT} ${FIAT_ACCOUNT} ${ALICE_ACCOUNT} "10000.00 EUR"
    send_tokens ${FIAT_ACCOUNT} ${FIAT_ACCOUNT} ${ALICE_ACCOUNT} "10000 JPY"

    send_tokens ${TOKEN_ACCOUNT} ${TOKEN_ACCOUNT} ${BARRY_ACCOUNT} "10000.00000000 BTC"
    send_tokens ${TOKEN_ACCOUNT} ${TOKEN_ACCOUNT} ${BARRY_ACCOUNT} "10000.00000000 ETH"
    send_tokens ${TOKEN_ACCOUNT} ${TOKEN_ACCOUNT} ${BARRY_ACCOUNT} "10000.00000000 LTC"
    send_tokens eosio.token eosio ${BARRY_ACCOUNT} "10000.0000 EOS"
}

function barry_seed_sells {
    send_tokens "eosio.token" ${BARRY_ACCOUNT} ${EXCHANGE_ACCOUNT} "1000.0000 EOS"
    # deposit ${FIAT_ACCOUNT} ${BARRY_ACCOUNT} "1000.00000000 BTC"
    # deposit ${FIAT_ACCOUNT} ${BARRY_ACCOUNT} "1000.00000000 ETH"
    # deposit ${FIAT_ACCOUNT} ${BARRY_ACCOUNT} "1000.00000000 LTC"

    sell ${BARRY_ACCOUNT} ${FIAT_ACCOUNT} "2.80 USD" "eosio.token" "10.0000 EOS"
    sell ${BARRY_ACCOUNT} ${FIAT_ACCOUNT} "2.81 USD" "eosio.token" "3.0000 EOS"
    sell ${BARRY_ACCOUNT} ${FIAT_ACCOUNT} "2.82 USD" "eosio.token" "1.0000 EOS"
    sell ${BARRY_ACCOUNT} ${FIAT_ACCOUNT} "2.83 USD" "eosio.token" "2.5000 EOS"
    sell ${BARRY_ACCOUNT} ${FIAT_ACCOUNT} "2.84 USD" "eosio.token" "8.5000 EOS"
    sell ${BARRY_ACCOUNT} ${FIAT_ACCOUNT} "2.85 USD" "eosio.token" "12.9500 EOS"
    sell ${BARRY_ACCOUNT} ${FIAT_ACCOUNT} "2.86 USD" "eosio.token" "5.0000 EOS"
    sell ${BARRY_ACCOUNT} ${FIAT_ACCOUNT} "2.87 USD" "eosio.token" "2.2500 EOS"
    sell ${BARRY_ACCOUNT} ${FIAT_ACCOUNT} "2.88 USD" "eosio.token" "4.0000 EOS"
    sell ${BARRY_ACCOUNT} ${FIAT_ACCOUNT} "2.89 USD" "eosio.token" "2.5500 EOS"
}

function alice_seed_buys {
    send_tokens ${FIAT_ACCOUNT} ${ALICE_ACCOUNT} ${EXCHANGE_ACCOUNT} "1000.00 USD"
    # deposit ${FIAT_ACCOUNT} ${ALICE_ACCOUNT} "1000.00 EUR"
    # deposit ${FIAT_ACCOUNT} ${ALICE_ACCOUNT} "1000 JPY"

    buy ${ALICE_ACCOUNT} ${FIAT_ACCOUNT} "2.56 USD" "eosio.token" "1.0000 EOS"
    buy ${ALICE_ACCOUNT} ${FIAT_ACCOUNT} "2.57 USD" "eosio.token" "0.5500 EOS"
    buy ${ALICE_ACCOUNT} ${FIAT_ACCOUNT} "2.58 USD" "eosio.token" "1.3500 EOS"
    buy ${ALICE_ACCOUNT} ${FIAT_ACCOUNT} "2.59 USD" "eosio.token" "10.2500 EOS"
    buy ${ALICE_ACCOUNT} ${FIAT_ACCOUNT} "2.60 USD" "eosio.token" "1.6600 EOS"
    buy ${ALICE_ACCOUNT} ${FIAT_ACCOUNT} "2.61 USD" "eosio.token" "2.5000 EOS"
    buy ${ALICE_ACCOUNT} ${FIAT_ACCOUNT} "2.62 USD" "eosio.token" "0.8000 EOS"
    buy ${ALICE_ACCOUNT} ${FIAT_ACCOUNT} "2.63 USD" "eosio.token" "9.8500 EOS"
    buy ${ALICE_ACCOUNT} ${FIAT_ACCOUNT} "2.64 USD" "eosio.token" "1.9500 EOS"
    buy ${ALICE_ACCOUNT} ${FIAT_ACCOUNT} "2.65 USD" "eosio.token" "10.0000 EOS"
}


# TOKEN FUNCTIONS

function send_tokens {
    local contract_account=$1
    local from=$2
    local to=$3
    local quantity=$4
    $ecmd push action $contract_account transfer "[\"$from\",\"$to\",\"$quantity\",\"memo\"]" -p $from
}

function setup_fiat_tokens {
    create_account ${FIAT_ACCOUNT}
    set_contract ${FIAT_ACCOUNT} $contracts_dir/eosio.token

    $ecmd push action ${FIAT_ACCOUNT} create "[\"${FIAT_ACCOUNT}\",\"1000000000.00 USD\"]" -p ${FIAT_ACCOUNT}
    $ecmd push action ${FIAT_ACCOUNT} issue "[\"${FIAT_ACCOUNT}\",\"1000000000.00 USD\",\"memo\"]" -p ${FIAT_ACCOUNT}

    $ecmd push action ${FIAT_ACCOUNT} create "[\"${FIAT_ACCOUNT}\",\"1000000000.00 EUR\"]" -p ${FIAT_ACCOUNT}
    $ecmd push action ${FIAT_ACCOUNT} issue "[\"${FIAT_ACCOUNT}\",\"1000000000.00 EUR\",\"memo\"]" -p ${FIAT_ACCOUNT}

    $ecmd push action ${FIAT_ACCOUNT} create "[\"${FIAT_ACCOUNT}\",\"1000000000 JPY\"]" -p ${FIAT_ACCOUNT}
    $ecmd push action ${FIAT_ACCOUNT} issue "[\"${FIAT_ACCOUNT}\",\"1000000000 JPY\",\"memo\"]" -p ${FIAT_ACCOUNT}
}

function setup_crypto_tokens {
    create_account ${TOKEN_ACCOUNT}
    set_contract ${TOKEN_ACCOUNT} $contracts_dir/eosio.token

    $ecmd push action ${TOKEN_ACCOUNT} create "[\"${TOKEN_ACCOUNT}\",\"21000000.00000000 BTC\"]" -p ${TOKEN_ACCOUNT}
    $ecmd push action ${TOKEN_ACCOUNT} issue "[\"${TOKEN_ACCOUNT}\",\"21000000.00000000 BTC\",\"memo\"]" -p ${TOKEN_ACCOUNT}

    $ecmd push action ${TOKEN_ACCOUNT} create "[\"${TOKEN_ACCOUNT}\",\"1000000000.00000000 ETH\"]" -p ${TOKEN_ACCOUNT}
    $ecmd push action ${TOKEN_ACCOUNT} issue "[\"${TOKEN_ACCOUNT}\",\"1000000000.00000000 ETH\",\"memo\"]" -p ${TOKEN_ACCOUNT}

    $ecmd push action ${TOKEN_ACCOUNT} create "[\"${TOKEN_ACCOUNT}\",\"84000000.00000000 LTC\"]" -p ${TOKEN_ACCOUNT}
    $ecmd push action ${TOKEN_ACCOUNT} issue "[\"${TOKEN_ACCOUNT}\",\"84000000.00000000 LTC\",\"memo\"]" -p ${TOKEN_ACCOUNT}
}


# EXCHANGE FUNCTIONS

function initialize_contract_ram_payer {
    local user_pays=$1
    $ecmd push action ${EXCHANGE_ACCOUNT} init "{\"user_pays\": $user_pays}" -p ${EXCHANGE_ACCOUNT}@owner
}

function sell {
    local trader=$1
    local price_contract_account=$2
    local price_quantity=$3
    local base_contract_account=$4
    local volume_quantity=$5

    $ecmd push action ${EXCHANGE_ACCOUNT} trade "{\"trader\": \"$trader\",\"order_type\": \"0\",\"price\": {\"quantity\": \"$price_quantity\",\"contract\": \"$price_contract_account\"},\"volume\": {\"quantity\": \"$volume_quantity\",\"contract\": \"$base_contract_account\"},\"auto_withdraw\": \"0\"}" -p $trader
}

function buy {
    local trader=$1
    local price_contract_account=$2
    local price_quantity=$3
    local base_contract_account=$4
    local volume_quantity=$5

    $ecmd push action ${EXCHANGE_ACCOUNT} trade "{\"trader\": \"$trader\",\"order_type\": \"1\",\"price\": {\"quantity\": \"$price_quantity\",\"contract\": \"$price_contract_account\"},\"volume\": {\"quantity\": \"$volume_quantity\",\"contract\": \"$base_contract_account\"},\"auto_withdraw\": \"0\"}" -p $trader
}

function create_market {
    local contract_account=$1
    local quantity=$2
    $ecmd push action ${EXCHANGE_ACCOUNT} createmarket "{\"owner\":\"${EXCHANGE_ACCOUNT}\",\"quote\":{\"contract\":\"$contract_account\",\"quantity\":\"$quantity\"}}" -p ${EXCHANGE_ACCOUNT}
}

function add_market_pair {
    local base_contract_account=$1
    local base_quantity=$2
    local quote_contract_account=$3
    local quote_quantity=$4
    $ecmd push action ${EXCHANGE_ACCOUNT} addpair "{\"owner\":\"${EXCHANGE_ACCOUNT}\",\"base\":{\"contract\":\"$base_contract_account\",\"quantity\":\"$base_quantity\"},\"quote\":{\"contract\":\"$quote_contract_account\",\"quantity\":\"$quote_quantity\"}}" -p ${EXCHANGE_ACCOUNT}
}

function setup_exchange {
    create_account ${EXCHANGE_ACCOUNT}
    set_contract ${EXCHANGE_ACCOUNT} $exchange_contract_dir
    set_eosio_code_permission ${EXCHANGE_ACCOUNT}
    initialize_contract_ram_payer false

    # EOS:USD
    create_market ${FIAT_ACCOUNT} "0.00 USD"
    add_market_pair "eosio.token" "0.0000 EOS" ${FIAT_ACCOUNT} "0.00 USD"
    add_market_pair ${TOKEN_ACCOUNT} "0.00000000 BTC" ${FIAT_ACCOUNT} "0.00 USD"
    add_market_pair ${TOKEN_ACCOUNT} "0.00000000 ETH" ${FIAT_ACCOUNT} "0.00 USD"
    add_market_pair ${TOKEN_ACCOUNT} "0.00000000 LTC" ${FIAT_ACCOUNT} "0.00 USD"

    # EOS:EUR
    create_market ${FIAT_ACCOUNT} "0.00 EUR"
    add_market_pair "eosio.token" "0.0000 EOS" ${FIAT_ACCOUNT} "0.00 EUR"

    # EOS:JPY
    create_market ${FIAT_ACCOUNT} "0 JPY"
    add_market_pair "eosio.token" "0.0000 EOS" ${FIAT_ACCOUNT} "0 JPY"
}


function bootstrap {
    setup_wallet
    wait_boot_node_ready

    setup_eosio

    setup_fiat_tokens
    setup_crypto_tokens
    setup_user_accounts

    setup_exchange
    barry_seed_sells
    alice_seed_buys
}

bootstrap
