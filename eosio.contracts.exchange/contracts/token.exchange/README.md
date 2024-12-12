# token.exchange

## Overview

This project intends to create a smart contract that facilitates an on-chain order book. Users can create markets between two different eosio assets.

## Design

**Deposits:**  
A user can deposit tokens he/she wishes to trade into the exchange contract.  Doing so creates a token balance on the exchange that can be used to place a buy or sell order.  Exchange balances are protected by the user's account (private key), limiting access of the token balance to only the user and the smart contract itself.

**Withdraw:**  
A user can withdraw any tokens from his/her exchange balance at any time.

**Market Creation:**  
In order to trade assets, a market must first be created.  Markets are created by defining a market pair.  A market pair consists of a **base** and a **quote** denoted as "*base:quote*".  For example EOS:USD (base = EOS, quote = USD).

**Place Order:**  
A user can place a buy/sell order to trade one asset for another at a specified price.

A **buy** order wishes to trade the quote for the base (ex: trade USD for EOS).  The buy price represents how much of the quote asset is needed to buy one unit of the base asset.

A **sell** order wishes to trade the base for the quote (ex: trade EOS for USD).  The sell price represents how much of the quote asset is received for selling one unit of base asset.

Once a trade is submitted, the counter order book (sell if buy, or buy if sell) is immediately checked and determines if a negative spread is available.  If a negative spread is found, the order matching algorithm is triggered and a trade is made.  The order book is updated and the trade results are subtracted/added to each parties exchange balance.

## Continuous Order Matching

Definitions:  
*best buy* = highest price buyer is willing to pay  
*best sell* = lowest price seller willing to offer  
*spread* = difference between best sell and best buy (best sell - best buy)  

Matching Algorithm:

```md
while spread <= 0:
    1. Take the best buy order and best sell order and generate a trade with the following properties:
        volume traded = the minimum volume between both the best buy and best sell orders
        if spread = 0:
            price = best buy price (same as best sell price)
        if spread < 0 (i.e. overlapping in the orderbook):
            price = price of the earlier order submitted
    2. Update the orderbook:
        if best buy volume == best sell volume:
            remove best buy and best sell orders from order book
        else:
            remove the order with the minimum volume (either best buy or best sell) from the orderbook, and update the volume of the other order
```

*example:*  
Base: EOS  
Quote: USD

Starting Order Book (only Sells)

New Sell: 500 EOS at 8.31 USD
New Sell: 700 EOS at 8.31 USD
New Sell: 500 EOS at 8.32 USD
New Sell: 500 EOS at 8.35 USD

| Time    | Buy/Sell | Price (USD) | Volume (EOS) |
|---------|----------|-------------|--------------|
| 7:33:01 | Sell     | 8.31        | 500          |
| 7:33:02 | Sell     | 8.31        | 700          |
| 7:33:03 | Sell     | 8.32        | 500          |
| 7:33:04 | Sell     | 8.35        | 500          |

---

1) Place Buy with no matching Sell orders

New Buy: 300 EOS at price 8.30 USD (8.30 USD = 1 EOS)

| Time    | Buy/Sell | Price (USD) | Volume (EOS) |                  |
|---------|----------|-------------|--------------|------------------|
| 7:33:01 | Sell     | 8.31        | 500          |                  |
| 7:33:02 | Sell     | 8.31        | 700          |                  |
| 7:33:03 | Sell     | 8.32        | 500          |                  |
| 7:33:04 | Sell     | 8.35        | 500          |                  |
| 7:34:00 | Buy      | 8.30        | 300          | **<- Buy (New)** |

Result: Buy is added to the order book

---

2) Place Buy that partial fills Sell order

New Buy: 100 EOS at price 8.31 USD

| Time    | Buy/Sell | Price (USD) | Volume (EOS) |                       |
|---------|----------|-------------|--------------|-----------------------|
| 7:33:01 | Sell     | 8.31        | 500          |  **<- Best Sell**     |
| 7:33:02 | Sell     | 8.31        | 700          |                       |
| 7:33:03 | Sell     | 8.32        | 500          |                       |
| 7:33:04 | Sell     | 8.35        | 500          |                       |
| 7:34:00 | Buy      | 8.30        | 300          |                       |
| 7:34:01 | Buy      | 8.31        | 100          | **<- Best Buy (New)** |

Result: Buy 100 EOS from the first best Sell order.  Update best Sell order volume and remove the Buy order from book.

| Time    | Buy/Sell | Price (USD) | Volume (EOS) |
|---------|----------|-------------|--------------|
| 7:33:01 | Sell     | 8.31        | 400          |
| 7:33:02 | Sell     | 8.31        | 700          |
| 7:33:03 | Sell     | 8.32        | 500          |
| 7:33:04 | Sell     | 8.35        | 500          |
| 7:34:00 | Buy      | 8.30        | 300          |

---

3) Place Buy that overlaps Sell orders

New Buy: 1000 EOS at price 8.32 USD

| Time    | Buy/Sell | Price (USD) | Volume (EOS) |                       |
|---------|----------|-------------|--------------|-----------------------|
| 7:33:01 | Sell     | 8.31        | 400          | **<- Best Sell**      |
| 7:33:02 | Sell     | 8.31        | 700          | **<- Next Best Sell** |
| 7:33:03 | Sell     | 8.32        | 500          |                       |
| 7:33:04 | Sell     | 8.35        | 500          |                       |
| 7:34:00 | Buy      | 8.30        | 300          |                       |
| 7:34:02 | Buy      | 8.32        | 1000         | **<- Best Buy**       |

Result: Buy entirety of first best Sell order (400 EOS) and 600 EOS of second best Sell order.  Then update the second best Sell order and the remove Buy order from book.

