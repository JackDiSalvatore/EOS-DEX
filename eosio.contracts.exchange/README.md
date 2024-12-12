# token.exchange

## Prerequisites

* docker

## How To

```bash
# Clone this repo
git clone https://github.com/objectcomputing/eosio.contracts.exchange
cd eosio.contracts.exchange

# Run build
./build.sh

# Run tests
./run_tests.sh
```

## Run Local Node
To stand up a local node on `http://localhost:8888` with the following accounts:

- **eosio.token:** Runs the EOS token
- **tradertokens:** Runs the demo tokens USD and TKN
- **exchange5432:** Runs the exchange contract
- **trader1alice:** User with USD tokens
- **trader2barry:** User with EOS and TKN tokens

Run

```bash
./deploy_blockchain.sh
```

---

### About

Built with

eosio: v1.8.9  
eosio.contracts: v1.7.0  
eosio.cdt: v1.6.3  

`token_exchange_tests` unit tests are run using a mock eos implementation.  Because the eosio.cdt uses different class implementations then eos, the mock eos implementation must include class headers from eosio.cdt to compile.  These header files are located in `tests/eosio_contract_tests/mock_lib` and have been copied from eosio.cdt v1.6.3.

`token_exchange_action_tests` tests the exchange smart contract actions.  This test class is derivated from `eosio.system_tester.hpp` and therefore relies on the wasm and abi files for the eosio.system, eosio.token, and eosio.msig contracts.  The wasm and abi files have been copied from eosio.contracts v1.7.0 into the `tests/test_contracts` directory.
