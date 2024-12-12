#pragma once

#ifndef SYSCONTATTRIBUTE
#define SYSCONTATTRIBUTE [[eosio::table, eosio::contract("token.exchange")]]
#endif

#ifndef SYSCON_TABLE
#define SYSCON_TABLE(name) [[eosio::table(name), eosio::contract("token.exchange")]]
#endif

#define BID 0
#define ASK 1

namespace tokenexchange {

   using eosio::asset;
   using eosio::extended_asset;
   using eosio::check;
   using eosio::const_mem_fun;
   using eosio::indexed_by;
   using eosio::name;
   using eosio::same_payer;
   using eosio::symbol;
   using eosio::time_point;

   using std::map;
   using std::string;

   uint128_t get_token_key( name contract_account, symbol sym );

   struct SYSCON_TABLE("config") config {
      bool user_pays;
      bool is_initialized;
   };

   typedef eosio::singleton<"config"_n, config> configuration;

   /**
    *  Each user has their own account with the exchange contract that keeps track
    *  of how much a user has on deposit for each extended asset type. The assumption
    *  is that storing a single flat map of all balances for a particular user will
    *  be more practical than breaking this down into a multi-index table sorted by
    *  the extended_symbol.  
    */
   struct SYSCONTATTRIBUTE exaccount {
      uint64_t        account_id;
      extended_asset  balance;

      uint64_t primary_key() const { return account_id; }
      uint128_t secondary_key() const {
         return get_token_key( balance.contract,
                                 balance.get_extended_symbol().get_symbol() );
      }
   };

   struct SYSCONTATTRIBUTE market {
      name           market_name;
      extended_asset quote;
      map<name, extended_asset> bases;

      uint64_t primary_key() const { return market_name.value; }
   };

   struct SYSCONTATTRIBUTE stat {
      name           market_name;
      extended_asset price;

      uint64_t primary_key() const { return market_name.value; }
   };

   struct SYSCONTATTRIBUTE order {
      uint64_t       id;
      name           trader;
      time_point     timestamp;
      extended_asset price;
      extended_asset volume;

      uint64_t primary_key() const { return id; }
      uint64_t  by_price() const { return price.quantity.amount; }
   };

   typedef eosio::multi_index<"exaccounts"_n, exaccount,
   indexed_by<"bybalance"_n, const_mem_fun<exaccount, uint128_t, &exaccount::secondary_key>>
   > exaccounts;
   typedef eosio::multi_index<"markets"_n, market> markets;
   typedef eosio::multi_index<"stats"_n, stat> stats;
   typedef eosio::multi_index<"bidorders"_n, order,
   indexed_by<"byprice"_n, const_mem_fun<order, uint64_t, &order::by_price>>
   > bids;
   typedef eosio::multi_index<"askorders"_n, order,
   indexed_by<"byprice"_n, const_mem_fun<order, uint64_t, &order::by_price>>
   > asks;

   struct exchange_base {
      // singletons
      configuration contract_config;

      // tables
      markets exchange_markets;
      stats   exchange_market_stats;

      name self;

      // constructor
      exchange_base( name _self );

      void init_contract( bool user_pays );
      name get_ram_payer(name owner);

      extended_asset normalize_precision( extended_asset token );
      void adjust_balance( name owner, extended_asset delta );
      void close_account( const name& owner, const name& contract_account, const symbol& sym );

      name create_market_name( extended_asset quote );
      name create_market_pair_name( extended_asset base, extended_asset quote );
      void create_market( name owner, extended_asset quote );
      void remove_market( extended_asset quote );
      void add_market_pair( name new_owner, name market_name, extended_asset base );

      void remove_market_pair( extended_asset base, extended_asset quote );
      void check_sufficient_funds( name trader, extended_asset volume_requested );
      extended_asset calculate_volume( extended_asset price, extended_asset volume );
      void cancel_order( extended_asset base, extended_asset quote, name trader, bool order_type, uint64_t id );
      void place_bid_order( name trader, extended_asset price, extended_asset volume, time_point time_stamp, uint64_t tx_id );
      void place_ask_order( name trader, extended_asset price, extended_asset volume, time_point time_stamp, uint64_t tx_id );

      template <typename T, typename F>
      extended_asset calculate_price( int64_t spread, T bid, F ask );

      void match_orders( name market_name );
   };

} // namespace tokenexchange
