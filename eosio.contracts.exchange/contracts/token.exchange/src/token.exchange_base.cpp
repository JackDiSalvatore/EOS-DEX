#include <token.exchange/token.exchange_base.hpp>

namespace tokenexchange {

   /**
    *  Constructor initializes self to the operating account name
    *  and all of the exchange tables scoped to self.
    */
   exchange_base::exchange_base( name _self )
   : contract_config(_self, _self.value)
   , exchange_markets( _self, _self.value )
   , exchange_market_stats( _self, _self.value )
   , self( _self ) {}

   /**
    *  Returns an uin128_t table key for searching account balances derived
    *  from a symbol and contract account name.
    *
    *  Description:
    *  Generates aggregated key by combining the contract account name and
    *  token symbol.
    *
    *  contract_account - Name of the account running the token contract.
    *  sym - Asset symbol.
    *
    *  return - contract_account raw value appended with the sym raw value.
    */
   uint128_t get_token_key( name contract_account, symbol sym ){
      return ( uint128_t( contract_account.value ) << 64 ) | sym.code().raw();
   }

   /**
    *  Returns RAM payer.
    *
    *  Description:
    *  Returns RAM payer based on contract initialization.
    *
    *  owner - user to pay for RAM if configured for user payer
    *
    *  return - RAM payer account name
    */
   name exchange_base::get_ram_payer(name owner) {
      check( contract_config.exists(), "contract not initialized" );
      auto entry_stored = contract_config.get();

      return entry_stored.user_pays ? owner : self;
   }

   /**
    *  Returns an extended_asset with precision to 8 decimal places.
    *
    *  Description:
    *  Initializes RAM payer.  Either the contract or the end users will pay for RAM
    *
    *  user_pays - true if end user will pay for RAM
    *
    *  return - None
    */
   void exchange_base::init_contract( bool user_pays ) {
      if ( !contract_config.exists() ) {
         contract_config.get_or_create( self, config{ user_pays, true } );
         return;
      }

      auto entry_stored = contract_config.get();
      check( !entry_stored.is_initialized, "contract already initialized" );
      return;
   }

   /**
    *  Returns an extended_asset with precision to 8 decimal places.
    *
    *  Description:
    *  Exchange assets need to be converted to 8 decimals places in order to
    *  be traded against one another without loss due to rounding.
    *
    *  For example, if 1.5555 EOS where to be sold at 3.60 USD each, then a
    *  total of 5.5998 USD (1.5555 EOS * 3.60 USD/EOS) would be needed to
    *  completely fill the order.  (Note that the USD required is now priced
    *  to 4 decimal places.)  Converting all assets in the exchange to 8
    *  decimal places ensures that asset pricing remains compatible for every
    *  asset pair.  Any asset with greater than 8 decimal places will not be
    *  supported.
    *
    *  input_token - Any extended_asset with percision of 8 decimal places or less.
    *                ex: "10.12345 ABC"
    *
    *  return - Input token converted to 8 decimal places.
    *           ex: "10.12345 ABC" becomes "10.12345000 ABC"
    */
   extended_asset exchange_base::normalize_precision( extended_asset input_token ) {
      name contract_name = input_token.contract;
      string symbol_name = input_token.get_extended_symbol().get_symbol().code().to_string();

      int64_t input_token_precision = input_token.get_extended_symbol().get_symbol().precision();
      int64_t input_token_amount    = input_token.quantity.amount;

      check( input_token_precision <= 8, "only supports precision up to 8 decimals" );

      if( input_token_precision < 8 ) {
         // find precision difference
         int64_t precision_delta = 8 - input_token_precision;

         // normalized amount by delta
         int64_t normalized_amount = input_token_amount * pow( 10, precision_delta );

         return extended_asset( asset(normalized_amount, symbol(symbol_name,8)), contract_name );
      } else {
         return input_token;
      }
   }