| Time    | Buy/Sell | Price (USD) | Volume (EOS) |
|---------|----------|-------------|--------------|
| 7:33:02 | Sell     | 8.31        | 100          |
| 7:33:03 | Sell     | 8.32        | 500          |
| 7:33:04 | Sell     | 8.35        | 500          |
| 7:34:00 | Buy      | 8.30        | 300          |

## Usage

**Contract Deployment:**  
Add the eosio@code permission to the exchange contract account to give the smart contract the authority to sign inline actions.

```bash
cleos set account permission <exchange-account> active --add-code
```

## Actions

**init:**  
Initialize RAM payer.

```bash
cleos push action exchange init '{"user_pays": true}' -p exchange@owner
```

**deposit:**  
To make a deposit, a user must transfers tokens from his/her account to the exchange account.

```bash
cleos push action eosio.token transfer '["alice","exchange","5.0000 EOS","memo optional"]' -p alice@active
```

**withdraw:**  
A user can withdraw his/her exchange balance at any time by calling the withdraw action.

```bash
cleos push action exchange withdraw '{"from":"alice","quantity":{"quantity":"5.0000 EOS","contract":"eosio.token"}}' -p alice@active
```

**createmarket:**  
Creates a trading market for the quote currency.

- **owner**: RAM payer if end users are configured to pay for RAM
- **quote**: quote currency

```bash
cleos push action exchange createmarket '{"owner":"alice","quote":{"quantity":"0.00 USD","contract":"usd.token"}}' -p alice@active
```

**removemarket:**  
Removes a trading market for the quote currency.

- **quote**: quote currency

```bash
cleos push action exchange removemarket '{"quote":{"quantity":"0.00 USD","contract":"usd.token"}}' -p alice@active
```

**addpair:**  
Pairs base currency with quote currency market.

- **owner**: RAM payer if end users are configured to pay for RAM
- **quote**: extened asset defining: the quote symbol, percision, and contract account
- **base**: extened asset defining: the base symbol, percision, and contract account

```bash
cleos push action exchange addpair '{"owner":"alice","quote":"{"quantity":"0.00 USD","contract":"usd.token"}","base":"{"quantity":"0.0000 EOS","contract":"eosio.token"}"}' -p alice@active
```

**removepair:**  
Pairs base currency with quote currency market.

- **quote**: extened asset defining: the quote symbol, percision, and contract account
- **base**: extened asset defining: the base symbol, percision, and contract account

```bash
cleos push action exchange removepair '{"quote":"{"quantity":"0.00 USD","contract":"usd.token"}","base":"{"quantity":"0.0000 EOS","contract":"eosio.token"}"}' -p alice@active
```

**trade:**  
A user can place a sell or buy order with their exchange balance.

- **trader**: trader account name
- **order_type**: 0 = sell, 1 = buy
- **price**: base price
- **volume**: quote volume
- **auto_withdraw**: 0 = limit order, 1 = make trade an atomic swap market order

sell:

```bash
cleos push action exchange trade '{"trader":"alice","order_type":"0","price":"{"quantity":"8.32 USD","contract":"usd.token"}","volume":"{"quantity":"100.0000 EOS","contract":"eosio.token"}","auto_withdraw":"0"}' -p alice@active
```

buy:

```bash
cleos push action exchange trade '{"trader":"alice","order_type":"1","price":"{"quantity":"8.32 USD","contract":"usd.token"}","volume":"{"quantity":"100.0000 EOS","contract":"eosio.token"}","auto_withdraw":"0"}' -p alice@active
```

**cancel:**  
A user can cancel an open order.

- **base**: base asset in market pair
- **quote**: quote asset in market pair
- **trader**: trader account name
- **order_type**: 0 = sell, 1 = buy
- **id**: order id

cancel sell order id #1:

```bash
cleos push action exchange cancel '{"base":"{"quantity":"0.0000 EOS","contract":"eosio.token"}","quote":"{"quantity":"0.00 USD","contract":"usd.token"}","trader":"alice","order_type":"0","id":"1"}' -p alice@active
```

## Singletons

**config**  
Scoped to contract

- **user_pays:** boolean designating if the end user will pay for RAM
- **is_initialized:** boolean stating if this singleton has been set

## Tables

**exaccounts:**  
Scoped to account owner

- **account_id**: unique identify used because uin128_t's are not supported as primary keys.  The majority of the time, exaccounts will be indexed by the balance
- **balance**: extended asset representing token symbol, contract account, and amount. Secondary key = combination of token symbol and contract account

**markets:**  
Scoped to contract.

Displays all market's and pairs available

- **market_name**: market name
- **quote**: quote asset
- **bases**: map of all base assets paired with quote

**stats**  
Scoped to contract.

Contains price information about a certain market pair

- **market_name**: market pair name
- **price**: base asset price in terms of the quote

**bidorders:**  
Scoped to market name (ie. "eosusd")

Sell Orders, ordered from lowest price to highest

- **id**: unique trade id
- **trader**: account making the trade
- **timestamp**: time stamp of trade
- **price**: base price
- **volume**: quote volume

**askorders:**  
Scoped to market name (ie. "eosusd")

Buy Orders, ordered from highest price to lowest

- **id**: unique trade id
- **trader**: account making the trade
- **timestamp**: time stamp of trade
- **price**: base price
- **volume**: quote volume

---

Built with

eosio: v1.8.4

eosio.cdt: version 1.6.3

To build

```bash
mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Debug # Debug or Release
```

To run tests

```bash
./build/tests/eosio_contract_tests/token_exchange_action_tests
./build/tests/eosio_contract_tests/token_exchange_tests
```
