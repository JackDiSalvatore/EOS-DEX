#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <cmath>
#include <cstdint>
#include <fc/time.hpp>

#include "mock_eosiolib.hpp"
#include <token.exchange_base.cpp>
#include <numeric>

using namespace tokenexchange;

struct exchange_base_mock : eosio::enable_multi_index, tokenexchange::exchange_base {

   exchange_base_mock(eosio::name code) 
   : eosio::enable_multi_index(code)
   , tokenexchange::exchange_base(code) {} 

};

TEST_CASE("test") {
   exchange_base_mock exchange{name("exchange")};

   // create exchange balance for alice
   exaccounts exchange_accounts( name("exchange"), name("alice").value );

    auto exchange_accounts_by_balance = exchange_accounts.get_index<"bybalance"_n>();

   exchange_accounts.emplace(name("exchange"), [&](auto& ex) {
       ex.account_id = exchange_accounts.available_primary_key();
       ex.balance = extended_asset( asset(100000, symbol("EOS",4)), name("eosio.token") );
   });

   auto useraccount = exchange_accounts_by_balance.find( get_token_key(name("eosio.token"), symbol("EOS",4)) );
   CHECK(useraccount != exchange_accounts_by_balance.end());
   CHECK(useraccount->balance.quantity.amount == 100000);
}

TEST_CASE("init_contract") {

   GIVEN("smart contract RAM payer not configured yet") {
      exchange_base_mock exchange{name("exchange")};
      configuration contract_config( name("exchange"), name("exchange").value );
      // CHECK(contract_config.exists());

      WHEN("contract is initialized to require the user to pay for RAM") {
         exchange.init_contract(true);

         THEN("the user will pay for RAM") {

            CHECK(contract_config.get().user_pays == true);
            CHECK(contract_config.get().is_initialized == true);
         }
         AND_WHEN("the contract is initialized again") {
            THEN("it will fail") {
               CHECK_THROWS_WITH(exchange.init_contract(false), "contract already initialized");
            }
         }
      }

      WHEN("contract is initialized to require the contract account to pay for RAM") {
         exchange.init_contract(false);

         THEN("the exchange will pay for RAM") {

            CHECK(contract_config.get().user_pays == false);
            CHECK(contract_config.get().is_initialized == true);
         }
         AND_WHEN("the contract is initialized again") {
            THEN("it will fail") {
               CHECK_THROWS_WITH(exchange.init_contract(false), "contract already initialized");
            }
         }
      }
   }

}

TEST_CASE("adjust_balance") {

   GIVEN("alice does not have an exchange balance") {
      exchange_base_mock exchange{name("exchange")};
      exchange.init_contract(false);

      WHEN("adjust_balance is called for alice with a positive delta") {
         name owner = name("alice");
         extended_asset delta = extended_asset( asset(1, symbol("EOS",4)), name("eosio.token") );

         exchange.adjust_balance(owner, delta);

         THEN("alice will have a positive exchange balance") {
            exaccounts exchange_accounts( name("exchange"), owner.value );
            auto exchange_accounts_by_balance = exchange_accounts.get_index<"bybalance"_n>();
            auto alice_ex_balance = exchange_accounts_by_balance.find( get_token_key(name("eosio.token"), symbol("EOS",4)) );

            CHECK(alice_ex_balance->balance.quantity.amount == delta.quantity.amount);

            AND_WHEN("adjust_balance is called again for alice with a positive delta") {
               exchange.adjust_balance(owner, delta);

               THEN("alices' exchange balance will increase") {
                  auto alice_ex_balance = exchange_accounts_by_balance.find( get_token_key(name("eosio.token"), symbol("EOS",4)) );

                  CHECK(alice_ex_balance->balance.quantity.amount == delta.quantity.amount * 2);

                  AND_WHEN("adjust_balance is called again with a negative delta that will not overdraw alices balance") {
                     exchange.adjust_balance(owner, -delta);

                     THEN("alices' balance will be decreased") {
                        auto alice_ex_balance = exchange_accounts_by_balance.find( get_token_key(name("eosio.token"), symbol("EOS",4)) );

                        CHECK(alice_ex_balance->balance.quantity.amount == delta.quantity.amount);

                        AND_WHEN("adjust_balance is called a final time with a negative balance that will overdraw alices balance") {
                           delta = extended_asset( asset(2, symbol("EOS",4)), name("eosio.token") );

                           CHECK_THROWS_WITH(exchange.adjust_balance(owner, -delta), "exchange balance overdrawn");
                        }
                     }
                  }
               }
            }
         }
      }

      WHEN("adjust_blance is called the first time for alice with a negative delta") {
         name owner = name("alice");
         extended_asset delta = extended_asset( asset(-1, symbol("EOS",4)), name("eosio.token") );

         CHECK_THROWS_WITH(exchange.adjust_balance(owner, delta), "exchange balance overdrawn");
      }
   }

}

TEST_CASE("close_account") {
   exchange_base_mock exchange{name("exchange")};
   name alice = name("alice");
   name bob   = name("bob");

   exchange.init_contract(false);

   GIVEN("alice has 0 USD in her exchange balance") {
      extended_asset deposit_USD = exchange.normalize_precision(extended_asset(asset(0, symbol("USD",2)), name("usd.token")));
      exchange.adjust_balance(alice, deposit_USD);

      WHEN("alice closes her USD account") {
         exchange.close_account(alice, name("usd.token"), symbol("USD",8));

         THEN("alices' USD balance will be closed") {
            CHECK_THROWS_WITH(exchange.close_account(alice, name("usd.token"), symbol("USD",8)), "balance row already deleted or never existed");
         }
      }

      WHEN("bob closes his non-existent USD account") {
         CHECK_THROWS_WITH(exchange.close_account(bob, name("usd.token"), symbol("USD",8)), "balance row already deleted or never existed");
      }
   }

   GIVEN("alice has 100 USD in her exchange balance") {
      extended_asset deposit_USD = exchange.normalize_precision(extended_asset(asset(10000, symbol("USD",2)), name("usd.token")));
      exchange.adjust_balance(alice, deposit_USD);

      WHEN("alice closes her USD account") {
         CHECK_THROWS_WITH(exchange.close_account(alice, name("usd.token"), symbol("USD",8)), "cannot close because the balance is not zero");
      }
   }

}