   /**
    *  No return value.
    *
    *  Description:
    *  Emplaces a new exchange balance for users on first time deposit, and
    *  modifies or deletes a users exchange balance upon placing orders
    *  or withdrawal.
    *
    *  owner  - Account name for user the balance belongs to.
    *  delta - Incoming asset balance. Positive delta increases users balance,
    *          negative delta decreases users balance.
    *
    *  return - None.
    */
   void exchange_base::adjust_balance( name owner, extended_asset delta ) {
      exaccounts exchange_accounts( self, owner.value );
      auto exchange_accounts_by_balance = exchange_accounts.get_index<"bybalance"_n>();

      uint128_t key = get_token_key( delta.contract, delta.get_extended_symbol().get_symbol() );

      auto useraccount = exchange_accounts_by_balance.find( key );

      if( useraccount == exchange_accounts_by_balance.end() ) {
         check( delta.quantity.amount >= 0, "exchange balance overdrawn" );
         exchange_accounts.emplace( get_ram_payer(owner), [&]( auto& exa ) {
            exa.account_id = exchange_accounts.available_primary_key();
            exa.balance = delta;
         });
         return;
      }

      int64_t new_user_balance = useraccount->balance.quantity.amount + delta.quantity.amount;

      check( new_user_balance >= 0, "exchange balance overdrawn" );
         exchange_accounts_by_balance.modify( useraccount, same_payer, [&]( auto& exa ) {
         exa.balance += delta;
      });
   }

   /**
    *  No return value.
    *
    *  Description:
    *  Removes user balance table entry
    *
    *  owner            - Account whom owns the balance.
    *  contract_account - Token contract account.
    *  sym              - Token symbol.
    *
    *  return - None.
    */
   void exchange_base::close_account( const name& owner, const name& contract_account, const symbol& sym ) {
      exaccounts exchange_accounts( self, owner.value );
      auto exchange_accounts_by_balance = exchange_accounts.get_index<"bybalance"_n>();

      uint128_t key = get_token_key( contract_account, sym );
      auto useraccount = exchange_accounts_by_balance.find( key );

      check( useraccount != exchange_accounts_by_balance.end(), "balance row already deleted or never existed" );
      check( useraccount->balance.quantity.amount == 0, "cannot close because the balance is not zero" );

      exchange_accounts_by_balance.erase( useraccount );
   }

   /**
    *  Returns a name object for a given market.
    *
    *  Description:
    *  Creates a market name from asset symbol.
    *  ex: "eos" for "EOS" token
    *
    *  quote  - Quote asset.
    *
    *  return - An eosio name for the market pair.
    */
   name exchange_base::create_market_name( extended_asset quote ) {
      string quote_symbol  = quote.get_extended_symbol().get_symbol().code().to_string();

      // convert to lowercase
      transform( quote_symbol.begin(), quote_symbol.end(), quote_symbol.begin(), ::tolower );

      check( quote_symbol.size() <= 12,
             "combined symbol name exceeds maximum length of 12 characters" );

      return name( quote_symbol );
   }

   /**
    *  Returns a name object for a given market pair.
    *
    *  Description:
    *  Extracts a market name by appending the base asset symbol
    *  with the quote asset symbol.
    *
    *  base  - Base asset.
    *  quote - Quote asset.
    *
    *  return - An eosio name for the market pair.
    */
   name exchange_base::create_market_pair_name( extended_asset base, extended_asset quote ) {
      string base_symbol  = base.get_extended_symbol().get_symbol().code().to_string();
      string quote_symbol = quote.get_extended_symbol().get_symbol().code().to_string();

      // convert to lowercase
      transform( base_symbol.begin(), base_symbol.end(), base_symbol.begin(), ::tolower );
      transform( quote_symbol.begin(), quote_symbol.end(), quote_symbol.begin(), ::tolower );

      check( base_symbol.size() + quote_symbol.size() <= 12,
             "combined symbol name exceeds maximum length of 12 characters" );

      return name( base_symbol + quote_symbol );
   }

   /**
    *  No return value.
    *
    *  Description:
    *  Emplaces a new market table describing a market and its pair assets.
    *
    *  owner - RAM payer if end-user set to pay for RAM
    *  quote - Quote asset.
    *
    *  return - None.
    */
   void exchange_base::create_market( name owner, extended_asset quote ) {
      name market_name = create_market_name( quote );
      auto market = exchange_markets.find( market_name.value );
      check( market == exchange_markets.end(), "market already exists" );

      exchange_markets.emplace( get_ram_payer(owner), [&]( auto& m ) {
         m.market_name = market_name;
         m.quote       = quote;
      });
   }

   /**
    *  No return value.
    *
    *  Description:
    *  Deletes an existing market table.
    *
    *  quote - Quote asset.
    *
    *  return - None.
    */
   void exchange_base::remove_market( extended_asset quote ) {
      name market_name = create_market_name( quote );
      auto market = exchange_markets.find( market_name.value );
      check( market != exchange_markets.end(), "market does not exist" );
      check( market->bases.empty(), "remove market pairs before deleting" );

      exchange_markets.erase( market );
   }

