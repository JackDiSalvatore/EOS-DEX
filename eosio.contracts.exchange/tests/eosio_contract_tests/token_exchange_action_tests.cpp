#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/resource_limits.hpp>

#include "../contracts.hpp"
#include "../eosio.system_tester.hpp"

using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

using mvo = fc::mutable_variant_object;

#define CONTRACT_ACCOUNT    name("exchange")
#define EOS_TOKEN_ACCOUNT   name("eosio.token")
#define EOS_2_TOKEN_ACCOUNT name("eosio2.token")
#define BTC_TOKEN_ACCOUNT   name("btc.token")


namespace eosio {
   namespace chain {

     /**
       * @brief Subtraction operator
       *
       * @details Subtraction operator. This subtracts the amount of the extended asset.
       * @param a - The extended asset to be subtracted
       * @param b - The extended asset used to subtract
       * @return extended_asset - New extended asset as the result of subtraction
       * @pre The owner of both extended asset need to be the same
       */
      extended_asset operator - ( const extended_asset& a, const extended_asset& b ) {
         assert( a.contract == b.contract );
         return {a.quantity - b.quantity, a.contract};
      }

      /**
       * @brief Addition operator
       *
       * @details Addition operator. This adds the amount of the extended asset.
       * @param a - The extended asset to be added
       * @param b - The extended asset to be added
       * @return extended_asset - New extended asset as the result of addition
       * @pre The owner of both extended asset need to be the same
       */
      extended_asset operator + ( const extended_asset& a, const extended_asset& b ) {
         assert( a.contract == b.contract );
         return {a.quantity + b.quantity, a.contract};
      }

      /// Addition operator.
      extended_asset& operator+=( extended_asset& a, const extended_asset& b ) {
         assert( a.contract == b.contract );
         a.quantity += b.quantity;
         return a;
      }

      /// Subtraction operator.
      extended_asset& operator-=( extended_asset& a, const extended_asset& b ) {
         assert( a.contract == b.contract );
         a.quantity -= b.quantity;
         return a;
      }

      /// Less than operator
      bool operator<( const extended_asset& a, const extended_asset& b ) {
         assert( a.contract == b.contract );
         return a.quantity < b.quantity;
      }


      /// Comparison operator
      bool operator==( const extended_asset& a, const extended_asset& b ) {
         return std::tie(a.quantity, a.contract) == std::tie(b.quantity, b.contract);
      }

      /// Comparison operator
      bool operator!=( const extended_asset& a, const extended_asset& b ) {
         return !(a == b);
      }

      /// Comparison operator
      bool operator<=( const extended_asset& a, const extended_asset& b ) {
         assert( a.contract == b.contract );
         return a.quantity <= b.quantity;
      }

      /// Comparison operator
      bool operator>=( const extended_asset& a, const extended_asset& b ) {
         assert( a.contract == b.contract );
         return a.quantity >= b.quantity;
      }
   }
}


namespace eosio_system {

   class exchange_tester : public eosio_system_tester {
   public:
      static name exchange;
      static name eosio_token;
      static name eosio2_token;
      static name btc_token;

      exchange_tester() {
         // create tester accounts
         create_account_with_resources(name("alice"), config::system_account_name, 1000000);
         create_account_with_resources(name("bob"),   config::system_account_name, 1000000);
         create_account_with_resources(name("carol"), config::system_account_name, 1000000);

         deploy_exchange();
      }

      abi_serializer deploy_code(name account, const std::vector<uint8_t>& wasm, const std::vector<char>& abiname) {
         set_code(account, wasm);
         set_abi(account, abiname.data());

         produce_blocks();
         const auto& accnt = control->db().get<account_object, by_name>(account);

         abi_serializer abi_ser;
         abi_def        abi;
         REQUIRE(abi_serializer::to_abi(accnt.abi, abi));
         abi_ser.set_abi(abi, abi_serializer_max_time);
         return abi_ser;
      }

      void deploy_token_contract(name token_account, asset max_supply) {
         create_account_with_resources(token_account, config::system_account_name, 1000000);
         produce_blocks(2);
         deploy_code(token_account, contracts::token_wasm(), contracts::token_abi());
         create_currency(token_account, token_account, max_supply);
         issue(token_account, token_account, max_supply);
      }