TEST_CASE("normalize_precision") {
   exchange_base_mock exchange{name("exchange")};

   // 100 JPY => 100.00000000 JPY
   CHECK(exchange.normalize_precision(
      extended_asset(asset(100, symbol("JPY",0)), name("jpy.token"))).quantity.amount
      ==
      extended_asset(asset(10000000000, symbol("JPY", 8)), name("jpy.token")).quantity.amount
   );

   // 10.1234 EOS => 10.12340000 EOS
   CHECK(exchange.normalize_precision(
      extended_asset(asset(101234, symbol("EOS",4)), name("eosio.token"))).quantity.amount
      ==
      extended_asset(asset(1012340000, symbol("EOS", 8)), name("eosio.token")).quantity.amount
   );

   // 100.12345678 BTC => 100.1234568 BTC
   CHECK(exchange.normalize_precision(
      extended_asset(asset(10012345678, symbol("BTC",8)), name("btc.token"))).quantity.amount
      ==
      extended_asset(asset(10012345678, symbol("EOS",8)), name("btc.token")).quantity.amount
   );

   // 100.123456789 throws "only supports precision up to 8 decimals"
   CHECK_THROWS_WITH(
      exchange.normalize_precision( extended_asset(asset(100123456789, symbol("BTC",9)), name("btc.token")) ),
      "only supports precision up to 8 decimals"
   );

   // -1 JPY => -1.00000000 USD
   CHECK(exchange.normalize_precision(
      extended_asset(-asset(1, symbol("JPY",0)), name("jpy.token"))).quantity.amount
      ==
      extended_asset(-asset(100000000, symbol("JPY", 8)), name("jpy.token")).quantity.amount
   );
}

TEST_CASE("create_market_name") {
   exchange_base_mock exchange{name("exchange")};
   extended_asset USD     = extended_asset(asset(0,     symbol("USD",2)), name("usd.token"));
   extended_asset EOS     = extended_asset(asset(0,     symbol("EOS",4)), name("eosio.token"));
   extended_asset ABCDEFG  = extended_asset(asset(0,  symbol("ABCDEFG",8)), name("eosio.token"));

   CHECK(exchange.create_market_name(USD) == name("usd"));
   CHECK(exchange.create_market_name(EOS) == name("eos"));
   CHECK(exchange.create_market_name(ABCDEFG) == name("abcdefg"));
}

TEST_CASE("create_market") {

   GIVEN("EOS market does not exist") {
      exchange_base_mock exchange{name("exchange")};
      extended_asset EOS = extended_asset(asset(0, symbol("EOS",4)), name("eosio.token"));

      markets exchange_markets( name("exchange"), name("exchange").value );

      exchange.init_contract(false);

      WHEN("create_market(EOS) is called") {
         exchange.create_market(name("exchange"), EOS);

         THEN("the EOS market is created") {
            auto eos_market = exchange_markets.find(name("eos").value);

            CHECK(eos_market->market_name == name("eos"));
            CHECK(eos_market->quote.contract == EOS.contract);
            CHECK(eos_market->quote.get_extended_symbol().get_symbol().code() == EOS.get_extended_symbol().get_symbol().code());

            AND_WHEN("create_market(EOS) is called again") {
               CHECK_THROWS_WITH(exchange.create_market(name("exchange"), EOS), "market already exists");
            }
         }
      }
   }

}

TEST_CASE("remove_market") {

   GIVEN("USD market does not exist") {
      exchange_base_mock exchange{name("exchange")};
      extended_asset EOS = extended_asset(asset(0, symbol("EOS",4)), name("eosio.token"));
      extended_asset USD = extended_asset(asset(0, symbol("USD",2)), name("usd.token"));

      markets exchange_markets( name("exchange"), name("exchange").value );

      exchange.init_contract(false);

      WHEN("remove_market(USD) is called") {
         CHECK_THROWS_WITH(exchange.remove_market(USD), "market does not exist");
      }

      WHEN("the USD market is created and paired with EOS") {
         exchange.create_market(name("exchange"), USD);
         exchange.add_market_pair(name("exchange"), name("usd"), EOS);

         AND_WHEN("remove_market(USD) is called") {
            CHECK_THROWS_WITH(exchange.remove_market(USD), "remove market pairs before deleting");
         }
      }
   }

}

TEST_CASE("add_market_pair") {

   GIVEN("USD market does not exist") {
      exchange_base_mock exchange{name("exchange")};
      extended_asset EOS = extended_asset(asset(0, symbol("EOS",4)), name("eosio.token"));
      extended_asset USD = extended_asset(asset(0, symbol("USD",2)), name("usd.token"));

      exchange.init_contract(false);

      WHEN("add market pair EOS to non-existing USD market") {
         CHECK_THROWS_WITH(exchange.add_market_pair(name("exchange"), name("usd"), EOS), "market does not exist");
      }
   }

   GIVEN("USD market exists") {
      exchange_base_mock exchange{name("exchange")};
      extended_asset EOS = extended_asset(asset(0, symbol("EOS",4)), name("eosio.token"));
      extended_asset USD = extended_asset(asset(0, symbol("USD",2)), name("usd.token"));

      exchange.init_contract(false);

      markets exchange_markets( name("exchange"), name("exchange").value );
      exchange.create_market(name("exchange"), USD);

      WHEN("EOS is paired with the USD market to create EOS/USD") {
         exchange.add_market_pair(name("exchange"), name("usd"), EOS);

         THEN("the EOS/USD market pair is created") {
            auto eos_market = exchange_markets.find(name("usd").value);

            CHECK(eos_market->market_name == name("usd"));
            CHECK(eos_market->quote.contract == USD.contract);
            CHECK(eos_market->quote.get_extended_symbol().get_symbol().code() == USD.get_extended_symbol().get_symbol().code());

            name eosusd = exchange.create_market_pair_name(EOS, USD);
            auto eos_usd = eos_market->bases.find(eosusd);

            CHECK( eos_usd->first.value == eosusd.value );
            CHECK( eos_usd->second.get_extended_symbol().get_symbol().code() == EOS.get_extended_symbol().get_symbol().code() );

            AND_WHEN("EOS is paired with the USD market again") {
               CHECK_THROWS_WITH(exchange.add_market_pair(name("exchange"), name("usd"), EOS), "market pair already exists");
            }
         }
      }

      WHEN("add_market_pair is called with the same base and quote") {
         CHECK_THROWS_WITH(exchange.add_market_pair(name("exchange"), name("usd"), USD), "cannot pair asset against itself");
      }
   }

}