   /**
    *  No return value.
    *
    *  Description:
    *  Adds a new market pair to an exchange market.
    *
    *  new_owner - RAM payer if end-user set to pay for RAM
    *  market_name - Market name for asset that pair will trade against.
    *  base        - Base asset.
    *
    *  return - None.
    */
   void exchange_base::add_market_pair( name new_owner, name market_name, extended_asset base ) {
      auto market = exchange_markets.find( market_name.value );
      check( market != exchange_markets.end(), "market does not exist" );
      check( market_name != create_market_name( base ), "cannot pair asset against itself" );

      check( market->bases.find( create_market_pair_name( base, market->quote ) ) == market->bases.end(),
             "market pair already exists" );

      exchange_markets.modify( market, get_ram_payer(new_owner), [&]( auto& m ) {
         m.bases.emplace( create_market_pair_name( base, m.quote ), base );

         check( exchange_market_stats.find( market_name.value ) == exchange_market_stats.end(), "market stats already exist" );

         exchange_market_stats.emplace( get_ram_payer(new_owner), [&]( auto& s ) {
            s.market_name = create_market_pair_name( base, m.quote );
            s.price = m.quote;
         });

      });
   }

   /**
    *  No return value.
    *
    *  Description:
    *  Removes an existing market pair.
    *
    *  base  - Base asset for market.
    *  quote - Quote asset to be unpaired from the market.
    *
    *  return - None.
    */
   void exchange_base::remove_market_pair( extended_asset base, extended_asset quote ) {
      auto market = exchange_markets.find( create_market_name( quote ).value );
      check( market != exchange_markets.end(), "market does not exist" );
      auto market_pair = market->bases.find( create_market_pair_name( base, quote ) );
      check( market_pair != market->bases.end(), "market pair does not exist" );

      exchange_markets.modify( market, same_payer, [&]( auto& m ) {
         m.bases.erase( market_pair );

         auto market_stats = exchange_market_stats.find( create_market_pair_name( base, quote ).value );
         exchange_market_stats.erase( market_stats );
      });
   }

   void exchange_base::check_sufficient_funds( name trader, extended_asset volume_requested ) {
      exaccounts exchange_accounts( self, trader.value );
      auto exaccounts = exchange_accounts.get_index<"bybalance"_n>();
      uint128_t key = get_token_key( volume_requested.contract, volume_requested.get_extended_symbol().get_symbol() );
      auto useraccount = exaccounts.find( key );

      int64_t user_balance_amount = useraccount->balance.quantity.amount;
      const char *error = ( "user does not have sufficient funds, only has "
         + std::to_string(user_balance_amount) + ", requires "
         + std::to_string(volume_requested.quantity.amount) ).c_str();

      check( user_balance_amount >= volume_requested.quantity.amount, error );
   }

   /**
    *  Returns the amount for tokens needed to place an ask order
    *
    *  Example:
    *  A ask for is request for buying 100 EOS at 3.50 USD per token.  To calculate the amount of
    *  USD needed to buy the entire order, perform the following math:
    *    volume_needed(USD) = price(USD/EOS) * volume(EOS)
    *
    *    volume_needed(USD) = 3.50000000 USD/EOS * 100.00000000 EOS
    *    volume_needed(USD) = 350.00000000 USD
    */
   extended_asset exchange_base::calculate_volume( extended_asset price, extended_asset volume ) {
      int64_t volume_total =  int128_t( price.quantity.amount ) * int128_t( volume.quantity.amount )
                              / pow( 10, price.get_extended_symbol().get_symbol().precision() );

      return extended_asset( asset( volume_total, price.get_extended_symbol().get_symbol() ), price.contract );
   }

   /**
    *  No return value.
    *
    *  Description:
    *  Cancels a traders order given the order type (BID/ASK)
    *  and trade ID.
    *
    *  market     - Market name where the order exists.
    *  trader     - Traders account name.
    *  order_type - Order type: BID or ASK.
    *  id         - trade ID.
    *
    *  return - None.
    */
   void exchange_base::cancel_order( extended_asset base, extended_asset quote, name trader, bool order_type, uint64_t id ) {
      // find market
      auto market = exchange_markets.find( create_market_name( quote ).value );
      check( market != exchange_markets.end(), "market does not exist" );
      auto market_pair = market->bases.find( create_market_pair_name( base, quote ) );
      check( market_pair != market->bases.end(), "market pair does not exist" );
      name market_pair_name = market_pair->first;

      // find order by id
      if( order_type == BID ) {
         bids bid_orders( self, market_pair_name.value );
         auto order = bid_orders.find( id );
         check(order != bid_orders.end(), "order does not exist");

         // refund trader
         adjust_balance( self, -order->volume );
         adjust_balance( trader, order->volume );

         // delete order
         bid_orders.erase( order );
      } else if ( order_type == ASK ) {
         asks ask_orders( self, market_pair_name.value );
         auto order = ask_orders.find( id );
         check(order != ask_orders.end(), "order does not exist");

         // refund trader
         adjust_balance( self, -calculate_volume(order->price, order->volume) );
         adjust_balance( trader, calculate_volume(order->price, order->volume) );

         // delete order
         ask_orders.erase( order );
      }
   }