      void deploy_exchange() {
         create_account_with_resources(exchange, config::system_account_name, 1000000);
         produce_blocks(2);
         deploy_code(exchange, contracts::exchange_wasm(), contracts::exchange_abi());
         add_code_permission(exchange);

         deploy_token_contract(btc_token,  asset(2100000000000000, symbol(8,"BTC")));
         deploy_token_contract(eosio2_token, asset(10000000000000, symbol(4,"EOS")));

         // native eosio account already deployed
         create_currency(eosio_token, eosio_token, asset(10000000000000, symbol(4,"EOS")));
         issue(eosio_token, eosio_token, asset(10000000000000, symbol(4,"EOS")));
      }

      action_result push_action_ex(account_name actor, const name& code, const action_name& acttype, const variant_object& data) {
         return base_tester::push_action(get_action(code, acttype, {permission_level{actor, config::active_name}}, data), uint64_t(actor));
      }

      action_result push_action_ex(const std::vector<permission_level>& perms,
                                   const name&                          code,
                                   const action_name&                   acttype,
                                   const variant_object&                data,
                                   const std::vector<permission_level>& signers) {
         signed_transaction trx;
         trx.actions.emplace_back(get_action(code, acttype, perms, data));
         set_transaction_headers(trx);
         for (const auto& auth : signers) {
            trx.sign(get_private_key(auth.actor, auth.permission.to_string()), control->get_chain_id());
         }
         try {
            // print_action_console(push_transaction(trx));
            push_transaction(trx);
         } catch (const fc::exception& ex) {
            edump((ex.to_detail_string()));
            return error(ex.top_message()); // top_message() is assumed by many tests; otherwise they fail
            // return error(ex.to_detail_string());
         }
         produce_block();
         BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
         return success();
      }

      // helper functions

      void add_code_permission(name account) {
         const auto priv_key = this->get_private_key(account.to_string(), "active");
         const auto pub_key  = priv_key.get_public_key();

         this->set_authority(account, name("active"), authority(1, {key_weight{pub_key, 1}}, {{permission_level{account, name("eosio.code")}, 1}}),
                             "owner");
      }

      // token contract actions

      action_result create_currency( name contract, name issuer, asset maxsupply ) {
         return push_action_ex(contract, contract, name("create"),
                                    mutable_variant_object()("issuer", issuer)("maximum_supply", maxsupply));
      }

      action_result issue(name token_account ,name to, asset quantity, std::string memo = "") {
         return push_action_ex(token_account, token_account, name("issue"),
                                 mutable_variant_object()("to", to)("quantity", quantity)("memo", memo));
      }

      action_result transfer(name token_contract, name from, name to, const asset& amount, std::string memo) {
         return push_action_ex(from, token_contract, name("transfer"),
                               mutable_variant_object()("from", from)("to", to)("quantity", amount)("memo", memo));
      }

      // exchange contract actions

      action_result init(name actor, bool user_pays) {
         return push_action_ex(actor, exchange, name("init"),
                               mutable_variant_object()("user_pays", user_pays));
      }

      action_result withdraw(name actor, name from, extended_asset token) {
         return push_action_ex(actor, exchange, name("withdraw"),
                               mutable_variant_object()("from", from)("token", token));
      }

      action_result createmarket(name actor, name owner, extended_asset quote ) {
         return push_action_ex(actor, exchange, name("createmarket"),
                               mutable_variant_object()
                               ("owner", owner)
                               ("quote", quote)
                              );
      }

      action_result removemarket(name actor, extended_asset quote) {
         return push_action_ex(actor, exchange, name("removemarket"),
                               mutable_variant_object()
                               ("quote", quote)
                              );
      }

      action_result addpair(name actor, name owner, extended_asset quote, extended_asset base) {
         return push_action_ex(actor, exchange, name("addpair"),
                               mutable_variant_object()
                               ("owner", owner)
                               ("quote", quote)
                               ("base", base)
                              );
      }

      action_result removepair(name actor, extended_asset quote, extended_asset base) {
         return push_action_ex(actor, exchange, name("removepair"),
                               mutable_variant_object()
                               ("quote", quote)
                               ("base", base)
                              );
      }