TEST_CASE("remove_market_pair") {

   GIVEN("EOS/USD market does not exists") {
      exchange_base_mock exchange{name("exchange")};
      extended_asset USD     = extended_asset(asset(0,     symbol("USD",2)), name("usd.token"));
      extended_asset EOS     = extended_asset(asset(0,     symbol("EOS",4)), name("eosio.token"));

      exchange.init_contract(false);

      WHEN("remove_market_pair(EOS, USD) is called") {
         CHECK_THROWS_WITH(exchange.remove_market_pair(EOS, USD), "market does not exist");
      }
   }

   GIVEN("USD market exists") {
      exchange_base_mock exchange{name("exchange")};
      extended_asset USD = extended_asset(asset(0, symbol("USD",2)), name("usd.token"));
      extended_asset EOS = extended_asset(asset(0, symbol("EOS",4)), name("eosio.token"));

      exchange.init_contract(false);
      exchange.create_market(name("exchange"), USD);

      WHEN("remove_market_pair(EOS, USD) is called") {
         CHECK_THROWS_WITH(exchange.remove_market_pair(EOS, USD), "market pair does not exist");
      }

      WHEN("EOS is paired with USD to create EOS/USD") {
         exchange.add_market_pair(name("exchange"), exchange.create_market_name(USD), EOS);

         AND_WHEN("EOS is removed from the USD market") {
            exchange.remove_market_pair(EOS, USD);

            THEN("The EOS pair is removed from the USD market") {
               auto usd_market = exchange.exchange_markets.find(exchange.create_market_name(USD).value);
               auto market_pair = usd_market->bases.find(exchange.create_market_pair_name(EOS, USD));

               CHECK(usd_market->market_name.value == name("usd").value);
               CHECK( market_pair == exchange.exchange_markets.find(exchange.create_market_name(USD).value )->bases.end());
            }
         }
      }
   }

}

TEST_CASE("calculate_volume") {
   exchange_base_mock exchange{name("exchange")};

   // 4 decimal precision : 2 decimal precision - EOS:USD
   GIVEN("price is 5.95 USD and volume is 100 EOS") {
      extended_asset price  = exchange.normalize_precision(extended_asset(asset(    595, symbol("USD",2)), name("usd.token")));
      extended_asset volume = exchange.normalize_precision(extended_asset(asset(1000000, symbol("EOS",4)), name("eosio.token")));

      WHEN("calculate_volume") {
         extended_asset trade_volume = exchange.calculate_volume(price, volume);

         THEN("trade volume is 595 USD") {
            // 5.95000000 USD/EOS * 100.00000000 EOS = 595.00000000 USD
            extended_asset result = extended_asset(asset(59500000000, symbol("USD",8)), name("usd.token"));
            CHECK(trade_volume.quantity.amount == result.quantity.amount);
            CHECK(trade_volume.get_extended_symbol().get_symbol().code() == result.get_extended_symbol().get_symbol().code());
         }
      }
   }

   // 4 decimal precision : 2 decimal precision - EOS:USD
   GIVEN("trade price is 17.12 USD and volume is 1.1234 EOS") {
      extended_asset price  = exchange.normalize_precision(extended_asset(asset( 1712, symbol("USD",2)), name("usd.token")));
      extended_asset volume = exchange.normalize_precision(extended_asset(asset(11234, symbol("EOS",4)), name("eosio.token")));

      WHEN("calculate_volume") {
         extended_asset trade_volume = exchange.calculate_volume(price, volume);

         THEN("trade volume is 19.23260800 USD") {
            // 17.12000000 USD/EOS * 1.12340000 EOS = 19.23260800 USD
            extended_asset result = extended_asset(asset(1923260800, symbol("USD",8)), name("usd.token"));
            CHECK(trade_volume.quantity.amount == result.quantity.amount);
            CHECK(trade_volume.get_extended_symbol().get_symbol().code() == result.get_extended_symbol().get_symbol().code());
         }
      }
   }

   // 8 decimal precision : 4 decimal precision - BTC:EOS
   GIVEN("trade price is 1.1234 EOS and volume is 1.12345678 BTC") {
      extended_asset price  = exchange.normalize_precision(extended_asset(asset(    11234, symbol("EOS",4)), name("eosio.token")));
      extended_asset volume = exchange.normalize_precision(extended_asset(asset(112345678, symbol("BTC",8)), name("eosio.token")));

      WHEN("calculate_volume") {
         extended_asset trade_volume = exchange.calculate_volume(price, volume);

         THEN("trade volume is 1.26209134 EOS") {
            // 1.1234 EOS/BTC * 1.12345678 BTC = 1.26209134[6652] EOS
            // The [6652] gets truncated
            extended_asset result = extended_asset(asset(126209134, symbol("EOS",8)), name("eosio.token"));
            CHECK(trade_volume.quantity.amount == result.quantity.amount);
            CHECK(trade_volume.get_extended_symbol().get_symbol().code() == result.get_extended_symbol().get_symbol().code());
         }
      }
   }

   // 8 decimal precision : 8 decimal precision - BTC:ETH
   GIVEN("trade price is 1.12345678 ETH and volume is 1.12345678 BTC") {
      extended_asset price  = exchange.normalize_precision(extended_asset(asset(112345678, symbol("ETH",8)), name("eosio.token")));
      extended_asset volume = exchange.normalize_precision(extended_asset(asset(112345678, symbol("BTC",8)), name("eosio.token")));

      WHEN("calculate_volume") {
         extended_asset trade_volume = exchange.calculate_volume(price, volume);

         THEN("trade volume is 1.26215513 ETH") {
            // 1.12345678 ETH/BTC * 1.12345678 BTC = 1.26215513[65279684] ETH
            // The [65279684] gets truncated
            extended_asset result = extended_asset(asset(126215513, symbol("ETH",8)), name("eosio.token"));
            CHECK(trade_volume.quantity.amount == result.quantity.amount);
            CHECK(trade_volume.get_extended_symbol().get_symbol().code() == result.get_extended_symbol().get_symbol().code());
         }
      }
   }

}

