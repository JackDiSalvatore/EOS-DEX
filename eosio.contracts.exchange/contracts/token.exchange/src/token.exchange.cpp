#include <token.exchange/token.exchange.hpp>

#include "token.exchange_base.cpp"

namespace tokenexchange {

   // ACTIONS

   void exchange::init( bool user_pays ) {
      require_auth( get_self() );
      init_contract( user_pays );
   }

   void exchange::withdraw( name from, extended_asset token ) {
      require_auth( from );
      extended_asset normalized_token = normalize_precision( token );

      check( normalized_token.quantity.is_valid(), "invalid quantity" );
      check( normalized_token.quantity.amount >= 0, "cannot withdraw negative balance" );
      adjust_balance( from, -normalized_token );

      action(
         permission_level{ get_self(), "active"_n },
         token.contract,
         "transfer"_n,
         std::make_tuple( get_self(), from, asset( token.quantity.amount, token.get_extended_symbol().get_symbol() ), std::string("withdraw") )
      ).send();
   }

   void exchange::transfer( name from, name to, asset quantity, string memo ) {
      if( to == get_self() ) {
         auto a = normalize_precision( extended_asset( quantity, get_first_receiver() ) );
         check( a.quantity.is_valid(), "invalid quantity in transfer" );
         check( a.quantity.amount > 0, "transfer quantity must be positive" );
         adjust_balance( from, a );
      }
   }

   void exchange::close( const name& owner, const name& contract_account, const symbol& sym ) {
      require_auth( owner );
      close_account( owner, contract_account, sym );
   }

   void exchange::createmarket( name owner, extended_asset quote ) {
      // allow anyone to create a new market
      create_market( owner, quote );
   }

   void exchange::removemarket( extended_asset quote ) {
      require_auth( get_self() );   // only contract account can remove markets
      remove_market( quote );
   }

   void exchange::addpair( name owner, extended_asset quote, extended_asset base ) {
      // allow anyone to add a new market pair
      name market_name = create_market_name( quote );
      add_market_pair( owner, market_name, base );
   }

   void exchange::removepair( extended_asset quote, extended_asset base ) {
      require_auth( get_self() );   // only contract account can remove market pairs
      remove_market_pair( base, quote );
   }

   void exchange::trade( name trader, bool order_type, extended_asset price, extended_asset volume, bool auto_withdraw ) {
      require_auth( trader );
      extended_asset return_asset;
      int64_t        return_amount;

      if ( order_type == BID ) {
         place_bid_order( trader, normalize_precision(price), normalize_precision(volume), current_time_point() );
      } else if ( order_type == ASK ) {
         place_ask_order( trader, normalize_precision(price), normalize_precision(volume), current_time_point() );
      }

      if ( auto_withdraw ) {
         perform_auto_withdraw( trader, order_type, price, volume );
      }
   }

   void exchange::cancel( extended_asset base, extended_asset quote, name trader, bool order_type, uint64_t id ) {
      require_auth( trader );

      cancel_order( base, quote, trader, order_type, id );
   }

   void exchange::perform_auto_withdraw( name trader, bool order_type, extended_asset return_bid, extended_asset return_ask ) {
      extended_asset return_asset;
      int64_t        return_amount;

      if ( order_type == BID ) {
         return_asset = return_bid;
      } else if ( order_type == ASK ) {
         return_asset = return_ask;
      }

      // find user balance
      exaccounts exchange_accounts( self, trader.value );
      auto exchange_accounts_by_balance = exchange_accounts.get_index<"bybalance"_n>();
      auto useraccount = exchange_accounts_by_balance.find( get_token_key( return_asset.contract, return_asset.quantity.symbol ) );

      if ( order_type == BID ) {
         return_amount = calculate_volume( normalize_precision(return_bid), normalize_precision(return_ask) ).quantity.amount;

         // denormalized token
         if ( return_asset.quantity.symbol.precision() < useraccount->balance.quantity.symbol.precision() )
            return_amount /= pow( 10, useraccount->balance.quantity.symbol.precision() - return_asset.quantity.symbol.precision() );

      } else if ( order_type == ASK ) {
         return_amount = return_ask.quantity.amount;
      }

      asset return_tokens = asset(
         return_amount,
         symbol( return_asset.quantity.symbol.code().to_string(), return_asset.quantity.symbol.precision() )
      );

      // withdraw to trader
      adjust_balance( trader, -normalize_precision( extended_asset(return_tokens, return_asset.contract) ) );
      action(
         permission_level{ get_self(), "active"_n },
         return_asset.contract,
         "transfer"_n,
         std::make_tuple( get_self(), trader, return_tokens, std::string("withdraw") )
      ).send();
   }

} /// namespace tokenexchange