      action_result trade(name actor, name trader, bool order_type, extended_asset price, extended_asset volume, bool auto_withdraw) {
         return push_action_ex(actor, exchange, name("trade"),
                               mutable_variant_object()
                               ("trader", trader)
                               ("order_type", order_type)
                               ("price", price)
                               ("volume", volume)
                               ("auto_withdraw", auto_withdraw)
                              );
      }

      action_result cancel(name actor, extended_asset base, extended_asset quote, name trader, bool order_type, uint64_t id, bool auto_withdraw) {
         return push_action_ex(actor, exchange, name("cancel"),
                               mutable_variant_object()
                               ("base", base)
                               ("quote", quote)
                               ("trader", trader)
                               ("order_type", order_type)
                               ("id", id)
                               ("auto_withdraw", auto_withdraw)
                              );
      }

      // tables

      abi_serializer get_serializer(name acc) {
         const auto& acnt = control->get_account(acc);
         auto        abi  = acnt.get_abi();
         return abi_serializer(abi, abi_serializer_max_time);
      }

      // token tables

      asset get_account_balance(name token_account, account_name acc, const string& symbolname) {
         auto symb = eosio::chain::symbol::from_string(symbolname);
         auto         symbol_code = symb.to_symbol_code().value;
         vector<char> data        = get_row_by_account(token_account, acc, name("accounts"), symbol_code);
         return data.empty() ? asset(0, symb)
                              : get_serializer(token_account).binary_to_variant("account", data, abi_serializer_max_time)["balance"].as<asset>();
      }

      // exchange tables

      extended_asset get_exchange_account_balance(account_name acc, const string& symbolname, const uint64_t& index) {
         auto symb = eosio::chain::symbol::from_string(symbolname);
         auto symbol_code = symb.to_symbol_code().value;
         vector<char> data = get_row_by_account(CONTRACT_ACCOUNT, acc, name("exaccounts"), index);

         return data.empty() ? extended_asset(asset(0,symb), name("eosio.token")) :
            get_serializer(CONTRACT_ACCOUNT).binary_to_variant("exaccount", data, abi_serializer_max_time)["balance"].as<extended_asset>();
      }

   };

   name exchange_tester::exchange     = CONTRACT_ACCOUNT;
   name exchange_tester::eosio_token  = EOS_TOKEN_ACCOUNT;
   name exchange_tester::eosio2_token = EOS_2_TOKEN_ACCOUNT;
   name exchange_tester::btc_token    = BTC_TOKEN_ACCOUNT;

} // namespace eosio_system


TEST_CASE_FIXTURE(eosio_system::exchange_tester, "init") try {

   GIVEN("contract not initialized") {
      WHEN("the exchange account initializes the contract") {
         auto r = init(exchange, true);

         THEN("the contract is successfully initialized") {
            CHECK(r == success());
         }
      }
      WHEN("alice attempts to initialize the contract") {
         auto r = init(name("alice"), true);

         THEN("the contract fails to initialize") {
            CHECK(r == "missing authority of exchange");
         }
      }
   }

} FC_LOG_AND_RETHROW()