TEST_CASE("place_bid_order") {
   exchange_base_mock exchange{name("exchange")};
   name bob = name("bob");

   exchange.init_contract(false);

   GIVEN("bob has 50 EOS in his exchange account and the EOS/USD market is created") {
      extended_asset USD = extended_asset(asset(0,symbol("USD",2)), name("usd.token"));
      extended_asset EOS = extended_asset(asset(0, symbol("EOS",4)), name("eosio.token"));
      extended_asset deposit_EOS = exchange.normalize_precision(extended_asset(asset(500000, symbol("EOS",4)), name("eosio.token")));

      exchange.adjust_balance(bob, deposit_EOS);
      exchange.create_market(name("exchange"), USD);
      exchange.add_market_pair(name("exchange"), exchange.create_market_name(USD), EOS);

      WHEN("bob places an BID to sell 10 EOS @ 3.50 USD") {
         extended_asset price  = exchange.normalize_precision(extended_asset(asset(   350, symbol("USD",2)), name("usd.token")));
         extended_asset volume = exchange.normalize_precision(extended_asset(asset(100000, symbol("EOS",4)), name("eosio.token")));

         // BID to sell 10 EOS @ 3.50 USD
         uint64_t tx_id = 1;
         exchange.place_bid_order(bob, price, volume, "2019-05-26T10:10:00"_tp, tx_id);

         THEN("the BID order is placed in the order book") {
            bids bid_orders(name("exchange"), name("eosusd").value);
            auto order = bid_orders.find(tx_id);

            CHECK(order->id == tx_id);
            CHECK(order->trader == bob);
            CHECK(order->price.quantity.amount == price.quantity.amount);
            CHECK(order->volume.quantity.amount == volume.quantity.amount);

            AND_THEN("the exchanges' EOS balance increases by the BID volume") {
               auto exchange_exaccounts = exaccounts(name("exchange"), name("exchange").value).get_index<"bybalance"_n>();
               auto exchange_ex_balance = exchange_exaccounts.find(get_token_key(name("eosio.token"), symbol("EOS",8)));

               CHECK(exchange_ex_balance->balance.quantity.amount == volume.quantity.amount);

               AND_THEN("bobs' exchange EOS balance decreases by the BID volume") {
                  auto bob_exaccounts = exaccounts(name("exchange"), bob.value).get_index<"bybalance"_n>();
                  auto bob_ex_balance = bob_exaccounts.find(get_token_key(name("eosio.token"), symbol("EOS",8)));

                  CHECK(bob_ex_balance->balance.quantity.amount == (deposit_EOS - volume).quantity.amount);
               }
            }
         }
      }
   }
}

TEST_CASE("place_ask_order") {
   exchange_base_mock exchange{name("exchange")};
   name alice = name("alice");

   exchange.init_contract(false);

   GIVEN("alice has 500 USD in her exchange account and the EOS/USD market exists") {
      extended_asset USD = extended_asset(asset(0,symbol("USD",2)), name("usd.token"));
      extended_asset EOS = extended_asset(asset(0, symbol("EOS",4)), name("eosio.token"));
      extended_asset deposit_USD = exchange.normalize_precision(extended_asset(asset(50000, symbol("USD",2)), name("usd.token")));

      exchange.adjust_balance(alice, deposit_USD);
      exchange.create_market(name("exchange"), USD);
      exchange.add_market_pair(name("exchange"), exchange.create_market_name(USD), EOS);

      WHEN("alice places an ASK for 100 EOS @ 3.50 USD") {
         extended_asset price  = exchange.normalize_precision(extended_asset(asset(    350, symbol("USD",2)), name("usd.token")));
         extended_asset volume = exchange.normalize_precision(extended_asset(asset(1000000, symbol("EOS",4)), name("eosio.token")));

         // ASK to buy 100 EOS @ 3.50 USD
         //            100 EOS * 3.50 USD/EOS = 350.00 USD
         uint64_t tx_id = 1;
         exchange.place_ask_order(alice, price, volume, "2019-05-26T10:10:00"_tp, tx_id);

         THEN("the ask order is placed in the order book") {
            asks ask_orders(name("exchange"), name("eosusd").value);
            auto order = ask_orders.find(tx_id);

            CHECK(order->id == tx_id);
            CHECK(order->trader == alice);
            CHECK(order->price.quantity.amount == price.quantity.amount);
            CHECK(order->volume.quantity.amount == volume.quantity.amount);

            AND_THEN("the exchanges' USD balance increases by the ASK volume") {
               auto exchange_exaccounts = exaccounts(name("exchange"), name("exchange").value).get_index<"bybalance"_n>();
               auto exchange_ex_balance = exchange_exaccounts.find(get_token_key(name("usd.token"), symbol("USD",8)));

               CHECK(exchange_ex_balance->balance.quantity.amount == exchange.calculate_volume(price, volume).quantity.amount);

               AND_THEN("alices' exchange USD balance decreases by the ASK volume") {
                  auto alice_exaccounts = exaccounts(name("exchange"), alice.value).get_index<"bybalance"_n>();
                  auto alice_ex_balance = alice_exaccounts.find(get_token_key(name("usd.token"), symbol("USD",8)));

                  CHECK(alice_ex_balance->balance.quantity.amount == (deposit_USD - exchange.calculate_volume(price, volume)).quantity.amount);
               }
            }
         }
      }
   }
}

