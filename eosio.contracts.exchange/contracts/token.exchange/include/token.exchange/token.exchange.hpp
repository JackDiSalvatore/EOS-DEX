#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <token.exchange/token.exchange_base.hpp>
#include <cmath>


namespace tokenexchange {

   using eosio::action;
   using eosio::current_time_point;
   using eosio::datastream;
   using eosio::extended_asset;
   using eosio::name;
   using eosio::permission_level;

   using std::string;

   /**
    *  This contract enables users to create an exchange between any pair of
    *  standard currency types.
    *
    *  Users must deposit funds into the exchange before they can trade on the
    *  exchange.
    */
   class [[eosio::contract("token.exchange")]] exchange : public eosio::contract, public exchange_base {
   public:
      exchange( name receiver, name code, datastream<const char*> ds )
      : contract( receiver, code, ds )
      , exchange_base( get_self() ) {}

      void perform_auto_withdraw( name trader, bool order_type, extended_asset return_bid, extended_asset return_ask );

      [[eosio::action]]
      void init( bool user_pays );

      [[eosio::action]]
      void withdraw( name  from, extended_asset token );

      [[eosio::on_notify("*::transfer")]]
      void transfer( name from, name to, asset quantity, string memo );

      [[eosio::action]]
      void close( const name& owner, const name& contract_account, const symbol& sym );

      [[eosio::action]]
      void createmarket( name owner, extended_asset quote );

      [[eosio::action]]
      void removemarket( extended_asset quote );

      [[eosio::action]]
      void addpair( name owner, extended_asset quote, extended_asset base );

      [[eosio::action]]
      void removepair( extended_asset quote, extended_asset base );

      [[eosio::action]]
      void trade( name trader, bool order_type, extended_asset price, extended_asset volume, bool auto_withdraw );

      [[eosio::action]]
      void cancel( extended_asset base, extended_asset quote, name trader, bool order_type, uint64_t id );
   };

} // namespace tokenexchange