TEST_CASE_FIXTURE(eosio_system::exchange_tester, "deposit") try {

   GIVEN("alice has 100 EOS her eosio.token account and 0 EOS her exchange account") {
      // tester accounts
      name alice            = name("alice");
      name eosio_token      = name("eosio.token");
      asset alice_EOS       = asset(1000000, symbol(4,"EOS"));  // 100.0000 EOS

      REQUIRE(init(exchange, false) == success());

      transfer(eosio_token, eosio_token, name("alice"), alice_EOS, "initial balance");
      REQUIRE(get_account_balance(eosio_token, name("alice"), "4,EOS") == alice_EOS);

      WHEN("alice deposits 5 EOS into the exchange") {
         uint64_t amount_alice = 50000;   // 5.0000 EOS
         asset alice_deposit_EOS = asset(amount_alice, symbol(4,"EOS"));

         auto r = transfer(eosio_token, alice, exchange, alice_deposit_EOS, "deposit");

         THEN("alices' EOS deposit will succeed") {
            CHECK(r == success());
            AND_WHEN("alice deposits 96 more EOS into the exchange") {
               amount_alice = 960000;   // 96.0000 EOS
               alice_deposit_EOS = asset(amount_alice, symbol(4,"EOS"));
               auto r = transfer(eosio_token, alice, exchange, alice_deposit_EOS, "deposit");

               THEN("alice will exceed her token account balance") {
                  CHECK(r == "assertion failure with message: overdrawn balance");
               }
            }
         }
      }

   }

   GIVEN("alice has 100 EOS her eosio.token account 100 BTC in her btc.token account and 0 EOS and 0 BTC her exchange account") {
      // tester accounts
      name alice            = name("alice");
      name eosio_token      = name("eosio.token");
      name btc_token        = name("btc.token");
      asset alice_EOS       = asset(    1000000, symbol(4,"EOS"));  // 100.0000 EOS
      asset alice_BTC       = asset(10000000000, symbol(8,"BTC"));  // 100.00000000 BTC

      REQUIRE(init(exchange, false) == success());

      transfer(eosio_token, eosio_token, name("alice"), alice_EOS, "initial balance");
      transfer(btc_token,     btc_token, name("alice"), alice_BTC, "initial balance");
      REQUIRE(get_account_balance(eosio_token, name("alice"), "4,EOS") == alice_EOS);
      REQUIRE(get_account_balance(btc_token,   name("alice"), "8,BTC") == alice_BTC);

      WHEN("alice deposits 5 EOS from her eosio.token account into the exchange") {
         uint64_t amount_alice = 50000;   // 5.0000 EOS
         asset alice_deposit_EOS = asset(amount_alice, symbol(4,"EOS"));

         auto r = transfer(eosio_token, alice, exchange, alice_deposit_EOS, "deposit");

         THEN("alices' EOS deposit will succeed") {
            CHECK(r == success());
         }
      }
      WHEN("alice deposits 5 BTC from her btc.token account into the exchange") {
         uint64_t amount_alice = 500000000;   // 5.00000000 BTC
         asset alice_deposit_BTC = asset(amount_alice, symbol(8,"BTC"));

         auto r = transfer(btc_token, alice, exchange, alice_deposit_BTC, "deposit");

         THEN("alices' BTC deposit will succeed") {
            CHECK(r == success());
         }
      }

   }

   GIVEN("alice has 100 EOS her eosio.token account 100 EOS in her eosio2.token account and 0 EOS her exchange account") {
      // tester accounts
      name alice            = name("alice");
      name eosio_token      = name("eosio.token");
      name eosio2_token     = name("eosio2.token");
      asset alice_EOS       = asset(1000000, symbol(4,"EOS"));  // 100.0000 EOS
      asset alice_EOS_2     = asset(1000000, symbol(4,"EOS"));  // 100.0000 EOS

      REQUIRE(init(exchange, false) == success());

      transfer(eosio_token, eosio_token, name("alice"), alice_EOS, "initial balance");
      transfer(eosio2_token, eosio2_token, name("alice"), alice_EOS_2, "initial balance");
      REQUIRE(get_account_balance(eosio_token, name("alice"), "4,EOS") == alice_EOS);
      REQUIRE(get_account_balance(eosio2_token, name("alice"), "4,EOS") == alice_EOS_2);

      WHEN("alice deposits 5 EOS from her eosio.token account into the exchange") {
         uint64_t amount_alice = 50000;   // 5.0000 EOS
         asset alice_deposit_EOS = asset(amount_alice, symbol(4,"EOS"));

         auto r = transfer(eosio_token, alice, exchange, alice_deposit_EOS, "deposit");

         THEN("alices' EOS deposit will succeed") {
            CHECK(r == success());

            AND_WHEN("alice deposits 5 EOS from her eosio2.token account into the exchange") {
               uint64_t amount_alice = 50000;   // 5.0000 EOS
               asset alice_deposit_EOS_2 = asset(amount_alice, symbol(4,"EOS"));

               auto r = transfer(eosio2_token, alice, exchange, alice_deposit_EOS_2, "deposit");

               THEN("alices' EOS deposit will succeed") {
                  CHECK(r == success());
               }
            }

         }
      }
   }

} FC_LOG_AND_RETHROW()