TEST_CASE("calculate_price") {
   exchange_base_mock exchange{name("exchange")};
   eosio::enable_multi_index enabler{name("exchange")};

   GIVEN("BIDs and ASKs have the same price") {
      extended_asset price_USD  = exchange.normalize_precision(extended_asset(asset(  131, symbol("USD",2)), name("usd.token")));
      extended_asset volume_EOS = exchange.normalize_precision(extended_asset(asset(50000, symbol("EOS",4)), name("eosio.token")));

      bids bid{name("exchange"), name("eosusd").value};
      auto bob_bid = bid.emplace(name("exchange"), [&](auto& b) {
         b.id        = 0;
         b.trader    = name("bob");
         b.timestamp = "2019-05-26T10:10:00"_tp;
         b.price     = price_USD;
         b.volume    = volume_EOS;
      });

      asks ask{name("exchange"), name("eosusd").value};
      auto alice_ask = ask.emplace(name("exchange"), [&](auto& a) {
         a.id        = 0;
         a.trader    = name("alice");
         a.timestamp = "2019-05-26T10:10:01"_tp;
         a.price     = price_USD;
         a.volume    = volume_EOS;
      });

      WHEN("calculate_price is called") {
         int64_t spread = bob_bid->price.quantity.amount - alice_ask->price.quantity.amount;
         CHECK(spread == 0);
         extended_asset trade_price = exchange.calculate_price( spread, bob_bid, alice_ask);

         THEN("trade price is same as BID and ASK price") {
            CHECK(trade_price.quantity.amount == price_USD.quantity.amount);
         }
      }
   }

   GIVEN("ASK has a higher price and was placed later than BID") {
      extended_asset bid_price_USD  = exchange.normalize_precision(extended_asset(asset(  131, symbol("USD",2)), name("usd.token")));
      extended_asset bid_volume_EOS = exchange.normalize_precision(extended_asset(asset(50000, symbol("EOS",4)), name("eosio.token")));

      extended_asset ask_price_USD  = exchange.normalize_precision(extended_asset(asset(  135, symbol("USD",2)), name("usd.token")));
      extended_asset ask_volume_EOS = exchange.normalize_precision(extended_asset(asset(50000, symbol("EOS",4)), name("eosio.token")));

      bids bid{name("exchange"), name("eosusd").value};
      auto bob_bid = bid.emplace(name("exchange"), [&](auto& b) {
         b.id        = 0;
         b.trader    = name("bob");
         b.timestamp = "2019-05-26T10:10:00"_tp;
         b.price     = bid_price_USD;
         b.volume    = bid_volume_EOS;
      });

      asks ask{name("exchange"), name("eosusd").value};
      auto alice_ask = ask.emplace(name("exchange"), [&](auto& a) {
         a.id        = 0;
         a.trader    = name("alice");
         a.timestamp = "2019-05-26T10:10:01"_tp;
         a.price     = ask_price_USD;
         a.volume    = ask_volume_EOS;
      });

      WHEN("calculate_price is called") {
         int64_t spread = bob_bid->price.quantity.amount - alice_ask->price.quantity.amount;
         CHECK(spread == -4000000);
         extended_asset trade_price = exchange.calculate_price( spread, bob_bid, alice_ask);

         THEN("trade price is the same as BID price") {
            CHECK(trade_price.quantity.amount == bid_price_USD.quantity.amount);
         }
      }
   }

   GIVEN("BID is lower and was placed later than ASK") {
      extended_asset bid_price_USD  = exchange.normalize_precision(extended_asset(asset(  130, symbol("USD",2)), name("usd.token")));
      extended_asset bid_volume_EOS = exchange.normalize_precision(extended_asset(asset(50000, symbol("EOS",4)), name("eosio.token")));

      extended_asset ask_price_USD  = exchange.normalize_precision(extended_asset(asset(  131, symbol("USD",2)), name("usd.token")));
      extended_asset ask_volume_EOS = exchange.normalize_precision(extended_asset(asset(50000, symbol("EOS",4)), name("eosio.token")));

      asks ask{name("exchange"), name("eosusd").value};
      auto alice_ask = ask.emplace(name("exchange"), [&](auto& a) {
         a.id        = 0;
         a.trader    = name("alice");
         a.timestamp = "2019-05-26T10:10:00"_tp;
         a.price     = ask_price_USD;
         a.volume    = ask_volume_EOS;
      });

      bids bid{name("exchange"), name("eosusd").value};
      auto bob_bid = bid.emplace(name("exchange"), [&](auto& b) {
         b.id        = 0;
         b.trader    = name("bob");
         b.timestamp = "2019-05-26T10:10:01"_tp;
         b.price     = bid_price_USD;
         b.volume    = bid_volume_EOS;
      });

      WHEN("calculate_price is called") {
         int64_t spread = bob_bid->price.quantity.amount - alice_ask->price.quantity.amount;
         CHECK(spread == -1000000);
         extended_asset trade_price = exchange.calculate_price( spread, bob_bid, alice_ask);

         THEN("trade price is the same as ASK price") {
            CHECK(trade_price.quantity.amount == ask_price_USD.quantity.amount);
         }
      }
   }

}