   /**
    *  No return value.
    *
    *  Description:
    *  Places a BID order.
    *
    *  trader     - Traders account name.
    *  price      - Price trader is will to accept in quote asset.
    *  volume     - Amount to trade in base asset.
    *  time_stamp - Time trade action was executed.
    *  tx_id      - (Optional) Provided trade ID, used for testing.
    *
    *  return - None.
    */
   void exchange_base::place_bid_order( name trader, extended_asset price, extended_asset volume, time_point time_stamp, uint64_t tx_id = 0 ) {
      extended_asset bid_volume = volume;

      check_sufficient_funds( trader, bid_volume );

      auto market = exchange_markets.find( create_market_name( price ).value );
      auto market_pair = market->bases.find( create_market_pair_name( volume, price ) );
      check( market_pair != market->bases.end(), "market pair does not exist" );

      //place ask order in order book
      bids bid_orders( self, market_pair->first.value );
      bid_orders.emplace( get_ram_payer(trader), [&]( auto& a ) {
         a.id        = tx_id != 0 ? tx_id : bid_orders.available_primary_key();
         a.trader    = trader;
         a.timestamp = time_stamp;
         a.price     = price;
         a.volume    = volume;
      });

      adjust_balance( trader, -bid_volume );  // subtract from traders available balance
      adjust_balance( self, bid_volume );     // add to exchanges balance

      match_orders( market_pair->first );
   }

   /**
    *  No return value.
    *
    *  Description:
    *  Places an ASK order.
    *
    *  trader     - Traders account name.
    *  price      - Price trader is will to pay in quote asset.
    *  volume     - Amount to trade in base asset.
    *  time_stamp - Time trade action was executed.
    *  tx_id      - (Optional) Provided trade ID, used for testing.
    *
    *  return - None.
    */
   void exchange_base::place_ask_order( name trader, extended_asset price, extended_asset volume, time_point time_stamp, uint64_t tx_id = 0 ) {
      extended_asset ask_volume = calculate_volume( price, volume );

      check_sufficient_funds( trader, ask_volume );

      auto market = exchange_markets.find( create_market_name( price ).value );
      auto market_pair = market->bases.find( create_market_pair_name( volume, price ) );
      check( market_pair != market->bases.end(), "market pair does not exist" );

      //place ask order in order book
      asks ask_orders( self, market_pair->first.value );
      ask_orders.emplace( get_ram_payer(trader), [&]( auto& a ) {
         a.id        = tx_id != 0 ? tx_id : ask_orders.available_primary_key();
         a.trader    = trader;
         a.timestamp = time_stamp;
         a.price     = price;
         a.volume    = volume;
      });

      adjust_balance( trader, -ask_volume );  // subtract from traders available balance
      adjust_balance( self, ask_volume );     // add to exchanges balance

      match_orders( market_pair->first );
   }

   /**
    *  Returns the quote price that two asset pairs will be traded at.
    *
    *  Description:
    *  Determines trade price based on which order was placed first.
    *
    *  spread  - Order book spread (bid->price - ask->price).
    *  bid     - Pointer to bid entry.
    *  ask     - Pointer to ask entry.
    *
    *  return - An extended_asset defining the trade price.
    */
   template <typename T, typename F>
   extended_asset exchange_base::calculate_price( int64_t spread, T bid, F ask ) {

      if( spread == 0 ) {
         // price = best ask price (same as best bid price)
         return bid->price;
      } else if( spread < 0 ) {  // spread is overlapping in the orderbook
         // price = price of the earlier order submitted
         if( bid->timestamp < ask->timestamp )
            return bid->price;
         else
            return ask->price;
      } else {
         // DO NOT CALL WITH A POSITIVE SPREAD
         return extended_asset(
                  asset(0, symbol(bid->price.get_extended_symbol().get_symbol().code(),8)),
                  bid->price.contract
               );
      }
   }