TEST_CASE_FIXTURE(eosio_system::exchange_tester, "withdraw") try {

   GIVEN("alice has 100 EOS in her exchange account") {
      name alice            = name("alice");
      name bob              = name("bob");
      name eosio_token      = name("eosio.token");
      asset alice_EOS       = asset(1000000, symbol(4,"EOS"));  // 100.0000 EOS

      REQUIRE(init(exchange, false) == success());

      // alice initial eosio.token balance
      transfer(eosio_token, eosio_token, name("alice"), alice_EOS, "initial balance");
      REQUIRE(get_account_balance(eosio_token, name("alice"), "4,EOS") == alice_EOS);

      // deposit EOS
      REQUIRE(transfer(eosio_token, alice, exchange, alice_EOS, "deposit") == success());

      WHEN("alice withdraws 5 EOS from her exchange account") {
         uint64_t withdraw_amount = 50000; // 5.0000 EOS
         asset amount_alice_EOS = asset(withdraw_amount, symbol(4,"EOS"));
         extended_asset alice_withdraw_EOS = extended_asset(amount_alice_EOS, eosio_token);

         auto r = withdraw(alice, alice, alice_withdraw_EOS);

         THEN("alice will successfully withdraw") {
               CHECK(r == success());

            AND_WHEN("alice withdraws 96 more EOS") {
               withdraw_amount = 960000; // 96.0000 EOS
               amount_alice_EOS = asset(withdraw_amount, symbol(4,"EOS"));
               alice_withdraw_EOS = extended_asset(amount_alice_EOS, eosio_token);

               auto r = withdraw(alice, alice, alice_withdraw_EOS);

               THEN("alices' withdraw will fail due to overdrawing from her exchange balance") {
                  CHECK(r == "assertion failure with message: exchange balance overdrawn");
               }
            }
         }
      }

      WHEN("alice withdraws more EOS than she has in her exchange account") {
         uint64_t withdraw_amount = 1000001; // 100.0001 EOS
         asset alice_withdraw_EOS = asset(withdraw_amount, symbol(4,"EOS"));
         auto r = withdraw(alice, alice, extended_asset(alice_withdraw_EOS, eosio_token));

         THEN("the transanction will fail because alice cannot withdraw a negative balance") {
            CHECK(r == "assertion failure with message: exchange balance overdrawn");
         }
      }

      WHEN("bob withdraws 5 EOS from alices exchange account") {
         uint64_t withdraw_amount = 50000; // 5.0000 EOS
         asset bob_withdraw_EOS = asset(withdraw_amount, symbol(4,"EOS"));
         auto r = withdraw(bob, alice, extended_asset(bob_withdraw_EOS, eosio_token));

         THEN("the transanction will fail because bob does not have permission to withdraw alices funds") {
            CHECK(r == "missing authority of alice");
         }
      }
   }

   GIVEN("alice has 100 BTC, btc.token in her exchange account") {
      name alice            = name("alice");
      name btc_token        = name("btc.token");
      asset alice_BTC       = asset(10000000000, symbol(8,"BTC"));  // 100.00000000 BTC

      REQUIRE(init(exchange, false) == success());

      // alice initial btc.token balance
      transfer(btc_token, btc_token, name("alice"), alice_BTC, "initial balance");
      REQUIRE(get_account_balance(btc_token, name("alice"), "8,BTC") == alice_BTC);

      // deposit BTC
      REQUIRE(transfer(btc_token, alice, exchange, alice_BTC, "deposit") == success());

      WHEN("alice withdraws 5 BTC from her exchange account") {
         uint64_t withdraw_amount = 500000000; // 5.00000000 BTC
         asset amount_alice_BTC = asset(withdraw_amount, symbol(8,"BTC"));
         extended_asset alice_withdraw_BTC = extended_asset(amount_alice_BTC, btc_token);

         auto r = withdraw(alice, alice, alice_withdraw_BTC);

         THEN("alice will successfully withdraw") {
               CHECK(r == success());
         }
      }
   }

} FC_LOG_AND_RETHROW()


TEST_CASE_FIXTURE(eosio_system::exchange_tester, "createmarket") try {

   GIVEN("no markets exist") {
      name alice              = name("alice");
      name eosio_token        = name("eosio.token");
      extended_asset EOS      = extended_asset(asset(0, symbol(4,"EOS")), eosio_token);

      REQUIRE(init(exchange, false) == success());

      WHEN("alice creates the EOS market") {
         auto r = createmarket(alice, exchange, EOS);

         THEN("EOS market will be successfully created") {
            CHECK(r == success());
         }
      }
   }

} FC_LOG_AND_RETHROW()