TEST_CASE("match_orders") {
   exchange_base_mock exchange{name("exchange")};
   name alice = name("alice");
   name bob   = name("bob");

   exchange.init_contract(false);

   GIVEN("bob has 50 EOS deposited, alice has 500 USD deposited, and the EOS/USD market is created") {
      extended_asset USD = extended_asset(asset(0,symbol("USD",2)), name("usd.token"));
      extended_asset EOS = extended_asset(asset(0, symbol("EOS",4)), name("eosio.token"));

      // $500.00 USD
      extended_asset deposit_USD = exchange.normalize_precision(extended_asset(asset(50000, symbol("USD",2)), name("usd.token")));
      // $50.0000 EOS
      extended_asset deposit_EOS = exchange.normalize_precision(extended_asset(asset(500000, symbol("EOS",4)), name("eosio.token")));

      // create EOS/USD market
      exchange.create_market(name("exchange"), USD);
      exchange.add_market_pair(name("exchange"), exchange.create_market_name(USD), EOS);

      exchange.adjust_balance(alice, deposit_USD);
      exchange.adjust_balance(bob, deposit_EOS);

      // Tables
      auto exchange_exaccounts = exaccounts(name("exchange"), name("exchange").value).get_index<"bybalance"_n>();
      auto alice_exaccounts = exaccounts(name("exchange"), alice.value).get_index<"bybalance"_n>();
      auto bob_exaccounts = exaccounts(name("exchange"), bob.value).get_index<"bybalance"_n>();

      /*
      Scenerio 1 Partial Fill

      BEFORE:
      | Time    | Buy/Sell | Price (USD)  | Volume (EOS) |                       |
      |---------|----------|------------- |--------------|-----------------------|
      | 7:33:01 | Sell     | 1.31         | 4            | **<- Best Sell**      |
      | 7:34:02 | Buy      | 1.32         | 10           | **<- Best Buy**       |

      AFTER:
      | Time    | Buy/Sell | Price (USD)  | Volume (EOS) |                       |
      |---------|----------|------------- |--------------|-----------------------|
      | 7:34:02 | Buy      | 1.32         | 6           | **<- Best Buy**       |

      */
      WHEN("bob places an BID to sell 4 EOS @ 1.31 USD") {
         extended_asset bid_price  = exchange.normalize_precision(extended_asset(asset(  131, symbol("USD",2)), name("usd.token")));
         extended_asset bid_volume = exchange.normalize_precision(extended_asset(asset(40000, symbol("EOS",4)), name("eosio.token")));

         // BID to sell 4 EOS @ 1.31 USD
         exchange.place_bid_order(bob, bid_price, bid_volume, "2019-05-26T10:10:00"_tp, 1);

         WHEN("alice places an ASK for 10 EOS @ 1.32 USD") {
            extended_asset ask_price  = exchange.normalize_precision(extended_asset(asset(   132, symbol("USD",2)), name("usd.token")));
            extended_asset ask_volume = exchange.normalize_precision(extended_asset(asset(100000, symbol("EOS",4)), name("eosio.token")));

            // ASK to buy 10 EOS @ 1.32 USD
            //            10 EOS * 1.32 USD/EOS = 13.20 USD
            exchange.place_ask_order(alice, ask_price, ask_volume, "2019-05-26T10:10:01"_tp, 1);

            THEN("alices ASK will partial fill") {
               // bob's BID clears

               asks ask_orders(name("exchange"), name("eosusd").value);
               auto ask = ask_orders.find(1);

               CHECK(ask->id == 1);
               CHECK(ask->trader == alice);
               CHECK(ask->price.quantity.amount == ask_price.quantity.amount);
               CHECK(ask->volume.quantity.amount == (ask_volume.quantity.amount - bid_volume.quantity.amount));

               // Remaing ASK = ASK required to purchase - ASK actually purchased
               // (ask_price * ask_volume) - (bid_price * bid_volume) - (volume offset from ask and trade price)
               // (1.32 * 10) - (1.31 * 4) - (1.32 * 4 - 1.31 * 4) = 7.92 USD
               AND_THEN("the exchanges' USD balance will be 7.92 USD") {
                  extended_asset exchanges_remaining_USD_ex_balance = exchange.calculate_volume(ask_price, ask_volume)
                                                                      -
                                                                      exchange.calculate_volume(bid_price, bid_volume)
                                                                      -
                                                                      (exchange.calculate_volume(ask_price, bid_volume) - exchange.calculate_volume(bid_price, bid_volume));
                  auto exchange_USD_ex_balance = exchange_exaccounts.find(get_token_key(name("usd.token"), symbol("USD",8)));

                  CHECK(exchange_USD_ex_balance->balance.quantity.amount == exchanges_remaining_USD_ex_balance.quantity.amount);  // 792000000
                  CHECK(exchange_USD_ex_balance->balance.get_extended_symbol().get_symbol().code() == exchanges_remaining_USD_ex_balance.get_extended_symbol().get_symbol().code());

                  // CHECK USD BALANCES //

                  // 500 USD - (4 * 1.31 USD) - (6 * 1.32 USD) = 486.84 USD
                  AND_THEN("alices' USD exchange balance will decrease by 13.16 USD") {
                     extended_asset alices_remaining_USD_ex_balance = deposit_USD
                                                                      -
                                                                      exchange.calculate_volume(bid_price, bid_volume)
                                                                      -
                                                                      exchange.calculate_volume(ask_price, ask_volume - bid_volume);
                     auto alice_USD_ex_balance = alice_exaccounts.find(get_token_key(name("usd.token"), symbol("USD",8)));

                     CHECK(alice_USD_ex_balance->balance.quantity.amount == alices_remaining_USD_ex_balance.quantity.amount); // 48684000000
                     CHECK(alice_USD_ex_balance->balance.get_extended_symbol().get_symbol().code() == alices_remaining_USD_ex_balance.get_extended_symbol().get_symbol().code());

                     // 1.31 USD/EOS * 4 EOS = 5.24 USD
                     AND_THEN("bobs' USD balance will increase by 5.24 USD") {
                        extended_asset bobs_payout_USD_ex_balance = exchange.calculate_volume(bid_price, bid_volume);
                        auto bob_USD_ex_balance = bob_exaccounts.find(get_token_key(name("usd.token"), symbol("USD",8)));

                        CHECK(bob_USD_ex_balance->balance.quantity.amount == bobs_payout_USD_ex_balance.quantity.amount);  // 524000000
                        CHECK(bob_USD_ex_balance->balance.get_extended_symbol().get_symbol().code() == bobs_payout_USD_ex_balance.get_extended_symbol().get_symbol().code());

                        // CHECK EOS BALANCES //

                        AND_THEN("the exchanges EOS balance will be zero") {
                           auto exchanges_remaining_EOS_ex_balance = exchange_exaccounts.find(get_token_key(name("eosio.token"), symbol("EOS",8)));

                           CHECK(exchanges_remaining_EOS_ex_balance->balance.quantity.amount == 0);
                           CHECK(exchanges_remaining_EOS_ex_balance->balance.get_extended_symbol().get_symbol().code() == bid_volume.get_extended_symbol().get_symbol().code());

                           AND_THEN("alices' EOS balance will increase by 4 EOS") {
                              auto alice_EOS_ex_balance = alice_exaccounts.find(get_token_key(name("eosio.token"), symbol("EOS",8)));

                              CHECK(alice_EOS_ex_balance->balance.quantity.amount == bid_volume.quantity.amount); // 400000000
                              CHECK(alice_EOS_ex_balance->balance.get_extended_symbol().get_symbol().code() == bid_volume.get_extended_symbol().get_symbol().code());
                           }
                        }
                     }
                  }
               }
            }
         }
      }

      /*
      Scenerio 2 Fill and Partial Fill

      BEFORE:
      | Time    | Buy/Sell | Price (USD)  | Volume (EOS) |                       |
      |---------|----------|------------- |--------------|-----------------------|
      | 7:33:01 | Sell     | 1.31         | 4            | **<- Best Sell**      |
      | 7:33:02 | Sell     | 1.31         | 7            | **<- Next Best Sell** |
      | 7:34:02 | Buy      | 1.32         | 10           | **<- Best Buy**       |

      AFTER:
      | Time    | Buy/Sell | Price (USD)  | Volume (EOS) |                       |
      |---------|----------|------------- |--------------|-----------------------|
      | 7:33:02 | Sell     | 1.31         | 1            | **<- Next Best Sell** |

      */
      WHEN("bob places two BIDs to sell 4 EOS @ 1.31 USD and 7 EOS @ 1.31 USD") {
         extended_asset bid_price_1 = exchange.normalize_precision(extended_asset(asset(  131, symbol("USD",2)), name("usd.token")));
         extended_asset bid_volume_1 = exchange.normalize_precision(extended_asset(asset(40000, symbol("EOS",4)), name("eosio.token")));

         // BID to sell 4 EOS @ 1.31 USD
         exchange.place_bid_order(bob, bid_price_1, bid_volume_1, "2019-05-26T10:10:00"_tp, 1);

         extended_asset bid_price_2  = exchange.normalize_precision(extended_asset(asset(  131, symbol("USD",2)), name("usd.token")));
         extended_asset bid_volume_2 = exchange.normalize_precision(extended_asset(asset(70000, symbol("EOS",4)), name("eosio.token")));

         // BID to sell 7 EOS @ 1.31 USD
         exchange.place_bid_order(bob, bid_price_2, bid_volume_2, "2019-05-26T10:10:01"_tp, 2);

         extended_asset total_bid_volume = bid_volume_1 + bid_volume_2;

         WHEN("alice places an ASK for 10 EOS @ 1.32 USD") {
            extended_asset ask_price  = exchange.normalize_precision(extended_asset(asset(   132, symbol("USD",2)), name("usd.token")));
            extended_asset ask_volume = exchange.normalize_precision(extended_asset(asset(100000, symbol("EOS",4)), name("eosio.token")));

            // ASK to buy 10 EOS @ 1.32 USD
            //            10 EOS * 1.32 USD/EOS = 13.20 USD
            exchange.place_ask_order(alice, ask_price, ask_volume, "2019-05-26T10:10:02"_tp, 1);

            THEN("bobs 1st BID order will fill and the 2nd BID will partial fill") {
               bids bid_orders(name("exchange"), name("eosusd").value);

               // 1st BID order filled
               CHECK(bid_orders.find(1) == bid_orders.end());

               // 2nd BID
               auto bid = bid_orders.find(2);
               CHECK(bid->id == 2);
               CHECK(bid->trader == bob);
               CHECK(bid->timestamp == "2019-05-26T10:10:01"_tp);
               CHECK(bid->price.quantity.amount == bid_price_2.quantity.amount);
               CHECK(bid->volume.quantity.amount  == (total_bid_volume - ask_volume).quantity.amount);


               // Remaing ASK = ASK required to purchase - ASK actually purchased
               // (ask_price * ask_volume) - (bid_price * bid_volume) - (volume offset from ask and trade price)
               // (1.32 * 10) - (1.31 * 10) - (1.32 * 10 - 1.31 * 10) = 0 USD
               AND_THEN("the exchanges' USD balance will be 0.10 USD") {
                  extended_asset exchanges_remaining_USD_ex_balance = exchange.calculate_volume(ask_price, ask_volume)
                                                                      -
                                                                      exchange.calculate_volume(bid_price_1, ask_volume)
                                                                      -
                                                                      (exchange.calculate_volume(ask_price, ask_volume) - exchange.calculate_volume(bid_price_1, ask_volume));
                  auto exchange_USD_ex_balance = exchange_exaccounts.find(get_token_key(name("usd.token"), symbol("USD",8)));

                  CHECK(exchange_USD_ex_balance->balance.quantity.amount == exchanges_remaining_USD_ex_balance.quantity.amount);  // 0.00000000 USD
                  CHECK(exchange_USD_ex_balance->balance.get_extended_symbol().get_symbol().code() == exchanges_remaining_USD_ex_balance.get_extended_symbol().get_symbol().code());

                  // CHECK USD BALANCES //

                  // 500 USD - (10 * 1.31 USD) = 486.90 USD
                  AND_THEN("alices' USD exchange balance will decrease by 13.20 USD") {
                     extended_asset alices_remaining_USD_ex_balance = deposit_USD
                                                                      -
                                                                      exchange.calculate_volume(bid_price_1, ask_volume);
                     auto alice_USD_ex_balance = alice_exaccounts.find(get_token_key(name("usd.token"), symbol("USD",8)));

                     CHECK(alice_USD_ex_balance->balance.quantity.amount == alices_remaining_USD_ex_balance.quantity.amount); // 48690000000 USD
                     CHECK(alice_USD_ex_balance->balance.get_extended_symbol().get_symbol().code() == alices_remaining_USD_ex_balance.get_extended_symbol().get_symbol().code());

                     // 1.31 USD/EOS * 10 EOS = 13.10 USD
                     AND_THEN("bobs' USD balance will increase by 13.10 USD") {
                        extended_asset bobs_payout_USD_ex_balance = exchange.calculate_volume(bid_price_1, ask_volume);
                        auto bob_USD_ex_balance = bob_exaccounts.find(get_token_key(name("usd.token"), symbol("USD",8)));

                        CHECK(bob_USD_ex_balance->balance.quantity.amount == bobs_payout_USD_ex_balance.quantity.amount);  // 1310000000 USD
                        CHECK(bob_USD_ex_balance->balance.get_extended_symbol().get_symbol().code() == bobs_payout_USD_ex_balance.get_extended_symbol().get_symbol().code());

                        // CHECK EOS BALANCES //

                        AND_THEN("the exchanges EOS balance will be 1 EOS") {
                           auto exchanges_remaining_EOS_ex_balance = exchange_exaccounts.find(get_token_key(name("eosio.token"), symbol("EOS",8)));

                           CHECK(exchanges_remaining_EOS_ex_balance->balance.quantity.amount == bid->volume.quantity.amount); // 100000000 USD
                           CHECK(exchanges_remaining_EOS_ex_balance->balance.get_extended_symbol().get_symbol().code() == bid->volume.get_extended_symbol().get_symbol().code());

                           AND_THEN("alices' EOS balance will increase by 10 EOS") {
                              auto alice_EOS_ex_balance = alice_exaccounts.find(get_token_key(name("eosio.token"), symbol("EOS",8)));

                              CHECK(alice_EOS_ex_balance->balance.quantity.amount == ask_volume.quantity.amount); // 1000000000 EOS
                              CHECK(alice_EOS_ex_balance->balance.get_extended_symbol().get_symbol().code() == ask_volume.get_extended_symbol().get_symbol().code());
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }

}

TEST_CASE("cancel_order") {
   exchange_base_mock exchange{name("exchange")};
   name alice = name("alice");
   name bob   = name("bob");

   exchange.init_contract(false);

   GIVEN("The EOS/USD market does not exists, and alice has USD and bob has EOS deposited") {
      extended_asset USD = extended_asset(asset(0,symbol("USD",2)), name("usd.token"));
      extended_asset EOS = extended_asset(asset(0, symbol("EOS",4)), name("eosio.token"));

      WHEN("alice cancels ASK ID: 0") {
         CHECK_THROWS_WITH(exchange.cancel_order(EOS, USD, alice, 0, 0), "market does not exist");
      }

      WHEN("the USD market is created") {
         exchange.create_market(name("exchange"), USD);

         AND_WHEN("alice cancels ASK ID: 0") {
            CHECK_THROWS_WITH(exchange.cancel_order(EOS, USD, alice, 0, 0), "market pair does not exist");
         }
      }
   }

   GIVEN("The EOS/USD market exists, alice has USD and bob has EOS deposited") {
      extended_asset USD = extended_asset(asset(0,symbol("USD",2)), name("usd.token"));
      extended_asset EOS = extended_asset(asset(0, symbol("EOS",4)), name("eosio.token"));
      extended_asset deposit_USD = exchange.normalize_precision(extended_asset(asset(  50000, symbol("USD",2)), name("usd.token")));
      extended_asset deposit_EOS = exchange.normalize_precision(extended_asset(asset(5000000, symbol("EOS",4)), name("eosio.token")));

      exchange.adjust_balance(alice, deposit_USD);
      exchange.adjust_balance(bob, deposit_EOS);
      // create EOS/USD market
      exchange.create_market(name("exchange"), USD);
      exchange.add_market_pair(name("exchange"), exchange.create_market_name(USD), EOS);

      WHEN("bob places a BID to sell 100 EOS @ 3.50 USD") {
         extended_asset price  = exchange.normalize_precision(extended_asset(asset(    350, symbol("USD",2)), name("usd.token")));
         extended_asset volume = exchange.normalize_precision(extended_asset(asset(1000000, symbol("EOS",4)), name("eosio.token")));

         // BID to sell 100 EOS @ 3.50 USD
         exchange.place_bid_order(bob, price, volume, "2019-05-26T10:10:00"_tp, 1);

         // bob exchange balance EOS
         auto bob_exaccounts = exaccounts(name("exchange"), bob.value).get_index<"bybalance"_n>();
         auto bob_ex_balance = bob_exaccounts.find(get_token_key(name("eosio.token"), symbol("EOS",8)));
         CHECK(bob_ex_balance->balance.quantity.amount == (deposit_EOS.quantity.amount - volume.quantity.amount));

         // exchanges exchange balance = 100 EOS
         auto exchange_exaccounts = exaccounts(name("exchange"), name("exchange").value).get_index<"bybalance"_n>();
         auto exchange_ex_balance = exchange_exaccounts.find(get_token_key(name("eosio.token"), symbol("EOS",8)));
         CHECK(exchange_ex_balance->balance.quantity.amount == volume.quantity.amount);

         AND_WHEN("bob cancels the order") {
            exchange.cancel_order(EOS, USD, bob, 0, 1);

            THEN("the exchange balance is returned to bob") {
               // alices exchange balance = 500 USD
               exchange_ex_balance = bob_exaccounts.find(get_token_key(name("eosio.token"), symbol("EOS",8)));

               CHECK(bob_ex_balance->balance.quantity.amount == deposit_EOS.quantity.amount);

               // exchanges exchange balance
               exchange_ex_balance = exchange_exaccounts.find(get_token_key(name("eosio.token"), symbol("EOS",8)));
               CHECK(exchange_ex_balance->balance.quantity.amount == 0);
            }
         }
      }

      WHEN("alices places an ASK for 100 EOS @ 3.50 USD") {
         extended_asset price  = exchange.normalize_precision(extended_asset(asset(    350, symbol("USD",2)), name("usd.token")));
         extended_asset volume = exchange.normalize_precision(extended_asset(asset(1000000, symbol("EOS",4)), name("eosio.token")));

         // ASK to buy 100 EOS @ 3.50 USD (100 EOS * 3.50 USD/EOS = 350.00 USD)
         exchange.place_ask_order(alice, price, volume, "2019-05-26T10:10:00"_tp, 1);

         // alices exchange balance USD
         auto alice_exaccounts = exaccounts(name("exchange"), alice.value).get_index<"bybalance"_n>();
         auto alice_ex_balance = alice_exaccounts.find(get_token_key(name("usd.token"), symbol("USD",8)));
         CHECK(alice_ex_balance->balance.quantity.amount == (deposit_USD.quantity.amount - exchange.calculate_volume(price, volume).quantity.amount));

         // exchanges exchange balance = 350 USD
         auto exchange_exaccounts = exaccounts(name("exchange"), name("exchange").value).get_index<"bybalance"_n>();
         auto exchange_ex_balance = exchange_exaccounts.find(get_token_key(name("usd.token"), symbol("USD",8)));
         CHECK(exchange_ex_balance->balance.quantity.amount == exchange.calculate_volume(price, volume).quantity.amount);

         AND_WHEN("alice cancels the order") {
            exchange.cancel_order(EOS, USD, alice, 1, 1);

            THEN("the exchange balance is returned to alice") {
               // alices exchange balance = 500 USD
               alice_ex_balance = alice_exaccounts.find(get_token_key(name("usd.token"), symbol("USD",8)));

               CHECK(alice_ex_balance->balance.quantity.amount == 50000000000);

               // exchanges exchange balance
               exchange_ex_balance = exchange_exaccounts.find(get_token_key(name("usd.token"), symbol("USD",8)));
               CHECK(exchange_ex_balance->balance.quantity.amount == 0);
            }
         }
      }
   }
}