   /**
    *  No return value.
    *
    *  Description:
    *  Continuous order matching algorithm.
    *
    *    while spread <= 0:
    *     1. Take the best ASK order and best BUY order and generate a trade with the following properties:
    *       volume traded = the minimum volume between both the best ASK and best BUY orders
    *       if spread = 0:
    *         price = best ASK price (same as best BUY price)
    *       if spread < 0 (i.e. overlapping in the orderbook):
    *         price = price of the earlier order submitted
    *     2. Update the orderbook:
    *       if best ASK volume == best BUY volume:
    *         remove best ASK and best BUY orders from order book
    *       else:
    *         remove the order with the minimum volume (either best ASK or best BUY) from the orderbook, and update the volume of the other order
    *
    *  market_name  - Market where order book exists.
    *
    *  return - None.
    */
   void exchange_base::match_orders( name market_name ) {
      extended_asset trade_price;
      extended_asset bid_volume;
      extended_asset ask_volume;
      extended_asset volume_offset;

      bids bid_orders( self, market_name.value );
      auto best_bids = bid_orders.get_index<"byprice"_n>();

      asks best_asks( self, market_name.value );

      // Find Lowest Bid (Sell) Order
      auto bid = best_bids.lower_bound( 0 );

      // Find Highest Ask (Buy) Order
      auto ask_itr = best_asks.begin();
      auto ask = ask_itr;
      int64_t highest_price = 0;

      while( ask_itr != best_asks.end() ) {
         if( highest_price < ask_itr->price.quantity.amount ) {
            highest_price = ask_itr->price.quantity.amount;
            ask = ask_itr;
         }
         ask_itr++;
      }

      if( bid != best_bids.end() && ask != best_asks.end() ) {
         int64_t spread = ( bid->price.quantity.amount - ask->price.quantity.amount );

         //  while spread <= 0:
         //  1. Take the best bid order and best ask order and generate a trade with the following properties:
         //      volume traded = the minimum volume between both the best bid and best ask orders
         //      if spread = 0:
         if( spread <= 0 ) {
            trade_price = calculate_price( spread, bid, ask );

            auto market_stats = exchange_market_stats.find( market_name.value );
            exchange_market_stats.modify( market_stats, same_payer, [&]( auto& s ) {
               s.price = extended_asset(
                  asset(
                     trade_price.quantity.amount / pow( 10, trade_price.quantity.symbol.precision() - s.price.quantity.symbol.precision() ),
                     symbol(s.price.get_extended_symbol().get_symbol().code(), s.price.quantity.symbol.precision())
                  ),
                  trade_price.contract
               );
            });

            //  2. Update the orderbook:
            //      if best bid volume == best ask volume:
            //          remove best bid and best ask orders from order book
            if( bid->volume == ask->volume ) {
               best_asks.erase( ask );
               best_bids.erase( bid );

               bid_volume = bid->volume;
               ask_volume = calculate_volume( trade_price, bid->volume);
            } else {
               // remove the order with the minimum volume (either best bid or best ask) from the orderbook
               // update the volume of the other order
               if( ask->volume < bid->volume ) { // bid is larger : update bid, remove ask
                  best_bids.modify( bid, same_payer, [&]( auto& b ) {
                     b.volume -= ask->volume;
                  });
                  best_asks.erase( ask );

                  bid_volume = ask->volume;
                  ask_volume = calculate_volume( trade_price, ask->volume );
               } else { // ask is larger: update ask, remove bid
                  best_asks.modify( ask, same_payer, [&]( auto& a ) {
                     a.volume -= bid->volume;
                  });
                  best_bids.erase( bid );

                  bid_volume = bid->volume;
                  ask_volume = calculate_volume( trade_price, bid->volume );
               }
            }

            if( trade_price < ask->price ) {
               volume_offset = calculate_volume( ask->price, bid_volume ) - calculate_volume( trade_price, bid_volume );
               // refund difference
               adjust_balance( self, -volume_offset );adjust_balance( ask->trader, volume_offset );
            }

            // send BID to ASK trader
            adjust_balance( self, -bid_volume );adjust_balance( ask->trader, bid_volume );
            // send ASK to BID trader
            adjust_balance( self, -ask_volume );adjust_balance( bid->trader, ask_volume );

            match_orders( market_name );
         }
      }
   }


} // namespace tokenexchange