TEST_CASE_FIXTURE(eosio_system::exchange_tester, "removemarket") try {

   GIVEN("The EOS markets exist") {
      name alice              = name("alice");
      name eosio_token        = name("eosio.token");
      extended_asset EOS      = extended_asset(asset(0, symbol(4,"EOS")), eosio_token);

      REQUIRE(init(exchange, false) == success());
      REQUIRE(createmarket(alice, exchange, EOS) == success());

      WHEN("alice removes the EOS market") {
         auto r = removemarket(alice, EOS);

         THEN("EOS market will not be removed due to missing the exchanges authority") {
            CHECK(r == "missing authority of exchange");
         }
      }

      WHEN("the exchange removes the EOS market") {
         auto r = removemarket(name("exchange"), EOS);

         THEN("EOS market will be successfully removed") {
            CHECK(r == success());
         }
      }
   }

} FC_LOG_AND_RETHROW()


TEST_CASE_FIXTURE(eosio_system::exchange_tester, "addpair") try {

   GIVEN("BTC market exist") {
      name alice               = name("alice");
      name eosio_token         = name("eosio.token");
      name btc_token           = name("btc.token");
      extended_asset EOS       = extended_asset(asset(0, symbol(4,"EOS")), eosio_token);  // 0.0000 EOS
      extended_asset BTC       = extended_asset(asset(0, symbol(8,"BTC")), btc_token);    // 0.00000000 BTC

      REQUIRE(init(exchange, false) == success());
      REQUIRE(createmarket(alice, exchange, BTC) == success());

      WHEN("alice attempts to add the BTC pair to the EOS market") {
         auto r = addpair(exchange, alice, BTC, EOS);

         THEN("the action will succeed") {
            CHECK(r == success());
         }
      }
   }

} FC_LOG_AND_RETHROW()


TEST_CASE_FIXTURE(eosio_system::exchange_tester, "removepair") try {

   GIVEN("EOS/BTC market exist") {
      name alice               = name("alice");
      name eosio_token         = name("eosio.token");
      name btc_token           = name("btc.token");
      extended_asset EOS       = extended_asset(asset(0, symbol(4,"EOS")), eosio_token);  // 0.0000 EOS
      extended_asset BTC       = extended_asset(asset(0, symbol(8,"BTC")), btc_token);    // 0.00000000 BTC

      REQUIRE(init(exchange, false) == success());
      REQUIRE(createmarket(alice, exchange, BTC) == success());
      REQUIRE(addpair(exchange, alice, BTC, EOS) == success());

      WHEN("alice attempts to remove the EOS pair from the BTC market") {
         auto r = removepair(alice, BTC, EOS);

         THEN("her action will fail beacuase she does not have the authority to remove markets") {
            CHECK(r == "missing authority of exchange");
         }
      }

      WHEN("the exchange attempts to remove the EOS pair from the BTC market") {
         auto r = removepair(exchange, BTC, EOS);

         THEN("the action will succeed") {
            CHECK(r == success());
         }
      }
   }

} FC_LOG_AND_RETHROW()


TEST_CASE_FIXTURE(eosio_system::exchange_tester, "trade") try {

   GIVEN("EOS/BTC market exist") {
      name alice               = name("alice");
      name bob                 = name("bob");
      name eosio_token         = name("eosio.token");
      name btc_token           = name("btc.token");
      extended_asset EOS       = extended_asset(asset(0, symbol(4,"EOS")), eosio_token);  // 0.0000 EOS
      extended_asset BTC       = extended_asset(asset(0, symbol(8,"BTC")), btc_token);    // 0.00000000 BTC
      asset alice_EOS          = asset(10000000000, symbol(4,"EOS"));  // 100.00000000 EOS

      REQUIRE(init(exchange, false) == success());

      // alice initial eosio.token balance
      transfer(eosio_token, eosio_token, name("alice"), alice_EOS, "initial balance");
      transfer(eosio_token, name("alice"), name("exchange"), alice_EOS, "initial balance");
      // REQUIRE(get_exchange_account_balance(eosio_token, name("alice"), "4,EOS") == alice_EOS);

      REQUIRE(createmarket(alice, exchange, BTC) == success());
      REQUIRE(addpair(exchange, alice, BTC, EOS) == success());

      WHEN("alice places a BID to sell 1 EOS at price 0.5 BTC/EOS (1 EOS = 0.5 BTC)") {
         extended_asset price  = extended_asset(asset(50000000, symbol(8,"BTC")), btc_token); // 0.00000000 BTC
         extended_asset volume = extended_asset(asset(20000, symbol(4,"EOS")), eosio_token);  // 0.0000 EOS

         auto r = trade(alice, alice, 0, price, volume, 0);

         THEN("her action will succeed") {
            CHECK(r == success());
         }
      }

      WHEN("alice places a BID for bob to sell 1 EOS at price 0.5 BTC/EOS (1 EOS = 0.5 BTC)") {
         extended_asset price  = extended_asset(asset(50000000, symbol(8,"BTC")), btc_token); // 0.00000000 BTC
         extended_asset volume = extended_asset(asset(20000, symbol(4,"EOS")), eosio_token);  // 0.0000 EOS

         auto r = trade(alice, bob, 0, price, volume, 0);

         THEN("her action will fail due to missing bobs authority") {
            CHECK(r == "missing authority of bob");
         }
      }

   }

} FC_LOG_AND_RETHROW()


TEST_CASE_FIXTURE(eosio_system::exchange_tester, "cancel") try {

   GIVEN("EOS/BTC market does not exist") {
      name alice               = name("alice");
      name bob                 = name("bob");
      name eosio_token         = name("eosio.token");
      name btc_token           = name("btc.token");
      extended_asset EOS       = extended_asset(asset(0, symbol(4,"EOS")), eosio_token);  // 0.0000 EOS
      extended_asset BTC       = extended_asset(asset(0, symbol(8,"BTC")), btc_token);    // 0.00000000 BTC
      asset alice_EOS          = asset(10000000000, symbol(4,"EOS"));  // 100.00000000 EOS

      REQUIRE(init(exchange, false) == success());

      // alice initial eosio.token exchange balance
      transfer(eosio_token, eosio_token, name("alice"), alice_EOS, "initial balance");
      transfer(eosio_token, name("alice"), name("exchange"), alice_EOS, "initial balance");

      WHEN("alice cancels a non-existing EOS/BTC order") {
         auto r = cancel(alice, EOS, BTC, alice, 0, 0, 0);

         THEN("the cancel will assert with: market does not exist") {
            CHECK(r == "assertion failure with message: market does not exist");
         }
      }

      WHEN("the BTC market is created") {
         REQUIRE(createmarket(alice, exchange, BTC) == success());

         AND_WHEN("alice cancels a non-existing EOS/BTC order") {
            auto r = cancel(alice, EOS, BTC, alice, 0, 0, 0);

            THEN("the cancel will assert with: market pair does not exist") {
               CHECK(r == "assertion failure with message: market pair does not exist");
            }
         }

         AND_WHEN("EOS is paired with the BTC market") {
            REQUIRE(addpair(exchange, alice, BTC, EOS) == success());

            AND_WHEN("alice places a BID to sell 1 EOS at price 0.5 BTC/EOS (1 EOS = 0.5 BTC)") {
               extended_asset price  = extended_asset(asset(50000000, symbol(8,"BTC")), btc_token); // 0.00000000 BTC
               extended_asset volume = extended_asset(asset(20000, symbol(4,"EOS")), eosio_token);  // 0.0000 EOS

               CHECK(trade(alice, alice, 0, price, volume, 0) == success());

               AND_WHEN("alice cancels the order") {
                  auto r = cancel(alice, EOS, BTC, alice, 0, 0, 0);

                  THEN("the cancel will succeed") {
                     CHECK(r == success());
                  }
               }
               AND_WHEN("bob cancels the order") {
                  auto r = cancel(bob, EOS, BTC, alice, 0, 0, 0);

                  THEN("the cancel will fail due to missing alices authority") {
                     CHECK(r == "missing authority of alice");
                  }
               }
            }
         }
      }
   }

} FC_LOG_AND_RETHROW()
